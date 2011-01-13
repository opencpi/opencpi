
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

#include "OcpiOsMisc.h"
#include "OcpiOsEvent.h"
#include "OcpiOsDebug.h"
#include "OcpiOsThreadManager.h"

#include <cppunit/extensions/HelperMacros.h>

namespace
{
  class TestOcpiOsEvent : public CppUnit::TestFixture
  {
    private:
      CPPUNIT_TEST_SUITE( TestOcpiOsEvent );
      CPPUNIT_TEST( test_1 );
      CPPUNIT_TEST( test_2 );
      CPPUNIT_TEST( test_3 );
      CPPUNIT_TEST( test_4 );
      CPPUNIT_TEST( test_5 );
      CPPUNIT_TEST_SUITE_END();

    public:
      void setUp ( );
      void tearDown ( );

     void test_1 ( );
     void test_2 ( );
     void test_3 ( );
     void test_4 ( );
     void test_5 ( );
  };

  void test3Thread ( void* opaque )
  {
    OCPI::OS::Event* e = reinterpret_cast<OCPI::OS::Event *> ( opaque );
    e->set ();
  }

  struct Test4Data
  {
    OCPI::OS::Event e1;
    OCPI::OS::Event e2;
    int i;
  };

  void test4Thread ( void* opaque )
  {
    Test4Data* t4d = reinterpret_cast<Test4Data*> ( opaque );

    for ( int i = 0; i < 42; i++ )
    {
      t4d->i = i;
      t4d->e1.set ( );
      t4d->e2.wait ( );
    }
  }

} // End: namespace<unamed>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestOcpiOsEvent, "os" );

void TestOcpiOsEvent::setUp ( )
{
  // Empty
}


void TestOcpiOsEvent::tearDown ( )
{
  // Empty
}


// Test 1: Setting and waiting for the event
void TestOcpiOsEvent::test_1 ( )
{
  OCPI::OS::Event e;
  e.set ( );
  e.wait ( );
  CPPUNIT_ASSERT( true );
}


// Test 2: Initializing event to "set", then waiting for the event
void TestOcpiOsEvent::test_2 ( )
{
  OCPI::OS::Event e ( true );
  e.wait ( );
  CPPUNIT_ASSERT( true );
}

// Test 3: Setting the event in another thread
void TestOcpiOsEvent::test_3 ( )
{
  OCPI::OS::Event e;
  OCPI::OS::ThreadManager tm ( test3Thread, &e );
  e.wait ( );
  tm.join ( );
  CPPUNIT_ASSERT( true );
}

// Test 4: Leader and follower
void TestOcpiOsEvent::test_4 ( )
{
  Test4Data t4d;
  OCPI::OS::ThreadManager tm ( test4Thread, &t4d );

  for ( int i = 0; i < 42; i++ )
  {
    t4d.e1.wait ( );
    CPPUNIT_ASSERT ( t4d.i == i );
    t4d.e2.set ( );
  }

  tm.join ();
  CPPUNIT_ASSERT( true );
}

// Test 5: Waiting with a timeout
void TestOcpiOsEvent::test_5 ( )
{
  OCPI::OS::Event e;
  e.set ( );
  bool to = e.wait ( 100 );
  CPPUNIT_ASSERT( to );
  to = e.wait ( 100 );
  CPPUNIT_ASSERT ( !to );
}
