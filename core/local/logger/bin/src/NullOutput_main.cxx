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
#include <CpiLoggerNullOutput.h>
#include "CpiUtilTest.h"

namespace NullOutputTests {

  /*
   * ----------------------------------------------------------------------
   * Test 1
   * ----------------------------------------------------------------------
   */

  class Test1 : public CPI::Util::Test::Test {
  public:
    Test1 ()
      : CPI::Util::Test::Test ("Testing the NullOutput logger")
    {
    }

    void run ()
    {
      CPI::Logger::NullOutput null;
      null.setProducerId ("01-NullOutput");
      null << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	   << CPI::Logger::ProducerName ("runNullTest")
	   << "Hello World"
	   << std::flush;
      test (null.good());
    }
  };

}

static
int
testNullOutputInt (int, char *[])
{
  CPI::Util::Test::Suite tests ("NullOutput tests");
  int n_failed;
  tests.add_test (new NullOutputTests::Test1);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testNullOutput (int argc, char * argv[])
  {
    return testNullOutputInt (argc, argv);
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

  return testNullOutputInt (argc, argv);
}
