
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
#include <OcpiLoggerPrefixInserter.h>
#include "MessageKeeper.h"
#include "OcpiUtilTest.h"

namespace PrefixInserterTests {
  /*
   * ----------------------------------------------------------------------
   * Test 1: See if Message Keeper is working properly
   * ----------------------------------------------------------------------
   */

  class Test1 : public OCPI::Util::Test::Test {
  public:
    Test1 ()
      : OCPI::Util::Test::Test ("See if Message Keeper is working properly")
    {
    }

    void run ()
    {
      MessageKeeperOutput logger;
      logger.setProducerId ("03-PrefixInserter");
      logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
             << OCPI::Logger::ProducerName ("testMessageKeeper")
             << "Hello World"
             << std::flush;

      test (logger.getLogLevel() == 8);
      test (logger.getProducerId() == "03-PrefixInserter");
      test (logger.getProducerName() == "testMessageKeeper");
      test (logger.getMessage() == "Hello World");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Use PrefixInserter to add a prefix to a message
   * ----------------------------------------------------------------------
   */

  class Test2 : public OCPI::Util::Test::Test {
  public:
    Test2 ()
      : OCPI::Util::Test::Test ("Use PrefixInserter to add a prefix to a message")
    {
    }

    void run ()
    {
      MessageKeeperOutput logger;
      logger.setProducerId ("03-PrefixInserter");

      OCPI::Logger::PrefixInserter pi (logger, "MyPrefix: ");
      pi << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
         << OCPI::Logger::ProducerName ("testPrefixInserter")
         << "Hello World"
         << std::flush;

      test (logger.getLogLevel() == 8);
      test (logger.getProducerId() == "03-PrefixInserter");
      test (logger.getProducerName() == "testPrefixInserter");
      test (logger.getMessage() == "MyPrefix: Hello World");
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Chain two PrefixInserters
   * ----------------------------------------------------------------------
   */

  class Test3 : public OCPI::Util::Test::Test {
  public:
    Test3 ()
      : OCPI::Util::Test::Test ("Chain two PrefixInserters")
    {
    }

    void run ()
    {
      MessageKeeperOutput logger;
      logger.setProducerId ("03-PrefixInserter");

      OCPI::Logger::PrefixInserter p1 (logger, "First Prefix: ");
      OCPI::Logger::PrefixInserter p2 (p1, "Second Prefix: ");

      p2 << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
         << OCPI::Logger::ProducerName ("testChainingPrefixInserters")
         << "Hello World"
         << std::flush;

      test (logger.getLogLevel() == 8);
      test (logger.getProducerId() == "03-PrefixInserter");
      test (logger.getProducerName() == "testChainingPrefixInserters");
      test (logger.getMessage() == "First Prefix: Second Prefix: Hello World");
    }
  };

}

static
int
testPrefixInserterInt (int, char *[])
{
  OCPI::Util::Test::Suite tests ("PrefixInserter tests");
  int n_failed;
  tests.add_test (new PrefixInserterTests::Test1);
  tests.add_test (new PrefixInserterTests::Test2);
  tests.add_test (new PrefixInserterTests::Test3);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testPrefixInserter (int argc, char * argv[])
  {
    return testPrefixInserterInt (argc, argv);
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

  return testPrefixInserterInt (argc, argv);
}
