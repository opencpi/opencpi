
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
#include <OcpiLoggerFallback.h>
#include "MessageKeeper.h"
#include "OcpiUtilTest.h"

namespace FallbackTests {
  /*
   * ----------------------------------------------------------------------
   * Test 1: An empty Fallback
   * ----------------------------------------------------------------------
   */

  class Test1 : public OCPI::Util::Test::Test {
  public:
    Test1 ()
      : OCPI::Util::Test::Test ("An empty Fallback should fail")
    {
    }

    void run ()
    {
      OCPI::Logger::Fallback logger;

      logger.setProducerId ("05-Fallback");
      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testEmptyFallback")
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

  class Test2 : public OCPI::Util::Test::Test {
  public:
    Test2 ()
      : OCPI::Util::Test::Test ("Fallback with one delegatee")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper;
      keeper.setProducerId ("05-Fallback");

      OCPI::Logger::Fallback logger;
      logger.addOutput (keeper);

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testOneDelegatee")
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

  class Test3 : public OCPI::Util::Test::Test {
  public:
    Test3 ()
      : OCPI::Util::Test::Test ("Fallback with two delegatees")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");

      OCPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testTwoDelegatees")
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

  class Test4 : public OCPI::Util::Test::Test {
  public:
    Test4 ()
      : OCPI::Util::Test::Test ("First delegatee fails")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");

      OCPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testFirstDelegateeFails")
             << "An Important Message For Delegatee 1"
             << std::flush;

      keeper1.setstate (std::ios_base::badbit);

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testFirstDelegateeFails")
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

  class Test5 : public OCPI::Util::Test::Test {
  public:
    Test5 ()
      : OCPI::Util::Test::Test ("First delegatee fails, then recovers")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");

      OCPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      keeper1.setstate (std::ios_base::badbit);

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testFirstDelegateeRecovers")
             << "An Important Message For Delegatee 2"
             << std::flush;

      keeper1.clear ();

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testFirstDelegateeRecovers")
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

  class Test6 : public OCPI::Util::Test::Test {
  public:
    Test6 ()
      : OCPI::Util::Test::Test ("Auto-recovering first delegatee")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");
      keeper1.setstate (std::ios_base::badbit);

      OCPI::Logger::Fallback logger;
      logger.addOutput (keeper1, true);
      logger.addOutput (keeper2);

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testAutoRecoveringFirstDelegatee")
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

  class Test7 : public OCPI::Util::Test::Test {
  public:
    Test7 ()
      : OCPI::Util::Test::Test ("Both delegatees fail")
    {
    }

    void run ()
    {
      MessageKeeperOutput keeper1, keeper2;
      keeper1.setProducerId ("05-Fallback");
      keeper2.setProducerId ("05-Fallback");
      keeper1.setstate (std::ios_base::badbit);
      keeper2.setstate (std::ios_base::badbit);

      OCPI::Logger::Fallback logger;
      logger.addOutput (keeper1);
      logger.addOutput (keeper2);

      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testBothDelegateesFail")
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
  OCPI::Util::Test::Suite tests ("Fallback tests");
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
        OCPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testFallbackInt (argc, argv);
}
