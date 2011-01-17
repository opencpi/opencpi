
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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

#include "OcpiOsDebug.h"
#include "OcpiLoggerPrefixInserter.h"
#include "MessageKeeper.h"

#include "gtest/gtest.h"

namespace
{
  class TestOcpiLoggerPrefixInserter : public ::testing::Test
  {
    // Empty
  };

  // Test 1: See if Message Keeper is working properly
  TEST( TestOcpiLoggerPrefixInserter, test_1 )
  {
    MessageKeeperOutput logger;
    logger.setProducerId ("03-PrefixInserter");
    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testMessageKeeper")
           << "Hello World"
           << std::flush;

    EXPECT_EQ( logger.getLogLevel(), 8);
    EXPECT_EQ( logger.getProducerId(), "03-PrefixInserter");
    EXPECT_EQ( logger.getProducerName(), "testMessageKeeper");
    EXPECT_EQ( logger.getMessage(), "Hello World");
  }

  // Test 2: Use PrefixInserter to add a prefix to a message
  TEST( TestOcpiLoggerPrefixInserter, test_2 )
  {
    MessageKeeperOutput logger;
    logger.setProducerId ("03-PrefixInserter");

    OCPI::Logger::PrefixInserter pi (logger, "MyPrefix: ");
    pi << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
       << OCPI::Logger::ProducerName ("testPrefixInserter")
       << "Hello World"
       << std::flush;

    EXPECT_EQ( logger.getLogLevel(), 8);
    EXPECT_EQ( logger.getProducerId(), "03-PrefixInserter");
    EXPECT_EQ( logger.getProducerName(), "testPrefixInserter");
    EXPECT_EQ( logger.getMessage(), "MyPrefix: Hello World");
  }

  // Test 3: Chain two PrefixInserters
  TEST( TestOcpiLoggerPrefixInserter, test_3 )
  {
    MessageKeeperOutput logger;
    logger.setProducerId ("03-PrefixInserter");

    OCPI::Logger::PrefixInserter p1 (logger, "First Prefix: ");
    OCPI::Logger::PrefixInserter p2 (p1, "Second Prefix: ");

    p2 << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
       << OCPI::Logger::ProducerName ("testChainingPrefixInserters")
       << "Hello World"
       << std::flush;

    EXPECT_EQ( logger.getLogLevel(), 8);
    EXPECT_EQ( logger.getProducerId(), "03-PrefixInserter");
    EXPECT_EQ( logger.getProducerName(), "testChainingPrefixInserters");
    EXPECT_EQ( logger.getMessage(), "First Prefix: Second Prefix: Hello World");
  }

} // End: namespace<unnamed>
