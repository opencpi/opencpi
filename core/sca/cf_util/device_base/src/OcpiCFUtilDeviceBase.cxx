
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

/*
 * Base class for SCA Device implementations.
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

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <CF_s.h>
#include <StandardEvent.h>
#include <CosEventComm.h>
#include <OcpiOsAssert.h>
#include <OcpiLoggerTee.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiLwLogLoggerOutput.h>
#include <OcpiCFUtilMisc.h>
#include <OcpiCFUtilReceptacleHelper.h>
#include <OcpiStringifyCorbaException.h>
#include "OcpiCFUtilDeviceBase.h"

OCPI::CFUtil::DeviceBase::
DeviceBase (CORBA::ORB_ptr orb,
            PortableServer::POA_ptr poa,
            CF::DeviceManager_ptr devMgr,
            const std::string & profileFileName,
            const std::string & deviceId,
            const std::string & deviceLabel,
            OCPI::Logger::Logger * logger,
            bool adoptLogger,
            bool shutdownOrbOnRelease)
  throw (std::string)
  : m_disabled (false),
    m_orb (CORBA::ORB::_duplicate (orb)),
    m_poa (PortableServer::POA::_duplicate (poa)),
    m_shutdownOrbOnRelease (shutdownOrbOnRelease),
    m_label (deviceLabel),
    m_identifier (deviceId),
    m_profileFileName (profileFileName),
    m_usageState (CF::Device::IDLE),
    m_adminState (CF::Device::UNLOCKED),
    m_operationalState (CF::Device::ENABLED),
    m_deviceManager (CF::DeviceManager::_duplicate (devMgr)),
    m_totalNumberOfUnits (1),
    m_availableNumberOfUnits (m_totalNumberOfUnits),
    m_logPort (m_poa, "LogOut", this),
    m_eventPort (m_poa, "EventOut", this),
    m_logProducerName (m_label)
{
  ocpiAssert (!CORBA::is_nil (m_orb));
  ocpiAssert (!CORBA::is_nil (m_poa));

  if (!m_profileFileName.empty()) {
    m_softwareProfile  = "<profile filename=\"";
    m_softwareProfile += m_profileFileName;
    m_softwareProfile += "\" type=\"SPD\"/>";
  }

  m_out.addOutput (m_logOut, false, true);

  if (logger) {
    m_out.addOutput (logger, adoptLogger);
  }

  m_out.setProducerId (m_identifier.c_str ());

  m_producerLogLevel.length (26);
  for (CORBA::ULong pli=0; pli<26; pli++) {
    m_producerLogLevel[pli] = static_cast<OCPI::CORBAUtil::LwLogLoggerOutput::LogLevelType> (pli+1);
  }
}

OCPI::CFUtil::DeviceBase::
~DeviceBase ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * OCPI::CFUtil::ReceptacleHelperCallback
 * ----------------------------------------------------------------------
 */

void
OCPI::CFUtil::DeviceBase::
connectPort (const std::string & portName,
             const std::string & connectionId)
  throw (CF::Port::InvalidPort,
         CF::Port::OccupiedPort,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);

  ocpiAssert (portName == "LogOut" ||
             portName == "EventOut");

  if (portName == "LogOut") {
    OCPI::CORBAUtil::LwLogLoggerOutput::LogProducer_var log = m_logPort.getConnection ();
    m_logOut.setLogService (log);
  }

  OCPI::Logger::DebugLogger debug (m_out);
  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Port \""
        << portName
        << "\" connected, connection id \""
        << connectionId
        << "\"."
        << std::flush;
}

void
OCPI::CFUtil::DeviceBase::
disconnectPort (const std::string & portName,
                const std::string & connectionId)
  throw (CF::Port::InvalidPort,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Disconnecting from port \""
        << portName
        << "\" connection id \""
        << connectionId
        << "\"."
        << std::flush;

  ocpiAssert (portName == "LogOut" ||
             portName == "EventOut");

  if (portName == "LogOut") {
    m_logOut.setLogService (OCPI::CORBAUtil::LwLogLoggerOutput::LogProducer::_nil ());
  }
}

/*
 * ----------------------------------------------------------------------
 * CF::LifeCycle
 * ----------------------------------------------------------------------
 */

void
OCPI::CFUtil::DeviceBase::
initialize ()
  throw (CF::LifeCycle::InitializeError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Initializing."
        << std::flush;
}

void
OCPI::CFUtil::DeviceBase::
releaseObject ()
  throw (CF::LifeCycle::ReleaseError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Releasing."
        << std::flush;

  try {
    PortableServer::ObjectId_var oid = m_poa->servant_to_id (this);
    m_poa->deactivate_object (oid);
  }
  catch (const CORBA::Exception & ex) {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "Error deactivating this servant: "
          << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
          << " (Ignored.)"
          << std::flush;
  }

  if (m_shutdownOrbOnRelease) {
    debug << m_logProducerName
          << OCPI::Logger::Verbosity (1)
          << "Shutting down the OCPI_CORBA_ORB."
          << std::flush;

    try {
      m_orb->shutdown (0);
    }
    catch (const CORBA::Exception & ex) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Error shutting down the OCPI_CORBA_ORB: "
            << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
            << " (Ignored.)"
            << std::flush;
    }
  }

  m_disabled = true;
}

/*
 * ----------------------------------------------------------------------
 * CF::TestableObject
 * ----------------------------------------------------------------------
 */

void
OCPI::CFUtil::DeviceBase::
runTest (CORBA::ULong testId,
         CF::Properties & testValues)
  throw (CF::TestableObject::UnknownTest,
         CF::UnknownProperties,
         CORBA::SystemException)
{
  OCPI::Logger::DebugLogger debug (m_out);

  /*
   * Provide some built-in tests.  If a device wants to implement other
   * tests, it can overload this function.
   */

  switch (testId) { 
  case 1:
    /*
     * S-BIT: Return result of start-up test.
     */
    /* fall through */

  case 2:
    /*
     * O-BIT: Perform run-time test.
     */
    /* fall through */

  case 3:
    {
      /*
       * C-BIT: Perform off-line test.
       */
        
      if (testValues.length() != 0) {
        debug << m_logProducerName
              << OCPI::Logger::Verbosity (1)
              << "Failed to run test " << testId
              << ": got non-empty set of input values."
              << std::flush;
        throw CF::UnknownProperties (testValues);
      }

      std::string testMessage;
      bool testResult;

      try {
        if (testId == 1) {
          testResult = sBit ();
        }
        else if (testId == 2) {
          testResult = oBit ();
        }
        else {
          testResult = cBit ();
        }

        if (testResult) {
          testMessage = "Ok";
        }
      }
      catch (const std::string & oops) {
        testResult = false;
        testMessage = oops;
      }

      debug << m_logProducerName
            << OCPI::Logger::Verbosity (2)
            << "Test " << testId
            << (testResult ? " succeeded" : " failed")
            << ", message: \""
            << testMessage << "\"."
            << std::flush;

      testValues.length (2);
      testValues[0].id = "result";
      testValues[0].value <<= CORBA::Any::from_boolean (testResult ? 1 : 0);
      testValues[1].id = "message";
      testValues[1].value <<= testMessage.c_str ();
    }
    break;

  default:
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "Failed to run test " << testId << ": no such test."
          << std::flush;
    throw CF::TestableObject::UnknownTest ();
  }
}

/*
 * ----------------------------------------------------------------------
 * CF::PropertySet
 * ----------------------------------------------------------------------
 */

void
OCPI::CFUtil::DeviceBase::
configure (const CF::Properties & configProperties)
  throw (CF::PropertySet::InvalidConfiguration,
         CF::PropertySet::PartialConfiguration,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);
  CORBA::ULong np = configProperties.length ();

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  /*
   * If a device has configurable properties, this function should be
   * overloaded in a derived class.  If it isn't, then this device
   * does not have any properties.
   */

  if (np) {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "Attempt to configure non-existent properties."
          << std::flush;

    for (CORBA::ULong pi=0; pi<np; pi++) {
      debug << m_logProducerName
            << OCPI::Logger::Verbosity (2)
            << "This device does not have a property named \""
            << configProperties[pi].id.in()
            << "\"."
            << std::flush;
    }

    CF::PropertySet::InvalidConfiguration ic;
    ic.msg = "No such properties.";
    ic.invalidProperties = configProperties;
    throw ic;
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Configuring zero properties."
        << std::flush;
}

void
OCPI::CFUtil::DeviceBase::
query (CF::Properties & configProperties)
  throw (CF::UnknownProperties,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);
  CORBA::ULong np = configProperties.length ();

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  /*
   * If a device has configurable properties, this function should be
   * overloaded in a derived class.  If it isn't, then this device
   * does not have any properties.
   */

  if (np) {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "Attempt to query non-existent properties."
          << std::flush;

    for (CORBA::ULong pi=0; pi<np; pi++) {
      debug << m_logProducerName
            << OCPI::Logger::Verbosity (2)
            << "This device does not have a property named \""
            << configProperties[pi].id.in()
            << "\"."
            << std::flush;
    }

    CF::UnknownProperties up;
    up.invalidProperties = configProperties;
    throw up;
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Queried zero properties."
        << std::flush;
}

/*
 * ----------------------------------------------------------------------
 * CF::PortSupplier
 * ----------------------------------------------------------------------
 */

CORBA::Object_ptr
OCPI::CFUtil::DeviceBase::
getPort (const char * name)
  throw (CF::PortSupplier::UnknownPort,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Query for port \""
        << name
        << "\"."
        << std::flush;

  if (std::strcmp (name, "LogOut") == 0) {
    return m_logPort.getPort ();
  }
  else if (std::strcmp (name, "EventOut") == 0) {
    return m_eventPort.getPort ();
  }
  else {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "This component does not have a port named \""
          << name
          << "\"."
          << std::flush;

    throw CF::PortSupplier::UnknownPort ();
  }
}

/*
 * ----------------------------------------------------------------------
 * CF::Resource
 * ----------------------------------------------------------------------
 */

char *
OCPI::CFUtil::DeviceBase::
identifier ()
  throw (CORBA::SystemException)
{ 
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Identifier attribute queried: \""
        << m_identifier
        << "\"."
        << std::flush;

  return CORBA::string_dup (m_identifier.c_str());
}

void
OCPI::CFUtil::DeviceBase::
start ()
  throw (CF::Resource::StartError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "Started. (No-op.)"
        << std::flush;
}

void
OCPI::CFUtil::DeviceBase::
stop ()
  throw (CF::Resource::StopError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "Stopped. (No-op.)"
        << std::flush;
}

/*
 * ----------------------------------------------------------------------
 * CF::Device
 * ----------------------------------------------------------------------
 */

CF::Device::UsageType
OCPI::CFUtil::DeviceBase::
usageState ()
  throw (CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Usage state attribute queried: \""
        << OCPI::CFUtil::usageTypeToString (m_usageState)
        << "\"."
        << std::flush;

  return m_usageState;
}

CF::Device::AdminType
OCPI::CFUtil::DeviceBase::
adminState ()
  throw (CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Admin state attribute queried: \""
        << OCPI::CFUtil::adminTypeToString (m_adminState)
        << "\"."
        << std::flush;

  return m_adminState;
}

void
OCPI::CFUtil::DeviceBase::
adminState (CF::Device::AdminType state)
  throw (CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  adminStateLocked (state);
}

CF::Device::OperationalType
OCPI::CFUtil::DeviceBase::
operationalState ()
  throw (CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Operational state attribute queried: \""
        << OCPI::CFUtil::operationalTypeToString (m_operationalState)
        << "\"."
        << std::flush;

  return m_operationalState;
}

char *
OCPI::CFUtil::DeviceBase::
label ()
  throw (CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Label attribute queried: \""
        << m_label
        << "\"."
        << std::flush;

  return CORBA::string_dup (m_label.c_str());
}

char *
OCPI::CFUtil::DeviceBase::
softwareProfile ()
  throw (CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Software profile attribute queried: \""
        << m_softwareProfile
        << "\"."
        << std::flush;

  return CORBA::string_dup (m_softwareProfile.c_str());
}

CF::AggregateDevice_ptr
OCPI::CFUtil::DeviceBase::
compositeDevice ()
  throw (CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Composite Device attribute queried: \""
        << "{nil}"
        << "\"."
        << std::flush;

  return CF::AggregateDevice::_nil ();
}

CORBA::Boolean
OCPI::CFUtil::DeviceBase::
allocateCapacity (const CF::Properties & capacities)
  throw (CF::Device::InvalidCapacity,
         CF::Device::InvalidState,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);
  CORBA::Boolean good = 1;

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  if (m_adminState != CF::Device::UNLOCKED ||
      m_operationalState == CF::Device::DISABLED) {
    /*
     * Spec says that we should raise the InvalidState exception in
     * this case.
     */

    std::string msg;

    if (m_adminState != CF::Device::UNLOCKED) {
      msg = "Administrative state is ";
      msg += OCPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += OCPI::CFUtil::operationalTypeToString (m_operationalState);
    }

    CF::Device::InvalidState is;
    is.msg = msg.c_str ();
    throw is;
  }

  /*
   * By default, this device offers a "numberOfUnits" capacity.  If a
   * device has capacities, or if "numberOfUnits" is not an appropriate
   * capacity for a device, this function should be overloaded in a
   * derived class.  The logic below assumes that "numberOfUnits" is the
   * only capacity.
   */

  CORBA::ULong numCaps = capacities.length ();
  CF::Properties invalidCapacities;
  CORBA::ULong numInvalidCapacities = 0;

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "allocateCapacity (";

  for (CORBA::ULong dci=0; dci<numCaps; dci++) {
    if (dci > 0) {
      debug << ", ";
    }

    debug << capacities[dci].id;
  }

  debug << ")" << std::flush;

  for (CORBA::ULong ci=0; ci<numCaps; ci++) {
    const CF::DataType & capacity = capacities[ci];
    const char * capacityId = capacity.id.in ();

    if (std::strcmp (capacityId, "numberOfUnits") == 0 ||
        std::strcmp (capacityId, "ea7a686f-b851-40b9-b6c4-dfb38123a0f4") == 0) {
      CORBA::ULong value;

      if ((capacity.value >>= value)) {
        if (value > 0 && value <= m_availableNumberOfUnits) {
          m_availableNumberOfUnits -= value;

          if (m_availableNumberOfUnits) {
            usageStateLocked (CF::Device::ACTIVE);
          }
          else {
            usageStateLocked (CF::Device::BUSY);
          }
        }
        else {
          m_out << OCPI::Logger::Level::EXCEPTION_ERROR
                << m_logProducerName
                << "Failed to allocate \"numberOfUnits\" capacity: invalid value: "
                << value << "."
                << std::endl;
          good = 0;
        }
      }
      else {
        invalidCapacities.length (numInvalidCapacities + 1);
        invalidCapacities[numInvalidCapacities++] = capacity;
      }
    }
    else {
      invalidCapacities.length (numInvalidCapacities + 1);
      invalidCapacities[numInvalidCapacities++] = capacity;
    }
  }

  if (numInvalidCapacities) {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "Allocation failed for "
          << ((numInvalidCapacities != 1) ? "capacities " : "capacity ");

    for (CORBA::ULong dci=0; dci<numInvalidCapacities; dci++) {
      if (dci > 0 && dci+1 == numInvalidCapacities) {
        m_out << " and ";
      }
      else if (dci > 0) {
        m_out << ", ";
      }

      m_out << invalidCapacities[dci].id;
    }

    m_out << "." << std::flush;

    CF::Device::InvalidCapacity ic;
    ic.msg = "Invalid capacities";
    ic.capacities = invalidCapacities;
    throw ic;
  }

  return good;
}

void
OCPI::CFUtil::DeviceBase::
deallocateCapacity (const CF::Properties & capacities)
  throw (CF::Device::InvalidCapacity,
         CF::Device::InvalidState,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  if (m_adminState != CF::Device::UNLOCKED ||
      m_operationalState == CF::Device::DISABLED) {
    /*
     * Spec says that we should raise the InvalidState exception in
     * this case.
     */

    std::string msg;

    if (m_adminState != CF::Device::UNLOCKED) {
      msg = "Administrative state is ";
      msg += OCPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += OCPI::CFUtil::operationalTypeToString (m_operationalState);
    }

    CF::Device::InvalidState is;
    is.msg = msg.c_str ();
    throw is;
  }

  /*
   * By default, this device offers a "numberOfUnits" capacity.  If a
   * device has capacities, or if "numberOfUnits" is not an appropriate
   * capacity for a device, this function should be overloaded in a
   * derived class.  The logic below assumes that "numberOfUnits" is the
   * only capacity.
   */

  CORBA::ULong numCaps = capacities.length ();
  CF::Properties invalidCapacities;
  CORBA::ULong numInvalidCapacities = 0;

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "deallocateCapacity (";

  for (CORBA::ULong dci=0; dci<numCaps; dci++) {
    if (dci > 0) {
      debug << ", ";
    }

    debug << capacities[dci].id;
  }

  debug << ")" << std::flush;

  for (CORBA::ULong ci=0; ci<numCaps; ci++) {
    const CF::DataType & capacity = capacities[ci];
    const char * capacityId = capacity.id.in ();

    if (std::strcmp (capacityId, "numberOfUnits") == 0 ||
        std::strcmp (capacityId, "ea7a686f-b851-40b9-b6c4-dfb38123a0f4") == 0) {
      CORBA::ULong value;

      if ((capacity.value >>= value) && value > 0 && value <= (m_totalNumberOfUnits - m_availableNumberOfUnits)) {
        m_availableNumberOfUnits += value;

        if (m_availableNumberOfUnits != m_totalNumberOfUnits) {
          usageStateLocked (CF::Device::ACTIVE);
        }
        else {
          usageStateLocked (CF::Device::IDLE);

          /*
           * If this device is shutting down, then set the admin state to
           * LOCKED, now that the last capacity has been released.
           */

          if (m_adminState == CF::Device::SHUTTING_DOWN) {
            adminStateLocked (CF::Device::LOCKED);
          }
        }
      }
      else {
        invalidCapacities.length (numInvalidCapacities + 1);
        invalidCapacities[numInvalidCapacities++] = capacity;
      }
    }
    else {
      invalidCapacities.length (numInvalidCapacities + 1);
      invalidCapacities[numInvalidCapacities++] = capacity;
    }
  }

  if (numInvalidCapacities) {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "Allocation failed for "
          << ((numInvalidCapacities != 1) ? "capacities " : "capacity ");

    for (CORBA::ULong dci=0; dci<numInvalidCapacities; dci++) {
      if (dci > 0 && dci+1 == numInvalidCapacities) {
        m_out << " and ";
      }
      else if (dci > 0) {
        m_out << ", ";
      }

      m_out << invalidCapacities[dci].id;
    }

    m_out << "." << std::flush;

    CF::Device::InvalidCapacity ic;
    ic.msg = "Invalid capacities";
    ic.capacities = invalidCapacities;
    throw ic;
  }
}

/*
 * ----------------------------------------------------------------------
 * Helper functions to configure and query PRODUCER_LOG_LEVEL.
 * ----------------------------------------------------------------------
 */

bool
OCPI::CFUtil::DeviceBase::
configureProducerLogLevel (const CF::DataType & property)
  throw ()
{
  /*
   * Logically, this PRODUCER_LOG_LEVEL property should be of type
   * CosLwLog::LogLevelSequence, so we try that first.
   *
   * However, this type can not be expressed in an SCA property file,
   * which allows for sequences of basic types only.  JTAP complains
   * if this property's type is different than a sequence of unsigned
   * short.
   *
   * SCARI++ (at least the 2.2 incarnation) instead uses a sequence
   * of longs ...
   */

  const OCPI::CORBAUtil::LwLogLoggerOutput::LogLevelSequence * clevels;
  const ::CORBA::UShortSeq * levels;
  const ::CORBA::LongSeq * llevels;

  if ((property.value >>= clevels)) {
    m_producerLogLevel = *clevels;
  }
  else if ((property.value >>= levels)) {
    CORBA::ULong nls = levels->length ();
    m_producerLogLevel.length (nls);

    for (CORBA::ULong li=0; li<nls; li++) {
      m_producerLogLevel[li] = static_cast<OCPI::CORBAUtil::LwLogLoggerOutput::LogLevelType> ((*levels)[li]);
    }
  }
  else if ((property.value >>= llevels)) {
    CORBA::ULong nls = llevels->length ();
    m_producerLogLevel.length (nls);

    for (CORBA::ULong li=0; li<nls; li++) {
      m_producerLogLevel[li] = static_cast<OCPI::CORBAUtil::LwLogLoggerOutput::LogLevelType> ((*llevels)[li]);
    }
  }
  else {
    return false;
  }

  m_logOut.setLogLevels (m_producerLogLevel);
  return true;
}

void
OCPI::CFUtil::DeviceBase::
queryProducerLogLevel (CF::DataType & property)
  throw ()
{
  /*
   * This implementation pleases JTAP but not SCARI++ 2.2.
   */

  CORBA::ULong nls = m_producerLogLevel.length();
  CORBA::UShortSeq levels (nls);
  levels.length (nls);

  for (CORBA::ULong li=0; li<nls; li++) {
    levels[li] = m_producerLogLevel[li];
  }

  property.value <<= levels;
}

/*
 * ----------------------------------------------------------------------
 * Built-in tests (default implementation).
 * ----------------------------------------------------------------------
 */

bool
OCPI::CFUtil::DeviceBase::
sBit ()
  throw (CF::TestableObject::UnknownTest,
         std::string)
{
  return cBit ();
}

bool
OCPI::CFUtil::DeviceBase::
oBit ()
  throw (CF::TestableObject::UnknownTest,
         std::string)
{
  return cBit ();
}

bool
OCPI::CFUtil::DeviceBase::
cBit ()
  throw (CF::TestableObject::UnknownTest,
         std::string)
{
  throw CF::TestableObject::UnknownTest ();
}

/*
 * ----------------------------------------------------------------------
 * Internal API.
 * ----------------------------------------------------------------------
 */

void
OCPI::CFUtil::DeviceBase::
adminStateLocked (CF::Device::AdminType state)
  throw ()
{
  /*
   * Note: assumes that the caller holds the mutex.
   */

  OCPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Request to change admin state from \""
        << OCPI::CFUtil::adminTypeToString (m_adminState)
        << "\" to \""
        << OCPI::CFUtil::adminTypeToString (state)
        << "\"."
        << std::flush;

  CF::Device::AdminType oldState = m_adminState;

  switch (state) {
  case CF::Device::LOCKED:
    switch (m_adminState) {
    case CF::Device::LOCKED:
      /* No-op. */
      break;

    case CF::Device::UNLOCKED:
      m_adminState = CF::Device::SHUTTING_DOWN;
      publishStateChangeEvent (StandardEvent::ADMINISTRATIVE_STATE_EVENT,
                               StandardEvent::UNLOCKED,
                               StandardEvent::SHUTTING_DOWN);
      /* Fall-through. */

    case CF::Device::SHUTTING_DOWN:
      if (m_usageState != CF::Device::IDLE) {
        /* Must wait until device is idle before transitioning to LOCKED. */
        debug << m_logProducerName
              << OCPI::Logger::Verbosity (2)
              << "Not transitioning from \""
              << OCPI::CFUtil::adminTypeToString (m_adminState)
              << "\" to \""
              << OCPI::CFUtil::adminTypeToString (state)
              << "\" yet because the device is not idle."
              << std::flush;
      }
      else {
        m_adminState = CF::Device::LOCKED;
        publishStateChangeEvent (StandardEvent::ADMINISTRATIVE_STATE_EVENT,
                                 StandardEvent::SHUTTING_DOWN,
                                 StandardEvent::LOCKED);
      }

      break;
    }
    break;

  case CF::Device::SHUTTING_DOWN:
    /*
     * The adminState attribute shall only allow the setting of LOCKED
     * and UNLOCKED values.  Illegal state transitions are ignored.
     */

    debug << m_logProducerName
          << OCPI::Logger::Verbosity (2)
          << "Not transitioning from \""
          << OCPI::CFUtil::adminTypeToString (m_adminState)
          << "\" to \""
          << OCPI::CFUtil::adminTypeToString (state)
          << "\" because this state transition is illegal. (Ignored.)"
          << std::flush;
    break;

  case CF::Device::UNLOCKED:
    switch (m_adminState) {
    case CF::Device::LOCKED:
      m_adminState = CF::Device::UNLOCKED;
      publishStateChangeEvent (StandardEvent::ADMINISTRATIVE_STATE_EVENT,
                               StandardEvent::LOCKED,
                               StandardEvent::UNLOCKED);
      break;

    case CF::Device::UNLOCKED:
      /* No-op. */
      break;

    case CF::Device::SHUTTING_DOWN:
      debug << m_logProducerName
            << OCPI::Logger::Verbosity (2)
            << "Not transitioning from \""
            << OCPI::CFUtil::adminTypeToString (m_adminState)
            << "\" to \""
            << OCPI::CFUtil::adminTypeToString (state)
            << "\" because this state transition is illegal. (Ignored.)"
            << std::flush;
      break;
    }

    break;

  default:
    debug << m_logProducerName
          << OCPI::Logger::Verbosity (2)
          << "Unknown target state (value "
          << static_cast<unsigned int> (state)
          << "). (Ignored.)"
          << std::flush;
  }

  if (state != oldState) {
    m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
          << m_logProducerName
          << "Administrative state changed from \""
          << OCPI::CFUtil::adminTypeToString (oldState)
          << "\" to \""
          << OCPI::CFUtil::adminTypeToString (state)
          << "\"."
          << std::flush;
  }
}

void
OCPI::CFUtil::DeviceBase::
usageStateLocked (CF::Device::UsageType state)
  throw ()
{
  /*
   * Note: assumes that the caller holds the mutex.
   */

  OCPI::Logger::DebugLogger debug (m_out);

  ocpiAssert (state == CF::Device::IDLE ||
             state == CF::Device::ACTIVE ||
             state == CF::Device::BUSY);

  if (state == m_usageState) {
    /* No-op. */
    return;
  }

  CF::Device::UsageType oldState = m_usageState;
  m_usageState = state;

  StandardEvent::StateChangeType oldType = StandardEvent::IDLE;
  StandardEvent::StateChangeType newType = StandardEvent::IDLE;

  switch (oldState) {
  case CF::Device::IDLE:   oldType = StandardEvent::IDLE;   break;
  case CF::Device::ACTIVE: oldType = StandardEvent::ACTIVE; break;
  case CF::Device::BUSY:   oldType = StandardEvent::BUSY;   break;
  default: ocpiCheck (0);
  }

  switch (state) {
  case CF::Device::IDLE:   newType = StandardEvent::IDLE;   break;
  case CF::Device::ACTIVE: newType = StandardEvent::ACTIVE; break;
  case CF::Device::BUSY:   newType = StandardEvent::BUSY;   break;
  default: ocpiCheck (0);
  }

  publishStateChangeEvent (StandardEvent::USAGE_STATE_EVENT,
                           oldType,
                           newType);

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "Usage state changed from \""
        << OCPI::CFUtil::usageTypeToString (oldState)
        << "\" to \""
        << OCPI::CFUtil::usageTypeToString (state)
        << "\"."
        << std::flush;
}

void
OCPI::CFUtil::DeviceBase::
operationalStateLocked (CF::Device::OperationalType state)
  throw ()
{
  /*
   * Note: assumes that the caller holds the mutex.
   */

  OCPI::Logger::DebugLogger debug (m_out);

  ocpiAssert (state == CF::Device::ENABLED ||
             state == CF::Device::DISABLED);

  if (state == m_operationalState) {
    /* No-op. */
    return;
  }

  CF::Device::OperationalType oldState = m_operationalState;
  m_operationalState = state;

  publishStateChangeEvent (StandardEvent::OPERATIONAL_STATE_EVENT,
                           (oldState == CF::Device::ENABLED) ? StandardEvent::ENABLED : StandardEvent::DISABLED,
                           (state    == CF::Device::ENABLED) ? StandardEvent::ENABLED : StandardEvent::DISABLED);

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "Operational state changed from \""
        << OCPI::CFUtil::operationalTypeToString (oldState)
        << "\" to \""
        << OCPI::CFUtil::operationalTypeToString (state)
        << "\"."
        << std::flush;
}

void
OCPI::CFUtil::DeviceBase::
publishStateChangeEvent (StandardEvent::StateChangeCategoryType category,
                         StandardEvent::StateChangeType from,
                         StandardEvent::StateChangeType to)
  throw ()
{
  StandardEvent::StateChangeEventType ev;
  ev.producerId = ev.sourceId = m_identifier.c_str ();
  ev.stateChangeCategory = category;
  ev.stateChangeFrom = from;
  ev.stateChangeTo = to;

  CORBA::Any a;
  a <<= ev;

  if (m_eventPort.isConnected()) {
    try {
      CosEventComm::PushConsumer_var ec = m_eventPort.getConnection ();
      ec->push (a);
    }
    catch (const CORBA::Exception & ex) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Failed to publish state change event: "
            << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
            << std::flush;
    }
  }
  else {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "Failed to publish state change event: "
          << "Event port is not connected."
          << std::flush;
  }
}
