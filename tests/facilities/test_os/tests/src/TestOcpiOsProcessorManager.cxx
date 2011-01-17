
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

#include "OcpiOsProcessManager.h"
#include "OcpiOsMisc.h"
#include "OcpiOsDebug.h"
#include "OcpiOsFileSystem.h"

#include "gtest/gtest.h"

#include <ctime>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <csetjmp>

#include <string>
#include <iostream>

namespace
{
  class TestOcpiOsProcessManager : public ::testing::Test
  {
    protected:
      std::string d_argv0;
      bool d_perform_test;

      virtual void SetUp ( );
      virtual void TearDown ( );
  };

  char i2sDigits [ 16 ] =
  {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };

  std::string unsignedToString ( unsigned int value,
                                 unsigned int base = 10,
                                 unsigned int mindigits = 0,
                                 char pad = '0')
  {
    unsigned int tmp;
    unsigned int count=1;
    unsigned int padLen;

    if ( base < 2 || base > 16 )
    {
      throw std::string ( "invalid base" );
    }

    /* Determine length of number */
    for ( tmp = value; tmp >= base; count++ )
    {
      tmp /= base;
    }

    /* Initialize string.*/
    std::string result;

    if ( mindigits > count )
    {
      padLen = mindigits - count;
      result.resize ( mindigits, pad );
    }
    else
    {
      padLen = 0;
      result.resize ( count );
    }

    /* Stringify number. */
    while ( value >= base )
    {
      result [ --count + padLen ] = i2sDigits [ value % base ];
      value /= base;
    }

    // assert (count == 1 && value < base)

    result [ --count + padLen ] = i2sDigits [ value ];
    return result;
  }

  unsigned int stringToUnsigned ( const std::string& str,
                                  unsigned int base = 10)
  {
    const char* txtPtr = str.c_str ( );
    unsigned long int value;
    char* endPtr;

    value = std::strtoul ( txtPtr, &endPtr, base );

    if ( ( value == 0 && endPtr == txtPtr ) || *endPtr )
    {
      throw std::string ( "not an unsigned integer" );
    }

    unsigned int res = static_cast<unsigned int> ( value );

    if ( static_cast<unsigned long int>( res ) != value )
    {
      throw std::string ( "unsigned integer out of range" );
    }

    return res;
  }

  const char* test5CommandLine1 [ ] = { "simple", 0 };
  const char* test5CommandLine2 [ ] = { "par1", "par2", "par3", "par4", 0 };
  const char* test5CommandLine3 [ ] = { "parameter with spaces", 0 };
  const char* test5CommandLine4 [ ] = { "parameter with \"quotes\"", 0 };
  const char* test5CommandLine5 [ ] = { "with\\some\\backslashes", 0 };
  const char* test5CommandLine6 [ ] = { "backslash before a \\\"quote\\\"", 0 };
  const char* test5CommandLine7 [ ] = { "\\backslashes at both ends\\", 0 };

  const char* const* test5CommandLines [ ] =
  {
    test5CommandLine1,
    test5CommandLine2,
    test5CommandLine3,
    test5CommandLine4,
    test5CommandLine5,
    test5CommandLine6,
    test5CommandLine7,
  };

  int testProcessManagerInt ( int argc, char* argv [ ] )
  {
    if ( argc >= 2 )
    {
      unsigned int testNum;

      try
      {
        testNum = stringToUnsigned ( argv [ 1 ] );
      }
      catch ( const std::string& oops )
      {
        std::cout << "oops: " << oops << std::endl;
        return 0;
      }

      if ( testNum == 1 )
      {
        // dummy test
      }
      else if ( testNum == 2 )
      {
        // test exit code
        return 42;
      }
      else if ( testNum == 3 )
      {
        // waiting for shutdown
        for ( int i = 0; i < 1000; i++ )
        {
          OCPI::OS::sleep ( 1000 );
        }
      }
      else if ( testNum == 4 )
      {
        // run multiple processes
        OCPI::OS::sleep ( 10000 );
        return ( std::time ( 0 ) % 128 );
      }
      else if ( testNum == 5 )
      {
        unsigned int numTests = sizeof ( test5CommandLines ) /
                                         sizeof ( char ** );
        unsigned int testNo;

        if ( argc < 3 )
        {
          return 0;
        }

        try
        {
          testNo = stringToUnsigned ( argv [ 2 ] );
        }
        catch ( const std::string& )
        {
          return 0;
        }

        if ( testNo > numTests )
        {
          return 0;
        }

        unsigned int testIdx = testNo - 1;

        const char* const* commandLine = test5CommandLines [ testIdx ];

        int argci = 3;

        for ( unsigned int pi = 0; commandLine [ pi ]; pi++, argci++ )
        {
          if ( argci >= argc )
          {
            return 0;
          }
          else if ( std::strcmp ( commandLine [ pi ], argv [ argci ] ) != 0 )
          {
            return 0;
          }
        }

        if ( argci != argc )
        {
          return 0;
        }

        return static_cast<int> ( testNo );
      }
      else
      {
        std::cout << "oops" << std::endl;
      }
    }
    else
    {
      std::cout << "oops" << std::endl;
    }

    return 0;
  }

  void TestOcpiOsProcessManager::SetUp ( )
  {
    std::string d_argv0 ( "testProcessManagerInt" );
    d_perform_test = false;
  }


  void TestOcpiOsProcessManager::TearDown ( )
  {
    // Empty
  }


  // Test 1: Starting and waiting for a process
  TEST_F( TestOcpiOsProcessManager, test_1 )
  {
    if ( !d_perform_test )
    {
      return;
    }
    OCPI::OS::ProcessManager::ParameterList p;
    p.push_back ( "1" );
    OCPI::OS::ProcessManager pm ( d_argv0, p );
    pm.wait ( );
    EXPECT_EQ( true, true );
  }


  // Test 2: Check exit code
  TEST_F( TestOcpiOsProcessManager, test_2 )
  {
    if ( !d_perform_test )
    {
      return;
    }
    OCPI::OS::ProcessManager::ParameterList p;
    p.push_back ( "2" );
    OCPI::OS::ProcessManager pm ( d_argv0, p );
    pm.wait ( );
    int ec = pm.getExitCode ( );
    EXPECT_EQ( ec, 42 );
  }


  // Test 3: Shutdown process
  TEST_F( TestOcpiOsProcessManager, test_3 )
  {
    if ( !d_perform_test )
    {
      return;
    }
    OCPI::OS::ProcessManager::ParameterList p;
    p.push_back ( "3" );
    OCPI::OS::ProcessManager pm ( d_argv0, p );
    pm.wait ( 1000 );
    pm.shutdown ( );
    pm.wait ( );
    EXPECT_EQ( true, true );
  }


  // Test 4: Run multiple processes
  TEST_F( TestOcpiOsProcessManager, test_4 )
  {
    if ( !d_perform_test )
    {
      return;
    }
    OCPI::OS::ProcessManager::ParameterList p;
    p.push_back ( "4" );
    OCPI::OS::ProcessManager pm1 ( d_argv0, p );
    OCPI::OS::ProcessManager pm2 ( d_argv0, p );
    pm1.detach  ();
    OCPI::OS::ProcessManager pm3 ( d_argv0, p );
    pm3.wait ( );
    pm2.wait ( );

    /*
     * The process returns time() as its exit code.  That should tell us that
     * the above actually happened in parallel (their exit codes are the same
     * or at least very similar) rather than sequentially (their exit codes
     * are 10 or more apart).
     */

    unsigned int ec2 = static_cast<unsigned int>( pm2.getExitCode ( ) );
    unsigned int ec3 = static_cast<unsigned int>( pm3.getExitCode ( ) );
    EXPECT_LT( ( ( ec3 - ec2 ) % 128 ), 2 );
  }


  // Test 5: Command-line parameters
  TEST_F( TestOcpiOsProcessManager, test_5 )
  {
    if ( !d_perform_test )
    {
      return;
    }
    unsigned int numTests = sizeof ( test5CommandLines ) / sizeof ( char** );

    for ( unsigned int testIdx = 0; testIdx < numTests; testIdx++ )
    {
      const char* const* commandLine = test5CommandLines [ testIdx ];

      unsigned int testNo = testIdx + 1;

      OCPI::OS::ProcessManager::ParameterList p;
      p.push_back ( "5" );
      p.push_back ( unsignedToString ( testNo ) );

      for ( unsigned int pi = 0; commandLine [ pi ]; pi++ )
      {
        p.push_back ( commandLine [ pi ] );
      }

      OCPI::OS::ProcessManager pm ( d_argv0, p );
      pm.wait ( );

      int ec = pm.getExitCode ( );
      EXPECT_EQ( static_cast<unsigned int>( ec ), testNo );
    }
  }

} // End: namespace<unnamed>
