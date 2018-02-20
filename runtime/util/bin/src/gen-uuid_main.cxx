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

#include <iostream>
#include <cstring>
#include <OcpiOsDebug.h>
#include "OcpiUtilUUID.h"

static
int
genUUIDInt (int, char *[])
{
  OCPI::Util::UUID::BinaryUUID uuid = OCPI::Util::UUID::produceRandomUUID ();
  std::cout << OCPI::Util::UUID::binaryToHex (uuid) << std::endl;
  return 0;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  genUUID (int argc, char * argv[])
  {
    return genUUIDInt (argc, argv);        ;
  }
}

/*
 * Entrypoint for everybody else.
 */

int
main (int argc, char * argv[])
{
#if !defined (NDEBUG)
  {
    for (int i=1; i<argc; i++) {
      if (std::strcmp (argv[i], "--break") == 0) {
        OCPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return genUUIDInt (argc, argv);
}
