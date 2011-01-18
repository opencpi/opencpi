
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

/*
 * Start an SCA Generic Assembly Controller.
 *
 * Revision History:
 *
 *     04/07/2009 - Frank Pilhofer
 *                  Add signal handler for proper shutdown.
 *
 */

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <OcpiOsMisc.h>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiLoggerOStreamOutput.h>
#include <OcpiStringifyCorbaException.h>
#include <OcpiCORBAUtilNameServiceBind.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <CosNaming.h>
#include <CF.h>
#include "OcpiScaGenericAssemblyController.h"

class SgacCommandLineConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  SgacCommandLineConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
  unsigned long debugLevel;
#endif
  std::string identifier;

  bool writeIORFile;
  std::string iorFileName;
  bool registerWithNamingService;
  std::string namingServiceName;
  std::string logFile;

private:
  static CommandLineConfiguration::Option g_options[];
};

SgacCommandLineConfigurator::
SgacCommandLineConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
    debugLevel (0),
#endif
    writeIORFile (false),
    registerWithNamingService (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
SgacCommandLineConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "identifier", "Identifier (for logging)",
    OCPI_CLC_OPT(&SgacCommandLineConfigurator::identifier), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "writeIORFile", "Write IOR to file",
    OCPI_CLC_OPT(&SgacCommandLineConfigurator::iorFileName),
    OCPI_CLC_SENT(&SgacCommandLineConfigurator::writeIORFile) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "registerWithNamingService", "Register with Naming Service",
    OCPI_CLC_OPT(&SgacCommandLineConfigurator::namingServiceName),
    OCPI_CLC_SENT(&SgacCommandLineConfigurator::registerWithNamingService) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "logFile", "Write log messages to file (- for stdout)",
    OCPI_CLC_OPT(&SgacCommandLineConfigurator::logFile), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&SgacCommandLineConfigurator::debugBreak), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "debugLevel", "Debugging level",
    OCPI_CLC_OPT(&SgacCommandLineConfigurator::debugLevel), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&SgacCommandLineConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (SgacCommandLineConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * Ctrl-C Handler and other helpers.
 */

namespace {
  CORBA::ORB_ptr g_sgacOrb = CORBA::ORB::_nil ();

  void
  sgacSignalHandler ()
  {
    if (!CORBA::is_nil (g_sgacOrb)) {
      try {
        g_sgacOrb->shutdown (0);
      }
      catch (...) {
      }

      g_sgacOrb = CORBA::ORB::_nil ();
    }
  }
}

/*
 * Start the ScaGenericAssemblyController when started from the command line.
 */

static
CF::Resource_ptr
startGenericAssemblyControllerCmdInt (CORBA::ORB_ptr orb,
                                      int & argc, char * argv[],
                                      bool shutdownOrbOnRelease)
{
  SgacCommandLineConfigurator config;
  CF::Resource_var res;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cout << "Oops: " << oops << std::endl;
    return CF::Resource::_nil ();
  }

  if (config.help) {
    printUsage (config, argv[0]);
    return CF::Resource::_nil ();
  }

  std::string applicationName;
  std::string::size_type colon = config.identifier.find (':');

  if (colon != std::string::npos) {
    applicationName = config.identifier.substr (colon + 1);
  }
  else {
    applicationName = config.identifier;
  }

  try {
    CORBA::Object_var obj = orb->resolve_initial_references ("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow (obj);

#if !defined (NDEBUG)
    OCPI::Logger::debug (applicationName, config.debugLevel);
#endif

    OCPI::Logger::Logger * logger = 0;

    if (config.logFile == "-") {
      logger = new OCPI::Logger::OStreamOutput (std::cout);
    }
    else if (config.logFile.length()) {
      std::ofstream * of = new std::ofstream (config.logFile.c_str());

      if (!of->good()) {
        std::cout << "Oops: can not open log file \""
                  << config.logFile
                  << "\" for writing."
                  << std::endl;
        delete of;
        return CF::Resource::_nil ();
      }

      logger = new OCPI::Logger::OStreamOutput (of, true);
    }

    /*
     * Activate the POA Manager.
     */

    PortableServer::POAManager_var mgr = poa->the_POAManager ();
    mgr->activate ();

    /*
     * Activate server.
     */

    OCPI::SCA::GenericAssemblyController * sgac =
      new OCPI::SCA::GenericAssemblyController (orb, poa,
                                               config.identifier,
                                               logger,
                                               true,
                                               shutdownOrbOnRelease);

    res = sgac->_this ();
    sgac->_remove_ref ();

    /*
     * Write object reference.
     */

    if (config.writeIORFile) {
      CORBA::String_var ior = orb->object_to_string (res);

      if (config.iorFileName == "-") {
        std::cout << ior << std::endl;
      }
      else {
        std::cout << "Writing IOR to \""
                  << config.iorFileName
                  << "\" ... "
                  << std::flush;
        std::ofstream out (config.iorFileName.c_str());
        out << ior << std::endl;

        if (out.good()) {
          std::cout << "done." << std::endl;
        }
        else {
          std::cout << "failed." << std::endl;
        }
      }
    }

    /*
     * Register with Naming Service.
     */

    if (config.registerWithNamingService) {
      std::cout << "Registering with Naming Service as \""
                << config.namingServiceName
                << "\" ... " << std::flush;

      try {
        CORBA::Object_var nso = orb->resolve_initial_references ("NameService");
        CosNaming::NamingContextExt_var ns = CosNaming::NamingContextExt::_narrow (nso);
        OCPI::CORBAUtil::Misc::nameServiceBind (ns, res, config.namingServiceName);
        std::cout << "done." << std::endl;
      }
      catch (const CORBA::Exception & ex) {
        std::cout << "failed: "
                  << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                  << std::endl;
      }
      catch (const std::string & oops) {
        std::cout << "failed: " << oops << std::endl;
      }
    }
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return CF::Resource::_nil ();
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
    return CF::Resource::_nil ();
  }

  return res._retn ();
}

/*
 * Start the Generic Assembly Controller when started by SCA.
 */

static
CF::Resource_ptr
startGenericAssemblyControllerScaInt (CORBA::ORB_ptr orb,
                                      int & argc, char *argv[],
                                      bool shutdownOrbOnRelease)
{
  CF::Resource_var res;
  int ai;

  try {
    std::string namingContextIor;
    std::string nameBinding;
    std::string componentIdentifier;
    std::string logFile;
    int debugLevel = 0;

    for (ai=1; ai<argc; ai++) {
      if (std::strcmp (argv[ai], "NAMING_CONTEXT_IOR") == 0) {
        namingContextIor = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "NAME_BINDING") == 0) {
        nameBinding = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "COMPONENT_IDENTIFIER") == 0) {
        componentIdentifier = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "COMPONENT_NAME") == 0) {
        /* Passed by SCARI++ 2.2.  Ignore. */
        ai++;
      }
      else if (std::strcmp (argv[ai], "debugLevel") == 0) {
        debugLevel = atoi (argv[++ai]);
      }
      else if (std::strcmp (argv[ai], "logFile") == 0) {
        logFile = argv[++ai];
      }
      else {
        std::string oops = "Invalid command-line option: \"";
        oops += argv[ai];
        oops += "\"";
        throw oops;
      }
    }

    std::string applicationName;
    std::string::size_type colon = componentIdentifier.find (':');

    if (colon != std::string::npos) {
      applicationName = componentIdentifier.substr (colon + 1);
    }
    else {
      applicationName = componentIdentifier;
    }

    CORBA::Object_var obj = orb->resolve_initial_references ("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow (obj);
    PortableServer::POAManager_var mgr = poa->the_POAManager ();
    mgr->activate ();

    /*
     * Set debug level and initialize logger.
     */

    OCPI::Logger::debug (applicationName, debugLevel);
    OCPI::Logger::Logger * logger = 0;

    if (logFile == "-") {
      logger = new OCPI::Logger::OStreamOutput (std::cout);
    }
    else if (logFile.length()) {
      std::ofstream * of = new std::ofstream (logFile.c_str());

      if (!of->good()) {
        std::cout << "Oops: can not open log file \""
                  << logFile
                  << "\" for writing."
                  << std::endl;
        delete of;
        return CF::Resource::_nil ();
      }

      logger = new OCPI::Logger::OStreamOutput (of, true);
      logger->setProducerId (componentIdentifier.c_str());
    }

    if (logger) {
      OCPI::Logger::DebugLogger debug (*logger);

      debug << OCPI::Logger::ProducerName (applicationName)
            << "Generic Assembly Controller for application "
            << applicationName
            << " starting."
            << std::flush;

      debug << OCPI::Logger::ProducerName (applicationName)
            << OCPI::Logger::Verbosity (2)
            << argc << " command-line options:"
            << std::flush;

      for (ai=0; ai<argc; ai++) {
        debug << OCPI::Logger::ProducerName (applicationName)
              << OCPI::Logger::Verbosity (2)
              << "argv[" << ai << "] = \""
              << argv[ai]
              << "\""
              << std::flush;
      }
    }

    /*
     * Activate server.
     */

    OCPI::SCA::GenericAssemblyController * sgac =
      new OCPI::SCA::GenericAssemblyController (orb, poa,
                                               componentIdentifier,
                                               logger,
                                               true,
                                               shutdownOrbOnRelease);

    PortableServer::ObjectId_var oid = poa->activate_object (sgac);
    CORBA::Object_var so = poa->id_to_reference (oid);
    res = CF::Resource::_narrow (so);

    /*
     * Register with the Naming Service.
     */

    CosNaming::Name nn;
    nn.length (1);
    nn[0].id = nameBinding.c_str ();

    try {
      CORBA::Object_var ncref = orb->string_to_object (namingContextIor.c_str());
      CosNaming::NamingContext_var nc = CosNaming::NamingContext::_narrow (ncref);
      nc->bind (nn, res);
    }
    catch (const CORBA::Exception & ex) {
      std::string msg = "Failed to register Generic Assembly Controller in Naming Service: ";
      msg += OCPI::CORBAUtil::Misc::stringifyCorbaException (ex);

      *logger << OCPI::Logger::Level::EXCEPTION_ERROR
              << OCPI::Logger::ProducerName (applicationName)
              << msg
              << "."
              << std::flush;

      /*
       * To do: undo all of the above and throw an exception.
       */
    }

    /*
     * Done.
     */

    sgac->_remove_ref ();
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return CF::Resource::_nil ();
  }
  catch (const std::string & ex) {
    std::cout << "Oops: " << ex << std::endl;
    return CF::Resource::_nil ();
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
    return CF::Resource::_nil ();
  }

  return res._retn ();
}

extern "C" {
  /*
   * Entrypoint for the SCA on VxWorks.
   */

  int
  startGenericAssemblyControllerSca (int argc, char *argv[])
  {
    CORBA::ORB_var orb;
    CF::Resource_var res;

    try {
      orb = CORBA::ORB_init (argc, argv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: ORB_init: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }
    catch (...) {
      std::cout << "Oops: ORB_init failed." << std::endl;
      return -1;
    }

    res = startGenericAssemblyControllerScaInt (orb, argc, argv, false);

    if (CORBA::is_nil (res)) {
      return -1;
    }

    return 0;
  }

  /*
   * Entrypoint for the VxWorks command line.
   */

  int
  startGenericAssemblyController (int argc, char *argv[])
  {
    CORBA::ORB_var orb;
    CF::Resource_var res;

    try {
      orb = CORBA::ORB_init (argc, argv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: ORB_init: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }
    catch (...) {
      std::cout << "Oops: ORB_init failed." << std::endl;
      return -1;
    }

    res = startGenericAssemblyControllerCmdInt (orb, argc, argv, false);

    if (CORBA::is_nil (res)) {
      return -1;
    }

    std::cout << "Generic Assembly Controller is running." << std::endl;

    return 0;
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

  CORBA::ORB_var orb;

  try {
    orb = CORBA::ORB_init (argc, argv);
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: ORB_init: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return 1;
  }
  catch (...) {
    std::cout << "Oops: ORB_init failed." << std::endl;
    return 1;
  }

  /*
   * Install signal handler.
   */

  g_sgacOrb = orb;
  OCPI::OS::setCtrlCHandler (sgacSignalHandler);

  /*
   * Try to determine if we are being started from the command line or
   * by the SCA.  We make that decision depending on whether there are
   * any command-line parameters that start with a dash.
   */

  bool isSca = (argc > 2);

  for (int i=1; i<argc; i++) {
    if (*argv[i] == '-' && argv[i][1]) {
      isSca = false;
    }
  }

  /*
   * If we are being started by the SCA, call our SCA entrypoint.
   */

  CF::Resource_var res;

  if (isSca) {
    res = startGenericAssemblyControllerScaInt (orb, argc, argv, true);
  }
  else {
    res = startGenericAssemblyControllerCmdInt (orb, argc, argv, true);
  }

  if (CORBA::is_nil (res)) {
    return 1;
  }

  /*
   * Run.
   */

  try {
    std::cout << "Generic Assembly Controller running." << std::endl;
    orb->run ();
    std::cout << "Generic Assembly Controller shutting down." << std::endl;

    try {
      res->releaseObject ();
    }
    catch (...) {
    }

    try {
      orb->shutdown (1);
    }
    catch (const CORBA::BAD_INV_ORDER &) {
      // this is expected in a synchronous server
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
    }

    try {
      orb->destroy ();
    }
    catch (...) {
    }
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
  }

  return 0;
}

