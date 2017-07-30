/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtest/gtest.h"

#include "OcpiOsDebug.h"
#include "OcpiLoggerTee.h"

#include "MessageKeeper.h"

namespace
{
  class TestOcpiLoggerTee : public ::testing::Test
  {
    // Empty
  };

  // Test 1: An empty Tee
  TEST( TestOcpiLoggerTee, test_1 )
  {
    OCPI::Logger::Tee logger;

    logger.setProducerId ("04-Tee");
    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testEmptyTee")
           << "Hello World"
           << std::flush;

    EXPECT_EQ( logger.good(), true);
  }

  // Test 2:  Tee with one delegatee
  TEST( TestOcpiLoggerTee, test_2 )
  {
    MessageKeeperOutput keeper;
    keeper.setProducerId ("04-Tee");

    OCPI::Logger::Tee logger;
    logger.addOutput (keeper);

    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testOneDelegatee")
           << "Hello World"
           << std::flush;

    EXPECT_EQ( keeper.getLogLevel(), 8);
    EXPECT_EQ( keeper.getProducerId(), "04-Tee");
    EXPECT_EQ( keeper.getProducerName(), "testOneDelegatee");
    EXPECT_EQ( keeper.getMessage(), "Hello World");
  }

  // Test 3:  Tee with two delegatees
  TEST( TestOcpiLoggerTee, test_3 )
  {
    MessageKeeperOutput keeper1, keeper2;
    keeper1.setProducerId ("04-Tee");
    keeper2.setProducerId ("04-Tee");

    OCPI::Logger::Tee logger;
    logger.addOutput (keeper1);
    logger.addOutput (keeper2);

    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testTwoDelegatees")
           << "An Important Message"
           << std::flush;

    EXPECT_EQ( keeper1.getLogLevel(), 8);
    EXPECT_EQ( keeper1.getProducerId(), "04-Tee");
    EXPECT_EQ( keeper1.getProducerName(), "testTwoDelegatees");
    EXPECT_EQ( keeper1.getMessage(), "An Important Message");

    EXPECT_EQ( keeper2.getLogLevel(), 8);
    EXPECT_EQ( keeper2.getProducerId(), "04-Tee");
    EXPECT_EQ( keeper2.getProducerName(), "testTwoDelegatees");
    EXPECT_EQ( keeper2.getMessage(), "An Important Message");
  }

  // Test 4: Tee with two delegatees, when the first one fails
  TEST( TestOcpiLoggerTee, test_4 )
  {
    MessageKeeperOutput keeper1, keeper2;
    keeper1.setProducerId ("04-Tee");
    keeper2.setProducerId ("04-Tee");
    keeper1.setstate (std::ios_base::badbit);

    OCPI::Logger::Tee logger;
    logger.addOutput (keeper1);
    logger.addOutput (keeper2);

    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testFirstDelegateeFailed")
           << "An Important Message"
           << std::flush;

    EXPECT_NE( logger.good(), true);
    EXPECT_EQ( keeper2.getLogLevel(), 8);
    EXPECT_EQ( keeper2.getProducerId(), "04-Tee");
    EXPECT_EQ( keeper2.getProducerName(), "testFirstDelegateeFailed");
    EXPECT_EQ( keeper2.getMessage(), "An Important Message");
  }

  // Test 5: Tee with two delegatees, ignoring the first one's failure
  TEST( TestOcpiLoggerTee, test_5 )
  {
    MessageKeeperOutput keeper1, keeper2;
    keeper1.setProducerId ("04-Tee");
    keeper2.setProducerId ("04-Tee");
    keeper1.setstate (std::ios_base::badbit);

    OCPI::Logger::Tee logger;
    logger.addOutput (keeper1, false, true);
    logger.addOutput (keeper2);

    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testIgnoringFirstDelegatee")
           << "An Important Message"
           << std::flush;

    EXPECT_EQ( logger.good(), true);
    EXPECT_EQ( keeper2.getLogLevel(), 8);
    EXPECT_EQ( keeper2.getProducerId(), "04-Tee");
    EXPECT_EQ( keeper2.getProducerName(), "testIgnoringFirstDelegatee");
    EXPECT_EQ( keeper2.getMessage(), "An Important Message");
  }

  // Test 6: Tee with two delegatees, recovering the first one's failure
  TEST( TestOcpiLoggerTee, test_6 )
  {
    MessageKeeperOutput keeper1, keeper2;
    keeper1.setProducerId ("04-Tee");
    keeper2.setProducerId ("04-Tee");
    keeper1.setstate (std::ios_base::badbit);

    OCPI::Logger::Tee logger;
    logger.addOutput (keeper1, true);
    logger.addOutput (keeper2);

    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testRecoveringFirstDelegatee")
           << "An Important Message"
           << std::flush;

    EXPECT_EQ( logger.good(), true );

    EXPECT_EQ( keeper1.getLogLevel(), 8);
    EXPECT_EQ( keeper1.getProducerId(), "04-Tee");
    EXPECT_EQ( keeper1.getProducerName(), "testRecoveringFirstDelegatee");
    EXPECT_EQ( keeper1.getMessage(), "An Important Message");

    EXPECT_EQ( keeper2.getLogLevel(), 8);
    EXPECT_EQ( keeper2.getProducerId(), "04-Tee");
    EXPECT_EQ( keeper2.getProducerName(), "testRecoveringFirstDelegatee");
    EXPECT_EQ( keeper2.getMessage(), "An Important Message");
  }

} // End: namespace<unnamed>
