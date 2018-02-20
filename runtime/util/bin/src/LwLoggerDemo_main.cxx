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

#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <OcpiLoggerLogger.h>
#include <OcpiUtilIIOP.h>
#include <OcpiUtilLwLoggerOutput.h>

#if !defined (NDEBUG)
#include <OcpiOsDebug.h>
#endif

void
usage (const char * argv0)
{
  std::cout << "usage: " << argv0 << " <IOR|file|-> <message>" << std::endl;
  std::cout << "    <IOR>   Uses this IOR." << std::endl;
  std::cout << "    <file>  Reads IOR from file." << std::endl;
  std::cout << "    -       Reads IOR from standard input." << std::endl;
}

int
main (int argc, char *argv[])
{
  int iorpos=0, msgpos=0;

#if !defined (NDEBUG)
  for (int i=1; i<argc; i++) {
    if (std::strcmp (argv[i], "--break") == 0) {
      OCPI::OS::debugBreak ();
      break;
    }
  }
#endif

  for (int cmdidx=1; cmdidx<argc; cmdidx++) {
    if (std::strcmp (argv[cmdidx], "--help") == 0) {
      usage (argv[0]);
      return 0;
    }
    else if (std::strcmp (argv[cmdidx], "--break") == 0) {
      // handled above
    }
    else if (!iorpos) {
      iorpos = cmdidx;
    }
    else if (!msgpos) {
      msgpos = cmdidx;
    }
    else {
      usage (argv[0]);
      return 1;
    }
  }

  if (!iorpos || !msgpos) {
    usage (argv[0]);
    return 0;
  }

  std::string stringifiedIOR;

  if (std::strncmp (argv[iorpos], "IOR:", 4) == 0 ||
      std::strncmp (argv[iorpos], "ior:", 4) == 0) {
    stringifiedIOR = argv[iorpos];
  }
  else if (std::strcmp (argv[iorpos], "-") == 0) {
    std::getline (std::cin, stringifiedIOR);
  }
  else {
    std::ifstream ifs (argv[iorpos]);

    if (!ifs.good()) {
      std::cout << "oops: can not open \"" << argv[iorpos] << " for reading."
                << std::endl;
      return 1;
    }

    std::getline (ifs, stringifiedIOR);
  }

  OCPI::Util::IOP::IOR ior;

  try {
    ior = OCPI::Util::IOP::string_to_ior (stringifiedIOR);
  }
  catch (const std::string & oops) {
    std::cout << "oops: " << oops << "." << std::endl;
    return 1;
  }

  OCPI::Util::LwLoggerOutput logger (ior);

  logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
         << OCPI::Logger::ProducerName ("main")
         << argv[msgpos]
         << std::flush;

  return 0;
}
