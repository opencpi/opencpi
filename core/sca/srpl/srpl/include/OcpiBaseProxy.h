
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

#ifndef OCPI_BASE_PROXY_H__
#define OCPI_BASE_PROXY_H__

/**
 * \file
 * \brief SCA Base Proxy
 *
 * Revision History:
 *
 *     06/03/2009 - Frank Pilhofer
 *                  Track worker state -- the SCA quietly ignores start and
 *                  stop when the resource is already started and stopped,
 *                  respectively, while the container would throw an error.
 *
 *     02/25/2009 - Frank Pilhofer
 *                  Merged common code from the RCC and RPL generic proxy.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <set>
#include <string>
#include <CF.h>
#include <CF_s.h>
#include <OcpiOsMutex.h>
#include <OcpiLoggerLogger.h>
#include "OcpiMetadataWorker.h"

namespace OCPI {
  namespace SCA {


    enum StartStopState {
      InitialState,
      StartedState,
      StoppedState
    };
    class BaseProxyPort;

    /**
     * \brief Base Proxy
     *
     * Implements the CF::Resource API.
     */

    class BaseProxy : public POA_CF::Resource {
      friend class BaseProxyPort;

#undef CONTROL_OP_I
#define CONTROL_OP_I(x,c,t,s1,s2,s3)
#define CONTROL_OP(x,c,t,s1,s2,s3) virtual void x##Worker() = 0;
OCPI_CONTROL_OPS
#undef CONTROL_OP      
#undef CONTROL_OP_I
#define CONTROL_OP_I CONTROL_OP
    public:

      /**
       * Constructor.
       *
       * \param[in] orb  Used to shutdown the OCPI_CORBA_ORB if \a shutdownOrbOnRelease
       *                 is true.
       * \param[in] poa  Used to deactivate this generic proxy in
       *                 #releaseObject().
       * \param[in] identifier This resource's identifier attribute.
       * \param[in] logger The logger for debug and error messages.
       * \param[in] adoptLogger Whether to delete \a logger in the destructor.
       * \param[in] shutdownOrbOnRelease Whether to call orb->shutdown() from
       *                 the releaseObject() operation.  This is usually
       *                 true in a stand-alone server and false if the OCPI_CORBA_ORB
       *                 is shared.
       * \throw std::string If initialization fails.
       */

      BaseProxy (CORBA::ORB_ptr orb,
                    PortableServer::POA_ptr poa,
                    const std::string & identifier,
                    OCPI::Logger::Logger * logger = 0,
                    bool adoptLogger = true,
                    bool shutdownOrbOnRelease = false)
        throw (std::string);

      ~BaseProxy ()
        throw ();

      /*
       * CF::LifeCycle
       */

      void initialize ()
        throw (CF::LifeCycle::InitializeError,
               CORBA::SystemException);
      void releaseObject ()
        throw (CF::LifeCycle::ReleaseError,
               CORBA::SystemException);

      /*
       * CF::TestableObject
       */

      void runTest (CORBA::ULong, CF::Properties &)
        throw (CF::TestableObject::UnknownTest,
               CF::UnknownProperties,
               CORBA::SystemException);

      /*
       * CF::PropertySet
       */

      void configure (const CF::Properties &)
        throw (CF::PropertySet::InvalidConfiguration,
               CF::PropertySet::PartialConfiguration,
               CORBA::SystemException);
      void query (CF::Properties &)
        throw (CF::UnknownProperties,
               CORBA::SystemException);

      /*
       * CF::Resource
       */

      char * identifier ()
        throw (CORBA::SystemException);

      void start ()
        throw (CF::Resource::StartError,
               CORBA::SystemException);

      void stop ()
        throw (CF::Resource::StopError,
               CORBA::SystemException);

      // These are public for the convenience of derived port types
      // FIXME: perhaps some proxy methods to support derived port types
      bool m_disabled;
      CORBA::ORB_var m_orb;
      PortableServer::POA_var m_poa;
      OCPI::OS::Mutex m_mutex;
      OCPI::Logger::Logger * m_logger;
      OCPI::Logger::ProducerName m_logProducerName;
    protected:
      /*
       * Configure and query the worker.
       */

      virtual void configureWorker (const char * name,
                                    const CORBA::Any & value,
                                    bool last,
                                    bool & needSync)
        throw (std::string) = 0;

      virtual void queryWorker (const char * name,
                                CORBA::Any & value,
                                bool & haveSync)
        throw (std::string) = 0;

#if 0
      virtual void controlWorker (WCI_control op,
                                  WCI_options flags = WCI_DEFAULT)
        throw (std::string) = 0;
#endif

      virtual const OCPI::Metadata::Property * getProperties (unsigned int & numProperties)
        throw () = 0;
#ifdef TEST

      virtual const OCPI::SCA::Port * findPort (const char * name, unsigned int & portOrdinal)
        throw (std::string) = 0;

      virtual const OCPI::SCA::Test * findTest (unsigned int testId)
        throw (CF::TestableObject::UnknownTest) = 0;

      /*
       * Connection management.
       */

      virtual void connectPort (const std::string & portName,
                                const std::string & connectionId,
                                CORBA::Object_ptr connection)
        throw (CF::Port::InvalidPort,
               CF::Port::OccupiedPort,
               CORBA::SystemException) = 0;

      virtual void disconnectPort (const std::string & portName,
                                   const std::string & connectionId)
        throw (CF::Port::InvalidPort,
               CORBA::SystemException) = 0;
#endif

    protected:
      typedef std::set<std::string> StringSet;

    protected:
      StartStopState m_state;
      std::string m_identifier;

      bool m_adoptLogger;
      bool m_shutdownOrbOnRelease;

    };

    /*
     * ----------------------------------------------------------------------
     * Helper for connection management.
     * ----------------------------------------------------------------------
     */

    class BaseProxyPort : virtual public POA_CF::Port {
    public:
      BaseProxyPort (const std::string & portName,
                        OCPI::SCA::BaseProxy * proxy)
        throw ();

      ~BaseProxyPort ()
        throw ();
      void release();
      virtual void connectPort (CORBA::Object_ptr connection, const char * connectionId)
        throw (CF::Port::InvalidPort,
               CF::Port::OccupiedPort,
               CORBA::SystemException);

      virtual void disconnectPort (const char * connectionId)
        throw (CF::Port::InvalidPort,
               CORBA::SystemException);

    protected:
      std::string m_portName;
      OCPI::SCA::BaseProxy * m_proxy;
    };

  }
}

#endif
