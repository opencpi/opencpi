
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
#include "OcpiLoggerFallback.h"
#include "MessageKeeper.h"

#include "gtest/gtest.h"

namespace
{
  class TestOcpiLoggerFallback : public ::testing::Test
  {
    // Empty
  };

  // Test 1: An empty Fallback
  TEST( TestOcpiLoggerFallback, test_1 )
  {
    OCPI::Logger::Fallback logger;

    logger.setProducerId ("05-Fallback");

    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testEmptyFallback")
           << "Hello World"
           << std::flush;

    EXPECT_NE( logger.good(), true );
  }

  // Test 2:  Fallback with one delegate
  TEST( TestOcpiLoggerFallback, test_2 )
  {
    MessageKeeperOutput keeper;
    keeper.setProducerId ("05-Fallback");

    OCPI::Logger::Fallback logger;
    logger.addOutput (keeper);

    logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
           << OCPI::Logger::ProducerName ("testOneDelegatee")
           << "Hello World"
           << std::flush;

    EXPECT_EQ( keeper.getLogLevel(), 8 );
    EXPECT_EQ( keeper.getProducerId(), "05-Fallback" );
    EXPECT_EQ( keeper.getProducerName(), "testOneDelegatee" );
    EXPECT_EQ( keeper.getMessage(), "Hello World" );
  }

  // Test 3: Fallback with two delegates
  TEST( TestOcpiLoggerFallback, test_3 )
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

    EXPECT_EQ( keeper1.getLogLevel(), 8 );
    EXPECT_EQ( keeper1.getProducerId(), "05-Fallback" );
    EXPECT_EQ( keeper1.getProducerName(), "testTwoDelegatees" );
    EXPECT_EQ( keeper1.getMessage(), "An Important Message" );
    EXPECT_EQ( keeper2.getMessage(), "" );
  }

  // Test 4: First delegatee fails
  TEST( TestOcpiLoggerFallback, test_4 )
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

    EXPECT_EQ( logger.good(), true );

    EXPECT_EQ( keeper1.getLogLevel(), 8 );
    EXPECT_EQ( keeper1.getProducerId(), "05-Fallback" );
    EXPECT_EQ( keeper1.getProducerName(), "testFirstDelegateeFails" );
    EXPECT_EQ( keeper1.getMessage(), "An Important Message For Delegatee 1" );

    EXPECT_EQ( keeper2.getLogLevel(), 8 );
    EXPECT_EQ( keeper2.getProducerId(), "05-Fallback" );
    EXPECT_EQ( keeper2.getProducerName(),  "testFirstDelegateeFails" );
    EXPECT_EQ( keeper2.getMessage(), "An Important Message For Delegatee 2" );
  }

  // Test 5: First delegatee fails, then recovers
  TEST( TestOcpiLoggerFallback, test_5 )
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

    EXPECT_EQ( logger.good(), true );

    EXPECT_EQ( keeper1.getLogLevel(), 8 );
    EXPECT_EQ( keeper1.getProducerId(), "05-Fallback" );
    EXPECT_EQ( keeper1.getProducerName(), "testFirstDelegateeRecovers" );
    EXPECT_EQ( keeper1.getMessage(), "An Important Message For Delegatee 1" );

    EXPECT_EQ( keeper2.getLogLevel(), 8 );
    EXPECT_EQ( keeper2.getProducerId(), "05-Fallback" );
    EXPECT_EQ( keeper2.getProducerName(), "testFirstDelegateeRecovers" );
    EXPECT_EQ( keeper2.getMessage(), "An Important Message For Delegatee 2" );
  }

  // Test 6: Auto-recovering first delegatee
  TEST( TestOcpiLoggerFallback, test_6 )
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

    EXPECT_EQ( logger.good(), true );
    EXPECT_EQ( keeper1.getLogLevel(), 8 );
    EXPECT_EQ( keeper1.getProducerId(), "05-Fallback" );
    EXPECT_EQ( keeper1.getProducerName(), "testAutoRecoveringFirstDelegatee" );
    EXPECT_EQ( keeper1.getMessage(), "An Important Message" );
    EXPECT_EQ( keeper2.getMessage(), "" );
  }

  // Test 7:  Both delegatees fail
  TEST( TestOcpiLoggerFallback, test_7 )
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

    EXPECT_NE( logger.good(), true );
  }

} // End: namespace<unnamed>
