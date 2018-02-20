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

#ifndef HDL_SIM_SERVER_H
#define HDL_SIM_SERVER_H
#include "HdlOCCP.h"
namespace OCPI {
  namespace HDL {
    namespace Sim {
      struct Sim;
      class Server {
	Sim *m_sim;
	std::string m_simDir;
      public:
	Server(const char *name, const std::string &platform, uint8_t spinCount,
	       unsigned sleepUsecs, unsigned simTicks, bool verbose, bool dump, bool isPublic,
	       std::string &error);
	~Server();
	bool run(const std::string &exec, std::string &error);
	static void initAdmin(OCPI::HDL::OccpAdminRegisters &admin, const char *platform,
			      HdlUUID &hdlUuid, OCPI::Util::UuidString *textUUID = NULL);
      };
    }
  }
}
#endif

