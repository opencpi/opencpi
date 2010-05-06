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

#include <CpiOsDebug.h>
#include <CpiOsProcessManager.h>
#include <CpiOsFileSystem.h>
#include <CpiOsMisc.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <csetjmp>
#include <ctime>
#include <signal.h>
#include "CpiUtilTest.h"

namespace {
  char i2sDigits[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };

  std::string
  unsignedToString (unsigned int value,
                    unsigned int base = 10,
                    unsigned int mindigits = 0,
                    char pad = '0')
  {
    unsigned int tmp;
    unsigned int count=1;
    unsigned int padLen;

    if (base < 2 || base > 16) {
      throw std::string ("invalid base");
    }

    /*
     * Determine length of number.
     */

    for (tmp=value; tmp>=base; count++) {
      tmp /= base;
    }

    /*
     * Initialize string.
     */

    std::string result;

    if (mindigits > count) {
      padLen = mindigits - count;
      result.resize (mindigits, pad);
    }
    else {
      padLen = 0;
      result.resize (count);
    }

    /*
     * Stringify number.
     */

    while (value >= base) {
      result[--count+padLen] = i2sDigits[value%base];
      value /= base;
    }

    // assert (count == 1 && value < base)

    result[--count+padLen] = i2sDigits[value];
    return result;
  }

  unsigned int
  stringToUnsigned (const std::string & str,
                    unsigned int base = 10)
  {
    const char * txtPtr = str.c_str ();
    unsigned long int value;
    char * endPtr;

    value = std::strtoul (txtPtr, &endPtr, base);

    if ((value == 0 && endPtr == txtPtr) || *endPtr) {
      throw std::string ("not an unsigned integer");
    }

    unsigned int res = static_cast<unsigned int> (value);

    if (static_cast<unsigned long int> (res) != value) {
      throw std::string ("unsigned integer out of range");
    }

    return res;
  }
}

namespace ProcessManagerTests {

  /*
   * ----------------------------------------------------------------------
   * Test 1: Starting and waiting for a process
   * ----------------------------------------------------------------------
   */

  class Test01 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test01 (const std::string & argv0)
      : CPI::Util::Test::Test ("Starting and waiting for a process"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      CPI::OS::ProcessManager::ParameterList p;
      p.push_back ("1");
      CPI::OS::ProcessManager pm (m_argv0, p);
      pm.wait ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Check exit code
   * ----------------------------------------------------------------------
   */

  class Test02 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test02 (const std::string & argv0)
      : CPI::Util::Test::Test ("Check exit code"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      CPI::OS::ProcessManager::ParameterList p;
      p.push_back ("2");
      CPI::OS::ProcessManager pm (m_argv0, p);
      pm.wait ();
      int ec = pm.getExitCode ();
      test (ec == 42);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Shutdown process
   * ----------------------------------------------------------------------
   */

  class Test03 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test03 (const std::string & argv0)
      : CPI::Util::Test::Test ("Shutdown a process"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      CPI::OS::ProcessManager::ParameterList p;
      p.push_back ("3");
      CPI::OS::ProcessManager pm (m_argv0, p);

      pm.wait (1000);
      pm.shutdown ();
      pm.wait ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 4: Run multiple processes
   * ----------------------------------------------------------------------
   */

  class Test04 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test04 (const std::string & argv0)
      : CPI::Util::Test::Test ("Run multiple processes"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      CPI::OS::ProcessManager::ParameterList p;
      p.push_back ("4");
      CPI::OS::ProcessManager pm1 (m_argv0, p);
      CPI::OS::ProcessManager pm2 (m_argv0, p);
      pm1.detach ();
      CPI::OS::ProcessManager pm3 (m_argv0, p);
      pm3.wait ();
      pm2.wait ();

      /*
       * The process returns time() as its exit code.  That should tell us that
       * the above actually happened in parallel (their exit codes are the same
       * or at least very similar) rather than sequentially (their exit codes
       * are 10 or more apart).
       */

      unsigned int ec2 = static_cast<unsigned int> (pm2.getExitCode ());
      unsigned int ec3 = static_cast<unsigned int> (pm3.getExitCode ());
      test (((ec3 - ec2) % 128) < 2);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 5: Command-line parameters
   * ----------------------------------------------------------------------
   */

  const char * test5CommandLine1[] = { "simple", 0 };
  const char * test5CommandLine2[] = { "par1", "par2", "par3", "par4", 0 };
  const char * test5CommandLine3[] = { "parameter with spaces", 0 };
  const char * test5CommandLine4[] = { "parameter with \"quotes\"", 0 };
  const char * test5CommandLine5[] = { "with\\some\\backslashes", 0 };
  const char * test5CommandLine6[] = { "backslash before a \\\"quote\\\"", 0 };
  const char * test5CommandLine7[] = { "\\backslashes at both ends\\", 0 };

  const char * const * test5CommandLines[] = {
    test5CommandLine1,
    test5CommandLine2,
    test5CommandLine3,
    test5CommandLine4,
    test5CommandLine5,
    test5CommandLine6,
    test5CommandLine7,
  };

  class Test05 : public CPI::Util::Test::Test {
  private:
    std::string m_argv0;

  public:
    Test05 (const std::string & argv0)
      : CPI::Util::Test::Test ("Command-line parameters"),
        m_argv0 (argv0)
    {
    }

    void run ()
    {
      unsigned int numTests = sizeof (test5CommandLines) / sizeof (char **);

      for (unsigned int testIdx=0; testIdx<numTests; testIdx++) {
        const char * const * commandLine = test5CommandLines[testIdx];
        unsigned int testNo = testIdx + 1;

        CPI::OS::ProcessManager::ParameterList p;
        p.push_back ("5");
        p.push_back (unsignedToString (testNo));

        for (unsigned int pi=0; commandLine[pi]; pi++) {
          p.push_back (commandLine[pi]);
        }

        CPI::OS::ProcessManager pm (m_argv0, p);
        pm.wait ();

        int ec = pm.getExitCode ();
        test (static_cast<unsigned int> (ec) == testNo);
      }
    }
  };

}

static
int
testProcessManagerInt (int argc, char * argv[])
{
  if (argc == 1) {
    CPI::Util::Test::Suite tests ("Process Manager tests");
    std::string argv0 = argv[0];
    int n_failed;

    tests.add_test (new ProcessManagerTests::Test01 (argv0));
    tests.add_test (new ProcessManagerTests::Test02 (argv0));
    tests.add_test (new ProcessManagerTests::Test03 (argv0));
    tests.add_test (new ProcessManagerTests::Test04 (argv0));
    tests.add_test (new ProcessManagerTests::Test05 (argv0));

    tests.run ();
    n_failed = tests.report ();
    return n_failed;
  }
  else if (argc >= 2) {
    unsigned int testNum;

    try {
      testNum = stringToUnsigned (argv[1]);
    }
    catch (const std::string & oops) {
      std::cout << "oops: " << oops << std::endl;
      return 0;
    }

    if (testNum == 1) {
      // dummy test
    }
    else if (testNum == 2) {
      // test exit code
      return 42;
    }
    else if (testNum == 3) {
      // waiting for shutdown
      for (int i=0; i<1000; i++) {
        CPI::OS::sleep (1000);
      }
    }
    else if (testNum == 4) {
      // run multiple processes
      CPI::OS::sleep (10000);
      return (std::time (0) % 128);
    }
    else if (testNum == 5) {
      unsigned int numTests = sizeof (ProcessManagerTests::test5CommandLines) / sizeof (char **);
      unsigned int testNo;

      if (argc < 3) {
        return 0;
      }

      try {
        testNo = stringToUnsigned (argv[2]);
      }
      catch (const std::string &) {
        return 0;
      }

      if (testNo > numTests) {
        return 0;
      }

      unsigned int testIdx = testNo - 1;
      const char * const * commandLine = ProcessManagerTests::test5CommandLines[testIdx];

      int argci = 3;

      for (unsigned int pi=0; commandLine[pi]; pi++, argci++) {
        if (argci >= argc) {
          return 0;
        }
        else if (std::strcmp (commandLine[pi], argv[argci]) != 0) {
          return 0;
        }
      }

      if (argci != argc) {
        return 0;
      }

      return static_cast<int> (testNo);
    }
    else {
      std::cout << "oops" << std::endl;
    }
  }
  else {
    std::cout << "oops" << std::endl;
  }

  return 0;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testProcessManager (int argc, char * argv[])
  {
    return testProcessManagerInt (argc, argv);
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

  return testProcessManagerInt (argc, argv);
}
