
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

#include "OcpiOsThreadManager.h"

#include <cppunit/extensions/HelperMacros.h>

namespace
{
  class TestOcpiOsThreadManager : public CppUnit::TestFixture
  {
    private:
      CPPUNIT_TEST_SUITE( TestOcpiOsThreadManager );
      CPPUNIT_TEST( test_1 );
      CPPUNIT_TEST( test_2 );
      CPPUNIT_TEST_SUITE_END();

    public:
      void setUp ( );
      void tearDown ( );

     void test_2 ( );
     void test_1 ( );
  };

  bool g_variable_to_modify;

  void thread_fn_variable_modification ( void* opaque )
  {
    ( void ) opaque;
    g_variable_to_modify = true;
  }

  void* g_passed_argument;

  void thread_fn_argument_passing ( void* opaque )
  {
    g_passed_argument = opaque;
  }

} // End: namespace<unamed>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestOcpiOsThreadManager, "os" );

void TestOcpiOsThreadManager::setUp ( )
{
  // Empty
}


void TestOcpiOsThreadManager::tearDown ( )
{
  // Empty
}


// Test 1: Observe a thread modifying a variable
void TestOcpiOsThreadManager::test_1 ( )
{
  g_variable_to_modify = false;
  OCPI::OS::ThreadManager tm ( thread_fn_variable_modification, 0 );
  tm.join ( );
  CPPUNIT_ASSERT( g_variable_to_modify == true );
}


// Test 2: Verify thread received its argument
void TestOcpiOsThreadManager::test_2 ( )
{
  g_passed_argument = 0;
  OCPI::OS::ThreadManager tm ( thread_fn_argument_passing,
                               ( void* ) thread_fn_argument_passing );
  tm.join ( );
  CPPUNIT_ASSERT( g_passed_argument == ( void* ) thread_fn_argument_passing );
}
