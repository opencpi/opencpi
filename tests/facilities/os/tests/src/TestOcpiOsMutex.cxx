
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

#include "OcpiOsMutex.h"
#include "OcpiOsMisc.h"
#include "OcpiOsDebug.h"
#include "OcpiOsThreadManager.h"

#include "gtest/gtest.h"

namespace
{
  class TestOcpiOsMutex : public ::testing::Test
  {
    // Empty
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

  // Test 1: Locking and Unlocking Mutex
  TEST( TestOcpiOsMutex, test_1 )
  {
    OCPI::OS::Mutex m;
    m.lock ( );
    m.unlock ( );
    EXPECT_EQ( true, true );
  }


  // Test 2: Attempting to lock mutex in another thread
  TEST( TestOcpiOsMutex, test_2 )
  {
    Test2Data t2d;
    t2d.flag = false;
    t2d.m.lock ( );
    OCPI::OS::ThreadManager tm ( test2Thread, &t2d );
    EXPECT_EQ( t2d.flag, false );
    t2d.m.unlock ( );
    tm.join ( );
    EXPECT_EQ( t2d.flag, true );
  }


  // Test 3: Using a recursive mutex
  TEST( TestOcpiOsMutex, test_3 )
  {
    Test3Data t3d;
    t3d.flag = false;
    t3d.m.lock ( );
    t3d.m.lock ( );
    OCPI::OS::ThreadManager tm ( test3Thread, &t3d );
    t3d.m.unlock ( );
    OCPI::OS::sleep ( 100 ); // give the other thread a chance to run
    EXPECT_EQ( t3d.flag, false );
    t3d.m.unlock ( );
    tm.join ( );
    EXPECT_EQ( t3d.flag, true );
  }

} // End: namespace<unnamed>
