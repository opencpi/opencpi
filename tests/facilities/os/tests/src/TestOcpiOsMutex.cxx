
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

#include "OcpiOsMutex.h"
#include "OcpiOsMisc.h"
#include "OcpiOsDebug.h"
#include "OcpiOsThreadManager.h"

#include <cppunit/extensions/HelperMacros.h>

namespace
{
  class TestOcpiOsMutex : public CppUnit::TestFixture
  {
    private:
      CPPUNIT_TEST_SUITE( TestOcpiOsMutex );
      CPPUNIT_TEST( test_1 );
      CPPUNIT_TEST( test_2 );
      CPPUNIT_TEST( test_3 );
      CPPUNIT_TEST_SUITE_END();

    public:
      void setUp ( );
      void tearDown ( );

     void test_1 ( );
     void test_2 ( );
     void test_3 ( );
  };

  struct Test2Data
  {
    OCPI::OS::Mutex m;
    bool flag;
  };

  void test2Thread ( void* opaque )
  {
    Test2Data* t2d = reinterpret_cast<Test2Data*> ( opaque );
    t2d->m.lock ( );
    t2d->flag = true;
    t2d->m.unlock ( );
  }


  struct Test3Data {
    Test3Data ( )
      : m ( true )
    {
      // Empty
    }
    OCPI::OS::Mutex m;
    bool flag;
  };

  void test3Thread ( void* opaque )
  {
    Test3Data* t3d = reinterpret_cast<Test3Data*> ( opaque );
    t3d->m.lock ( );
    t3d->flag = true;
    t3d->m.unlock ( );
  }

} // End: namespace<unamed>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestOcpiOsMutex, "os" );

void TestOcpiOsMutex::setUp ( )
{

}


void TestOcpiOsMutex::tearDown ( )
{
  // Empty
}


// Test 1: Locking and Unlocking Mutex
void TestOcpiOsMutex::test_1 ( )
{
  OCPI::OS::Mutex m;
  m.lock ( );
  m.unlock ( );
  CPPUNIT_ASSERT( true );
}


// Test 2: Attempting to lock mutex in another thread
void TestOcpiOsMutex::test_2 ( )
{
  Test2Data t2d;
  t2d.flag = false;
  t2d.m.lock ( );
  OCPI::OS::ThreadManager tm ( test2Thread, &t2d );
  CPPUNIT_ASSERT( !t2d.flag );
  t2d.m.unlock ( );
  tm.join ( );
  CPPUNIT_ASSERT( t2d.flag );
}


// Test 3: Using a recursive mutex
void TestOcpiOsMutex::test_3 ( )
{
  Test3Data t3d;
  t3d.flag = false;
  t3d.m.lock ( );
  t3d.m.lock ( );
  OCPI::OS::ThreadManager tm ( test3Thread, &t3d );
  t3d.m.unlock ( );
  OCPI::OS::sleep ( 100 ); // give the other thread a chance to run
  CPPUNIT_ASSERT( !t3d.flag );
  t3d.m.unlock ( );
  tm.join ( );
  CPPUNIT_ASSERT( t3d.flag );
}

