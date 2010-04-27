/*
 * SCA GPP Executable Device
 *
 * Revision History:
 *
 *     05/28/2009 - Frank Pilhofer
 *                  Make os_name configurable, so that we can set it to
 *                  TimesysLinux, which is what SCARI uses.
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2.
 *
 *     04/06/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <new>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <CpiOsFileSystem.h>
#include <CpiOsProcessManager.h>
#include <CpiLoggerLogger.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiUtilVfs.h>
#include <CpiUtilMisc.h>
#include <CpiUtilAutoMutex.h>
#include <CpiCFUtilMisc.h>
#include <CpiCFUtilSCAFs.h>
#include <CpiCFUtilDeviceBase.h>
#include <CpiCFUtilLegacyErrorNumbers.h>
#include <CpiStringifyCorbaException.h>
#include <CosNaming.h>
#include <CF_s.h>
#include "CpiScaGppExecutableDevice.h"

#if defined (__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#endif

const unsigned int
CPI::SCA::GppExecutableDevice::
s_shutdownGracePeriod = 3000;

CPI::SCA::GppExecutableDevice::
GppExecutableDevice (CORBA::ORB_ptr orb,
		     PortableServer::POA_ptr poa,
		     CF::DeviceManager_ptr devMgr,
		     const std::string & profileFileName,
		     const std::string & deviceId,
		     const std::string & deviceLabel,
		     const std::string & tempFileLocation,
		     unsigned int cpiDeviceId,
		     const std::string & osName,
		     const std::string & processorName,
		     CPI::Logger::Logger * logger,
		     bool adoptLogger,
		     bool shutdownOrbOnRelease)
  throw (std::string)
  : CPI::CFUtil::DeviceBase (orb,
			     poa,
			     devMgr,
			     profileFileName,
			     deviceId,
			     deviceLabel,
			     logger,
			     adoptLogger,
			     shutdownOrbOnRelease),
    m_cpiDeviceId (cpiDeviceId),
    m_osName (osName),
    m_processorName (processorName),
    m_fileFs (tempFileLocation)
{
  CPI::Logger::DebugLogger debug (m_out);
  debug << m_logProducerName
	<< "GppExecutableDevice for device id "
	<< m_cpiDeviceId
	<< " constructor."
	<< std::flush;

  /*
   * If os_name and processor_name weren't specified, use defaults.
   *
   * Use compiler-defined preprocessor values (use cpp -dM to find
   * out) to pick the appropriate names according to the enumeration
   * in SCA Attachment 2 to Appendix D.
   */

  if (!m_osName.length()) {
#if defined (__linux__)
    m_osName = "Linux";
#elif defined (__VXWORKS__)
    m_osName = "VxWorks";
#elif defined (__WINNT__) || defined (_WIN32)
    m_osName = "WinNT";
#else
    m_osName = "Unknown";
#endif
  }

  if (!m_processorName.length()) {
#if defined (__i386__) || defined (__x86_64__) || defined (_X86_)
    m_processorName = "x86";
#elif defined (_ARCH_PPC) || defined (__PPC__)
    m_processorName = "ppc";
#else
    m_processorName = "Unknown";
#endif
  }
}

CPI::SCA::GppExecutableDevice::
~GppExecutableDevice ()
  throw ()
{
  CPI::Logger::DebugLogger debug (m_out);
  debug << m_logProducerName
	<< "GppExecutableDevice for device id "
	<< m_cpiDeviceId
	<< " destructor."
	<< std::flush;
  cleanup ();
}

void
CPI::SCA::GppExecutableDevice::
cleanup ()
  throw ()
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  /*
   * Terminate all processes that we started.
   */

  for (ProcessInfos::iterator rit = m_processInfos.begin ();
       rit != m_processInfos.end (); rit++) {
    CPI::OS::ProcessManager * pm = (*rit).second.processManager;

    debug << m_logProducerName
	  << "Terminating process "
	  << (*rit).first
	  << "."
	  << std::flush;

    try {
      pm->shutdown ();
    }
    catch (const std::string & oops) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Error terminating process " << (*rit).first
	    << ": " << oops << ". (Ignored.)" << std::flush;
    }

    bool hasTerminated;

    try {
      hasTerminated = pm->wait (s_shutdownGracePeriod);
    }
    catch (...) {
      hasTerminated = false;
    }

    if (!hasTerminated) {
      pm->detach ();
    }

    delete pm;
  }

  m_processInfos.clear ();

  /*
   * Close all shared libraries.
   */

  for (LoadedDllInfos::iterator ldi = m_loadedDlls.begin ();
       ldi != m_loadedDlls.end (); ldi++) {
    debug << m_logProducerName
	  << "Unloading DLL \""
	  << (*ldi).first
	  << "\"."
	  << std::flush;

    try {
      (*ldi).second.module->close ();
    }
    catch (const std::string & oops) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Error unloading DLL \""
	    << (*ldi).first
	    << "\": "
	    << oops
	    << ". (Ignored.)"
	    << std::flush;
    }
  }

  m_loadedDlls.clear ();

  /*
   * Remove all files that we downloaded.
   */

  for (LoadedFileInfos::iterator lfit = m_loadedFiles.begin ();
       lfit != m_loadedFiles.end (); lfit++) {
    debug << m_logProducerName
	  << "Removing \""
	  << (*lfit).first
	  << "\"."
	  << std::flush;

    try {
      m_fileFs.remove ((*lfit).first);
    }
    catch (const std::string & oops) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Error removing file " << (*lfit).first
	    << ": " << oops << ". (Ignored.)" << std::flush;
    }
  }

  m_loadedFiles.clear ();
}

void
CPI::SCA::GppExecutableDevice::
releaseObject ()
  throw (CF::LifeCycle::ReleaseError,
	 CORBA::SystemException)
{
  /*
   * First call the base class' implementation, which deactivates us.
   */

  DeviceBase::releaseObject ();

  /*
   * Then clean up.
   */

  cleanup ();
}

void
CPI::SCA::GppExecutableDevice::
configure (const CF::Properties & props)
  throw (CF::PropertySet::InvalidConfiguration,
	 CF::PropertySet::PartialConfiguration,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  CORBA::ULong numProps = props.length ();
  CF::Properties invalidProperties;
  CORBA::ULong numInvalidProperties = 0;

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "configure (";

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    if (dpi > 0) {
      debug << ", ";
    }

    debug << props[dpi].id;
  }

  debug << ")" << std::flush;

  for (CORBA::ULong pi=0; pi<numProps; pi++) {
    const CF::DataType & property = props[pi];
    const char * propertyId = property.id.in ();

    if (std::strcmp (propertyId, "PRODUCER_LOG_LEVEL") == 0) {
      if (!configureProducerLogLevel (property)) {
	invalidProperties.length (numInvalidProperties + 1);
	invalidProperties[numInvalidProperties++] = property;
      }
    }
    else {
      invalidProperties.length (numInvalidProperties + 1);
      invalidProperties[numInvalidProperties++] = property;
    }
  }

  if (numInvalidProperties) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Configuration failed for "
	  << ((numInvalidProperties != 1) ? "properties " : "property ");

    for (CORBA::ULong dpi=0; dpi<numInvalidProperties; dpi++) {
      if (dpi > 0 && dpi+1 == numInvalidProperties) {
	m_out << " and ";
      }
      else if (dpi > 0) {
	m_out << ", ";
      }

      m_out << invalidProperties[dpi].id;
    }

    m_out << "." << std::flush;
  }

  if (numInvalidProperties > 0 && numInvalidProperties == numProps) {
    // No property was successfully configured.
    CF::PropertySet::InvalidConfiguration ic;
    ic.msg = "configure failed";
    ic.invalidProperties = invalidProperties;
    throw ic;
  }
  else if (numInvalidProperties) {
    // Some, but not all properties were successfully configured.
    CF::PropertySet::PartialConfiguration pc;
    pc.invalidProperties = invalidProperties;
    throw pc;
  }

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Configuration complete."
	<< std::flush;
}

void
CPI::SCA::GppExecutableDevice::
query (CF::Properties & props)
  throw (CF::UnknownProperties,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  CORBA::ULong numProps = props.length ();
  CORBA::ULong numInvalidProperties = 0;
  CF::UnknownProperties up;

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "query (";

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    if (dpi > 0) {
      debug << ", ";
    }

    debug << props[dpi].id;
  }

  debug << ")" << std::flush;

  if (!numProps) {
    /*
     * We're meant to return all properties.  Initialize props with all
     * property names, so that the code below fills them in.
     */

    numProps = 5;
    props.length (numProps);
    props[0].id = "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2"; // CPIDeviceId allocation capability
    props[1].id = "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43"; // CPIContainerType allocation capability
    props[2].id = "DCE:80bf17f0-6c7f-11d4-a226-0050da314cd6"; // OS Name allocation capability
    props[3].id = "DCE:9b445600-6c7f-11d4-a226-0050da314cd6"; // Processor Name allocation capability
    props[4].id = "PRODUCER_LOG_LEVEL"; // SCA required
  }

  for (CORBA::ULong pi=0; pi<numProps; pi++) {
    CF::DataType & property = props[pi];
    const char * propertyId = property.id.in ();

    if (CPI::Util::Misc::caseInsensitiveStringCompare (propertyId, "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2") == 0) {
      property.value <<= static_cast<CORBA::ULong> (m_cpiDeviceId);
    }
    else if (CPI::Util::Misc::caseInsensitiveStringCompare (propertyId, "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43") == 0) {
      property.value <<= "GPP";
    }
    else if (CPI::Util::Misc::caseInsensitiveStringCompare (propertyId, "DCE:80bf17f0-6c7f-11d4-a226-0050da314cd6") == 0) {
      property.value <<= m_osName.c_str ();
    }
    else if (CPI::Util::Misc::caseInsensitiveStringCompare (propertyId, "DCE:9b445600-6c7f-11d4-a226-0050da314cd6") == 0) {
      property.value <<= m_processorName.c_str ();
    }
    else if (std::strcmp (propertyId, "PRODUCER_LOG_LEVEL") == 0) {
      queryProducerLogLevel (property);
    }
    else {
      up.invalidProperties.length (numInvalidProperties + 1);
      up.invalidProperties[numInvalidProperties++] = property;
    }
  }

  if (numInvalidProperties) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Query failed for "
	  << ((numInvalidProperties != 1) ? "properties " : "property ");

    for (CORBA::ULong dpi=0; dpi<numInvalidProperties; dpi++) {
      if (dpi > 0 && dpi+1 == numInvalidProperties) {
	m_out << " and ";
      }
      else if (dpi > 0) {
	m_out << ", ";
      }

      m_out << up.invalidProperties[dpi].id;
    }

    m_out << "." << std::flush;
    throw up;
  }
}

void
CPI::SCA::GppExecutableDevice::
load (CF::FileSystem_ptr fileSystem,
      const char * fileName,
      CF::LoadableDevice::LoadType loadKind)
  throw (CF::LoadableDevice::InvalidLoadKind,
	 CF::LoadableDevice::LoadFail,
	 CF::Device::InvalidState,
	 CF::InvalidFileName,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "load (\""
	<< fileName
	<< "\", "
	<< CPI::CFUtil::loadTypeToString (loadKind)
	<< ")"
	<< std::flush;

  if (m_adminState != CF::Device::UNLOCKED ||
      m_operationalState == CF::Device::DISABLED) {
    /*
     * Spec says that we should raise the InvalidState exception in
     * this case.
     */

    std::string msg;

    if (m_adminState != CF::Device::UNLOCKED) {
      msg = "Administrative state is ";
      msg += CPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
    }

    CF::Device::InvalidState is;
    is.msg = msg.c_str ();
    throw is;
  }

  /*
   * We support files of the "shared library" and "executable" kind.
   */

  if (loadKind != CF::LoadableDevice::SHARED_LIBRARY &&
      loadKind != CF::LoadableDevice::EXECUTABLE) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Can not load "
	  << CPI::CFUtil::loadTypeToString (loadKind)
	  << " \""
	  << fileName
	  << "\": only \"executable\" files are supported."
	  << std::flush;

    throw CF::LoadableDevice::InvalidLoadKind ();
  }

  try {
    CPI::CFUtil::SCAFs scaFs (m_orb, fileSystem);
    std::string theFileName (fileName);

    if (!fileSystem->exists (fileName)) {
      std::string msg = "File not found: \"";
      msg += theFileName;
      msg += "\"";

      CF::InvalidFileName ifn;
      ifn.errorNumber = CF::CF_ENOENT;
      ifn.msg = msg.c_str ();
      throw ifn;
    }

    /*
     * Handle shared libraries.
     */

    std::string loadFileName = normalizeFileName (fileName);
    std::string::size_type dot = loadFileName.rfind ('.');

    if (dot != std::string::npos) {
      std::string fileExtension = loadFileName.substr (dot + 1);

      if (fileExtension == "so" ||
	  fileExtension == "dll" ||
	  fileExtension == "out") {
	/*
	 * Treat this file as a shared library.
	 */

	loadDll (scaFs, theFileName);
	return;
      }
    }

    /*
     * Check whether we've already loaded that file.
     */

    LoadedFileInfos::iterator lfi = m_loadedFiles.find (loadFileName);

    if (lfi != m_loadedFiles.end()) {
      /*
       * Yes, this file is already loaded.  All we have to do is to increase
       * its reference count.
       */

      (*lfi).second.referenceCount++;

      debug << m_logProducerName
	    << "Incremented reference count for \""
	    << loadFileName
	    << "\" to "
	    << (*lfi).second.referenceCount
	    << "."
	    << std::flush;

      return;
    }

    /*
     * Download that file.
     */

    scaFs.copy (fileName, &m_fileFs, loadFileName);

    /*
     * On Linux, we must mark the file as executable.
     */

#if defined (__linux__)
    std::string nativeFileName = m_fileFs.toNativeName (loadFileName);
    ::chmod (nativeFileName.c_str(), 0700);
#endif

    /*
     * And remember that it was loaded.
     */

    LoadedFileInfo & fileInfo = m_loadedFiles[loadFileName];
    fileInfo.referenceCount = 1;
  }
  catch (const std::string & oops) {
    std::string msg;
    msg  = "Failed to read from \"";
    msg += fileName;
    msg += "\": ";
    msg += oops;

    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << msg << "."
	  << std::flush;

    CF::LoadableDevice::LoadFail lf;
    lf.errorNumber = CF::CF_EIO;
    lf.msg = msg.c_str ();
    throw lf;
  }
  catch (const CF::InvalidFileName & ifn) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Invalid file name: "
	  << ifn.msg
	  << "."
	  << std::flush;
    throw;
  }

  /*
   * All seems well.
   */

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Loaded executable \""
	<< fileName
	<< "\" on device "
	<< m_cpiDeviceId
	<< "."
	<< std::flush;
}

void
CPI::SCA::GppExecutableDevice::
unload (const char * fileName)
  throw (CF::Device::InvalidState,
	 CF::InvalidFileName,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "unload (\""
	<< fileName
	<< "\")"
	<< std::flush;

  if (m_adminState != CF::Device::UNLOCKED ||
      m_operationalState == CF::Device::DISABLED) {
    /*
     * Spec says that we should raise the InvalidState exception in
     * this case.
     */

    std::string msg;

    if (m_adminState != CF::Device::UNLOCKED) {
      msg = "Administrative state is ";
      msg += CPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
    }

    CF::Device::InvalidState is;
    is.msg = msg.c_str ();
    throw is;
  }

  /*
   * Handle shared libraries.
   */

  std::string loadFileName = normalizeFileName (fileName);
  std::string::size_type dot = loadFileName.rfind ('.');

  if (dot != std::string::npos) {
    std::string fileExtension = loadFileName.substr (dot + 1);

    if (fileExtension == "so" ||
	fileExtension == "dll" ||
	fileExtension == "out") {
      /*
       * Treat this file as a shared library.
       */

      unloadDll (fileName);
      return;
    }
  }

  /*
   * Check whether this file is loaded.
   */

  LoadedFileInfos::iterator lfi = m_loadedFiles.find (loadFileName);

  if (lfi == m_loadedFiles.end()) {
    std::string msg = "Can not unload file \"";
    msg += fileName;
    msg += "\": not loaded";

    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << msg << "."
	  << std::flush;

    CF::InvalidFileName ifn;
    ifn.errorNumber = CF::CF_EINVAL;
    ifn.msg = msg.c_str ();
    throw ifn;
  }

  /*
   * Decrement reference count.
   */

  LoadedFileInfo & fileInfo = (*lfi).second;
  fileInfo.referenceCount--;

  debug << m_logProducerName
	<< "Decremented reference count for \""
	<< loadFileName
	<< "\" to "
	<< fileInfo.referenceCount
	<< "."
	<< std::flush;

  if (fileInfo.referenceCount) {
    /*
     * This file is still in use.
     */

    return;
  }

  /*
   * Reference count dropped to zero.  Unload.
   */

  m_loadedFiles.erase (lfi);

  try {
    m_fileFs.remove (loadFileName);
  }
  catch (const std::string & oops) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Failed to delete \""
	  << loadFileName
	  << "\": "
	  << oops
	  << ". (Ignored.)"
	  << std::flush;
  }
}

CF::ExecutableDevice::ProcessID_Type
CPI::SCA::GppExecutableDevice::
execute (const char * entrypoint,
	 const CF::Properties & options,
	 const CF::Properties & parameters)
  throw (CF::ExecutableDevice::InvalidFunction,
	 CF::ExecutableDevice::InvalidParameters,
	 CF::ExecutableDevice::InvalidOptions,
	 CF::ExecutableDevice::ExecuteFail,
	 CF::Device::InvalidState,
	 CF::InvalidFileName,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "execute (\""
	<< entrypoint
	<< "\")"
	<< std::flush;

  if (m_adminState != CF::Device::UNLOCKED ||
      m_operationalState == CF::Device::DISABLED) {
    /*
     * Spec says that we should raise the InvalidState exception in
     * this case.
     */

    std::string msg;

    if (m_adminState != CF::Device::UNLOCKED) {
      msg = "Administrative state is ";
      msg += CPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
    }

    CF::Device::InvalidState is;
    is.msg = msg.c_str ();
    throw is;
  }

  /*
   * We expect the "entrypoint" to match the name of the file.
   */

  std::string loadFileName = normalizeFileName (entrypoint);
  LoadedFileInfos::iterator lfi = m_loadedFiles.find (loadFileName);

  if (lfi == m_loadedFiles.end()) {
    std::string msg = "Can not execute \"";
    msg += entrypoint;
    msg += "\": file not loaded";

    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << msg << "."
	  << std::flush;

    throw CF::ExecutableDevice::InvalidFunction ();
  }

  /*
   * Validate that the options look legit.
   */

  {
    CF::ExecutableDevice::InvalidOptions io;
    CORBA::ULong numInvalidOptions = 0;

    for (CORBA::ULong oi=0; oi<options.length(); oi++) {
      const CF::DataType & option = options[oi];
      const char * optionId = option.id.in ();

      if (std::strcmp (optionId, "STACK_SIZE") == 0) {
	CORBA::ULong value;

	if (!(option.value >>= value)) {
	  io.invalidOpts.length (numInvalidOptions + 1);
	  io.invalidOpts[numInvalidOptions++] = option;
	}

	/* STACK_SIZE value is ignored */
      }
      else if (std::strcmp (optionId, "PRIORITY") == 0) {
	CORBA::ULong value;

	if (!(option.value >>= value)) {
	  io.invalidOpts.length (numInvalidOptions + 1);
	  io.invalidOpts[numInvalidOptions++] = option;
	}

	/* PRIORITY value is ignored */
      }
      else {
	io.invalidOpts.length (numInvalidOptions + 1);
	io.invalidOpts[numInvalidOptions++] = option;
      }
    }

    if (numInvalidOptions) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "execute: invalid options: ";

      for (CORBA::ULong ioi=0; ioi<numInvalidOptions; ioi++) {
	if (ioi) {
	  m_out << ", ";
	}
	m_out << io.invalidOpts[ioi].id;
      }

      m_out << "." << std::flush;
      throw io;
    }
  }

  /*
   * Compose the command line.
   */

  CPI::OS::ProcessManager::ParameterList pp;

  {
    CF::ExecutableDevice::InvalidParameters ip;
    CORBA::ULong numInvalidParameters = 0;

    for (CORBA::ULong pi=0; pi<parameters.length(); pi++) {
      const CF::DataType & parameter = parameters[pi];
      const char * name = parameter.id.in();
      const char * value;

      if ((parameter.value >>= value)) {
	pp.push_back (name);
	pp.push_back (value);
      }
      else {
	ip.invalidParms.length (numInvalidParameters + 1);
	ip.invalidParms[numInvalidParameters++] = parameter;
      }
    }

    if (numInvalidParameters) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "execute: invalid parameters: ";

      for (CORBA::ULong ipi=0; ipi<numInvalidParameters; ipi++) {
	if (ipi) {
	  m_out << ", ";
	}
	m_out << ip.invalidParms[ipi].id;
      }

      m_out << "." << std::flush;
      throw ip;
    }
  }

  /*
   * Start the process.
   */

  CPI::OS::ProcessManager * pm;

  try {
    std::string fileName = m_fileFs.toNativeName (loadFileName);
    pm = new CPI::OS::ProcessManager (fileName, pp);
  }
  catch (const std::string & oops) {
    std::string msg = "Failed to execute \"";
    msg += entrypoint;
    msg += "\": ";
    msg += oops;

    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << msg << "."
	  << std::flush;

    CF::ExecutableDevice::ExecuteFail ef;
    ef.errorNumber = CF::CF_ENOEXEC;
    ef.msg = oops.c_str ();
    throw ef;
  }

  unsigned int pid = static_cast<unsigned int> (pm->pid());
  ProcessInfo & pi = m_processInfos[pid];
  pi.executableFile = loadFileName;
  pi.processManager = pm;

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Started \""
	<< entrypoint
	<< "\", process id "
	<< pid
	<< "."
	<< std::flush;

  return static_cast<CF::ExecutableDevice::ProcessID_Type> (pid);
}

void
CPI::SCA::GppExecutableDevice::
terminate (CF::ExecutableDevice::ProcessID_Type processId)
  throw (CF::ExecutableDevice::InvalidProcess,
	 CF::Device::InvalidState,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "terminate ("
	<< processId
	<< ")"
	<< std::flush;

  if (m_adminState != CF::Device::UNLOCKED ||
      m_operationalState == CF::Device::DISABLED) {
    /*
     * Spec says that we should raise the InvalidState exception in
     * this case.
     */

    std::string msg;

    if (m_adminState != CF::Device::UNLOCKED) {
      msg = "Administrative state is ";
      msg += CPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
    }

    CF::Device::InvalidState is;
    is.msg = msg.c_str ();
    throw is;
  }

  /*
   * Locate this process.
   */

  unsigned int pid = static_cast<unsigned int> (processId);
  ProcessInfos::iterator pit = m_processInfos.find (pid);

  if (pit == m_processInfos.end()) {
    std::string msg = "Process id \"";
    msg += CPI::Util::Misc::unsignedToString (pid);
    msg += "\" not found";

    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << msg << "."
	  << std::flush;

    CF::ExecutableDevice::InvalidProcess ip;
    ip.errorNumber = CF::CF_EINVAL;
    ip.msg = msg.c_str ();
    throw ip;
  }

  ProcessInfo & pi = (*pit).second;
  CPI::OS::ProcessManager * pm = pi.processManager;

  try {
    pm->shutdown ();
  }
  catch (const std::string & oops) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Failed to shutdown pid " << pid << ": " << oops << "."
	  << std::flush;
  }

  bool didExit;

  try {
    didExit = pm->wait (s_shutdownGracePeriod);
  }
  catch (const std::string & oops) {
    didExit = true;
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Failed to wait for process pid " << pid << ": " << oops << "."
	  << std::flush;
  }

  if (!didExit) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Process pid " << pid << " did not exit, killing."
	  << std::flush;

    try {
      pm->kill ();
    }
    catch (const std::string & oops) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Failed to kill pid " << pid << ": " << oops << "."
	    << std::flush;
    }

    try {
      didExit = pm->wait (s_shutdownGracePeriod);
    }
    catch (const std::string & oops) {
      didExit = true;
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Failed to wait for process pid " << pid << ": " << oops << "."
	    << std::flush;
    }

    if (!didExit) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Process pid " << pid << " still did not exit, forgetting about it."
	    << std::flush;
      pm->detach ();
    }
  }

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Terminated \""
	<< pi.executableFile
	<< "\", process id "
	<< pid
	<< "."
	<< std::flush;

  delete pm;
  m_processInfos.erase (pit);
}

void
CPI::SCA::GppExecutableDevice::
loadDll (CPI::Util::Vfs::Vfs & fs,
	 const std::string & dllFileName)
  throw (std::string)
{
  CPI::Logger::DebugLogger debug (m_out);

  // Assumes that a lock is held.
  std::string normalName = normalizeFileName (dllFileName);
  LoadedDllInfos::iterator ldi = m_loadedDlls.find (normalName);

  if (ldi != m_loadedDlls.end()) {
    LoadedDllInfo & dllInfo = (*ldi).second;
    dllInfo.referenceCount++;

    debug << m_logProducerName
	  << CPI::Logger::Verbosity (2)
	  << "Incremented reference count for DLL \""
	  << normalName
	  << "\" to "
	  << dllInfo.referenceCount
	  << "."
	  << std::flush;

    return;
  }

  CPI::Util::LoadableModule * lm;

  try {
    lm = new CPI::Util::LoadableModule (&fs, dllFileName);
  }
  catch (const std::bad_alloc & oops) {
    throw std::string (oops.what());
  }

  LoadedDllInfo & dllInfo = m_loadedDlls[normalName];
  dllInfo.referenceCount = 1;
  dllInfo.module = lm;

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Loaded DLL file \""
	<< dllFileName
	<< "\"."
	<< std::flush;
}

void
CPI::SCA::GppExecutableDevice::
unloadDll (const std::string & dllFileName)
  throw ()
{
  CPI::Logger::DebugLogger debug (m_out);

  // Assumes that a lock is held.
  LoadedDllInfos::iterator ldi = m_loadedDlls.find (dllFileName);
  cpiAssert (ldi != m_loadedDlls.end());
  LoadedDllInfo & dllInfo = (*ldi).second;

  if (--dllInfo.referenceCount) {
    debug << m_logProducerName
	  << CPI::Logger::Verbosity (2)
	  << "Decremented reference count for DLL \""
	  << dllFileName
	  << "\" to "
	  << dllInfo.referenceCount
	  << "."
	  << std::flush;
    return;
  }

  /*
   * Unload the DLL.
   */

  try {
    dllInfo.module->close ();
  }
  catch (const std::string & oops) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Error unloading DLL \""
	  << dllFileName
	  << "\": "
	  << oops
	  << ". (Ignored.)"
	  << std::flush;
  }

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Unloaded DLL file \""
	<< dllFileName
	<< "\"."
	<< std::flush;

  delete dllInfo.module;
  m_loadedDlls.erase (ldi);
}

std::string
CPI::SCA::GppExecutableDevice::
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
