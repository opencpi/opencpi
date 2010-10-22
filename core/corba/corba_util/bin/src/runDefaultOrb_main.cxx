
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
#include <corba.h>
#include <OcpiOsThreadManager.h>
#include "OcpiStringifyCorbaException.h"

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
      std::cerr << "Running the OCPI_CORBA_ORB failed: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
    }
    catch (...) {
      std::cerr << "Running the OCPI_CORBA_ORB failed." << std::endl;
    }
  }

  int
  OcpiCORBAUtilRunOrbTask (int argc, char * argv[])
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
      std::cout << "OCPI_CORBA_ORB initialization failed: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return 0;
    }
    catch (...) {
      std::cout << "OCPI_CORBA_ORB initialization failed." << std::endl;
      return 0;
    }

    runOrbRun (orb);
    std::cout << "OCPI_CORBA_ORB done." << std::endl;
    return 0;
  }

  CORBA::ORB_ptr
  OcpiCORBAUtilRunOrb (int argc, char * argv[])
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
      std::cout << "OCPI_CORBA_ORB initialization failed: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return 0;
    }
    catch (...) {
      std::cout << "OCPI_CORBA_ORB initialization failed." << std::endl;
      return 0;
    }

    try {
      OCPI::OS::ThreadManager ror (runOrbRun,
                                  static_cast<void *> (orb));
      ror.detach ();
    }
    catch (const std::string & ex) {
      std::cout << "Failed to create OCPI_CORBA_ORB thread: " << ex << std::endl;
      CORBA::release (orb);
      return 0;
    }

    return orb;
  }

  int
  OcpiCORBAUtilShutdownOrb (CORBA::ORB_ptr orb, int waitForCompletion)
  {
    if (CORBA::is_nil (orb)) {
      std::cout << "Nil OCPI_CORBA_ORB.";
      return -1;
    }

    try {
      orb->shutdown (waitForCompletion);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "OCPI_CORBA_ORB shutdown failed: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }

    return 0;
  }

  int
  OcpiCORBAUtilDestroyOrb (CORBA::ORB_ptr orb)
  {
    if (CORBA::is_nil (orb)) {
      std::cout << "Nil OCPI_CORBA_ORB.";
      return -1;
    }

    try {
      orb->destroy ();
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "OCPI_CORBA_ORB destruction failed: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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

