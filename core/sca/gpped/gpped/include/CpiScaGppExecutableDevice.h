// -*- c++ -*-

#ifndef CPI_SCA_GPP_EXECUTABLE_DEVICE_H__
#define CPI_SCA_GPP_EXECUTABLE_DEVICE_H__

/**
 * \file
 * \brief SCA GPP Executable Device
 *
 * Revision History:
 *
 *     05/28/2009 - Frank Pilhofer
 *                  Make os_name configurable, so that we can set it to
 *                  TimesysLinux, which is what SCARI uses.
 *
 *     04/06/2009 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <map>
#include <string>
#include <CpiOsProcessManager.h>
#include <CpiUtilVfs.h>
#include <CpiUtilFileFs.h>
#include <CpiUtilLoadableModule.h>
#include <CpiLwLogLoggerOutput.h>
#include <CpiCFUtilReceptacleHelper.h>
#include <CpiCFUtilDeviceBase.h>
#include <CF_s.h>

namespace CPI {
  namespace SCA {

    class GppExecutableDevice :
        virtual public CPI::CFUtil::DeviceBase,
	virtual public POA_CF::ExecutableDevice
    {
    public:
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
			   CPI::Logger::Logger * logger = 0,
			   bool adoptLogger = true,
			   bool shutdownOrbOnRelease = false)
	throw (std::string);

      ~GppExecutableDevice ()
	throw ();

      /*
       * Overloaded from DeviceBase.
       */

      void releaseObject ()
	throw (CF::LifeCycle::ReleaseError,
	       CORBA::SystemException);

      void configure (const CF::Properties & props)
	throw (CF::PropertySet::InvalidConfiguration,
	       CF::PropertySet::PartialConfiguration,
	       CORBA::SystemException);

      void query (CF::Properties & props)
	throw (CF::UnknownProperties,
	       CORBA::SystemException);

      /*
       * CF::LoadableDevice
       */

      void load (CF::FileSystem_ptr fs,
		 const char * fileName,
		 CF::LoadableDevice::LoadType loadKind)
	throw (CF::LoadableDevice::InvalidLoadKind,
	       CF::LoadableDevice::LoadFail,
	       CF::Device::InvalidState,
	       CF::InvalidFileName,
	       CORBA::SystemException);

      void unload (const char * fileName)
	throw (CF::Device::InvalidState,
	       CF::InvalidFileName,
	       CORBA::SystemException);

      /*
       * CF::ExecutableDevice
       */

      CF::ExecutableDevice::ProcessID_Type
      execute (const char * fileName,
	       const CF::Properties & options,
	       const CF::Properties & parameters)
	throw (CF::ExecutableDevice::InvalidFunction,
	       CF::ExecutableDevice::InvalidParameters,
	       CF::ExecutableDevice::InvalidOptions,
	       CF::ExecutableDevice::ExecuteFail,
	       CF::Device::InvalidState,
	       CF::InvalidFileName,
	       CORBA::SystemException);

      void terminate (CF::ExecutableDevice::ProcessID_Type)
	throw (CF::ExecutableDevice::InvalidProcess,
	       CF::Device::InvalidState,
	       CORBA::SystemException);

    protected:
      void cleanup ()
	throw ();

      static std::string normalizeFileName (const std::string & fileName)
	throw ();

      void loadDll (CPI::Util::Vfs::Vfs & fs,
		    const std::string & fileName)
	throw (std::string);

      void unloadDll (const std::string & fileName)
	throw ();

    protected:
      /*
       * Information about loaded files.
       */

      struct LoadedFileInfo {
	unsigned int referenceCount;
      };

      typedef std::map<std::string, LoadedFileInfo> LoadedFileInfos;

      /*
       * Information about loaded DLLs.
       */

      struct LoadedDllInfo {
	unsigned int referenceCount;
	CPI::Util::LoadableModule * module;
      };

      typedef std::map<std::string, LoadedDllInfo> LoadedDllInfos;

      /*
       * Information about running processes.
       */

      struct ProcessInfo {
	std::string executableFile;
	CPI::OS::ProcessManager * processManager;
      };

      typedef std::map<unsigned int, ProcessInfo> ProcessInfos;

    protected:
      unsigned int m_cpiDeviceId;
      std::string m_osName;
      std::string m_processorName;
      LoadedFileInfos m_loadedFiles;
      LoadedDllInfos m_loadedDlls;
      ProcessInfos m_processInfos;
      CPI::Util::FileFs::FileFs m_fileFs;
      static const unsigned int s_shutdownGracePeriod;
    };

  }
}

#endif
