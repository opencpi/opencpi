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

#include <iostream>

#include "gtest/gtest.h"

#include "OcpiOsTimer.h"
#include "OcpiOsMisc.h"
#include "OcpiOsDebug.h"

namespace OS = OCPI::OS;

std::ostream& operator<< ( std::ostream& s, OS::ElapsedTime e )
{
  s << e.seconds() << " seconds " << e.nanoseconds() << " nanoseconds " << '\n';

  return s;
}

namespace
{
  class TestOcpiOsTimer : public ::testing::Test
  {
    protected:
      std::string d_argv0;
  };

  // Test 1: Check that the timer functionally works.
  TEST( TestOcpiOsTimer, test_1 )
  {
    OS::Timer t;
    t.start ( );
    OS::sleep(1);
    t.stop( );
    OS::ElapsedTime e = t.getElapsed();
    EXPECT_NE ( e, 0 );
  }


  // Test 2: Sleep for a while and see that the timer isn't too far off.
  TEST( TestOcpiOsTimer, test_2 )
  {
    OS::Timer t ( true );
    OS::sleep ( 3000 );
    t.stop ( );
    OS::ElapsedTime e = t.getElapsed();
    unsigned int msecs = e.seconds() * 1000 + e.nanoseconds() / 1000000;
    // Allow for a 15% skew
    EXPECT_GE( msecs, 2550u );
    EXPECT_LE( msecs, 3450u );
  }


  // Test 3: Test that the timer can be re-started.
  TEST( TestOcpiOsTimer, test_3 )
  {
    OS::Timer t;

    for ( unsigned int i = 0; i < 10; i++ )
    {
      t.start ( );
      OS::sleep ( 300 );
      t.stop ( );
      OS::sleep ( 300 );
    }

    OS::ElapsedTime e = t.getElapsed();
    unsigned int msecs = e.seconds() * 1000 + e.nanoseconds() / 1000000;
    // Allow for a 1% skew
    EXPECT_GE( msecs, 2970u );
    EXPECT_LE( msecs, 3095u );
  }


  // Test 4: Timer reset
  TEST( TestOcpiOsTimer, test_4 )
  {
    OS::Timer t;

    for ( unsigned int i = 0; i < 10; i++ )
    {
      t.start ( );
      OS::sleep ( 300 );
      t.stop ( );
    }

    t.reset ( );

    for ( unsigned int i = 0; i < 10; i++ )
    {
      t.start ( );
      OS::sleep ( 300 );
      t.stop ( );
    }

    OS::ElapsedTime e = t.getElapsed();
    unsigned int msecs = e.seconds() * 1000 + e.nanoseconds() / 1000000;
    // Allow for a 1% skew
    EXPECT_GE( msecs, 2970u );
    EXPECT_LE( msecs, 3095u );
  }


  // Test 5: Timer precision: Expect better than 100ms
  TEST( TestOcpiOsTimer, test_5 )
  {
    OS::ElapsedTime e;
    OS::Timer::getPrecision ( e );
    EXPECT_EQ( e.seconds(), 0u );
    EXPECT_NE( e.bits(), 0u );
    EXPECT_LE( e.nanoseconds(), 100000000u );
  }


  //  Test 6: Elapsed time comparators
  TEST( TestOcpiOsTimer, test_6 )
  {
    OS::ElapsedTime e1, e2;

    e1.set( 100, 100);
    e2.set( 100, 100);

    EXPECT_EQ( e1, e2 );

    e1.set( 101, 100);
    e2.set( 100, 100);

    EXPECT_NE( e1, e2 );
    EXPECT_GT( e1, e2 );

    e1.set( 100, 101);
    e2.set( 100, 100);

    EXPECT_NE( e1, e2 );
    EXPECT_GT( e1, e2 );

    e1.set( 100, 99);
    e2.set( 100, 100);

    EXPECT_NE( e1, e2 );
    EXPECT_LT( e1, e2 );

    e1.set( 99, 100);
    e2.set( 100, 100);

    EXPECT_NE( e1, e2 );
    EXPECT_LT( e1, e2 );
  }


  // Test 7: Elapsed time arithmetic
  TEST( TestOcpiOsTimer, test_7 )
  {
    OS::ElapsedTime e1, e2, e3;

    e1.set( 100, 300);
    e2.set( 200, 400);
    e3 = e1 + e2;

    EXPECT_EQ( e3.seconds(), 300u );
    EXPECT_EQ( e3.nanoseconds(), 700u );

    e1.set( 100, 600000000);
    e2.set( 100, 600000000);
    e3 = e1 + e2;

    EXPECT_EQ( e3.seconds(), 201u );
    EXPECT_EQ( e3.nanoseconds(), 200000000u );

    e1.set( 300, 400);
    e2.set( 100, 100);
    e3 = e1 - e2;

    EXPECT_EQ( e3.seconds(), 200u );
    EXPECT_EQ( e3.nanoseconds(), 300u );

    e1.set( 300, 400);
    e2.set( 100, 500);
    e3 = e1 - e2;

    EXPECT_EQ( e3.seconds(), 199u );
    EXPECT_EQ( e3.nanoseconds(), 999999900u );
  }

} // End: namespace<unnamed>
