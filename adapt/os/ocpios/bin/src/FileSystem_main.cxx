
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


#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cassert>
#include <OcpiOsDebug.h>
#include <OcpiOsFileSystem.h>
#include "OcpiUtilTest.h"

namespace FileSystemTests {

  /*
   * ----------------------------------------------------------------------
   * Test 01: Precondition: delete test directory
   * ----------------------------------------------------------------------
   */

  void
  removeAllFiles ()
  {
    OCPI::OS::FileIterator it = OCPI::OS::FileSystem::list ();
    while (!it.end()) {
      if (it.isDirectory()) {
        std::string cwd = OCPI::OS::FileSystem::cwd ();
        // sanity check
        std::string name = it.relativeName ();
        assert (name != "." && name != "..");
        OCPI::OS::FileSystem::cd (name);
        removeAllFiles ();
        OCPI::OS::FileSystem::cd (cwd);
      }
      else {
        OCPI::OS::FileSystem::remove (it.relativeName());
      }
      it.next ();
    }
  }

  class Test01 : public OCPI::Util::Test::Test {
  public:
    Test01 ()
      : OCPI::Util::Test::Test ("Delete test directory, if it exists")
    {
    }

    void run ()
    {
      std::string cwd = OCPI::OS::FileSystem::cwd ();
      std::string dirName = "fileSystemTestDirectory";
      std::string absDirName = OCPI::OS::FileSystem::joinNames (cwd, dirName);
      if (OCPI::OS::FileSystem::exists (dirName)) {
        OCPI::OS::FileSystem::cd (dirName);
        test (OCPI::OS::FileSystem::cwd() == absDirName);
        removeAllFiles ();
        OCPI::OS::FileSystem::cd (cwd);
        OCPI::OS::FileSystem::rmdir (absDirName);
      }
     test (!OCPI::OS::FileSystem::exists (dirName));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 02: Changing to current working directory
   * ----------------------------------------------------------------------
   */

  class Test02 : public OCPI::Util::Test::Test {
  public:
    Test02 ()
      : OCPI::Util::Test::Test ("Changing to current working directory")
    {
    }

    void run ()
    {
      std::string cwd = OCPI::OS::FileSystem::cwd ();
      OCPI::OS::FileSystem::cd (cwd);
      test (OCPI::OS::FileSystem::cwd() == cwd);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 03: Checking that argv0 exists
   * ----------------------------------------------------------------------
   */

  class Test03 : public OCPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test03 (const std::string & argv0)
      : OCPI::Util::Test::Test ("Checking that argv0 exists"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName (m_argv0);
      bool isDir;
      test (OCPI::OS::FileSystem::exists (nArgv0, &isDir));
      test (!isDir);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 04: Checking absolute name of argv0
   * ----------------------------------------------------------------------
   */

  class Test04 : public OCPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test04 (const std::string & argv0)
      : OCPI::Util::Test::Test ("Checking absolute name of argv0"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = OCPI::OS::FileSystem::absoluteName (nArgv0);
      test (OCPI::OS::FileSystem::exists (absArgv0));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 05: Joining names
   * ----------------------------------------------------------------------
   */

  class Test05 : public OCPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test05 (const std::string & argv0)
      : OCPI::Util::Test::Test ("Joining names"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = OCPI::OS::FileSystem::absoluteName (nArgv0);
      std::string argv0Dir = OCPI::OS::FileSystem::directoryName (absArgv0);
      std::string argv0Rel = OCPI::OS::FileSystem::relativeName (absArgv0);
      std::string joinedName = OCPI::OS::FileSystem::joinNames (argv0Dir, argv0Rel);
      test (absArgv0 == joinedName);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 06: Joining absolute names
   * ----------------------------------------------------------------------
   */

  class Test06 : public OCPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test06 (const std::string & argv0)
      : OCPI::Util::Test::Test ("Joining absolute names"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string cwd = OCPI::OS::FileSystem::cwd ();
      std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = OCPI::OS::FileSystem::absoluteName (nArgv0);
      std::string joinedName = OCPI::OS::FileSystem::joinNames (cwd, absArgv0);
      test (absArgv0 == joinedName);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 07: Checking directory listing for argv0
   * ----------------------------------------------------------------------
   */

  class Test07 : public OCPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test07 (const std::string & argv0)
      : OCPI::Util::Test::Test ("Checking directory listing for argv0"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = OCPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = OCPI::OS::FileSystem::absoluteName (nArgv0);
      std::string argv0Dir = OCPI::OS::FileSystem::directoryName (absArgv0);
      std::string argv0Rel = OCPI::OS::FileSystem::relativeName (absArgv0);
      bool found = false;
      OCPI::OS::FileIterator it = OCPI::OS::FileSystem::list (argv0Dir);
      while (!it.end()) {
        std::string relName = it.relativeName ();
        std::string absName = it.absoluteName ();
        if (relName == argv0Rel) {
          test (absName == absArgv0);
          test (!it.isDirectory());
          found = true;
          break;
        }
        it.next ();
      }
      it.close ();
      test (found);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 08: Making and removing test directory
   * ----------------------------------------------------------------------
   */

  class Test08 : public OCPI::Util::Test::Test {
  public:
    Test08 ()
      : OCPI::Util::Test::Test ("Making and removing test directory")
    {
    }

    void run ()
    {
      std::string dirName = "fileSystemTestDirectory";
      test (!OCPI::OS::FileSystem::exists (dirName));
      OCPI::OS::FileSystem::mkdir (dirName);
      bool isDir;
      test (OCPI::OS::FileSystem::exists (dirName, &isDir));
      test (isDir);
      OCPI::OS::FileSystem::rmdir (dirName);
      test (!OCPI::OS::FileSystem::exists (dirName));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 09: Path name of test directory
   * ----------------------------------------------------------------------
   */

  class Test09 : public OCPI::Util::Test::Test {
  public:
    Test09 ()
      : OCPI::Util::Test::Test ("Path name of test directory")
    {
    }

    void run ()
    {
      std::string cwd = OCPI::OS::FileSystem::cwd ();
      std::string dirName = "fileSystemTestDirectory";
      test (!OCPI::OS::FileSystem::exists (dirName));
      OCPI::OS::FileSystem::mkdir (dirName);
      OCPI::OS::FileSystem::cd (dirName);
      std::string ncd = OCPI::OS::FileSystem::cwd ();
      std::string ncdDir = OCPI::OS::FileSystem::directoryName (ncd);
      std::string ncdRel = OCPI::OS::FileSystem::relativeName (ncd);
      test (cwd == ncdDir);
      test (ncdRel == dirName);
      OCPI::OS::FileSystem::cd (cwd);
      OCPI::OS::FileSystem::rmdir (ncd);
      test (!OCPI::OS::FileSystem::exists (dirName));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 10: Creating a file
   * ----------------------------------------------------------------------
   */

  class Test10 : public OCPI::Util::Test::Test {
  public:
    Test10 ()
      : OCPI::Util::Test::Test ("Creating a file")
    {
    }

    void run ()
    {
      std::string cwd = OCPI::OS::FileSystem::cwd ();
      std::string dirName = "fileSystemTestDirectory";
      test (!OCPI::OS::FileSystem::exists (dirName));
      OCPI::OS::FileSystem::mkdir (dirName);
      OCPI::OS::FileSystem::cd (dirName);
      std::string ncd = OCPI::OS::FileSystem::cwd ();
      std::string fileName = "test10File.dat";
      std::string absFileName = OCPI::OS::FileSystem::joinNames (ncd, fileName);
      std::string nativeName = OCPI::OS::FileSystem::toNativeName (absFileName);

      {
        std::ofstream file (nativeName.c_str(),
                            std::ios_base::out |
                            std::ios_base::trunc |
                            std::ios_base::binary);
        test (file.good ());
        file << "Hello World!" << std::endl;
        file.close ();
      }

      bool isDir;
      test (OCPI::OS::FileSystem::exists (fileName, &isDir));
      test (!isDir);
      test (OCPI::OS::FileSystem::exists (absFileName, &isDir));
      test (!isDir);
      unsigned long long fileSize = OCPI::OS::FileSystem::size (fileName);
      test (fileSize == 13);
      std::time_t now = time (0);
      std::time_t fileTime = OCPI::OS::FileSystem::lastModified (fileName);
      test ((now - fileTime) < 3);

      OCPI::OS::FileSystem::remove (absFileName);
      test (!OCPI::OS::FileSystem::exists (fileName));

      OCPI::OS::FileSystem::cd (cwd);
      OCPI::OS::FileSystem::rmdir (ncd);
      test (!OCPI::OS::FileSystem::exists (dirName));
    }
  };
}

static
int
testFileSystemInt (int argc, char * argv[])
{
  ( void ) argc;
  OCPI::Util::Test::Suite tests ("File System tests");
  std::string argv0 = argv[0];
  int n_failed;

  if (OCPI::OS::FileSystem::exists ("/tmp")) {
    // Go to a better place where we may have write permission.
    OCPI::OS::FileSystem::cd ("/tmp");
  }

  tests.add_test (new FileSystemTests::Test01);
  tests.add_test (new FileSystemTests::Test02);
  tests.add_test (new FileSystemTests::Test03 (argv0));
  tests.add_test (new FileSystemTests::Test04 (argv0));
  tests.add_test (new FileSystemTests::Test05 (argv0));
  tests.add_test (new FileSystemTests::Test06 (argv0));
  tests.add_test (new FileSystemTests::Test07 (argv0));
  tests.add_test (new FileSystemTests::Test08);
  tests.add_test (new FileSystemTests::Test09);
  tests.add_test (new FileSystemTests::Test10);

  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testFileSystem (int argc, char * argv[])
  {
    return testFileSystemInt (argc, argv);
  }
}

/*
 * Entrypoint for everybody else.
 */

int
main (int argc, char * argv[])
{
#if !defined (NDEBUG)
  {
    for (int i=1; i<argc; i++) {
      if (std::strcmp (argv[i], "--break") == 0) {
        OCPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testFileSystemInt (argc, argv);
}
