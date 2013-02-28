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

#include <errno.h> 
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>
#include "OcpiOsFileSystem.h"
#include "OcpiOsMisc.h"
#include "OcpiOsTimer.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "SimDriver.h"

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
	  : Net::Device(driver, ifc, name, addr, discovery, "ocpi-udp-rdma", 1000, error) {
	  // Send the "flush all state - I am a new master" command.
	  if (error.empty())
	    command("", 1, NULL, 0, 2000);
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
	  command(name, strlen(name)+1, err, sizeof(err), 2000);
	  err[sizeof(err)-1] = 0;
	  if (*err)
	    throwit("Loading new executable %s failed: %s", name, err);
	  ocpiInfo("Successfully loaded new sim executable: %s", name);
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
#if 0
      
      // Our official names are just sim:<ipaddr>
      OCPI::HDL::Device *Driver::
      open(const char *name, bool discovery, std::string &error) {
	std::string myName;
	if (!strncasecmp("sim:", name, 4)) {
	  myName = name;
	  name += 4;
	} else {
	  myName = "sim:";
	  myName += name;
	}
	ocpiDebug("Attemping to open sim device \"%s\"", myName.c_str());
	pid_t pid = atoi(name);
	if (kill(pid, 0)) {
	  OU::formatString(error, "Found simulator sim:%s, but its process is gone or privileged",
		    name);
	  return NULL;
	}
	std::string dir;
	OU::formatString(dir, "%s/%s/%s.%s", TMPDIR, SIMDIR, SIMPREF, name);
	bool isDir;
	if (!OS::FileSystem::exists(dir, &isDir) || !isDir) {
	  OU::formatString(error, "Directory for sim:%s, which is \"%s\", doesn't exist or isn't a directory",
			   name, dir.c_str());
	  return NULL;
	}
	return new Device(myName, dir, discovery, error);
      }
      unsigned Driver::
      search(const OU::PValue */*params*/, const char **exclude, std::string &error) {
	unsigned count = 0;
	std::string simDir;
	OU::formatString(simDir, "%s/%s", TMPDIR, SIMDIR);
	DIR *d = opendir(simDir.c_str());
	if (!d) {
	  ocpiDebug("Couldn't open simulator directory \"%s\" so search finds nothing",
		    simDir.c_str());
	  return 0;
	} else {
	  for (struct dirent *ent; error.empty() && (ent = readdir(d)) != NULL;)
	    if (ent->d_name[0] != '.') {
	      unsigned len = strlen(SIMPREF);
	      if (strncmp(ent->d_name, SIMPREF, len) ||
		  ent->d_name[len] != '.') {
		ocpiDebug("Found unexpected file \"%s\" in simulator directory \"%s\"",
			  ent->d_name, simDir.c_str());
		continue;
	      }
	      const char *pidStr = ent->d_name + len + 1;
	      // Opening implies canonicalizing the name, which is needed for excludes
	      OCPI::HDL::Device *dev = open(pidStr, true, error);
	      if (error.empty()) {
		if (exclude)
		  for (const char **ap = exclude; *ap; ap++)
		    if (!strcmp(*ap, dev->name().c_str()))
		      goto skipit; // continue(2);
		if (!found(*dev, error)) {
		  count++;
		  continue;
		}
	      }
	    skipit:
	      if (error.size())
		ocpiInfo("Error opening sim device: %s", error.c_str());
	      if (dev)
		delete dev;
	      error.clear();
	    }
	  closedir(d); // FIXME: try/catch?
	}
	return count;
      }
#endif
    } // namespace Sim
  } // namespace HDL
} // namespace OCPI
