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
#include <CpiLoggerTee.h>
#include "MessageKeeper.h"
#include "CpiUtilTest.h"

namespace TeeTests {
  /*
   * ----------------------------------------------------------------------
   * Test 1: An empty Tee
   * ----------------------------------------------------------------------
   */

  class Test1 : public CPI::Util::Test::Test {
  public:
    Test1 ()
      : CPI::Util::Test::Test ("An empty Tee")
    {
    }

    void run ()
    {
      CPI::Logger::Tee logger;

      logger.setProducerId ("04-Tee");
      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testEmptyTee")
             << "Hello World"
             << std::flush;

      test (logger.good());
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Tee with one delegatee
   * ----------------------------------------------------------------------
   */

  class Test2 : public CPI::Util::Test::Test {
  public:
    Test2 ()
      : CPI::Util::Test::Test ("Tee with one delegatee")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper;
      keeper.setProducerId ("04-Tee");

      CPI::Logger::Tee logger;
      logger.addOutput (keeper);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testOneDelegatee")
             << "Hello World"
             << std::flush;

      test (keeper.getLogLevel() == 8);
      test (keeper.getProducerId() == "04-Tee");
      test (keeper.getProducerName() == "testOneDelegatee");
      test (keeper.getMessage() == "Hello World");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Tee with two delegatees
   * ----------------------------------------------------------------------
   */

  class Test3 : public CPI::Util::Test::Test {
  public:
    Test3 ()
      : CPI::Util::Test::Test ("Tee with two delegatees")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("04-Tee");
      keeper2.setProducerId ("04-Tee");

      CPI::Logger::Tee logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testTwoDelegatees")
             << "An Important Message"
             << std::flush;

      test (keeper1.getLogLevel() == 8);
      test (keeper1.getProducerId() == "04-Tee");
      test (keeper1.getProducerName() == "testTwoDelegatees");
      test (keeper1.getMessage() == "An Important Message");

      test (keeper2.getLogLevel() == 8);
      test (keeper2.getProducerId() == "04-Tee");
      test (keeper2.getProducerName() == "testTwoDelegatees");
      test (keeper2.getMessage() == "An Important Message");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 4: Tee with two delegatees, when the first one fails
   * ----------------------------------------------------------------------
   */

  class Test4 : public CPI::Util::Test::Test {
  public:
    Test4 ()
      : CPI::Util::Test::Test ("Tee with two delegatees, when the first one fails")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("04-Tee");
      keeper2.setProducerId ("04-Tee");
      keeper1.setstate (std::ios_base::badbit);

      CPI::Logger::Tee logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testFirstDelegateeFailed")
             << "An Important Message"
             << std::flush;

      test (!logger.good());
      test (keeper2.getLogLevel() == 8);
      test (keeper2.getProducerId() == "04-Tee");
      test (keeper2.getProducerName() == "testFirstDelegateeFailed");
      test (keeper2.getMessage() == "An Important Message");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 5: Tee with two delegatees, ignoring the first one's failure
   * ----------------------------------------------------------------------
   */

  class Test5 : public CPI::Util::Test::Test {
  public:
    Test5 ()
      : CPI::Util::Test::Test ("Tee with two delegatees, ignoring the first one's failure")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("04-Tee");
      keeper2.setProducerId ("04-Tee");
      keeper1.setstate (std::ios_base::badbit);

      CPI::Logger::Tee logger;
      logger.addOutput (keeper1, false, true);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testIgnoringFirstDelegatee")
             << "An Important Message"
             << std::flush;

      test (logger.good());
      test (keeper2.getLogLevel() == 8);
      test (keeper2.getProducerId() == "04-Tee");
      test (keeper2.getProducerName() == "testIgnoringFirstDelegatee");
      test (keeper2.getMessage() == "An Important Message");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 6: Tee with two delegatees, recovering the first one's failure
   * ----------------------------------------------------------------------
   */

  class Test6 : public CPI::Util::Test::Test {
  public:
    Test6 ()
      : CPI::Util::Test::Test ("Tee with two delegatees, recovering the first one's failure")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("04-Tee");
      keeper2.setProducerId ("04-Tee");
      keeper1.setstate (std::ios_base::badbit);

      CPI::Logger::Tee logger;
      logger.addOutput (keeper1, true);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testRecoveringFirstDelegatee")
             << "An Important Message"
             << std::flush;

      test (logger.good());

      test (keeper1.getLogLevel() == 8);
      test (keeper1.getProducerId() == "04-Tee");
      test (keeper1.getProducerName() == "testRecoveringFirstDelegatee");
      test (keeper1.getMessage() == "An Important Message");

      test (keeper2.getLogLevel() == 8);
      test (keeper2.getProducerId() == "04-Tee");
      test (keeper2.getProducerName() == "testRecoveringFirstDelegatee");
      test (keeper2.getMessage() == "An Important Message");
    }
  };

}

static
int
testTeeInt (int, char *[])
{
  CPI::Util::Test::Suite tests ("Tee tests");
  int n_failed;
  tests.add_test (new TeeTests::Test1);
  tests.add_test (new TeeTests::Test2);
  tests.add_test (new TeeTests::Test3);
  tests.add_test (new TeeTests::Test4);
  tests.add_test (new TeeTests::Test5);
  tests.add_test (new TeeTests::Test6);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testTee (int argc, char * argv[])
  {
    return testTeeInt (argc, argv);
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

  return testTeeInt (argc, argv);
}
