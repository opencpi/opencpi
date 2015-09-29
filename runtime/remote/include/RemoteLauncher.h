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
      void emitCrew(const OCPI::Container::Launcher::Crew &crew);
      void emitMember(const char *name, unsigned contN, unsigned artN, unsigned crewN,
		      const Launcher::Member &i, int slave);
      void emitSide(const Launcher::Members &members, Launcher::Port &p, bool input);
      void emitConnection(const Launcher::Members &members, Launcher::Connection &c);
      void emitConnectionUpdate(unsigned nConn, const char *iname, std::string &sinfo);
      void loadArtifact(ezxml_t ax); // Just push the bytes down the pipe, getting a response for each.
      void updateConnection(ezxml_t cx);
    public:
      bool
	wait(unsigned remoteInstance, OCPI::OS::ElapsedTime timeout),
	launch(Launcher::Members &members, Launcher::Connections &connections),
	work(Launcher::Members &members, Launcher::Connections &connections);
      OCPI::Util::Worker::ControlState getState(unsigned remoteInstance);
      void
	controlOp(unsigned remoteInstance, OU::Worker::ControlOperation),
	setPropertyValue(unsigned remoteInstance, size_t propN, std::string &v),
	getPropertyValue(unsigned remoteInstance, size_t propN, std::string &v, bool hex,
			 bool add);
      static void
	encodeDescriptor(const char *iname, const std::string &s, std::string &out),
	decodeDescriptor(const char *info, std::string &s);

      static bool
	receiveXml(int fd, ezxml_t &rx, std::vector<char> &buf, bool &eof, std::string &error),
	sendXml(int fd, std::string &buf, const char *msg, std::string &error);
    };
  }
}
#endif
