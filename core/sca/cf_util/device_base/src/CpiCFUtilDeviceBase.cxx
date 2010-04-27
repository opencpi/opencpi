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
#include <CpiOsAssert.h>
#include <CpiLoggerTee.h>
#include <CpiLoggerLogger.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiUtilAutoMutex.h>
#include <CpiLwLogLoggerOutput.h>
#include <CpiCFUtilMisc.h>
#include <CpiCFUtilReceptacleHelper.h>
#include <CpiStringifyCorbaException.h>
#include "CpiCFUtilDeviceBase.h"

CPI::CFUtil::DeviceBase::
DeviceBase (CORBA::ORB_ptr orb,
	    PortableServer::POA_ptr poa,
	    CF::DeviceManager_ptr devMgr,
	    const std::string & profileFileName,
	    const std::string & deviceId,
	    const std::string & deviceLabel,
	    CPI::Logger::Logger * logger,
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
  cpiAssert (!CORBA::is_nil (m_orb));
  cpiAssert (!CORBA::is_nil (m_poa));

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
    m_producerLogLevel[pli] = static_cast<CPI::CORBAUtil::LwLogLoggerOutput::LogLevelType> (pli+1);
  }
}

CPI::CFUtil::DeviceBase::
~DeviceBase ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * CPI::CFUtil::ReceptacleHelperCallback
 * ----------------------------------------------------------------------
 */

void
CPI::CFUtil::DeviceBase::
connectPort (const std::string & portName,
	     const std::string & connectionId)
  throw (CF::Port::InvalidPort,
	 CF::Port::OccupiedPort,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);

  cpiAssert (portName == "LogOut" ||
	     portName == "EventOut");

  if (portName == "LogOut") {
    CPI::CORBAUtil::LwLogLoggerOutput::LogProducer_var log = m_logPort.getConnection ();
    m_logOut.setLogService (log);
  }

  CPI::Logger::DebugLogger debug (m_out);
  debug << m_logProducerName
	<< CPI::Logger::Verbosity (1)
	<< "Port \""
	<< portName
	<< "\" connected, connection id \""
	<< connectionId
	<< "\"."
	<< std::flush;
}

void
CPI::CFUtil::DeviceBase::
disconnectPort (const std::string & portName,
		const std::string & connectionId)
  throw (CF::Port::InvalidPort,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (1)
	<< "Disconnecting from port \""
	<< portName
	<< "\" connection id \""
	<< connectionId
	<< "\"."
	<< std::flush;

  cpiAssert (portName == "LogOut" ||
	     portName == "EventOut");

  if (portName == "LogOut") {
    m_logOut.setLogService (CPI::CORBAUtil::LwLogLoggerOutput::LogProducer::_nil ());
  }
}

/*
 * ----------------------------------------------------------------------
 * CF::LifeCycle
 * ----------------------------------------------------------------------
 */

void
CPI::CFUtil::DeviceBase::
initialize ()
  throw (CF::LifeCycle::InitializeError,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (1)
	<< "Initializing."
	<< std::flush;
}

void
CPI::CFUtil::DeviceBase::
releaseObject ()
  throw (CF::LifeCycle::ReleaseError,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (1)
	<< "Releasing."
	<< std::flush;

  try {
    PortableServer::ObjectId_var oid = m_poa->servant_to_id (this);
    m_poa->deactivate_object (oid);
  }
  catch (const CORBA::Exception & ex) {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Error deactivating this servant: "
	  << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
	  << " (Ignored.)"
	  << std::flush;
  }

  if (m_shutdownOrbOnRelease) {
    debug << m_logProducerName
	  << CPI::Logger::Verbosity (1)
	  << "Shutting down the ORB."
	  << std::flush;

    try {
      m_orb->shutdown (0);
    }
    catch (const CORBA::Exception & ex) {
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Error shutting down the ORB: "
	    << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
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
CPI::CFUtil::DeviceBase::
runTest (CORBA::ULong testId,
	 CF::Properties & testValues)
  throw (CF::TestableObject::UnknownTest,
	 CF::UnknownProperties,
	 CORBA::SystemException)
{
  CPI::Logger::DebugLogger debug (m_out);

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
	      << CPI::Logger::Verbosity (1)
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
	    << CPI::Logger::Verbosity (2)
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
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
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
CPI::CFUtil::DeviceBase::
configure (const CF::Properties & configProperties)
  throw (CF::PropertySet::InvalidConfiguration,
	 CF::PropertySet::PartialConfiguration,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);
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
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Attempt to configure non-existent properties."
	  << std::flush;

    for (CORBA::ULong pi=0; pi<np; pi++) {
      debug << m_logProducerName
	    << CPI::Logger::Verbosity (2)
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
	<< CPI::Logger::Verbosity (1)
	<< "Configuring zero properties."
	<< std::flush;
}

void
CPI::CFUtil::DeviceBase::
query (CF::Properties & configProperties)
  throw (CF::UnknownProperties,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);
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
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Attempt to query non-existent properties."
	  << std::flush;

    for (CORBA::ULong pi=0; pi<np; pi++) {
      debug << m_logProducerName
	    << CPI::Logger::Verbosity (2)
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
	<< CPI::Logger::Verbosity (1)
	<< "Queried zero properties."
	<< std::flush;
}

/*
 * ----------------------------------------------------------------------
 * CF::PortSupplier
 * ----------------------------------------------------------------------
 */

CORBA::Object_ptr
CPI::CFUtil::DeviceBase::
getPort (const char * name)
  throw (CF::PortSupplier::UnknownPort,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (1)
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
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
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
CPI::CFUtil::DeviceBase::
identifier ()
  throw (CORBA::SystemException)
{ 
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Identifier attribute queried: \""
	<< m_identifier
	<< "\"."
	<< std::flush;

  return CORBA::string_dup (m_identifier.c_str());
}

void
CPI::CFUtil::DeviceBase::
start ()
  throw (CF::Resource::StartError,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Started. (No-op.)"
	<< std::flush;
}

void
CPI::CFUtil::DeviceBase::
stop ()
  throw (CF::Resource::StopError,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
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
CPI::CFUtil::DeviceBase::
usageState ()
  throw (CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Usage state attribute queried: \""
	<< CPI::CFUtil::usageTypeToString (m_usageState)
	<< "\"."
	<< std::flush;

  return m_usageState;
}

CF::Device::AdminType
CPI::CFUtil::DeviceBase::
adminState ()
  throw (CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Admin state attribute queried: \""
	<< CPI::CFUtil::adminTypeToString (m_adminState)
	<< "\"."
	<< std::flush;

  return m_adminState;
}

void
CPI::CFUtil::DeviceBase::
adminState (CF::Device::AdminType state)
  throw (CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  adminStateLocked (state);
}

CF::Device::OperationalType
CPI::CFUtil::DeviceBase::
operationalState ()
  throw (CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Operational state attribute queried: \""
	<< CPI::CFUtil::operationalTypeToString (m_operationalState)
	<< "\"."
	<< std::flush;

  return m_operationalState;
}

char *
CPI::CFUtil::DeviceBase::
label ()
  throw (CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Label attribute queried: \""
	<< m_label
	<< "\"."
	<< std::flush;

  return CORBA::string_dup (m_label.c_str());
}

char *
CPI::CFUtil::DeviceBase::
softwareProfile ()
  throw (CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Software profile attribute queried: \""
	<< m_softwareProfile
	<< "\"."
	<< std::flush;

  return CORBA::string_dup (m_softwareProfile.c_str());
}

CF::AggregateDevice_ptr
CPI::CFUtil::DeviceBase::
compositeDevice ()
  throw (CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Composite Device attribute queried: \""
	<< "{nil}"
	<< "\"."
	<< std::flush;

  return CF::AggregateDevice::_nil ();
}

CORBA::Boolean
CPI::CFUtil::DeviceBase::
allocateCapacity (const CF::Properties & capacities)
  throw (CF::Device::InvalidCapacity,
	 CF::Device::InvalidState,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);
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
      msg += CPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
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
	<< CPI::Logger::Verbosity (2)
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
	  m_out << CPI::Logger::Level::EXCEPTION_ERROR
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
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
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
CPI::CFUtil::DeviceBase::
deallocateCapacity (const CF::Properties & capacities)
  throw (CF::Device::InvalidCapacity,
	 CF::Device::InvalidState,
	 CORBA::SystemException)
{
  CPI::Util::AutoMutex mutex (m_mutex);
  CPI::Logger::DebugLogger debug (m_out);

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
      msg += CPI::CFUtil::adminTypeToString (m_adminState);
    }
    else {
      msg = "Operational state is ";
      msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
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
	<< CPI::Logger::Verbosity (2)
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
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
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
CPI::CFUtil::DeviceBase::
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

  const CPI::CORBAUtil::LwLogLoggerOutput::LogLevelSequence * clevels;
  const ::CORBA::UShortSeq * levels;
  const ::CORBA::LongSeq * llevels;

  if ((property.value >>= clevels)) {
    m_producerLogLevel = *clevels;
  }
  else if ((property.value >>= levels)) {
    CORBA::ULong nls = levels->length ();
    m_producerLogLevel.length (nls);

    for (CORBA::ULong li=0; li<nls; li++) {
      m_producerLogLevel[li] = static_cast<CPI::CORBAUtil::LwLogLoggerOutput::LogLevelType> ((*levels)[li]);
    }
  }
  else if ((property.value >>= llevels)) {
    CORBA::ULong nls = llevels->length ();
    m_producerLogLevel.length (nls);

    for (CORBA::ULong li=0; li<nls; li++) {
      m_producerLogLevel[li] = static_cast<CPI::CORBAUtil::LwLogLoggerOutput::LogLevelType> ((*llevels)[li]);
    }
  }
  else {
    return false;
  }

  m_logOut.setLogLevels (m_producerLogLevel);
  return true;
}

void
CPI::CFUtil::DeviceBase::
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
CPI::CFUtil::DeviceBase::
sBit ()
  throw (CF::TestableObject::UnknownTest,
	 std::string)
{
  return cBit ();
}

bool
CPI::CFUtil::DeviceBase::
oBit ()
  throw (CF::TestableObject::UnknownTest,
	 std::string)
{
  return cBit ();
}

bool
CPI::CFUtil::DeviceBase::
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
CPI::CFUtil::DeviceBase::
adminStateLocked (CF::Device::AdminType state)
  throw ()
{
  /*
   * Note: assumes that the caller holds the mutex.
   */

  CPI::Logger::DebugLogger debug (m_out);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
	<< CPI::Logger::Verbosity (2)
	<< "Request to change admin state from \""
	<< CPI::CFUtil::adminTypeToString (m_adminState)
	<< "\" to \""
	<< CPI::CFUtil::adminTypeToString (state)
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
	      << CPI::Logger::Verbosity (2)
	      << "Not transitioning from \""
	      << CPI::CFUtil::adminTypeToString (m_adminState)
	      << "\" to \""
	      << CPI::CFUtil::adminTypeToString (state)
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
	  << CPI::Logger::Verbosity (2)
	  << "Not transitioning from \""
	  << CPI::CFUtil::adminTypeToString (m_adminState)
	  << "\" to \""
	  << CPI::CFUtil::adminTypeToString (state)
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
	    << CPI::Logger::Verbosity (2)
	    << "Not transitioning from \""
	    << CPI::CFUtil::adminTypeToString (m_adminState)
	    << "\" to \""
	    << CPI::CFUtil::adminTypeToString (state)
	    << "\" because this state transition is illegal. (Ignored.)"
	    << std::flush;
      break;
    }

    break;

  default:
    debug << m_logProducerName
	  << CPI::Logger::Verbosity (2)
	  << "Unknown target state (value "
	  << static_cast<unsigned int> (state)
	  << "). (Ignored.)"
	  << std::flush;
  }

  if (state != oldState) {
    m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	  << m_logProducerName
	  << "Administrative state changed from \""
	  << CPI::CFUtil::adminTypeToString (oldState)
	  << "\" to \""
	  << CPI::CFUtil::adminTypeToString (state)
	  << "\"."
	  << std::flush;
  }
}

void
CPI::CFUtil::DeviceBase::
usageStateLocked (CF::Device::UsageType state)
  throw ()
{
  /*
   * Note: assumes that the caller holds the mutex.
   */

  CPI::Logger::DebugLogger debug (m_out);

  cpiAssert (state == CF::Device::IDLE ||
	     state == CF::Device::ACTIVE ||
	     state == CF::Device::BUSY);

  if (state == m_usageState) {
    /* No-op. */
    return;
  }

  CF::Device::UsageType oldState = m_usageState;
  m_usageState = state;

  StandardEvent::StateChangeType oldType, newType;

  switch (oldState) {
  case CF::Device::IDLE:   oldType = StandardEvent::IDLE;   break;
  case CF::Device::ACTIVE: oldType = StandardEvent::ACTIVE; break;
  case CF::Device::BUSY:   oldType = StandardEvent::BUSY;   break;
  default: cpiAssert (0);
  }

  switch (state) {
  case CF::Device::IDLE:   newType = StandardEvent::IDLE;   break;
  case CF::Device::ACTIVE: newType = StandardEvent::ACTIVE; break;
  case CF::Device::BUSY:   newType = StandardEvent::BUSY;   break;
  default: cpiAssert (0);
  }

  publishStateChangeEvent (StandardEvent::USAGE_STATE_EVENT,
			   oldType,
			   newType);

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Usage state changed from \""
	<< CPI::CFUtil::usageTypeToString (oldState)
	<< "\" to \""
	<< CPI::CFUtil::usageTypeToString (state)
	<< "\"."
	<< std::flush;
}

void
CPI::CFUtil::DeviceBase::
operationalStateLocked (CF::Device::OperationalType state)
  throw ()
{
  /*
   * Note: assumes that the caller holds the mutex.
   */

  CPI::Logger::DebugLogger debug (m_out);

  cpiAssert (state == CF::Device::ENABLED ||
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

  m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
	<< m_logProducerName
	<< "Operational state changed from \""
	<< CPI::CFUtil::operationalTypeToString (oldState)
	<< "\" to \""
	<< CPI::CFUtil::operationalTypeToString (state)
	<< "\"."
	<< std::flush;
}

void
CPI::CFUtil::DeviceBase::
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
      m_out << CPI::Logger::Level::EXCEPTION_ERROR
	    << m_logProducerName
	    << "Failed to publish state change event: "
	    << CPI::CORBAUtil::Misc::stringifyCorbaException (ex)
	    << std::flush;
    }
  }
  else {
    m_out << CPI::Logger::Level::EXCEPTION_ERROR
	  << m_logProducerName
	  << "Failed to publish state change event: "
	  << "Event port is not connected."
	  << std::flush;
  }
}
