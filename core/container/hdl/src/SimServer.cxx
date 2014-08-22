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
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>     // for kill
#include <unistd.h>
#include <sys/types.h>  // for mkfifo, waitpid
#include <sys/stat.h>   // for mkfifo
#include <sys/wait.h>   // for waitpid
#include <netinet/in.h> // for ntohs etc.
#include <set>
#include <queue>
#include <list>
#include "OcpiOsDebug.h"
#include "OcpiOsFileIterator.h"
#include "OcpiOsFileSystem.h"
#include "OcpiOsServerSocket.h"
#include "OcpiOsTimer.h"
#include "OcpiUuid.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "OcpiLibraryManager.h"
#include "SimServer.h"
#include "SimDriver.h"

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


#if 0
      // return true on error, otherwise read the amount requested
      static bool
      myread(int fd, char *buf, unsigned nRequested, std::string &error) {
	ocpiDebug("SIM reading %u from  fd %d", nRequested, fd);
	do {
	  ssize_t nread = read(fd, buf, nRequested);
	  if (nread == 0)
	    // shouldn't happen because we have it open for writing too
	    error = "eof on FIFO";
	  else if (nread > 0) {
	    nRequested -= nread;
	    buf += nread;
	  } else if (errno != EINTR)
	    error = "error reading FIFO";
	} while (error.empty() && nRequested);
	return !error.empty();
      }
#endif

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
	  ocpiDebug("Starting to flush any state from previous simulation run");
	  while (ioctl(m_rfd, FIONREAD, &n) >= 0 && n > 0 &&
		 ((r = read(m_rfd, buf, sizeof(buf))) > 0 ||
		  r < 0 && errno == EINTR))
	    ;
	  ocpiDebug("Ending flush of any state from previous simulation run");
	}
      };

      struct Sim {
	enum Action {
	  SPIN_CREDIT = 0,
	  DCP_CREDIT = 1,
	  DUMP_OFF = 253,
	  DUMP_ON = 254,
	  TERMINATE = 255
	};

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
	OE::Address m_lastClient;
	bool m_haveTag;
	size_t m_respLeft;
	uint8_t *m_respPtr;
	// This structure is what we remember about a request: which socket and from which address
	struct Request {
	  OE::Socket &sock;
	  OE::Address from;
	  unsigned index;
	  Request(OE::Socket &sock, OE::Address addr, unsigned index)
	    : sock(sock), from(addr), index(index) {}
	};
	std::queue<Request> m_respQueue;
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
	OS::ServerSocket *m_xferSrvr; // not NULL after establishment and before acceptace
	OS::Socket *m_xferSckt;       // not NULL after acceptance, before EOF
	char m_xferBuf[64 * 1024];
	uint64_t m_xferSize;        // how many bytes need to be transferred?
	uint64_t m_xferCount;       // how many bytes transferred so far?
	bool m_xferDone;
	std::string m_xferError;    // eror occurred during background file transfer
	int m_xfd;                  // fd for writing local copy of executable
	char *m_metadata;
	ezxml_t m_xml;
	// End local state for executable file transfer
	Sim(std::string &simDir, std::string &script, const std::string &platform,
	    uint8_t spinCount, unsigned sleepUsecs, unsigned simTicks, bool verbose, bool dump,
	    std::string &error)
	  : m_udp("udp", error),                       // the generic interface for udp broadcast receives
	    m_disc(m_udp, ocpi_slave, NULL, 0, error), // the broadcast receiver
	    //      m_ext(m_udp, ocpi_device, NULL, 0, error),
	    m_req(simDir + "/request", false, "/tmp/OpenCPI0_Req", error),
	    m_resp(simDir + "/response", true, "/tmp/OpenCPI0_Resp", error),
	    m_ctl(simDir + "/control", false, "/tmp/OpenCPI0_IOCtl", error),
	    m_ack(simDir + "/ack", false, NULL, error),
	    //      m_server(simDir + "/server", true, NULL, error),
	    //      m_client(simDir + "/client", true, NULL, error),
	    m_maxFd(-1), m_dcp(0), m_script(script), m_haveTag(false),
	    m_respLeft(0), m_respPtr(NULL), m_platform(platform), m_verbose(verbose),
	    m_dump(dump), m_spinning(false), m_sleepUsecs(sleepUsecs), m_simTicks(simTicks),
	    m_spinCount(spinCount), m_cumTicks(0), m_xferSrvr(NULL), m_xferSckt(NULL),
	    m_xferSize(0), m_xferCount(0), m_xferDone(false), m_xfd(-1), m_metadata(NULL),
	    m_xml(NULL) {
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
	    if (eif.up && eif.connected) {
	      OE::Socket *s = new OE::Socket(eif, ocpi_device, &udp, 0, error);
	      if (!error.empty()) {
		delete s;
		return;
	      }
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
	  flush();
	  while (!m_clients.empty()) {
	    OE::Socket *s = m_clients.front();
	    m_clients.pop_front();
	    delete s;
	  }
	  if (m_metadata)
	    delete [] m_metadata;
	  if (m_xml)
	    ezxml_free(m_xml);
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
	    error = "waitpid error";
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
	  if (r != 1 || c != 1) {
	    OU::format(error, "ack read from sim failed. r %zd c %u", r, c);
	    return true;
	  }
	  m_spinning = false;
	  return false;
	}
	// Do the work of loading the file and starting the sim
	// when we have the bit file ready to process
	// This is called:
	//  from loadRun when we are not transfering the file
	//  from doServer when we are done transferring the file (the 'R' command)
	bool
	finishLoadRun(char *response, std::string &err) {
	  *response = 'E';
	  assert(!m_xferSrvr);
	  try {
	    if (!(m_metadata = OL::Artifact::getMetadata(m_exec.c_str()))) {
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
	  std::string platform;
	  if ((e = OX::getRequiredString(m_xml, platform, "platform", "artifact"))) {
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
	  std::string uuid;
	  e = OX::getRequiredString(m_xml, uuid, "uuid", "artifact");
	  if (e) {
	    OU::format(err, "invalid metadata in binary/artifact file \"%s\": %s",
		       m_exec.c_str(), e);
	    return true;
	  }
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
	  OU::format(cmd, "exec %s %s req=%s resp=%s ctl=%s ack=%s cwd=%s", m_script.c_str(), m_file.c_str(),
		     m_req.m_name.c_str(), m_resp.m_name.c_str(), m_ctl.m_name.c_str(),
		     m_ack.m_name.c_str(), m_cwd.c_str());

	  if (m_dump)
	    cmd += " bscvcd";

	  if (m_verbose)
	    fprintf(stderr, "Starting execution of simulator for HDL assembly: %s "
		    "(executable \"%s\", dir \"%s\").\n", m_app.c_str(), m_exec.c_str(),
		    m_dir.c_str());
	  switch ((s_pid = fork())) {
	  case 0:
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
	      if (execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL))
		_exit(10 + errno);
	    }
	    break; // not used.
	  case -1:
	    OU::format(err, "Could not create simulator sub-process for: %s", m_exec.c_str());
	    return true;
	  default:
	    ocpiInfo("Simluator subprocess has pid: %u.", s_pid);
	  }
	  if (m_verbose)
	    fprintf(stderr, "Simulator process (process id %u) started, with its output in %s/sim.out\n",
		    s_pid, m_dir.c_str());
	  uint8_t msg[2];
	  msg[0] = m_dump ? DUMP_ON : DUMP_OFF;
	  msg[1] = 0;
	  assert(write(m_ctl.m_wfd, msg, 2) == 2);
	  // Improve the odds of an immediate error giving a good error message by letting the sim run
	  ocpiInfo("Waiting for simulator to start before issueing any more credits.");
	  for (unsigned n = 0; n < 1; n++)
	    if (spin(err) || mywait(s_pid, false, err) || ack(err))
	      return true;
	  if (m_verbose)
	    fprintf(stderr, "Simulator process is running.\n");
	  err.clear();
	  *response = 'O';
	  return false;
	}
	// Establish a new simulator from a new executable.
	// Set the response code back to the client:
	// E for error, O for OK, X for transfer
	// Put the string for the response in "err"
	// Return true if not ok
	bool
	loadRun(const char *file, uint64_t size, const char *cwd, char *response, std::string &err) {
	  *response = 'E';
	  if (m_verbose)
	    fprintf(stderr, "Initializing %s simulator from executable/bitstream: %s\n",
		    m_platform.c_str(), file);
	  ocpiDebug("Starting the %s simulator loading bitstream file: %s", m_platform.c_str(), file);
	  // First establish a directory for the simulation based on the name of the file
	  const char
	    *slash = strrchr(file, '/'),
	    *suff = strrchr(file, '-');
	  if (!suff) {
	    OU::format(err, "simulator file name %s is not formatted properly", file);
	    return true;
	  }
	  const char *dot = strchr(suff + 1, '.');
	  if (!dot) {
	    OU::format(err, "simulator file name %s is not formatted properly", file);
	    return true;
	  }
	  slash = slash ? slash + 1 : file;
	  m_file.assign(slash, dot - slash);
	  m_app.assign(slash, suff - slash);
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
	  // If the client is local, we try and ready it directly (from "file"), otherwise
	  if (size == 0) {
	    m_exec = file;
	    m_cwd = cwd ? cwd : "";
	    return finishLoadRun(response, err);
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
	  OU::format(err, "%u", port);
	  *response = 'X';
	  addFd(m_xferSrvr->fd(), false);
	  return true; // not OK/done
	}
	// Flush all comms to the sim process since we have a new client.
	// Called from destructor too
	void
	flush() {
	  m_req.flush();  // FIXME: could this steal partial requests and get things out of sync?
	  m_resp.flush(); // FIXME: should we wait for the request fifo to clear?
	  m_exec.clear();
	  if (m_xferSrvr) {
	    delete m_xferSrvr;
	    m_xferSrvr = NULL;
	  }
	  if (m_xferSckt) {
	    delete m_xferSckt;
	    m_xferSckt = NULL;
	  }
	}
	void
	shutdown() {
	  if (s_pid) {
	    uint8_t msg[2];
	    std::string error;
	    msg[0] = TERMINATE;
	    msg[1] = 0;
	    ocpiInfo("Telling the simulator process (%u) to exit", s_pid);
	    assert(write(m_ctl.m_wfd, msg, 2) == 2);
	    ocpiInfo("Waiting for simulator process to exit");
	    mywait(s_pid, true, error);
	    m_ctl.flush();
	    flush();
	    if (error.size())
	      ocpiBad("Error when shutting down simulator: %s", error.c_str());
	    m_dcp = 0;
	    s_pid = 0;
	  }
	}
	bool
	doResponse(std::string &error) {
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
	      ocpiDebug("writing response to client: len %zu tag %d to %s via index %u", len,
			m_response.payload[RESP_TAG], request.from.pretty(), request.index);
	      bool bad = sendToIfc(request.sock, request.index, m_response, len + 2, request.from, error);
	      m_respQueue.pop();
	      if (m_respQueue.empty() && spin(error))
		return true;
	      return bad;
	    }
	  }
	  return false;
	}

	bool
	isServerCommand(uint8_t *payload) {
	  return OCCP_ETHER_RESERVED(((HN::EtherControlHeader *)payload)->typeEtc) != 0;
	}

	// This is essentially a separate channel with its own tags
	bool
	doServer(OE::Socket &ext, OE::Packet &rFrame, size_t &length, OE::Address &from,
		 bool local, unsigned maxPayLoad, std::string &error) {
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
	  if (!m_haveTag || hdr_in.tag != hdr_out.tag || from != m_lastClient) {
	    m_lastClient = from;
	    m_haveTag = true;
	    hdr_out.tag = hdr_in.tag;
	    char *response = (char *)(&hdr_out + 1);
	    ocpiDebug("Server received command '%s'", command);
	    switch (*command) {
	    case 'L': // load and execute the simulation executable
	      {
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
		  ocpiInfo("Shutting down previous simulation before (re)loading");
		  shutdown();
		  loadRun(bitfile, local ? 0 : size, cwd, response, error);
		}
	      }
	      break;
	    case 'F': // flush all file and sim executable state
	      *response = 'O';
	      flush();
	      break;
	    case 'R':
	      // Run the executable after it has been transferred to the server
	      // This is only seen when the Load command returned X (transfer).
	      if (m_xferError.length()) {
		*response++ = 'E';
		error = m_xferError;
	      } else if (!m_xferSize || m_xferSrvr)
		// Nothing in progress or Work in progress, don't return anything
		return false;
	      else if (m_xferSize != m_xferCount) {
		*response++ = 'E';
		OU::format(error, "Received %" PRIu64 " bytes, expected %" PRIu64" bytes when receiving file",
			   m_xferCount, m_xferSize);
	      } else
		// So everything is ok.  Let's try running the sim
		finishLoadRun(response, error);
	      delete m_xferSckt;
	      m_xferSckt = NULL;
	      break;
	    default:
	      OU::format(error, "received invalid command: '%s'", command);
	      return true;
	    }
	    if (error.length() >= maxPayLoad - 1) {
	      OU::format(error, "command response to '%s' too long (%zu) for %u",
			 command, error.length(), maxPayLoad);
	      return true;
	    }
	    strcpy(response + 1, error.c_str());
	    length = sizeof(hdr_out) + strlen(response) + 1;
	    hdr_out.length = htons(OCPI_UTRUNCATE(uint16_t, length - 2));
	    hdr_out.typeEtc = OCCP_ETHER_TYPE_ETC(HN::OCCP_RESPONSE, HN::OK, 0, 1);
	    ocpiDebug("command result is: '%s'", response);
	  } else
	    length = ntohs(hdr_out.length) + 2;
	  error.clear();
	  memcpy(payload, m_serverResponse.payload, length);
	  return !ext.send(rFrame, length, from, 0, NULL, error);
	}
	// There is no simulator running, handle it ourselves.
	// Return true if error is set
	// Set the length arg to the length of the response to send back
	// The response is created in the payload that was passed in
	bool
	doEmulate(uint8_t *payload, size_t &length, std::string &error) {
	  HN::EtherControlPacket &pkt = *(HN::EtherControlPacket *)payload;
	  //    ocpiDebug("Got header.  Need %u header %u", n, sizeof(HN::EtherControlHeader));
	  if (length - 2 != ntohs(pkt.header.length)) {
	    OU::format(error, "bad client message length: %zu vs %u", length, ntohs(pkt.header.length));
	    return true;
	  }
	  unsigned uncache = OCCP_ETHER_UNCACHED(pkt.header.typeEtc) ? 1 : 0;
	  HN::EtherControlMessageType action = OCCP_ETHER_MESSAGE_TYPE(pkt.header.typeEtc);
	  pkt.header.typeEtc = OCCP_ETHER_TYPE_ETC(HN::OCCP_RESPONSE, HN::OK, uncache, 0);
	  //ocpiDebug("Got message");
	  size_t offset;
	  ssize_t len;
	  switch (action) {
	  default:
	    OU::format(error, "Invalid control message received when no sim executable %x",
		       pkt.header.typeEtc);
	    return true;
	  case HN::OCCP_WRITE:
	    offset = ntohl(pkt.write.address); // for read or write
	    len = sizeof(HN::EtherControlWriteResponse);
	    if (offset <= sizeof(m_admin))
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
	    offset = ntohl(pkt.read.address); // for read or write
	    len = sizeof(HN::EtherControlReadResponse);
	    ocpiDebug("Read command, offset 0x%zx", offset);
	    if (offset <= sizeof(m_admin))
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
	    } else  {
	      ocpiBad("Read offset out of range3: 0x%zx", offset);
	      pkt.readResponse.data = 0xa5a5a5a5;
	    }
	    ocpiDebug("Read command response: 0x%"PRIx32, ntohl(pkt.readResponse.data));
	    break;
	  case HN::OCCP_NOP:
	    len = sizeof(HN::EtherControlNopResponse);
	    ocpiAssert(ntohs(pkt.header.length) == sizeof(HN::EtherControlNopResponse) - 2);
	    // Tag is the same
	    pkt.header.typeEtc = OCCP_ETHER_TYPE_ETC(HN::OCCP_RESPONSE, HN::OK, uncache, 0);
	    pkt.nopResponse.mbx40 = 0x40;
	    pkt.nopResponse.mbz0 = 0;
	    pkt.nopResponse.mbz1 = 0;
	    pkt.nopResponse.maxCoalesced = 1;
	  }
	  pkt.header.length = htons(OCPI_UTRUNCATE(uint16_t, len - 2));
	  length = len;
	  return false;
	}

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

	void
	printTime(const char *msg) {
	  OS::ElapsedTime et = m_spinTimer.getElapsed();
	  ocpiDebug("When %s time since spin is: %" PRIu32 ".%03" PRIu32 " s ago", msg,
		    et.seconds(), (et.nanoseconds() + 500000) / 1000000);
	}
	// Send to the client over the given interface
	// If the interface is zero, use the socket
	bool
	sendToIfc(OE::Socket &sock, unsigned index, OE::Packet &rFrame, size_t length,
		  OE::Address &to, std::string &error) {
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
	  return !s->send(rFrame, length, to, 0, NULL, error);
	}
	// A select call has indicated a socket ready to read.
	// It might be the discovery socket
	bool
	receiveExt(OE::Socket &ext, bool discovery, std::string &error) {
	  OE::Packet rFrame;
	  size_t length;
	  OE::Address from;
	  unsigned index = 0;
	  if (ext.receive(rFrame, length, 0, from, error, discovery ? &index : NULL)) {
	    assert(from != m_udp.addr);
	    ocpiDebug("Received request packet from %s, length %zu", from.pretty(), length);
	    if (isServerCommand(rFrame.payload)) {
	      assert(!discovery);
	      if (doServer(ext, rFrame, length, from, false, sizeof(rFrame.payload), error))
		return true;
	    } else if (s_pid) {
	      if (sendToSim(ext, rFrame.payload, length, from, index, error))
		return true;
	    } else if (doEmulate(rFrame.payload, length, error) ||
		       sendToIfc(ext, index, rFrame, length, from, error))
	      return true;
	  }
	  return false;
	}
	// Do some reading from the file transfer socket
	// Return true if fatal error
	bool
	doXfer(std::string &/*err*/) {
	  // Reader has stuff to read: FIXME: make this fd non-blocking for cleanliness
	  assert(m_xfd >= 0);
	  size_t n = m_xferSckt->recv(m_xferBuf, sizeof(m_xferBuf), 0);
	  switch (n) {
	  default:
	    if (write(m_xfd, m_xferBuf, n) == (ssize_t)n) {
	      m_xferCount += n;
	      return false;
	    }
	    OU::format(m_xferError, "Error writing executable file: %s", strerror(errno));
	    break;
	  case 0:
	    if (m_xferCount != m_xferSize)
	      OU::format(m_xferError, "Executable transfer got %" PRIu64 " bytes, expected %" PRIu64,
			 m_xferCount, m_xferSize);
	    // Force EOF on the other end
	    m_xferSckt->shutdown(true);
	    m_xferDone = true;
	    return false;
	    break;
	  case ~(size_t)0:
	    OU::format(m_xferError, "Unexpected timeout on reading transfer socket");
	    break;
	  }
	  close(m_xfd);
	  delete m_xferSckt;
	  m_xferSckt = 0;
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
#ifndef NDEBUG
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
	  if (FD_ISSET(m_resp.m_rfd, fds) && doResponse(error) ||
	      FD_ISSET(m_disc.fd(), fds) && receiveExt(m_disc, true, error))
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
	    // Client has connected to our bit file transfer socket
	    if ((m_xfd = creat(m_exec.c_str(), 0666)) < 0) {
	      OU::format(m_xferError, "Couldn't create local copy of executable: '%s' (%s)",
			 m_exec.c_str(), strerror(errno));
	    } else {
	      m_xferSckt = new OS::Socket();
	      *m_xferSckt = m_xferSrvr->accept();
	      m_xferDone = false;
	      addFd(m_xferSckt->fd(), false);

	    }
	    delete m_xferSrvr;
	    m_xferSrvr = NULL;
	  }
	  if (m_xferSckt && FD_ISSET(m_xferSckt->fd(), fds) && doXfer(error))
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

	bool
	run(const std::string &exec, std::string &error) {
	  assert(signal(SIGINT, sigint) != SIG_ERR);
	  // If we were given an executable, start sim with it.
	  char response[1];
	  if (exec.length() && loadRun(exec.c_str(), 0, NULL, response, error))
	    return true;
	  uint64_t last = 0;
	  while (!s_exited && !s_stopped && error.empty() && m_cumTicks < m_simTicks) {
	    if (m_cumTicks - last > 1000) {
	      ocpiInfo("Spin credit at: %20" PRIu64, m_cumTicks);
	      last = m_cumTicks;
	    }
	    if (doit(error))
	      break;
	  }
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
	     unsigned sleepUsecs, unsigned simTicks, bool verbose, bool dump, std::string &error)
	: m_sim(NULL) {
	// FIXME - determine this via argv0
	const char *xenv = getenv("OCPI_CDK_DIR");
	if (!xenv) {
	  error = "The OCPI_CDK_DIR environment variable is not set";
	  return;
	}
	std::string script(xenv);
	script += "/scripts/";
	std::string actualPlatform;
	if (platform.empty()) {
	  OS::FileIterator fi(script, "runSimExec.*");
	  if (fi.end()) {
	    OU::format(error, "There is no supported simulation platform (no %s/runSimExec.*)",
		       script.c_str());
	    return;
	  }
	  std::string cmd = fi.relativeName();
	  const char *cp = strchr(cmd.c_str(), '.');
	  assert(cp);
	  actualPlatform = ++cp;
	  script += cmd;
	} else {
	  size_t len = platform.length();
	  actualPlatform.assign(platform.c_str(), !strcmp("_pf", platform.c_str() + len - 3) ? len - 3 : len);
	  script += "runSimExec.";
	  script += actualPlatform;
	  if (!OS::FileSystem::exists(script)) {
	    OU::format(error, "\"%s\" is not a supported simulation platform (no %s)",
		       platform.c_str(), script.c_str());
	    return;
	  }
	}
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
	m_sim = new Sim(m_simDir, script, actualPlatform, spinCount, sleepUsecs, simTicks, verbose, dump,
			error);
	if (error.size())
	  return;
	ocpiInfo("Simulator named \"sim:%s\" created in %s. All fifos are open.",
		 strrchr(m_simDir.c_str(), '.') + 1, m_simDir.c_str());
      }
      Server::
      ~Server() {
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
	unconst32(admin.config) = 0xf0;
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
	unconst64(admin.dna) = 0;
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

