
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
#include <OcpiOsMutex.h>
#include <OcpiOsThreadManager.h>
#include <OcpiOsMisc.h>
#include "OcpiUtilTest.h"

namespace MutexTests {

  /*
   * ----------------------------------------------------------------------
   * Test 1: Locking and Unlocking Mutex
   * ----------------------------------------------------------------------
   */

  class Test1 : public OCPI::Util::Test::Test {
  public:
    Test1 ()
      : OCPI::Util::Test::Test ("Locking and unlocking mutex")
    {
    }

    void run ()
    {
      OCPI::OS::Mutex m;
      m.lock ();
      m.unlock ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Attempting to lock mutex in another thread
   * ----------------------------------------------------------------------
   */

  struct Test2Data {
    OCPI::OS::Mutex m;
    bool flag;
  };

  void
  test2Thread (void * opaque)
  {
    Test2Data * t2d = reinterpret_cast<Test2Data *> (opaque);
    t2d->m.lock ();
    t2d->flag = true;
    t2d->m.unlock ();
  }

  class Test2 : public OCPI::Util::Test::Test {
  public:
    Test2 ()
      : OCPI::Util::Test::Test ("Attempting to lock mutex in another thread")
    {
    }

    void run ()
    {
      Test2Data t2d;
      t2d.flag = false;
      t2d.m.lock ();
      OCPI::OS::ThreadManager tm (test2Thread, &t2d);
      test (!t2d.flag);
      t2d.m.unlock ();
      tm.join ();
      test (t2d.flag);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Using a recursive mutex
   * ----------------------------------------------------------------------
   */

  struct Test3Data {
    Test3Data ();
    OCPI::OS::Mutex m;
    bool flag;
  };

  Test3Data::Test3Data ()
    : m (true)
  {
  }

  void
  test3Thread (void * opaque)
  {
    Test3Data * t3d = reinterpret_cast<Test3Data *> (opaque);
    t3d->m.lock ();
    t3d->flag = true;
    t3d->m.unlock ();
  }

  class Test3 : public OCPI::Util::Test::Test {
  public:
    Test3 ()
      : OCPI::Util::Test::Test ("Using a recursive mutex")
    {
    }

    void run ()
    {
      Test3Data t3d;
      t3d.flag = false;
      t3d.m.lock ();
      t3d.m.lock ();
      OCPI::OS::ThreadManager tm (test3Thread, &t3d);
      t3d.m.unlock ();
      OCPI::OS::sleep (100); // give the other thread a chance to run
      test (!t3d.flag);
      t3d.m.unlock ();
      tm.join ();
      test (t3d.flag);
    }
  };

}

static
int
testMutexInt (int, char *[])
{
  OCPI::Util::Test::Suite tests ("Mutex tests");
  int n_failed;
  tests.add_test (new MutexTests::Test1);
  tests.add_test (new MutexTests::Test2);
  tests.add_test (new MutexTests::Test3);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testMutex (int argc, char * argv[])
  {
    return testMutexInt (argc, argv);
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

  return testMutexInt (argc, argv);
}
