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
// Launchers manage a set of containers that are controlled from the same processor.
// A local launcher is simply an object through which local containers are managed.
// If the containers are remote, then there is a "remote launcher" that is acting as a
// client for a container server, which itself will use a local launcher for the containers
// it is serving
#ifndef CONTAINER_LAUNCHER_H
#define CONTAINER_LAUNCHER_H
#include <string>

#include "OcpiContainerApi.h"
#include "OcpiUtilValue.h"
#include "OcpiLibraryManager.h"

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
	size_t m_crewSize, m_member;
	Instance();
      };
      typedef std::vector<Instance> Instances;
      // The instance object needed by the launcher
      // FIXME: create a "Launcher::Port" here...
      struct Port {
	Launcher *m_launcher;
	Instance *m_instance;
	OCPI::API::Port *m_port;
	const char *m_name;
	OCPI::Util::PValueList m_params;
	OCPI::Util::Port *m_metaPort;
	std::string m_initial, m_final;
	Port();
      };
      struct Connection {
	Port m_in, m_out;
	const char *m_url;
	Connection();
	void prepare();
      };
      typedef std::vector<Connection> Connections;
    protected:
      std::string m_name;
      bool m_more;
      Launcher() : m_more(true) {}
    public:
      bool notDone() const { return m_more; }
      virtual bool
	launch(Launcher::Instances &instances, Launcher::Connections &connections) = 0,
	work(Launcher::Instances &instances, Launcher::Connections &connections) = 0;
    };
    // Concrete class that will be a singleton
    class LocalLauncher : public Launcher, public OCPI::Driver::Singleton<LocalLauncher> {
      void createWorker(Launcher::Instance &i);
    public:
      bool launch(Launcher::Instances &instances, Launcher::Connections &connections);
      bool work(Launcher::Instances &instances, Launcher::Connections &connections);
    };
  }
}
#endif
