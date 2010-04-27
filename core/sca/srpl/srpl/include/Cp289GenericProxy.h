// -*- c++ -*-

#ifndef CPI_SCA_CP289_GENERIC_PROXY_H__
#define CPI_SCA_CP289_GENERIC_PROXY_H__

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
#include "CpiContainerInterface.h"
#include "CpiContainerPort.h"
#include "CpiBaseProxy.h"
#include "Cp289ProviderPort.h"
#include "CpiUtilIOP.h"

namespace CPI {
  namespace SCA {
    // CORBA-specific for SCA (although it would be shared with other things like CCM)
    enum {
      /*
       * These ought to be registered with the OMG.
       */
      CPI_PORT_INFO_TAG = 0x43504931,
      //      CPI_TARGET_PORT_DATA_TAG = 0x43504931,
      CPI_SOURCE_SHADOW_PORT_DATA_TAG = 0x43504932,
    };
    namespace CC = CPI::Container;
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
      
      CC::Port &m_cpiPort;
      CORBA::Object_var m_scaPort;
      typedef std::set<std::string> ConnectionSet;
      ConnectionSet m_connections;
      inline CORBA::Object_var getScaPort() { return m_scaPort; }
      void getProfile(CORBA::Object_ptr, CPI::Util::IOP::ProfileId, std::string &);
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
       * \param[in] orb  Used to shutdown the ORB if \a shutdownOrbOnRelease
       *                 is true.
       * \param[in] poa  Used to deactivate this generic proxy in
       *                 #releaseObject().
       * \param[in] identifier This resource's identifier attribute.
       * \param[in] cpiDeviceId The device id where this worker is running.
       * \param[in] cpiWorkerId The worker ordinal according to CPI.
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
       *                 true in a stand-alone server and false if the ORB
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
			 CC::Application &appContext,
			 const char *namingContextIor,
			 const char *nameBinding,
			 // Optional
			 CPI::Logger::Logger * logger = 0,
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

#define CONTROL_OP(x,c,t,s1,s2,s3) virtual void x##Worker();
CPI_CONTROL_OPS
#undef CONTROL_OP      
#if 0
      void controlWorker (WCI_control op,
			  WCI_options flags = WCI_DEFAULT)
	throw (std::string);
#endif

      const CPI::Metadata::Property * getProperties (unsigned int & numProperties)
	throw ();

#ifdef TEST
      const CPI::SCA::Port * findPort (const char * name, unsigned int & portOrdinal)
        throw (std::string);

      const CPI::SCA::Test * findTest (unsigned int testId)
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
      CC::Application &m_application;
      CC::Worker &m_worker;
      std::string m_name;
      CORBA::Object_var m_scaResource;
      CF::ExecutableDevice::ProcessID_Type m_scaPid;
      PortMap m_portMap;
    };

  }
}

#endif
