
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

#ifndef OCPI_SCA_CP289_GENERIC_PROXY_H__
#define OCPI_SCA_CP289_GENERIC_PROXY_H__

/**
 * \file
 * \brief SCA Generic Proxy for CP289 Workers
 *
 * Revision History:
 *
 *     06/29/2009 - Jim Kulp
 *                  Initial version.
 *
 */

#include <map>
#include "OcpiContainerInterface.h"
#include "OcpiContainerPort.h"
#include "OcpiBaseProxy.h"
#include "Cp289ProviderPort.h"
#include "OcpiUtilIOP.h"

namespace OCPI {
  namespace SCA {
    // CORBA-specific for SCA (although it would be shared with other things like CCM)
    enum {
      /*
       * These ought to be registered with the OMG.
       */
      OCPI_PORT_INFO_TAG = 0x43504931,
      //      OCPI_TARGET_PORT_DATA_TAG = 0x43504931,
      OCPI_SOURCE_SHADOW_PORT_DATA_TAG = 0x43504932,
    };
    namespace CC = OCPI::Container;
    /**
     * \brief CP289 Generic Proxy
     *
     * Implements the CF::Resource API by delegating to a worker using the
     * container APIs.
     */

    class Cp289GenericProxy;
    class Cp289GenericProxyPort : public BaseProxyPort {
      friend class Cp289GenericProxy;
      Cp289GenericProxyPort(const std::string &portName, Cp289GenericProxy *proxy);
      ~Cp289GenericProxyPort() throw();
      
      OCPI::Container::Port &m_ocpiPort;
      CORBA::Object_var m_scaPort;
      typedef std::set<std::string> ConnectionSet;
      ConnectionSet m_connections;
      inline CORBA::Object_var getScaPort() { return m_scaPort; }
      void getProfile(CORBA::Object_ptr, OCPI::Util::IOP::ProfileId, std::string &);
      Cp289ProviderPort::Octets* connectInitial(const Cp289ProviderPort::Octets& initialUserInfo);
      void connectFinal(const Cp289ProviderPort::Octets& finalUserInfo);
    public:
      void connectPort (CORBA::Object_ptr connection, const char * connectionId)
        throw (CF::Port::InvalidPort,
               CF::Port::OccupiedPort,
               CORBA::SystemException);

      void disconnectPort (const char * connectionId)
        throw (CF::Port::InvalidPort,
               CORBA::SystemException);
    };

    class Cp289GenericProxy : virtual public BaseProxy {
      friend class Cp289GenericProxyPort;
    protected:
      /*
       * Disable all of our active ports and dereference them.
       */
      void releasePorts();
    public:
      /**
       * Constructor.
       *
       * \param[in] orb  Used to shutdown the OCPI_CORBA_ORB if \a shutdownOrbOnRelease
       *                 is true.
       * \param[in] poa  Used to deactivate this generic proxy in
       *                 #releaseObject().
       * \param[in] identifier This resource's identifier attribute.
       * \param[in] ocpiDeviceId The device id where this worker is running.
       * \param[in] ocpiWorkerId The worker ordinal according to OCPI.
       * \param[in] container The container that manages this worker.
       * \param[in] appContext The container's application context for this
       *                 worker.
       * \param[in] containerWorkerId The worker ID according to the container.
       * \param[in] name The worker name.  Must be a valid parameter for
       *                 wci_open().
       * \param[in] props A magic string that encodes the worker's property
       *                 space.
       * \param[in] logger The logger for debug and error messages.
       * \param[in] adoptLogger Whether to delete \a logger in the destructor.
       * \param[in] shutdownOrbOnRelease Whether to call orb->shutdown() from
       *                 the releaseObject() operation.  This is usually
       *                 true in a stand-alone server and false if the OCPI_CORBA_ORB
       *                 is shared.
       * \throw std::string If initialization fails.
       */

      Cp289GenericProxy (// needed by base class
                         CORBA::ORB_ptr orb,
                         PortableServer::POA_ptr poa,
                         const std::string & identifier,
                         // needed by this class
                         // spd:softpkg/implementation/code/localfile@name
                         const char *codeLocalFileName,
                         // spd:softpkg/implementation/code/entrypoint
                         const char *functionName,
                         const char *instanceName,
                         OCPI::API::ContainerApplication &appContext,
                         const char *namingContextIor,
                         const char *nameBinding,
                         // Optional
                         OCPI::Logger::Logger * logger = 0,
                         bool adoptLogger = true,
                         bool shutdownOrbOnRelease = false)
        throw (std::string);

      ~Cp289GenericProxy ()
      throw ();
      /*
       * CF::LifeCycle
       */

      void releaseObject ()
        throw (CF::LifeCycle::ReleaseError,
               CORBA::SystemException);

      /*
       * CF::PortSupplier
       */

      CORBA::Object_ptr getPort (const char *)
        throw (CF::PortSupplier::UnknownPort,
               CORBA::SystemException);
      inline CF::ExecutableDevice::ProcessID_Type getPid() {
        return m_scaPid;
      }
    protected:
      /*
       * Configure and query the worker.
       */

      void configureWorker (const char * name,
                            const CORBA::Any & value,
                            bool last,
                            bool & needSync)
        throw (std::string);

      void queryWorker (const char * name,
                        CORBA::Any & value,
                        bool & haveSync)
        throw (std::string);

#undef CONTROL_OP_I
#define CONTROL_OP_I(x,c,t,s1,s2,s3,s4)
#define CONTROL_OP(x,c,t,s1,s2,s3,s4) virtual void x##Worker();
OCPI_CONTROL_OPS
#undef CONTROL_OP      
#undef CONTROL_OP_I
#define CONTROL_OP_I CONTROL_OP
#if 0
      void controlWorker (WCI_control op,
                          WCI_options flags = WCI_DEFAULT)
        throw (std::string);
#endif

      const OCPI::Util::Property * getProperties (unsigned int & numProperties)
        throw ();

#ifdef TEST
      const OCPI::SCA::Port * findPort (const char * name, unsigned int & portOrdinal)
        throw (std::string);

      const OCPI::SCA::Test * findTest (unsigned int testId)
        throw (CF::TestableObject::UnknownTest);

#endif
      /*
       * Connection management.
       */

      void connectPort (const std::string & portName,
                        const std::string & connectionId,
                        CORBA::Object_ptr connection)
        throw (CF::Port::InvalidPort,
               CF::Port::OccupiedPort,
               CORBA::SystemException);

      void disconnectPort (const std::string & portName,
                           const std::string & connectionId)
        throw (CF::Port::InvalidPort,
               CORBA::SystemException);

    private:
      void disconnectPortLocked (const std::string & portName,
                                 const std::string & connectionId)
        throw (std::string);

      static unsigned int computeMaximumBufferSize (unsigned int memorySize,
                                                    unsigned int bufferCount)
        throw ();

    private:
      struct ConnectionData {
        std::string localPortName;
        CF::Port_var remotePort;
        CC::PortData remotePortData;
        CC::ConnectionCookie * connectionCookie;
      };


      typedef std::map<std::string, Cp289GenericProxyPort*> PortMap;

    private:
      OCPI::API::ContainerApplication &m_application;
      OCPI::API::Worker &m_worker;
      std::string m_name;
      CORBA::Object_var m_scaResource;
      CF::ExecutableDevice::ProcessID_Type m_scaPid;
      PortMap m_portMap;
    };

  }
}

#endif
