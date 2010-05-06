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
#include <CpiLoggerFallback.h>
#include "MessageKeeper.h"
#include "CpiUtilTest.h"

namespace FallbackTests {
  /*
   * ----------------------------------------------------------------------
   * Test 1: An empty Fallback
   * ----------------------------------------------------------------------
   */

  class Test1 : public CPI::Util::Test::Test {
  public:
    Test1 ()
      : CPI::Util::Test::Test ("An empty Fallback should fail")
    {
    }

    void run ()
    {
      CPI::Logger::Fallback logger;

      logger.setProducerId ("05-Fallback");
      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testEmptyFallback")
             << "Hello World"
             << std::flush;

      test (!logger.good());
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Fallback with one delegatee
   * ----------------------------------------------------------------------
   */

  class Test2 : public CPI::Util::Test::Test {
  public:
    Test2 ()
      : CPI::Util::Test::Test ("Fallback with one delegatee")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper;
      keeper.setProducerId ("05-Fallback");

      CPI::Logger::Fallback logger;
      logger.addOutput (keeper);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testOneDelegatee")
             << "Hello World"
             << std::flush;

      test (keeper.getLogLevel() == 8);
      test (keeper.getProducerId() == "05-Fallback");
      test (keeper.getProducerName() == "testOneDelegatee");
      test (keeper.getMessage() == "Hello World");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Fallback with two delegatees
   * ----------------------------------------------------------------------
   */

  class Test3 : public CPI::Util::Test::Test {
  public:
    Test3 ()
      : CPI::Util::Test::Test ("Fallback with two delegatees")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");

      CPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testTwoDelegatees")
             << "An Important Message"
             << std::flush;

      test (keeper1.getLogLevel() == 8);
      test (keeper1.getProducerId() == "05-Fallback");
      test (keeper1.getProducerName() == "testTwoDelegatees");
      test (keeper1.getMessage() == "An Important Message");
      test (keeper2.getMessage() == "");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 4: First delegatee fails
   * ----------------------------------------------------------------------
   */

  class Test4 : public CPI::Util::Test::Test {
  public:
    Test4 ()
      : CPI::Util::Test::Test ("First delegatee fails")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");

      CPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testFirstDelegateeFails")
             << "An Important Message For Delegatee 1"
             << std::flush;

      keeper1.setstate (std::ios_base::badbit);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testFirstDelegateeFails")
             << "An Important Message For Delegatee 2"
             << std::flush;

      test (logger.good());

      test (keeper1.getLogLevel() == 8);
      test (keeper1.getProducerId() == "05-Fallback");
      test (keeper1.getProducerName() == "testFirstDelegateeFails");
      test (keeper1.getMessage() == "An Important Message For Delegatee 1");

      test (keeper2.getLogLevel() == 8);
      test (keeper2.getProducerId() == "05-Fallback");
      test (keeper2.getProducerName() == "testFirstDelegateeFails");
      test (keeper2.getMessage() == "An Important Message For Delegatee 2");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 5: First delegatee fails, then recovers
   * ----------------------------------------------------------------------
   */

  class Test5 : public CPI::Util::Test::Test {
  public:
    Test5 ()
      : CPI::Util::Test::Test ("First delegatee fails, then recovers")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");

      CPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      keeper1.setstate (std::ios_base::badbit);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testFirstDelegateeRecovers")
             << "An Important Message For Delegatee 2"
             << std::flush;

      keeper1.clear ();

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testFirstDelegateeRecovers")
             << "An Important Message For Delegatee 1"
             << std::flush;

      test (logger.good());

      test (keeper1.getLogLevel() == 8);
      test (keeper1.getProducerId() == "05-Fallback");
      test (keeper1.getProducerName() == "testFirstDelegateeRecovers");
      test (keeper1.getMessage() == "An Important Message For Delegatee 1");

      test (keeper2.getLogLevel() == 8);
      test (keeper2.getProducerId() == "05-Fallback");
      test (keeper2.getProducerName() == "testFirstDelegateeRecovers");
      test (keeper2.getMessage() == "An Important Message For Delegatee 2");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 6: Auto-recovering first delegatee
   * ----------------------------------------------------------------------
   */

  class Test6 : public CPI::Util::Test::Test {
  public:
    Test6 ()
      : CPI::Util::Test::Test ("Auto-recovering first delegatee")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");
      keeper1.setstate (std::ios_base::badbit);

      CPI::Logger::Fallback logger;
      logger.addOutput (keeper1, true);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testAutoRecoveringFirstDelegatee")
             << "An Important Message"
             << std::flush;

      test (logger.good());
      test (keeper1.getLogLevel() == 8);
      test (keeper1.getProducerId() == "05-Fallback");
      test (keeper1.getProducerName() == "testAutoRecoveringFirstDelegatee");
      test (keeper1.getMessage() == "An Important Message");
      test (keeper2.getMessage() == "");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 7: Both delegatees fail
   * ----------------------------------------------------------------------
   */

  class Test7 : public CPI::Util::Test::Test {
  public:
    Test7 ()
      : CPI::Util::Test::Test ("Both delegatees fail")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");
      keeper1.setstate (std::ios_base::badbit);
      keeper2.setstate (std::ios_base::badbit);

      CPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testBothDelegateesFail")
             << "An Important Message"
             << std::flush;

      test (!logger.good());
    }
  };

}

static
int
testFallbackInt (int, char *[])
{
  CPI::Util::Test::Suite tests ("Fallback tests");
  int n_failed;
  tests.add_test (new FallbackTests::Test1);
  tests.add_test (new FallbackTests::Test2);
  tests.add_test (new FallbackTests::Test3);
  tests.add_test (new FallbackTests::Test4);
  tests.add_test (new FallbackTests::Test5);
  tests.add_test (new FallbackTests::Test6);
  tests.add_test (new FallbackTests::Test7);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testFallback (int argc, char * argv[])
  {
    return testFallbackInt (argc, argv);
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

  return testFallbackInt (argc, argv);
}
