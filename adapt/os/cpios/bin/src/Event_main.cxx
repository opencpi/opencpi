// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#include <CpiOsEvent.h>
#include <CpiOsDebug.h>
#include <CpiOsThreadManager.h>
#include <CpiOsMisc.h>
#include "CpiUtilTest.h"

namespace EventTests {

  /*
   * ----------------------------------------------------------------------
   * Test 1: Setting and waiting for the event
   * ----------------------------------------------------------------------
   */

  class Test1 : public CPI::Util::Test::Test {
  public:
    Test1 ()
      : CPI::Util::Test::Test ("Set and Wait")
    {
    }

    void run ()
    {
      CPI::OS::Event e;
      e.set ();
      e.wait ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Initializing event to "set", then waiting for the event
   * ----------------------------------------------------------------------
   */

  class Test2 : public CPI::Util::Test::Test {
  public:
    Test2 ()
      : CPI::Util::Test::Test ("Signaled and Wait")
    {
    }

    void run ()
    {
      CPI::OS::Event e (true);
      e.wait ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Setting the event in another thread
   * ----------------------------------------------------------------------
   */

  void
  test3Thread (void * opaque)
  {
    CPI::OS::Event * e = reinterpret_cast<CPI::OS::Event *> (opaque);
    e->set ();
  }

  class Test3 : public CPI::Util::Test::Test {
  public:
    Test3 ()
      : CPI::Util::Test::Test ("Setting the event in another thread")
    {
    }

    void run ()
    {
      CPI::OS::Event e;
      CPI::OS::ThreadManager tm (test3Thread, &e);
      e.wait ();
      tm.join ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 4: Leader and follower
   * ----------------------------------------------------------------------
   */

  struct Test4Data {
    CPI::OS::Event e1;
    CPI::OS::Event e2;
    int i;
  };

  void
  test4Thread (void * opaque)
  {
    Test4Data * t4d = reinterpret_cast<Test4Data *> (opaque);

    for (int i=0; i<42; i++) {
      t4d->i = i;
      t4d->e1.set ();
      t4d->e2.wait ();
    }
  }

  class Test4 : public CPI::Util::Test::Test {
  public:
    Test4 ()
      : CPI::Util::Test::Test ("Leader and follower")
    {
    }

    void run ()
    {
      Test4Data t4d;
      CPI::OS::ThreadManager tm (test4Thread, &t4d);

      for (int i=0; i<42; i++) {
        t4d.e1.wait ();
        test (t4d.i == i);
        t4d.e2.set ();
      }

      tm.join ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 5: Waiting with a timeout
   * ----------------------------------------------------------------------
   */

  class Test5 : public CPI::Util::Test::Test {
  public:
    Test5 ()
      : CPI::Util::Test::Test ("Waiting with a timeout")
    {
    }

    void run ()
    {
      CPI::OS::Event e;
      e.set ();
      bool to = e.wait (100);
      test (to);
      to = e.wait (100);
      test (!to);
    }
  };

}

static
int
testEventInt (int, char *[])
{
  CPI::Util::Test::Suite tests ("Event tests");
  int n_failed;
  tests.add_test (new EventTests::Test1);
  tests.add_test (new EventTests::Test2);
  tests.add_test (new EventTests::Test3);
  tests.add_test (new EventTests::Test4);
  tests.add_test (new EventTests::Test5);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testEvent (int argc, char * argv[])
  {
    return testEventInt (argc, argv);
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
        CPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testEventInt (argc, argv);
}
