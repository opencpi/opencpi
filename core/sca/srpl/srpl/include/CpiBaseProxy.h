// -*- c++ -*-

#ifndef CPI_BASE_PROXY_H__
#define CPI_BASE_PROXY_H__

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
#include <wci.h>
#include <CpiOsMutex.h>
#include <CpiLoggerLogger.h>
#include "CpiMetadataWorker.h"

namespace CPI {
  namespace SCA {

    class BaseProxyPort;

    /**
     * \brief Base Proxy
     *
     * Implements the CF::Resource API.
     */

    class BaseProxy : public POA_CF::Resource {
      friend class BaseProxyPort;

#define CONTROL_OP(x,c,t,s1,s2,s3) virtual void x##Worker() = 0;
CPI_CONTROL_OPS
#undef CONTROL_OP      
    public:

      /**
       * Constructor.
       *
       * \param[in] orb  Used to shutdown the ORB if \a shutdownOrbOnRelease
       *                 is true.
       * \param[in] poa  Used to deactivate this generic proxy in
       *                 #releaseObject().
       * \param[in] identifier This resource's identifier attribute.
       * \param[in] logger The logger for debug and error messages.
       * \param[in] adoptLogger Whether to delete \a logger in the destructor.
       * \param[in] shutdownOrbOnRelease Whether to call orb->shutdown() from
       *                 the releaseObject() operation.  This is usually
       *                 true in a stand-alone server and false if the ORB
       *                 is shared.
       * \throw std::string If initialization fails.
       */

      BaseProxy (CORBA::ORB_ptr orb,
                    PortableServer::POA_ptr poa,
                    const std::string & identifier,
                    CPI::Logger::Logger * logger = 0,
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
      CPI::OS::Mutex m_mutex;
      CPI::Logger::Logger * m_logger;
      CPI::Logger::ProducerName m_logProducerName;
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

      virtual const CPI::Metadata::Property * getProperties (unsigned int & numProperties)
        throw () = 0;
#ifdef TEST

      virtual const CPI::SCA::Port * findPort (const char * name, unsigned int & portOrdinal)
        throw (std::string) = 0;

      virtual const CPI::SCA::Test * findTest (unsigned int testId)
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
      WCI_control m_state;
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
                        CPI::SCA::BaseProxy * proxy)
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
      CPI::SCA::BaseProxy * m_proxy;
    };

  }
}

#endif
