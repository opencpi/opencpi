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
#include "OcpiRDTInterface.h"

namespace OCPI {
  namespace Container {
    // This is a base class
    class Application;
    class Port;
    class Container;
    class Worker;
    class LocalPort;

    // This structure describes what a container can do with connections that
    // go outside the container.  A container will offer these in preference order.
    // If a transport is only usable for one direction, the "role" for the other
    // direction will be NoRole;
    struct Transport {
      std::string   transport;     // transport driver/mechanism/protocol to move data
      std::string   id;            // the identity of the instance of the fabric/network
      OCPI::RDT::PortRole roleIn;  // what is the preferred role for input
      OCPI::RDT::PortRole roleOut; // what is the preferred role for output
      uint32_t optionsIn;          // available options for input
      uint32_t optionsOut;         // available options for output
    };
    typedef std::vector<Transport> Transports;

    class Launcher {
      // This instance class contains the minimal amount needed for local launching.
    public:
      // This structure shared by launch instances (members) in the same crew.
      struct Crew {
	size_t m_size;
	std::vector<OCPI::Util::Value> m_propValues;   // Array of property values to set
	std::vector<unsigned> m_propOrdinals;          // Array of property ordinals
	Crew();
      };
      struct Member {
	Application *m_containerApp;
	Container *m_container;  // note that this will be set for external ports
	std::string m_name;                            // if local, copied from assembly
	const OCPI::Library::Implementation *m_impl;   // ptr since set after construction
	bool m_hasMaster, m_doneInstance;
	Member *m_slave;
	Worker *m_worker;
	size_t m_member;
	Crew *m_crew;
	Member();
      };
      typedef std::vector<Member> Members;
      struct Port {
	Launcher *m_launcher;
	Container *m_container;
	Application *m_containerApp;
	const Member *m_member;
	LocalPort *m_port;
	const char *m_name;
	OCPI::Util::PValueList m_params;
	const OCPI::Util::Port *m_metaPort; // needed on a server for the port that is not local
	size_t m_scale, m_index;      // ditto
	const char *m_url;
	std::string m_initial, m_final;
	bool m_started; // the connection has passed its initial phase and initial info has been sent
	bool m_done;
	Port();
      };
      struct Connection {
	Port m_in, m_out;
	size_t m_bufferSize;   // negotiated/final
	Transport m_transport; // negotiated/final
	bool m_done;
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
	launch(Launcher::Members &members, Launcher::Connections &connections) = 0,
	work(Launcher::Members &members, Launcher::Connections &connections) = 0;
    };
    // Concrete class that will be a singleton
    class LocalLauncher : public Launcher, public OCPI::Driver::Singleton<LocalLauncher> {
      void createWorker(Launcher::Member &i);
    public:
      virtual ~LocalLauncher();
      bool launch(Launcher::Members &members, Launcher::Connections &connections);
      bool work(Launcher::Members &members, Launcher::Connections &connections);
    };
  }
}
#endif
