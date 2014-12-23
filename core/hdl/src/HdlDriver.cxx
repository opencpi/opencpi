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
    open(const char *which, bool discovery, bool forLoad, std::string &err) {
      parent().parent().configureOnce();
      lock();
      // FIXME: obviously this should be registered and dispatched nicely..
      bool pci = false, ether = false, sim = false, bus = false;
      if (!strncasecmp("PCI:", which, 4)) {
	pci = true;
	which += 4;
      } else if (!strncasecmp("pl:", which, 3)) {
	bus = true;
	which += 3;
      } else if (!strncasecmp("sim:", which, 4)) {
	sim = true;
	which += 4;
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
      Device *dev =
	pci ? PCI::Driver::open(which, err) : 
	bus ? Zynq::Driver::open(which, forLoad, err) : 
	ether ? Ether::Driver::open(which, discovery, err) :
	sim ? Sim::Driver::open(which, discovery, err) : NULL;
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

    // Return true on error or when the device is no longer needed
    // Caller is reponsible for deleting the device on error
    bool Driver::
    found(Device &dev,  std::string &error) {
      ezxml_t config;
      if (setup(dev, config, error))
	return true;
      bool printOnly;
      if (OU::findBool(m_params, "printOnly", printOnly) && printOnly) {
	dev.print();
	return true;
      }
      return createContainer(dev, config, m_params) == NULL;
    } 

    unsigned Driver::
    search(const OA::PValue *params, const char **exclude, bool discoveryOnly) {
      OU::SelfAutoMutex x(this); // protect m_params etc.
      unsigned count = 0;
      m_params = params;
      std::string error;
      count += Zynq::Driver::search(params, exclude, discoveryOnly, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got PL search error: %s", error.c_str());
	error.clear();
      }
      count += Ether::Driver::search(params, exclude, discoveryOnly, false, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got ethernet search error: %s", error.c_str());
	error.clear();
      }
      count += PCI::Driver::search(params, exclude, discoveryOnly, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got pci search error: %s", error.c_str());
	error.clear();
      }
      count += Sim::Driver::search(params, exclude, discoveryOnly, true, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got sim/udp search error: %s", error.c_str());
	error.clear();
      }
      return count;
    }
    
    OC::Container *Driver::
    probeContainer(const char *which, std::string &error, const OA::PValue *params) {
      Device *dev;
      ezxml_t config;
      if ((dev = open(which, false, false, error)))
	if (setup(*dev, config, error))
	  delete dev;
	else
	  return createContainer(*dev, config, params);
      if (error.size())
	ocpiBad("While probing %s: %s", which, error.c_str());
      return NULL;
    }      
    // Internal method common to "open" and "found"
    // Return true on error
    bool Driver::
    setup(Device &dev, ezxml_t &config, std::string &err) {
      // Get any specific configuration information for this device
      const char *name = dev.name().c_str();
      config = getDeviceConfig(name);
      if (!config && !strncmp("PCI:", name, 4)) // compatibility
	config = getDeviceConfig(name+4);
      // Configure the device
      return dev.configure(config, err);
    }

    OC::RegisterContainerDriver<Driver> driver;
  }
}
