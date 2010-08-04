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

/*
 * RCC Worker Test Harness.
 *
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  Set default for --polled to true to match the default
 *                  "cpi-smb-pio" endpoint.  Can be overridden using
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
#include <CpiOsMisc.h>
#include <CpiOsDebug.h>
#include <CpiOsAssert.h>
#include <CpiOsTimer.h>
#include <CpiOsDataTypes.h>
#include <CpiOsFileSystem.h>
#include <CpiOsFileIterator.h>
#include <CpiOsThreadManager.h>
#include <CpiUtilVfs.h>
#include <CpiUtilMisc.h>
#include <CpiUtilFileFs.h>
#include <CpiUtilZipFs.h>
#include <CpiUtilEzxml.h>
#include <CpiUtilLoadableModule.h>
#include <CpiUtilCommandLineConfiguration.h>
#include <CpiContainerInterface.h>
#include <CpiContainerPort.h>
#include <CpiWorker.h>
#include <CpiWciWorker.h>
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
    CPI_TEST_WORKER_DEFAULT_NUM_BUFFERS = 2,
    CPI_TEST_WORKER_DEFAULT_BUFFER_SIZE = 1 << 20,
    CPI_TEST_WORKER_DEFAULT_ENDPOINT_SIZE = 1 << 26
  };
}

class TestWorkerCommandLineConfigurator
  : public CPI::Util::CommandLineConfiguration
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
  : CPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    numBuffers (CPI_TEST_WORKER_DEFAULT_NUM_BUFFERS),
    bufferSize (CPI_TEST_WORKER_DEFAULT_BUFFER_SIZE),
    endpointSize (CPI_TEST_WORKER_DEFAULT_ENDPOINT_SIZE),
    polled (true),
    defaultPacketSize (0),
    pauseBeforeStart (false),
    timeout (0),
    slackTime (3),
    printProperties (false)
{
}

CPI::Util::CommandLineConfiguration::Option
TestWorkerCommandLineConfigurator::g_options[] = {
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::verbose) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Endpoint (overrides endpoint size)",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::endpoint) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "polled", "Run the container in polling mode",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::polled) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "numBuffers", "Number of buffers per port (2)",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::numBuffers) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "bufferSize", "Buffer size (1M)",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::bufferSize) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "endpointSize", "Endpoint size (64M)",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::endpointSize) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "workerFile", "Worker executable file",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::workerFile) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "entryPoint", "Worker entrypoint",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::entryPoint) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "defaultPacketSize", "Default packet size for input ports",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::defaultPacketSize) },
  { CPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "inputFile", "Connect input file to input port",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::inputFile) },
  { CPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "outputFile", "Connect output file to output port",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::outputFile) },
  { CPI::Util::CommandLineConfiguration::OptionType::MULTINAMEVALUE,
    "configure", "Worker property configuration",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::property) },
  { CPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
    "loadDll", "Load this DLL before loading the worker",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::loadDll) },
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "dllDir", "Use this directory for linking DLL files",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::dllDir) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "pauseBeforeStart", "Pause before start",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::pauseBeforeStart) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "timeout", "Seconds to wait for input ports to drain",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::timeout) },
  { CPI::Util::CommandLineConfiguration::OptionType::UNSIGNEDLONG,
    "slackTime", "Seconds to wait for things to settle",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::slackTime) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "printProperties", "Print properties before and after run",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::printProperties) },
#if !defined (NDEBUG)
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::debugBreak) },
#endif
  { CPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    CPI_CLC_OPT(&TestWorkerCommandLineConfigurator::help) },
  { CPI::Util::CommandLineConfiguration::OptionType::END }
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
 * Dispatch thread helper.
 * ----------------------------------------------------------------------
 */

namespace {

  struct DispatchThreadData {
    CPI::Container::Interface * container;
    DataTransfer::EventManager * eventManager;
  };

  void
  dispatchThread (void * opaque)
  {
    DispatchThreadData * dtd = static_cast<DispatchThreadData *> (opaque);
    CPI::Container::Interface::DispatchRetCode rc;
    bool keepWorking = true;

    while (keepWorking) {
      rc = dtd->container->dispatch (dtd->eventManager);

      switch (rc) {
      case CPI::Container::Interface::DispatchNoMore:
        // All done, exit from dispatch thread.
        keepWorking = false;
        break;

      case CPI::Container::Interface::MoreWorkNeeded:
        // No-op. To prevent blocking the CPU, yield.
        CPI::OS::sleep (0);
        break;

      case CPI::Container::Interface::Stopped:
        // Exit from dispatch thread, it will be restarted.
        keepWorking = false;
        break;

      case CPI::Container::Interface::Spin:
        /*
         * If we have an event manager, ask it to go to sleep and wait for
         * an event.  If we are not event driven, the event manager will
         * tell us that it is spinning.  In that case, yield to give other
         * threads a chance to run.
         */

        if (dtd->eventManager) {
          if (dtd->eventManager->waitForEvent (0) == DataTransfer::EventSpin) {
            CPI::OS::sleep (0);
          }
        }
        else {
          CPI::OS::sleep (0);
        }
        break;
      }
    }

    delete dtd;
  }

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

  const CPI::SCA::Port * findPort (const char * name, unsigned int & portOrdinal)
    throw (std::string);

  void loadDll (CPI::Util::Vfs::Vfs & fs,
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
    CPI::Container::Port * localPort;

    bool connected;
    std::string fileName;
    CPI::Container::WorkerId fileIoWorkerId;
    CPI::Container::Port * fileIoPort;
  };

  typedef std::map<std::string, PortData> PortMap;

  /*
   * Information about loaded DLLs.
   */

  typedef std::vector<std::string> Strings;

  struct LoadedDllInfo {
    unsigned int referenceCount;
    CPI::OS::LoadableModule * lm;
    CPI::Util::LoadableModule * module;
  };

  typedef std::map<std::string, LoadedDllInfo> LoadedDllInfos;

private:
  const TestWorkerCommandLineConfigurator & m_config;

  CPI::Container::Interface * m_container;

  CPI::Container::Application * m_appContext;
  DataTransfer::EventManager * m_eventManager;
  CPI::OS::ThreadManager * m_dispatchThreadMgr;

  CPI::Container::WorkerId m_containerWorkerId;

  std::string workerProperties;
  LoadedDllInfos m_loadedDlls;
  PortMap m_portMap;

  unsigned int m_nprops;
  unsigned int m_nports;
  unsigned int m_ntests;
  unsigned int m_sizeOfPropertySpace;
  CPI::SCA::Property * m_props;
  CPI::SCA::Port * m_ports;
  CPI::SCA::Test * m_tests;

  unsigned int m_numBuffers;
  unsigned int m_bufferSize;
  bool m_verbose;

protected:
  static const std::string s_metaFileName;
};

const std::string
TestWorker::
s_metaFileName = "/cpi-meta.inf";

TestWorker::
TestWorker (const TestWorkerCommandLineConfigurator & config)
  throw ()
  : m_config (config),
    m_container (0),
    m_appContext (0),
    m_eventManager (0),
    m_dispatchThreadMgr (0),
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
    if (m_eventManager) {
      m_container->stop (m_eventManager);
    }
  }
  catch (...) {
  }

  if (m_dispatchThreadMgr) {
    m_dispatchThreadMgr->join ();
    delete m_dispatchThreadMgr;
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


static CPI::Util::DriverManager dm("Container");

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
    endpoint  = "cpi-smb-pio://cpi-rcc-test:";
    endpoint += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (m_config.endpointSize));
    endpoint += ".1.1";
  }

  if (m_verbose) {
    std::cout << "Creating RCC container ... " << std::flush;
  }

  try {
    dm.discoverDevices(0,0);
    CPI::Util::PValue cprops[] = {CPI::Util::PVString("endpoint",(char*)endpoint.c_str() ),
                                  CPI::Util::PVBool("polling",m_config.polled),
                                  CPI::Util::PVEnd };
    CPI::Util::Device* d = dm.getDevice( cprops, "RCC" );
    m_container = static_cast<CPI::Container::Interface*>(d);
    m_eventManager = m_container->getEventManager ();
    m_appContext = m_container->createApplication ();
  }
  catch (const CPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error creating RCC container: error code ";
    msg += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

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

  CPI::Util::FileFs::FileFs fileFs ("/");
  std::string workerFile = fileFs.fromNativeName (m_config.workerFile);
  CPI::Util::ZipFs::ZipFs zfs (&fileFs, workerFile, std::ios_base::in,
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

  CPI::Util::EzXml::Doc metaDoc (zfs, s_metaFileName);
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

  for (CPI::Util::CommandLineConfiguration::MultiString::const_iterator dllit = m_config.loadDll.begin();
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
  cpiAssert (ldi != m_loadedDlls.end());
  LoadedDllInfo & loadedDll = (*ldi).second;
  cpiAssert (loadedDll.module);

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

  cpiAssert (epPtr);

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
    m_containerWorkerId = & m_appContext->createWorker (NULL, NULL, (char*)epPtr);
  }
  catch (const CPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error loading worker into container: error code ";
    msg += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

    if (auxInfo && *auxInfo) {
      msg += ": ";
      msg += auxInfo;
    }

    throw msg;
  }

  if (CPI::SCA::decode_props (workerProperties.c_str(),
                              &m_props, &m_nprops, &m_sizeOfPropertySpace,
                              &m_ports, &m_nports,
                              &m_tests, &m_ntests)) {
    throw std::string ("Invalid property string");
  }

  createAllPorts ();

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  if (m_verbose) {
    std::cout << "Initializing worker ... " << std::flush;
  }

  m_containerWorkerId->control ( WCI_CONTROL_INITIALIZE, WCI_DEFAULT);

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
                << CPI::OS::getProcessId()
                << " to continue ... " << std::flush;
    }

    g_expectSignal = true;
    CPI::OS::setCtrlCHandler (testerSignalHandler);

#if defined (__linux__)
    pause ();
#else
    CPI::OS::debugBreak ();
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

  m_container->start (m_eventManager);

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Start the dispatch thread.
   */

  if (m_verbose) {
    std::cout << "Starting the dispatch thread ... " << std::flush;
  }

  DispatchThreadData * dtd = new DispatchThreadData;
  dtd->container = m_container;
  dtd->eventManager = m_eventManager;
  m_dispatchThreadMgr = new CPI::OS::ThreadManager;
  m_dispatchThreadMgr->start (dispatchThread, dtd);

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Start all file i/o workers.
   */

  for (PortMap::iterator spit = m_portMap.begin(); spit != m_portMap.end(); spit++) {
    PortData & pd = (*spit).second;

    if (pd.connected) {
      if ( pd.fileIoWorkerId->control ( WCI_CONTROL_START, WCI_DEFAULT) != WCI_SUCCESS) {
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

  if ( m_containerWorkerId->control ( WCI_CONTROL_START, WCI_DEFAULT) != WCI_SUCCESS) {
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
  CPI::OS::Timer timer (true);

  while (!allInputWorkersAtEof) {
    CPI::OS::sleep (100);

    allInputWorkersAtEof = true;

    for (PortMap::iterator spit = m_portMap.begin(); spit != m_portMap.end(); spit++) {
      PortData & pd = (*spit).second;

      if (pd.connected && pd.provider) {
        WCI_u8 atEof;

        pd.fileIoWorkerId->read (
                                  offsetof (FileSourceProperties, atEof),
                                  1, WCI_DATA_TYPE_U8,
                                  WCI_DEFAULT, &atEof);

        if (!atEof) {
          allInputWorkersAtEof = false;
          break;
        }
      }
    }

    CPI::OS::Timer::ElapsedTime et;
    timer.stop ();
    timer.getValue (et);
    timer.start ();

    if (m_config.timeout && et.seconds >= static_cast<unsigned int> (m_config.timeout)) {
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
    CPI::OS::sleep (1000);
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

  if (  m_containerWorkerId->control (  WCI_CONTROL_STOP, WCI_DEFAULT) != WCI_SUCCESS) {
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
      if ( pd.fileIoWorkerId->control (WCI_CONTROL_STOP, WCI_DEFAULT) != WCI_SUCCESS) {
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

  m_container->stop (m_eventManager);

  if (m_verbose) {
    std::cout << "done." << std::endl;
  }

  /*
   * Stop the dispatch thread.
   */

  if (m_verbose) {
    std::cout << "Stopping the dispatch thread ... " << std::flush;
  }

  m_dispatchThreadMgr->join ();
  delete m_dispatchThreadMgr;
  m_dispatchThreadMgr = 0;

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
  const CPI::SCA::Port * portInfo = m_ports;
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
                                                               m_bufferSize);
      }
      else {
        pd.localPort = & m_containerWorkerId->createOutputPort (
                                                          portOrdinal,
                                                          m_numBuffers,
                                                          m_bufferSize);
      }
    }
    catch (const CPI::Util::EmbeddedException & oops) {
      const char * auxInfo = oops.getAuxInfo ();

      std::string msg = "Failed to create ";
      msg += (portInfo->provider ? "target" : "source");
      msg += " port \"";
      msg += portInfo->name;
      msg += "\": ";
      msg += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

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
    for (CPI::Util::CommandLineConfiguration::MultiNameValue::const_iterator ipit = m_config.inputFile.begin();
         ipit != m_config.inputFile.end(); ipit++) {
      const std::string & portName = (*ipit).first;
      std::string fileName = (*ipit).second;
      std::string::size_type colonPos = fileName.find (':');
      uint32_t packetSize = static_cast<uint32_t> (m_config.defaultPacketSize);

      if (colonPos != std::string::npos) {
        packetSize = CPI::Util::Misc::stringToUnsigned (fileName.substr (colonPos+1));
        fileName = fileName.substr (0, colonPos);
      }

      if (m_verbose) {
        std::cout << "Connecting file \"" << fileName << "\" to input port \"" << portName << "\" ... " << std::flush;
      }

      const CPI::SCA::Port * portInfo;
      unsigned int portOrdinal;

      portInfo = findPort (portName.c_str(), portOrdinal);

      PortMap::iterator pit = m_portMap.find (portName);
      PortData & pd = (*pit).second;
      std::string shadowPort;

      if (!pd.provider) {
        std::string msg = "Port \"";
        msg += portName;
        msg += "\" is not an input port";
        throw msg;
      }

      /*
       * Instantiate file input worker.
       */

      pd.fileIoWorkerId = & m_appContext->createWorker ( NULL, NULL, (char*)&TestWorkerFileSourceWorker);

      /*
       * Connect file input worker to worker port.
       */

      pd.fileIoPort = & pd.fileIoWorkerId->createOutputPort ( 0,
                                                         m_numBuffers,
                                                         m_bufferSize);

      shadowPort = pd.fileIoPort->setFinalProviderInfo( pd.localPort->getInitialProviderInfo() );
      pd.localPort->setFinalUserInfo ( shadowPort);

      pd.connected = true;
      pd.fileName = fileName;

      /*
       * Configure file input worker.
       */

      pd.fileIoWorkerId->control ( WCI_CONTROL_INITIALIZE, WCI_DEFAULT);

      cpiAssert (fileName.length()+1 <= 256);
      pd.fileIoWorkerId->write (
                                 offsetof (FileSourceProperties, fileName),
                                 fileName.length() + 1,
                                 WCI_DATA_TYPE_U8,
                                 WCI_DEFAULT,
                                 fileName.c_str());

      cpiAssert (portName.length()+1 <= 256);
      pd.fileIoWorkerId->write (
                                 offsetof (FileSourceProperties, portName),
                                 portName.length() + 1,
                                 WCI_DATA_TYPE_U8,
                                 WCI_DEFAULT,
                                 portName.c_str());

      WCI_u8 data = m_verbose ? 1 : 0;
      pd.fileIoWorkerId->write (
                                 offsetof (FileSourceProperties, verbose),
                                 1, WCI_DATA_TYPE_U8,
                                 WCI_DEFAULT, &data);

      if (packetSize) {
        pd.fileIoWorkerId->write (
                                   offsetof (FileSourceProperties, bytesPerPacket),
                                   4, WCI_DATA_TYPE_U32,
                                   WCI_DEFAULT, &packetSize);
      }

      if (m_verbose) {
        std::cout << "done." << std::endl;
      }
    }
  }
  catch (const CPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error connectint input ports: error code ";
    msg += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

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
    for (CPI::Util::CommandLineConfiguration::MultiNameValue::const_iterator opit = m_config.outputFile.begin();
         opit != m_config.outputFile.end(); opit++) {
      const std::string & portName = (*opit).first;
      const std::string & fileName = (*opit).second;

      if (m_verbose) {
        std::cout << "Connecting output port \"" << portName << "\" to file \"" << fileName << "\" ... " << std::flush;
      }

      const CPI::SCA::Port * portInfo;
      unsigned int portOrdinal;

      portInfo = findPort (portName.c_str(), portOrdinal);

      PortMap::iterator pit = m_portMap.find (portName);
      PortData & pd = (*pit).second;
      std::string shadowPort;

      if (pd.provider) {
        std::string msg = "Port \"";
        msg += portName;
        msg += "\" is not an output port";
        throw msg;
      }

      /*
       * Instantiate file output worker.
       */

      pd.fileIoWorkerId = & m_appContext->createWorker (NULL, NULL, (char*)&TestWorkerFileSinkWorker);

      /*
       * Connect file input worker to worker port.
       */

      pd.fileIoPort = & pd.fileIoWorkerId->createInputPort (
                                                         0,
                                                         m_numBuffers,
                                                         m_bufferSize);


      shadowPort = pd.localPort->setFinalProviderInfo( pd.fileIoPort->getInitialProviderInfo() );
      pd.fileIoPort->setFinalUserInfo ( shadowPort );

      pd.connected = true;
      pd.fileName = fileName;

      /*
       * Configure file output worker.
       */

      pd.fileIoWorkerId->control (WCI_CONTROL_INITIALIZE, WCI_DEFAULT);

      cpiAssert (fileName.length()+1 <= 256);
      pd.fileIoWorkerId->write (offsetof (FileSinkProperties, fileName),
                                 fileName.length() + 1,
                                 WCI_DATA_TYPE_U8,
                                 WCI_DEFAULT,
                                 fileName.c_str());

      cpiAssert (portName.length()+1 <= 256);
      pd.fileIoWorkerId->write (
                                 offsetof (FileSinkProperties, portName),
                                 portName.length() + 1,
                                 WCI_DATA_TYPE_U8,
                                 WCI_DEFAULT,
                                 portName.c_str());

      WCI_u8 data = m_verbose ? 1 : 0;
      pd.fileIoWorkerId->write (
                                 offsetof (FileSinkProperties, verbose),
                                 1, WCI_DATA_TYPE_U8,
                                 WCI_DEFAULT, &data);

      if (m_verbose) {
        std::cout << "done." << std::endl;
      }
    }
  }
  catch (const CPI::Util::EmbeddedException & oops) {
    const char * auxInfo = oops.getAuxInfo ();
    std::string msg = "Error connectint input ports: error code ";
    msg += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

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

  for (CPI::Util::CommandLineConfiguration::MultiNameValue::const_iterator propit = m_config.property.begin();
       propit != m_config.property.end(); propit++) {
    const std::string & name = (*propit).first;
    const std::string & value = (*propit).second;

    unsigned int n = findProperty (name.c_str());
    const CPI::SCA::Property & p = m_props[n];

    if (p.read_sync) {
      needSync = true;
    }

    if (!p.is_test && !p.is_writable) {
      throw std::string ("Property is not writable");
    }

    if (p.is_struct) {
      throw std::string ("Structures not supported yet");
    }

    cpiAssert (p.num_members == 1);
    CPI::SCA::SimpleType & pt = p.types[0];
    WCI_error e = WCI_SUCCESS;

    if (!p.is_sequence) {
      switch (pt.data_type) {
      case CPI::SCA::SCA_boolean:
        {
          WCI_u8 data;

          if (value == "true" || value == "TRUE" || value == "1") {
            data = 1;
          }
          else if (value == "false" || value == "FALSE" || value == "0") {
            data = 0;
          }
          else {
            throw std::string ("Failed to extract value of type boolean");
          }

          e = m_containerWorkerId->write ( p.offset,
                                         1, WCI_DATA_TYPE_U8,
                                         WCI_DEFAULT, &data);
        }
        break;

      case CPI::SCA::SCA_short:
        {
          int idata = CPI::Util::Misc::stringToInteger (value);
          int16_t data = static_cast<int16_t> (idata);

          e = m_containerWorkerId->write ( p.offset,
                                         2, WCI_DATA_TYPE_U16,
                                         WCI_DEFAULT, &data);
        }
        break;

      case CPI::SCA::SCA_long:
        {
          int idata = CPI::Util::Misc::stringToInteger (value);
          int32_t data = static_cast<int32_t> (idata);

          e = m_containerWorkerId->write (  p.offset,
                                         4, WCI_DATA_TYPE_U32,
                                         WCI_DEFAULT, &data);
        }
        break;

      case CPI::SCA::SCA_ulong:
        {
          unsigned int idata = CPI::Util::Misc::stringToUnsigned (value);
          uint32_t data = static_cast<uint32_t> (idata);

          e = m_containerWorkerId->write (  p.offset,
                                         4, WCI_DATA_TYPE_U32,
                                         WCI_DEFAULT, &data);
        }
        break;

      case CPI::SCA::SCA_ushort:
        {
          unsigned int idata = CPI::Util::Misc::stringToUnsigned (value);
          uint16_t data = static_cast<uint16_t> (idata);

          e = m_containerWorkerId->write ( p.offset,
                                         2, WCI_DATA_TYPE_U16,
                                         WCI_DEFAULT, &data);
        }
        break;

      case CPI::SCA::SCA_string:
        {
          unsigned int len = value.length () + 1;

          if (len > pt.size + 1) {
            throw std::string ("String exceeds maximum length");
          }

          e = m_containerWorkerId->write ( p.offset,
                                         len, WCI_DATA_TYPE_U8,
                                         WCI_DEFAULT,
                                         value.c_str ());
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
    if (m_containerWorkerId->control ( WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT) != WCI_SUCCESS) {
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
    const CPI::SCA::Property & p = m_props[n];

    if (p.write_sync && !haveSync) {
      if (m_containerWorkerId->control ( WCI_CONTROL_BEFORE_QUERY, WCI_DEFAULT) != WCI_SUCCESS) {
        throw std::string ("Failed to query worker");
      }

      haveSync = true;
    }

    std::cout << p.name << ": ";

    if (p.is_struct) {
      std::cout << "{structs not supported yet}" << std::endl;
      continue;
    }

    cpiAssert (p.num_members == 1);
    CPI::SCA::SimpleType & pt = p.types[0];
    WCI_error e = WCI_SUCCESS;

    try {
      if (!p.is_sequence) {

#define PRINTPROPERTY(type,wcitype,printitem) do {        \
          type data;                                        \
          e = m_containerWorkerId->read ( \
                                        p.offset,        \
                                        sizeof(type),        \
                                        wcitype,        \
                                        WCI_DEFAULT,        \
                                        &data);                \
          std::cout << (printitem);                        \
        } while (0)

        switch (pt.data_type) {
        case CPI::SCA::SCA_boolean:
          PRINTPROPERTY(WCI_u8, WCI_DATA_TYPE_U8, (data ? "true" : "false"));
          break;

        case CPI::SCA::SCA_char:
          PRINTPROPERTY(WCI_u8, WCI_DATA_TYPE_U8, ((int) data));
          break;

        case CPI::SCA::SCA_double:
          PRINTPROPERTY(WCI_f64, WCI_DATA_TYPE_F64, data);
          break;

        case CPI::SCA::SCA_float:
          PRINTPROPERTY(WCI_f32, WCI_DATA_TYPE_F32, data);
          break;

        case CPI::SCA::SCA_short:
          PRINTPROPERTY(WCI_u16, WCI_DATA_TYPE_U16, static_cast<CPI::OS::int16_t> (data));
          break;

        case CPI::SCA::SCA_long:
          PRINTPROPERTY(WCI_u32, WCI_DATA_TYPE_U32, static_cast<CPI::OS::int32_t> (data));
          break;

        case CPI::SCA::SCA_octet:
          PRINTPROPERTY(WCI_u8, WCI_DATA_TYPE_U8, data);
          break;

        case CPI::SCA::SCA_ulong:
          PRINTPROPERTY(WCI_u32, WCI_DATA_TYPE_U32, data);
          break;

        case CPI::SCA::SCA_ushort:
          PRINTPROPERTY(WCI_u16, WCI_DATA_TYPE_U16, data);
          break;

        case CPI::SCA::SCA_string:
          {
            char * buf = new char[pt.size+1];

            e = m_containerWorkerId->read ( p.offset,
                                          pt.size, WCI_DATA_TYPE_U8,
                                          WCI_DEFAULT, buf);

            buf[pt.size] = '\0';
            std::cout << buf;
            delete [] buf;
          }
          break;

        default:
          cpiAssert (0);
        }

#undef PRINTPROPERTY

      }
      else {
        WCI_u32 length;

        if ((e = m_containerWorkerId->read ( p.offset,
                                           4, WCI_DATA_TYPE_U32,
                                           WCI_DEFAULT, &length)) != WCI_SUCCESS) {
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
          for (WCI_u32 i=0; i<length; i++) {        \
            if (i) {                                \
              std::cout << ", ";                \
            }                                        \
            std::cout << printitem;                \
          }                                        \
        } while (0)

#define PRINTPROPERTY(type,wcitype,printitem) do {                \
          type * data = new type [length];                        \
          e = m_containerWorkerId->read (                \
                                        p.data_offset,                \
                                        sizeof(type)*length,        \
                                        wcitype,                \
                                        WCI_DEFAULT,                \
                                        data);                        \
          PRINTSEQ(printitem);                                        \
          delete [] data;                                        \
        } while (0)

        switch (pt.data_type) {
        case CPI::SCA::SCA_boolean:
          PRINTPROPERTY(WCI_u8, WCI_DATA_TYPE_U8, (data[i] ? "true" : "false"));
          break;

        case CPI::SCA::SCA_char:
        case CPI::SCA::SCA_octet:
          {
            WCI_u8 * data = new WCI_u8 [length];
            e = m_containerWorkerId->read ( p.data_offset,
                                          length, WCI_DATA_TYPE_U8,
                                          WCI_DEFAULT, data);
            std::cout << std::endl;
            dumpOctets (data, length);
            delete [] data;
          }
          break;

        case CPI::SCA::SCA_double:
          PRINTPROPERTY(WCI_f64, WCI_DATA_TYPE_F64, data[i]);
          break;

        case CPI::SCA::SCA_float:
          PRINTPROPERTY(WCI_f32, WCI_DATA_TYPE_F32, data[i]);
          break;

        case CPI::SCA::SCA_short:
          PRINTPROPERTY(WCI_u16, WCI_DATA_TYPE_U16, static_cast<CPI::OS::int16_t> (data[i]));
          break;

        case CPI::SCA::SCA_long:
          PRINTPROPERTY(WCI_u32, WCI_DATA_TYPE_U32, static_cast<CPI::OS::int32_t> (data[i]));
          break;

        case CPI::SCA::SCA_ulong:
          PRINTPROPERTY(WCI_u32, WCI_DATA_TYPE_U32, data[i]);
          break;

        case CPI::SCA::SCA_ushort:
          PRINTPROPERTY(WCI_u16, WCI_DATA_TYPE_U16, data[i]);
          break;

        case CPI::SCA::SCA_string:
          std::cout << "{String sequences not implemented yet}";
          break;

        default:
          cpiAssert (0);
        }

#undef PRINTPROPERTY
#undef PRINTSEQ

      }

      if (e != WCI_SUCCESS) {
        std::cout << "{" << wci_strerror (e) << "}";
      }

      std::cout << std::endl;
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

const CPI::SCA::Port *
TestWorker::
findPort (const char * name, unsigned int & portOrdinal)
  throw (std::string)
{
  CPI::SCA::Port * p = m_ports;

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
loadDll (CPI::Util::Vfs::Vfs & fs,
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

  CPI::Util::FileFs::FileFs * ffs = dynamic_cast<CPI::Util::FileFs::FileFs *> (&fs);

  if (m_verbose) {
    std::cout << "Loading DLL \"" << normalName << "\" ... " << std::flush;
  }

  if (ffs) {
    CPI::OS::LoadableModule * lm;
    std::string nativeName;

    try {
      nativeName = ffs->toNativeName (dllFileName);
      lm = new CPI::OS::LoadableModule (nativeName);

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
    CPI::Util::LoadableModule * lm;

    try {
      lm = new CPI::Util::LoadableModule (&fs, dllFileName);
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
  cpiAssert (ldi != m_loadedDlls.end());
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
    return CPI::Util::Vfs::relativeName (fileName);
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

    std::string relName = "cpi-rcc-test";
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
      std::cerr << "Oops: CPI RCC Tester is already running, pid "
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

  CPI::Util::LoadableModule::setTemporaryFileLocation (g_tempFileLocation);
#endif

  int result = startTestWorkerCmdInt (argc, argv);

#if ! defined (__VXWORKS__)
  try {
    CPI::OS::FileSystem::remove (pidFileName);
  }
  catch (...) {
  }

  if (!haveDllDir) {
    cleanTempFileLocation (g_tempFileLocation);
  }
#endif

  return result;
}
