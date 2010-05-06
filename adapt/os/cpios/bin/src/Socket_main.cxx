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
#include <CpiOsSocket.h>
#include <CpiOsClientSocket.h>
#include <CpiOsServerSocket.h>
#include <CpiOsThreadManager.h>
#include <CpiOsMisc.h>
#include <cstring>
#include <ctime>
#include "CpiUtilTest.h"

namespace SocketTests {

  /*
   * ----------------------------------------------------------------------
   * Test 01: Binding ServerSocket to random port
   * ----------------------------------------------------------------------
   */

  class Test01 : public CPI::Util::Test::Test {
  public:
    Test01 ()
      : CPI::Util::Test::Test ("Binding ServerSocket to random port")
    {
    }

    void run ()
    {
      CPI::OS::ServerSocket so (0);
      unsigned int portNo = so.getPortNo ();
      so.close ();
      test (portNo > 0 && portNo < 65536);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 02: Binding ServerSocket to specific port
   * ----------------------------------------------------------------------
   */

  class Test02 : public CPI::Util::Test::Test {
  public:
    Test02 ()
      : CPI::Util::Test::Test ("Binding ServerSocket to specific port")
    {
    }

    void run ()
    {
      CPI::OS::ServerSocket so (6274, true);
      unsigned int portNo = so.getPortNo ();
      so.close ();
      test (portNo == 6274);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 03: Accepting connection
   * ----------------------------------------------------------------------
   */

  void
  test03Thread (void * opaque)
  {
    unsigned int * portNoPtr = reinterpret_cast<unsigned int *> (opaque);
    CPI::OS::Socket cs =
      CPI::OS::ClientSocket::connect (CPI::OS::getHostname(), *portNoPtr);
    cs.close ();
  }

  class Test03 : public CPI::Util::Test::Test {
  public:
    Test03 ()
      : CPI::Util::Test::Test ("Accepting connection")
    {
    }

    void run ()
    {
      unsigned int portNo = 6275;
      CPI::OS::ServerSocket se (portNo, true);
      CPI::OS::ThreadManager tm (test03Thread, &portNo);
      CPI::OS::Socket so = se.accept ();
      so.close ();
      se.close ();
      tm.join ();
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 04: Receiving data
   * ----------------------------------------------------------------------
   */

  void
  test04Thread (void * opaque)
  {
    unsigned int * portNoPtr = reinterpret_cast<unsigned int *> (opaque);
    CPI::OS::Socket cs =
      CPI::OS::ClientSocket::connect (CPI::OS::getHostname(), *portNoPtr);
    cs.send ("Hello World!", 13);
    cs.close ();
  }

  class Test04 : public CPI::Util::Test::Test {
  public:
    Test04 ()
      : CPI::Util::Test::Test ("Receiving data")
    {
    }

    void run ()
    {
      unsigned int portNo = 6276;
      CPI::OS::ServerSocket se (portNo, true);
      CPI::OS::ThreadManager tm (test04Thread, &portNo);
      CPI::OS::Socket so = se.accept ();
      char buf[16];
      unsigned long long count = so.recv (buf, 16);
      test (count == 13);
      test (std::strcmp (buf, "Hello World!") == 0);
      so.close ();
      se.close ();
      tm.join ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 05: Shutting down one side of a connection
   * ----------------------------------------------------------------------
   */

  void
  test05Thread (void * opaque)
  {
    unsigned int * portNoPtr = reinterpret_cast<unsigned int *> (opaque);
    CPI::OS::Socket cs =
      CPI::OS::ClientSocket::connect (CPI::OS::getHostname(), *portNoPtr);

    char buffer[1024], *ptr = buffer;
    unsigned long long count, total = 0;

    do {
      count = cs.recv (ptr, 1024-total);
      ptr += count;
      total += count;
    }
    while (count > 0);

    ptr = buffer;

    do {
      count = cs.send (ptr, total);
      ptr += count;
      total -= count;
    }
    while (count > 0 && total > 0);
    
    cs.close ();
  }

  class Test05 : public CPI::Util::Test::Test {
  public:
    Test05 ()
      : CPI::Util::Test::Test ("Shutting down one side of a connection")
    {
    }

    void run ()
    {
      unsigned int portNo = 6277;
      CPI::OS::ServerSocket se (portNo, true);
      CPI::OS::ThreadManager tm (test05Thread, &portNo);
      CPI::OS::Socket so = se.accept ();

      const char * ptr = "Hello World!";
      unsigned long long count;

      while (*ptr) {
        count = so.send (ptr, 1);
        test (count == 1);
        ptr++;
      }

      so.shutdown ();

      char buf[16];
      char *ptr2 = buf;

      do {
        count = so.recv (ptr2, 2);
        ptr2 += count;
      }
      while (count > 0);
      *ptr2 = 0;

      test (std::strcmp (buf, "Hello World!") == 0);
      so.close ();
      se.close ();
      tm.join ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 06: Wake up a thread that is blocked in accept
   * ----------------------------------------------------------------------
   */

  struct Test06Data {
    CPI::OS::ServerSocket * sock;
    int timeWaited;
    bool gotException;
  };

  void
  test06Thread (void * opaque)
  {
    Test06Data * td = reinterpret_cast<Test06Data *> (opaque);
    std::time_t beginning = std::time (0);

    try {
      td->sock->wait (10000);
    }
    catch (...) {
      td->gotException = true;
    }

    std::time_t end = std::time (0);
    td->timeWaited = (int) (end - beginning);
  }

  class Test06 : public CPI::Util::Test::Test {
  public:
    Test06 ()
      : CPI::Util::Test::Test ("Wake up a thread that is blocked in accept")
    {
    }

    void run ()
    {
      CPI::OS::ServerSocket se (0);
      Test06Data td;
      td.sock = &se;
      td.timeWaited = -1;
      td.gotException = false;
      CPI::OS::ThreadManager tm (test06Thread, &td);
      CPI::OS::sleep (500);
      se.close ();
      tm.join ();
      test (td.timeWaited != -1);
      test (td.timeWaited < 5);
      test (td.gotException);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 07: Receiving data, using dotted IP address to connect
   * ----------------------------------------------------------------------
   */

  void
  test07Thread (void * opaque)
  {
    std::string myAddress = CPI::OS::getIPAddress ();
    const char * ap = myAddress.c_str ();

    while (*ap) {
      if (*ap != '.' &&
          *ap != '0' && *ap != '1' && *ap != '2' && *ap != '3' &&
          *ap != '4' && *ap != '5' && *ap != '6' && *ap != '7' &&
          *ap != '8' && *ap != '9') {
        break;
      }
      ap++;
    }

    unsigned int * portNoPtr = reinterpret_cast<unsigned int *> (opaque);
    CPI::OS::Socket cs =
      CPI::OS::ClientSocket::connect (myAddress, *portNoPtr);

    if (!*ap) {
      cs.send ("Hello World!", 13);
    }
    else {
      // oops, the address was not in dotted IP notation
      cs.send ("WrongAddress", 13);
    }

    cs.close ();
  }

  class Test07 : public CPI::Util::Test::Test {
  public:
    Test07 ()
      : CPI::Util::Test::Test ("Use dotted decimal IP address")
    {
    }

    void run ()
    {
      unsigned int portNo = 6276;
      CPI::OS::ServerSocket se (portNo, true);
      CPI::OS::ThreadManager tm (test07Thread, &portNo);
      CPI::OS::Socket so = se.accept ();
      char buf[16];
      unsigned long long count = so.recv (buf, 16);
      test (count == 13);
      test (std::strcmp (buf, "Hello World!") == 0);
      so.close ();
      se.close ();
      tm.join ();
    }
  };

}

static
int
testSocketInt (int, char *[])
{
  CPI::Util::Test::Suite tests ("Socket tests");
  int n_failed;
  tests.add_test (new SocketTests::Test01);
  tests.add_test (new SocketTests::Test02);
  tests.add_test (new SocketTests::Test03);
  tests.add_test (new SocketTests::Test04);
  tests.add_test (new SocketTests::Test05);
  tests.add_test (new SocketTests::Test06);
  tests.add_test (new SocketTests::Test07);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testSocket (int argc, char * argv[])
  {
    return testSocketInt (argc, argv);
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

  return testSocketInt (argc, argv);
}
