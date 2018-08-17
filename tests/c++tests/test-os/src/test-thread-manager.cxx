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

#include "OcpiOsThreadManager.h"

namespace
{
  class TestOcpiOsThreadManager : public ::testing::Test
  {
    // Empty
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

  // Test 1: Observe a thread modifying a variable
  TEST( TestOcpiOsThreadManager, test_1 )
  {
    g_variable_to_modify = false;
    OCPI::OS::ThreadManager tm ( thread_fn_variable_modification, 0 );
    tm.join ( );
    EXPECT_EQ( g_variable_to_modify, true );
  }


  // Test 2: Verify thread received its argument
  TEST( TestOcpiOsThreadManager, test_2 )
  {
    g_passed_argument = 0;
    OCPI::OS::ThreadManager tm ( thread_fn_argument_passing,
                                 ( void* ) thread_fn_argument_passing );
    tm.join ( );
    EXPECT_EQ( g_passed_argument, ( void* ) thread_fn_argument_passing );
  }

} // End: namespace<unnamed>

