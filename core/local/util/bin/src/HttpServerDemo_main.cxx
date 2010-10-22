
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

#include <OcpiLoggerLogger.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiLoggerOStreamOutput.h>
#include <OcpiUtilHttpServer.h>
#include <OcpiUtilTcpServer.h>
#include <OcpiUtilFileFs.h>
#include <OcpiOsFileSystem.h>
#include <iostream>
#include <string>
#include <cstdlib>

int
main (int argc, char *argv[])
{
  if (argc != 1 && argc != 2) {
    std::cout << "usage: " << argv[0] << std::endl;
    return 1;
  }

  unsigned int portNo = 0;

  if (argc == 2) {
    portNo = std::strtoul (argv[1], 0, 0);
  }

  try {
    std::string currentDir = OCPI::OS::FileSystem::cwd ();
    OCPI::Logger::OStreamOutput logger (std::cout);
    OCPI::Util::FileFs::FileFs localFs (currentDir);
    OCPI::Util::Http::Server server (&localFs, &logger);
    OCPI::Util::Tcp::Server serverPort (portNo, true);

    OCPI::Logger::debug ("All", 42);

    std::cout << "Running on port "
              << serverPort.getPortNo()
              << "."
              << std::endl;

    OCPI::Util::Tcp::Stream * stream;

    while ((stream = serverPort.accept())) {
      server.resetConn (stream, stream);
      server.run ();
      delete stream;
    }
  }
  catch (const std::string & oops) {
    std::cout << "Oops: " << oops << std::endl;
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
  }

  return 0;
}
