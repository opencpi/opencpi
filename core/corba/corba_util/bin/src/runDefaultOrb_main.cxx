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
#include <corba.h>
#include <CpiOsThreadManager.h>
#include "CpiStringifyCorbaException.h"

/*
 * These functions are for use from the VxWorks console.
 */

extern "C" {

  static
  void
  runOrbRun (void * theorb)
  {
    CORBA::ORB_ptr orb = static_cast<CORBA::ORB_ptr> (theorb);

    try {
      orb->run ();
    }
    catch (const CORBA::Exception & ex) {
      std::cerr << "Running the ORB failed: "
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
    }
    catch (...) {
      std::cerr << "Running the ORB failed." << std::endl;
    }
  }

  int
  CpiCORBAUtilRunOrbTask (int argc, char * argv[])
  {
    char * myargv[2];
    char ** theargv;
    int theargc;

    if (argc && argv) {
      theargc = argc;
      theargv = argv;
    }
    else {
      theargc = 2;
      theargv = myargv;
      myargv[0] = const_cast<char *> ("dummy");
      myargv[1] = 0;
    }

    CORBA::ORB_ptr orb;

    try {
      orb = CORBA::ORB_init (theargc, theargv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "ORB initialization failed: "
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return 0;
    }
    catch (...) {
      std::cout << "ORB initialization failed." << std::endl;
      return 0;
    }

    runOrbRun (orb);
    std::cout << "ORB done." << std::endl;
    return 0;
  }

  CORBA::ORB_ptr
  CpiCORBAUtilRunOrb (int argc, char * argv[])
  {
    char * myargv[2];
    char ** theargv;
    int theargc;

    if (argc && argv) {
      theargc = argc;
      theargv = argv;
    }
    else {
      theargc = 2;
      theargv = myargv;
      myargv[0] = const_cast<char *> ("dummy");
      myargv[1] = 0;
    }

    CORBA::ORB_ptr orb;

    try {
      orb = CORBA::ORB_init (theargc, theargv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "ORB initialization failed: "
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return 0;
    }
    catch (...) {
      std::cout << "ORB initialization failed." << std::endl;
      return 0;
    }

    try {
      CPI::OS::ThreadManager ror (runOrbRun,
                                  static_cast<void *> (orb));
      ror.detach ();
    }
    catch (const std::string & ex) {
      std::cout << "Failed to create ORB thread: " << ex << std::endl;
      CORBA::release (orb);
      return 0;
    }

    return orb;
  }

  int
  CpiCORBAUtilShutdownOrb (CORBA::ORB_ptr orb, int waitForCompletion)
  {
    if (CORBA::is_nil (orb)) {
      std::cout << "Nil ORB.";
      return -1;
    }

    try {
      orb->shutdown (waitForCompletion);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "ORB shutdown failed: "
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }

    return 0;
  }

  int
  CpiCORBAUtilDestroyOrb (CORBA::ORB_ptr orb)
  {
    if (CORBA::is_nil (orb)) {
      std::cout << "Nil ORB.";
      return -1;
    }

    try {
      orb->destroy ();
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "ORB destruction failed: "
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }

    CORBA::release (orb);
    return 0;
  }
}

/*
 * Dummy main() to please the compiler on platforms other than VxWorks.
 */

int
main (int, char *[])
{
  std::cout << "No-op." << std::endl;
  return 0;
}

