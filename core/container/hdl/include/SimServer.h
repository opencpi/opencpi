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
#ifndef SIM_SERVER_H
#define SIM_SERVER_H
#include "HdlOCCP.h"
namespace OCPI {
  namespace HDL {
    namespace Sim {
      struct Sim;
      class Server {
	Sim *m_sim;
	std::string m_simDir;
      public:
	Server(const char *name, const std::string &platform, unsigned spinCount,
	       unsigned sleepUsecs, unsigned simTicks, bool verbose, bool dump, std::string &error);
	~Server();
	bool run(const std::string &exec, std::string &error);
	static void initAdmin(OCPI::HDL::OccpAdminRegisters &admin, const char *platform,
			      uuid_string_t *textUUID = NULL);
      };
    }
  }
}
#endif

