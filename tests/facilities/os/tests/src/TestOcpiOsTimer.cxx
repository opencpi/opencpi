
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


#include "OcpiOsTimer.h"
#include "OcpiOsMisc.h"
#include "OcpiOsDebug.h"

#include <cppunit/extensions/HelperMacros.h>

namespace
{
  class TestOcpiOsTimer : public CppUnit::TestFixture
  {
    private:
      std::string d_argv0;

    private:
      CPPUNIT_TEST_SUITE( TestOcpiOsTimer );
      CPPUNIT_TEST( test_1 );
      CPPUNIT_TEST( test_2 );
      CPPUNIT_TEST( test_3 );
      CPPUNIT_TEST( test_4 );
      CPPUNIT_TEST( test_5 );
      CPPUNIT_TEST( test_6 );
      CPPUNIT_TEST( test_7 );
      CPPUNIT_TEST_SUITE_END();

    public:
      void setUp ( );
      void tearDown ( );

     void test_1 ( );
     void test_2 ( );
     void test_3 ( );
     void test_4 ( );
     void test_5 ( );
     void test_6 ( );
     void test_7 ( );
  };

} // End: namespace<unamed>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestOcpiOsTimer, "os" );

void TestOcpiOsTimer::setUp ( )
{

}


void TestOcpiOsTimer::tearDown ( )
{
  // Empty
}


// Test 1: Check that the timer functionally works.
void TestOcpiOsTimer::test_1 ( )
{
  OCPI::OS::Timer t;
  OCPI::OS::Timer::ElapsedTime e;
  t.start ( );
  t.stop( );
  t.getValue ( e );
  CPPUNIT_ASSERT ( true );
}


// Test 2: Sleep for a while and see that the timer isn't too far off.
void TestOcpiOsTimer::test_2 ( )
{
  OCPI::OS::Timer::ElapsedTime e;
  OCPI::OS::Timer t ( true );
  OCPI::OS::sleep ( 3000 );
  t.stop ( );
  t.getValue ( e );
  unsigned int msecs = e.seconds * 1000 + e.nanoseconds / 1000000;
  // Allow for a 1% skew
  CPPUNIT_ASSERT( msecs >= 2970 && msecs <= 3030 );
}


// Test 3: Test that the timer can be re-started.
void TestOcpiOsTimer::test_3 ( )
{
  OCPI::OS::Timer::ElapsedTime e;
  OCPI::OS::Timer t;

  for ( unsigned int i = 0; i < 10; i++ )
  {
    t.start ( );
    OCPI::OS::sleep ( 300 );
    t.stop ( );
    OCPI::OS::sleep ( 300 );
  }

  t.getValue ( e );
  unsigned int msecs = e.seconds * 1000 + e.nanoseconds / 1000000;
  // Allow for a 1% skew
  CPPUNIT_ASSERT( msecs >= 2970 && msecs <= 3030 );
}


// Test 4: Timer reset
void TestOcpiOsTimer::test_4 ( )
{
  OCPI::OS::Timer::ElapsedTime e;
  OCPI::OS::Timer t;

  for ( unsigned int i = 0; i < 10; i++ )
  {
    t.start ( );
    OCPI::OS::sleep ( 300 );
    t.stop ( );
  }

  t.reset ( );

  for ( unsigned int i = 0; i < 10; i++ )
  {
    t.start ( );
    OCPI::OS::sleep ( 300 );
    t.stop ( );
  }

  t.getValue ( e );
  unsigned int msecs = e.seconds * 1000 + e.nanoseconds / 1000000;
  // Allow for a 1% skew
  CPPUNIT_ASSERT( msecs >= 2970 && msecs <= 3030 );
}


// Test 5: Timer precision: Expect better than 100ms
void TestOcpiOsTimer::test_5 ( )
{
  OCPI::OS::Timer::ElapsedTime e;
  OCPI::OS::Timer::getPrecision ( e );
  CPPUNIT_ASSERT( e.seconds == 0 );
  CPPUNIT_ASSERT( e.nanoseconds != 0 );
  CPPUNIT_ASSERT( e.nanoseconds <= 100000000 );
}


//  Test 6: Elapsed time comparators
void TestOcpiOsTimer::test_6 ( )
{
  OCPI::OS::Timer::ElapsedTime e1, e2;

  e1.seconds = 100; e1.nanoseconds = 100;
  e2.seconds = 100; e2.nanoseconds = 100;

  CPPUNIT_ASSERT( e1 == e2 );
  CPPUNIT_ASSERT( !( e1 != e2 ) );
  CPPUNIT_ASSERT( !( e1 > e2 ) );
  CPPUNIT_ASSERT( !( e1 < e2 ) );

  e1.seconds = 101; e1.nanoseconds = 100;
  e2.seconds = 100; e2.nanoseconds = 100;

  CPPUNIT_ASSERT( !( e1 == e2 ) );
  CPPUNIT_ASSERT( e1 != e2 );
  CPPUNIT_ASSERT( !( e1 < e2 ) );
  CPPUNIT_ASSERT( e1 > e2 );

  e1.seconds = 100; e1.nanoseconds = 101;
  e2.seconds = 100; e2.nanoseconds = 100;

  CPPUNIT_ASSERT( !( e1 == e2 ) );
  CPPUNIT_ASSERT( e1 != e2);
  CPPUNIT_ASSERT( !( e1 < e2 ) );
  CPPUNIT_ASSERT( e1 > e2 );

  e1.seconds = 100; e1.nanoseconds = 99;
  e2.seconds = 100; e2.nanoseconds = 100;

  CPPUNIT_ASSERT( !( e1 == e2 ) );
  CPPUNIT_ASSERT( e1 != e2) ;
  CPPUNIT_ASSERT( e1 < e2 );
  CPPUNIT_ASSERT( !(e1 > e2));

  e1.seconds = 99;  e1.nanoseconds = 100;
  e2.seconds = 100; e2.nanoseconds = 100;

  CPPUNIT_ASSERT( !(e1 == e2 ) );
  CPPUNIT_ASSERT( e1 != e2 );
  CPPUNIT_ASSERT( e1 < e2 );
  CPPUNIT_ASSERT( !( e1 > e2 ) );
}


// Test 7: Elapsed time arithmetic
void TestOcpiOsTimer::test_7 ( )
{
  OCPI::OS::Timer::ElapsedTime e1, e2, e3;

  e1.seconds = 100; e1.nanoseconds = 300;
  e2.seconds = 200; e2.nanoseconds = 400;
  e3 = e1 + e2;

  CPPUNIT_ASSERT( e3.seconds == 300 );
  CPPUNIT_ASSERT( e3.nanoseconds == 700 );

  e1.seconds = 100; e1.nanoseconds = 600000000;
  e2.seconds = 100; e2.nanoseconds = 600000000;
  e3 = e1 + e2;

  CPPUNIT_ASSERT( e3.seconds == 201 );
  CPPUNIT_ASSERT( e3.nanoseconds == 200000000 );

  e1.seconds = 300; e1.nanoseconds = 400;
  e2.seconds = 100; e2.nanoseconds = 100;
  e3 = e1 - e2;

  CPPUNIT_ASSERT( e3.seconds == 200 );
  CPPUNIT_ASSERT( e3.nanoseconds == 300 );

  e1.seconds = 300; e1.nanoseconds = 400;
  e2.seconds = 100; e2.nanoseconds = 500;
  e3 = e1 - e2;

  CPPUNIT_ASSERT( e3.seconds == 199 );
  CPPUNIT_ASSERT( e3.nanoseconds == 999999900 );
}
