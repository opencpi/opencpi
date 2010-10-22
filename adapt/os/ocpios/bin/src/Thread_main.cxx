
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


#include <OcpiOsDebug.h>
#include <OcpiOsThreadManager.h>
#include "OcpiUtilTest.h"

namespace ThreadTests {

  /*
   * ----------------------------------------------------------------------
   * Test 1
   * ----------------------------------------------------------------------
   */

  bool simpleThreadFlag = false;

  void
  simpleThread (void *)
  {
    simpleThreadFlag = true;
  }

  class Test1 : public OCPI::Util::Test::Test {
  public:
    Test1 ()
      : OCPI::Util::Test::Test ("Running and joining a simple thread")
    {
    }

    void run ()
    {
      simpleThreadFlag = false;
      OCPI::OS::ThreadManager tm (simpleThread, 0);
      tm.join ();
      test (simpleThreadFlag);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2
   * ----------------------------------------------------------------------
   */

  void * simpleThread2Parameter = 0;

  void
  simpleThread2 (void * opaque)
  {
    simpleThread2Parameter = opaque;
  }

  class Test2 : public OCPI::Util::Test::Test {
  public:
    Test2 ()
      : OCPI::Util::Test::Test ("Passing a parameter to a thread")
    {
    }

    void run ()
    {
      simpleThread2Parameter = 0;
      OCPI::OS::ThreadManager tm (simpleThread2, (void *) (simpleThread2));
      tm.join ();
      test (simpleThread2Parameter == (void *) simpleThread2);
    }
  };

}

static
int
testThreadInt (int, char *[])
{
  OCPI::Util::Test::Suite tests ("Thread tests");
  int n_failed;
  tests.add_test (new ThreadTests::Test1);
  tests.add_test (new ThreadTests::Test2);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testThread (int argc, char * argv[])
  {
    return testThreadInt (argc, argv);
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

  return testThreadInt (argc, argv);
}
