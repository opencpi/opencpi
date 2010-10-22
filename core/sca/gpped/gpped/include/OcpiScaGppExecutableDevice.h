
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

// -*- c++ -*-

#ifndef OCPI_SCA_GPP_EXECUTABLE_DEVICE_H__
#define OCPI_SCA_GPP_EXECUTABLE_DEVICE_H__

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
#include <OcpiOsProcessManager.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilLoadableModule.h>
#include <OcpiLwLogLoggerOutput.h>
#include <OcpiCFUtilReceptacleHelper.h>
#include <OcpiCFUtilDeviceBase.h>
#include <CF_s.h>

namespace OCPI {
  namespace SCA {

    class GppExecutableDevice :
        virtual public OCPI::CFUtil::DeviceBase,
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
                           unsigned int ocpiDeviceId,
                           const std::string & osName,
                           const std::string & processorName,
                           OCPI::Logger::Logger * logger = 0,
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

      void loadDll (OCPI::Util::Vfs::Vfs & fs,
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
        OCPI::Util::LoadableModule * module;
      };

      typedef std::map<std::string, LoadedDllInfo> LoadedDllInfos;

      /*
       * Information about running processes.
       */

      struct ProcessInfo {
        std::string executableFile;
        OCPI::OS::ProcessManager * processManager;
      };

      typedef std::map<unsigned int, ProcessInfo> ProcessInfos;

    protected:
      unsigned int m_ocpiDeviceId;
      std::string m_osName;
      std::string m_processorName;
      LoadedFileInfos m_loadedFiles;
      LoadedDllInfos m_loadedDlls;
      ProcessInfos m_processInfos;
      OCPI::Util::FileFs::FileFs m_fileFs;
      static const unsigned int s_shutdownGracePeriod;
    };

  }
}

#endif
