
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

#include "OcpiOsFileSystem.h"
#include "OcpiOsDebug.h"

#include "gtest/gtest.h"

#include <ctime>
#include <string>
#include <cassert>
#include <fstream>
#include <iostream>

namespace
{
  class TestOcpiOsFileSystem : public ::testing::Test
  {
    protected:
      std::string d_a_file;

      virtual void SetUp ( );
      virtual void TearDown ( );
  };

  void removeAllFiles ( )
  {
    OCPI::OS::FileIterator it = OCPI::OS::FileSystem::list ( );
    while ( !it.end ( ) )
    {
      if ( it.isDirectory ( ) )
      {
        std::string cwd = OCPI::OS::FileSystem::cwd ( );
        // sanity check
        std::string name = it.relativeName ( );
        EXPECT_NE( name, "." );
        EXPECT_NE( name,".." );
        OCPI::OS::FileSystem::cd ( name );
        removeAllFiles ( );
        OCPI::OS::FileSystem::cd ( cwd );
      }
      else
      {
        OCPI::OS::FileSystem::remove ( it.relativeName ( ) );
      }
      it.next ( );
    }
  }

  void TestOcpiOsFileSystem::SetUp ( )
  {
    d_a_file = "/etc/hosts";

    // Go to a better place where we may have write permission.
    if ( OCPI::OS::FileSystem::exists ( "/tmp" ) )
    {
      OCPI::OS::FileSystem::cd ( "/tmp" );
    }
  }


  void TestOcpiOsFileSystem::TearDown ( )
  {
    // Empty
  }


  // Test 1: Precondition: delete test directory
  TEST_F( TestOcpiOsFileSystem, test_1 )
  {
    std::string cwd = OCPI::OS::FileSystem::cwd ( );
    std::string dirName = "fileSystemTestDirectory";
    std::string absDirName = OCPI::OS::FileSystem::joinNames ( cwd, dirName );

    if ( OCPI::OS::FileSystem::exists ( dirName ) )
    {
      OCPI::OS::FileSystem::cd ( dirName );
      EXPECT_EQ( OCPI::OS::FileSystem::cwd(), absDirName );
      removeAllFiles ( );
      OCPI::OS::FileSystem::cd ( cwd );
      OCPI::OS::FileSystem::rmdir ( absDirName );
    }
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName ), false );
  }


  // Test 2: Changing to current working directory
  TEST_F( TestOcpiOsFileSystem, test_2 )
  {
    std::string cwd = OCPI::OS::FileSystem::cwd ( );
    OCPI::OS::FileSystem::cd ( cwd );
    EXPECT_EQ( OCPI::OS::FileSystem::cwd( ), cwd );
  }


  // Test 3: Checking that d_a_file exists
  TEST_F( TestOcpiOsFileSystem, test_3 )
  {
    std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
    bool isDir;
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( nArgv0, &isDir ), true );
    EXPECT_EQ( isDir, false );
  }


  // Test 4: Checking absolute name of d_a_file
  TEST_F( TestOcpiOsFileSystem, test_4 )
  {
    std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
    std::string absArgv0 = OCPI::OS::FileSystem::absoluteName ( nArgv0 );
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( absArgv0 ), true );
  }


  // Test 5: Joining names
  TEST_F( TestOcpiOsFileSystem, test_5 )
  {
    std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
    std::string absArgv0 = OCPI::OS::FileSystem::absoluteName ( nArgv0 );
    std::string argv0Dir = OCPI::OS::FileSystem::directoryName ( absArgv0 );
    std::string argv0Rel = OCPI::OS::FileSystem::relativeName ( absArgv0 );
    std::string joinedName = OCPI::OS::FileSystem::joinNames ( argv0Dir,
                                                               argv0Rel );
    EXPECT_EQ( absArgv0, joinedName );
  }


  // Test 6: Joining absolute names
  TEST_F( TestOcpiOsFileSystem, test_6 )
  {
    std::string cwd = OCPI::OS::FileSystem::cwd ( );
    std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
    std::string absArgv0 = OCPI::OS::FileSystem::absoluteName ( nArgv0 );
    std::string joinedName = OCPI::OS::FileSystem::joinNames ( cwd, absArgv0 );
    EXPECT_EQ( absArgv0, joinedName );
  }

  // Test 7: Checking directory listing for argv0
  TEST_F( TestOcpiOsFileSystem, test_7 )
  {
    std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
    std::string absArgv0 = OCPI::OS::FileSystem::absoluteName ( nArgv0 );
    std::string argv0Dir = OCPI::OS::FileSystem::directoryName ( absArgv0 );
    std::string argv0Rel = OCPI::OS::FileSystem::relativeName ( absArgv0 );
    bool found = false;
    OCPI::OS::FileIterator it = OCPI::OS::FileSystem::list ( argv0Dir );
    while ( !it.end ( ) )
    {
      std::string relName = it.relativeName ( );
      std::string absName = it.absoluteName ( );
      if ( relName == argv0Rel )
      {
        EXPECT_EQ( absName, absArgv0 );
        EXPECT_EQ( it.isDirectory ( ), false );
        found = true;
        break;
      }
      it.next ( );
    }
    it.close ( );
    EXPECT_EQ( found, true );
  }


  // Test 8: Making and removing test directory
  TEST_F( TestOcpiOsFileSystem, test_8 )
  {
    std::string dirName = "fileSystemTestDirectory";
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName ), false );
    OCPI::OS::FileSystem::mkdir ( dirName );
    bool isDir;
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName, &isDir ), true );
    EXPECT_EQ( isDir, true );
    OCPI::OS::FileSystem::rmdir ( dirName );
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName ), false );
  }


  // Test 9: Path name of test directory
  TEST_F( TestOcpiOsFileSystem, test_9 )
  {
    std::string cwd = OCPI::OS::FileSystem::cwd ( );
    std::string dirName = "fileSystemTestDirectory";
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName ), false );
    OCPI::OS::FileSystem::mkdir ( dirName );
    OCPI::OS::FileSystem::cd ( dirName );
    std::string ncd = OCPI::OS::FileSystem::cwd ( );
    std::string ncdDir = OCPI::OS::FileSystem::directoryName ( ncd );
    std::string ncdRel = OCPI::OS::FileSystem::relativeName ( ncd );
    EXPECT_EQ( cwd, ncdDir );
    EXPECT_EQ( ncdRel, dirName );
    OCPI::OS::FileSystem::cd ( cwd );
    OCPI::OS::FileSystem::rmdir ( ncd );
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName ), false );
  }


  // Test 10: Creating a file
  TEST_F( TestOcpiOsFileSystem, test_10 )
  {
    std::string cwd = OCPI::OS::FileSystem::cwd ( );
    std::string dirName = "fileSystemTestDirectory";
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName ), false );
    OCPI::OS::FileSystem::mkdir ( dirName );
    OCPI::OS::FileSystem::cd ( dirName );
    std::string ncd = OCPI::OS::FileSystem::cwd ( );
    std::string fileName = "test10File.dat";
    std::string absFileName = OCPI::OS::FileSystem::joinNames ( ncd, fileName );
    std::string nativeName = OCPI::OS::FileSystem::toNativeName ( absFileName );

    {
      std::ofstream file ( nativeName.c_str ( ),
                           std::ios_base::out |
                           std::ios_base::trunc |
                           std::ios_base::binary );
      EXPECT_EQ( file.good ( ), true );
      file << "Hello World!" << std::endl;
      file.close ( );
    }

    bool isDir;
    EXPECT_EQ( OCPI::OS::FileSystem::exists (fileName, &isDir ), true );
    EXPECT_EQ( isDir, false );
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( absFileName, &isDir ), true );
    EXPECT_EQ( isDir, false );
    unsigned long long fileSize = OCPI::OS::FileSystem::size ( fileName );
    EXPECT_EQ( fileSize, 13ull );
    std::time_t now = time ( 0 );
    std::time_t fileTime = OCPI::OS::FileSystem::lastModified ( fileName );
    EXPECT_LT( ( now - fileTime ), 3 );

    OCPI::OS::FileSystem::remove ( absFileName );
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( fileName ), false );

    OCPI::OS::FileSystem::cd ( cwd );
    OCPI::OS::FileSystem::rmdir ( ncd );
    EXPECT_EQ( OCPI::OS::FileSystem::exists ( dirName ), false );
  }

} // End: namespace<unnamed>
