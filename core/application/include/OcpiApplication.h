
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


/*
 * Definitions for the top level application, whose workers are executed on a
 * variety of containers.  The "Application" object is a runtime thing that is an instance of
 * an assembly.  Thus the assembly is the template for the application.
 *
 * This file is NOT an exposed API file.
 */
#ifndef OCPI_APPLICATION_H
#define OCPI_APPLICATION_H

#include <string>
#include <map>
#include "OcpiLibraryAssembly.h"
#include "OcpiContainerManager.h"
#include "OcpiContainerApplication.h"
#include "OcpiApplicationApi.h"

namespace OCPI {
  namespace API {

    class ApplicationI : public OCPI::Container::Callback {
      typedef OCPI::Container::Container::CMap CMap;
      const OCPI::Library::Assembly &m_assembly;
      bool m_ownAssembly;

      struct Instance {
	const OCPI::Library::Implementation *m_impl; // The chosen, best implementation
	unsigned m_container;                        // LOCAL ordinal - among those we are using
	OCPI::Util::Value *m_propValues;             // the parsed property values to set
	unsigned *m_propOrdinals;
	Instance();
	~Instance();
      } *m_instances;

      // This class represents a mapping from an externally visible property of the assembly
      // to an individual property of an instance. It must be at this layer
      // (not util::assembly or library::assembly) because it potentially depends on the 
      // implementation specific properties of the implementations selected.
      struct Property {
	std::string m_name;  // qualified name:   instance.property
	unsigned m_instance; // ordinal of instance in assembly
	unsigned m_property; // ordinal of property in implememtation of instance
      } *m_properties;
      unsigned m_nProperties;
      // The bits in the cmap show which containers are possible for that candidate implementation
      CMap m_curMap;   // A temporary that accumulates containers for a candidate
      CMap m_allMap;   // A map of all containers chosen/used
      unsigned m_nContainers;                         // how many containers have been used
      unsigned *m_usedContainers;                     // per used container, its container ordinal (global)
      // Now the runtime state.
      OCPI::Container::Container **m_containers;      // the actual containers we are using
      OCPI::Container::Application **m_containerApps; // per used container, the container app
      // External ports - recorded until we know whether it will be a ExternalPort, or a remote port
      struct External {
	Port &m_port; // The internal worker port
	const PValue *m_params; //  Connection parameters from the OU::Assembly
	ExternalPort *m_external; // The external port created from connectExternal.
	inline External(Port &port, const PValue *params)
	  : m_port(port), m_params(params), m_external(NULL) {}
      };
      typedef std::map<const char*, External, OCPI::Library::Comp> Externals;
      typedef std::pair<const char*, External> ExternalPair;
      Externals m_externals;
      OCPI::Container::Worker **m_workers;
      OCPI::Container::ExternalPort **m_externalPorts;
      const char **m_externalNames;
      void init();
    public:
      explicit ApplicationI(const char *file);
      explicit ApplicationI(const std::string &string);
      explicit ApplicationI(const OCPI::Library::Assembly &);
      ~ApplicationI();
      bool foundContainer(OCPI::Container::Container &i);
      void initialize();
      void start();
      void stop();
      void wait();
      ExternalPort &getPort(const char *);
      bool getProperty(unsigned ordinal, std::string &name, std::string &value);
      // This will be used for the port connection protocol
      OCPI::Container::Port &getRemote(const char *);
    };
  }
}
#endif
