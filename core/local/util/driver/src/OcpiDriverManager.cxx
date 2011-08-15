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
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilMisc.h"
#include "OcpiDriverManager.h"
#include "OcpiDriverApi.h"
namespace OCPI {
  namespace Driver {
    void debug_hook() {} // easier static constructor debug
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
	    throw OCPI::Util::ApiError("Duplicate XML driver element for: "
				       "\"%s\"", d->name().c_str(), NULL);
	  else
	    found = c;
	d->configure(found);
      }
    }
    unsigned Manager::cleanupPosition() { return 1; }
   // Get the singleton ManagerManager, possibly constructing it.
    ManagerManager *ManagerManager::getManagerManager() {
      return &Singleton<ManagerManager>::getSingleton();
    }
    ManagerManager::ManagerManager()
      : m_configured(false)
    {}
    // This is the static API method
    void ManagerManager::configure(const char *file) {
      getManagerManager()->configureOnce(file);
    }
    static void loadit(ezxml_t load) {
      (void)load;
    }

    // This is NOT a static method
    void ManagerManager::configureOnce(const char *file) {
      if (m_configured)
	return; // fast path without mutex
      OCPI::Util::AutoMutex guard(m_mutex); 
      if (!m_configured) {
	m_configured = true;
	bool optional = false;
	if (!file)
	  file = getenv("OCPI_PLATFORM_CONFIG");
	if (!file) {
	  file = "/opt/opencpi/platform.xml";
	  optional = true;
	}
	ezxml_t x = ezxml_parse_file(file);
	if (!x && !optional)
	  throw OCPI::Util::ApiError("OpenCPI system configuration file: \"%s\""
				     " couldn't be opened", file);
	// First perform any top-level loads.
	if (x) {
	  for (ezxml_t l = ezxml_cchild(x, "load"); l; l = ezxml_next(l))
	    loadit(l);
	  // Now find any loads under any of the managers
	  for (Manager *m = firstChild(); m; m = m->nextChild()) {
	    ezxml_t c = ezxml_cchild(x, m->name().c_str());
	    if (c)
	      for (ezxml_t l = ezxml_cchild(c, "load"); l; l = ezxml_next(l))
		loadit(l);
	  }
	}
	// Now perform the configuration process, where managers and their children can do
	// things they would not do earlier, at static construction time.
	for (Manager *m = firstChild(); m; m = m->nextChild()) {
	  ezxml_t c = x ? ezxml_cchild(x, m->name().c_str()) : NULL;
	  m->configure(c);
	  m->discover();
	}
      }
    }
    // Cleanup all managers
    void ManagerManager::cleanup() {
      ManagerManager *mm = &Singleton<ManagerManager>::getSingleton();
      // Before simply deleting the managermanager, we delete the
      // managers in order of their "cleanupPosition()"
      for (unsigned i = 0; i < 10; i++)
	for (Manager *m = mm->firstChild(); m; ) {
	  Manager *d = m;
	  m = m->nextChild();
	  if (d->cleanupPosition() == i)
	    delete d;
	}
      delete mm;
    }
    // A static-destructor hook to perform manager cleanup.
    static class cleanup {
    public:
      ~cleanup() {
	ManagerManager::cleanup();
      }
    } x;
    Driver::Driver() : m_config(NULL) {}
    // Default implementation for a driver is to configure devices that exist
    // at configuration time.
    void Driver::configure(ezxml_t x) {
      if (x) {
	m_config = x;
	for (ezxml_t dx = ezxml_cchild(x, "device"); dx; dx = ezxml_next(dx))
	  for (Device *d = firstDeviceBase(); d; d = d->nextDeviceBase())
	    if (d->name() == ezxml_name(dx))
	      d->configure(dx);
      }
    }
    ezxml_t Driver::getDeviceConfig(const char *name) {
      for (ezxml_t dx = ezxml_cchild(m_config, "device"); dx; dx = ezxml_next(dx))
	if (!strcmp(name, dx->name))
	  return dx;
      return NULL;
    }
    Driver::~Driver(){}
    const char *device = "device"; // template argument
    void Device::configure(ezxml_t x) { (void)x;}
    Device::~Device(){}
  }
  namespace API {
    void DriverManager::configure(const char *cf) {
      OCPI::Driver::ManagerManager::configure(cf);
    }
  }
}
