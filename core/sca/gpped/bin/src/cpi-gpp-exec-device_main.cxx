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
#include <CpiOsMisc.h>
#include <CpiOsDebug.h>
#include <CpiOsAssert.h>
#include <CpiOsFileSystem.h>
#include <CpiOsFileIterator.h>
#include <CpiUtilMisc.h>
#include <CpiUtilLoadableModule.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiLoggerOStreamOutput.h>
#include <CpiStringifyCorbaException.h>
#include <CpiCORBAUtilNameServiceBind.h>
#include <CpiUtilCommandLineConfiguration.h>
#include <CosNaming.h>
#include <CF.h>
#include "CpiScaGppExecutableDevice.h"

#if defined (__linux__)
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#endif


class GppedCommandLineConfigurator
  : public CPI::Util::CommandLineConfiguration
{
public:
  GppedCommandLineConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
  unsigned long debugLevel;
#endif
  unsigned long cpiDeviceId;
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
  : CPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
    debugLevel (0),
#endif
    cpiDeviceId (0),
    writeIORFile (false),
    registerWithNamingService (false)
{
}

CPI::Util::CommandLineConfiguration::Option
GppedCommandLineConfigurator::g_options[] = {
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "cpiDeviceId", "CPI Device Id",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::cpiDeviceId) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "osName", "Operating system name to advertise",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::osName) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "processorName", "Processor name to advertise",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::processorName) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "label", "Executable device label",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::label) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "identifier", "Executable device identifier (for logging)",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::identifier) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "writeIORFile", "Write IOR to file",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::iorFileName),
    CPI_CLC_SENT(&GppedCommandLineConfigurator::writeIORFile) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "registerWithNamingService", "Register with Naming Service",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::namingServiceName),
    CPI_CLC_SENT(&GppedCommandLineConfigurator::registerWithNamingService) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "logFile", "Write log messages to file (- for stdout)",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::logFile) },
#if !defined (NDEBUG)
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::debugBreak) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "debugLevel", "Debugging level",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::debugLevel) },
#endif
  { CPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    CPI_CLC_OPT(&GppedCommandLineConfigurator::help) },
  { CPI::Util::CommandLineConfiguration::OptionType::END }
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
  makeTempFileLocation (unsigned int cpiDeviceId)
    throw (std::string)
  {
    bool isdir=false, exists=false;
    const char * tmpDir;
    std::string ntd;

    if ((tmpDir = std::getenv ("TEMP"))) {
      try {
	ntd = CPI::OS::FileSystem::fromNativeName (tmpDir);
	exists = CPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
	exists = false;
      }
    }

    if ((!exists || !isdir) && (tmpDir = std::getenv ("TMP"))) {
      try {
	ntd = CPI::OS::FileSystem::fromNativeName (tmpDir);
	exists = CPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
	exists = false;
      }
    }

    if (!exists || !isdir) {
      try {
	ntd = CPI::OS::FileSystem::fromNativeName ("/tmp");
	exists = CPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
	exists = false;
      }
    }

    if (!exists || !isdir) {
      throw std::string ("No temp directory found");
    }

    std::string relName = "gppExecutableDevice-";
    relName += CPI::Util::Misc::unsignedToString (cpiDeviceId);

    std::string tempFileLocation = CPI::OS::FileSystem::joinNames (ntd, relName);

    try {
      CPI::OS::FileSystem::mkdir (tempFileLocation);
    }
    catch (...) {
    }

    try {
      exists = CPI::OS::FileSystem::exists (tempFileLocation, &isdir);
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
    CPI::Logger::debug (config.label, config.debugLevel);
#endif

    CPI::Logger::Logger * logger = 0;

    if (config.logFile == "-") {
      logger = new CPI::Logger::OStreamOutput (std::cout);
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

      logger = new CPI::Logger::OStreamOutput (of, true);
    }

    /*
     * Activate the POA Manager.
     */

    PortableServer::POAManager_var mgr = poa->the_POAManager ();
    mgr->activate ();

    /*
     * Activate server.
     */

    CPI::SCA::GppExecutableDevice * sred =
      new CPI::SCA::GppExecutableDevice (orb, poa,
					 CF::DeviceManager::_nil (),
					 std::string(),
					 config.identifier,
					 config.label,
					 g_tempFileLocation,
					 static_cast<unsigned int> (config.cpiDeviceId),
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
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
	      << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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
    unsigned int cpiDeviceId = 0;
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
      else if (std::strcmp (argv[ai], "cpiDeviceId") == 0) {
	cpiDeviceId = CPI::Util::Misc::stringToUnsigned (argv[++ai]);
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

    CPI::Logger::debug (deviceLabel, debugLevel);
    CPI::Logger::Logger * logger = 0;

    if (logFile == "-") {
      logger = new CPI::Logger::OStreamOutput (std::cout);
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

      logger = new CPI::Logger::OStreamOutput (of, true);
      logger->setProducerId (deviceId.c_str());
    }

    if (logger) {
      CPI::Logger::DebugLogger debug (*logger);

      debug << CPI::Logger::ProducerName (deviceLabel)
	    << "GPP Executable Device for device id "
	    << cpiDeviceId
	    << " starting."
	    << std::flush;

      debug << CPI::Logger::ProducerName (deviceLabel)
	    << CPI::Logger::Verbosity (2)
	    << argc << " command-line options:"
	    << std::flush;

      for (ai=0; ai<argc; ai++) {
	debug << CPI::Logger::ProducerName (deviceLabel)
	      << CPI::Logger::Verbosity (2)
	      << "argv[" << ai << "] = \""
	      << argv[ai]
	      << "\""
	      << std::flush;
      }
    }

    /*
     * Activate server.
     */

    CPI::SCA::GppExecutableDevice * sred =
      new CPI::SCA::GppExecutableDevice (orb, poa, devMgr,
					 profileFileName,
					 deviceId,
					 deviceLabel,
					 g_tempFileLocation,
					 cpiDeviceId,
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
	      << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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
		<< CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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
		<< CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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
      }
    }

    g_tempFileLocation = makeTempFileLocation (cpiDeviceId);
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
      std::cout << "Oops: GPP Executable Device for CPI Device Id "
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

  g_gppedOrb = orb;
  CPI::OS::setCtrlCHandler (gppedSignalHandler);

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
		<< CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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
	      << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
	      << std::endl;
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
  }

  return 0;
}

