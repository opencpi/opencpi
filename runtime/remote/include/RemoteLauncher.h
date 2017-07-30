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
#ifndef REMOTE_LAUNCHER_H
#define REMOTE_LAUNCHER_H
#include <set>
#include <vector>
#include <string>

#include "OcpiOsSocket.h"
#include "ContainerLauncher.h"
namespace OCPI {
  namespace Remote {
    class Launcher : public OCPI::Container::LocalLauncher {
      int m_fd;              // socket fd
      bool m_sending;        // Is next phase to send something?
      std::string m_request; // xml text request being constructed
      std::vector<char> m_response;      // char buffer of received response
      ezxml_t m_rx;          // parsed xml of received response
      // A map from global instance to remote instance
      std::vector<unsigned> m_instanceMap;
      std::vector<unsigned> m_connectionMap;
      // These maps are indexed by the remote server according to the position in the
      // launching XML we sent them
      std::vector<OCPI::Container::Launcher::Connection *> m_connections;
      std::vector<OCPI::Library::Artifact *> m_artifacts;
    protected:
      Launcher(OCPI::OS::Socket &socket);
      virtual ~Launcher();
    private:
      virtual const std::string &name() const = 0;
      void send();
      void receive();
      void emitContainer(const OCPI::Container::Container &cont);
      void emitArtifact(const OCPI::Library::Artifact &art);
      void emitInstance(const char *name, unsigned contN, unsigned artN, const Launcher::Instance &i, int slave);
      void emitPort(const Launcher::Instance &i, const char *port,
		    const OCPI::API::PValue *params, const char *which);
      void emitConnection(const Launcher::Instances &instances, const Launcher::Connection &c);
      void emitConnectionUpdate(unsigned nConn, const char *iname, std::string &sinfo);
      void loadArtifact(ezxml_t ax); // Just push the bytes down the pipe, getting a response for each.
      void updateConnection(ezxml_t cx);
    public:
      bool
	wait(unsigned remoteInstance, OCPI::OS::ElapsedTime timeout),
	launch(Launcher::Instances &instances, Launcher::Connections &connections),
	work(Launcher::Instances &instances, Launcher::Connections &connections);
      OCPI::Util::Worker::ControlState getState(unsigned remoteInstance);
      void
	controlOp(unsigned remoteInstance, OU::Worker::ControlOperation),
	setPropertyValue(unsigned remoteInstance, size_t propN, std::string &v),
	getPropertyValue(unsigned remoteInstance, size_t propN, std::string &v, bool hex,
			 bool add);
    };
  }
}
#endif
