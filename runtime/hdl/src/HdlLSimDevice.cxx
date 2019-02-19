/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>   // for mkdir and mkfifo
#include <signal.h>
#include <errno.h>
#include <queue>
#include <list>
#include <set>
#include <sys/wait.h>
#include "OcpiOsFileSystem.h"
#include "OcpiOsSemaphore.h"
#include "OcpiOsMisc.h"
#include "OcpiUtilAutoMutex.h"
#include "XferManager.h"
#include "OcpiTransport.h"
#include "LibrarySimple.h"
#include "HdlSdp.h"
#include "HdlLSimDriver.h"
#include "HdlDriver.h"
#include "HdlContainer.h"

namespace OCPI {
  namespace HDL {
    namespace LSim {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
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
  bool m_read;
  Fifo(std::string strName, bool iRead) : m_name(strName), m_rfd(-1), m_wfd(-1), m_read(iRead) {
  }
  // Open is separate from construction so we can create the "device", but not create
  // any files or directories until we actually use it.
  bool
  open(std::string &error) {
    assert(m_rfd == -1 && m_wfd == -1);
    const char *name = m_name.c_str();
    if (::mkfifo(name, 0666))
      OU::format(error, "can't create fifo: %s (%s %d)", name, strerror(errno), errno);
    else if ((m_rfd = ::open(name, O_RDONLY | O_NONBLOCK)) < 0)
      OU::format(error, "can't open fifo %s for reading (%s %d)", name, strerror(errno), errno);
    else if (::fcntl(m_rfd, F_SETFL, 0) != 0)
      OU::format(error, "can't set blocking flags on reading fifo %s (%s %d)", name,
			 strerror(errno), errno);
    else if ((m_wfd = ::open(name, O_WRONLY | O_NONBLOCK)) < 0)
      OU::format(error, "can't open fifo %s for writing (%s %d)", name, strerror(errno), errno);
    else if (::fcntl(m_wfd, F_SETFL, 0) != 0)
      OU::format(error, "can't set blocking flags on writing fifo %s (%s %d)", name,
		 strerror(errno), errno);
    else {
      ocpiDebug("Fifo %s created and opened for %s", name, m_read ? "reading" : "writing");
      return false;
    }
    return true;
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
	    (r < 0 && errno == EINTR)))
      ;
    ocpiDebug("Ending flush of any state from previous simulation run");
  }
};

class Device
  : public OH::Device, public OH::Accessor, DT::EndPoint::Receiver,
    virtual public OCPI::Util::SelfMutex
 {
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
  bool m_stopped;
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
    Request(bool read, uint64_t address, size_t length, uint8_t *a_data)
      : header(read, address, length), data(a_data), sem(0) {
    }
  };
  std::queue<Request*, std::list<Request*> > m_respQueue;
  OS::Mutex m_sdpSendMutex; // mutex usage between control and data
  typedef std::vector<DT::XferServices *> XferServices;
  XferServices m_writeServices;
  XferServices m_readServices;
  std::string m_exec; // simulation executable local relative path name
  bool m_dump, m_spinning;
  unsigned m_sleepUsecs, m_simTicks;
  uint8_t m_spinCount;
  uint64_t m_cumTicks;
  OS::Timer m_spinTimer;
  OU::UuidString m_textUUID;
  //  char *m_metadata;
  //  ezxml_t m_xml;
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
  Device(const std::string &a_name, const std::string &simDir, const std::string &a_platform,
	 const std::string &script, uint8_t spinCount, unsigned sleepUsecs,
	 unsigned simTicks, const OU::PValue *params, bool dump, std::string &error)
    : OH::Device("lsim:" + a_name, "ocpi-socket-rdma", params),
      m_state(EMULATING),
      m_req(simDir + "/request", false),
      m_resp(simDir + "/response", true),
      m_ctl(simDir + "/control", false),
      m_ack(simDir + "/ack", false),
      m_maxFd(-1), m_pid(0), m_exited(false), m_stopped(false), m_dcp(0), m_respLeft(0),
      m_simDir(simDir), m_platform(a_platform), m_script(script), m_dump(dump), m_spinning(false),
      m_sleepUsecs(sleepUsecs), m_simTicks(simTicks), m_spinCount(spinCount),
      m_cumTicks(0), /* m_metadata(NULL), m_xml(NULL),*/ m_firstRun(true), m_lastTicks(0) {
    if (error.length())
      return;
    FD_ZERO(&m_alwaysSet);
    initAdmin(*(OH::OccpAdminRegisters *)m_admin, m_platform.c_str(), m_uuid, &m_textUUID);
    if (m_verbose) {
      fprintf(stderr, "Simulation HDL device %s for %s (UUID %s, dir %s, ticks %u)\n",
	      m_name.c_str(), m_platform.c_str(), uuid(), simDir.c_str(), simTicks);
      fflush(stderr);
    }
    m_endpointSpecific = "ocpi-socket-rdma";
    m_endpointSize = OH::SDP::Header::max_addressable_bytes * OH::SDP::Header::max_nodes;
    cAccess().setAccess(NULL, this, OCPI_UTRUNCATE(RegisterOffset, 0));
    // data offset overlays the control plane since it is SDP node 0.
    dAccess().setAccess(NULL, this, OCPI_UTRUNCATE(RegisterOffset, 0));
    //						   OH::SDP::Header::max_addressable_bytes));
    init(error);
    // Note we are emulating here, still not creating any file system dirs or fifos
  }
  bool
  initFifos(std::string &error) {
    // We do not clean this up - it is created on demand.
    const char *slash = strrchr(m_simDir.c_str(), '/');
    assert(slash);
    size_t len = slash - m_simDir.c_str();
    std::string parent;
    parent.assign(m_simDir.c_str(), len);
    // This is created on demand and not removed.
    if (mkdir(parent.c_str(), 0777) != 0 && errno != EEXIST)
      return OU::eformat(error, "Can't create directory for simulations: \"%s\"", parent.c_str());
    if (mkdir(m_simDir.c_str(), 0777) != 0)
      return errno == EEXIST ?
	OU::eformat(error, "Directory for this simulation, \"%s\", already exists",
		    m_simDir.c_str()) :
	OU::eformat(error, "Can't create the new directory for this simulation: %s: %s (%d)",
		    m_simDir.c_str(), strerror(errno), errno);
    if (m_req.open(error) || m_resp.open(error) || m_ctl.open(error) || m_ack.open(error)) {
      ocpiDebug("initFifos failed for simulator: %s : %s", m_name.c_str(), error.c_str());
      return true;
    }
    ocpiDebug("initFifos resp %d ack %d nfds %d", m_resp.m_rfd, m_ack.m_rfd, m_maxFd);
    addFd(m_resp.m_rfd, true);
    addFd(m_ack.m_rfd, false);
    return false;
  }
  // We assume all sim platforms use SDP
  // The sdp sender (output) must push for now
  // The sdp receiver can push or pull and uses flagismeta
  uint32_t
  dmaOptions(ezxml_t /*icImplXml*/, ezxml_t /*icInstXml*/, bool isProvider) {
    const char *e = getenv("OCPI_HDL_FORCE_SIM_DMA_PULL");
    return isProvider ?
      (e && !strcmp(e, "1") ? 0 : 1 << OCPI::RDT::ActiveFlowControl) |
      (1 << OCPI::RDT::ActiveMessage) | (1 << OCPI::RDT::FlagIsMeta) :
      1 << OCPI::RDT::ActiveMessage  | (1 << OCPI::RDT::FlagIsMetaOptional) ;
  }
  // Our added-value wait-for-process call.
  // If "hang", we wait for the process to end, and if it stops, we term+kill it.
  // Return true on bad unexpected things
  bool
  mywait(bool hang, std::string &error) {
    int status;
    pid_t wpid;
    assert(m_pid);
    do
      wpid = waitpid(m_pid, &status, (hang ? 0 : WNOHANG) | WUNTRACED);
    while (wpid == -1 && errno == EINTR);
    if (wpid == 0) // can't happen if hanging
      ;//    ocpiDebug("Wait returned 0 - subprocess running");
    else if ((int)wpid == -1) {
      if (errno == ECHILD)
	OU::format(error, "simulator failed to start: look in %s/sim.out", m_dir.c_str());
      else
	OU::format(error, "unexpected waitpid error %s (%d)", strerror(errno), errno);
      killpg(m_pid, SIGKILL); // probably nothing there, but just in case
      m_pid = 0;
      return true;
    } else if (WIFEXITED(status)) {
      assert(m_pid == wpid);
      killpg(m_pid, SIGKILL); // in case any orphaned children
      m_pid = 0;
      int exitStatus = WEXITSTATUS(status);
      if (exitStatus > 10)
	OU::format(error,
		   "Simulation subprocess couldn't execute from simulator executable \"%s\" (got %s - %d)",
		   m_exec.c_str(), strerror(exitStatus - 10), exitStatus - 10);
      else if (exitStatus)
	OU::format(error, "Simulation subprocess for executable \"%s\" terminated with exit status %d: check %s/sim.out",
		   m_exec.c_str(), exitStatus, m_dir.c_str());
      else {
	ocpiInfo("Simulation subprocess exited normally.  Hang %u", hang);
	m_exited = true;
      }
      if (hang) {
	// If waiting for termination, its not our error
	if (error.length()) {
	  ocpiInfo("%s", error.c_str());
	  error.clear();
	}
      } else
	return true;
    } else if (WIFSIGNALED(status)) {
      killpg(m_pid, SIGKILL); // in case any orphaned children
      m_pid = 0;
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
	killpg(m_pid, SIGTERM);
	sleep(1);
	killpg(m_pid, SIGKILL);
	return mywait(true, error);
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
	       "D=\"%s\" && exec %s %s %s "
	       "\"sw2sim=${D}%s\" \"sim2sw=${D}%s\" \"ctl=${D}%s\" \"ack=${D}%s\" \"cwd=%s\"",
	       dir.c_str(), m_script.c_str(), OS::logGetLevel() >= 8 ? "-v" : "", m_file.c_str(),
	       m_req.m_name.c_str(), m_resp.m_name.c_str(), m_ctl.m_name.c_str(),
	       m_ack.m_name.c_str(), cwd.c_str());
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
	(void)write(2, x.c_str(), x.length());
	_exit(10 + e);
      }
      {
	int fd = creat("sim.out", 0666);
	if (fd < 0) {
	  std::string x("Error: Cannot create sim.out file for simulation output.\n");
	  int e = errno;
	  (void)write(2, x.c_str(), x.length());
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
      ocpiInfo("Simulator subprocess has pid: %u.", m_pid);
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
    ocpiInfo("Waiting for simulator to start before issuing any more credits.");
    OS::sleep(100);
    ocpiCheck(signal(SIGINT, sigint) != SIG_ERR);
    for (unsigned n = 0; n < 1; n++)
      if (spin(err) || mywait(false, err) || ack(err))
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
    if (m_verbose)
      fprintf(stderr, "Simulator device flushing all session state\n");
    m_req.flush();  // FIXME: could this steal partial requests and get things out of sync?
    m_resp.flush(); // FIXME: should we wait for the request fifo to clear?
    m_exec.clear();
    while (!m_respQueue.empty())
      m_respQueue.pop();
    m_spinning = false;
  }
  void
  shutdown() {
    ocpiInfo("HDL Simulator Shutdown.  Current pid: %u", m_pid);
    OU::SelfAutoMutex guard (this);
    // Disable container thread
    if (m_state == EMULATING)
      return;
    setState(EMULATING);
    m_firstRun = true;
    if (m_pid) {
      uint8_t msg[2];
      std::string error;
      msg[0] = TERMINATE;
      msg[1] = 0;
      ocpiInfo("Telling the simulator process (%u) to stop", m_pid);
      ocpiCheck(write(m_ctl.m_wfd, msg, 2) == 2);
      ocpiInfo("Waiting for simulator process to exit");
      mywait(true, error);
      if (m_pid && killpg(m_pid, SIGTERM) == 0) {
	sleep(1);
	killpg(m_pid, SIGKILL);
      }
      m_ctl.flush();
      if (error.size())
	ocpiBad("Error when shutting down simulator: %s", error.c_str());
      m_dcp = 0;
      m_pid = 0;
    }
    m_exited = false;
    flush();
    m_isAlive = false;
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
  // Read a single character '1' from the sim process
  //   Return failure (true) if sim process exits
  //   Use select (with timeout) to probe for readiness
  //   On select timeout, try again
  bool
  ack(std::string &error) {
    // setup file descriptors for select
    fd_set fds;
    const int timeoutUsecs = 30000000; // number of micro seconds to wait
    struct timeval timeout; // set the timeout for the timed read
    timeout.tv_sec = timeoutUsecs / 1000000;
    timeout.tv_usec =  timeoutUsecs % 1000000;
    // Continue to try select/read until we read 1 byte, a non-eintr error occurs or sim process exits
    while (true) {
      FD_ZERO(&fds);
      FD_SET(m_ack.m_rfd, &fds);
      struct timeval tmpTimeout = timeout; // cleared by select attempt
      if (mywait(false, error))
        return true; // process ended
      switch (::select(m_ack.m_rfd + 1, &fds, NULL, NULL, &tmpTimeout)) {
        case 0:
          continue; // timeout try again
        case 1: { // fd ready for read
          char c;
          switch (::read(m_ack.m_rfd, &c, 1)) {
            case 1: // one byte read as expected
              if (c != '1')
                return OU::eformat(error, "Unexpected: ack read from sim failed. c %d", c);
              ocpiDebug("Sim sent ACK. Tick count at %" PRIu64, m_cumTicks);
              m_spinning = false;
              return false; // successful ack read
            case -1:
              if (errno == EINTR) // EINTR is not a failure. try select/read again
                continue;
              return OU::eformat(error, "Timed read failed. Error in 'select()': %s", strerror(errno));
            default:
              return OU::eformat(error, "Unexpected EOF");
          }
        }
        default:
          if (errno == EINTR) // EINTR is not a failure. try select/read again
            continue;
          return OU::eformat(error, "Timed read failed. Error in 'select()': %s", strerror(errno));
      }
    }
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
    if (m_pid && mywait(false, error))
      return true;
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
#define myassert(cond) do if (!(cond)) { OS::dumpStack(); terminate(); assert(cond); } while (0)
  // This is the way control plane responses come back.
  // It is also the way data plane output request (writes) come out of the sim
  bool
  doResponse(std::string &error) {
    ocpiDebug("doResponse: sim is producing SDP output");
    if (m_respLeft == 0) {
      OH::SDP::Header h;
      bool request;
      if (h.getHeader(m_resp.m_rfd, request, error))
	return true;
      if (request) {
	bool writing = h.get_op() == OH::SDP::Header::WriteOp;
	XferServices &xfs = writing ? m_writeServices : m_readServices;
	uint64_t whole_addr = h.getWholeByteAddress();
	uint16_t mbox = OCPI_UTRUNCATE(uint16_t, whole_addr >> 32);
	whole_addr &= ~(-(uint64_t)1 << 32);
	myassert(mbox < xfs.size() && xfs[mbox]);
	if (writing) {
	  myassert(h.getLength() <= sizeof(m_sdpDataBuf));
	  if (h.endRequest(h, m_resp.m_rfd, m_sdpDataBuf, error))
	    return true;
	  xfs[mbox]->send(OCPI_UTRUNCATE(DtOsDataTypes::Offset, whole_addr),
			  m_sdpDataBuf, h.getLength());
	} else {
	  // Active message read/pull DMA, which will only work with locally mapped endpoints
	  uint8_t *data =
	    (uint8_t *)xfs[mbox]->
	    from().sMemServices().map(OCPI_UTRUNCATE(DtOsDataTypes::Offset, whole_addr),
				      h.getLength());
	  send2sdp(h, data, true, "DMA read response", error);
	}
      } else {
	myassert(!m_respQueue.empty());
	Request &r = *m_respQueue.front();
	if (r.header.endRequest(h, m_resp.m_rfd, r.data, error))
	  return true;
	r.sem.post();
	m_respQueue.pop();
      }
      return false;
    }
    return false;
  }

  static void w2(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    static char buf[1000];
    vsprintf(buf, fmt, ap);
    (void)write(2, buf, strlen(buf));
  }
  void terminate() {
    uint8_t msg[2];
    msg[0] = TERMINATE;
    msg[1] = 0;
    w2("Telling the simulator process (pid %u) to exit.\n", m_pid);
    (void)write(m_ctl.m_wfd, msg, 2);
  }

  // FIXME: signal safety even if we are just terminating anyway...
  static void
  sigint(int /* signal */) {
    bool any = false;
    OH::Driver &dv = OH::Driver::getSingleton();
    for (auto it = dv.m_devices.begin(); it != dv.m_devices.end(); ++it) {
      Device &d = **it;
      if (d.m_pid) {
	any = true;
	w2("\nInterrupt: Simulator process pid %u running.\n", d.m_pid);
	d.terminate();
	if (d.m_stopped) {
	  w2("\nSimulator process pid %u still running.\n", d.m_pid);
	  signal(SIGINT, SIG_DFL);
	}
	d.m_stopped = true;
      }
    }
    if (!any) {
      signal(SIGINT, SIG_DFL);
      kill(0, SIGINT);
    }
  }
public:
  ~Device() {
    lock(); // unlocked by SelfMutex destructor
    shutdown();
    OH::Driver::getSingleton().m_devices.erase(this);
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
    OU::SelfAutoMutex guard(this);
    if (m_firstRun) {
      if (m_state != RUNNING) {
	OS::sleep(100);
	return false;
      }
      m_lastTicks = 0;
      m_firstRun = false;
    }
    std::string error;
    if (!m_exited && !m_stopped && m_cumTicks < m_simTicks) {
      if (m_cumTicks - m_lastTicks > 1000) {
	ocpiDebug("Spin credit at: %20" PRIu64, m_cumTicks);
	m_lastTicks = m_cumTicks;
      }
      if (!doit(error) && m_cumTicks < m_simTicks)
	return false;
    }
    ocpiInfo("exit simulator container thread x %d s %d ct %" PRIu64 " st %u e '%s'",
	      m_exited, m_stopped, m_cumTicks, m_simTicks, error.c_str());
    if (m_exited) {
      if (m_verbose)
	fprintf(stderr, "Simulator \"%s\" exited normally\n", m_name.c_str());
      ocpiInfo("Simulator \"%s\" exited normally", m_name.c_str());
    } else if (m_stopped) {
      if (m_verbose)
	fprintf(stderr, "Stopping simulator \"%s\" due to signal\n", m_name.c_str());
      ocpiInfo("Stopping simulator \"%s\" due to signal", m_name.c_str());
    } else if (m_cumTicks >= m_simTicks) {
      if (m_verbose)
	fprintf(stderr, "Simulator \"%s\" credits at %" PRIu64 " exceeded %u, stopping simulation\n",
		m_name.c_str(), m_cumTicks, m_simTicks);
      ocpiInfo("Simulator \"%s\" credits at %" PRIu64 " exceeded %u, stopping simulation",
	       m_name.c_str(), m_cumTicks, m_simTicks);
    } else {
      if (m_verbose)
	fprintf(stderr, "Simulator \"%s\" shutting down. Error: %s\n",
		m_name.c_str(), error.empty() ? "none" : error.c_str());
      ocpiInfo("Simulator \"%s\" shutting down. Error: %s",
	       m_name.c_str(), error.empty() ? "none" : error.c_str());
    }
    shutdown();
    return true;
  }
private:
  const char *uuid() const {
    return m_textUUID.uuid;
  }
  void addFd(int fd, bool always) {
    if (fd > m_maxFd)
      m_maxFd = fd;
    if (always)
      FD_SET(fd, &m_alwaysSet);
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
    // For the local simulator, the connection is implicit, so there is nothing to do.
#if 0
    std::string err;
    if (start(err))
      throwit("Can't start \"%s\" simulator from \"%s\": %s",
	      m_platform.c_str(), m_exec.c_str(), err.c_str());
#endif
  }

  bool load(const char *file, std::string &error) {
#if 0
    static Device *loaded = NULL;
    if (loaded && loaded != this) {
      error = "Multiple HDL simulator containers are not yet supported";
      return true;
    }
    loaded = this;
#endif
    if (myload(file, error)) {
      m_isAlive = false;
      return true;
    }
    return init(error);
  }
  bool myload(const char *file, std::string &error) {
    OU::SelfAutoMutex guard (this);
    assert(m_state != LOADING || m_state != SERVING);
    if (m_state == EMULATING) {
      if (initFifos(error))
	  return true;
      OH::Driver::getSingleton().m_devices.insert(this);
    } else {
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
	return OU::eformat(error, "simulator file name %s is not formatted properly", file);
      const char *dot = strchr(suff + 1, '.');
      if (!dot)
	return OU::eformat(error, "simulator file name %s is not formatted properly", file);
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
      return OU::eformat(error,
			 "Can't create directory \"%s\" to run \"%s\" simulation from \"%s\" (%s)",
			 m_dir.c_str(), m_platform.c_str(), m_exec.c_str(), strerror(errno));
    OL::Artifact &art = *OL::Simple::getDriver().addArtifact(file);
    std::string l_platform;
    const char *e;
    if ((e = OX::getRequiredString(art.xml(), l_platform, "platform", "artifact")))
      return OU::eformat(error, "invalid metadata in binary/artifact file \"%s\": %s",
			 m_exec.c_str(), e);
    if (!strcmp("_pf", l_platform.c_str() + l_platform.length() - 3))
      l_platform.resize(l_platform.length() - 3);
    if (l_platform != m_platform)
      return OU::eformat(error,
			 "simulator platform mismatch:  executable (%s) has '%s', we are '%s'",
			 m_exec.c_str(), l_platform.c_str(), m_platform.c_str());
    std::string l_uuid;
    if ((e = OX::getRequiredString(art.xml(), l_uuid, "uuid", "artifact")))
      return OU::eformat(error, "invalid metadata in binary/artifact file \"%s\": %s",
			 m_exec.c_str(), e);
    ocpiInfo("Bitstream %s has uuid %s", m_exec.c_str(), l_uuid.c_str());
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
      return OU::eformat(error, "Couldn't start execution of command: %s", untar.c_str());
    case -1:
      return OU::eformat(error,
			 "System error (%s, errno %d) while executing bitstream loading command",
			 strerror(errno), errno);
    default:
      return OU::eformat(error, "Error return %u while executing bitstream loading command", rc);
    case 0:
      if (m_verbose)
	fprintf(stderr, "Executable/bitstream is installed and ready, in directory %s.\n",
		m_dir.c_str());
      ocpiInfo("Successfully loaded bitstream file: \"%s\" for simulation", m_exec.c_str());
    }
    setState(SERVING);   // Were loaded or loading
    if (start(error)) {
      std::string tmp(error);
      return OU::eformat(error, "Can't start \"%s\" simulator from \"%s\": %s",
			 m_platform.c_str(), m_exec.c_str(), tmp.c_str());
    }
    setState(RUNNING);   // Were loaded or loading
    return false;
  }
  void
  send2sdp(SDP::Header &h, uint8_t *data, bool response, const char *type, std::string &error) {
    size_t rlen;
    bool bad;
    {
      OU::AutoMutex m(m_sdpSendMutex);
      bad = response ?
	h.sendResponse(m_req.m_wfd, data, rlen, error) :
	h.startRequest(m_req.m_wfd, data, rlen, error);
    }
    // FIXME: is this a dead lock?  should we send the credit first?
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
      static uint32_t success = OCCP_SUCCESS_RESULT;
      static uint64_t zero = 0, junk;
      if (offset <= sizeof(m_admin))
	eaddr = &m_admin[offset];
      else if (offset >= offsetof(OccpSpace, config))
	if ((offset - offsetof(OccpSpace, config)) >=
	    OCCP_WORKER_CONFIG_SIZE + 2*sizeof(uint64_t))
	  throwit("Read/write offset out of range1 when emulating: 0x%" PRIx64, offset);
	else {
	  offset -= offsetof(OccpSpace, config);
	  if (offset >= OCCP_WORKER_CONFIG_SIZE)
	    eaddr = (uint8_t*)&zero;
	  else
	    eaddr = (uint8_t*)&m_uuid + offset;
	}
      else if (offset >= offsetof(OccpSpace, worker)) {
	if ((offset - offsetof(OccpSpace, worker)) >= sizeof(OccpWorker) * 2)
	  throwit("Read/write offset out of range2 when emulating: 0x%" PRIx64, offset);
	else {
	  //	  offset -= offsetof(OccpSpace, worker);
	  if (offset >= offsetof(OccpSpace, worker))
	    offset -= offsetof(OccpSpace, worker);
	  if (offset > sizeof(OccpWorker))
	    offset -= sizeof(OccpWorker); // second worker?
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
    send2sdp(r.header, data, false, "control", r.error);
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
    sdpRequest(false, offset, bytes, (uint8_t *)&data/* + (offset & 3)*/, status);
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
  bool
  unload(std::string &error) {
    return OU::eformat(error, "Can't unload bitstreams for simulated devices yet");
  }
  void receive(DtOsDataTypes::Offset offset, uint8_t *data, size_t count) {
    SDP::Header h(false, offset, count);
    std::string error;
    send2sdp(h, data, false, "data", error);
  }
  DT::EndPoint &getEndPoint() {
    if (m_endPoint)
      return *m_endPoint;
    // Create an endpoint that is specialized to call back the container
    // when data arrives for the endpount rather than allocating a large internal buffer.
    // Size comes from the properties of the SDP worker.
    DT::EndPoint &ep =
      DT::getManager().allocateProxyEndPoint(m_endpointSpecific.c_str(), true,
					     OCPI_UTRUNCATE(size_t, m_endpointSize));
    ep.finalize();
    // This is the hook that allows us to receive data/metadata/flags pushed to this
    // endpoint from elsewhere - from multiple other endpoints.
    ep.setReceiver(*this);
    m_readServices.resize(ep.maxCount(), 0);
    m_writeServices.resize(ep.maxCount(), 0);
    m_endPoint = &ep;
    return ep;
  }
  // FIXME: the "other" argument should be an endpoint, but that isn't easy now
  void connect(DT::EndPoint &ep, OCPI::RDT::Descriptors &mine,
	       const OCPI::RDT::Descriptors &other) {
    OT::Transport::fillDescriptorFromEndPoint(ep, mine); // needed?
    assert(m_endPoint);
    assert(&ep == m_endPoint);
    DT::EndPoint &otherEp = m_endPoint->factory().getEndPoint(other.desc.oob.oep);
    assert(otherEp.mailBox() < m_writeServices.size() &&
	   otherEp.mailBox() < m_readServices.size());
    DT::XferServices
      &newWrite = m_endPoint->factory().getTemplate(*m_endPoint, otherEp),
      &newRead = m_endPoint->factory().getTemplate(otherEp, *m_endPoint),
      **oldWrite = &m_writeServices[otherEp.mailBox()],
      **oldRead = &m_readServices[otherEp.mailBox()];
    if (*oldWrite != &newWrite) {
      if (*oldWrite)
	(*oldWrite)->release();
      *oldWrite = &newWrite;
      newWrite.addRef();
    }
    if (*oldRead != &newRead) {
      if (*oldRead)
	(*oldRead)->release();
      *oldRead = &newRead;
      newRead.addRef();
    }
  }
};

Driver::
~Driver() {
}

static const char *
getSims(std::vector<std::string> &sims) {
  std::string path;
  const char *err;
  // In a riuntimne environment there are no projects
  if ((err = OU::getAllProjects(path))) {
    ocpiInfo("When looking for simulators, could not find any projects: %s", err);
    return NULL;
  }
  std::vector<std::string> pdirs;
  sims.clear();
  std::string first;
  if (OU::searchPath(path.c_str(), "lib/platforms/*", first, "exports", &pdirs)) {
    ocpiInfo("No HDL platforms found (no lib/platforms/*) in path %s",
	     path.c_str());
    return NULL;
  }
  std::set<std::string> seen;
  for (unsigned n = 0; n < pdirs.size(); n++) {
    std::string name(strrchr(pdirs[n].c_str(), '/') + 1);
    if (!seen.insert(name).second) // in case sim platform is in more than one project
      continue;
    std::string sim;
    OU::format(sim, "%s/runSimExec.%s", pdirs[n].c_str(), name.c_str());
    bool isDir;
    ocpiDebug("Looking for %s for simulator %s", name.c_str(), sim.c_str());
    if (OS::FileSystem::exists(sim.c_str(), &isDir))
      sims.push_back(pdirs[n]);
  }
  return NULL;
}

unsigned Driver::
search(const OU::PValue *params, const char **excludes, bool discoveryOnly, std::string &error) {
  error.clear();
  const char *env;
  // Note that the default here is to DO discovery, i.e. to disablediscovery
  // the variable must be set and set to 0
  if ((env = getenv("OCPI_ENABLE_HDL_SIMULATOR_DISCOVERY")) && env[0] == '0')
    return 0;
  ocpiInfo("Searching for local HDL simulators.");
  bool verbose = false;
  OU::findBool(params, "verbose", verbose);
  const char *envsims = getenv("OCPI_HDL_SIMULATORS");
  if (!envsims)
    envsims = getenv("OCPI_HDL_SIMULATOR");
  std::set<std::string> onlySims;
  if (envsims) {
    ocpiInfo("Restricting discovery of HDL simulators to: %s", envsims);
    char
      *mylist = strdup(envsims),
      *base = mylist,
      *last = 0,
      *tok;
    for (unsigned n = 0; (tok = strtok_r(base, ", \t", &last)); base = NULL, n++)
      onlySims.insert(tok);
    free(mylist);
    if (onlySims.empty())
      return 0;
  }
  {
    std::vector<std::string> sims;
    const char *err = getSims(sims);
    if (err) {
      OU::format(error, "Cannot find any simulation platforms: %s", err);
      return 0;
    }
    unsigned count = 0;
    for (unsigned n = 0; n < sims.size(); n++) {
      const char *name = strrchr(sims[n].c_str(), '/') + 1;
      if (!onlySims.empty() && onlySims.find(name) == onlySims.end()) {
	ocpiInfo("Skipping simulator \"%s\" due to OCPI_HDL_SIMULATOR(S)", name);
	continue;
      }

      std::string cmd;
      OU::format(cmd, "bash %s/runSimExec.%s %s probe", sims[n].c_str(), name,
		 OS::logGetLevel() >= 8 ? "-v" : "");
      ocpiInfo("Checking whether the %s simulator is available and licensed", name);
      //  FIXME: make this more of a utility
      int rc = system(cmd.c_str());
      std::string serr;
      switch (rc) {
      case 127:
	OU::format(serr, "Couldn't start execution of command: %s", cmd.c_str()); break;
	break;
      case -1:
	OU::format(serr, "System error (%s, errno %d) while executing license validation command",
			   strerror(errno), errno);
	break;
      default:
	if (!WIFEXITED(rc))
	  OU::format(serr, "Error return %u/0x%x while executing license validation command",
		     rc, rc);
	else if (WEXITSTATUS(rc) != 0)
	  ocpiInfo("Check for simulator %s failed.", name);
	else {
	  OCPI::HDL::Device *dev = open(name, params, serr);
	  if (dev && !found(*dev, excludes, discoveryOnly, serr))
	    count++;
	}
	break;
      }
      if (error.empty())
	error = serr;
    }
  }
  return 0;
}

OH::Device *Driver::
open(const char *name, const OA::PValue *params, std::string &err) {
  const char *cp;
  for (cp = name; *cp && !isdigit(*cp); cp++)
    ;
  std::string platform;
  platform.assign(name, cp - name);
  bool verbose = false;
  OU::findBool(params, "verbose", verbose);
  const char *dir = "simulations";
  // Backward compatibility for old default of "simtest".
  // If you don't mention it, and simtest exists, use it
  if (!OU::findString(params, "simDir", dir)) {
    bool isDir;
    if (OS::FileSystem::exists("simtest", &isDir) && isDir)
      dir = "simtest";
  }
  uint32_t simTicks = 100000000, sleepUsecs = 200000;
  uint8_t spinCount = 20;
  OU::findULong(params, "simTicks", simTicks);

  return createDevice(name, platform, spinCount, sleepUsecs, simTicks, params, false, dir, err);
}

Device *Driver::
createDevice(const std::string &name, const std::string &platform, uint8_t spinCount,
	     unsigned sleepUsecs, unsigned simTicks, const OU::PValue *params, bool dump,
	     const char *dir, std::string &error) {
  std::string path, script, actualPlatform;
  const char *err;
  if ((err = OU::getAllProjects(path))) {
    error = err;
    return NULL;
  }
  size_t len = platform.length();
  actualPlatform.assign(platform.c_str(),
			!strcmp("_pf", platform.c_str() + len - 3) ? len - 3 : len);
  std::string relScript;
  OU::format(relScript, "lib/platforms/%s/runSimExec.%s", actualPlatform.c_str(),
	     actualPlatform.c_str());
  if (OU::searchPath(path.c_str(), relScript.c_str(), script, "exports")) {
    OU::format(error, "No simulation platform found named %s (no %s)",
	       platform.c_str(), relScript.c_str());
    return NULL;
  }
  std::string simDir;
  static unsigned n;
  OU::format(simDir, "%s/%s.%s.%u.%u", dir, actualPlatform.c_str(), name.c_str(), getpid(), n++);
  Device *d = new Device(name, simDir, actualPlatform, script, spinCount, sleepUsecs,
			 simTicks, params, dump, error);
  if (error.empty())
    return d;
  delete d;
  return NULL;
}


    } // namespace LSim
  } // namespace HDL
} // namespace OCPI
