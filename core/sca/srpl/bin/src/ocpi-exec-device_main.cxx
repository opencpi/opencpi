
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
 * Start an SCA CP289 Executable Device.
 *
 * Revision History:
 *
 *     04/07/2009 - Frank Pilhofer
 *                  Add signal handler for proper shutdown.
 *
 *     03/30/2009 - Frank Pilhofer
 *                  Make OCPIDeviceType property configurable.
 *
 *     02/12/2009 - Frank Pilhofer
 *                  Add "polled" option.
 *
 *     01/21/2009 - Frank Pilhofer
 *                  Integration with container for dataplane.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <signal.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <OcpiOsMisc.h>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiOsFileSystem.h>
#include <OcpiUtilLoadableModule.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiLoggerOStreamOutput.h>
#include <OcpiUtilMisc.h>
#include <OcpiStringifyCorbaException.h>
#include <OcpiCORBAUtilNameServiceBind.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <CosNaming.h>
#include <CF.h>
#include "Cp289ExecutableDevice.h"

class OcpiEdCommandLineConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiEdCommandLineConfigurator ();

public:
  unsigned long ocpiDeviceId;
  std::string ocpiDeviceType;
  std::string ocpiDeviceUnit;
  std::string endpoint;
  bool polled;
  std::string osName;
  std::string processorName;
  std::string label;
  std::string identifier;
  std::string iorFileName;
  bool writeIORFile;
  std::string namingServiceName;
  bool registerWithNamingService;
  std::string logFile;
#if !defined (NDEBUG)
  bool debugBreak;
  unsigned long debugLevel;
#endif
  bool help;
  std::string tempDir;
  std::string devMgrIOR;
  std::string profileFileName;
private:
  static CommandLineConfiguration::Option g_options[];
};

OcpiEdCommandLineConfigurator::
OcpiEdCommandLineConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    ocpiDeviceId (0),
    ocpiDeviceType(),
    ocpiDeviceUnit(),
    endpoint(),
    polled (false),
    osName(),
    processorName(),
    label(),
    identifier(),
    writeIORFile (false),
    registerWithNamingService (false),
    logFile(),
#if !defined (NDEBUG)
    debugBreak (false),
    debugLevel (0),
#endif
    help (false),
    tempDir(),
    devMgrIOR(),
    profileFileName()
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiEdCommandLineConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "ocpiDeviceId", "OCPI Device Id",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::ocpiDeviceId), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "ocpiDeviceType", "OCPI Device Type",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::ocpiDeviceType), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "ocpiDeviceUnit", "OCPI Device Unit id",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::ocpiDeviceUnit), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Endpoint",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::endpoint), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "polled", "Run the container in polling mode",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::polled), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "osName", "Operating system name to advertise",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::osName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "processorName", "Processor name to advertise",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::processorName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "label", "Executable device label",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::label), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "identifier", "Executable device identifier (for logging)",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::identifier), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "writeIORFile", "Write IOR to file",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::iorFileName),
    OCPI_CLC_SENT(&OcpiEdCommandLineConfigurator::writeIORFile) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "registerWithNamingService", "Register with Naming Service",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::namingServiceName),
    OCPI_CLC_SENT(&OcpiEdCommandLineConfigurator::registerWithNamingService) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "logFile", "Write log messages to file (- for stdout)",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::logFile), 0},
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::debugBreak), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "debugLevel", "Debugging level",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::debugLevel), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "tempDir", "Use dir for temp files (otherwise TEMP env, TMP env, or /tmp). Use '-' for none.",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::tempDir), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "devMgrIOR", "IOR of device manager",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::devMgrIOR), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "profileFileName", "profile file name for device",
    OCPI_CLC_OPT(&OcpiEdCommandLineConfigurator::profileFileName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiEdCommandLineConfigurator & config,
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
  CORBA::ORB_ptr g_cp289edOrb = CORBA::ORB::_nil ();
  // This is needed if we need to unpack files to a local temporary place.
  std::string g_tempFileLocation;
  void
  cp289edSignalHandler ()
  {
    if (!CORBA::is_nil (g_cp289edOrb)) {
      try {
        g_cp289edOrb->shutdown (0);
      }
      catch (...) {
      }

      g_cp289edOrb = CORBA::ORB::_nil ();
    }
  }

  std::string
  makeTempFileLocation (unsigned int ocpiDeviceId, const char *tempDir)
    throw (std::string)
  {
    bool isdir=false, exists=false;
    const char * tmpDir;
    std::string ntd;
    static const char *temps[] = { "TEMP", "e", "TMP", "e", "/tmp", "d", "dummy", "a", 0 };
    for (const char **ap = temps; *ap; ap += 2) {
      switch (ap[1][0]) {
      case 'e':
        tmpDir = std::getenv(ap[0]); break;
      case 'd':
        tmpDir = ap[0]; break;
      case 'a':
        tmpDir = tempDir; break;
      default:
        tmpDir = 0;
      }
      if (tmpDir)  {
        try {
          ntd = OCPI::OS::FileSystem::fromNativeName (tmpDir);
          exists = OCPI::OS::FileSystem::exists (ntd, &isdir);
        } catch (...) {
        }
        if (exists && isdir) {
          std::string relName = "cp289ExecutableDevice-";
          relName += OCPI::Util::unsignedToString (ocpiDeviceId);
          std::string tempFileLocation = OCPI::OS::FileSystem::joinNames (ntd, relName);
          try {
            exists = OCPI::OS::FileSystem::exists (tempFileLocation, &isdir);
            if (!exists) {
              OCPI::OS::FileSystem::mkdir (tempFileLocation);
              exists = OCPI::OS::FileSystem::exists (tempFileLocation, &isdir);
            }
          } catch (...) {
          }
          if (exists && isdir)
            return tempFileLocation;
        }
      }
    }
    throw std::string ("Could not create temporary file location");
  }

  void
  cleanTempFileLocation (const std::string & tempFileLocation)
    throw ()
  {
    try {
      /*
       * Saftety check: we don't want to erase our home directory ...
       */

      if (!tempFileLocation.length()) {
        return;
      }

      if (tempFileLocation == OCPI::OS::FileSystem::cwd()) {
        return;
      }
    }
    catch (...) {
    }

    try {
      OCPI::OS::FileIterator fit = OCPI::OS::FileSystem::list (tempFileLocation);

      while (!fit.end()) {
        try {
          OCPI::OS::FileSystem::remove (fit.absoluteName());
        }
        catch (...) {
        }

        fit.next ();
      }

      fit.close ();
    }
    catch (...) {
    }

    try {
      OCPI::OS::FileSystem::rmdir (tempFileLocation);
    }
    catch (...) {
    }
  }

}

/*
 * Start the ExecutableDevice when started from the command line.
 */

static
CF::ExecutableDevice_ptr
startOcpiExecutableDeviceCmdInt (CORBA::ORB_ptr orb,
                                int & argc, char * argv[],
                                bool shutdownOrbOnRelease, bool sca = false)
{
  OcpiEdCommandLineConfigurator config;
  CF::ExecutableDevice_var ed;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cout << "Oops: " << oops << std::endl;
    return CF::ExecutableDevice::_nil ();
  }

  if (config.help) {
    printUsage (config, argv[0]);
    return CF::ExecutableDevice::_nil ();
  }
  CF::DeviceManager_var devMgr;
  if (config.devMgrIOR.length()) {
    CORBA::Object_var obj;
    try {
      obj = orb->string_to_object(config.devMgrIOR.c_str());
    } catch (...) {
      throw std::string ("DEVICE_MGR_IOR parameter invalid");
    }
    devMgr = CF::DeviceManager::_narrow (obj);
  } else
    devMgr = 0;

  try {
    CORBA::Object_var obj = orb->resolve_initial_references ("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow (obj);

#if !defined (NDEBUG)
    OCPI::Logger::debug (config.label, config.debugLevel);
#endif

    OCPI::Logger::Logger * logger = 0;

    if (config.logFile == "-")
      logger = new OCPI::Logger::OStreamOutput (std::cout);
    else if (config.logFile.length()) {
      std::ofstream * of = new std::ofstream (config.logFile.c_str());

      if (!of->good()) {
        std::cout << "Oops: can not open log file \""
                  << config.logFile
                  << "\" for writing."
                  << std::endl;
        delete of;
        return CF::ExecutableDevice::_nil ();
      }

      logger = new OCPI::Logger::OStreamOutput (of, true);
      if (sca) {
        logger->setProducerId (config.identifier.c_str());
        OCPI::Logger::DebugLogger debug (*logger);

        debug << OCPI::Logger::ProducerName (config.label)
              << "CP289 Executable Device for device id "
              << config.ocpiDeviceId
              << " starting."
              << std::flush;

        debug << OCPI::Logger::ProducerName (config.label)
              << OCPI::Logger::Verbosity (2)
              << argc << " command-line options:"
              << std::flush;

        for (int ai=0; ai<argc; ai++) {
          debug << OCPI::Logger::ProducerName (config.label)
                << OCPI::Logger::Verbosity (2)
                << "argv[" << ai << "] = \""
                << argv[ai]
                << "\""
                << std::flush;
        }
      }
    }
    /*
     * Activate the POA Manager.
     */

    PortableServer::POAManager_var mgr = poa->the_POAManager ();
    mgr->activate ();

    /*
     * Activate server.
     */

    OCPI::SCA::Cp289ExecutableDevice * cp289ed =
      new OCPI::SCA::Cp289ExecutableDevice (orb, poa, devMgr,
                                           config.profileFileName,
                                           config.identifier,
                                           config.label,                // pretty description
                                           g_tempFileLocation,                // for use by temporary files as needed
                                           static_cast<unsigned int> (config.ocpiDeviceId), // sytem-wide ordinal assignment
                                           config.ocpiDeviceType,        // which driver we are expecting to use for this container
                                           config.ocpiDeviceUnit,
                                           config.endpoint,             // OCPI comm endpoint to use
                                           config.polled,
                                           config.osName,
                                           config.processorName,
                                           logger,
                                           true,
                                           shutdownOrbOnRelease);
    if (sca) {
      PortableServer::ObjectId_var oid = poa->activate_object (cp289ed);
      CORBA::Object_var so = poa->id_to_reference (oid);
      ed = CF::ExecutableDevice::_narrow (so);
      devMgr->registerDevice (ed);
    } else {
      ed = cp289ed->_this ();
    }
    cp289ed->_remove_ref ();
    /*
     * Write object reference.
     */

    if (config.writeIORFile) {
      CORBA::String_var ior = orb->object_to_string (ed);

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
        OCPI::CORBAUtil::Misc::nameServiceBind (ns, ed, config.namingServiceName);
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
  } catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return CF::ExecutableDevice::_nil ();
  } catch (const std::string &s) {
    std::cout << "Couldn't create OcpiExectableDevice because: "
              << s
              << std::endl;
    return CF::ExecutableDevice::_nil ();
  } catch (...) {
    std::cout << "Oops." << std::endl;
    return CF::ExecutableDevice::_nil ();
  }
  return ed._retn ();
}

/*
 * Start the OcpiExecutableDevice when started by SCA.
 * Basically do some very specific argv parsing, converting the
 * incoming arg list into our command-line arglist.
 */

static
CF::ExecutableDevice_ptr
startOcpiExecutableDeviceScaInt (CORBA::ORB_ptr orb,
                                int & argc, char *argv[],
                                bool shutdownOrbOnRelease)
{
  CF::ExecutableDevice_var ed;
  char **myargv = new char *[argc*2];
  int myargc = 0;

  try {
    myargv[myargc++] = strdup(argv[0]);

    for (int ai=1; ai<argc; ai++) {
      const char *fmt = 0, *val = argv[ai+1];
      if (std::strcmp (argv[ai], "DEVICE_MGR_IOR") == 0) {
        if (ai+1 >= argc)
          throw std::string ("DEVICE_MGR_IOR parameter lacks value");
        fmt = "--devMgrIOR";
      } else if (std::strcmp (argv[ai], "PROFILE_NAME") == 0)
        fmt = "--profileFileName";
      else if (std::strcmp (argv[ai], "DEVICE_ID") == 0)
        fmt = "--identifier";
      else if (std::strcmp (argv[ai], "DEVICE_LABEL") == 0)
        fmt = "--label";
      else if (std::strcmp (argv[ai], "ocpiDeviceId") == 0)
        fmt = "--ocpiDeviceId";
      else if (std::strcmp (argv[ai], "ocpiDeviceType") == 0)
        fmt = "--ocpiDeviceType";
      else if (std::strcmp (argv[ai], "ocpiDeviceUnit") == 0)
        fmt = "--ocpiDeviceUnit";
      else if (std::strcmp (argv[ai], "endpoint") == 0)
        fmt = "--endpoint";
      else if (std::strcmp (argv[ai], "osName") == 0)
        fmt = "--osName";
      else if (std::strcmp (argv[ai], "processorName") == 0)
        fmt = "--processorName";
      else if (std::strcmp (argv[ai], "polled") == 0) {
        fmt = "--polled";
        val = !strcmp(argv[ai+1], "true") ? "true" :
          !strcmp(argv[ai+1], "false") ? "false" :
          atoi(argv[ai + 1]) ? "true" : "false";
      } else if (std::strcmp (argv[ai], "debugLevel") == 0)
        fmt = "--debugLevel";
      else if (std::strcmp (argv[ai], "logFile") == 0)
        fmt = "--logFile";
      else {
        std::string oops = "invalid command-line option: \"";
        oops += argv[ai];
        oops += "\"";
        throw oops;
      }
      asprintf(&myargv[++argc], "%s=%s", fmt, val);
    }
    myargv[argc] = 0;
    ed = startOcpiExecutableDeviceCmdInt (orb, myargc, myargv, shutdownOrbOnRelease, true);
  } catch (...) {
    for (unsigned i = 0; myargv[i]; i++)
      free(argv[i]);
    delete []myargv;
  }
  return ed;
}

extern "C" {
  /*
   * Entrypoint for the SCA on VxWorks.
   */

  int
  startOcpiExecutableDeviceSca (int argc, char *argv[])
  {
    CORBA::ORB_var orb;
    CF::ExecutableDevice_var ed;

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

    ed = startOcpiExecutableDeviceScaInt (orb, argc, argv, false);

    if (CORBA::is_nil (ed)) {
      return -1;
    }

    return 0;
  }

  /*
   * Entrypoint for the VxWorks command line.
   */

  int
  startOcpiExecutableDevice (int argc, char *argv[])
  {
    CORBA::ORB_var orb;
    CF::ExecutableDevice_var ed;

    try {
      orb = CORBA::ORB_init (argc, argv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }
    catch (...) {
      std::cout << "Oops: ORB_init failed." << std::endl;
      return -1;
    }

    ed = startOcpiExecutableDeviceCmdInt (orb, argc, argv, false);

    if (CORBA::is_nil (ed)) {
      return -1;
    }

    std::cout << "OcpiExecutableDevice is running." << std::endl;

    return 0;
  }
}

/*
 * Entrypoint for everybody else, using our command line parsing.
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

#if ! defined (__VXWORKS__)
  /*
   * Find a good place for our temporary files (i.e., the shared libraries
   * that we download).  We do that here to enable the horrible Linux hack
   * below.
   */

  unsigned int ocpiDeviceId = 0;
  const char *tempDir = 0;

  try {
    for (int cdi=1; cdi<argc; cdi++) {
      if (std::strcmp (argv[cdi], "ocpiDeviceId") == 0 ||
          std::strcmp (argv[cdi], "--ocpiDeviceId") == 0) {
        ocpiDeviceId = OCPI::Util::stringToUnsigned (argv[cdi+1]);
        break;
      }
      else if (std::strncmp (argv[cdi], "--ocpiDeviceId=", 14) == 0) {
        ocpiDeviceId = OCPI::Util::stringToUnsigned (argv[cdi]+14);
        break;
      } else if (std::strncmp (argv[cdi], "--tempDir=", 10) == 0)
        tempDir = argv[cdi] + 10;
    }

    g_tempFileLocation = makeTempFileLocation (ocpiDeviceId, tempDir);
  }
  catch (const std::string & oops) {
    std::cout << "Oops: " << oops << std::endl;
    return 1;
  }

  /*
   * Here's a horrible hack for Linux.
   *
   * We want the dynamic linker to recognize the shared libraries that we
   * download when considering load dependencies.  However, LD_LIBRARY_PATH
   * is only considered at start-up time, so by now it is too late.
   *
   * The workaround is to modify LD_LIBRARY_PATH and then restart.
   */

#if defined (__linux__)
  const char * oldLdLibraryPath = std::getenv ("LD_LIBRARY_PATH");

  if (!oldLdLibraryPath || !std::strstr (oldLdLibraryPath, g_tempFileLocation.c_str())) {
    std::string newLdLibraryPath = g_tempFileLocation;

    if (oldLdLibraryPath) {
      newLdLibraryPath += ':';
      newLdLibraryPath += oldLdLibraryPath;
    }

    setenv ("LD_LIBRARY_PATH", newLdLibraryPath.c_str(), 1);
    execv (argv[0], argv);
  }
#endif

  /*
   * Look at the directory that we plan to use for temporary files, and
   * make sure that it isn't in use.  We write our PID to .pid to make
   * sure.
   */

  std::string pidFileName = OCPI::OS::FileSystem::joinNames (g_tempFileLocation, ".pid");
  std::string nativePidFileName = OCPI::OS::FileSystem::toNativeName (pidFileName);
  std::ifstream iPidFile (nativePidFileName.c_str());

  if (iPidFile.good()) {
    std::string strPid;
    unsigned int pid;

    iPidFile >> strPid;

    try {
      pid = OCPI::Util::stringToUnsigned (strPid);
    }
    catch (...) {
      pid = 0;
    }

    if (pid && (kill (pid, 0) == 0 || errno == EPERM)) {
      std::cout << "Oops: RCC Executable Device for OCPI Device Id "
                << ocpiDeviceId
                << " already running, pid "
                << pid
                << "."
                << std::endl;
      return 1;
    }

    iPidFile.close ();
  }

  std::ofstream oPidFile (nativePidFileName.c_str());
  oPidFile << OCPI::OS::getProcessId () << std::endl;

  if (!oPidFile.good()) {
    std::cout << "Oops: Failed to write PID to \""
              << nativePidFileName
              << "\"." << std::endl;
    std::cout << "      Please delete \""
              << g_tempFileLocation
              << "\" before continuing."
              << std::endl;
    return 1;
  }

  oPidFile.close ();

  OCPI::Util::LoadableModule::setTemporaryFileLocation (g_tempFileLocation);
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

  g_cp289edOrb = orb;
  OCPI::OS::setCtrlCHandler (cp289edSignalHandler);

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

  CF::ExecutableDevice_var ed;

  if (isSca) {
    ed = startOcpiExecutableDeviceScaInt (orb, argc, argv, true);
  }
  else {
    ed = startOcpiExecutableDeviceCmdInt (orb, argc, argv, true);
  }

  if (CORBA::is_nil (ed)) {
    return 1;
  }

  /*
   * Run.
   */

  try {
    std::cout << "OcpiExecutableDevice running." << std::endl;
    orb->run ();
    std::cout << "OcpiExecutableDevice shutting down." << std::endl;

    try {
      ed->releaseObject ();
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
#if ! defined (__VXWORKS__)
    try {
      OCPI::OS::FileSystem::remove (pidFileName);
    }
    catch (...) {
    }

    cleanTempFileLocation (g_tempFileLocation);
#endif
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

