
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

#ifndef OCPI_SCA_GENERIC_ASSEMBLY_CONTROLLER_H__
#define OCPI_SCA_GENERIC_ASSEMBLY_CONTROLLER_H__

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
#include <OcpiOsMutex.h>
#include <OcpiLoggerTee.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLwLogLoggerOutput.h>
#include <OcpiCFUtilReceptacleHelper.h>

namespace OCPI {
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
      virtual public OCPI::CFUtil::ReceptacleHelperCallback,
      virtual public POA_CF::Resource
    {
    public:
      /**
       * Constructor.
       *
       * \param[in] orb  Used to shutdown the OCPI_CORBA_ORB if \a shutdownOrbOnRelease
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
       *                 true in a stand-alone server and false if the OCPI_CORBA_ORB
       */

      GenericAssemblyController (CORBA::ORB_ptr orb,
                                 PortableServer::POA_ptr poa,
                                 const std::string & componentIdentifier,
                                 OCPI::Logger::Logger * logger = 0,
                                 bool adoptLogger = true,
                                 bool shutdownOrbOnRelease = false)
        throw (std::string);

      ~GenericAssemblyController ()
        throw ();

      /*
       * OCPI::CFUtil::ReceptacleHelperCallback
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
      OCPI::OS::Mutex m_mutex;

      /*
       * Resource logistics.
       */

      std::string m_componentIdentifier;
      std::string m_applicationName;
      std::string m_instantiationIdentifier;

      /*
       * Assembly controller logistics.
       */

      typedef std::map<std::string, OCPI::CFUtil::ReceptacleHelper<CF::Resource> * > PortMap;
      PortMap m_portMap;

      typedef std::map<std::string, CF::Properties> SetOfProperties;

      /*
       * Default ports.
       */

      OCPI::CFUtil::ReceptacleHelper<OCPI::CORBAUtil::LwLogLoggerOutput::LogProducer> m_logPort;

      /*
       * Logging.
       */

      OCPI::Logger::Tee m_out;
      OCPI::Logger::ProducerName m_logProducerName;
      OCPI::CORBAUtil::LwLogLoggerOutput m_logOut;
      OCPI::CORBAUtil::LwLogLoggerOutput::LogLevelSequence m_producerLogLevel;
    };

  }
}

#endif
