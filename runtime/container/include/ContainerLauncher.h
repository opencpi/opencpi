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

// Launchers manage a set of containers that are controlled from the same processor.
// A local launcher is simply an object through which local containers are managed.
// If the containers are remote, then there is a "remote launcher" that is acting as a
// client for a container server, which itself will use a local launcher for the containers
// it is serving
#ifndef CONTAINER_LAUNCHER_H
#define CONTAINER_LAUNCHER_H
#include <string>

#include "OcpiUtilMisc.h" // Singleton
#include "OcpiUtilValue.h"
#include "OcpiLibraryManager.h"
#include "OcpiContainerApi.h"

namespace OCPI {
  namespace Container {
    // This is a base class
    class Application;
    class Port;
    class Container;
    class Worker;
    class Launcher {
      // This instance class contains the minimal amount needed for local launching.
    public:
      struct Instance {
	Application *m_containerApp;
	Container *m_container;       // ptr since set after construction
	std::string m_name;                            // if local, copied from assembly
	const OCPI::Library::Implementation *m_impl;   // ptr since set after construction
	std::vector<OCPI::Util::Value> m_propValues;   // Array of property values to set
	std::vector<unsigned> m_propOrdinals;          // Array of property ordinals
	bool m_hasMaster, m_doneInstance;
	Instance *m_slave;
	Worker *m_worker;
	Instance();
      };
      typedef std::vector<Instance> Instances;
      // The instance object needed by the launcher
      struct Connection {
	Launcher *m_launchIn, *m_launchOut;
	Instance *m_instIn, *m_instOut;
	Port *m_input, *m_output;
	const char *m_nameIn, *m_nameOut, *m_url;
	OCPI::Util::PValueList m_paramsIn, m_paramsOut;
	std::string m_ipi, m_fpi, m_iui, m_fui;
	Connection();
	void prepare();
      };
      typedef std::vector<Connection> Connections;
    protected:
      std::string m_name;
      bool m_more;
      Launcher() : m_more(true) {}
      virtual ~Launcher() {}
    public:
      bool notDone() const { return m_more; }
      virtual bool
	launch(Launcher::Instances &instances, Launcher::Connections &connections) = 0,
	work(Launcher::Instances &instances, Launcher::Connections &connections) = 0;
    };
    // Concrete class that will be a singleton
    class LocalLauncher : public Launcher, public OCPI::Util::Singleton<LocalLauncher> {
      void createWorker(Launcher::Instance &i);
    public:
      virtual ~LocalLauncher();
      bool launch(Launcher::Instances &instances, Launcher::Connections &connections);
      bool work(Launcher::Instances &instances, Launcher::Connections &connections);
    };
  }
}
#endif
