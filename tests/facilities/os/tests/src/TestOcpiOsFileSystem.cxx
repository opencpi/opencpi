
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

#include "OcpiOsFileSystem.h"
#include "OcpiOsDebug.h"

#include <ctime>
#include <string>
#include <cassert>
#include <fstream>
#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

namespace
{
  class TestOcpiOsFileSystem : public CppUnit::TestFixture
  {
    private:
      std::string d_a_file;

    private:
      CPPUNIT_TEST_SUITE( TestOcpiOsFileSystem );
      CPPUNIT_TEST( test_1 );
      CPPUNIT_TEST( test_2 );
      CPPUNIT_TEST( test_3 );
      CPPUNIT_TEST( test_4 );
      CPPUNIT_TEST( test_5 );
      CPPUNIT_TEST( test_6 );
      CPPUNIT_TEST( test_7 );
      CPPUNIT_TEST( test_8 );
      CPPUNIT_TEST( test_9 );
      CPPUNIT_TEST( test_10 );
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
     void test_8 ( );
     void test_9 ( );
     void test_10 ( );
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
        CPPUNIT_ASSERT ( name != "." && name != ".." );
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

} // End: namespace<unamed>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TestOcpiOsFileSystem, "os" );

void TestOcpiOsFileSystem::setUp ( )
{
  d_a_file = "/etc/motd";

  // Go to a better place where we may have write permission.
  if ( OCPI::OS::FileSystem::exists ( "/tmp" ) )
  {
    OCPI::OS::FileSystem::cd ( "/tmp" );
  }
}


void TestOcpiOsFileSystem::tearDown ( )
{
  // Empty
}


// Test 1: Precondition: delete test directory
void TestOcpiOsFileSystem::test_1 ( )
{
  std::string cwd = OCPI::OS::FileSystem::cwd ( );
  std::string dirName = "fileSystemTestDirectory";
  std::string absDirName = OCPI::OS::FileSystem::joinNames ( cwd, dirName );

  if ( OCPI::OS::FileSystem::exists ( dirName ) )
  {
    OCPI::OS::FileSystem::cd ( dirName );
    CPPUNIT_ASSERT( OCPI::OS::FileSystem::cwd() == absDirName );
    removeAllFiles ( );
    OCPI::OS::FileSystem::cd ( cwd );
    OCPI::OS::FileSystem::rmdir ( absDirName );
  }
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( dirName ) );
}


// Test 2: Changing to current working directory
void TestOcpiOsFileSystem::test_2 ( )
{
  std::string cwd = OCPI::OS::FileSystem::cwd ( );
  OCPI::OS::FileSystem::cd ( cwd );
  CPPUNIT_ASSERT( OCPI::OS::FileSystem::cwd( ) == cwd );
}


// Test 3: Checking that d_a_file exists
void TestOcpiOsFileSystem::test_3 ( )
{
  std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
  bool isDir;
  CPPUNIT_ASSERT( OCPI::OS::FileSystem::exists ( nArgv0, &isDir ) );
  CPPUNIT_ASSERT( !isDir );
}


// Test 4: Checking absolute name of d_a_file
void TestOcpiOsFileSystem::test_4 ( )
{
  std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
  std::string absArgv0 = OCPI::OS::FileSystem::absoluteName ( nArgv0 );
  CPPUNIT_ASSERT( OCPI::OS::FileSystem::exists ( absArgv0 ) );
}


// Test 5: Joining names
void TestOcpiOsFileSystem::test_5 ( )
{
  std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
  std::string absArgv0 = OCPI::OS::FileSystem::absoluteName ( nArgv0 );
  std::string argv0Dir = OCPI::OS::FileSystem::directoryName ( absArgv0 );
  std::string argv0Rel = OCPI::OS::FileSystem::relativeName ( absArgv0 );
  std::string joinedName = OCPI::OS::FileSystem::joinNames ( argv0Dir,
                                                             argv0Rel );
  CPPUNIT_ASSERT( absArgv0 == joinedName );
}


// Test 6: Joining absolute names
void TestOcpiOsFileSystem::test_6 ( )
{
  std::string cwd = OCPI::OS::FileSystem::cwd ( );
  std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName ( d_a_file );
  std::string absArgv0 = OCPI::OS::FileSystem::absoluteName ( nArgv0 );
  std::string joinedName = OCPI::OS::FileSystem::joinNames ( cwd, absArgv0 );
  CPPUNIT_ASSERT( absArgv0 == joinedName );
}

// Test 7: Checking directory listing for argv0
void TestOcpiOsFileSystem::test_7 ( )
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
      CPPUNIT_ASSERT( absName == absArgv0 );
      CPPUNIT_ASSERT( !it.isDirectory ( ) );
      found = true;
      break;
    }
    it.next ( );
  }
  it.close ( );
  CPPUNIT_ASSERT( found );
}


// Test 8: Making and removing test directory
void TestOcpiOsFileSystem::test_8 ( )
{
  std::string dirName = "fileSystemTestDirectory";
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( dirName ) );
  OCPI::OS::FileSystem::mkdir ( dirName );
  bool isDir;
  CPPUNIT_ASSERT( OCPI::OS::FileSystem::exists ( dirName, &isDir ) );
  CPPUNIT_ASSERT( isDir );
  OCPI::OS::FileSystem::rmdir ( dirName );
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( dirName ) );
}


// Test 9: Path name of test directory
void TestOcpiOsFileSystem::test_9 ( )
{
  std::string cwd = OCPI::OS::FileSystem::cwd ( );
  std::string dirName = "fileSystemTestDirectory";
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( dirName ) );
  OCPI::OS::FileSystem::mkdir ( dirName );
  OCPI::OS::FileSystem::cd ( dirName );
  std::string ncd = OCPI::OS::FileSystem::cwd ( );
  std::string ncdDir = OCPI::OS::FileSystem::directoryName ( ncd );
  std::string ncdRel = OCPI::OS::FileSystem::relativeName ( ncd );
  CPPUNIT_ASSERT( cwd == ncdDir );
  CPPUNIT_ASSERT( ncdRel == dirName );
  OCPI::OS::FileSystem::cd ( cwd );
  OCPI::OS::FileSystem::rmdir ( ncd );
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( dirName ) );
}


// Test 10: Creating a file
void TestOcpiOsFileSystem::test_10 ( )
{
  std::string cwd = OCPI::OS::FileSystem::cwd ( );
  std::string dirName = "fileSystemTestDirectory";
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( dirName ) );
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
    CPPUNIT_ASSERT( file.good ( ) );
    file << "Hello World!" << std::endl;
    file.close ( );
  }

  bool isDir;
  CPPUNIT_ASSERT( OCPI::OS::FileSystem::exists (fileName, &isDir ) );
  CPPUNIT_ASSERT( !isDir );
  CPPUNIT_ASSERT( OCPI::OS::FileSystem::exists ( absFileName, &isDir ) );
  CPPUNIT_ASSERT( !isDir );
  unsigned long long fileSize = OCPI::OS::FileSystem::size ( fileName );
  CPPUNIT_ASSERT( fileSize == 13 );
  std::time_t now = time ( 0 );
  std::time_t fileTime = OCPI::OS::FileSystem::lastModified ( fileName );
  CPPUNIT_ASSERT( ( now - fileTime ) < 3 );

  OCPI::OS::FileSystem::remove ( absFileName );
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( fileName ) );

  OCPI::OS::FileSystem::cd ( cwd );
  OCPI::OS::FileSystem::rmdir ( ncd );
  CPPUNIT_ASSERT( !OCPI::OS::FileSystem::exists ( dirName ) );
}
