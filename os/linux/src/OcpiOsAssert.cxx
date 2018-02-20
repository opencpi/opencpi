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
#include <cstdlib>
#include "OcpiOsAssert.h"
#include "OcpiOsDebug.h"

namespace {

  OCPI::OS::AssertionCallback g_assertionCallback = 0;

  void
  defaultAssertionCallback (const char * cond,
                            const char * file,
                            unsigned int line)
  {
    std::cerr << std::endl
              << "Assertion failed: " << cond << " is false at "
              << file << ":" << line << "."
              << std::endl;

#if !defined (NDEBUG)
    std::cerr << "Stack Dump:"
              << std::endl;
    OCPI::OS::dumpStack (std::cerr);
    OCPI::OS::debugBreak ();
#endif

    std::abort ();
  }

}

void
OCPI::OS::setAssertionCallback (AssertionCallback cb)
  throw ()
{
  g_assertionCallback = cb;
}

bool
OCPI::OS::assertionFailed (const char * cond, const char * file, unsigned int line)
  throw ()
{
  if (g_assertionCallback) {
    (*g_assertionCallback) (cond, file, line);
  }
  else {
    defaultAssertionCallback (cond, file, line);
  }

  // the assertion callback shall not return, so we should not be here.
  std::abort ();
  return false;
}
