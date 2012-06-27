
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
 * Start an SCA GPP Executable Device.
 *
 * Revision History:
 *
 *     05/28/2009 - Frank Pilhofer
 *                  Make os_name configurable, so that we can set it to
 *                  TimesysLinux, which is what SCARI uses.
 *
 *     04/06/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <OcpiOsMisc.h>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiOsFileSystem.h>
#include <OcpiOsFileIterator.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilLoadableModule.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiLoggerOStreamOutput.h>
#include <OcpiStringifyCorbaException.h>
#include <OcpiCORBAUtilNameServiceBind.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <CosNaming.h>
#include <CF.h>
#include "OcpiScaGppExecutableDevice.h"

#if defined (__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#endif


class GppedCommandLineConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  GppedCommandLineConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
  unsigned long debugLevel;
#endif
  unsigned long ocpiDeviceId;
  std::string label;
  std::string identifier;
  bool polled;
  std::string osName;
  std::string processorName;

  bool writeIORFile;
  std::string iorFileName;
  bool registerWithNamingService;
  std::string namingServiceName;
  std::string logFile;

private:
  static CommandLineConfiguration::Option g_options[];
};

GppedCommandLineConfigurator::
GppedCommandLineConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
    debugLevel (0),
#endif
    ocpiDeviceId (0),
    writeIORFile (false),
    registerWithNamingService (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
GppedCommandLineConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "ocpiDeviceId", "OCPI Device Id",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::ocpiDeviceId), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "osName", "Operating system name to advertise",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::osName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "processorName", "Processor name to advertise",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::processorName), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "label", "Executable device label",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::label), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "identifier", "Executable device identifier (for logging)",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::identifier), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "writeIORFile", "Write IOR to file",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::iorFileName),
    OCPI_CLC_SENT(&GppedCommandLineConfigurator::writeIORFile) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "registerWithNamingService", "Register with Naming Service",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::namingServiceName),
    OCPI_CLC_SENT(&GppedCommandLineConfigurator::registerWithNamingService) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "logFile", "Write log messages to file (- for stdout)",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::logFile), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::debugBreak), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "debugLevel", "Debugging level",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::debugLevel), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&GppedCommandLineConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (GppedCommandLineConfigurator & config,
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
  CORBA::ORB_ptr g_gppedOrb = CORBA::ORB::_nil ();
  std::string g_tempFileLocation;

  void
  gppedSignalHandler ()
  {
    if (!CORBA::is_nil (g_gppedOrb)) {
      try {
        g_gppedOrb->shutdown (0);
      }
      catch (...) {
      }

      g_gppedOrb = CORBA::ORB::_nil ();
    }
  }

  std::string
  makeTempFileLocation (unsigned int ocpiDeviceId)
    throw (std::string)
  {
    bool isdir=false, exists=false;
    const char * tmpDir;
    std::string ntd;

    if ((tmpDir = std::getenv ("TEMP"))) {
      try {
        ntd = OCPI::OS::FileSystem::fromNativeName (tmpDir);
        exists = OCPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
        exists = false;
      }
    }

    if ((!exists || !isdir) && (tmpDir = std::getenv ("TMP"))) {
      try {
        ntd = OCPI::OS::FileSystem::fromNativeName (tmpDir);
        exists = OCPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
        exists = false;
      }
    }

    if (!exists || !isdir) {
      try {
        ntd = OCPI::OS::FileSystem::fromNativeName ("/tmp");
        exists = OCPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
        exists = false;
      }
    }

    if (!exists || !isdir) {
      throw std::string ("No temp directory found");
    }

    std::string relName = "gppExecutableDevice-";
    relName += OCPI::Util::unsignedToString (ocpiDeviceId);

    std::string tempFileLocation = OCPI::OS::FileSystem::joinNames (ntd, relName);

    try {
      OCPI::OS::FileSystem::mkdir (tempFileLocation);
    }
    catch (...) {
    }

    try {
      exists = OCPI::OS::FileSystem::exists (tempFileLocation, &isdir);
    }
    catch (...) {
      exists = false;
    }

    if (!exists || !isdir) {
      throw std::string ("Could not create temporary file location");
    }

    return tempFileLocation;
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
 * Start the GppExecutableDevice when started from the command line.
 */

static
CF::ExecutableDevice_ptr
startGppExecutableDeviceCmdInt (CORBA::ORB_ptr orb,
                                int & argc, char * argv[],
                                bool shutdownOrbOnRelease)
{
  GppedCommandLineConfigurator config;
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

  try {
    CORBA::Object_var obj = orb->resolve_initial_references ("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow (obj);

#if !defined (NDEBUG)
    OCPI::Logger::debug (config.label, config.debugLevel);
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
        return CF::ExecutableDevice::_nil ();
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

    OCPI::SCA::GppExecutableDevice * sred =
      new OCPI::SCA::GppExecutableDevice (orb, poa,
                                         CF::DeviceManager::_nil (),
                                         std::string(),
                                         config.identifier,
                                         config.label,
                                         g_tempFileLocation,
                                         static_cast<unsigned int> (config.ocpiDeviceId),
                                         config.osName,
                                         config.processorName,
                                         logger,
                                         true,
                                         shutdownOrbOnRelease);

    ed = sred->_this ();
    sred->_remove_ref ();

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
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return CF::ExecutableDevice::_nil ();
  }
  catch (const std::string & ex) {
    std::cout << "Oops: " << ex << std::endl;
    return CF::ExecutableDevice::_nil ();
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
    return CF::ExecutableDevice::_nil ();
  }

  return ed._retn ();
}

/*
 * Start the GppExecutableDevice when started by SCA.
 */

static
CF::ExecutableDevice_ptr
startGppExecutableDeviceScaInt (CORBA::ORB_ptr orb,
                                int & argc, char *argv[],
                                bool shutdownOrbOnRelease)
{
  CF::ExecutableDevice_var ed;
  int ai;

  try {
    CF::DeviceManager_var devMgr;
    unsigned int ocpiDeviceId = 0;
    std::string profileFileName;
    std::string deviceId;
    std::string deviceLabel;
    std::string logFile;
    std::string osName;
    std::string processorName;
    int debugLevel = 0;

    for (ai=1; ai<argc; ai++) {
      if (std::strcmp (argv[ai], "DEVICE_MGR_IOR") == 0) {
        if (ai+1 >= argc) {
          throw std::string ("DEVICE_MGR_IOR parameter lacks value");
        }

        CORBA::Object_var obj;

        try {
          obj = orb->string_to_object (argv[++ai]);
        }
        catch (...) {
          throw std::string ("DEVICE_MGR_IOR parameter invalid");
        }

        devMgr = CF::DeviceManager::_narrow (obj);
      }
      else if (std::strcmp (argv[ai], "PROFILE_NAME") == 0) {
        profileFileName  = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "DEVICE_ID") == 0) {
        deviceId = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "DEVICE_LABEL") == 0) {
        deviceLabel = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "ocpiDeviceId") == 0) {
        ocpiDeviceId = OCPI::Util::stringToUnsigned (argv[++ai]);
      }
      else if (std::strcmp (argv[ai], "osName") == 0) {
        osName = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "processorName") == 0) {
        processorName = argv[++ai];
      }
      else if (std::strcmp (argv[ai], "debugLevel") == 0) {
        debugLevel = atoi (argv[++ai]);
      }
      else if (std::strcmp (argv[ai], "logFile") == 0) {
        logFile = argv[++ai];
      }
      else {
        std::string oops = "invalid command-line option: \"";
        oops += argv[ai];
        oops += "\"";
        throw oops;
      }
    }

    CORBA::Object_var obj = orb->resolve_initial_references ("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow (obj);
    PortableServer::POAManager_var mgr = poa->the_POAManager ();
    mgr->activate ();

    /*
     * Set debug level and initialize logger.
     */

    OCPI::Logger::debug (deviceLabel, debugLevel);
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
        return CF::ExecutableDevice::_nil ();
      }

      logger = new OCPI::Logger::OStreamOutput (of, true);
      logger->setProducerId (deviceId.c_str());
    }

    if (logger) {
      OCPI::Logger::DebugLogger debug (*logger);

      debug << OCPI::Logger::ProducerName (deviceLabel)
            << "GPP Executable Device for device id "
            << ocpiDeviceId
            << " starting."
            << std::flush;

      debug << OCPI::Logger::ProducerName (deviceLabel)
            << OCPI::Logger::Verbosity (2)
            << argc << " command-line options:"
            << std::flush;

      for (ai=0; ai<argc; ai++) {
        debug << OCPI::Logger::ProducerName (deviceLabel)
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

    OCPI::SCA::GppExecutableDevice * sred =
      new OCPI::SCA::GppExecutableDevice (orb, poa, devMgr,
                                         profileFileName,
                                         deviceId,
                                         deviceLabel,
                                         g_tempFileLocation,
                                         ocpiDeviceId,
                                         osName,
                                         processorName,
                                         logger,
                                         true,
                                         shutdownOrbOnRelease);

    PortableServer::ObjectId_var oid = poa->activate_object (sred);
    CORBA::Object_var so = poa->id_to_reference (oid);
    ed = CF::ExecutableDevice::_narrow (so);

    /*
     * Register with Device Manager.
     */

    devMgr->registerDevice (ed);

    /*
     * Done.
     */

    sred->_remove_ref ();
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return CF::ExecutableDevice::_nil ();
  }
  catch (const std::string & ex) {
    std::cout << "Oops: " << ex << std::endl;
    return CF::ExecutableDevice::_nil ();
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
    return CF::ExecutableDevice::_nil ();
  }

  return ed._retn ();
}

extern "C" {
  /*
   * Entrypoint for the SCA on VxWorks.
   */

  int
  startGppExecutableDeviceSca (int argc, char *argv[])
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

    ed = startGppExecutableDeviceScaInt (orb, argc, argv, false);

    if (CORBA::is_nil (ed)) {
      return -1;
    }

    return 0;
  }

  /*
   * Entrypoint for the VxWorks command line.
   */

  int
  startGppExecutableDevice (int argc, char *argv[])
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

    ed = startGppExecutableDeviceCmdInt (orb, argc, argv, false);

    if (CORBA::is_nil (ed)) {
      return -1;
    }

    std::cout << "GppExecutableDevice is running." << std::endl;

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

#if ! defined (__VXWORKS__)
  /*
   * Find a good place for our temporary files (i.e., the shared libraries
   * that we download).  We do that here to enable the horrible Linux hack
   * below.
   */

  unsigned int ocpiDeviceId = 0;

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
      }
    }

    g_tempFileLocation = makeTempFileLocation (ocpiDeviceId);
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
      std::cout << "Oops: GPP Executable Device for OCPI Device Id "
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

  g_gppedOrb = orb;
  OCPI::OS::setCtrlCHandler (gppedSignalHandler);

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
    ed = startGppExecutableDeviceScaInt (orb, argc, argv, true);
  }
  else {
    ed = startGppExecutableDeviceCmdInt (orb, argc, argv, true);
  }

  if (CORBA::is_nil (ed)) {
    return 1;
  }

  /*
   * Run.
   */

  try {
    std::cout << "GppExecutableDevice running." << std::endl;
    orb->run ();
    std::cout << "GppExecutableDevice shutting down." << std::endl;

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

