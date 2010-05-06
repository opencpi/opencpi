/*
 * Start an SCA CP289 Executable Device.
 *
 * Revision History:
 *
 *     04/07/2009 - Frank Pilhofer
 *                  Add signal handler for proper shutdown.
 *
 *     03/30/2009 - Frank Pilhofer
 *                  Make CPIDeviceType property configurable.
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
#include <fstream>
#include <CpiOsMisc.h>
#include <CpiOsDebug.h>
#include <CpiOsAssert.h>
#include <CpiOsFileSystem.h>
#include <CpiUtilLoadableModule.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiLoggerOStreamOutput.h>
#include <CpiUtilMisc.h>
#include <CpiStringifyCorbaException.h>
#include <CpiCORBAUtilNameServiceBind.h>
#include <CpiUtilCommandLineConfiguration.h>
#include <CosNaming.h>
#include <CF.h>
#include "Cp289ExecutableDevice.h"

class CpiEdCommandLineConfigurator
  : public CPI::Util::CommandLineConfiguration
{
public:
  CpiEdCommandLineConfigurator ();

public:
  unsigned long cpiDeviceId;
  std::string cpiDeviceType;
  std::string cpiDeviceUnit;
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

CpiEdCommandLineConfigurator::
CpiEdCommandLineConfigurator ()
  : CPI::Util::CommandLineConfiguration (g_options),
    cpiDeviceId (0),
    cpiDeviceType(),
    cpiDeviceUnit(),
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

CPI::Util::CommandLineConfiguration::Option
CpiEdCommandLineConfigurator::g_options[] = {
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "cpiDeviceId", "CPI Device Id",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::cpiDeviceId) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "cpiDeviceType", "CPI Device Type",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::cpiDeviceType) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "cpiDeviceUnit", "CPI Device Unit id",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::cpiDeviceUnit) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Endpoint",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::endpoint) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "polled", "Run the container in polling mode",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::polled) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "osName", "Operating system name to advertise",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::osName) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "processorName", "Processor name to advertise",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::processorName) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "label", "Executable device label",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::label) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "identifier", "Executable device identifier (for logging)",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::identifier) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "writeIORFile", "Write IOR to file",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::iorFileName),
    CPI_CLC_SENT(&CpiEdCommandLineConfigurator::writeIORFile) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "registerWithNamingService", "Register with Naming Service",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::namingServiceName),
    CPI_CLC_SENT(&CpiEdCommandLineConfigurator::registerWithNamingService) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "logFile", "Write log messages to file (- for stdout)",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::logFile) },
#if !defined (NDEBUG)
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::debugBreak) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "debugLevel", "Debugging level",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::debugLevel) },
#endif
  { CPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::help) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "tempDir", "Use dir for temp files (otherwise TEMP env, TMP env, or /tmp). Use '-' for none.",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::tempDir) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "devMgrIOR", "IOR of device manager",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::devMgrIOR) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "profileFileName", "profile file name for device",
    CPI_CLC_OPT(&CpiEdCommandLineConfigurator::profileFileName) },
  { CPI::Util::CommandLineConfiguration::OptionType::END }
};

static
void
printUsage (CpiEdCommandLineConfigurator & config,
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
  makeTempFileLocation (unsigned int cpiDeviceId, const char *tempDir)
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
          ntd = CPI::OS::FileSystem::fromNativeName (tmpDir);
          exists = CPI::OS::FileSystem::exists (ntd, &isdir);
        } catch (...) {
        }
        if (exists && isdir) {
          std::string relName = "cp289ExecutableDevice-";
          relName += CPI::Util::Misc::unsignedToString (cpiDeviceId);
          std::string tempFileLocation = CPI::OS::FileSystem::joinNames (ntd, relName);
          try {
            exists = CPI::OS::FileSystem::exists (tempFileLocation, &isdir);
            if (!exists) {
              CPI::OS::FileSystem::mkdir (tempFileLocation);
              exists = CPI::OS::FileSystem::exists (tempFileLocation, &isdir);
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

      if (tempFileLocation == CPI::OS::FileSystem::cwd()) {
        return;
      }
    }
    catch (...) {
    }

    try {
      CPI::OS::FileIterator fit = CPI::OS::FileSystem::list (tempFileLocation);

      while (!fit.end()) {
        try {
          CPI::OS::FileSystem::remove (fit.absoluteName());
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
      CPI::OS::FileSystem::rmdir (tempFileLocation);
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
startCpiExecutableDeviceCmdInt (CORBA::ORB_ptr orb,
                                int & argc, char * argv[],
                                bool shutdownOrbOnRelease, bool sca = false)
{
  CpiEdCommandLineConfigurator config;
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
    CPI::Logger::debug (config.label, config.debugLevel);
#endif

    CPI::Logger::Logger * logger = 0;

    if (config.logFile == "-")
      logger = new CPI::Logger::OStreamOutput (std::cout);
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

      logger = new CPI::Logger::OStreamOutput (of, true);
      if (sca) {
        logger->setProducerId (config.identifier.c_str());
        CPI::Logger::DebugLogger debug (*logger);

        debug << CPI::Logger::ProducerName (config.label)
              << "CP289 Executable Device for device id "
              << config.cpiDeviceId
              << " starting."
              << std::flush;

        debug << CPI::Logger::ProducerName (config.label)
              << CPI::Logger::Verbosity (2)
              << argc << " command-line options:"
              << std::flush;

        for (int ai=0; ai<argc; ai++) {
          debug << CPI::Logger::ProducerName (config.label)
                << CPI::Logger::Verbosity (2)
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

    CPI::SCA::Cp289ExecutableDevice * cp289ed =
      new CPI::SCA::Cp289ExecutableDevice (orb, poa, devMgr,
                                           config.profileFileName,
                                           config.identifier,
                                           config.label,                // pretty description
                                           g_tempFileLocation,                // for use by temporary files as needed
                                           static_cast<unsigned int> (config.cpiDeviceId), // sytem-wide ordinal assignment
                                           config.cpiDeviceType,        // which driver we are expecting to use for this container
                                           config.cpiDeviceUnit,
                                           config.endpoint,             // CPI comm endpoint to use
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
        CPI::CORBAUtil::Misc::nameServiceBind (ns, ed, config.namingServiceName);
        std::cout << "done." << std::endl;
      }
      catch (const CORBA::Exception & ex) {
        std::cout << "failed: "
                  << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                  << std::endl;
      }
      catch (const std::string & oops) {
        std::cout << "failed: " << oops << std::endl;
      }
    }
  } catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return CF::ExecutableDevice::_nil ();
  } catch (const std::string &s) {
    std::cout << "Couldn't create CpiExectableDevice because: "
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
 * Start the CpiExecutableDevice when started by SCA.
 * Basically do some very specific argv parsing, converting the
 * incoming arg list into our command-line arglist.
 */

static
CF::ExecutableDevice_ptr
startCpiExecutableDeviceScaInt (CORBA::ORB_ptr orb,
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
      else if (std::strcmp (argv[ai], "cpiDeviceId") == 0) 
        fmt = "--cpiDeviceId";
      else if (std::strcmp (argv[ai], "cpiDeviceType") == 0) 
        fmt = "--cpiDeviceType";
      else if (std::strcmp (argv[ai], "cpiDeviceUnit") == 0) 
        fmt = "--cpiDeviceUnit";
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
    ed = startCpiExecutableDeviceCmdInt (orb, myargc, myargv, shutdownOrbOnRelease, true);
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
  startCpiExecutableDeviceSca (int argc, char *argv[])
  {
    CORBA::ORB_var orb;
    CF::ExecutableDevice_var ed;

    try {
      orb = CORBA::ORB_init (argc, argv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: ORB_init: "
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }
    catch (...) {
      std::cout << "Oops: ORB_init failed." << std::endl;
      return -1;
    }

    ed = startCpiExecutableDeviceScaInt (orb, argc, argv, false);

    if (CORBA::is_nil (ed)) {
      return -1;
    }

    return 0;
  }

  /*
   * Entrypoint for the VxWorks command line.
   */

  int
  startCpiExecutableDevice (int argc, char *argv[])
  {
    CORBA::ORB_var orb;
    CF::ExecutableDevice_var ed;

    try {
      orb = CORBA::ORB_init (argc, argv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: "
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }
    catch (...) {
      std::cout << "Oops: ORB_init failed." << std::endl;
      return -1;
    }

    ed = startCpiExecutableDeviceCmdInt (orb, argc, argv, false);

    if (CORBA::is_nil (ed)) {
      return -1;
    }

    std::cout << "CpiExecutableDevice is running." << std::endl;

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
        CPI::OS::debugBreak ();
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

  unsigned int cpiDeviceId = 0;
  const char *tempDir;

  try {
    for (int cdi=1; cdi<argc; cdi++) {
      if (std::strcmp (argv[cdi], "cpiDeviceId") == 0 ||
          std::strcmp (argv[cdi], "--cpiDeviceId") == 0) {
        cpiDeviceId = CPI::Util::Misc::stringToUnsigned (argv[cdi+1]);
        break;
      }
      else if (std::strncmp (argv[cdi], "--cpiDeviceId=", 14) == 0) {
        cpiDeviceId = CPI::Util::Misc::stringToUnsigned (argv[cdi]+14);
        break;
      } else if (std::strncmp (argv[cdi], "--tempDir=", 10) == 0)
        tempDir = argv[cdi] + 10;
    }

    g_tempFileLocation = makeTempFileLocation (cpiDeviceId, tempDir);
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

  std::string pidFileName = CPI::OS::FileSystem::joinNames (g_tempFileLocation, ".pid");
  std::string nativePidFileName = CPI::OS::FileSystem::toNativeName (pidFileName);
  std::ifstream iPidFile (nativePidFileName.c_str());

  if (iPidFile.good()) {
    std::string strPid;
    unsigned int pid;

    iPidFile >> strPid;

    try {
      pid = CPI::Util::Misc::stringToUnsigned (strPid);
    }
    catch (...) {
      pid = 0;
    }

    if (pid && (kill (pid, 0) == 0 || errno == EPERM)) {
      std::cout << "Oops: RCC Executable Device for CPI Device Id "
                << cpiDeviceId
                << " already running, pid "
                << pid
                << "."
                << std::endl;
      return 1;
    }

    iPidFile.close ();
  }

  std::ofstream oPidFile (nativePidFileName.c_str());
  oPidFile << CPI::OS::getProcessId () << std::endl;

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

  CPI::Util::LoadableModule::setTemporaryFileLocation (g_tempFileLocation);
#endif

  CORBA::ORB_var orb;

  try {
    orb = CORBA::ORB_init (argc, argv);
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: ORB_init: "
              << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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
  CPI::OS::setCtrlCHandler (cp289edSignalHandler);

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
    ed = startCpiExecutableDeviceScaInt (orb, argc, argv, true);
  }
  else {
    ed = startCpiExecutableDeviceCmdInt (orb, argc, argv, true);
  }

  if (CORBA::is_nil (ed)) {
    return 1;
  }

  /*
   * Run.
   */

  try {
    std::cout << "CpiExecutableDevice running." << std::endl;
    orb->run ();
    std::cout << "CpiExecutableDevice shutting down." << std::endl;

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
                << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
    }

    try {
      orb->destroy ();
    }
    catch (...) {
    }
#if ! defined (__VXWORKS__)
    try {
      CPI::OS::FileSystem::remove (pidFileName);
    }
    catch (...) {
    }

    cleanTempFileLocation (g_tempFileLocation);
#endif
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
  }

  return 0;
}

