// -*- c++ -*-

#ifndef CPI_CP289_EXECUTABLE_DEVICE_H__
#define CPI_CP289_EXECUTABLE_DEVICE_H__

/**
 * \file
 * \brief SCA CP289 Executable Device
 *
 * Revision History:
 *
 *     07/09/2009 - Jim Kulp
 *                  Initial version copied as template
 */

#include <string>
#include <CosEventComm.h>
#include <CpiLwLogLoggerOutput.h>
#include <CpiCFUtilReceptacleHelper.h>
#include <CpiCFUtilDeviceBase.h>
#include <CpiContainerInterface.h>
#include <Cp289GenericProxy.h>
#include <CF_s.h>

namespace CPI {
  namespace SCA {

    /**
     * \brief SCA CP289 Executable Device
     *
     * Implements the SCA CF::ExecutableDevice interface, supporting
     * CP289 workers running.  An instance of the SCA Generic
     * Proxy is created for each worker.  See the SCA specification
     * for the semantics of each method.
     *
     * The device has two ports (implemented in CPI::CFUtil::DeviceBase),
     * "LogOut" for a connection to the Log Service, and "EventOut" for a
     * connection to the Domain Manager's "IDM_Channel."  The device will
     * work in the absence of either connection.
     */

    class Cp289ExecutableDevice :
        virtual public CPI::CFUtil::DeviceBase,
        virtual public POA_CF::ExecutableDevice
    {
    public:
      /**
       * Constructor.
       *
       * \param[in] orb    Used for SCA Generic Proxy servants.
       * \param[in] poa    Used to activate SCA Generic Proxy servants.
       * \param[in] devMgr The SCA Device Manager that manages this device.
       *                   May be nil.
       * \param[in] profileFileName The SPD file for this executable device;
       *                   this file is not read or used but reported as the
       *                   softwareProfile attribute.
       * \param[in] deviceId The UUID for this executable device.  It is used
       *                   in log messages and usually comes from the
       *                   componentinstantiation element in the DCD file
       *                   via the DEVICE_ID execution parameter.
       * \param[in] deviceLabel The label for this executable device.  It is
       *                   used in log messages and usually comes from the
       *                   usagename element in the DCD file via the
       *                   DEVICE_LABEL execution parameter.
       * \param[in] cpiDeviceId The unique container ordinal for this container.
       * \param[in] endpoint The container endpoint.
       * \param[in] polled Whether the container should poll (true) or be
       *                   event driven (false).
       * \param[in] logger The logger for logging messages.  If non-null,
       *                   messages are logged to this logger in addition to
       *                   any logging service that is connected to this
       *                   device's LogOut port.
       * \param[in] adoptLogger Whether to delete \a logger in the destructor.
       * \param[in] shutdownOrbOnRelease Whether to call orb->shutdown() from
       *                   the releaseObject() operation.  This is usually
       *                   true in a stand-alone server and false if the ORB
       *                   is shared.
       */

      Cp289ExecutableDevice (CORBA::ORB_ptr orb,
                             PortableServer::POA_ptr poa,
                             CF::DeviceManager_ptr devMgr,
                             const std::string & profileFileName,
                             const std::string &identifier,
                             const std::string &label,
                             const std::string &tempDir,
                             unsigned int cpiDeviceId,
                             const std::string & cpiDeviceType,
                             const std::string & cpiDeviceUnit,
                             const std::string & endpoint,
                             bool polled,
                             const std::string & osName,
                             const std::string & processorName,
                             CPI::Logger::Logger * logger = 0,
                             bool adoptLogger = true,
                             bool shutdownOrbOnRelease = false)
        throw (std::string);

      ~Cp289ExecutableDevice ()
        throw ();

      /*
       * Overloaded from DeviceBase.
       */

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
#if 0
      bool cBit ()
        throw ();

    protected:
      struct WorkerInfo {
        WorkerInfo ()
          throw ();
        RplGenericProxy * proxy;
        CPI::Container::WorkerId containerWorkerId;
      };

      typedef std::vector<WorkerInfo> WorkerInfos;
#endif

    protected:
      CPI::Util::DriverManager  m_driverManager;
      std::string m_osName, m_processorName;
      unsigned int m_cpiDeviceId;
      std::string m_cpiDeviceType;
      // Map pid to proxy
      typedef std::map<CF::ExecutableDevice::ProcessID_Type,Cp289GenericProxy *> PidMap;
      PidMap m_pidMap;
      CPI::Container::Interface *m_container;
      CPI::Container::Application *m_application;
      DataTransfer::EventManager * m_eventManager;
      std::string m_tempDir;
      //      CPI::OS::ThreadManager m_dispatchThreadMgr;

    };

  }
}

#endif
