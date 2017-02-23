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
#include <stdlib.h>
#include <signal.h>     // for kill
#include <sys/types.h>  // for mkfifo, waitpid
#include <sys/stat.h>   // for mkfifo
#include <sys/wait.h>   // for waitpid
#include <netinet/in.h> // for ntohs etc.
#include <string>
#include <list>
#include <queue>
#include <climits>
#include <cerrno>
#include "OcpiOsFileIterator.h"
#include "OcpiOsFileSystem.h"
#include "OcpiOsServerSocket.h"
#include "OcpiOsMisc.h"
#include "OcpiUuid.h"
#include "OcpiUtilEzxml.h"
#include "LibrarySimple.h"
#include "HdlSimDriver.h"
#include "HdlNetDriver.h"
#include "HdlSimServer.h"
#include "HdlSdp.h"

namespace OCPI {
  namespace HDL {
    namespace Sim {
      namespace HN = OCPI::HDL::Net;
      namespace OH = OCPI::HDL;
      namespace OS = OCPI::OS;
      namespace OL = OCPI::Library;
      namespace OU = OCPI::Util;
      namespace OX = OCPI::Util::EzXml;
      namespace OE = OCPI::OS::Ether;


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
	~Fifo() {
	  ocpiDebug("Destroying fifo %s", m_name.c_str());
	  if (m_rfd >= 0) close(m_rfd);
	  if (m_wfd >= 0) close(m_wfd);
	  unlink(m_name.c_str());
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
		 ((r = read(m_rfd, buf, sizeof(buf))) > 0 || (r < 0 && errno == EINTR)))
	    ;
	  ocpiDebug("Ending flush of any state from previous simulation run");
	}
      };

      struct Sim {
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
	} m_state;
	OE::Interface m_udp;
	// This endpoint is where we get "discovered" and we only only receive, never transmit
	OE::Socket m_disc;
	// This endpoint is what we use for everything but receiving discovery messages
	// We forward responses from the sim back to the requester
	typedef std::list<OE::Socket *> Clients;
	typedef Clients::iterator ClientsIter;
	Clients m_clients;

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
	static pid_t s_pid;
	static bool s_stopped;
	static bool s_exited;
	uint64_t m_dcp;
	char m_admin[sizeof(OH::OccpAdminRegisters)];
	HdlUUID m_uuid;
	std::string m_script, m_dir, m_app, m_file;
	static const unsigned RESP_MIN = 10;
	static const unsigned RESP_MAX = 14;
	static const unsigned RESP_LEN = 3; // index in layload buffer where length hides
	static const unsigned RESP_TAG = 7; // index in layload buffer where length hides
	OE::Packet m_response, m_serverResponse;
	OE::Address m_lastClient, m_runClient;
	bool m_haveTag;
	size_t m_respLeft;
	uint8_t *m_respPtr;
	// This structure is what we remember about a request: which socket and from which 
	// address.  This is to allow multiple clients to discover the same device, when
	// it is not attached to an SDP client.  Since SDP writes are posted, this really
	// only applies to read requests
	struct Request {
	  OE::Socket &sock;
	  OE::Address from;
	  unsigned index;
	  size_t length;
	  OH::SDP::Header &header;    // storage owned here
	  HN::EtherControlPacket packet;  // not initialized - we copy on top of this.
	  Request(OE::Socket &a_sock, OE::Address addr, unsigned a_index, size_t a_length,
		  OH::SDP::Header &a_header, OE::Packet &pkt)
	    : sock(a_sock), from(addr), index(a_index), length(a_length), header(a_header) {
	    packet = *(HN::EtherControlPacket*)pkt.payload;
	  }
	  ~Request() { delete &header; }
	};
	std::queue<Request, std::list<Request> > m_respQueue;
	std::string m_platform;
	std::string m_exec; // simulation executable local relative path name
	std::string m_cwd;  // current working dir (where ocpirun is being launched)
	bool m_verbose, m_dump, m_spinning, m_loading;
	unsigned m_sleepUsecs, m_simTicks;
	uint8_t m_spinCount;
	uint64_t m_cumTicks;
	OS::Timer m_spinTimer;
	std::string m_name;
	OU::UuidString m_textUUID;
	// Start local state for executable file transfer
	OS::ServerSocket *m_xferSrvr; // not NULL after establishment and before acceptance
	OS::Socket *m_xferSckt;       // not NULL after acceptance, before EOF
	char m_xferBuf[64 * 1024];
	uint64_t m_xferSize;        // how many bytes need to be transferred?
	uint64_t m_xferCount;       // how many bytes transferred so far?
	bool m_xferDone;
	std::string m_xferError;    // error occurred during background file transfer
	int m_xfd;                  // fd for writing local copy of executable
	OS::ServerSocket *m_sdpSrvr;   // Socket to accept connection to simulator's SDP
	OS::Socket *m_sdpSckt;         // Socket to simulator's SDP
	char m_toSdpBuf[16 * 1024];
	char m_fromSdpBuf[16 * 1024];
	//	char *m_metadata;
	//	ezxml_t m_xml;
	bool m_public;
	// End local state for executable file transfer
	Sim(std::string &simDir, std::string &script, const std::string &platform,
	    uint8_t spinCount, unsigned sleepUsecs, unsigned simTicks, bool verbose, bool dump,
	    bool isPublic, std::string &error)
	  : m_state(EMULATING), m_udp(isPublic ? "udp" : "udplocal", error),
	    m_disc(m_udp, ocpi_slave, NULL, 0, error), // the broadcast receiver
	    //      m_ext(m_udp, ocpi_device, NULL, 0, error),
	    m_req(simDir + "/request", false, NULL, error),
	    m_resp(simDir + "/response", true, NULL, error),
	    m_ctl(simDir + "/control", false, NULL, error),
	    m_ack(simDir + "/ack", false, NULL, error),
	    //      m_server(simDir + "/server", true, NULL, error),
	    //      m_client(simDir + "/client", true, NULL, error),
	    m_maxFd(-1), m_dcp(0), m_script(script), m_haveTag(false),
	    m_respLeft(0), m_respPtr(NULL), m_platform(platform), m_verbose(verbose),
	    m_dump(dump), m_spinning(false), m_sleepUsecs(sleepUsecs), m_simTicks(simTicks),
	    m_spinCount(spinCount), m_cumTicks(0), m_spinTimer(true), m_xferSrvr(NULL), m_xferSckt(NULL),
	    m_xferSize(0), m_xferCount(0), m_xferDone(false), m_xfd(-1), m_sdpSrvr(NULL),
	  m_sdpSckt(NULL), /*m_metadata(NULL), m_xml(NULL),*/ m_public(isPublic) {

	  if (error.length())
	    return;
	  FD_ZERO(&m_alwaysSet);
	  addFd(m_disc.fd(), true);
	  addFd(m_resp.m_rfd, true);
	  addFd(m_ack.m_rfd, false);
	  OE::IfScanner ifs(error);
	  if (!error.empty())
	    return;
	  OE::Interface eif;
	  OE::Address udp(true);
	  while (ifs.getNext(eif, error, NULL) && error.empty()) {
	    if (eif.up && eif.connected && !eif.ipAddr.empty() && (isPublic || eif.loopback)) {
	      OE::Socket *s = new OE::Socket(eif, ocpi_device, &udp, 0, error);
	      if (!error.empty()) {
		delete s;
		return;
	      }
	      ocpiInfo("Creating discovery interface for server: ifc %s addr %s",
		       eif.name.c_str(), s->ifAddr().pretty());
	      m_clients.push_back(s);
	      addFd(s->fd(), true);
	    }
	  }
	  if (!error.empty())
	    return;
	  if (m_clients.empty()) {
	    error = "no network interfaces found";
	    return;
	  }
	  ocpiDebug("resp %d ack %d nfds %d", m_resp.m_rfd, m_ack.m_rfd, m_maxFd);
	  std::string plat(m_platform);
	  plat += "_pf";
	  Server::initAdmin(*(OH::OccpAdminRegisters *)m_admin, plat.c_str(), m_uuid, &m_textUUID);
	  m_name = "sim:";
	  m_name += m_clients.front()->ifAddr().pretty();
	  if (verbose) {
	    fprintf(stderr, "Simulation server for %s (UUID %s), reachable using HDL device(s): ",
		    m_platform.c_str(), uuid());
	    for (ClientsIter ci = m_clients.begin(); ci != m_clients.end(); ci++)
	      fprintf(stderr, "%ssim:%s", ci == m_clients.begin() ? "" : ", ", (*ci)->ifAddr().pretty());
	    fprintf(stderr, "\n");
	    fflush(stderr);
	  }
	}
	~Sim() {
	  ocpiDebug("Simulator destruction");
	  flush();
	  while (!m_clients.empty()) {
	    OE::Socket *s = m_clients.front();
	    m_clients.pop_front();
	    delete s;
	  }
#if 0
	  if (m_metadata)
	    delete [] m_metadata;
	  if (m_xml)
	    ezxml_free(m_xml);
#endif
	}
	const char *uuid() const {
	  return m_textUUID;
	}
	void addFd(int fd, bool always) {
	  if (fd > m_maxFd)
	    m_maxFd = fd;
	  if (always)
	    FD_SET(fd, &m_alwaysSet);
	}
	const std::string &name() {
	  return m_name;
	}
	void
	setState(State state) {
	  ocpiDebug("Server state set to %u", state);
	  m_state = state;
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
	      OU::format(error, "simulator failed: look in sim.out");
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
	  m_spinning = false;
	  return false;
	}
	// Start the executable, which means start up the simulator.
	// This is called:
	//  from load when we are not transfering the file
	//  from doServer when we are done transferring the file (the 'R' command)
	bool
	start(std::string &response, std::string &err) {
	  response = "E";
	  assert(!m_xferSrvr);
#if 0
	  try {
	    std::time_t mtime;
	    uint64_t length;
	    if (!(m_metadata = OL::Artifact::getMetadata(m_exec.c_str(), mtime, length))) {
	      OU::format(err, "simulation executable file '%s' is not valid: cannot find metadata",
			 m_exec.c_str());
	      return true;
	    }
	  } catch (std::string &s) {
	    OU::format(err, "When processing simulation executable file '%s': %s",
		       m_exec.c_str(), s.c_str());
	    return true;
	  }
	  const char *e = OCPI::Util::EzXml::ezxml_parse_str(m_metadata, strlen(m_metadata), m_xml);
	  if (e) {
	    OU::format(err, "invalid metadata in binary/artifact file \"%s\": %s",
		       m_exec.c_str(), e);
	    return true;
	  }
	  char *xname = ezxml_name(m_xml);
	  if (!xname || strcasecmp("artifact", xname)) {
	    OU::format(err, "invalid metadata in binary/artifact file \"%s\": no <artifact/>",
		       m_exec.c_str());
	    return true;
	  }
#else
	  OL::Artifact &art = *OL::Simple::getDriver().addArtifact(m_exec.c_str());
#endif
	  std::string platform;
	  const char *e;
	  if ((e = OX::getRequiredString(art.xml(), platform, "platform", "artifact"))) {
	    OU::format(err, "invalid metadata in binary/artifact file \"%s\": %s",
		       m_exec.c_str(), e);
	    return true;
	  }
	  if (!strcmp("_pf", platform.c_str() + platform.length() - 3))
	    platform.resize(platform.length() - 3);
	  if (platform != m_platform) {
	    OU::format(err, "simulator platform mismatch:  executable (%s) has '%s', we are '%s'",
		       m_exec.c_str(), platform.c_str(), m_platform.c_str());
	    return true;
	  }
	  std::string l_uuid;
	  e = OX::getRequiredString(art.xml(), l_uuid, "uuid", "artifact");
	  if (e) {
	    OU::format(err, "invalid metadata in binary/artifact file \"%s\": %s",
		       m_exec.c_str(), e);
	    return true;
	  }
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
	    OU::format(err, "Couldn't start execution of command: %s", untar.c_str());
	    return 0;
	  case -1:
	    OU::format(err, "System error (%s, errno %d) while executing bitstream loading command",
		       strerror(errno), errno);
	    return 0;
	  default:
	    OU::format(err, "Error return %u while executing bitstream loading command", rc);
	    return 0;
	  case 0:
	    if (m_verbose)
	      fprintf(stderr, "Executable/bitstream is installed and ready, in directory %s.\n",
		      m_dir.c_str());
	    ocpiInfo("Successfully loaded bitstream file: \"%s\" for simulation", m_exec.c_str());
	    break;
	  }
	  std::string cmd;

	  const char *xenv = getenv("OCPI_CDK_DIR");
	  if (!xenv) {
	    err = " The OCPI_CDK_DIR environment variable is not set";
	    return true;
	  }
	  OU::format(cmd, "exec %s %s sw2sim=%s sim2sw=%s ctl=%s ack=%s cwd=%s", m_script.c_str(), m_file.c_str(),
		     m_req.m_name.c_str(), m_resp.m_name.c_str(), m_ctl.m_name.c_str(),
		     m_ack.m_name.c_str(), m_cwd.c_str());

	  if (m_dump)
	    cmd += " bscvcd";

	  if (m_verbose)
	    fprintf(stderr, "Starting execution of simulator for HDL assembly: %s "
		    "(executable \"%s\", dir \"%s\" pwd \"%s\").\n", m_app.c_str(), m_exec.c_str(),
		    m_dir.c_str(), getenv("PWD"));
	  switch ((s_pid = fork())) {
	  case 0:
	    if (chdir(m_dir.c_str()) != 0) {
	      std::string x("Cannot change to simulation subdirectory: ");
	      int en = errno;
	      x += m_dir;
	      x += "\n";
	      ocpiCheck(write(2, x.c_str(), x.length()) == (ssize_t)x.length());
	      _exit(10 + en);
	    }
	    {
	      int fd = creat("sim.out", 0666);
	      if (fd < 0) {
		std::string x("Error: Cannot create sim.out file for simulation output.\n");
		int en = errno;
		ocpiCheck(write(2, x.c_str(), x.length()) == (ssize_t)x.length());
		_exit(10 + en);
	      }
	      if (dup2(fd, 1) < 0 ||
		  dup2(fd, 2) < 0)
		_exit(10 + errno);
	      assert(fd > 2);
	      // FIXME this conditional is nonsense, right?
	      if (execl("/bin/sh", "/bin/sh", "--noprofile", "-c", cmd.c_str(), NULL))
		_exit(10 + errno);
	    }
	    break; // not used.
	  case -1:
	    OU::format(err, "Could not create simulator sub-process for: %s", m_exec.c_str());
	    return true;
	  default:
	    ocpiInfo("Simulator subprocess has pid: %u.", s_pid);
	  }
	  if (m_verbose)
	    fprintf(stderr, "Simulator process (process id %u) started, with its output in %s/sim.out\n",
		    s_pid, m_dir.c_str());
#if 0
	  uint8_t msg[2];
	  msg[0] = m_dump ? DUMP_ON : DUMP_OFF;
	  msg[1] = 0;
	  ocpiCheck(write(m_ctl.m_wfd, msg, 2) == 2);
#endif
	  // Improve the odds of an immediate error giving a good error message by letting the sim run
	  ocpiInfo("Waiting for simulator to start before issuing any more credits.");
	  OS::sleep(100);
	  for (unsigned n = 0; n < 1; n++)
	    if (spin(err) || mywait(s_pid, false, err) || ack(err))
	      return true;
	  if (m_verbose)
	    fprintf(stderr, "Simulator process is running.\n");
	  m_sdpSrvr = new OS::ServerSocket(0);
	  addFd(m_sdpSrvr->fd(), false);
	  OU::format(response, "O%u", m_sdpSrvr->getPortNo());
	  err.clear();
	  return false;
	}
	// Load the simulation executable and prepare the response back to the client:
	// E for error, O for OK, X for transfer
	// Return true if not ok and set error
	// Called either at server startup (when given an executable) OR on demand,
	// possible from a connected client
	// Return true with no error if transfer is in progress
	bool
	load(const char *file, uint64_t size, const char *cwd, std::string &response,
	     std::string &err) {
	  if (m_verbose)
	    fprintf(stderr, "Initializing %s simulator from executable/bitstream: %s\n",
		    m_platform.c_str(), file);
	  ocpiDebug("Starting to load bitstream file for %s: %s", m_platform.c_str(), file);
	  // First establish a directory for the simulation based on the name of the file
	  const char *slash = strrchr(file, '/');
	  slash = slash ? slash + 1 : file;
	  const char *suff = strstr(slash, m_platform.c_str());
	  if (suff) {
	    if (*--suff != '_' && *suff != '-') {
	      OU::format(err, "simulator file name %s is not formatted properly", file);
	      return true;
	    }
	    const char *dot = strchr(suff + 1, '.');
	    if (!dot) {
	      OU::format(err, "simulator file name %s is not formatted properly", file);
	      return true;
	    }
	    m_file.assign(slash, dot - slash);
	    m_app.assign(slash, suff - slash);
	  } else
	    m_file = m_app = slash;
	  m_dir = m_app;

	  char date[100];
	  time_t now = time(NULL);
	  struct tm nowtm;
	  localtime_r(&now, &nowtm);
	  strftime(date, sizeof(date), ".%Y%m%d%H%M%S", &nowtm);
	  m_dir += ".";
	  m_dir += m_platform;
	  m_dir += date;
	  ocpiDebug("Sim executable is %s(%s), assy is %s, platform is %s dir is %s",
		    file, m_file.c_str(), m_app.c_str(), m_platform.c_str(), m_dir.c_str());
	  if (mkdir(m_dir.c_str(), 0777) != 0 && errno != EEXIST) {
	    OU::format(err, "Can't create directory %s to run simulation (%s)",
		       m_dir.c_str(), strerror(errno));
	    return true;
	  }
	  // At this point we are ready to actually receive the file.
	  // If the client is local, we try and read it directly (from "file"), otherwise
	  m_cwd = cwd ? cwd : "";
	  if (size == 0) {
	    m_exec = file;
	    return start(response, err);
	  }
	  // This name is the local name for when a copy is made
	  m_exec = m_dir;
	  m_exec += '/';
	  m_exec += slash; // remember file name, whether local or not
	  m_xferSize = size;
	  m_xferCount = 0;
	  // The file is remote.
	  // We need to create a socket to receive it
	  m_xferSrvr = new OS::ServerSocket(0); // m_xferSrvr specifies the state of being loaded
	  unsigned port = m_xferSrvr->getPortNo();
	  ocpiDebug("Socket for loading bit file established at port %u", port);
	  OU::format(response, "X%u", port);
	  addFd(m_xferSrvr->fd(), false);
	  err.clear();
	  return true; // we're working on it.  FIXME: time out for dead client?
	}
	// Flush all comms to the sim process since we have a new client.
	// Called from destructor too
	void
	flush() {
	  ocpiDebug("Flushing all session state");
	  m_req.flush();  // FIXME: could this steal partial requests and get things out of sync?
	  m_resp.flush(); // FIXME: should we wait for the request fifo to clear?
	  m_exec.clear();
	  delete m_xferSrvr;
	  m_xferSrvr = NULL;
	  delete m_xferSckt;
	  m_xferSckt = NULL;
	  delete m_sdpSrvr;
	  m_sdpSrvr = NULL;
	  delete m_sdpSckt;
	  m_sdpSckt = NULL;
	  while (!m_respQueue.empty())
	    m_respQueue.pop();
	}
	void
	shutdown() {
	  if (s_pid) {
	    uint8_t msg[2];
	    std::string error;
	    msg[0] = TERMINATE;
	    msg[1] = 0;
	    ocpiInfo("Telling the simulator process (%u) to exit", s_pid);
	    ocpiCheck(write(m_ctl.m_wfd, msg, 2) == 2);
	    ocpiInfo("Waiting for simulator process to exit");
	    mywait(s_pid, true, error);
	    m_ctl.flush();
	    if (error.size())
	      ocpiBad("Error when shutting down simulator: %s", error.c_str());
	    m_dcp = 0;
	    s_pid = 0;
	  }
	  flush();
	}
	bool
	doResponse(std::string &error) {
	  ocpiDebug("doResponse: sim is producing SDP output");
	  if (!m_respQueue.empty()) {
	    Request &request = m_respQueue.front();
	    
	    // A discovery request was routed to the running simulator via SDP
	    HN::EtherControlPacket &pkt = *(HN::EtherControlPacket *)m_response.payload;
	    uint32_t data;
	    bool ret = request.header.endRequest(m_resp.m_rfd, (uint8_t*)&data, error);
	    if (!ret) {
	      *(HN::EtherControlPacket *)m_response.payload = request.packet;
	      pkt.readResponse.data = htonl(data);
	      ocpiBad("writing response %p to client: len %zu tag %d to %s via index %u",
			&m_response, request.length, m_response.payload[RESP_TAG],
			request.from.pretty(), request.index);
	      ret = sendToIfc(request.sock, request.index, request.length, request.from, error);
	    }
	    m_respQueue.pop();
	    return ret;
	  }
	  ssize_t nread = 0, room = sizeof(m_fromSdpBuf);
	  assert(!(room&3));
	  char *cp = m_fromSdpBuf;
	  do {
	    ssize_t n = read(m_resp.m_rfd, cp, room);
	    if (n == 0) {
	      OU::format(error, "SDP channel from simulator got EOF");
	      return true;
	    }
	    if (n < 0) {
	      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
		n = 0;
	      else {
		OU::format(error, "SDP channel from simulator broken: %s (%zd %d)",
			   strerror(errno), n, errno);
		return true;
	      }
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
	  return false;
	}
#if 0
	{
	  bool first = false;
	  if (!m_respLeft) {
	    m_respLeft = RESP_MIN;
	    m_respPtr = m_response.payload + 2;
	    first = true;
	  }
	  ssize_t n = read(m_resp.m_rfd, m_respPtr, m_respLeft);
	  if (n <= 0) {
	    if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
	      if (first)
		m_respLeft = 0;
	      return false;
	    }
	    OU::format(error, "Response channel from simulator broken: %zd %d", n, errno);
	    return true;
	  }
	  m_respPtr += n;
	  m_respLeft -= n;
	  if (m_respPtr > m_response.payload + RESP_LEN) {
	    // we are past the length so we can look at it
	    size_t len = m_response.payload[RESP_LEN];
	    if ((m_respLeft = (m_response.payload + 2 + len) - m_respPtr) == 0) {
	      if (m_respQueue.empty()) {
		OU::format(error, "Response client queue empty");
		return true;
	      }
	      Request &request = m_respQueue.back();
	      printTime("after response full read");
	      ocpiBad("writing response %p to client: len %zu tag %d to %s via index %u", len,
			&m_response, m_response.payload[RESP_TAG], request.from.pretty(),
			request.index);
	      bool bad = sendToIfc(request.sock, request.index, len + 2, request.from, error);
	      m_respQueue.pop();
	      if (m_respQueue.empty() && spin(error))
		return true;
	      return bad;
	    }
	  }
	  return false;
	}
#endif
	bool
	isServerCommand(uint8_t *payload) {
	  return OCCP_ETHER_RESERVED(((HN::EtherControlHeader *)payload)->typeEtc) != 0;
	}

	bool
	isNopCommand(uint8_t *payload) {
	  HN::EtherControlPacket &pkt = *(HN::EtherControlPacket *)payload;
	  return OCCP_ETHER_MESSAGE_TYPE(pkt.header.typeEtc) ==  HN::OCCP_NOP;
	}

	// This is essentially a separate channel with its own tags
	// Perform a command initiated by a client via UDP.
	bool
	doServer(OE::Socket &ext, OE::Packet &rFrame, size_t &length, OE::Address &from,
		 unsigned maxPayLoad, std::string &error) {
	  uint8_t *payload = rFrame.payload;
	  HN::EtherControlHeader &hdr_in = *(HN::EtherControlHeader *)payload;
	  HN::EtherControlMessageType action = OCCP_ETHER_MESSAGE_TYPE(hdr_in.typeEtc);
	  size_t clen = ntohs(hdr_in.length) - (sizeof(hdr_in) - 2);
	  char *command = (char *)(payload + sizeof(hdr_in));
	  if (action != HN::OCCP_NOP || length == 0 || length < (unsigned)ntohs(hdr_in.length) + 2 ||
	      strlen(command) + 1 != clen) {
	    OU::format(error, "bad client message: action is %u, length is %zu", action, length);
	    return true;
	  }
	  HN::EtherControlHeader &hdr_out =  *(HN::EtherControlHeader *)(m_serverResponse.payload);
	  ocpiDebug("server command from %s: action %u length %zu tag %u actual '%s'",
		    from.pretty(), action, clen, hdr_in.tag, command);
	  std::string response;
	  if (!m_haveTag || hdr_in.tag != hdr_out.tag || from != m_lastClient) {
	    if (m_state == CONNECTED && from != m_runClient) {
	      ocpiInfo("Server command refused when there is a connected client");
	      return false;
	    }
	    m_lastClient = from;
	    m_haveTag = true;
	    hdr_out.tag = hdr_in.tag;
	    ocpiDebug("Server received command '%s'", command);
	    switch (*command) {
	    case 'L': // load  the simulation executable
	      {
		assert(m_state != LOADING || m_state != SERVING);
		if (m_state != EMULATING) {
		  ocpiInfo("Shutting down previous simulation before (re)loading");
		  shutdown();
		}
		char *bitfile;
		char *cwd;
		m_xferError.clear();
		errno = 0;
		unsigned long long size = strtoull(command + 1, &bitfile, 0);
		if (errno || size == ULLONG_MAX)
		  error = "EInvalid file size";
		else {
		  // remove any whitespace b4 the bitfile name
		  while (*bitfile && isspace(*bitfile))
		    bitfile++;
		  cwd = bitfile;
		  // insert a null character and grab the cwd
		  while (*cwd && !isspace(*cwd))
		    cwd++;
		  *cwd++ = '\0';
		  while (*cwd && isspace(*cwd))
		    cwd++;
		  if (load(bitfile, size, cwd, response, error) && error.length())
		    setState(EMULATING); // something bad happened, we're back to square one.
		  else {
		    m_runClient = from;
		    setState(SERVING);   // Were loaded or loading
		  }
		}
	      }
	      break;
	    case 'F': // flush all file and sim executable state
	      response = "O";
	      ocpiDebug("Received flush request");
	      // shutdown();
	      flush();
	      break;
	    case 'S':
	      // Start the executable after it has been transferred to the server
	      if (from != m_runClient || m_state != SERVING) {
		OU::format(response,
			   "EStart command from wrong client or in wrong state");
		ocpiBad("EStart command from wrong client '%s' vs '%s' or in wrong state: %i",
			from.pretty(), m_runClient.pretty(), m_state);
	      } else if (m_xferError.length()) {
		assert(!m_xferDone && !m_xferSckt);
		OU::format(response, "E%s", m_xferError.c_str());
		setState(EMULATING);
		m_xferError.clear();
	      } else if (m_xferDone)
		if (start(response, error))
		  setState(EMULATING);
		else
		  setState(RUNNING);
	      else {
		OU::format(response,
			   "ETransfer not done: received %" PRIu64 " bytes, expected %" PRIu64
			   " bytes when receiving file", m_xferCount, m_xferSize);
		setState(EMULATING);
	      }
	      delete m_xferSckt;
	      m_xferSckt = NULL;
	      delete m_xferSrvr;
	      m_xferSrvr = NULL;
	      if (m_state != RUNNING)
		break;
	      /******* FALL INTO *******/
	    case 'C':
	      // Connect to the running system
	      if (m_state == RUNNING || m_state == DISCONNECTED) {
		if (!m_sdpSrvr) {
		  m_sdpSrvr = new OS::ServerSocket(0);
		  addFd(m_sdpSrvr->fd(), false);
		}
		OU::format(response, "O%u", m_sdpSrvr->getPortNo());
		m_runClient = from;
	      } else
		response = "ESimulation Server does not have a running simulator";
	      break;
	    default:
	      OU::format(error, "received invalid command: '%s'", command);
	      return true;
	    }
	    // command processed, and a response or error prepared.
	    if (error.length() >= maxPayLoad - 1) {
	      OU::format(error, "command response to '%s' too long (%zu) for %u",
			 command, error.length(), maxPayLoad);
	      return true;
	    }
	    if (error.size())
	      OU::format(response, "E%s", error.c_str());
	    length = sizeof(hdr_out) + response.length() + 1;
	    hdr_out.length = htons(OCPI_UTRUNCATE(uint16_t, length - 2));
	    hdr_out.typeEtc = OCCP_ETHER_TYPE_ETC(HN::OCCP_RESPONSE, HN::OK, 0, 1);
	    ocpiDebug("command result is: '%s', length %zu", response.c_str(), length);
	    strcpy((char *)(&hdr_out + 1), response.c_str());
	  } else {
	    ocpiDebug("Redundant response being sent");
	    // A redundant request, just return the last response
	    length = ntohs(hdr_out.length) + 2;
	  }
	  // Send the response back to the client.
	  error.clear();
	  memcpy(payload, m_serverResponse.payload, length);
	  return !ext.send(rFrame, length, from, 0, NULL, error);
	}
	// There is no simulator running, handle it ourselves.
	// Return true if error is set
	// Set the length arg to the length of the response to send back
	// The response is created in the payload that was passed in
	bool
	doEmulate(size_t &length, OH::SDP::Header **sdp, std::string &error) {
	  HN::EtherControlPacket &pkt = *(HN::EtherControlPacket *)m_response.payload;
	  ocpiDebug("doEmulate: Got header.  Need %zu header %zu", length,
		    sizeof(HN::EtherControlHeader));
	  if (length - 2 != ntohs(pkt.header.length)) {
	    OU::format(error, "bad client message length: %zu vs %u", length, ntohs(pkt.header.length));
	    return true;
	  }
	  unsigned uncache = OCCP_ETHER_UNCACHED(pkt.header.typeEtc) ? 1 : 0;
	  HN::EtherControlMessageType action = OCCP_ETHER_MESSAGE_TYPE(pkt.header.typeEtc);
	  //ocpiDebug("Got message");
	  size_t offset = ntohl(pkt.write.address);
	  unsigned be = OCCP_ETHER_BYTE_ENABLES(pkt.header.typeEtc);
	  unsigned blen;
	  // convert the byte enables back to a byte offset
	  switch (be) {
	  case 0x1: blen = 1;break;
	  case 0x2: offset += 1; blen = 1; break;
	  case 0x4: offset += 2; blen = 1; break;
	  case 0x8: offset += 3; blen = 1; break;
	  case 0x3: blen = 2; break;
	  case 0xc: offset += 2; blen = 2; break;
	  case 0xf: blen = 4; break;
	  default: 
	    ocpiBad("Byte enable unexpected: 0x%x", be);
	  }
	  ssize_t len;
	  // Clobber the header now that we have read everything.
	  pkt.header.typeEtc = OCCP_ETHER_TYPE_ETC(HN::OCCP_RESPONSE, HN::OK, uncache, 0);
	  switch (action) {
	  default:
	    OU::format(error, "Invalid control message received when no sim executable %x",
		       pkt.header.typeEtc);
	    return true;
	  case HN::OCCP_WRITE:
	    len = sizeof(HN::EtherControlWriteResponse);
	    if (sdp) {
	      OH::SDP::Header h(false, offset, blen);
	      uint32_t data = ntohl(pkt.write.data);
	      size_t l;
	      if (h.startRequest(m_req.m_wfd, (uint8_t*)&data, l, error) ||
		  sendCredit(l, error))
		return true;
	    } else if (offset <= sizeof(m_admin))
	      *(uint32_t *)(&((char *)&m_admin) [offset]) = ntohl(pkt.write.data);
	    else if (offset >= offsetof(OccpSpace, config))
	      if ((offset - offsetof(OccpSpace, config)) >= sizeof(m_uuid))
		ocpiBad("Write offset out of range: 0x%zx", offset);
	      else {
		offset -= offsetof(OccpSpace, config);
		*(uint32_t *)(&((char *)&m_uuid) [offset]) = ntohl(pkt.write.data);
	      }
	    else if (offset >= offsetof(OccpSpace, worker)) {
	      if ((offset - offsetof(OccpSpace, worker)) >= sizeof(OccpWorker))
		ocpiBad("Write offset out of range: 0x%zx", offset);
	      else {
		switch (offset - offsetof(OccpSpace, worker)) {
		case offsetof(OccpWorkerRegisters, control):
		case offsetof(OccpWorkerRegisters, clearError):
		case offsetof(OccpWorkerRegisters, window):
		  break;
		default:
		  ocpiBad("Write offset out of range: 0x%zx", offset);
		}
	      }
	    } else
	      ocpiBad("Write offset out of range: 0x%zx", offset);
	    break;
	  case HN::OCCP_READ:
	    ocpiDebug("Read command, offset 0x%zx sdp %p", offset, sdp);
	    len = sizeof(HN::EtherControlReadResponse);
	    if (sdp) {
	      *sdp = new OH::SDP::Header(true, offset, blen);
	      size_t l;
	      if ((*sdp)->startRequest(m_req.m_wfd, NULL, l, error) ||
		  sendCredit(l, error)) {
		delete *sdp;
		*sdp = NULL;
		return true;
	      }
	      ocpiDebug("Started SDP Request from client");
	    } else if (offset <= sizeof(m_admin))
	      pkt.readResponse.data = htonl(*(uint32_t *)(&((char *)&m_admin)[offset]));
	    else if (offset >= offsetof(OccpSpace, config))
	      if ((offset - offsetof(OccpSpace, config)) >= sizeof(m_uuid) + sizeof(uint64_t))
		ocpiBad("Read offset out of range1: 0x%zx", offset);
	      else {
		offset -= offsetof(OccpSpace, config);
		pkt.readResponse.data = htonl(*(uint32_t *)(&((char *)&m_uuid)[offset]));
	      }
	    else if (offset >= offsetof(OccpSpace, worker)) {
	      if ((offset - offsetof(OccpSpace, worker)) >= sizeof(OccpWorker))
		ocpiBad("Read offset out of range2: 0x%zx", offset);
	      else {
		offset -= offsetof(OccpSpace, worker);
		switch (offset) {
		case offsetof(OccpWorkerRegisters, initialize):
		case offsetof(OccpWorkerRegisters, start):
		  pkt.readResponse.data = htonl(OCCP_SUCCESS_RESULT);
		break;
		case offsetof(OccpWorkerRegisters, status):
		default:
		  pkt.readResponse.data = 0; // no errors
		}
	      }
	    } else {
	      ocpiBad("Read offset out of range3: 0x%zx", offset);
	      pkt.readResponse.data = 0xa5a5a5a5;
	    }
	    ocpiDebug("Read command response: 0x%" PRIx32 " %p", ntohl(pkt.readResponse.data),
		      &pkt.readResponse.data);
	    break;
	  case HN::OCCP_NOP:
	    // We emulate this whether SDP is alive or not since the normal control plane
	    // doesn't handle it anyway.
	    len = sizeof(HN::EtherControlNopResponse);
	    ocpiAssert(ntohs(pkt.header.length) == sizeof(HN::EtherControlNop) - 2);
	    // Tag is the same
	    pkt.header.typeEtc = OCCP_ETHER_TYPE_ETC(HN::OCCP_RESPONSE, HN::OK, uncache, 0);
	    pkt.nopResponse.mbx40 = 0x40;
	    pkt.nopResponse.mbz0 = 0;
	    memcpy(pkt.nopResponse.mac, OU::getSystemAddr().addr(), OS::Ether::Address::s_size);
	    pkt.nopResponse.pid = getpid();
	  }
	  pkt.header.length = htons(OCPI_UTRUNCATE(uint16_t, len - 2));
	  length = len;
	  return false;
	}

#if 1
	// Ugh - we have a running simulator, but need to process an old style command
	// that arrived in a discovery packet...
	bool
	sendToSim(OE::Socket &ext, size_t length, OE::Address &from,
		  unsigned index, std::string &error) {
	  OH::SDP::Header *sdp = NULL;
	  ocpiDebug("Processing UDP command");
	  if (doEmulate(length, &sdp, error))
	     return true;
	  ocpiDebug("Processing UDP command: sdp %p", sdp);
	  if (sdp) {
	    Request r(ext, from, index, length, *sdp, m_response);
	    // This request has created a pending read request.
	    m_respQueue.push(r);
	  } else
	    sendToIfc(ext, index, length, from, error);
	  return false;
	}
#else
	bool
	sendToSim(OE::Socket &s, uint8_t *payload, size_t length, OE::Address &from, unsigned index,
		  std::string &error) {
	  HN::EtherControlHeader &hdr_in = *(HN::EtherControlHeader *)payload;
	  uint8_t *bp = payload + 2;
	  size_t nactual = length - 2;
	  ssize_t nn;
	  for (size_t nw = 0; nw < nactual; nw += nn, bp += nn) {
	    if ((nn = write(m_req.m_wfd, bp, nactual - nw)) < 0) {
	      if (errno != EINTR) {
		error = "write error to request fifo";
		return true;
	      }
	    } else if (nn == 0) {
	      error = "wrote zero bytes to request fifo";
	      return true;
	    } else {
	      uint8_t msg[2];
	      msg[0] = DCP_CREDIT;
	      msg[1] = OCPI_UTRUNCATE(uint8_t, nn);
	      if (write(m_ctl.m_wfd, msg, 2) != 2) {
		error = "write error to control fifo";
		return true;
	      }
	      m_dcp += nn;
	      if (spin(error))
		return true;
	    }
	  }
	  ocpiDebug("written request to sim: len %zu action %u tag %u proto len %u", length,
		    OCCP_ETHER_MESSAGE_TYPE(hdr_in.typeEtc), hdr_in.tag, ntohs(hdr_in.length));
	  printTime("request written to sim");
	  m_respQueue.push(Request(s, from, index));
	  return false;
	}
#endif

	void
	printTime(const char *msg) {
	  OS::ElapsedTime et = m_spinTimer.getElapsed();
	  ocpiDebug("When %s time since spin is: %" PRIu32 ".%03" PRIu32 " s ago", msg,
		    et.seconds(), (et.nanoseconds() + 500000) / 1000000);
	}
	// Send to the client over the given interface
	// If the interface is zero, use the socket
	bool
	sendToIfc(OE::Socket &sock, unsigned index, size_t length, OE::Address &to,
		  std::string &error) {
	  OE::Socket *s = &sock;
	  if (index) {
	    for (ClientsIter ci = m_clients.begin(); ci != m_clients.end(); ci++)
	      if ((*ci)->ifIndex() == index) {
		s = *ci;
		break;
	      }
	    if (!s) {
	      OU::format(error, "can't find a client socket with interface %u", index);
	      return true;
	    }
	  }
	  ocpiBad("writing response %p: len %zu", &m_response, length);
	  return !s->send(m_response, length, to, 0, NULL, error);
	}
	// A select call has indicated a socket ready to read.
	// It might be the discovery socket
	bool
	receiveExt(OE::Socket &ext, bool discovery, std::string &error) {
	  size_t length;
	  OE::Address from;
	  unsigned index = 0;
	  if (ext.receive(m_response, length, 0, from, error, discovery ? &index : NULL)) {
	    assert(from != m_udp.addr);
	    ocpiDebug("Received request packet from %s, length %zu", from.pretty(), length);
	    if (!m_public && from.addrInAddr() != m_udp.addr.addrInAddr()) {
	      ocpiDebug("Discovery request received ignored since we are not public %s/%s/%s",
			from.pretty(), m_udp.addr.pretty(), m_udp.ipAddr.pretty());
	      return false;
	    }
	    if (isServerCommand(m_response.payload)) {
	      assert(!discovery);
	      if (doServer(ext, m_response, length, from, sizeof(m_response.payload), error))
		return true;
	    } else if (m_sdpSckt) {
	      // We do nothing since if we have an SDP socket active to a client, we
	      // are not discoverable.
	    } else if (s_pid) {
	      // We are running a simulator without a current client.
	      if (sendToSim(ext, length, from, index, error))
		return true;
	    } else if (doEmulate(length, NULL, error) ||
		       sendToIfc(ext, index, length, from, error))
	      return true;
	  }
	  return false;
	}
	bool
	sendCredit(size_t credit, std::string &error) {
	  assert(!(credit & 3));
	  credit >>= 2;
	  m_dcp += credit;
	  uint8_t msg[3];
	  msg[0] = DCP_CREDIT;
	  msg[1] = OCPI_UTRUNCATE(uint8_t, credit & 0xff);
	  msg[2] = OCPI_UTRUNCATE(uint8_t, credit >> 8);
	  if (write(m_ctl.m_wfd, msg, 3) != 3) {
	    error = "write error to control fifo";
	    return true;
	  }
	  return false;
	}
	// read from sdp socket and write into simulator sdp/req channel
	bool
	doSdp(std::string &error) {
	  assert(m_state == CONNECTED);
	  assert(m_sdpSckt->fd() >= 0);
	  size_t nrcvd;
	  try {
	    nrcvd = m_sdpSckt->recv(m_toSdpBuf, sizeof(m_toSdpBuf), 0);
	  } catch (...) {
	    nrcvd = 0;
	  }
	  if (!nrcvd) {
	    ocpiDebug("SDP socket from client EOF or failure");
	    delete m_sdpSckt;
	    m_sdpSckt = NULL;
	    flush();
	    setState(DISCONNECTED);
	    return false;
	  }
	  ssize_t nn;
	  char *bp = m_toSdpBuf;
	  uint8_t residue = 0;
	  for (size_t nw = 0; nw < nrcvd; nw += nn, bp += nn) {
	    if ((nn = write(m_req.m_wfd, bp, nrcvd - nw)) < 0) {
	      if (errno != EINTR) {
		error = "write error to request fifo";
		return true;
	      }
	    } else if (nn == 0) {
	      error = "wrote zero bytes to request fifo";
	      return true;
	    } else {
	      assert(!(nn &3));
#if 0
	      uint32_t *p32 = (uint32_t*)bp;
	      fprintf(stderr, "To sdp (res %u, nn %zu):", residue, nn);
	      for (unsigned n = 0; n < nn; n += 4)
		fprintf(stderr, " %2d: %8x", n/4, *p32++);
	      fprintf(stderr, "\n");
#endif
	      size_t credit = nn + residue;
	      residue = credit & 3;
	      assert(!residue);
	      if (sendCredit(credit & ~3, error))
		return true;
#if 0
	      credit >>= 2;
	      m_dcp += credit;
	      uint8_t msg[3];
	      msg[0] = DCP_CREDIT;
	      msg[1] = OCPI_UTRUNCATE(uint8_t, credit & 0xff);
	      msg[2] = OCPI_UTRUNCATE(uint8_t, credit >> 8);
#if 0
	      fprintf(stderr, "Writing a credit to the simulator for reading\n");
	      char c[100];
	      read(0, c, 100); 
#endif
	      if (write(m_ctl.m_wfd, msg, 3) != 3) {
		error = "write error to control fifo";
		return true;
	      }
#endif
	      if (spin(error))
		return true;
	    }
	  }
	  ocpiDebug("written sdp data to sim: len %zu", nrcvd);
	  printTime("request written to sim");
	  //m_respQueue.push(Request(s, from, index));
	  return false;
	}
	// Do some reading from the file transfer socket
	// Return true if fatal error
	bool
	doXfer(std::string &/*err*/) {
	  // Reader has stuff to read: FIXME: make this fd non-blocking for cleanliness
	  assert(m_xfd >= 0);
	  size_t n = m_xferSckt->recv(m_xferBuf, sizeof(m_xferBuf), 0);
	  ocpiDebug("Transfer socket received %zu", n);
	  switch (n) {
	  default:
	    if (m_xferCount + n > m_xferSize)
	      OU::format(m_xferError,
			 "Executable transfer got %" PRIu64 " bytes, expected %" PRIu64,
			 m_xferCount + n, m_xferSize);
	    else if (write(m_xfd, m_xferBuf, n) == (ssize_t)n) {
	      m_xferCount += n;
	      return false;
	    } else
	      OU::format(m_xferError, "Error writing executable file: %s", strerror(errno));
	    break;
	  case 0:
	    if (m_xferCount != m_xferSize)
	      OU::format(m_xferError, "Executable transfer got %" PRIu64 " bytes, expected %" PRIu64,
			 m_xferCount, m_xferSize);
	    // Force EOF on the other end
	    m_xferSckt->shutdown(true);
	    m_xferDone = true;
	    // don't delete the socket in case "shutdown" takes some time?
	    return false;
	  case SIZE_MAX:
	    OU::format(m_xferError, "Unexpected timeout on reading transfer socket");
	    break;
	  }
	  close(m_xfd);
	  delete m_xferSckt;
	  m_xferSckt = NULL;
	  return false;  // FIXME: start a timeout if client never calls back?
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
#if 0
	  // Just interesting debug info
	  {
	    int n = 0;
	    if (ioctl(m_req.m_rfd, FIONREAD, &n) == -1) {
	      error = "fionread syscall on req";
	      return true;
	    }
	    int n1 = 0;
	    if (ioctl(m_ctl.m_rfd, FIONREAD, &n1) == -1) {
	      error = "fionread syscall on ctl";
	      return true;
	    }
	    int n2 = 0;
	    if (ioctl(m_ack.m_rfd, FIONREAD, &n2) == -1) {
	      error = "fionread syscall on ctl";
	      return true;
	    }
	    if (n || n1 || n2)
	      ocpiDebug("Request FIFO has %d, control has %d, ack has %d, dcp %" PRIu64, n, n1, n2, m_dcp);
	  }
#endif
	  if (s_pid && mywait(s_pid, false, error)) {
	    if (error.empty()) {
	      s_exited = true;
	      s_pid = 0;
	    }
	    return true;
	  }
	  fd_set fds[1];
	  *fds = m_alwaysSet;
	  if (m_dcp)                    // only do this after SOME control op
	    FD_SET(m_ack.m_rfd, fds);   // spin credit ACKS from sim
	  if (m_xferSrvr)
	    FD_SET(m_xferSrvr->fd(), fds);
	  if (m_xferSckt && !m_xferDone)
	    FD_SET(m_xferSckt->fd(), fds);
	  if (m_sdpSrvr)
	    FD_SET(m_sdpSrvr->fd(), fds);
	  if (m_sdpSckt)
	    FD_SET(m_sdpSckt->fd(), fds);
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
	  if ((FD_ISSET(m_resp.m_rfd, fds) && doResponse(error)) ||
	      (FD_ISSET(m_disc.fd(), fds) && receiveExt(m_disc, true, error)))
	    return true;
	  for (ClientsIter ci = m_clients.begin(); ci != m_clients.end(); ci++)
	    if (FD_ISSET((*ci)->fd(), fds) && receiveExt(**ci, false, error))
	      return true;
	  // Next is to keep sim running by providing more credits
	  // We will only enable this fd when there is no response queue
	  if (FD_ISSET(m_ack.m_rfd, fds)) {
	    printTime("Received ACK indication");
	    if (ack(error) || spin(error))
	      return true;
	  }
	  if (m_xferSrvr && FD_ISSET(m_xferSrvr->fd(), fds)) {
	    ocpiDebug("Client has connected to our bit file transfer socket.");
	    if ((m_xfd = creat(m_exec.c_str(), 0666)) < 0) {
	      OU::format(m_xferError, "Couldn't create local copy of executable: '%s' (%s)",
			 m_exec.c_str(), strerror(errno));
	    } else {
	      m_xferSckt = new OS::Socket();
	      m_xferSrvr->accept(*m_xferSckt);
	      m_xferDone = false;
	      addFd(m_xferSckt->fd(), false);

	    }
	    delete m_xferSrvr;
	    m_xferSrvr = NULL;
	  }
	  if (m_xferSckt && FD_ISSET(m_xferSckt->fd(), fds) && doXfer(error))
	    return true;
	  // FIXME we should have single-connection server sockets...
	  if (m_sdpSrvr && FD_ISSET(m_sdpSrvr->fd(), fds)) {
	    assert(m_state == RUNNING || m_state == DISCONNECTED);
	    m_sdpSckt = new OS::Socket();
	    m_sdpSrvr->accept(*m_sdpSckt);
	    addFd(m_sdpSckt->fd(), false);
	    delete m_sdpSrvr;
	    m_sdpSrvr = NULL;
	    setState(CONNECTED);
	  }
	  if (m_sdpSckt && FD_ISSET(m_sdpSckt->fd(), fds) && doSdp(error))
	    return true;
	  return false;
	}
	static void
	sigint(int /* signal */) {
	  if (s_pid) {
	    if (s_stopped) {
	      fflush(stderr);
	      fprintf(stderr, "\nSimulator process pid %u still running.\n", Sim::s_pid);
	      fflush(stderr);
	      signal(SIGINT, SIG_DFL);
	    }
	    s_stopped = true;
	  } else
	    _exit(1);
	}

	// The (single threaded) operation of the server
	bool
	run(const std::string &exec, std::string &error) {
	  ocpiCheck(signal(SIGINT, sigint) != SIG_ERR);
	  // If we were given an executable, start sim with it.
	  assert(m_state == EMULATING);
	  if (exec.length()) {
	    std::string response; // dummy
	    if (load(exec.c_str(), 0, NULL, response, error) || start(response, error))
	      return true;
	    // We're loaded, and running a simulator, with no client.
	    setState(DISCONNECTED);
	  }
	  uint64_t last = 0;
	  while (!s_exited && !s_stopped && error.empty() && m_cumTicks < m_simTicks) {
	    if (m_cumTicks - last > 1000) {
	      ocpiInfo("Spin credit at: %20" PRIu64, m_cumTicks);
	      last = m_cumTicks;
	    }
	    if (doit(error))
	      break;
	  }
	  ocpiDebug("exit server loop x %d s %d ct %" PRIu64 " st %u e '%s'",
		    s_exited, s_stopped, m_cumTicks, m_simTicks, error.c_str());
	  if (s_exited) {
	    if (m_verbose)
	      fprintf(stderr, "Simulator exited normally\n");
	    ocpiInfo("Simulator exited normally");
	  } else if (s_stopped) {
	    if (m_verbose)
	      fprintf(stderr, "Stopping simulator due to signal\n");
	    ocpiInfo("Stopping simulator due to signal");
	  } else if (m_cumTicks >= m_simTicks) {
	    if (m_verbose)
	      fprintf(stderr, "Simulator credits at %" PRIu64 " exceeded %u, stopping simulation\n",
		      m_cumTicks, m_simTicks);
	    ocpiInfo("Simulator credits at %" PRIu64 " exceeded %u, stopping simulation",
		     m_cumTicks, m_simTicks);
	  } else
	    ocpiInfo("Simulator shutting down. Error: %s", error.empty() ? "none" : error.c_str());
	  shutdown();
	  return !error.empty();
	}
      };
      pid_t Sim::s_pid = 0;
      bool Sim::s_stopped = false;
      bool Sim::s_exited = false;

      Server::
      Server(const char *name, const std::string &platform, uint8_t spinCount,
	     unsigned sleepUsecs, unsigned simTicks, bool verbose, bool dump, bool isPublic,
	     std::string &error)
	: m_sim(NULL) {
	// FIXME - determine this via argv0
	const char *xenv = getenv("OCPI_CDK_DIR");
	if (!xenv) {
	  error = "The OCPI_CDK_DIR environment variable is not set";
	  return;
	}
	std::string path, item, script, internalPlatform, actualPlatform;
	const char *ppenv = getenv("OCPI_PROJECT_PATH");
	if (ppenv) {
	  path = ppenv;
	  path += ":";
	}
	path += xenv;
	if (platform.empty()) {
	  OU::format(error, "You must specify a simulation platform (-p <sim_pf>");
	  return;
	}
	internalPlatform = platform;
	size_t len = platform.length();
	if (strcmp("_pf", platform.c_str() + platform.length() - 3))
	  internalPlatform += "_pf";
	OU::format(item, "lib/platforms/%s/runSimExec.%s", internalPlatform.c_str(),
		   internalPlatform.c_str());
	if (OU::searchPath(path.c_str(), item.c_str(), script, "exports")) {
	  OU::format(error,
		     "\"%s\" not a supported or built simulation platform? could not find \"%s\" in OCPI_CDK_DIR or OCPI_PROJECT_PATH",
		     internalPlatform.c_str(), item.c_str());
	  return;
	}
	actualPlatform = platform;
	actualPlatform.assign(platform.c_str(), !strcmp("_pf", platform.c_str() + len - 3) ? len - 3 : len);
	pid_t pid = getpid();
	OU::format(m_simDir, "%s/%s", OH::Sim::TMPDIR, OH::Sim::SIMDIR);
	// We do not clean this up - it is created on demand.
	// FIXME: do we need this at all?
	if (mkdir(m_simDir.c_str(), 0777) != 0 && errno != EEXIST) {
	  OU::format(error, "Can't create directory for all OpenCPI simulator containers: %s",
		     m_simDir.c_str());
	  m_simDir.clear();
	  return;
	}
	std::string simName;
	if (name)
	  simName = name;
	else
	  OU::format(simName, "%s.%u", actualPlatform.c_str(), pid);
	OU::format(m_simDir, "%s/%s/%s.%s", OH::Sim::TMPDIR, OH::Sim::SIMDIR, OH::Sim::SIMPREF,
		   simName.c_str());
	if (mkdir(m_simDir.c_str(), 0777) != 0) {
	  if (errno == EEXIST)
	    OU::format(error, "Directory for this simulator, \"%s\", already exists (/tmp not cleared?)",
		       m_simDir.c_str());
	  else
	    OU::format(error, "Can't create the new diretory for this simulator: %s", m_simDir.c_str());
	  return;
	}
	m_sim = new Sim(m_simDir, script, actualPlatform, spinCount, sleepUsecs, simTicks,
			verbose, dump, isPublic, error);
	if (error.size())
	  return;
	ocpiInfo("Simulator named \"sim:%s\" created in %s. All fifos are open.",
		 strrchr(m_simDir.c_str(), '.') + 1, m_simDir.c_str());
      }
      Server::
      ~Server() {
	ocpiDebug("Simulation server destruction");
	delete m_sim;
	if (m_simDir.length()) {
	  ocpiDebug("Removing sim directory: %s", m_simDir.c_str());
	  std::string cmd;
	  OU::format(cmd, "rm -r -f %s", m_simDir.c_str());
	  if (system(cmd.c_str()) != 0)
	    ocpiBad("Cannot remove the simulation directory: %s", m_simDir.c_str());
	}
      }
      bool Server::
      run(const std::string &exec, std::string &error) {
	return m_sim->run(exec, error);
      }

      void Server::
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
    }
  }
}

