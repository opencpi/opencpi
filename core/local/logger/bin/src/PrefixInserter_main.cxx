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
#include <CpiLoggerPrefixInserter.h>
#include "MessageKeeper.h"
#include "CpiUtilTest.h"

namespace PrefixInserterTests {
  /*
   * ----------------------------------------------------------------------
   * Test 1: See if Message Keeper is working properly
   * ----------------------------------------------------------------------
   */

  class Test1 : public CPI::Util::Test::Test {
  public:
    Test1 ()
      : CPI::Util::Test::Test ("See if Message Keeper is working properly")
    {
    }

    void run ()
    {
      MessageKeeperOutput logger;
      logger.setProducerId ("03-PrefixInserter");
      logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
             << CPI::Logger::ProducerName ("testMessageKeeper")
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

  class Test2 : public CPI::Util::Test::Test {
  public:
    Test2 ()
      : CPI::Util::Test::Test ("Use PrefixInserter to add a prefix to a message")
    {
    }

    void run ()
    {
      MessageKeeperOutput logger;
      logger.setProducerId ("03-PrefixInserter");

      CPI::Logger::PrefixInserter pi (logger, "MyPrefix: ");
      pi << CPI::Logger::Level::ADMINISTRATIVE_EVENT
         << CPI::Logger::ProducerName ("testPrefixInserter")
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

  class Test3 : public CPI::Util::Test::Test {
  public:
    Test3 ()
      : CPI::Util::Test::Test ("Chain two PrefixInserters")
    {
    }

    void run ()
    {
      MessageKeeperOutput logger;
      logger.setProducerId ("03-PrefixInserter");

      CPI::Logger::PrefixInserter p1 (logger, "First Prefix: ");
      CPI::Logger::PrefixInserter p2 (p1, "Second Prefix: ");

      p2 << CPI::Logger::Level::ADMINISTRATIVE_EVENT
         << CPI::Logger::ProducerName ("testChainingPrefixInserters")
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
  CPI::Util::Test::Suite tests ("PrefixInserter tests");
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
        CPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testPrefixInserterInt (argc, argv);
}
