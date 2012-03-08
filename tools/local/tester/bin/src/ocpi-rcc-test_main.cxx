
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
 * RCC Worker Test Harness.
 *
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  Set default for --polled to true to match the default
 *                  "ocpi-smb-pio" endpoint.  Can be overridden using
 *                  --polled=false (requires matching --endpoint).
 *
 *     06/04/2009 - Frank Pilhofer
 *                  - Added --dllDir and --pauseBeforeStart options.
 *                  - Add support for sequences in --printProperties.
 *
 *     05/25/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <map>
#include <cctype>
#include <OcpiOsMisc.h>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiOsTimer.h>
#include <OcpiOsDataTypes.h>
#include <OcpiOsFileSystem.h>
#include <OcpiOsFileIterator.h>
#include <OcpiOsThreadManager.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilZipFs.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilLoadableModule.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <OcpiContainerInterface.h>
#include <OcpiContainerPort.h>
#include <OcpiWorker.h>
#include <RCC_Worker.h>
#include "sca_props.h"

#if ! defined (__VXWORKS__)
#include <sys/types.h>
#include <signal.h>
#include <cerrno>
#endif

#if defined (__linux__)
#include <unistd.h>
#endif
#define CREATE_WORKER(app, disp) \
  (static_cast<OCPI::Container::Worker *>(&app->createWorker (NULL, NULL, (char*)disp, NULL)))

/*
 * ----------------------------------------------------------------------
 * File I/O Workers.
 * ----------------------------------------------------------------------
 */

#include "fileSink.h"
#include "fileSource.h"

extern RCCDispatch TestWorkerFileSinkWorker;
extern RCCDispatch TestWorkerFileSourceWorker;

/*
 * ----------------------------------------------------------------------
 * Command-line configuration.
 * ----------------------------------------------------------------------
 */

namespace {
  enum {
    OCPI_TEST_WORKER_DEFAULT_NUM_BUFFERS = 2,
    OCPI_TEST_WORKER_DEFAULT_BUFFER_SIZE = 1 << 20,
    OCPI_TEST_WORKER_DEFAULT_ENDPOINT_SIZE = 1 << 26
  };
}

class TestWorkerCommandLineConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  TestWorkerCommandLineConfigurator ();

public:
  bool help;
  bool verbose;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  unsigned long numBuffers;
  unsigned long bufferSize;
  unsigned long endpointSize;
  std::string endpoint;
  bool polled;

  std::string workerFile;
  std::string entryPoint;
  unsigned long defaultPacketSize;
  MultiNameValue inputFile;
  MultiNameValue outputFile;
  MultiNameValue property;
  MultiString loadDll;
  std::string dllDir;
  bool pauseBeforeStart;
  unsigned long timeout;
  unsigned long slackTime;
  bool printProperties;

  std::string logFile;

private:
  static CommandLineConfiguration::Option g_options[];
};

TestWorkerCommandLineConfigurator::
TestWorkerCommandLineConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    numBuffers (OCPI_TEST_WORKER_DEFAULT_NUM_BUFFERS),
    bufferSize (OCPI_TEST_WORKER_DEFAULT_BUFFER_SIZE),
    endpointSize (OCPI_TEST_WORKER_DEFAULT_ENDPOINT_SIZE),
    polled (true),
    defaultPacketSize (0),
    pauseBeforeStart (false),
    timeout (0),
    slackTime (3),
    printProperties (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
TestWorkerCommandLineConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Endpoint (overrides endpoint size)",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::endpoint), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "polled", "Run the container in polling mode",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::polled), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "numBuffers", "Number of buffers per port (2)",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::numBuffers), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "bufferSize", "Buffer size (1M)",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::bufferSize), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "endpointSize", "Endpoint size (64M)",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::endpointSize), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "workerFile", "Worker executable file",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::workerFile), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "entryPoint", "Worker entrypoint",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::entryPoint), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "defaultPacketSize", "Default packet size for input ports",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::defaultPacketSize), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "inputFile", "Connect input file to input port",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::inputFile), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "outputFile", "Connect output file to output port",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::outputFile), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "configure", "Worker property configuration",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::property), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
    "loadDll", "Load this DLL before loading the worker",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::loadDll), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "dllDir", "Use this directory for linking DLL files",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::dllDir), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "pauseBeforeStart", "Pause before start",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::pauseBeforeStart), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "timeout", "Seconds to wait for input ports to drain",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::timeout), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "slackTime", "Seconds to wait for things to settle",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::slackTime), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "printProperties", "Print properties before and after run",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::printProperties), 0 },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&TestWorkerCommandLineConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (TestWorkerCommandLineConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * ----------------------------------------------------------------------
 * Signal handler.
 * ----------------------------------------------------------------------
 */

namespace {
  bool g_expectSignal = false;

  void
  testerSignalHandler ()
  {
    /*
     * Ignore this SIGINT, if it is expected.
     */

    if (!g_expectSignal) {
      exit (1);
    }

    g_expectSignal = false;
  }
}


/*
 * ----------------------------------------------------------------------
 * Worker Test Harness.
 * ----------------------------------------------------------------------
 */

class TestWorker {
public:
  TestWorker (const TestWorkerCommandLineConfigurator & config)
    throw ();

  ~TestWorker ()
    throw ();

  void runTest ()
    throw (std::string);

private:
  void createAllPorts ()
    throw (std::string);

  void connectInputPorts ()
    throw (std::string);

  void connectOutputPorts ()
    throw (std::string);

  void configureWorker ()
    throw (std::string);

  void printWorkerProperties ()
    throw ();

  unsigned int findProperty (const char * name)
    throw (std::string);

  const OCPI::SCA::Port * findPort (const char * name, unsigned int & portOrdinal)
    throw (std::string);

  void loadDll (OCPI::Util::Vfs::Vfs & fs,
                const std::string & fileName)
    throw (std::string);

  void unloadDll (const std::string & fileName)
    throw ();

  static std::string normalizeFileName (const std::string & fileName)
    throw ();

  static void dumpOctets (const unsigned char * data, unsigned int length)
    throw ();

private:
  struct PortData {
    bool provider;
    OCPI::Container::Port * localPort;

    bool connected;
    std::string fileName;
    OCPI::Container::Worker *fileIoWorkerId;
    OCPI::Container::Port * fileIoPort;
  };

  typedef std::map<std::string, PortData> PortMap;

  /*
   * Information about loaded DLLs.
   */

  typedef std::vector<std::string> Strings;

  struct LoadedDllInfo {
    unsigned int referenceCount;
    OCPI::OS::LoadableModule * lm;
    OCPI::Util::LoadableModule * module;
  };

  typedef std::map<std::string, LoadedDllInfo> LoadedDllInfos;

private:
  const TestWorkerCommandLineConfigurator & m_config;

  OCPI::API::Container * m_container;

  OCPI::API::ContainerApplication * m_appContext;
  OCPI::Container::Worker *m_containerWorkerId;

  std::string workerProperties;
  LoadedDllInfos m_loadedDlls;
  PortMap m_portMap;

  unsigned int m_nprops;
  unsigned int m_nports;
  unsigned int m_ntests;
  unsigned int m_sizeOfPropertySpace;
  OCPI::SCA::Property * m_props;
  OCPI::SCA::Port * m_ports;
  OCPI::SCA::Test * m_tests;

  unsigned int m_numBuffers;
  unsigned int m_bufferSize;
  bool m_verbose;

protected:
  static const std::string s_metaFileName;
};

const std::string
TestWorker::
s_metaFileName = "/ocpi-meta.inf";

TestWorker::
TestWorker (const TestWorkerCommandLineConfigurator & config)
  throw ()
  : m_config (config),
    m_container (0),
    m_appContext (0),
    m_containerWorkerId (0),
    m_props (0),
    m_ports (0),
    m_tests (0),
    m_numBuffers (static_cast<unsigned int> (config.numBuffers)),
    m_bufferSize (static_cast<unsigned int> (config.bufferSize)),
    m_verbose (config.verbose)
{
}

TestWorker::
~TestWorker ()
  throw ()
{
  try {
    m_container->stop();
  }
  catch (...) {
  }

  for (PortMap::iterator spit = m_portMap.begin(); spit != m_portMap.end(); spit++) {
    PortData & pd = (*spit).second;

    if (pd.connected) {
      try {
        pd.localPort->disconnect();
        pd.fileIoPort->disconnect();
      }
      catch (...) {
      }

      try {
        delete pd.fileIoWorkerId;
      }
      catch (...) {
      }
    }
  }

  try {
    delete m_containerWorkerId;
  }
  catch (...) {
  }

  try {
    if (m_appContext) {
      delete m_appContext;
    }
  }
  catch (...) {
  }

  delete m_container;

  /*
   * Unload all DLLs.
   */

  for (LoadedDllInfos::iterator ldi = m_loadedDlls.begin ();
       ldi != m_loadedDlls.end ();
       ldi++) {
    LoadedDllInfo & dllInfo = (*ldi).second;

    try {
      if (dllInfo.lm) {
        dllInfo.lm->close ();
      }
      else {
        dllInfo.module->close ();
      }
    }
    catch (const std::string &) {
    }

    delete dllInfo.lm;
    delete dllInfo.module;
  }

  free (m_props);
}


void
TestWorker::
runTest ()
  throw (std::string)
{
  std::string endpoint;
  if (m_config.endpoint.length()) {
    endpoint = m_config.endpoint.c_str ();
  }
  else {
    endpoint  = "ocpi-smb-pio:ocpi-rcc-test:";
    endpoint += OCPI::Util::Misc::unsignedToString (static_cast<unsigned int> (m_config.endpointSize));
    endpoint += ".1.1";
  }

  if (m_verbose) {
    std::cout << "Creating RCC container ... " << std::flush;
  }

  try {
    OCPI::Util::PValue cprops[] = {OCPI::Util::PVString("endpoint",(char*)endpoint.c_str() ),
                                  OCPI::Util::PVBool("polling",m_config.polled),
                                  OCPI::Util::PVBool("ownThread",true),
                                  OCPI::Util::PVEnd };
    m_container = OCPI::API::ContainerManager::find("rcc", NULL, cprops);
    m_appContext = m_container->createApplication ();
  }
  catch (const OCPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error creating RCC container: error code ";
    msg += OCPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

    if (auxInfo && *auxInfo) {
      msg += ": ";
      msg += auxInfo;
    }

    throw msg;
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  if (m_verbose) {
    std::cout << "Opening Worker ... " << std::flush;
  }

  OCPI::Util::FileFs::FileFs fileFs ("/");
  std::string workerFile = fileFs.fromNativeName (m_config.workerFile);
  OCPI::Util::ZipFs::ZipFs zfs (&fileFs, workerFile, std::ios_base::in,
                               false, true);

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Parse meta-data.
   */

  if (m_verbose) {
    std::cout << "Loading worker meta-data ... " << std::flush;
  }

  if (!zfs.exists (s_metaFileName)) {
    std::string msg = "File \"";
    msg += s_metaFileName;
    msg += "\" not found in ZIP archive \"";
    msg += m_config.workerFile;
    msg += "\"";
    throw msg;
  }

  OCPI::Util::EzXml::Doc metaDoc (zfs, s_metaFileName);
  ezxml_t metaRoot = metaDoc.getRootNode ();
  ezxml_t fileNode = ezxml_child (metaRoot, "file");
  bool foundEntrypoint = false;

  if (!fileNode || !ezxml_txt (fileNode)) {
    std::string msg = "Error parsing meta-data from \"";
    msg += s_metaFileName;
    msg += "\": \"file\" field missing or empty";
    throw msg;
  }

  std::string workerFileName = ezxml_txt (fileNode);

  /*
   * Worker information.
   */

  ezxml_t workerNode = ezxml_child (metaRoot, "worker");

  while (workerNode) {
    ezxml_t entrypointNode = ezxml_child (workerNode, "entrypoint");
    const char * entrypointName;

    if (!entrypointNode || !(entrypointName  = ezxml_txt (entrypointNode))) {
      std::string msg = "Error parsing meta-data from \"";
      msg += s_metaFileName;
      msg += "\": \"entrypoint\" field missing or empty";
      throw msg;
    }

    ezxml_t propNode = ezxml_child (workerNode, "properties");
    const char * propertyMagic;

    if (!propNode || !(propertyMagic = ezxml_txt (propNode))) {
      std::string msg = "Error parsing meta-data from \"";
      msg += s_metaFileName;
      msg += "\": \"properties\" field missing or empty";
      throw msg;
    }

    if (m_config.entryPoint == entrypointName) {
      workerProperties = propertyMagic;
      foundEntrypoint = true;
      break;
    }

    workerNode = ezxml_next (workerNode);
  }

  /*
   * Dependencies.
   */

  Strings dllFiles;
  ezxml_t depNode = ezxml_child (metaRoot, "dependency");

  while (depNode) {
    const char * depFileName = ezxml_txt (depNode);

    if (!depFileName) {
      std::string msg = "Error parsing meta-data from \"";
      msg += s_metaFileName;
      msg += "\": \"dependency\" field empty";
      throw msg;
    }

    dllFiles.push_back (depFileName);
    depNode = ezxml_next (depNode);
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  if (!foundEntrypoint) {
    std::string msg = "Entrypoint \"";
    msg += m_config.entryPoint;
    msg += "\" not found in meta-data";
    throw msg;
  }

  /*
   * Load all DLLs requested on the command line.
   */

  for (OCPI::Util::CommandLineConfiguration::MultiString::const_iterator dllit = m_config.loadDll.begin();
       dllit != m_config.loadDll.end(); dllit++) {
    std::string dllFileName = fileFs.fromNativeName (*dllit);
    loadDll (fileFs, dllFileName);
  }

  /*
   * Load all DLLs from the worker package.
   *
   * Traverse the list of DLLs in reverse order, i.e., load the
   * "least-significant" DLLs first.
   */

  Strings::iterator dfi = dllFiles.end ();

  while (dfi != dllFiles.begin()) {
    dfi--;
    loadDll (zfs, *dfi);
  }

  loadDll (zfs, workerFileName);

  if (m_verbose) {
    std::cout << "Locating entry point \"" << m_config.entryPoint << "\" ... " << std::flush;
  }

  LoadedDllInfos::iterator ldi = m_loadedDlls.find (workerFileName);
  ocpiAssert (ldi != m_loadedDlls.end());
  LoadedDllInfo & loadedDll = (*ldi).second;
  ocpiAssert (loadedDll.module);

  void * epPtr;

  try {
    epPtr = loadedDll.module->getSymbol (m_config.entryPoint);
  }
  catch (const std::string & oops) {
    std::string msg = "Entrypoint symbol \"";
    msg += m_config.entryPoint;
    msg += "\" not found in \"";
    msg += workerFileName;
    msg += "\"";
    throw msg;
  }

  ocpiAssert (epPtr);

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Load worker.
   */

  if (m_verbose) {
    std::cout << "Creating worker instance ... " << std::flush;
  }

  try {
    m_containerWorkerId = CREATE_WORKER(m_appContext, epPtr);
  }
  catch (const OCPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error loading worker into container: error code ";
    msg += OCPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

    if (auxInfo && *auxInfo) {
      msg += ": ";
      msg += auxInfo;
    }

    throw msg;
  }

  if (OCPI::SCA::decode_props (workerProperties.c_str(),
                              &m_props, &m_nprops, &m_sizeOfPropertySpace,
                              &m_ports, &m_nports,
                              &m_tests, &m_ntests)) {
    throw std::string ("Invalid property string");
  }

  createAllPorts ();

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Connect ports.
   */

  connectInputPorts ();
  connectOutputPorts ();

  /*
   * Configure worker.
   */

  configureWorker ();

  /*
   * Print worker properties.
   */

  if (m_config.printProperties) {
    std::cout << "Worker properties before start:" << std::endl;
    printWorkerProperties ();
  }

  /*
   * Take a debug break, if desired.
   */

  if (m_config.pauseBeforeStart) {
    if (m_verbose) {
      std::cout << "Pausing, send SIGINT to pid "
                << OCPI::OS::getProcessId()
                << " to continue ... " << std::flush;
    }

    g_expectSignal = true;
    OCPI::OS::setCtrlCHandler (testerSignalHandler);

#if defined (__linux__)
    pause ();
#else
    OCPI::OS::debugBreak ();
#endif

    if (m_verbose) {
      std::cout << "done" << std::endl;
    }
  }

  /*
   * Start the container.
   */

  if (m_verbose) {
    std::cout << "Starting the RCC container ... " << std::flush;
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Start the dispatch thread.
   */

  if (m_verbose) {
    std::cout << "Starting the dispatch thread ... " << std::flush;
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Start all file i/o workers.
   */

  for (PortMap::iterator spit = m_portMap.begin(); spit != m_portMap.end(); spit++) {
    PortData & pd = (*spit).second;

    if (pd.connected) {
      try {
	pd.fileIoWorkerId->start();
      } catch (...) {
        std::string msg = "Failed to open ";
        msg += pd.provider ? "output" : "input";
        msg += " file \"";
        msg += pd.fileName;
        msg += "\" for port \"";
        msg += (*spit).first;
        msg += "\"";
        throw msg;
      }
    }
  }

  /*
   * Start the worker-under-test.
   */

  if (m_verbose) {
    std::cout << "Starting worker ... " << std::flush;
  }

  try {
    m_containerWorkerId->start();
  } catch (...)  {
    throw std::string ("Failed to start worker");
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Now let everything run for a while, until all input ports are at EOF or
   * we time out.
   */

  if (m_verbose) {
    std::cout << "Running worker ... " << std::flush;
  }

  bool allInputWorkersAtEof = false;
  OCPI::OS::Timer timer (true);

  while (!allInputWorkersAtEof) {
    OCPI::OS::sleep (100);

    allInputWorkersAtEof = true;

    for (PortMap::iterator spit = m_portMap.begin(); spit != m_portMap.end(); spit++) {
      PortData & pd = (*spit).second;

      if (pd.connected && pd.provider) {
        uint8_t atEof;

        pd.fileIoWorkerId->read (
                                  offsetof (FileSourceProperties, atEof),
                                  1, &atEof);

        if (!atEof) {
          allInputWorkersAtEof = false;
          break;
        }
      }
    }

    OCPI::OS::ElapsedTime et = timer.getElapsed();

    if (m_config.timeout && et.seconds() >= static_cast<unsigned int> (m_config.timeout)) {
      break;
    }
  }

  if (m_verbose) {
    if (!allInputWorkersAtEof) {
      std::cout << "timeout!" << std::endl;
    }
    else {
      std::cout << "done." << std::endl;
    }
  }

  /*
   * All input workers are at EOF.  Give everything a little more time to
   * settle down.
   */

  if (m_verbose) {
    std::cout << "Giving the worker some slack ... " << std::flush;
  }

  unsigned int slackTime = static_cast<unsigned int> (m_config.slackTime);

  for (unsigned int foo=0; foo<slackTime; foo++) {
    OCPI::OS::sleep (1000);
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Stop the worker-under-test.
   */

  if (m_verbose) {
    std::cout << "Stopping worker ... " << std::flush;
  }

  try {
    m_containerWorkerId->stop();
  } catch (...) {
    throw std::string ("Failed to stop worker");
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Print worker properties.
   */

  if (m_config.printProperties) {
    std::cout << "Worker properties after stop:" << std::endl;
    printWorkerProperties ();
  }

  /*
   * Stop all file i/o workers.
   */

  for (PortMap::iterator spit = m_portMap.begin(); spit != m_portMap.end(); spit++) {
    PortData & pd = (*spit).second;

    if (pd.connected) {
      try {
	pd.fileIoWorkerId->stop();
      } catch (...) {
        std::string msg = "Failed to stop ";
        msg += pd.provider ? "output" : "input";
        msg += " file \"";
        msg += pd.fileName;
        msg += "\" for port \"";
        msg += (*spit).first;
        msg += "\"";
        throw msg;
      }
    }
  }

  /*
   * Stop the container.
   */

  if (m_verbose) {
    std::cout << "Stopping the RCC container ... " << std::flush;
  }

  m_container->stop ();

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Stop the dispatch thread.
   */

  if (m_verbose) {
    std::cout << "Stopping the dispatch thread ... " << std::flush;
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }
}

/*
 * Create all ports.
 */

void
TestWorker::
createAllPorts ()
  throw (std::string)
{
  const OCPI::SCA::Port * portInfo = m_ports;
  unsigned int portOrdinal = 0;

  /*
   * The container wants us to create all ports at construction time.
   */

  for (; portOrdinal<m_nports; portOrdinal++, portInfo++) {
    PortData & pd = m_portMap[portInfo->name];
    pd.provider = portInfo->provider;
    pd.connected = false;

    try {
      if (portInfo->provider) {
        pd.localPort = & m_containerWorkerId->createInputPort (  portOrdinal,
								 m_numBuffers,
								 m_bufferSize, NULL);
      }
      else {
        pd.localPort = & m_containerWorkerId->createOutputPort (
                                                          portOrdinal,
                                                          m_numBuffers,
                                                          m_bufferSize, NULL);
      }
    }
    catch (const OCPI::Util::EmbeddedException & oops) {
      const char * auxInfo = oops.getAuxInfo ();

      std::string msg = "Failed to create ";
      msg += (portInfo->provider ? "target" : "source");
      msg += " port \"";
      msg += portInfo->name;
      msg += "\": ";
      msg += OCPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

      if (auxInfo) {
        msg += " (";
        msg += auxInfo;
        msg += ")";
      }

      throw msg;
    }
  }
}

/*
 * Connect input ports.
 */

void
TestWorker::
connectInputPorts ()
  throw (std::string)
{
  try {
    for (OCPI::Util::CommandLineConfiguration::MultiNameValue::const_iterator ipit = m_config.inputFile.begin();
         ipit != m_config.inputFile.end(); ipit++) {
      const std::string & portName = (*ipit).first;
      std::string fileName = (*ipit).second;
      std::string::size_type colonPos = fileName.find (':');
      uint32_t packetSize = static_cast<uint32_t> (m_config.defaultPacketSize);

      if (colonPos != std::string::npos) {
        packetSize = OCPI::Util::Misc::stringToUnsigned (fileName.substr (colonPos+1));
        fileName = fileName.substr (0, colonPos);
      }

      if (m_verbose) {
        std::cout << "Connecting file \"" << fileName << "\" to input port \"" << portName << "\" ... " << std::flush;
      }

      unsigned int portOrdinal;

      findPort (portName.c_str(), portOrdinal);

      PortMap::iterator pit = m_portMap.find (portName);
      PortData & pd = (*pit).second;

      if (!pd.provider) {
        std::string msg = "Port \"";
        msg += portName;
        msg += "\" is not an input port";
        throw msg;
      }

      /*
       * Instantiate file input worker.
       */

      pd.fileIoWorkerId = CREATE_WORKER(m_appContext, &TestWorkerFileSourceWorker);

      /*
       * Connect file input worker to worker port.
       */

      pd.fileIoPort = & pd.fileIoWorkerId->createOutputPort ( 0,
                                                         m_numBuffers,
							      m_bufferSize,NULL);

      std::string initialProviderInfo, shadowPort;
      pd.localPort->getInitialProviderInfo(NULL, initialProviderInfo);
      pd.fileIoPort->setFinalProviderInfo(initialProviderInfo, shadowPort);
      pd.localPort->setFinalUserInfo(shadowPort);

      pd.connected = true;
      pd.fileName = fileName;

      /*
       * Configure file input worker.
       */

      ocpiAssert (fileName.length()+1 <= 256);
      pd.fileIoWorkerId->write (
                                 offsetof (FileSourceProperties, fileName),
                                 fileName.length() + 1,
                                 fileName.c_str());

      ocpiAssert (portName.length()+1 <= 256);
      pd.fileIoWorkerId->write (
                                 offsetof (FileSourceProperties, portName),
                                 portName.length() + 1,
                                 portName.c_str());

      uint8_t data = m_verbose ? 1 : 0;
      pd.fileIoWorkerId->write (
                                 offsetof (FileSourceProperties, verbose),
                                 1, &data);

      if (packetSize) {
        pd.fileIoWorkerId->write (
                                   offsetof (FileSourceProperties, bytesPerPacket),
                                   4, &packetSize);
      }

      if (m_verbose) {
        std::cout << "done." << std::endl;
      }
    }
  }
  catch (const OCPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error connectint input ports: error code ";
    msg += OCPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

    if (auxInfo && *auxInfo) {
      msg += ": ";
      msg += auxInfo;
    }

    throw msg;
  }
}

/*
 * Connect output ports.
 */

void
TestWorker::
connectOutputPorts ()
  throw (std::string)
{
  try {
    for (OCPI::Util::CommandLineConfiguration::MultiNameValue::const_iterator opit = m_config.outputFile.begin();
         opit != m_config.outputFile.end(); opit++) {
      const std::string & portName = (*opit).first;
      const std::string & fileName = (*opit).second;

      if (m_verbose) {
        std::cout << "Connecting output port \"" << portName << "\" to file \"" << fileName << "\" ... " << std::flush;
      }

      unsigned int portOrdinal;

      findPort (portName.c_str(), portOrdinal);

      PortMap::iterator pit = m_portMap.find (portName);
      PortData & pd = (*pit).second;

      if (pd.provider) {
        std::string msg = "Port \"";
        msg += portName;
        msg += "\" is not an output port";
        throw msg;
      }

      /*
       * Instantiate file output worker.
       */

      pd.fileIoWorkerId = CREATE_WORKER(m_appContext, &TestWorkerFileSinkWorker);

      /*
       * Connect file input worker to worker port.
       */

      pd.fileIoPort = & pd.fileIoWorkerId->createInputPort (
                                                         0,
                                                         m_numBuffers,
                                                         m_bufferSize, NULL);


      std::string shadowPort, initialProviderInfo;
      pd.fileIoPort->getInitialProviderInfo(NULL, initialProviderInfo);
      pd.localPort->setFinalProviderInfo(initialProviderInfo, shadowPort );
      pd.fileIoPort->setFinalUserInfo ( shadowPort );

      pd.connected = true;
      pd.fileName = fileName;

      /*
       * Configure file output worker.
       */

      ocpiAssert (fileName.length()+1 <= 256);
      pd.fileIoWorkerId->write (offsetof (FileSinkProperties, fileName),
                                 fileName.length() + 1,
                                 fileName.c_str());

      ocpiAssert (portName.length()+1 <= 256);
      pd.fileIoWorkerId->write (
                                 offsetof (FileSinkProperties, portName),
                                 portName.length() + 1,
                                 portName.c_str());

      uint8_t data = m_verbose ? 1 : 0;
      pd.fileIoWorkerId->write (
                                 offsetof (FileSinkProperties, verbose),
                                 1, &data);

      if (m_verbose) {
        std::cout << "done." << std::endl;
      }
    }
  }
  catch (const OCPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error connectint input ports: error code ";
    msg += OCPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

    if (auxInfo && *auxInfo) {
      msg += ": ";
      msg += auxInfo;
    }

    throw msg;
  }
}

/*
 * Configure worker
 */

void
TestWorker::
configureWorker ()
  throw (std::string)
{
  if (m_verbose) {
    std::cout << "Configuring worker ... " << std::flush;
  }

  bool needSync = false;

  for (OCPI::Util::CommandLineConfiguration::MultiNameValue::const_iterator propit = m_config.property.begin();
       propit != m_config.property.end(); propit++) {
    const std::string & name = (*propit).first;
    const std::string & value = (*propit).second;

    unsigned int n = findProperty (name.c_str());
    const OCPI::SCA::Property & p = m_props[n];

    if (p.read_sync) {
      needSync = true;
    }

    if (!p.is_test && !p.is_writable) {
      throw std::string ("Property is not writable");
    }

    if (p.is_struct) {
      throw std::string ("Structures not supported yet");
    }

    ocpiAssert (p.num_members == 1);
    OCPI::SCA::SimpleType & pt = p.types[0];

    if (!p.is_sequence) {
      switch (pt.data_type) {
      case OCPI::SCA::SCA_boolean:
        {
          uint8_t data;

          if (value == "true" || value == "TRUE" || value == "1") {
            data = 1;
          }
          else if (value == "false" || value == "FALSE" || value == "0") {
            data = 0;
          }
          else {
            throw std::string ("Failed to extract value of type boolean");
          }

          m_containerWorkerId->write ( p.offset,
                                         1, &data);
        }
        break;

      case OCPI::SCA::SCA_short:
        {
          int idata = OCPI::Util::Misc::stringToInteger (value);
          int16_t data = static_cast<int16_t> (idata);

          m_containerWorkerId->write ( p.offset,
                                         2, &data);
        }
        break;

      case OCPI::SCA::SCA_long:
        {
          int idata = OCPI::Util::Misc::stringToInteger (value);
          int32_t data = static_cast<int32_t> (idata);

          m_containerWorkerId->write (  p.offset,
                                         4, &data);
        }
        break;

      case OCPI::SCA::SCA_ulong:
        {
          unsigned int idata = OCPI::Util::Misc::stringToUnsigned (value);
          uint32_t data = static_cast<uint32_t> (idata);

          m_containerWorkerId->write (  p.offset,
                                         4, &data);
        }
        break;

      case OCPI::SCA::SCA_ushort:
        {
          unsigned int idata = OCPI::Util::Misc::stringToUnsigned (value);
          uint16_t data = static_cast<uint16_t> (idata);

          m_containerWorkerId->write ( p.offset,
                                         2, &data);
        }
        break;

      case OCPI::SCA::SCA_string:
        {
          unsigned int len = value.length () + 1;

          if (len > pt.size + 1) {
            throw std::string ("String exceeds maximum length");
          }

          m_containerWorkerId->write ( p.offset,
                                         len, value.c_str ());
        }
        break;

      default:
        throw std::string ("type not supported yet");
      }
    }
    else {
      throw std::string ("type not supported yet");
    }
  }

  if (needSync) {
    try { m_containerWorkerId->afterConfigure(); }
    catch (...) {
      throw std::string ("Failed to configure worker");
    }
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }
}

/*
 * Configure worker
 */

void
TestWorker::
printWorkerProperties ()
  throw ()
{
  if (m_verbose) {
    std::cout << "Printing worker properties:" << std::endl;
  }

  bool haveSync = false;

  for (unsigned int n=0; n<m_nprops; n++) {
    const OCPI::SCA::Property & p = m_props[n];

    if (p.write_sync && !haveSync) {
      try { m_containerWorkerId->beforeQuery(); }
      catch (...) {
        throw std::string ("Failed to query worker");
      }

      haveSync = true;
    }

    std::cout << p.name << ": ";

    if (p.is_struct) {
      std::cout << "{structs not supported yet}" << std::endl;
      continue;
    }

    ocpiAssert (p.num_members == 1);
    OCPI::SCA::SimpleType & pt = p.types[0];

    try {
      if (!p.is_sequence) {

#define PRINTPROPERTY(type,wcitype,printitem) do {        \
          type data;                                        \
          m_containerWorkerId->read ( \
                                        p.offset,        \
                                        sizeof(type),        \
                                        &data);                \
          std::cout << (printitem);                        \
        } while (0)

        switch (pt.data_type) {
        case OCPI::SCA::SCA_boolean:
          PRINTPROPERTY(uint8_t, WCI_DATA_TYPE_U8, (data ? "true" : "false"));
          break;

        case OCPI::SCA::SCA_char:
          PRINTPROPERTY(uint8_t, WCI_DATA_TYPE_U8, ((int) data));
          break;

        case OCPI::SCA::SCA_double:
          PRINTPROPERTY(double, WCI_DATA_TYPE_F64, data);
          break;

        case OCPI::SCA::SCA_float:
          PRINTPROPERTY(float, WCI_DATA_TYPE_F32, data);
          break;

        case OCPI::SCA::SCA_short:
          PRINTPROPERTY(uint16_t, WCI_DATA_TYPE_U16, static_cast<OCPI::OS::int16_t> (data));
          break;

        case OCPI::SCA::SCA_long:
          PRINTPROPERTY(uint32_t, WCI_DATA_TYPE_U32, static_cast<OCPI::OS::int32_t> (data));
          break;

        case OCPI::SCA::SCA_octet:
          PRINTPROPERTY(uint8_t, WCI_DATA_TYPE_U8, data);
          break;

        case OCPI::SCA::SCA_ulong:
          PRINTPROPERTY(uint32_t, WCI_DATA_TYPE_U32, data);
          break;

        case OCPI::SCA::SCA_ushort:
          PRINTPROPERTY(uint16_t, WCI_DATA_TYPE_U16, data);
          break;

        case OCPI::SCA::SCA_string:
          {
            char * buf = new char[pt.size+1];

            m_containerWorkerId->read ( p.offset,
                                          pt.size, buf);

            buf[pt.size] = '\0';
            std::cout << buf;
            delete [] buf;
          }
          break;

        default:
          ocpiAssert (0);
        }

#undef PRINTPROPERTY

      }
      else {
        uint32_t length;

        try { m_containerWorkerId->read ( p.offset, 4, &length); }
	catch (...) {{
          std::cout << "{Failed to read sequence length}";
          continue;
        }

        if (length > p.sequence_size) {
          std::cout << "{Sequence length (" << length
                    << ") exceeds maximum length (" << p.sequence_size
                    << ")}";
          continue;
        }

#define PRINTSEQ(printitem) do {                \
          for (unsigned i=0; i<length; i++) {        \
            if (i) {                                \
              std::cout << ", ";                \
            }                                        \
            std::cout << printitem;                \
          }                                        \
        } while (0)

#define PRINTPROPERTY(type,wcitype,printitem) do {                \
          type * data = new type [length];                        \
          m_containerWorkerId->read (                \
                                        p.data_offset,                \
                                        sizeof(type)*length,        \
                                        data);                        \
          PRINTSEQ(printitem);                                        \
          delete [] data;                                        \
        } while (0)

        switch (pt.data_type) {
        case OCPI::SCA::SCA_boolean:
          PRINTPROPERTY(uint8_t, WCI_DATA_TYPE_U8, (data[i] ? "true" : "false"));
          break;

        case OCPI::SCA::SCA_char:
        case OCPI::SCA::SCA_octet:
          {
            uint8_t * data = new uint8_t [length];
            m_containerWorkerId->read ( p.data_offset,
                                          length, data);
            std::cout << std::endl;
            dumpOctets (data, length);
            delete [] data;
          }
          break;

        case OCPI::SCA::SCA_double:
          PRINTPROPERTY(double, WCI_DATA_TYPE_F64, data[i]);
          break;

        case OCPI::SCA::SCA_float:
          PRINTPROPERTY(float, WCI_DATA_TYPE_F32, data[i]);
          break;

        case OCPI::SCA::SCA_short:
          PRINTPROPERTY(uint16_t, WCI_DATA_TYPE_U16, static_cast<OCPI::OS::int16_t> (data[i]));
          break;

        case OCPI::SCA::SCA_long:
          PRINTPROPERTY(int32_t, WCI_DATA_TYPE_U32, static_cast<OCPI::OS::int32_t> (data[i]));
          break;

        case OCPI::SCA::SCA_ulong:
          PRINTPROPERTY(uint32_t, WCI_DATA_TYPE_U32, data[i]);
          break;

        case OCPI::SCA::SCA_ushort:
          PRINTPROPERTY(uint16_t, WCI_DATA_TYPE_U16, data[i]);
          break;

        case OCPI::SCA::SCA_string:
          std::cout << "{String sequences not implemented yet}";
          break;

        default:
          ocpiAssert (0);
        }

#undef PRINTPROPERTY
#undef PRINTSEQ

      }

      std::cout << std::endl;
    }
    }
    catch (const std::bad_alloc & oops) {
      std::cout << "{" << oops.what() << "}" << std::endl;
    }
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }
}

/*
 * Helper.
 */

unsigned int
TestWorker::
findProperty (const char * name)
  throw (std::string)
{
  for (unsigned int n=0; n<m_nprops; n++) {
    if (std::strcmp (m_props[n].name, name) == 0) {
      return n;
    }
  }

  std::string msg = "Unknown property: \"";
  msg += name;
  msg += "\"";
  throw msg;
}

const OCPI::SCA::Port *
TestWorker::
findPort (const char * name, unsigned int & portOrdinal)
  throw (std::string)
{
  OCPI::SCA::Port * p = m_ports;

  for (unsigned int n=0; n<m_nports; n++, p++) {
    if (std::strcmp (p->name, name) == 0) {
      portOrdinal = n;
      return p;
    }
  }

  std::string msg = "Unknown port: \"";
  msg += name;
  msg += "\"";
  throw msg;
}

/*
 * DLL handling.
 */

void
TestWorker::
loadDll (OCPI::Util::Vfs::Vfs & fs,
         const std::string & dllFileName)
  throw (std::string)
{
  // Assumes that a lock is held.
  std::string normalName = normalizeFileName (dllFileName);
  LoadedDllInfos::iterator ldi = m_loadedDlls.find (normalName);

  if (ldi != m_loadedDlls.end()) {
    LoadedDllInfo & dllInfo = (*ldi).second;
    dllInfo.referenceCount++;
    return;
  }

  OCPI::Util::FileFs::FileFs * ffs = dynamic_cast<OCPI::Util::FileFs::FileFs *> (&fs);

  if (m_verbose) {
    std::cout << "Loading DLL \"" << normalName << "\" ... " << std::flush;
  }

  if (ffs) {
    OCPI::OS::LoadableModule * lm;
    std::string nativeName;

    try {
      nativeName = ffs->toNativeName (dllFileName);
      lm = new OCPI::OS::LoadableModule (nativeName);

      LoadedDllInfo & dllInfo = m_loadedDlls[normalName];
      dllInfo.referenceCount = 1;
      dllInfo.lm = lm;
      dllInfo.module = 0;
    }
    catch (...) {
      // try again below.
      ffs = 0;
    }
  }

  if (!ffs) {
    OCPI::Util::LoadableModule * lm;

    try {
      lm = new OCPI::Util::LoadableModule (&fs, dllFileName);
    }
    catch (const std::bad_alloc & oops) {
      throw std::string (oops.what());
    }

    LoadedDllInfo & dllInfo = m_loadedDlls[normalName];
    dllInfo.referenceCount = 1;
    dllInfo.lm = 0;
    dllInfo.module = lm;
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }
}

void
TestWorker::
unloadDll (const std::string & dllFileName)
  throw ()
{
  // Assumes that a lock is held.
  std::string normalName = normalizeFileName (dllFileName);
  LoadedDllInfos::iterator ldi = m_loadedDlls.find (normalName);
  ocpiAssert (ldi != m_loadedDlls.end());
  LoadedDllInfo & dllInfo = (*ldi).second;

  if (--dllInfo.referenceCount) {
    return;
  }

  /*
   * Unload the DLL.
   */

  if (m_verbose) {
    std::cout << "Unloading DLL \"" << normalName << "\" ... " << std::flush;
  }

  try {
    if (dllInfo.lm) {
      dllInfo.lm->close ();
    }
    else {
      dllInfo.module->close ();
    }
  }
  catch (const std::string & oops) {
  }

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  delete dllInfo.lm;
  delete dllInfo.module;
  m_loadedDlls.erase (ldi);
}

std::string
TestWorker::
normalizeFileName (const std::string & fileName)
  throw ()
{
  /*
   * JTAP only sends the last path component of the file name (i.e., it
   * calls load ("/path/name") but then calls execute ("name")).  Let's
   * normalize file names and only consider the last path component.
   */

  try {
    return OCPI::Util::Vfs::relativeName (fileName);
  }
  catch (...) {
  }

  return fileName;
}

void
TestWorker::
dumpOctets (const unsigned char * data, unsigned int length)
  throw ()
{
  static const char * hex = "0123456789abcdef";
  unsigned int i;

  for (unsigned int index=0; index<length; index+=16, data+=16) {
    std::cout << "    ";

    for (i=0; i<16 && index+i<length; i++) {
      std::cout << hex[data[i] >> 4] << hex[data[i] & 15] << ' ';
    }

    for (; i<16; i++) {
      std::cout << "   ";
    }

    std::cout << "   ";

    for (i=0; i<16 && index+i<length; i++) {
      if (std::isprint(data[i])) {
        std::cout << data[i];
      }
      else {
        std::cout << '.';
      }
    }

    std::cout << std::endl;
  }
}

/*
 * ----------------------------------------------------------------------
 * Command line handling.
 * ----------------------------------------------------------------------
 */

namespace {
  std::string g_tempFileLocation;

  std::string
  makeTempFileLocation ()
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

    std::string relName = "ocpi-rcc-test";
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

static
int
startTestWorkerCmdInt (int & argc, char * argv[])
{
  TestWorkerCommandLineConfigurator config;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Oops: " << oops << std::endl;
    return 1;
  }

  if (config.help) {
    printUsage (config, argv[0]);
    return 0;
  }

  if (!config.workerFile.length()) {
    std::cerr << "Error: no --workerFile specified!" << std::endl;
    return 1;
  }

  TestWorker testWorker (config);

  try {
    testWorker.runTest ();
  }
  catch (const std::string & ex) {
    std::cerr << "Oops: " << ex << "." << std::endl;
    return 1;
  }

  return 0;
}

extern "C" {
  /*
   * Entrypoint for the VxWorks command line.
   */

  int
  startTestWorker (int argc, char *argv[])
  {
    return startTestWorkerCmdInt (argc, argv);
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

  bool haveDllDir = false;

  for (int cdi=1; cdi<argc; cdi++) {
    if (std::strcmp (argv[cdi], "--dllDir") == 0) {
      g_tempFileLocation = argv[cdi+1];
      haveDllDir = true;
      break;
    }
    else if (std::strncmp (argv[cdi], "--dllDir=", 9) == 0) {
      g_tempFileLocation = argv[cdi] + 9;
      haveDllDir = true;
      break;
    }
  }

  if (!haveDllDir) {
    try {
      g_tempFileLocation = makeTempFileLocation ();
    }
    catch (const std::string & oops) {
      std::cerr << "Oops: " << oops << std::endl;
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
      if (haveDllDir) {
        std::cerr << "Oops: --dllDir not found in LD_LIBRARY_PATH" << std::endl;
        return 1;
      }

      std::string newLdLibraryPath = g_tempFileLocation;

      if (oldLdLibraryPath) {
        newLdLibraryPath += ':';
        newLdLibraryPath += oldLdLibraryPath;
      }

      setenv ("LD_LIBRARY_PATH", newLdLibraryPath.c_str(), 1);
      execv (argv[0], argv);
    }
#endif
  }

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
      pid = OCPI::Util::Misc::stringToUnsigned (strPid);
    }
    catch (...) {
      pid = 0;
    }

    if (pid && (kill (pid, 0) == 0 || errno == EPERM)) {
      std::cerr << "Oops: OCPI RCC Tester is already running, pid "
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
    std::cerr << "Oops: Failed to write PID to \""
              << nativePidFileName
              << "\"." << std::endl;
    std::cerr << "      Please delete \""
              << g_tempFileLocation
              << "\" before continuing."
              << std::endl;
    return 1;
  }

  oPidFile.close ();

  OCPI::Util::LoadableModule::setTemporaryFileLocation (g_tempFileLocation);
#endif

  int result = startTestWorkerCmdInt (argc, argv);

#if ! defined (__VXWORKS__)
  try {
    OCPI::OS::FileSystem::remove (pidFileName);
  }
  catch (...) {
  }

  if (!haveDllDir) {
    cleanTempFileLocation (g_tempFileLocation);
  }
#endif

  return result;
}
