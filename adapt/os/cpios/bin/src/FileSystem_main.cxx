// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cassert>
#include <CpiOsDebug.h>
#include <CpiOsFileSystem.h>
#include "CpiUtilTest.h"

namespace FileSystemTests {

  /*
   * ----------------------------------------------------------------------
   * Test 01: Precondition: delete test directory
   * ----------------------------------------------------------------------
   */

  void
  removeAllFiles ()
  {
    CPI::OS::FileIterator it = CPI::OS::FileSystem::list ();
    while (!it.end()) {
      if (it.isDirectory()) {
        std::string cwd = CPI::OS::FileSystem::cwd ();
        // sanity check
        std::string name = it.relativeName ();
        assert (name != "." && name != "..");
        CPI::OS::FileSystem::cd (name);
        removeAllFiles ();
        CPI::OS::FileSystem::cd (cwd);
      }
      else {
        CPI::OS::FileSystem::remove (it.relativeName());
      }
      it.next ();
    }
  }

  class Test01 : public CPI::Util::Test::Test {
  public:
    Test01 ()
      : CPI::Util::Test::Test ("Delete test directory, if it exists")
    {
    }

    void run ()
    {
      std::string cwd = CPI::OS::FileSystem::cwd ();
      std::string dirName = "fileSystemTestDirectory";
      std::string absDirName = CPI::OS::FileSystem::joinNames (cwd, dirName);
      if (CPI::OS::FileSystem::exists (dirName)) {
        CPI::OS::FileSystem::cd (dirName);
        test (CPI::OS::FileSystem::cwd() == absDirName);
        removeAllFiles ();
        CPI::OS::FileSystem::cd (cwd);
        CPI::OS::FileSystem::rmdir (absDirName);
      }
     test (!CPI::OS::FileSystem::exists (dirName));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 02: Changing to current working directory
   * ----------------------------------------------------------------------
   */

  class Test02 : public CPI::Util::Test::Test {
  public:
    Test02 ()
      : CPI::Util::Test::Test ("Changing to current working directory")
    {
    }

    void run ()
    {
      std::string cwd = CPI::OS::FileSystem::cwd ();
      CPI::OS::FileSystem::cd (cwd);
      test (CPI::OS::FileSystem::cwd() == cwd);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 03: Checking that argv0 exists
   * ----------------------------------------------------------------------
   */

  class Test03 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test03 (const std::string & argv0)
      : CPI::Util::Test::Test ("Checking that argv0 exists"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = CPI::OS::FileSystem::fromNativeName (m_argv0);
      bool isDir;
      test (CPI::OS::FileSystem::exists (nArgv0, &isDir));
      test (!isDir);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 04: Checking absolute name of argv0
   * ----------------------------------------------------------------------
   */

  class Test04 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test04 (const std::string & argv0)
      : CPI::Util::Test::Test ("Checking absolute name of argv0"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = CPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = CPI::OS::FileSystem::absoluteName (nArgv0);
      test (CPI::OS::FileSystem::exists (absArgv0));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 05: Joining names
   * ----------------------------------------------------------------------
   */

  class Test05 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test05 (const std::string & argv0)
      : CPI::Util::Test::Test ("Joining names"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = CPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = CPI::OS::FileSystem::absoluteName (nArgv0);
      std::string argv0Dir = CPI::OS::FileSystem::directoryName (absArgv0);
      std::string argv0Rel = CPI::OS::FileSystem::relativeName (absArgv0);
      std::string joinedName = CPI::OS::FileSystem::joinNames (argv0Dir, argv0Rel);
      test (absArgv0 == joinedName);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 06: Joining absolute names
   * ----------------------------------------------------------------------
   */

  class Test06 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test06 (const std::string & argv0)
      : CPI::Util::Test::Test ("Joining absolute names"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string cwd = CPI::OS::FileSystem::cwd ();
      std::string nArgv0 = CPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = CPI::OS::FileSystem::absoluteName (nArgv0);
      std::string joinedName = CPI::OS::FileSystem::joinNames (cwd, absArgv0);
      test (absArgv0 == joinedName);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 07: Checking directory listing for argv0
   * ----------------------------------------------------------------------
   */

  class Test07 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test07 (const std::string & argv0)
      : CPI::Util::Test::Test ("Checking directory listing for argv0"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      std::string nArgv0 = CPI::OS::FileSystem::fromNativeName (m_argv0);
      std::string absArgv0 = CPI::OS::FileSystem::absoluteName (nArgv0);
      std::string argv0Dir = CPI::OS::FileSystem::directoryName (absArgv0);
      std::string argv0Rel = CPI::OS::FileSystem::relativeName (absArgv0);
      bool found = false;
      CPI::OS::FileIterator it = CPI::OS::FileSystem::list (argv0Dir);
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

  class Test08 : public CPI::Util::Test::Test {
  public:
    Test08 ()
      : CPI::Util::Test::Test ("Making and removing test directory")
    {
    }

    void run ()
    {
      std::string dirName = "fileSystemTestDirectory";
      test (!CPI::OS::FileSystem::exists (dirName));
      CPI::OS::FileSystem::mkdir (dirName);
      bool isDir;
      test (CPI::OS::FileSystem::exists (dirName, &isDir));
      test (isDir);
      CPI::OS::FileSystem::rmdir (dirName);
      test (!CPI::OS::FileSystem::exists (dirName));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 09: Path name of test directory
   * ----------------------------------------------------------------------
   */

  class Test09 : public CPI::Util::Test::Test {
  public:
    Test09 ()
      : CPI::Util::Test::Test ("Path name of test directory")
    {
    }

    void run ()
    {
      std::string cwd = CPI::OS::FileSystem::cwd ();
      std::string dirName = "fileSystemTestDirectory";
      test (!CPI::OS::FileSystem::exists (dirName));
      CPI::OS::FileSystem::mkdir (dirName);
      CPI::OS::FileSystem::cd (dirName);
      std::string ncd = CPI::OS::FileSystem::cwd ();
      std::string ncdDir = CPI::OS::FileSystem::directoryName (ncd);
      std::string ncdRel = CPI::OS::FileSystem::relativeName (ncd);
      test (cwd == ncdDir);
      test (ncdRel == dirName);
      CPI::OS::FileSystem::cd (cwd);
      CPI::OS::FileSystem::rmdir (ncd);
      test (!CPI::OS::FileSystem::exists (dirName));
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 10: Creating a file
   * ----------------------------------------------------------------------
   */

  class Test10 : public CPI::Util::Test::Test {
  public:
    Test10 ()
      : CPI::Util::Test::Test ("Creating a file")
    {
    }

    void run ()
    {
      std::string cwd = CPI::OS::FileSystem::cwd ();
      std::string dirName = "fileSystemTestDirectory";
      test (!CPI::OS::FileSystem::exists (dirName));
      CPI::OS::FileSystem::mkdir (dirName);
      CPI::OS::FileSystem::cd (dirName);
      std::string ncd = CPI::OS::FileSystem::cwd ();
      std::string fileName = "test10File.dat";
      std::string absFileName = CPI::OS::FileSystem::joinNames (ncd, fileName);
      std::string nativeName = CPI::OS::FileSystem::toNativeName (absFileName);

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
      test (CPI::OS::FileSystem::exists (fileName, &isDir));
      test (!isDir);
      test (CPI::OS::FileSystem::exists (absFileName, &isDir));
      test (!isDir);
      unsigned long long fileSize = CPI::OS::FileSystem::size (fileName);
      test (fileSize == 13);
      std::time_t now = time (0);
      std::time_t fileTime = CPI::OS::FileSystem::lastModified (fileName);
      test ((now - fileTime) < 3);

      CPI::OS::FileSystem::remove (absFileName);
      test (!CPI::OS::FileSystem::exists (fileName));

      CPI::OS::FileSystem::cd (cwd);
      CPI::OS::FileSystem::rmdir (ncd);
      test (!CPI::OS::FileSystem::exists (dirName));
    }
  };
}

static
int
testFileSystemInt (int argc, char * argv[])
{
  CPI::Util::Test::Suite tests ("File System tests");
  std::string argv0 = argv[0];
  int n_failed;

  if (CPI::OS::FileSystem::exists ("/tmp")) {
    // Go to a better place where we may have write permission.
    CPI::OS::FileSystem::cd ("/tmp");
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
        CPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testFileSystemInt (argc, argv);
}
