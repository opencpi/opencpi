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
#ifndef OCPI_APPLICATION_LAUNCHER_H
#define OCPI_APPLICATION_LAUNCHER_H
#include <OcpiUtilValue.h>
#include <OcpiContainerApplication.h>
#include <OcpiContainerPort.h>
namespace OCPI {
  namespace Application {
    // This is a base class
    class Launcher {
      // This instance class contains the minimal amount needed for local launching.
    public:
      struct Instance {
	OCPI::Container::Application *m_containerApp;
	OCPI::Container::Container *m_container;       // ptr since set after construction
	std::string m_name;                            // if local, copied from assembly
	const OCPI::Library::Implementation *m_impl;   // ptr since set after construction
	std::vector<OCPI::Util::Value> m_propValues;   // Array of property values to set
	std::vector<unsigned> m_propOrdinals;          // Array of property ordinals
	bool m_hasMaster, m_doneInstance;
	Instance *m_slave;
	OCPI::Container::Worker *m_worker;
	Instance();
      };
      typedef std::vector<Instance> Instances;
      // The instance object needed by the launcher
      struct Connection {
	Launcher *m_launchIn, *m_launchOut;
	Instance *m_instIn, *m_instOut;
	OCPI::API::Port *m_input, *m_output;
	const char *m_nameIn, *m_nameOut, *m_url;
	const OCPI::API::PValue *m_paramsIn, *m_paramsOut;
	std::string m_ipi, m_fpi, m_iui, m_fui;
	Connection();
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
    class RemoteLauncher : public LocalLauncher {
      int m_fd;              // socket fd
      bool m_sending;        // Is next phase to send something?
      std::string m_request; // xml text request being constructed
      std::vector<char> m_response;      // char buffer of received response
      ezxml_t m_rx;          // parsed xml of received response
      const char *m_tag;     // tag of current request
      std::set<const OCPI::Library::Artifact *> m_artifacts;
      std::set<const Launcher::Instance *> m_instances;
      std::set<const Launcher::Connection *> m_connections;
      virtual ~RemoteLauncher();
      void send();
      void receive();
      void emitArtifact(const OCPI::Library::Artifact &art);
      void emitInstance(const char *name, size_t artN, const Launcher::Instance &i,
			const char *slave);
      void emitPort(const Launcher::Instance &i, const char *port,
		    const OCPI::API::PValue *params, const char *which);
      void emitConnection(const Launcher::Connection &c);
      void emitConnectionUpdate(Launcher::Connection &c, const char *iname, std::string &sinfo);
      void loadArtifact(ezxml_t ax); // Just push the bytes down the pipe, getting a response for each.
      void updateInstance(Launcher::Instances &instances, ezxml_t ix);
      void updateConnection(ezxml_t cx);
    public:
      bool launch(Launcher::Instances &instances, Launcher::Connections &connections);
      bool work(Launcher::Instances &instances, Launcher::Connections &connections);
    };
    extern bool
      receiveXml(int fd, ezxml_t &rx, std::vector<char> &buf, std::string &error),
      sendXml(int fd, const std::string &buf, const char *msg, std::string &error);
  }
}
#endif
