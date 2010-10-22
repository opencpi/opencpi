
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

#ifndef OCPI_CFUTIL_DEVICE_BASE_H__
#define OCPI_CFUTIL_DEVICE_BASE_H__

/**
 * \file
 * \brief Base class for SCA Device implementations.
 *
 * Revision History:
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2 log service.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiOsMutex.h>
#include <OcpiLoggerTee.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLwLogLoggerOutput.h>
#include <OcpiCFUtilReceptacleHelper.h>
#include <CosEventComm.h>
#include <CF_s.h>
#include <StandardEvent.h>

namespace OCPI {
  namespace CFUtil {

    /**
     * \brief Base class to ease the implementation of SCA Devices.
     *
     * This base class implements functionality that is common to SCA
     * Devices and is intended to ease their implementation effort.
     *
     * This includes implementing most operations and attributes with
     * reasonable defaults, the provision of two "uses" ports ("LogOut"
     * for connections to the Log Service and "EventOut" for connections
     * to the SCA Domain Manager's "IDM_Channel"), state management and
     * some helper functions for S-BIT, O-BIT and C-BIT tests.
     *
     * Actual device implementations need to overload some of these
     * methods, all of which are virtual (by inheritance from the
     * POA_CF::Device servant).
     *
     * It is not mandatory to use this base class; if there is too much
     * discrepancy beteen this class and a device's desired behavior, it
     * is perfectly possible to implement an SCA device without it.
     */

    class DeviceBase :
        virtual public OCPI::CFUtil::ReceptacleHelperCallback,
        virtual public POA_CF::Device
    {
    public:
      /**
       * Constructor.
       *
       * \param[in] orb    Used in the releaseObject() implementation.
       * \param[in] poa    used in the releaseObject() implementation.
       * \param[in] devMgr The SCA Device Manager that manages this device.
       *                   May be nil.
       * \param[in] profileFileName The SPD file for this device; this file
       *                   is not read or used but reported as the
       *                   softwareProfile attribute.
       * \param[in] deviceId The UUID for this device.  It is used
       *                   in log messages and usually comes from the
       *                   componentinstantiation element in the DCD file
       *                   via the DEVICE_ID execution parameter.
       * \param[in] deviceLabel The label for this device.  It is
       *                   used in log messages and usually comes from the
       *                   usagename element in the DCD file via the
       *                   DEVICE_LABEL execution parameter.
       * \param[in] logger The logger for logging messages.  If non-null,
       *                   messages are logged to this logger in addition to
       *                   any logging service that is connected to this
       *                   device's LogOut port.
       * \param[in] adoptLogger Whether to delete \a logger in the destructor.
       * \param[in] shutdownOrbOnRelease Whether to call orb->shutdown() from
       *                   the releaseObject() operation.  This is usually
       *                   true in a stand-alone server and false if the OCPI_CORBA_ORB
       *                   is shared.
       */

      DeviceBase (CORBA::ORB_ptr orb,
                  PortableServer::POA_ptr poa,
                  CF::DeviceManager_ptr devMgr,
                  const std::string & profileFileName,
                  const std::string & deviceId,
                  const std::string & deviceLabel,
                  OCPI::Logger::Logger * logger = 0,
                  bool adoptLogger = true,
                  bool shutdownOrbOnRelease = false)
        throw (std::string);

      ~DeviceBase ()
        throw ();

      /**
       * \name Implements OCPI::CFUtil::ReceptacleHelperCallback.
       */

      //@{

      /**
       * Connect the "LogOut" and "EventOut" ports.  If a device supports
       * other ports via OCPI::CFUtil::ReceptacleHelper, this method should
       * be overloaded, and the derived class should call this implementation
       * if \a portName is "LogOut" or "EventOut".
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

      //@}

      /**
       * \name Implements CF::LifeCycle.
       */

      //@{

      /**
       * Default implementation as a no-op.
       */

      void initialize ()
        throw (CF::LifeCycle::InitializeError,
               CORBA::SystemException);

      /**
       * Default implementation.  Deactivates this device and calls
       * orb->shutdown() if the shutdownOrbOnRelease parameter was
       * set to true when constructing this device.
       */

      void releaseObject ()
        throw (CF::LifeCycle::ReleaseError,
               CORBA::SystemException);

      //@}

      /**
       * \name Implements CF::TestableObject.
       */

      //@{

      /**
       * This default implementation provides two built-in tests using
       * test id 1 ("S-BIT"), 2 ("O-BIT") and 3 ("C-BIT").  If a device
       * wants to implement other tests, it can overload this method
       * and optionally call this base implementation for test ids 1
       * to 3.
       *
       * When testId==1, calls sBit().  When testId==2, calls cBit(). When
       * testId==3, calls oBit().  In each case, returns a set of two test
       * results.  testValues[0] is a boolean value indicating whether the
       * test passed or not.  testValues[1] is a string that carries an
       * error message when the test did not pass.  (testValues[1] is
       * present but can be ignored when the test passed.)
       */

      void runTest (CORBA::ULong testId, CF::Properties & testValues)
        throw (CF::TestableObject::UnknownTest,
               CF::UnknownProperties,
               CORBA::SystemException);

      //@}

      /**
       * \name Implements CF::PropertySet.
       */

      //@{

      /**
       * Default implementation.
       * Throws CF::PropertySet::InvalidConfiguration.
       */

      void configure (const CF::Properties & configProperties)
        throw (CF::PropertySet::InvalidConfiguration,
               CF::PropertySet::PartialConfiguration,
               CORBA::SystemException);

      /**
       * Default implementation.
       * Throws CF::UnknownProperties.
       */

      void query (CF::Properties & configProperties)
        throw (CF::UnknownProperties,
               CORBA::SystemException);

      //@}

      /**
       * \name Implements CF::PortSupplier.
       */

      //@{

      /**
       * This default implementation supports two "uses" ports, "EventOut"
       * and "LogOut".  If a device supports other ports, then it should
       * overload this method but call this base class' implementation if
       * the port name is "EventOut" or "LogOut".
       */

      CORBA::Object_ptr getPort (const char * name)
        throw (CF::PortSupplier::UnknownPort,
               CORBA::SystemException);

      //@}

      /**
       * \name Implements CF::Resource.
       */

      //@{

      /**
       * Returns m_identifier, which is initialized from the deviceId
       * parameter to the constructor.
       */

      char * identifier ()
        throw (CORBA::SystemException);

      /**
       * Default implementation as a no-op.
       */

      void start ()
        throw (CF::Resource::StartError,
               CORBA::SystemException);

      /**
       * Default implementation as a no-op.
       */

      void stop ()
        throw (CF::Resource::StopError,
               CORBA::SystemException);

      //@}

      /**
       * \name Implements CF::Device.
       */

      //@{

      /**
       * Default implementation.  Returns m_usageState.
       */

      CF::Device::UsageType usageState ()
        throw (CORBA::SystemException);

      /**
       * Default implementation.  Returns m_adminState.
       */

      CF::Device::AdminType adminState ()
        throw (CORBA::SystemException);

      /**
       * Default implementation.  Updates m_adminState, publishing a
       * state change event if applicable.
       */

      void adminState (CF::Device::AdminType state)
        throw (CORBA::SystemException);

      /**
       * Default implementation.  Returns m_operationalState.
       */

      CF::Device::OperationalType operationalState ()
        throw (CORBA::SystemException);

      /**
       * Returns m_label, which is initialized from the deviceLabel
       * parameter to the constructor.
       */

      char * label ()
        throw (CORBA::SystemException);

      /**
       * Returns m_softwareProfile, which is initialized from the
       * profileFileName parameter to the constructor.
       */

      char * softwareProfile ()
        throw (CORBA::SystemException);

      /**
       * Returns nil.
       */

      CF::AggregateDevice_ptr compositeDevice ()
        throw (CORBA::SystemException);

      /**
       * Default implementation as a no-op.  If a device has capacities,
       * it must overload this method.
       */

      CORBA::Boolean allocateCapacity (const CF::Properties & capacities)
        throw (CF::Device::InvalidCapacity,
               CF::Device::InvalidState,
               CORBA::SystemException);

      /**
       * Default implementation as a no-op.  If a device has capacities,
       * it must overload this method.
       */

      void deallocateCapacity (const CF::Properties & capacities)
        throw (CF::Device::InvalidCapacity,
               CF::Device::InvalidState,
               CORBA::SystemException);

      //@}

    protected:
      /**
       * \name Helpers to configure the PRODUCER_LOG_LEVEL property.
       */

      //@{

      /**
       * If a derived class overloads configure(), it shall
       * call this function if the property id is "PRODUCER_LOG_LEVEL".
       *
       * If configureProducerLogLevel() returns false, the the caller
       * shall add this property to the list of invalid properties.
       *
       * Updates m_producerLogLevel and configures m_logOut appropriately.
       */

      bool configureProducerLogLevel (const CF::DataType & property)
        throw ();

      /**
       * If a derived class overloads query(), it shall
       * call this function if the property id is "PRODUCER_LOG_LEVEL".
       */

      void queryProducerLogLevel (CF::DataType & property)
        throw ();

      //@}

    protected:
      /**
       * \name Helpers for default built-in tests.
       */

      //@{

      /**
       * Built-in S-BIT (Startup Built-In Test).  This function should
       * return the start-up status of the device, if applicable.  I.e.,
       * a device that implements S-BIT should save its initialization
       * status and return it from this method.
       *
       * The implementation shall return true if the S-BIT is successful,
       * false if the S-BIT is unsuccessful.  It may throw an exception
       * of type std::string instead of simply returning false to supply
       * an error message to the client.
       *
       * The default implementation calls cBit ().
       */

      virtual bool sBit ()
        throw (CF::TestableObject::UnknownTest,
               std::string);

      /**
       * Built-in O-BIT (Offline Built-In Test).  This function should
       * check that the device is still functional.  It is assumed that
       * the device is not "running"; testing may "disrupt" the device
       * in ways that would not be appropriate for a C-BIT (continuous)
       * test.
       *
       * The implementation shall return true if the O-BIT is successful,
       * false if the O-BIT is unsuccessful.  It may throw an exception
       * of type std::string instead of simply returning false to supply
       * an error message to the client.
       *
       * The default implementation calls cBit ().
       */

      virtual bool oBit ()
        throw (CF::TestableObject::UnknownTest,
               std::string);

      /**
       * Built-in C-BIT (Continuous Built-In Test).  This function should
       * check that the device is still functional.  Testing shall not
       * interfere with operation of the device (i.e., it shall be possible
       * to run this test while the device is running).
       *
       * The implementation shall return true if the C-BIT is successful,
       * false if the C-BIT is unsuccessful.  It may throw an exception
       * of type std::string instead of simply returning false to supply
       * an error message to the client.
       *
       * The default implementation throws CF::TestableObject::UnknownTest,
       * indicating that C-BIT is not implemented.
       */

      virtual bool cBit ()
        throw (CF::TestableObject::UnknownTest,
               std::string);

      //@}

    protected:
      /**
       * \name Internal API.
       */

      //@{

      void adminStateLocked (CF::Device::AdminType state)
        throw ();

      void usageStateLocked (CF::Device::UsageType state)
        throw ();

      void operationalStateLocked (CF::Device::OperationalType state)
        throw ();

      void publishStateChangeEvent (StandardEvent::StateChangeCategoryType category,
                                    StandardEvent::StateChangeType from,
                                    StandardEvent::StateChangeType to)
        throw ();

      //@}

    protected:
      /**
       * \name General logistics.
       */

      //@{

      bool m_disabled;
      CORBA::ORB_var m_orb;
      PortableServer::POA_var m_poa;
      bool m_shutdownOrbOnRelease;
      OCPI::OS::Mutex m_mutex;

      //@}

      /**
       * \name Device logistics.
       */

      //@{

      std::string m_label;
      std::string m_identifier;
      std::string m_profileFileName;
      std::string m_softwareProfile;
      CF::Device::UsageType m_usageState;
      CF::Device::AdminType m_adminState;
      CF::Device::OperationalType m_operationalState;
      CF::DeviceManager_var m_deviceManager;
      unsigned int m_totalNumberOfUnits;
      unsigned int m_availableNumberOfUnits;

      //@}

      /**
       * \name Default ports.
       */

      //@{

      OCPI::CFUtil::ReceptacleHelper<OCPI::CORBAUtil::LwLogLoggerOutput::LogProducer> m_logPort;
      OCPI::CFUtil::ReceptacleHelper<CosEventComm::PushConsumer> m_eventPort;

      //@}

      /**
       * \name Logging.
       */

      //@{

      OCPI::Logger::Tee m_out;
      OCPI::Logger::ProducerName m_logProducerName;
      OCPI::CORBAUtil::LwLogLoggerOutput m_logOut;
      OCPI::CORBAUtil::LwLogLoggerOutput::LogLevelSequence m_producerLogLevel;

      //@}
    };

  }
}

#endif
