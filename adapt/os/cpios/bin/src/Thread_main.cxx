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

#include <CpiOsDebug.h>
#include <CpiOsThreadManager.h>
#include "CpiUtilTest.h"

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

  class Test1 : public CPI::Util::Test::Test {
  public:
    Test1 ()
      : CPI::Util::Test::Test ("Running and joining a simple thread")
    {
    }

    void run ()
    {
      simpleThreadFlag = false;
      CPI::OS::ThreadManager tm (simpleThread, 0);
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

  class Test2 : public CPI::Util::Test::Test {
  public:
    Test2 ()
      : CPI::Util::Test::Test ("Passing a parameter to a thread")
    {
    }

    void run ()
    {
      simpleThread2Parameter = 0;
      CPI::OS::ThreadManager tm (simpleThread2, (void *) (simpleThread2));
      tm.join ();
      test (simpleThread2Parameter == (void *) simpleThread2);
    }
  };

}

static
int
testThreadInt (int, char *[])
{
  CPI::Util::Test::Suite tests ("Thread tests");
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
        CPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testThreadInt (argc, argv);
}
