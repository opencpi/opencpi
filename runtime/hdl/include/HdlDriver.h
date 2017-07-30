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

#ifndef HdlDriver_H
#define HdlDriver_H

#include <string>
#include "HdlSimDriver.h"
#include "HdlLSimDriver.h"
#include "HdlBusDriver.h"
#include "HdlEtherDriver.h"
#include "HdlPciDriver.h"
#include "ContainerManager.h"

namespace OCPI {
  namespace HDL {
    extern const char *hdl;

    class Container;
    class Driver
      : public OCPI::Container::DriverBase<Driver, Container, hdl>,
	OCPI::HDL::PCI::Driver,
	OCPI::HDL::Ether::Driver,
	OCPI::HDL::Sim::Driver,
	OCPI::HDL::LSim::Driver,
	OCPI::HDL::Zynq::Driver,
	Access, // for temporary probing
	virtual protected OCPI::Util::SelfMutex
    {
      const OCPI::Util::PValue *m_params; // a temporary during discovery
      bool setup(Device &dev, ezxml_t &config, std::string &err);
    public:
      void print(const char *name, Access &access);
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the
      // "found" method.
      unsigned search(const OCPI::API::PValue*, const char **exclude, bool discoveryOnly);
      bool found(Device &dev, const char **excludes, bool discoveryOnly, std::string &error);
      // Probe a specific container
      OCPI::Container::Container *probeContainer(const char *which, std::string &error,
						 const OCPI::API::PValue *props);
      OCPI::HDL::Device * open(const char *which, bool discovery, bool forLoad,
			       const OCPI::API::PValue *params, std::string &err);
      void close();

      // Create an actual container.
      static OCPI::Container::Container *
      createContainer(Device &dev, ezxml_t config = NULL,
		      const OCPI::Util::PValue *params = NULL);
    };
  }
}
#endif
