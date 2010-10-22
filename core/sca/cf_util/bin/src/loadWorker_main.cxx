
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
 * ----------------------------------------------------------------------
 * Load and execute a worker on an SCA Executable Device.  Just in case
 * there is no Core Framework.
 * ----------------------------------------------------------------------
 */

#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <OcpiOsMisc.h>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiOsFileSystem.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <OcpiStringifyCorbaException.h>
#include <OcpiCORBAUtilNameServiceBind.h>
#include <OcpiCORBAUtilNameServiceWait.h>
#include <OcpiCFUtilVfsFileSystem.h>
#include <OcpiCFUtilStringifyCFException.h>
#include <CosNaming.h>
#include <CF.h>

/*
 * ----------------------------------------------------------------------
 * Command-line Options.
 * ----------------------------------------------------------------------
 */

class LoadWorkerConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  LoadWorkerConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool verbose;
  bool writeIORFile;
  bool registerWithNamingService;
  std::string identifier;
  std::string iorFileName;
  std::string namingServiceName;
  std::string loadType;
  std::string executableDevice;
  std::string entrypoint;
  MultiNameValue execParameters;
  unsigned long bindTimeout;

private:
  static CommandLineConfiguration::Option g_options[];
};

LoadWorkerConfigurator::
LoadWorkerConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    verbose (true),
    writeIORFile (false),
    registerWithNamingService (false),
    bindTimeout (60)
{
}

OCPI::Util::CommandLineConfiguration::Option
LoadWorkerConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "identifier", "Resource identifier",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::identifier), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "executableDevice", "Executable Device IOR",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::executableDevice), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "loadType", "CF::LoadableDevice::LoadType",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::loadType), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "entrypoint", "If --loadType=SHARED_LIBRARY",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::entrypoint), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "execParameters", "Executable Parameters",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::execParameters), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "bindTimeout", "Timeout for worker binding",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::bindTimeout), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "writeIORFile", "Write IOR to file",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::iorFileName),
    OCPI_CLC_SENT(&LoadWorkerConfigurator::writeIORFile) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "registerWithNamingService", "Register with Naming Service",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::namingServiceName),
    OCPI_CLC_SENT(&LoadWorkerConfigurator::registerWithNamingService) },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "v", "Verbose messages",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&LoadWorkerConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (LoadWorkerConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options] <file>" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * To Do: Implement this.
 */

/*
 * ----------------------------------------------------------------------
 * Load and execute a worker from a local file.  This involves:
 * - Creating a local FileFs.
 * - Wrapping the FileFs into an SCA CF::FileSystem.
 * - Loading the file.
 * - Starting a local Naming Service (at least one that's good enough
 *   to fool the worker).
 * - Calling execute().
 * - Waiting for the new worker to bind to the Naming Service.
 * ----------------------------------------------------------------------
 */

static
bool
loadWorkerInt (CORBA::ORB_ptr orb,
               int & argc, char * argv[])
{
  LoadWorkerConfigurator config;
  CF::ExecutableDevice::ProcessID_Type workerPid;
  CORBA::Object_var workerRef;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cout << "Oops: " << oops << std::endl;
    return false;
  }

  if (config.help || argc != 2) {
    printUsage (config, argv[0]);
    return false;
  }

  try {
    /*
     * Activate the RootPOA.  It will be used by the SCA File System
     * and the Naming Context.
     */

    CORBA::Object_var obj = orb->resolve_initial_references ("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow (obj);
    PortableServer::POAManager_var mgr = poa->the_POAManager ();
    mgr->activate ();

    /*
     * Instantiate a local FileFs that provides access to the worker file.
     */

    std::string absWorkerFileName = OCPI::OS::FileSystem::absoluteName (argv[1]);
    std::string workerDirName = OCPI::OS::FileSystem::directoryName (absWorkerFileName);
    std::string workerRelName = OCPI::OS::FileSystem::relativeName (absWorkerFileName);
    std::string workerFileNameInFileFs = "/";
    workerFileNameInFileFs += workerRelName;

    OCPI::Util::FileFs::FileFs * fileFs =
      new OCPI::Util::FileFs::FileFs (workerDirName);

    if (!fileFs->exists (workerFileNameInFileFs)) {
      delete fileFs;
      std::string msg = "File \"";
      msg += argv[1];
      msg += "\" not found";
      throw msg;
    }

    /*
     * Wrap the FileFs in an SCA File System that the Executable Device
     * can load the worker file from.
     */

    OCPI::CFUtil::VfsFileSystem * vfsFileSystem =
      new OCPI::CFUtil::VfsFileSystem (orb, poa, fileFs, true);
    PortableServer::ObjectId_var fsOid;
    CF::FileSystem_var fileSystem;

    try {
      fsOid = poa->activate_object (vfsFileSystem);

      try {
        CORBA::Object_var fsobj = poa->id_to_reference (fsOid);
        fileSystem = CF::FileSystem::_narrow (fsobj);
        ocpiAssert (!CORBA::is_nil (fileSystem));
      }
      catch (...) {
        try {
          poa->deactivate_object (fsOid);
        }
        catch (...) {
        }

        throw;
      }
    }
    catch (...) {
      vfsFileSystem->_remove_ref ();
      throw std::string ("Error activating File System");
    }

    vfsFileSystem->_remove_ref ();

    /*
     * If an exception happens below, we must deactivate the FileSystem
     * servant.
     */

    try {
      /*
       * Load the worker onto the Executable Device.
       */

      CORBA::Object_var edObj;

      try {
        edObj = orb->string_to_object (config.executableDevice.c_str());
      }
      catch (...) {
        std::string msg = "Invalid IOR for Executable Device: \"";
        msg += config.executableDevice;
        msg += "\"";
        throw msg;
      }

      CF::ExecutableDevice_var ed = CF::ExecutableDevice::_narrow (edObj);

      if (CORBA::is_nil (ed)) {
        std::string msg = "Not an Executable Device: \"";
        msg += config.executableDevice;
        msg += "\"";
        throw msg;
      }

      CF::LoadableDevice::LoadType loadType;

      if (!config.loadType.length() ||
          config.loadType == "EXECUTABLE") {
        loadType = CF::LoadableDevice::EXECUTABLE;
      }
      else if (config.loadType == "SHARED_LIBRARY") {
        loadType = CF::LoadableDevice::SHARED_LIBRARY;
      }
      else {
        std::string msg = "Invalid LoadType: \"";
        msg += config.loadType;
        msg += "\"";
        throw msg;
      }

      if (config.verbose) {
        std::cout << "Loading \"" << workerRelName << "\" ... " << std::flush;
      }

      try {
        ed->load (fileSystem,
                  workerFileNameInFileFs.c_str (),
                  loadType);
      }
      catch (const CORBA::Exception & ex) {
        if (config.verbose) {
          std::cout << OCPI::CFUtil::stringifyCFException (ex) << std::endl;
        }
        std::string msg = "Failed to load \"";
        msg += argv[1];
        msg += "\": ";
        msg += OCPI::CFUtil::stringifyCFException (ex);
        throw msg;
      }

      if (config.verbose) {
        std::cout << "done." << std::endl;
      }

      /*
       * Start a Naming Context that the worker can bind to.
       */

      std::string workerNameStr = "theWorker";
      CosNaming::Name workerName (1);
      workerName.length (1);
      workerName[0].id = workerNameStr.c_str ();

      OCPI::CORBAUtil::WaitForNameServiceBinding ncb (orb, poa, workerName);
      CosNaming::NamingContext_var nc = ncb.getContext ();
      CORBA::String_var ncior = orb->object_to_string (nc);

      /*
       * Make up a nice list of execution parameters.
       */

      CORBA::ULong numExecParams = 3 + config.execParameters.size ();
      CF::Properties execParams (numExecParams);
      execParams.length (numExecParams);

      execParams[0].id = "NAMING_CONTEXT_IOR";
      execParams[0].value <<= ncior.in();

      execParams[1].id = "NAME_BINDING";
      execParams[1].value <<= workerNameStr.c_str();

      execParams[2].id = "COMPONENT_IDENTIFIER";
      execParams[2].value <<= config.identifier.c_str();

      for (unsigned int epi=0; epi<config.execParameters.size(); epi++) {
        execParams[3 + epi].id = config.execParameters[epi].first.c_str();
        execParams[3 + epi].value <<= config.execParameters[epi].second.c_str();
      }

      /*
       * Execute the worker.
       */

      CF::Properties options;
      std::string executeWhat;

      if (config.entrypoint.length()) {
        executeWhat = config.entrypoint;
      }
      else {
        executeWhat = workerRelName;
      }

      if (config.verbose) {
        std::cout << "Executing \"" << executeWhat << "\" ... " << std::flush;
      }

      try {
        workerPid = ed->execute (executeWhat.c_str (),
                                 options,
                                 execParams);
      }
      catch (const CORBA::Exception & ex) {
        if (config.verbose) {
          std::cout << OCPI::CFUtil::stringifyCFException (ex) << std::endl;
        }

        try {
          ed->unload (workerRelName.c_str());
        }
        catch (...) {
        }

        std::string msg = "Failed to execute \"";
        msg += argv[1];
        msg += "\": ";
        msg += OCPI::CFUtil::stringifyCFException (ex);
        throw msg;
      }

      if (config.verbose) {
        std::cout << "done." << std::endl;
      }

      /*
       * Wait for the worker to bind to our Naming Context.
       */

      if (config.verbose) {
        std::cout << "Waiting for worker to bind to Naming Context ... " << std::flush;
      }

      if (!ncb.waitForBinding (config.bindTimeout)) {
        if (config.verbose) {
          std::cout << "timed out." << std::endl;
        }

        if (workerPid != static_cast<CF::ExecutableDevice::ProcessID_Type> (-1)) {
          try {
            ed->terminate (workerPid);
          }
          catch (...) {
          }
        }

        try {
          ed->unload (workerRelName.c_str());
        }
        catch (...) {
        }

        std::string msg = "Timed out waiting for the worker to bind to Naming Context";
        throw msg;
      }

      workerRef = ncb.getBinding ();

      if (config.verbose) {
        std::cout << "done." << std::endl;
      }

      /*
       * Ok, the worker is up and running.  Unwind a bit before continuing,
       * we don't need the naming context or the file system any more.
       */

      try {
        poa->deactivate_object (fsOid);
      }
      catch (...) {
      }
    }
    catch (...) {
      try {
        poa->deactivate_object (fsOid);
      }
      catch (...) {
      }

      throw;
    }

    /*
     * Sanity check: Test that the worker is of type CF::Resource.
     */

    CF::Resource_var workerRes;

    if (config.verbose) {
      std::cout << "Testing that the worker is of type CF::Resource ... " << std::flush;
    }

    try {
      workerRes = CF::Resource::_narrow (workerRef);

      if (CORBA::is_nil (workerRef)) {
        if (config.verbose) {
          std::cout << "failed: nil reference." << std::endl;
        }
        else {
          std::cerr << "Warning: worker reference is nil." << std::endl;
        }
      }
      else if (CORBA::is_nil (workerRes)) {
        if (config.verbose) {
          std::cout << "failed." << std::endl;
        }
        else {
          std::cerr << "Warning: worker is not of type \"CF::Resource\"." << std::endl;
        }
      }
      else {
        if (config.verbose) {
          std::cout << "done." << std::endl;
        }
      }
    }
    catch (const CORBA::Exception & ex) {
      if (config.verbose) {
        std::cout << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex) << std::endl;
      }
      else {
        std::cerr << "Warning: Failed to test worker for \"CF::Resource\" type: "
                  << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                  << "."
                  << std::endl;
      }

      workerRes = CF::Resource::_nil ();
    }

    /*
     * Initialize worker.
     */

    if (!CORBA::is_nil (workerRes)) {
      if (config.verbose) {
        std::cout << "Initializing worker ... " << std::endl;
      }

      try {
        workerRes->initialize ();

        if (config.verbose) {
          std::cout << "done." << std::endl;
        }
      }
      catch (const CORBA::Exception & ex) {
        if (config.verbose) {
          std::cout << OCPI::CFUtil::stringifyCFException (ex) << std::endl;
        }
        else {
          std::cerr << "Warning: Failed to initialize worker: "
                    << OCPI::CFUtil::stringifyCFException (ex)
                    << "."
                    << std::endl;
        }
      }
    }

    /*
     * Write object reference.
     */

    if (config.writeIORFile) {
      CORBA::String_var ior = orb->object_to_string (workerRef);

      if (config.iorFileName == "-") {
        std::cout << ior << std::endl;
      }
      else {
        if (config.verbose) {
          std::cout << "Writing worker IOR to \""
                    << config.iorFileName
                    << "\" ... "
                    << std::flush;
        }

        std::ofstream out (config.iorFileName.c_str());
        out << ior << std::endl;

        if (out.good()) {
          if (config.verbose) {
            std::cout << "done." << std::endl;
          }
        }
        else {
          if (config.verbose) {
            std::cout << "failed." << std::endl;
          }
          else {
            std::cerr << "Warning: failed to write worker IOR to \""
                      << config.iorFileName
                      << "\"."
                      << std::endl;
          }
        }
      }
    }

    /*
     * Register with Naming Service.
     */

    if (config.registerWithNamingService) {
      if (config.verbose) {
        std::cout << "Registering with Naming Service as \""
                  << config.namingServiceName
                  << "\" ... " << std::flush;
      }

      try {
        CORBA::Object_var nso = orb->resolve_initial_references ("NameService");
        CosNaming::NamingContextExt_var ns = CosNaming::NamingContextExt::_narrow (nso);
        OCPI::CORBAUtil::Misc::nameServiceBind (ns, workerRef, config.namingServiceName);

        if (config.verbose) {
          std::cout << "done." << std::endl;
        }
      }
      catch (const CORBA::Exception & ex) {
        if (config.verbose) {
          std::cout << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex) << std::endl;
        }
        else {
          std::cerr << "Warning: Failed to register worker with Naming Service as \""
                    << config.namingServiceName
                    << "\": "
                    << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                    << "."
                    << std::endl;
        }
      }
      catch (const std::string & oops) {
        if (config.verbose) {
          std::cout << oops << std::endl;
        }
        else {
          std::cerr << "Warning: Failed to register worker with Naming Service as \""
                    << config.namingServiceName
                    << "\": "
                    << oops
                    << "."
                    << std::endl;
        }
      }
    }
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return false;
  }
  catch (const std::string & ex) {
    std::cout << "Oops: "
              << ex
              << std::endl;
    return false;
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
    return false;
  }

  std::cout << "Worker is running, process id " << workerPid << "." << std::endl;
  return true;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  loadWorker (int argc, char * argv[])
  {
    CORBA::ORB_var orb;

    try {
      orb = CORBA::ORB_init (argc, argv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: ORB_init: " << ex << std::endl;
      return -1;
    }
    catch (...) {
      std::cout << "Oops: ORB_init failed." << std::endl;
      return -1;
    }

    if (!loadWorkerInt (orb, argc, argv)) {
      return -1;
    }

    /*
     * Keep the OCPI_CORBA_ORB going.
     */

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
    std::cout << "Oops: ORB_init: " << ex << std::endl;
    return 0;
  }
  catch (...) {
    std::cout << "Oops: ORB_init failed." << std::endl;
    return 0;
  }

  if (!loadWorkerInt (orb, argc, argv)) {
    return 0;
  }

  /*
   * Shut down and destroy the OCPI_CORBA_ORB.
   */

  try {
    orb->shutdown (1);
    orb->destroy ();
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

