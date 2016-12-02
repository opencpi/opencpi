/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
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
#include <unistd.h>
#include "OcpiOsFileSystem.h"
#include "OcpiOsLoadableModule.h"
#include "OcpiUtilCppMacros.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilMisc.h"
#include "OcpiDriverManager.h"
#include "OcpiDriverApi.h"
#include "OcpiTimeEmit.h"
namespace OCPI {
  namespace Driver {
    namespace OU = OCPI::Util;
    namespace OX = OCPI::Util::EzXml;
    void Registration_debug_hook() {} // easier static constructor debug
    Manager::~Manager(){}
    // If a manager has no configuration method, this is the default implementation.
    // The argument is the element for this manager.
    // We just look for driver-specific configuration info
    // FIXME:: check for elements that don't match..
    void Manager::configure(ezxml_t x) {
      for (Driver *d = firstDriverBase(); d; d = d->nextDriverBase()) {
	ezxml_t found = NULL;
	for (ezxml_t c = ezxml_cchild(x, d->name().c_str()); c; c = ezxml_next(c))
	  if (found)
	    throw OU::Error("Duplicate XML driver element for: \"%s\"", d->name().c_str());
	  else
	    found = c;
	ocpiDebug("Configuring the %s driver with %p/%s", d->name().c_str(), found,
		  ezxml_name(found));
	d->configure(found);
      }
    }
    unsigned Manager::cleanupPosition() { return 1; }
    // Get the singleton ManagerManager, possibly constructing it.
    ManagerManager &ManagerManager::getManagerManager() {
      return OU::Singleton<ManagerManager>::getSingleton();
    }
    ManagerManager::ManagerManager()
      : m_configured(false), m_doNotDiscover(false), m_xml(NULL)
    {}
    ManagerManager::~ManagerManager() {
      ezxml_free(m_xml);
    }
    // This is the static API method
    void ManagerManager::configure(const char *file) {
      getManagerManager().configureOnce(file);
    }
    static bool
    checkLibPath(std::string &path, std::string &dir, const char *name, bool mode, bool debug) {
      OU::format(path, "%s%s%s/libocpi_%s%s.%s", dir.c_str(),
		 mode ? "/d" : "", mode ? (debug ? "d" : "o") : "",
		 name, OCPI_DYNAMIC ? "" : "_s", OS::LoadableModule::suffix());
      return OS::FileSystem::exists(path.c_str());
    }

    // This is NOT a static method
    void ManagerManager::configureOnce(const char *file, const OCPI::Util::PValue *params) {
      if (m_configured)
	return;
      OCPI::Util::AutoMutex guard(m_mutex); 
      if (m_configured)
	return;
      m_configured = true;
      std::string configFile;
      if (!file)
	file = getenv("OCPI_SYSTEM_CONFIG");
      if (!file) {
	configFile = "/opt/opencpi/system.xml";
	if (!OS::FileSystem::exists(configFile)) {
	  assert(getenv("OCPI_CDK_DIR"));
	  OU::format(configFile, "%s/default-system.xml", getenv("OCPI_CDK_DIR"));
	}
      } else
	configFile = file;
      std::string err;
      if (configFile.empty())
	ocpiInfo("Skipping XML system configuration due to explicitly empty config file name");
      else {	
	ocpiInfo("Processing XML system configuration file: \"%s\"", configFile.c_str());
	if (OS::FileSystem::exists(configFile)) {
	  const char *e = OX::ezxml_parse_file(configFile.c_str(), m_xml);
	  if (e)
	    err = e;
	} else
	  err = "file does not exist";
      }
      if (err.empty() && m_xml) {
	for (Manager *m = firstChild(); m; m = m->nextChild())
	  ocpiDebug("Found a \"%s\" manager", m->name().c_str());
	for (ezxml_t x = OX::ezxml_firstChild(m_xml);
	     err.empty() && x; x = OX::ezxml_nextChild(x)) {
	  ocpiDebug("Processing \"%s\" element in system config file", ezxml_name(x));
	  if (!strcasecmp(ezxml_name(x), "load")) {
	    const char *file_attr = ezxml_cattr(x, "file");
	    if (!file_attr)
	      err = "missing \"file\" attribute in \"load\" element";
	    else {
	      ocpiInfo("Loading module requested in system config file: \"%s\"", file_attr);
	      OS::LoadableModule::load(file_attr, true, err);
	    }
	    continue;
	  }
	  Manager *m;
	  for (m = firstChild(); m; m = m->nextChild())
	    if (!strcasecmp(m->name().c_str(), ezxml_name(x)))
	      break;
	  if (!m) {
	    OU::format(err, "unknown/unsupported system config file element \"%s\"",
		       ezxml_name(x));
	    break;
	  }
	  for (ezxml_t d = OX::ezxml_firstChild(x); d; d = OX::ezxml_nextChild(d)) {
	    bool load;
	    const char *e = OX::getBoolean(d, "load", &load);
	    if (e) {
	      err = e;
	      break;
	    }
	    if (!load)
	      continue;
	    std::string libDir;
	    assert(getenv("OCPI_CDK_DIR"));
	    OU::format(libDir, "%s/lib/%s-%s-%s", getenv("OCPI_CDK_DIR"),
		       OCPI_CPP_STRINGIFY(OCPI_OS) + strlen("OCPI"),
		       OCPI_CPP_STRINGIFY(OCPI_OS_VERSION), OCPI_CPP_STRINGIFY(OCPI_ARCH));
	    if (!OS::FileSystem::exists(libDir)) {
	      OU::format(err, "when loading the \"%s\" \"%s\" driver, directory \"%s\" does not exist",
			 d->name, m->name().c_str(), libDir.c_str());
	      break;
	    }
	    // Search, in order:
	    // 1. The driver built like we are built, if modes are available
	    // 2. The driver built with modes that is not the way we were built
	    // 3. The driver built without modes
	    std::string lib;
	    if (!checkLibPath(lib, libDir, ezxml_name(d), true, OCPI_DEBUG) &&
		!checkLibPath(lib, libDir, ezxml_name(d), true, !OCPI_DEBUG) &&
		!checkLibPath(lib, libDir, ezxml_name(d), false, OCPI_DEBUG)) {
	      OU::format(err,
			 "could not find the \"%s\" \"%s\" driver in directory \"%s\", e.g.: %s",
			 d->name, m->name().c_str(), libDir.c_str(), lib.c_str());
	      break;
	    }
	    ocpiInfo("Loading the \"%s\" \"%s\" driver from \"%s\"",
		     d->name, m->name().c_str(), lib.c_str());
	    std::string lme;
	    if (!OS::LoadableModule::load(lib.c_str(), true, lme)) {
	      OU::format(err, "error loading the \"%s\" \"%s\" driver from \"%s\": %s",
			 d->name, m->name().c_str(), lib.c_str(), lme.c_str());
	      break;
	    }
	  }
	}
      }
      if (!err.empty())
	throw OU::Error("Error processing system configuration file \"%s\": %s",
			configFile.c_str(), err.c_str());
      // Now perform the configuration process, where managers and their children can do
      // things they would not do earlier, at static construction time.
      ocpiDebug("Configuring the driver managers");
      for (Manager *m = firstChild(); m; m = m->nextChild()) {
	ocpiDebug("Configuring the %s manager", m->name().c_str());
	m->configure(m_xml ? ezxml_cchild(m_xml, m->name().c_str()) : NULL);
      }
      // The discovery happens in a second pass to make sure everything is configured before
      // anything is discovered so that one driver's discovery can depend on another type of
      // driver's configuration.
      if (!m_doNotDiscover)
	for (Manager *m = firstChild(); m; m = m->nextChild())
	  if (m->shouldDiscover()) {
	    ocpiDebug("Performing discovery for the %s manager", m->name().c_str());
	    m->discover(params);
	  }
    }
    // Cleanup all managers
    bool ManagerManager::s_exiting = false;
    void ManagerManager::cleanup() {
      s_exiting = true;
      ManagerManager *mm = &OU::Singleton<ManagerManager>::getSingleton();
      // Before simply deleting the managermanager, we delete the
      // managers in order of their "cleanupPosition()"
      for (unsigned i = 0; i < 10; i++)
	for (Manager *m = mm->firstChild(); m; ) {
	  Manager *d = m;
	  m = m->nextChild();
	  if (d->cleanupPosition() == i)
	    delete d;
	}
      OCPI::Time::Emit::shutdown();
      delete mm;
    }
    static void exitbad(const char *e) {
      static const char msg[] = "\n*********During shutdown********\n";
      write(2, e, strlen(e));
      write(2, msg, sizeof(msg));
      _exit(1);
    }
    // A static-destructor hook to perform manager cleanup.
    static class cleanup {
    public:
      ~cleanup() {
	try {
	  ManagerManager::cleanup();
	} catch (std::string &e) {
	  exitbad(e.c_str());
	} catch (const char *e) {
	  exitbad(e);
	} catch (...) {
	  exitbad("Unexpected exception");
	}
      }
    } staticCleanup;
    Driver::Driver() : m_config(NULL) {}
    // Default implementation for a driver is to configure devices that exist
    // at configuration time.
    void Driver::configure(ezxml_t xml) {
      if (xml) {
	m_config = xml;
	for (ezxml_t dx = ezxml_cchild(xml, "device"); dx; dx = ezxml_next(dx))
	  for (Device *d = firstDeviceBase(); d; d = d->nextDeviceBase())
	    if (d->name() == ezxml_name(dx))
	      d->configure(dx);
      }
    }
    ezxml_t Driver::getDeviceConfig(const char *argName) {
      ocpiDebug("getDeviceConfig driver '%s' for device '%s' m_config %p", name().c_str(), argName, m_config);
      for (ezxml_t dx = ezxml_cchild(m_config, "device"); dx; dx = ezxml_next(dx)) {
	const char *devName = ezxml_cattr(dx, "name");
	if (devName && !strcmp(argName, devName))
	  return dx;
      }
      return NULL;
    }
    Driver::~Driver(){}
    const char *device = "device"; // template argument
    void Device::configure(ezxml_t xml) { (void)xml;}
    Device::~Device(){}
  }
  namespace API {
    void DriverManager::configure(const char *cf) {
      OCPI::Driver::ManagerManager::configure(cf);
    }
  }
}
