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
#include "OcpiOsFileSystem.h"
#include "OcpiOsSocket.h"
#include "HdlSimDriver.h"

namespace OCPI {
  namespace HDL {
    namespace Sim {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
      namespace OE = OCPI::OS::Ether;
      
      class Device
	: public OCPI::HDL::Net::Device {
	friend class Driver;
      protected:
	Device(Driver &driver, OS::Ether::Interface &ifc, std::string &name,
	       OE::Address addr, bool discovery, std::string &error)
	  : Net::Device(driver, ifc, name, addr, discovery, "ocpi-udp-rdma", 10000, error) {
	  // Send the "flush all state - I am a new master" command.
	  if (error.empty())
	    command("F", 2, NULL, 0, 5000);
	  init(error);
	}
      public:
	~Device() {
	}
      private:
	// Convenience for setting the m_isAlive and throwing OU::Error
	void throwit(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
	  m_isAlive = false;
	  va_list ap;
	  va_start(ap, fmt);
	  throw OU::Error(ap, fmt);
	}
      public:
	// Load a bitstream via jtag
	void load(const char *name) {
	  char err[1000];
	  err[0] = 0;
	  std::string cmd(name);
	  bool isDir;
	  uint64_t length;
	  if (!OS::FileSystem::exists(cmd, &isDir, &length) || isDir)
	    throwit("New sim executable file '%s' is non-existent or a directory", name);
	  std::string cwd_str = OS::FileSystem::cwd();
	  OU::format(cmd, "L%" PRIu64 " %s %s", length, name, cwd_str.c_str());
	  command(cmd.c_str(), cmd.length()+1, err, sizeof(err), 5000);
	  err[sizeof(err)-1] = 0;
	  switch (*err) {
	  case 'E': // we have an error
	    throwit("Loading new executable %s failed: %s", name, err + 1);
	  case 'O': // OK its loaded and running
	    ocpiInfo("Successfully loaded new sim executable: %s", name);
	    break;
	  case 'X': // The file needs to be transferred
	    {
	      uint16_t port = OCPI_UTRUNCATE(uint16_t, atoi(err + 1));
	      std::string addr = this->addr().pretty(); // this will have a colon and port...
	      addr.resize(addr.find_first_of(':'));
	      
	      OS::Socket wskt;
	      wskt.connect(addr, port);
	      wskt.linger(true); //  wait on close for far side ack of all data
	      int rfd;
	      try { // socket I/O can throw
		if ((rfd = open(name, O_RDONLY)) < 0)
		  throwit("Can't open executable: %s", name);
		char buf[64*1024];
		ssize_t n;
		while ((n = read(rfd, buf, sizeof(buf))) > 0) {
		  while (n) {
		    unsigned nn = (unsigned)wskt.send(buf, n);
		    // no errors - it throws
		    n -= nn;
		  }
		}
		if (n < 0)
		  throwit("Error reading executable file: %s", name);
		wskt.shutdown(true);
		char c;
		// Wait for the other end to shutdown its writing side to give us the EOF
		wskt.recv(&c, 1, 0);
		wskt.close();
		close(rfd);
	      } catch(...) {
		try {
		  wskt.close();
		} catch(...) {};
		if (rfd > 0)
		  close(rfd);
		throw;
	      }
	      if (rfd > 0)
		close(rfd);
	      cmd = "R";
	      command(cmd.c_str(), cmd.length()+1, err, sizeof(err), 5000);
	      if (*err != 'O')
		throwit("Error, after transferring executable, starting sim: %s", err + 1);
	    }
	  }
	}
	void
	unload() {
	  throw "Can't unload bitstreams for simulated devices yet";
	}
      };

      Driver::
      ~Driver() {
      }
      Net::Device &Driver::
      createDevice(OS::Ether::Interface &ifc, OS::Ether::Address &addr,
		   bool discovery, std::string &error) {
	std::string name("sim:");
	name += addr.pretty();
	return *new Device(*this, ifc, name, addr, discovery, error);
      }
    } // namespace Sim
  } // namespace HDL
} // namespace OCPI
