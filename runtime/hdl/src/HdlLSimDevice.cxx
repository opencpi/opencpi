/*
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <inttypes.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>   // for mkdir and mkfifo
#include <signal.h>
#include <errno.h>
#include <queue>
#include <list>
#include <sys/wait.h>
#include "OcpiOsFileSystem.h"
#include "OcpiOsSemaphore.h"
#include "OcpiOsMisc.h"
#include "OcpiTransport.h"
#include "OcpiUtilAutoMutex.h"
#include "OcpiLibraryManager.h"
#include "HdlSdp.h"
#include "HdlLSimDriver.h"

namespace OCPI {
  namespace HDL {
    namespace LSim {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
      namespace OE = OCPI::OS::Ether;
      namespace OH = OCPI::HDL;
      namespace OL = OCPI::Library;
      namespace OX = OCPI::Util::EzXml;
      namespace OA = OCPI::API;
      namespace OT = OCPI::DataTransport;
      namespace DT = DataTransfer;
      
      // Our named pipes.
struct Fifo {
  std::string m_name;
  int m_rfd, m_wfd;
  Fifo(std::string strName, bool iRead, const char *old, std::string &error)
    : m_name(strName), m_rfd(-1), m_wfd(-1) { // , m_read(iRead) {
    if (error.size())
      return;
    const char *name = m_name.c_str();
    if (mkfifo(name, 0666))
      OU::format(error, "can't create fifo: %s (%s %d)", name, strerror(errno), errno);
    else if ((m_rfd = open(name, O_RDONLY | O_NONBLOCK)) < 0)
      OU::format(error, "can't open fifo %s for reading (%s %d)", name, strerror(errno), errno);
    else if (fcntl(m_rfd, F_SETFL, 0) != 0)
      OU::format(error, "can't set blocking flags on reading fifo %s (%s %d)", name,
		 strerror(errno), errno);
    else if ((m_wfd = open(name, O_WRONLY | O_NONBLOCK)) < 0)
      OU::format(error, "can't open fifo %s for writing (%s %d)", name, strerror(errno), errno);
    else if (fcntl(m_wfd, F_SETFL, 0) != 0)
      OU::format(error, "can't set blocking flags on writing fifo %s (%s %d)", name,
		 strerror(errno), errno);
    if (old) {
      unlink(old);
      if (symlink(name, old) != 0)
	OU::format(error, "can't symlink from %s to %s (%s %d)", old, name, strerror(errno), errno);
    }
    ocpiDebug("Fifo %s created and opened for %s", name, iRead ? "reading" : "writing");
  }
  void close() {
    if (m_rfd >= 0) {
      ::close(m_rfd);
      m_rfd = -1;
    }
    if (m_wfd >= 0) {
      ::close(m_wfd);
      m_wfd = -1;
    }
    unlink(m_name.c_str());
  }
  ~Fifo() {
    ocpiDebug("Destroying fifo %s", m_name.c_str());
    close();
  }
  // Get rid of any state in the pipe
  void
  flush() {
    ssize_t r;
    int n; // FIONREAD dictates int
    char buf[256];
    ocpiDebug("Starting to flush any state from previous simulation run for %s",
	      m_name.c_str());
    while (ioctl(m_rfd, FIONREAD, &n) >= 0 && n > 0 &&
	   ((r = read(m_rfd, buf, sizeof(buf))) > 0 ||
	    r < 0 && errno == EINTR))
      ;
    ocpiDebug("Ending flush of any state from previous simulation run");
  }
};

class Device 
  : public OH::Device, public OH::Accessor, DT::EndPoint::Receiver {
  friend class Driver;
  enum Action {
    SPIN_CREDIT = 0,
    DCP_CREDIT = 1, // with a two byte DWORD count
    DUMP_OFF = 253,
    DUMP_ON = 254,
    TERMINATE = 255
  };
  enum State {
    EMULATING,    // emulating a device, but nothing is loaded or running
    LOADING,      // starting up a simulation with loading without a client
    SERVING,      // serving a client, not yet fully connected, perhaps loading
    RUNNING,      // serving a client, simulator running
    CONNECTED,    // connected to a client and running
    DISCONNECTED, // running, but not connected to a client.
  };
  State m_state;
  // This fifo is read by the sim and we write it.
  // It carries CP traffic from us to the sim
  Fifo m_req;
  // This fifo is read by us, and we forward responses back to the relevant client
  // It carries CP traffic from the sim back to us, and then to the client via m_ext
  Fifo m_resp;
  // This fifo is read by the sim and we write it.
  // It carries sim controls from us to the sim.
  Fifo m_ctl;
  // This fifo is for control responses from the sim to us.
  // Currently this is simply an "ack" for spincredits.
  Fifo m_ack;
  int m_maxFd;
  fd_set m_alwaysSet;
  pid_t m_pid;
  bool m_exited;
  static bool s_stopped;
  uint64_t m_dcp;
  size_t m_respLeft;
  uint8_t m_admin[sizeof(OH::OccpAdminRegisters)];
  HdlUUID m_uuid;
  std::string m_simDir, m_platform, m_script, m_dir, m_app, m_file;
  static const unsigned RESP_MIN = 10;
  static const unsigned RESP_MAX = 14;
  static const unsigned RESP_LEN = 3; // index in layload buffer where length hides
  static const unsigned RESP_TAG = 7; // index in layload buffer where length hides
  // This structure is what we remember about a request: which socket and from which 
  // address.  This is to allow multiple clients to discover the same device, when
  // it is not attached to an SDP client.  Since SDP writes are posted, this really
  // only applies to read requests
  struct Request {
    OH::SDP::Header header;    // storage owned here
    uint8_t *data;             // data pointer associated with request
    OS::Semaphore sem;         // handshake with requester
    std::string error;
    Request(bool read, uint64_t address, size_t length, uint8_t *data)
      : header(read, address, length), data(data), sem(0) {
    }
  };
  std::queue<Request*, std::list<Request*> > m_respQueue;
  OS::Mutex m_sdpSendMutex; // mutex usage between control and data
  DT::EndPoint *m_endPoint;
  std::vector<DT::XferServices *> m_xferServices;
  std::string m_exec; // simulation executable local relative path name
  bool m_verbose, m_dump, m_spinning;
  unsigned m_sleepUsecs, m_simTicks;
  uint8_t m_spinCount;
  uint64_t m_cumTicks;
  OS::Timer m_spinTimer;
  OU::UuidString m_textUUID;
  char *m_metadata;
  ezxml_t m_xml;
  bool m_firstRun; // First time running in the container thread.
  uint64_t m_lastTicks;
  uint8_t m_sdpDataBuf[SDP::Header::max_message_bytes];

protected:
  // name - should indicate something, but locally scopped...
  // platform - which simulator translated adds _pf... to Sim
  // spincount - 8 bits of spin cycles - Sim
  // sleepusecs - Sim
  // simticks - Sim
  // dump - Sim
  // verbose
  /*
    Server:
  */
  Device(const std::string &name, const std::string &simDir, const std::string &platform,
	 const std::string &script, uint8_t spinCount, unsigned sleepUsecs,
	 unsigned simTicks, bool verbose, bool dump, std::string &error)
    : OH::Device("lsim:" + name, "ocpi-socket-rdma"),
      m_state(EMULATING),
      m_req(simDir + "/request", false, NULL, error),
      m_resp(simDir + "/response", true, NULL, error),
      m_ctl(simDir + "/control", false, NULL, error),
      m_ack(simDir + "/ack", false, NULL, error),
      m_maxFd(-1), m_pid(0), m_exited(false), m_dcp(0), m_respLeft(0), m_simDir(simDir),
      m_platform(platform), m_script(script), m_endPoint(NULL), m_verbose(verbose), m_dump(dump),
      m_spinning(false), m_sleepUsecs(sleepUsecs), m_simTicks(simTicks), m_spinCount(spinCount),
      m_cumTicks(0), m_metadata(NULL), m_xml(NULL), m_firstRun(true), m_lastTicks(0) {
    if (error.length())
      return;
    FD_ZERO(&m_alwaysSet);
    addFd(m_resp.m_rfd, true);
    addFd(m_ack.m_rfd, false);
    ocpiDebug("resp %d ack %d nfds %d", m_resp.m_rfd, m_ack.m_rfd, m_maxFd);
    initAdmin(*(OH::OccpAdminRegisters *)m_admin, m_platform.c_str(), m_uuid, &m_textUUID);
    if (verbose) {
      fprintf(stderr, "Simulation HDL device %s for %s (UUID %s)\n",
	      m_name.c_str(), m_platform.c_str(), uuid());
      fflush(stderr);
    }
    if (error.empty()) {
      m_endpointSpecific = "ocpi-socket-rdma";
      m_endpointSize = OH::SDP::Header::max_addressable_bytes * OH::SDP::Header::max_nodes;
      cAccess().setAccess(NULL, this, OCPI_UTRUNCATE(RegisterOffset, 0));
      // data offset is after the first node
      dAccess().setAccess(NULL, this, OCPI_UTRUNCATE(RegisterOffset,
						     OH::SDP::Header::max_addressable_bytes));
      init(error);
    }
  }
  // Our added-value wait-for-process call.
  // If "hang", we wait for the process to end, and if it stops, we term+kill it.
  // Return true on bad unexpected things
  bool
  mywait(pid_t pid, bool hang, std::string &error) {
    int status;
    pid_t wpid;
    do
      wpid = waitpid(pid, &status, (hang ? 0 : WNOHANG) | WUNTRACED);
    while (wpid == -1 && errno == EINTR);
    if (wpid == 0) // can't happen if hanging
      ;//    ocpiDebug("Wait returned 0 - subprocess running");
    else if ((int)wpid == -1) {
      if (errno == ECHILD)
	OU::format(error, "simulator failed to start: look in sim.out");
      else
	OU::format(error, "waitpid error %s (%d)", strerror(errno), errno);
      return true;
    } else if (WIFEXITED(status)) {
      int exitStatus = WEXITSTATUS(status);
      if (exitStatus > 10)
	OU::format(error,
		   "Simulation subprocess couldn't execute from simulator executable \"%s\" (got %s - %d)",
		   m_exec.c_str(), strerror(exitStatus - 10), exitStatus - 10);
      else if (exitStatus)
	OU::format(error, "Simulation subprocess for executable \"%s\" terminated with exit status %d",
		   m_exec.c_str(), exitStatus);
      else
	ocpiInfo("Simulation subprocess exited normally");
      if (hang) {
	// If waiting for termination, its not our error
	ocpiInfo("%s", error.c_str());
	error.clear();
      } else
	return true;
    } else if (WIFSIGNALED(status)) {
      int termSig = WTERMSIG(status);
      OU::format(error, "Simulation subprocess for executable \"%s\" terminated with signal %s (%d)",
		 m_exec.c_str(), strsignal(termSig), termSig);
      if (hang) {
	// If waiting for termination, its not our error
	ocpiInfo("%s", error.c_str());
	error.clear();
      } else
	return true;
    } else if (WIFSTOPPED(status)) {
      int stopSig = WSTOPSIG(status);
      ocpiInfo("Simulator subprocess for executable \"%s\" stopped with signal %s (%d)",
	       m_exec.c_str(), strsignal(stopSig), stopSig);
      if (hang) {
	kill(pid, SIGTERM);
	sleep(1);
	kill(pid, SIGKILL);
	return mywait(pid, true, error);
      }
    }
    return false;
  }
  // Start the executable, which means start up the simulator.
  bool
  start(std::string &err) {
    std::string cmd;
    std::string dir = m_simDir[0] == '/' ? m_simDir : "../../";

    std::string cwd = OS::FileSystem::cwd();
    OU::format(cmd,
	       "D=\"%s\" && exec %s %s "
	       "\"sw2sim=${D}%s\" \"sim2sw=${D}%s\" \"ctl=${D}%s\" \"ack=${D}%s\" \"cwd=%s\"",
	       dir.c_str(), m_script.c_str(), m_file.c_str(), m_req.m_name.c_str(),
	       m_resp.m_name.c_str(), m_ctl.m_name.c_str(), m_ack.m_name.c_str(),
	       cwd.c_str());
    if (m_dump)
      cmd += " bscvcd";
    if (m_verbose)
      fprintf(stderr, "Starting execution of simulator for HDL assembly: %s "
	      "(executable \"%s\", dir \"%s\" pwd \"%s\").\n", m_app.c_str(), m_exec.c_str(),
	      m_dir.c_str(), getenv("PWD"));
    switch ((m_pid = fork())) {
    case 0:
      setpgid(0, 0);
      if (chdir(m_dir.c_str()) != 0) {
	std::string x("Cannot change to simulation subdirectory: ");
	int e = errno;
	x += m_dir;
	x += "\n";
	write(2, x.c_str(), x.length());
	_exit(10 + e);
      }
      {
	int fd = creat("sim.out", 0666);
	if (fd < 0) {
	  std::string x("Error: Cannot create sim.out file for simulation output.\n");
	  int e = errno;
	  write(2, x.c_str(), x.length());
	  _exit(10 + e);
	}
	if (dup2(fd, 1) < 0 ||
	    dup2(fd, 2) < 0)
	  _exit(10 + errno);
	assert(fd > 2);
	if (execl("/bin/sh", "/bin/sh", "--noprofile", "-c", cmd.c_str(), NULL))
	  _exit(10 + errno);
      }
      break; // not used.
    case -1:
      OU::format(err, "Could not create simulator sub-process for: %s", m_exec.c_str());
      return true;
    default:
      ocpiInfo("Simluator subprocess has pid: %u.", m_pid);
    }
    if (m_verbose)
      fprintf(stderr, "Simulator process (process id %u) started, with its output in %s/sim.out\n",
	      m_pid, m_dir.c_str());
#if 0
    uint8_t msg[2];
    msg[0] = m_dump ? DUMP_ON : DUMP_OFF;
    msg[1] = 0;
    ocpiCheck(write(m_ctl.m_wfd, msg, 2) == 2);
#endif
    // Improve the odds of an immediate error giving a good error message by letting the sim run
    ocpiInfo("Waiting for simulator to start before issueing any more credits.");
    OS::sleep(100);
    for (unsigned n = 0; n < 1; n++)
      if (spin(err) || mywait(m_pid, false, err) || ack(err))
	return true;
    if (m_verbose)
      fprintf(stderr, "Simulator process is running.\n");
    err.clear();
    return false;
  }
  // Flush all comms to the sim process since we have a new client.
  // Called from destructor too
  void
  flush() {
    ocpiDebug("Flushing all session state");
    m_req.flush();  // FIXME: could this steal partial requests and get things out of sync?
    m_resp.flush(); // FIXME: should we wait for the request fifo to clear?
    m_exec.clear();
    while (!m_respQueue.empty())
      m_respQueue.pop();
  }
  void
  shutdown() {
    if (m_pid) {
      uint8_t msg[2];
      std::string error;
      msg[0] = TERMINATE;
      msg[1] = 0;
      ocpiInfo("Telling the simulator process (%u) to exit", m_pid);
      ocpiCheck(write(m_ctl.m_wfd, msg, 2) == 2);
      ocpiInfo("Waiting for simulator process to exit");
      mywait(m_pid, true, error);
      if (killpg(m_pid, SIGTERM) == 0) {
	sleep(1);
	killpg(m_pid, SIGKILL);
      }
      m_ctl.flush();
      if (error.size())
	ocpiBad("Error when shutting down simulator: %s", error.c_str());
      m_dcp = 0;
      m_pid = 0;
    }
    flush();
  }
  bool
  spin(std::string &error) {
    if (!m_spinning) {
      uint8_t msg[2];
      msg[0] = SPIN_CREDIT;
      msg[1] = m_spinCount;
      ssize_t w = write(m_ctl.m_wfd, msg, 2);
      if (w != 2) {
	OU::format(error, "spin control write to sim failed. w %zd", w);
	return true;
      }
      ocpiDebug("Sent spin for %u", m_spinCount);
      m_cumTicks += m_spinCount;
      m_spinTimer.restart();
      m_spinning = true;
    }
    return false;
  }
  bool
  ack(std::string &error) {
    char c;
    ssize_t r = read(m_ack.m_rfd, &c, 1);
    if (r != 1 || c != '1') {
      OU::format(error, "ack read from sim failed. r %zd c %u", r, c);
      return true;
    }
    ocpiDebug("Sim sent ACK");
    m_spinning = false;
    return false;
  }
  bool
  sendCredit(size_t credit, std::string &error) {
    assert(!(credit & 3));
    credit >>= 2;
    uint8_t msg[3];
    msg[0] = DCP_CREDIT;
    msg[1] = OCPI_UTRUNCATE(uint8_t, credit & 0xff);
    msg[2] = OCPI_UTRUNCATE(uint8_t, credit >> 8);
    if (write(m_ctl.m_wfd, msg, 3) != 3) {
      error = "write error to control fifo";
      return true;
    }
    if (!m_dcp && spin(error))
      return true;
    m_dcp += credit;
    ocpiDebug("Sent credit: %zu", credit);
    return false;
  }
// Perform one action, waiting for any of:
// 1. Data from sim to forward back to client
// 2. Data from client to either act on as server or forward to sim.
// 3. Ack from sim to release more spin credits
// 4. Timeout meaning sim is taking a long time to go through spin credits
// set error on fatal problems
// return any execution spin credits provided.
  bool
  doit(std::string &error) {
    if (m_pid && mywait(m_pid, false, error)) {
      if (error.empty()) {
	m_exited = true;
	killpg(m_pid, SIGKILL);
	m_pid = 0;
      }
      return true;
    }
    fd_set fds[1];
    *fds = m_alwaysSet;
    if (m_dcp)                    // only do this after SOME control op
      FD_SET(m_ack.m_rfd, fds);   // spin credit ACKS from sim
    struct timeval timeout[1];
    timeout[0].tv_sec = m_sleepUsecs / 1000000;
    timeout[0].tv_usec = m_sleepUsecs % 1000000;
    errno = 0;
    switch (select(m_maxFd + 1, fds, NULL, NULL, timeout)) {
    case 0: // timeout.   Someday accumulate this time and assume sim is hung/crashes
      printTime("select timeout");
      return false;
    case -1:
      if (errno == EINTR)
	return false;
      OU::format(error, "Select failed: %s %u", strerror(errno), errno);
      return true;
    default:
      ;
    }
    // Top priority is getting responses from the sim back to clients
    // Next priority is to process messages from clients.
    // Especially, if they are CP requests, we want to send DCP credits
    // before spin credits, so that the CP info is read before spin credits
    if (FD_ISSET(m_resp.m_rfd, fds) && doResponse(error))
      return true;
    // Next is to keep sim running by providing more credits
    // We will only enable this fd when there is no response queue
    if (FD_ISSET(m_ack.m_rfd, fds)) {
      printTime("Received ACK indication");
      if (ack(error) || spin(error))
	return true;
    }
    return false;
  }
  void
  printTime(const char *msg) {
    OS::ElapsedTime et = m_spinTimer.getElapsed();
    ocpiDebug("When %s time since spin is: %" PRIu32 ".%03" PRIu32 " s ago", msg,
	      et.seconds(), (et.nanoseconds() + 500000) / 1000000);
  }
#define myassert(cond) do if (!(cond)) { terminate(); assert(cond); } while (0)
  // This is the way control plane responses come back.
  // It is also the way data plane output request (writes) come out of the sim
  bool
  doResponse(std::string &error) {
    ocpiDebug("doResponse: sim is producing SDP output");
    if (m_respLeft == 0) {
      OH::SDP::Header h;
      bool request;
      size_t length;
      if (h.getHeader(m_resp.m_rfd, request, length, error))
	return true;
      if (request) {
	// A DMA write request, with "length" being the data payload
	myassert(h.get_op() == OH::SDP::Header::WriteOp);
	uint64_t whole_addr = h.getWholeByteAddress();
	uint16_t mbox = OCPI_UTRUNCATE(uint16_t, whole_addr >> 32);
	whole_addr &= ~(-(uint64_t)1 << 32);
	myassert(mbox < m_xferServices.size() || m_xferServices[mbox]);
	myassert(length <= SDP::Header::max_message_bytes);
	if (SDP::read(m_resp.m_rfd, m_sdpDataBuf, length, error))
	  return true;
	myassert(mbox < m_xferServices.size());
	myassert(m_xferServices[mbox]);
	m_xferServices[mbox]->send(OCPI_UTRUNCATE(DtOsDataTypes::Offset, whole_addr),
				   m_sdpDataBuf, length);
      } else {
	myassert(!m_respQueue.empty());
	Request &request = *m_respQueue.front();
	if (request.header.endRequest(h, m_resp.m_rfd, request.data, error))
	  return true;
	request.sem.post();
	m_respQueue.pop();
      }
      return false;
    }
#if 0
    // Read dataplane data and pass it to the right endpount

    ssize_t nread = 0, room = sizeof(m_fromSdpBuf);
    assert(!(room&3));
    char *cp = m_fromSdpBuf;
    do {
      ssize_t n = read(m_resp.m_rfd, cp, room);
      if (n == 0) {
	OU::format(error, "SDP channel from simulator got EOF");
	return true;
      }
      if (n < 0)
	if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
	  n = 0;
	else {
	  OU::format(error, "SDP channel from simulator broken: %s (%zd %d)",
		     strerror(errno), n, errno);
	  return true;
	}
      nread += n;
      cp += n;
      room -= n;
    } while (nread == 0 || nread & 3);
    std::string resp;
    OU::format(resp, "Response from SDP: %zu bytes in %zu byte buffer: ",
	       nread, sizeof(m_fromSdpBuf));
    for (ssize_t nw = 0; nw < nread; nw += 4)
      OU::formatAdd(resp, " %08x", ((uint32_t*)m_fromSdpBuf)[nw/4]);
    ocpiDebug("%s", resp.c_str()); // overloaded version for std::string?
    



    try {
      m_sdpSckt->send(m_fromSdpBuf, nread);
    } catch (std::string &s) {
      error = s;
      return true;
    }
#endif
    return false;
  }

  static void w2(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    static char buf[1000];
    vsprintf(buf, fmt, ap);
    write(2, buf, strlen(buf));
  }
  void terminate() {
    uint8_t msg[2];
    msg[0] = TERMINATE;
    msg[1] = 0;
    w2("Telling the simulator process (pid %u) to exit", m_pid);
    write(m_ctl.m_wfd, msg, 2);
  }

  // FIXME: for object scope
  static Device *s_one;
  static void
  sigint(int /* signal */) {
    if (s_one && s_one->m_pid) {
      w2("\nInterrupt: Simulator process pid %u running.\n", s_one->m_pid);
      s_one->terminate();
      if (s_stopped) {
	w2("\nSimulator process pid %u still running.\n", s_one->m_pid);
	signal(SIGINT, SIG_DFL);
      }
      s_stopped = true;
    } else
      _exit(1);
  }
public:
  ~Device() {
    shutdown();
    if (s_one == this)
      s_one = NULL;
    ocpiDebug("Simulation server %s destruction", m_name.c_str());
    if (m_simDir.length()) {
      m_req.close();
      m_resp.close();
      m_ctl.close();
      m_ack.close();
      ocpiDebug("Removing sim directory: %s", m_simDir.c_str());
      std::string cmd;
      OU::format(cmd, "rm -r -f %s > /dev/null 2>&1", m_simDir.c_str());
      int r;
      for (unsigned n = 0; n < 5; n++) {
	if ((r = system(cmd.c_str())) == 0)
	  break;
	ocpiInfo("Cannot remove the simulation directory: %s. Retrying...", m_simDir.c_str());
	sleep(1);
      }
      if (r)
	ocpiBad("Could not remove the simulation directory: %s", m_simDir.c_str());
    }
  }
  bool needThread() const {
    return true;
  }
  // The container background thread calls this.  Return true when done
  bool run() {
    if (m_firstRun) {
      ocpiCheck(signal(SIGINT, sigint) != SIG_ERR);
      assert(m_state == RUNNING);
      m_lastTicks = 0;
    }
    std::string error;
    if (!m_exited && !s_stopped && m_cumTicks < m_simTicks) {
      if (m_cumTicks - m_lastTicks > 1000) {
	ocpiDebug("Spin credit at: %20" PRIu64, m_cumTicks);
	m_lastTicks = m_cumTicks;
      }
      if (!doit(error))
	return false;
    }
    ocpiDebug("exit simulator container thread x %d s %d ct %" PRIu64 " st %u e '%s'",
	      m_exited, s_stopped, m_cumTicks, m_simTicks, error.c_str());
    if (m_exited) {
      if (m_verbose)
	fprintf(stderr, "Simulator \"%s\" exited normally\n", m_name.c_str());
      ocpiInfo("Simulator \"%s\" exited normally", m_name.c_str());
    } else if (s_stopped) {
      if (m_verbose)
	fprintf(stderr, "Stopping simulator \"%s\" due to signal\n", m_name.c_str());
      ocpiInfo("Stopping simulator \"%s\" due to signal", m_name.c_str());
    } else if (m_cumTicks >= m_simTicks) {
      if (m_verbose)
	fprintf(stderr, "Simulator \"%s\" credits at %" PRIu64 " exceeded %u, stopping simulation\n",
		m_name.c_str(), m_cumTicks, m_simTicks);
      ocpiInfo("Simulator \"%s\" credits at %" PRIu64 " exceeded %u, stopping simulation",
	       m_name.c_str(), m_cumTicks, m_simTicks);
    } else
      ocpiInfo("Simulator \"%s\" shutting down. Error: %s",
	       m_name.c_str(), error.empty() ? "none" : error.c_str());
    shutdown();
    return true;
  }
private:
  const char *uuid() const {
    return m_textUUID;
  }
  void addFd(int fd, bool always) {
    if (fd > m_maxFd)
      m_maxFd = fd;
    if (always)
      FD_SET(fd, &m_alwaysSet);
  }
  void
  initAdmin(OH::OccpAdminRegisters &admin, const char *platform, HdlUUID &hdlUuid,
	    OU::UuidString *uuidString) {
    memset(&admin, 0, sizeof(admin));
#define unconst32(a) (*(uint32_t *)&(a))
#define unconst64(a) (*(uint64_t *)&(a))
    unconst64(admin.magic) = OCCP_MAGIC;
    unconst32(admin.revision) = 0;
    unconst32(admin.birthday) = OCPI_UTRUNCATE(uint32_t, time(0));
    unconst32(admin.pciDevice) = 0;
    unconst32(admin.attention) = 0;
    unconst32(admin.status) = 0;
    admin.scratch20 = 0xf00dface;
    admin.scratch24 = 0xdeadbeef;
    admin.control = 0;
    unconst32(admin.reset) = 0;
    unconst32(admin.timeStatus) = 0;
    admin.timeControl = 0;
    admin.time = 0;
    admin.timeDelta = 0;
    unconst32(admin.timeClksPerPps) = 0;
    unconst64(admin.present) = 1;
    unconst64(admin.attention) = 1;
    unconst32(admin.numRegions) = 1;
    unconst32(admin.regions[0]) = 0;
    OU::Uuid uuid;
    OU::generateUuid(uuid);
    if (uuidString) {
      OU::uuid2string(uuid, *uuidString);
      ocpiDebug("Emulator UUID: %s", *uuidString);
    }
    OH::HdlUUID temp;
    temp.birthday = OCPI_UTRUNCATE(uint32_t, time(0) + 1);
    memcpy(temp.uuid, uuid, sizeof(uuid));
    strcpy(temp.platform, platform);
    strcpy(temp.device, "devemu");
    strcpy(temp.load, "ld");
    strcpy(temp.dna, "\001\002\003\004\005\006\007");
    for (unsigned n = 0; n < sizeof(OH::HdlUUID); n++)
      ((uint8_t *)&hdlUuid)[n] = ((uint8_t *)&temp)[(n & ~3) + (3 - (n & 3))];
  }
  // Convenience for setting the m_isAlive and throwing OU::Error
  void throwit(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    m_isAlive = false;
    va_list ap;
    va_start(ap, fmt);
    throw OU::Error(ap, fmt);
  }
  void
  setState(State state) {
    ocpiDebug("Server state set to %u", state);
    m_state = state;
  }
public:
  // After loading or after deciding that the loaded bitstream is ok, this is called.
  void
  connect() {
    std::string err;
    if (start(err))
      throwit("Can't start \"%s\" simulator from \"%s\": %s",
	      m_platform.c_str(), m_exec.c_str(), err.c_str());
  }

  void load(const char *file) {
    assert(m_state != LOADING || m_state != SERVING);
    if (m_state != EMULATING) {
      ocpiInfo("Shutting down previous simulation before (re)loading");
      shutdown();
    }
    if (m_verbose)
      fprintf(stderr, "Initializing %s simulator from executable/bitstream: %s\n",
	      m_platform.c_str(), file);
    ocpiDebug("Starting to load bitstream file for %s: %s", m_platform.c_str(), file);
    // First establish a directory for the simulation based on the name of the file
    const char *slash = strrchr(file, '/');
    slash = slash ? slash + 1 : file;
    const char *suff = strstr(slash, m_platform.c_str());
    if (suff) {
      if (*--suff != '_' && *suff != '-')
	throwit("simulator file name %s is not formatted properly", file);
      const char *dot = strchr(suff + 1, '.');
      if (!dot)
	throwit("simulator file name %s is not formatted properly", file);
      m_file.assign(slash, dot - slash);
      m_app.assign(slash, suff - slash);
    } else
      m_file = m_app = slash;
    char date[100];
    time_t now = time(NULL);
    struct tm nowtm;
    localtime_r(&now, &nowtm);
    strftime(date, sizeof(date), ".%Y%m%d%H%M%S", &nowtm);
    OU::format(m_dir, "%.*s/%s.%s%s", (int)(strchr(m_simDir.c_str(), '/') - m_simDir.c_str()),
	       m_simDir.c_str(), m_app.c_str(), m_platform.c_str(), date);
    m_exec = file;
    ocpiDebug("Sim executable is %s(%s), assy is %s, platform is %s dir is %s",
	      file, m_file.c_str(), m_app.c_str(), m_platform.c_str(), m_dir.c_str());
    if (mkdir(m_dir.c_str(), 0777) != 0 && errno != EEXIST)
      throwit("Can't create directory \"%s\" to run \"%s\" simulation from \"%s\" (%s)",
	      m_dir.c_str(), m_platform.c_str(), m_exec.c_str(), strerror(errno));
    try {
      std::time_t mtime;
      uint64_t length;
      m_metadata = OL::Artifact::getMetadata(m_exec.c_str(), mtime, length);
    } catch (std::string &s) {
      throwit("When processing simulation executable file '%s': %s", m_exec.c_str(), s.c_str());
    }
    const char *e = OX::ezxml_parse_str(m_metadata, strlen(m_metadata), m_xml);
    if (e)
      throwit("invalid metadata in binary/artifact file \"%s\": %s", m_exec.c_str(), e);
    char *xname = ezxml_name(m_xml);
    if (!xname || strcasecmp("artifact", xname))
      throwit("invalid metadata in binary/artifact file \"%s\": no <artifact/>", m_exec.c_str());
    std::string platform;
    if ((e = OX::getRequiredString(m_xml, platform, "platform", "artifact")))
      throwit("invalid metadata in binary/artifact file \"%s\": %s", m_exec.c_str(), e);
    if (!strcmp("_pf", platform.c_str() + platform.length() - 3))
      platform.resize(platform.length() - 3);
    if (platform != m_platform)
      throwit("simulator platform mismatch:  executable (%s) has '%s', we are '%s'",
	      m_exec.c_str(), platform.c_str(), m_platform.c_str());
    std::string uuid;
    e = OX::getRequiredString(m_xml, uuid, "uuid", "artifact");
    if (e)
      throwit("invalid metadata in binary/artifact file \"%s\": %s", m_exec.c_str(), e);
    ocpiInfo("Bitstream %s has uuid %s", m_exec.c_str(), uuid.c_str());
    std::string untar;
    OU::format(untar,
	       "set -e; file=%s; "
	       "xlength=`tail -1 $file | sed 's/^.*X//'` && "
	       "trimlength=$(($xlength + ${#xlength} + 2)) && "
	       "head -c -$trimlength < $file | tar -xzf - -C %s",
	       m_exec.c_str(), m_dir.c_str());
    ocpiDebug("Executing command to load bit stream for sim: %s", untar.c_str());
    int rc = system(untar.c_str());
    switch (rc) {
    case 127:
      throwit("Couldn't start execution of command: %s", untar.c_str());
    case -1:
      throwit("System error (%s, errno %d) while executing bitstream loading command",
	      strerror(errno), errno);
    default:
      throwit("Error return %u while executing bitstream loading command", rc);
    case 0:
      if (m_verbose)
	fprintf(stderr, "Executable/bitstream is installed and ready, in directory %s.\n",
		m_dir.c_str());
      ocpiInfo("Successfully loaded bitstream file: \"%s\" for simulation", m_exec.c_str());
    }
    setState(SERVING);   // Were loaded or loading
    std::string err;
    if (start(err))
      throwit("Can't start \"%s\" simulator from \"%s\": %s",
	      m_platform.c_str(), m_exec.c_str(), err.c_str());
    setState(RUNNING);   // Were loaded or loading
  }
  void
  send2sdp(SDP::Header &h, uint8_t *data, const char *type, std::string &error) {
    size_t rlen;
    bool bad;
    {
      OU::AutoMutex m(m_sdpSendMutex);
      bad = h.startRequest(m_req.m_wfd, data, rlen, error);
    }
    if (bad || sendCredit(rlen, error)) {
      m_isFailed = true;
      throw OU::Error("HDL Sim SDP %s error: %s", type, error.c_str());
    }
  }
  void
  sdpRequest(bool read, uint64_t offset, size_t length, uint8_t *data,
	     uint32_t *status) {
    if (m_isFailed)
      throw OU::Error("HDL::Sim::Device::request after previous failure");
    if (m_state == EMULATING) {
      uint8_t *eaddr;
      if (offset <= sizeof(m_admin))
	eaddr = &m_admin[offset];
      else if (offset >= offsetof(OccpSpace, config))
	if ((offset - offsetof(OccpSpace, config)) >= sizeof(m_uuid) + sizeof(uint64_t))
	  throwit("Read/write offset out of range1: 0x%" PRIx64, offset);
	else {
	  offset -= offsetof(OccpSpace, config);
	  eaddr = (uint8_t*)&m_uuid + offset;
	}
      else if (offset >= offsetof(OccpSpace, worker)) {
	if ((offset - offsetof(OccpSpace, worker)) >= sizeof(OccpWorker))
	  throwit("Read/write offset out of range2: 0x%" PRIx64, offset);
	else {
	  offset -= offsetof(OccpSpace, worker);
	  static uint32_t success = OCCP_SUCCESS_RESULT, zero = 0, junk;
	  switch (offset) {
	  case offsetof(OccpWorkerRegisters, initialize):
	  case offsetof(OccpWorkerRegisters, start):
	    assert(read);
	    eaddr = (uint8_t *)&success;
	    break;
	  case offsetof(OccpWorkerRegisters, control):
	  case offsetof(OccpWorkerRegisters, clearError):
	  case offsetof(OccpWorkerRegisters, window):
	  case offsetof(OccpWorkerRegisters, status):
	  default:
	    eaddr = (uint8_t *)&(read ? zero : junk); // no errors
	  }
	}
      } else
	throwit("Read offset out of range3: 0x%" PRIx64, offset);
      if (read)
	memcpy(data, eaddr, length);
      else
	memcpy(eaddr, data, length);
      return;
    }
    Request r(read, offset, length, data);
    if (read)
      m_respQueue.push(&r);
    send2sdp(r.header, data, "control", r.error);
    if (read) {
      r.sem.wait();
      if (r.error.length()) {
	if (status)
	  *status = OCCP_STATUS_READ_ERROR;
	else {
	  m_isFailed = true;
	  throw OU::Error("HDL Sim SDP control error: %s", r.error.c_str());
	}
      } else if (status)
	*status = 0;
    }
  }
  uint32_t
  get(RegisterOffset offset, size_t bytes, uint32_t *status) {
    ocpiDebug("SDP Accessor read for offset 0x%zx of %zu bytes", offset, bytes);
    uint32_t data;
    sdpRequest(true, offset, bytes, (uint8_t*)&data, status);
    ocpiDebug("SDP Accessor read received 0x%x (%d) from offset %zx", data, data, offset);
    return data;
  }
  void 
  set(RegisterOffset offset, size_t bytes, uint32_t data, uint32_t *status) {
    ocpiDebug("SDP Accessor write 0x%x (%d) for offset 0x%zx of %zu bytes", data, data, offset, bytes);
    sdpRequest(false, offset, bytes, (uint8_t *)&data, status);
    ocpiDebug("SDP Accessor write from offset %zx complete", offset);
  }
  uint64_t
  get64(RegisterOffset offset, uint32_t *status) {
    ocpiDebug("SDP Accessor read64 for offset 0x%zx", offset);
    uint64_t data;
    sdpRequest(true, offset, sizeof(uint64_t), (uint8_t*)&data, status);
    ocpiDebug("SDP Accessor read received 0x%" PRIx64 " (%" PRId64 ") from offset %zx", data,
	      data, offset);
    return data;
  }
  void
  getBytes(RegisterOffset offset, uint8_t *buf, size_t length, size_t elementBytes,
	   uint32_t *status, bool string) {
    ocpiDebug("Accessor read %zu bytes for offset 0x%zx", length, offset);
    for (size_t bytes; length; length -= bytes, buf += bytes, offset += bytes) {
      bytes = offset & 7 || length < 8 ? (offset & 3 || length < 4 ?
					  (offset & 1 || length < 2 ? 1 : 2) : 4) : 8;
      if (bytes > elementBytes)
	bytes = elementBytes;
      if (offset & (elementBytes - 1))
	bytes = std::min(elementBytes - (offset & (elementBytes - 1)), bytes);
      sdpRequest(true, offset, bytes, buf, status);
      if (string && strnlen((char *)buf, bytes) < bytes)
	break;
    }
    ocpiDebug("Accessor read %zu bytes complete", length);
  }
  void
  set64(RegisterOffset offset, uint64_t val, uint32_t *status) {
    ocpiDebug("SDP Accessor write64 for offset 0x%zx data %" PRIx64 " (%" PRId64 ")",
	      offset, val, val);
    assert(!(offset & 7));
    sdpRequest(false, offset, sizeof(uint64_t), (uint8_t*)&val, status);
    ocpiDebug("SDP Accessor write64 from offset %zx complete", offset);
  }
  void
  setBytes(RegisterOffset offset, const uint8_t *buf, size_t length,
	   size_t elementBytes, uint32_t *status) {
    ocpiDebug("SDP Accessor write %zu bytes to offset 0x%zx size %zu",
	      length, offset, elementBytes);
    for (size_t bytes; length; length -= bytes, buf += bytes, offset += bytes) {
      bytes = offset & 7 || length < 8 ? (offset & 3 || length < 4 ?
					  (offset & 1 || length < 2 ? 1 : 2) : 4) : 8;
      if (bytes > elementBytes)
	bytes = elementBytes;
      if (offset & (elementBytes - 1))
	bytes = std::min(elementBytes - (offset & (elementBytes - 1)), bytes);
      sdpRequest(false, offset, bytes, (uint8_t *)buf, status);
    }
    ocpiDebug("SDP Accessor write to offset %zx complete", offset);
  }
  void
  unload() {
    throw "Can't unload bitstreams for simulated devices yet";
  }
  void receive(DtOsDataTypes::Offset offset, uint8_t *data, size_t count) {
    SDP::Header h(false, offset, count);
    std::string error;
    send2sdp(h, data, "data", error);
  }
  DT::EndPoint &getEndPoint() {
    if (m_endPoint)
      return *m_endPoint;
    // Create a local endpoint that is specialized to call back the container
    // when data arrives for the endpount rather than allocating a large internal buffer.
    // Size comes from the properties of the SDP worker.
    DT::EndPoint &ep =
      DT::getManager().allocateProxyEndPoint(m_endpointSpecific.c_str(),
						       m_endpointSize);
    // This is the hook that allows us to receive data/metadata/flags pushed to this
    // endpoint from elsewhere - from multiple other endpoints.
    ep.setReceiver(*this);
    m_xferServices.resize(ep.maxCount, 0);
    m_endPoint = &ep;
    return ep;
  }
  // FIXME: the "other" argument should be an endpoint, but that isn't easy now
  void connect(DT::EndPoint &ep, OCPI::RDT::Descriptors &mine,
	       const OCPI::RDT::Descriptors &other) {
    assert(m_endPoint);
    assert(&ep == m_endPoint);
    DT::EndPoint *otherEp = m_endPoint->factory->findEndPoint(other.desc.oob.oep);
    assert(otherEp);
    assert(otherEp->mailbox < m_xferServices.size());
    DT::XferServices *s = DT::XferFactoryManager::getSingleton().getService(m_endPoint, otherEp);
    assert(m_xferServices[otherEp->mailbox] == NULL || m_xferServices[otherEp->mailbox] == s);
    if (m_xferServices[otherEp->mailbox] == NULL)
      m_xferServices[otherEp->mailbox] = s;
    OT::Transport::fillDescriptorFromEndPoint(ep, mine);
  }
};
Device *Device::s_one = NULL;
bool Device::s_stopped = false;

Driver::
~Driver() {
}

// We don't find anything automatically
unsigned Driver::
search(const OU::PValue */*params*/, const char **/*exclude*/, bool /*discoveryOnly*/,
       std::string &/*error*/) {
  return 0;
}

OH::Device *Driver::
open(const char *name, bool discovery, const OA::PValue *params, std::string &err) {
  assert(!discovery);
  const char *cp;
  for (cp = name; *cp && !isdigit(*cp); cp++)
    ;
  std::string platform;
  platform.assign(name, cp - name);
  bool verbose = false;
  OU::findBool(params, "verbose", verbose);
  const char *dir = "simulations";
  OU::findString(params, "directory", dir);
  return createDevice(name, platform, 20, 200000, 10000000, verbose, false, dir, err);
}

Device *Driver::
createDevice(const std::string &name, const std::string &platform, uint8_t spinCount,
	     unsigned sleepUsecs, unsigned simTicks, bool verbose, bool dump,
	     const char *dir, std::string &error) {
  std::string actualPlatform, script;
  const char *xenv = getenv("OCPI_CDK_DIR");
  if (!xenv) {
    error = "The OCPI_CDK_DIR environment variable is not set";
    return NULL;
  }
  script = xenv;
  script += "/scripts/";
  if (platform.empty()) {
    OS::FileIterator fi(script, "runSimExec.*");
    if (fi.end()) {
      OU::format(error, "There is no supported simulation platform (no %s/runSimExec.*)",
		 script.c_str());
      return NULL;
    }
    std::string cmd = fi.relativeName();
    const char *cp = strchr(cmd.c_str(), '.');
    assert(cp);
    actualPlatform = ++cp;
    script += cmd;
  } else {
    size_t len = platform.length();
    actualPlatform.assign(platform.c_str(),
			  !strcmp("_pf", platform.c_str() + len - 3) ? len - 3 : len);
    script += "runSimExec.";
    script += actualPlatform;
    if (!OS::FileSystem::exists(script)) {
      OU::format(error, "\"%s\" is not a supported simulation platform (no %s)",
		 platform.c_str(), script.c_str());
      return NULL;
    }
  }
  std::string simDir;
  pid_t pid = getpid();
  // We do not clean this up - it is created on demand.
  // FIXME: do we need this at all?
  if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
    OU::format(error, "Can't create directory for simulations: %s", dir);
    return NULL;
  }
  std::string simName;
  if (name.length())
    simName = name;
  else
    OU::format(simName, "%s.%u", actualPlatform.c_str(), pid);
  OU::format(simDir, "%s/%s.%u", dir, simName.c_str(), getpid());
  if (mkdir(simDir.c_str(), 0777) != 0) {
    if (errno == EEXIST)
      OU::format(error,
		 "Directory for this simulator, \"%s\", already exists (/tmp not cleared?)",
		 simDir.c_str());
    else
      OU::format(error, "Can't create the new diretory for this simulator: %s",
		 simDir.c_str());
    return NULL;
  }
  Device *d = new Device(name, simDir, actualPlatform, script, spinCount, sleepUsecs,
			 simTicks, verbose, dump, error);
  if (error.empty()) {
    Device::s_one = d;
    return d;
  }
  delete d;
  return NULL;
}


    } // namespace LSim
  } // namespace HDL
} // namespace OCPI
