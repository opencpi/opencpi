// -*- c++ -*-

#ifndef CPI_SCA_GENERIC_ASSEMBLY_CONTROLLER_H__
#define CPI_SCA_GENERIC_ASSEMBLY_CONTROLLER_H__

/**
 * \file
 * \brief Generic Assembly Controller
 *
 * Revision History:
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <map>
#include <CpiOsMutex.h>
#include <CpiLoggerTee.h>
#include <CpiLoggerLogger.h>
#include <CpiLwLogLoggerOutput.h>
#include <CpiCFUtilReceptacleHelper.h>

namespace CPI {
  namespace SCA {

    /**
     * \brief Generic Assembly Controller
     *
     * This class acts as an SCA Assembly Controller, implementing the
     * CF::Resource API.  It requires no application-specific information
     * (such as properties or XML files) but rather uses incidental
     * information to adapt itself:
     *
     * - It happily returns a port object for any getPort() operation,
     *   and then assumes that this port object is used to connect an
     *   application component of the same name.
     * - When the port is connected, the assembly controller calls query()
     *   on the component and incorporates its set of properties, using
     *   the port name as a prefix.
     * - The start() and stop() operations are delegated to all connected
     *   components.
     *
     * E.g., when a component is connected to the assembly's "foo" port,
     * and the component has a property "bar", then the assembly controller
     * will advertise that it has the "foo.bar" property.
     */

    class GenericAssemblyController :
      virtual public CPI::CFUtil::ReceptacleHelperCallback,
      virtual public POA_CF::Resource
    {
    public:
      /**
       * Constructor.
       *
       * \param[in] orb  Used to shutdown the ORB if \a shutdownOrbOnRelease
       *                 is true.
       * \param[in] poa  Used to deactivate this generic proxy in
       *                 #releaseObject().
       * \param[in] componentIdentifier This resource's identifier attribute.
       *                 This usually comes from the SCA COMPONENT_IDENTIFIER
       *                 execution parameter and is in the format
       *                 "Component_Instantiation_Identifier:Application_Name".
       *                 The instantiation identifier is used as the producer
       *                 id for log messages, while the application name is
       *                 used as the producer name.  If the string does not
       *                 contain a colon, then the \a componentIdentifier is
       *                 used as both instantiation identifier and application
       *                 name.
       * \param[in] logger The logger for logging messages.  If non-null,
       *                 messages are logged to this logger in addition to
       *                 any logging service that is connected to this
       *                 device's LogOut port.
       * \param[in] adoptLogger Whether to delete \a logger in the destructor.
       * \param[in] shutdownOrbOnRelease Whether to call orb->shutdown() from
       *                 the releaseObject() operation.  This is usually
       *                 true in a stand-alone server and false if the ORB
       */

      GenericAssemblyController (CORBA::ORB_ptr orb,
				 PortableServer::POA_ptr poa,
				 const std::string & componentIdentifier,
				 CPI::Logger::Logger * logger = 0,
				 bool adoptLogger = true,
				 bool shutdownOrbOnRelease = false)
	throw (std::string);

      ~GenericAssemblyController ()
	throw ();

      /*
       * CPI::CFUtil::ReceptacleHelperCallback
       */

      void connectPort (const std::string & portName,
			const std::string & connectionId)
	throw (CF::Port::InvalidPort,
	       CF::Port::OccupiedPort,
	       CORBA::SystemException);

      void disconnectPort (const std::string & portName,
			   const std::string & connectionId)
	throw (CF::Port::InvalidPort,
	       CORBA::SystemException);

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

      void runTest (CORBA::ULong testId, CF::Properties & testValues)
	throw (CF::TestableObject::UnknownTest,
	       CF::UnknownProperties,
	       CORBA::SystemException);

      /*
       * CF::PropertySet
       */

      void configure (const CF::Properties & configProperties)
	throw (CF::PropertySet::InvalidConfiguration,
	       CF::PropertySet::PartialConfiguration,
	       CORBA::SystemException);

      void query (CF::Properties & configProperties)
	throw (CF::UnknownProperties,
	       CORBA::SystemException);

      /*
       * CF::PortSupplier
       */

      CORBA::Object_ptr getPort (const char * name)
	throw (CF::PortSupplier::UnknownPort,
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

    protected:
      /*
       * Helper functions to configure and query the PRODUCER_LOG_LEVEL
       * property.
       *
       * Any log producer shall implement the PRODUCER_LOG_LEVEL property.
       *
       * If a derived class overloads configure() or query(), it shall
       * call these functions if the property id is PRODUCER_LOG_LEVEL.
       *
       * If configureProducerLogLevel() returns false, the the caller
       * shall add this property to the list of invalid properties.
       */

      bool configureProducerLogLevel (const CF::DataType & property)
	throw ();

      void queryProducerLogLevel (CF::DataType & property)
	throw ();

    protected:
      /*
       * Helpers.
       */

      void mergeProperties (CF::Properties & set,
			    CORBA::ULong & setSize,
			    const CF::Properties & toMerge,
			    const std::string & propPrefix)
	throw ();

    protected:
      /*
       * General logistics.
       */

      CORBA::ORB_var m_orb;
      PortableServer::POA_var m_poa;
      bool m_shutdownOrbOnRelease;
      CPI::OS::Mutex m_mutex;

      /*
       * Resource logistics.
       */

      std::string m_componentIdentifier;
      std::string m_applicationName;
      std::string m_instantiationIdentifier;

      /*
       * Assembly controller logistics.
       */

      typedef std::map<std::string, CPI::CFUtil::ReceptacleHelper<CF::Resource> * > PortMap;
      PortMap m_portMap;

      typedef std::map<std::string, CF::Properties> SetOfProperties;

      /*
       * Default ports.
       */

      CPI::CFUtil::ReceptacleHelper<CPI::CORBAUtil::LwLogLoggerOutput::LogProducer> m_logPort;

      /*
       * Logging.
       */

      CPI::Logger::Tee m_out;
      CPI::Logger::ProducerName m_logProducerName;
      CPI::CORBAUtil::LwLogLoggerOutput m_logOut;
      CPI::CORBAUtil::LwLogLoggerOutput::LogLevelSequence m_producerLogLevel;
    };

  }
}

#endif
