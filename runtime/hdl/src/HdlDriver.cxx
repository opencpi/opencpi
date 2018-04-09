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

/*
 * This file contains driver-level code that does not know about the guts of the 
 * container class.
 */
#include "ContainerManager.h"
#include "HdlContainer.h"
#include "HdlDriver.h"

namespace OCPI {
  namespace HDL {
    namespace OC = OCPI::Container;
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OP = OCPI::HDL::PCI;


    const char *hdl = "hdl";

    OCPI::HDL::Device *Driver::
    open(const char *name, bool discovery, bool forLoad, const OA::PValue *params,
	 std::string &err) {
      parent().parent().configureOnce();
      lock();
      // FIXME: obviously this should be registered and dispatched nicely..
      bool pci = false, ether = false, sim = false, bus = false, lsim = false;
      const char *which = name;
      if (!strncasecmp("PCI:", which, 4)) {
	pci = true;
	which += 4;
      } else if (!strncasecmp("pl:", which, 3)) {
	bus = true;
	which += 3;
      } else if (!strncasecmp("sim:", which, 4)) {
	sim = true;
	which += 4;
      } else if (!strncasecmp("lsim:", which, 5)) {
	lsim = true;
	which += 5;
      } else if (!strncasecmp("Ether:", which, 6)) {
	ether = true;
	which += 6;
      } else {
	unsigned n = 0;
	for (const char *cp = strchr(which, ':'); cp; n++, cp = strchr(cp+1, ':'))
	  ;
	if (n == 5)
	  ether = true;
	else
	  pci = true;
      }
      if (!which[0]) {
	OU::format(err, "Missing device name after prefix \"%s\"", name);
	return NULL;
      }
      Device *dev =
	pci ? PCI::Driver::open(which, params, err) : 
	bus ? Zynq::Driver::open(which, forLoad, params, err) : 
	ether ? Ether::Driver::open(which, discovery, params, err) :
	sim ? Sim::Driver::open(which, discovery, params, err) : 
	lsim ? LSim::Driver::open(which, params, err) : NULL;
      ezxml_t config;
      if (forLoad && bus)
	return dev;
      if (dev && !setup(*dev, config, err))
	return dev;
      delete dev;
      return NULL;
    }

    void Driver::
    close() {
      closeAccess();
      unlock();
    }

    // A device has been found (and created).  Its ownership has been passed in to us,
    // so we will delete it here if there is any error.
    // Return true if error or if we otherwise discarded the device
    // Assuming we are called possibly multiple times from a give driver's search method,
    // record the first error seen.
    bool Driver::
    found(Device &dev, const char **excludes, bool discoveryOnly, std::string &error) {
      ezxml_t config;
      error.clear();
      if (excludes)
	for (const char **ap = excludes; *ap; ap++)
	  if (!strcasecmp(*ap, dev.name().c_str()))
	    goto out;
      if (!setup(dev, config, error)) {
	bool printOnly = false;
	if ((OU::findBool(m_params, "printOnly", printOnly) && printOnly))
	  dev.print(); // fall through to delete
	else {
#if 0
	  if (dev.m_verbose)
	    dev.print();
#endif
	  if (!discoveryOnly)
	    createContainer(dev, config, m_params); // no errors?
	  return false;
	}
      }
    out:
      delete &dev;
      return true;
    } 

    unsigned Driver::
    search(const OA::PValue *params, const char **exclude, bool discoveryOnly) {
      OU::SelfAutoMutex x(this); // protect m_params etc.
      unsigned count = 0;
      m_params = params;
      std::string error;
      count += Zynq::Driver::search(params, exclude, discoveryOnly, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got Zynq search error: %s", error.c_str());
	error.clear();
      }
      count += Ether::Driver::search(params, exclude, discoveryOnly, false, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got ethernet search error: %s", error.c_str());
	error.clear();
      }
      count += PCI::Driver::search(params, exclude, discoveryOnly, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got PCI search error: %s", error.c_str());
	error.clear();
      }
      count += Sim::Driver::search(params, exclude, discoveryOnly, true, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got SIM/UDP search error: %s", error.c_str());
	error.clear();
      }
      count += LSim::Driver::search(params, exclude, discoveryOnly, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got LSIM search error: %s", error.c_str());
	error.clear();
      }
      return count;
    }
    
    OC::Container *Driver::
    probeContainer(const char *which, std::string &error, const OA::PValue *params) {
      Device *dev;
      ezxml_t config;
      if ((dev = open(which, false, false, params, error))) {
	if (setup(*dev, config, error))
	  delete dev;
	else
	  return createContainer(*dev, config, params);
      }
      if (error.size())
	ocpiBad("While probing %s: %s", which, error.c_str());
      return NULL;
    }      
    // Internal method common to "open" and "found"
    // Return true on error
    bool Driver::
    setup(Device &dev, ezxml_t &config, std::string &err) {
      // Get any specific configuration information for this device
      const char *l_name = dev.name().c_str();
      config = getDeviceConfig(l_name);
      if (!config && !strncmp("PCI:", l_name, 4)) // compatibility
	config = getDeviceConfig(l_name + 4);
      // Configure the device
      return dev.configure(config, err);
    }
    void Driver::initAdmin(OccpAdminRegisters &admin, const char *platform, HdlUUID &hdlUuid,
			   OU::UuidString *uuidString) {
      Device::initAdmin(admin, platform, hdlUuid, uuidString);
    }
    DirectWorker *Driver::
    createDirectWorker(Device &dev, const Access &cAccess, Access &wAccess, ezxml_t impl,
		       ezxml_t inst, const char *idx, unsigned timeout) {
      return new DirectWorker(dev, cAccess, wAccess, impl, inst, idx, timeout);
    }

    OC::RegisterContainerDriver<Driver> driver;
  }
}
