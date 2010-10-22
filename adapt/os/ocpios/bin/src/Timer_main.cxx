
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


#include <OcpiOsTimer.h>
#include <OcpiOsMisc.h>
#include <OcpiOsDebug.h>
#include "OcpiUtilTest.h"

namespace TimerTests {

  /*
   * ----------------------------------------------------------------------
   * Test 1: Check that the timer functionally works.
   * ----------------------------------------------------------------------
   */

  class Test1 : public OCPI::Util::Test::Test {
  public:
    Test1 ()
      : OCPI::Util::Test::Test ("Timer functional test")
    {
    }

    void run ()
    {
      OCPI::OS::Timer t;
      OCPI::OS::Timer::ElapsedTime e;
      t.start ();
      t.stop();
      t.getValue (e);
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Sleep for a while and see that the timer isn't too far off.
   * ----------------------------------------------------------------------
   */

  class Test2 : public OCPI::Util::Test::Test {
  public:
    Test2 ()
      : OCPI::Util::Test::Test ("Timing 3 second sleep")
    {
    }

    void run ()
    {
      OCPI::OS::Timer::ElapsedTime e;
      OCPI::OS::Timer t (true);
      OCPI::OS::sleep (3000);
      t.stop ();
      t.getValue (e);
      unsigned int msecs = e.seconds * 1000 + e.nanoseconds / 1000000;
      // Allow for a 1% skew
      test (msecs >= 2970 && msecs <= 3030);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Test that the timer can be re-started.
   * ----------------------------------------------------------------------
   */

  class Test3 : public OCPI::Util::Test::Test {
  public:
    Test3 ()
      : OCPI::Util::Test::Test ("Timer stop and restart")
    {
    }

    void run ()
    {
      OCPI::OS::Timer::ElapsedTime e;
      OCPI::OS::Timer t;

      for (unsigned int i=0; i<10; i++) {
        t.start ();
        OCPI::OS::sleep (300);
        t.stop ();
        OCPI::OS::sleep (300);
      }

      t.getValue (e);
      unsigned int msecs = e.seconds * 1000 + e.nanoseconds / 1000000;
      // Allow for a 1% skew
      test (msecs >= 2970 && msecs <= 3030);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 4: Timer reset
   * ----------------------------------------------------------------------
   */

  class Test4 : public OCPI::Util::Test::Test {
  public:
    Test4 ()
      : OCPI::Util::Test::Test ("Timer reset")
    {
    }

    void run ()
    {
      OCPI::OS::Timer::ElapsedTime e;
      OCPI::OS::Timer t;

      for (unsigned int i=0; i<10; i++) {
        t.start ();
        OCPI::OS::sleep (300);
        t.stop ();
      }

      t.reset ();

      for (unsigned int i=0; i<10; i++) {
        t.start ();
        OCPI::OS::sleep (300);
        t.stop ();
      }

      t.getValue (e);
      unsigned int msecs = e.seconds * 1000 + e.nanoseconds / 1000000;
      // Allow for a 1% skew
      test (msecs >= 2970 && msecs <= 3030);
    }
  };
   
  /*
   * ----------------------------------------------------------------------
   * Test 5: Timer precision: Expect better than 100ms
   * ----------------------------------------------------------------------
   */

  class Test5 : public OCPI::Util::Test::Test {
  public:
    Test5 ()
      : OCPI::Util::Test::Test ("Timer precision")
    {
    }

    void run ()
    {
      OCPI::OS::Timer::ElapsedTime e;
      OCPI::OS::Timer::getPrecision (e);
      test (e.seconds == 0);
      test (e.nanoseconds != 0);
      test (e.nanoseconds <= 100000000);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 6: Elapsed time comparators
   * ----------------------------------------------------------------------
   */

  class Test6 : public OCPI::Util::Test::Test {
  public:
    Test6 ()
      : OCPI::Util::Test::Test ("Elapsed time comparators")
    {
    }

    void run ()
    {
      OCPI::OS::Timer::ElapsedTime e1, e2;

      e1.seconds = 100; e1.nanoseconds = 100;
      e2.seconds = 100; e2.nanoseconds = 100;

      test (e1 == e2);
      test (!(e1 != e2));
      test (!(e1 > e2));
      test (!(e1 < e2));

      e1.seconds = 101; e1.nanoseconds = 100;
      e2.seconds = 100; e2.nanoseconds = 100;

      test (!(e1 == e2));
      test (e1 != e2);
      test (!(e1 < e2));
      test (e1 > e2);

      e1.seconds = 100; e1.nanoseconds = 101;
      e2.seconds = 100; e2.nanoseconds = 100;

      test (!(e1 == e2));
      test (e1 != e2);
      test (!(e1 < e2));
      test (e1 > e2);

      e1.seconds = 100; e1.nanoseconds = 99;
      e2.seconds = 100; e2.nanoseconds = 100;

      test (!(e1 == e2));
      test (e1 != e2);
      test (e1 < e2);
      test (!(e1 > e2));

      e1.seconds = 99;  e1.nanoseconds = 100;
      e2.seconds = 100; e2.nanoseconds = 100;

      test (!(e1 == e2));
      test (e1 != e2);
      test (e1 < e2);
      test (!(e1 > e2));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 7: Elapsed time arithmetic
   * ----------------------------------------------------------------------
   */

  class Test7 : public OCPI::Util::Test::Test {
  public:
    Test7 ()
      : OCPI::Util::Test::Test ("Elapsed time arithmetic")
    {
    }

    void run ()
    {
      OCPI::OS::Timer::ElapsedTime e1, e2, e3;

      e1.seconds = 100; e1.nanoseconds = 300;
      e2.seconds = 200; e2.nanoseconds = 400;
      e3 = e1 + e2;

      test (e3.seconds == 300);
      test (e3.nanoseconds == 700);

      e1.seconds = 100; e1.nanoseconds = 600000000;
      e2.seconds = 100; e2.nanoseconds = 600000000;
      e3 = e1 + e2;

      test (e3.seconds == 201);
      test (e3.nanoseconds == 200000000);

      e1.seconds = 300; e1.nanoseconds = 400;
      e2.seconds = 100; e2.nanoseconds = 100;
      e3 = e1 - e2;

      test (e3.seconds == 200);
      test (e3.nanoseconds == 300);

      e1.seconds = 300; e1.nanoseconds = 400;
      e2.seconds = 100; e2.nanoseconds = 500;
      e3 = e1 - e2;

      test (e3.seconds == 199);
      test (e3.nanoseconds == 999999900);
    }
  };

}

static
int
testTimerInt (int, char *[])
{
  OCPI::Util::Test::Suite tests ("Timer tests");
  int n_failed;
  tests.add_test (new TimerTests::Test1);
  tests.add_test (new TimerTests::Test2);
  tests.add_test (new TimerTests::Test3);
  tests.add_test (new TimerTests::Test4);
  tests.add_test (new TimerTests::Test5);
  tests.add_test (new TimerTests::Test6);
  tests.add_test (new TimerTests::Test7);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testTimer (int argc, char * argv[])
  {
    return testTimerInt (argc, argv);
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

  return testTimerInt (argc, argv);
}
