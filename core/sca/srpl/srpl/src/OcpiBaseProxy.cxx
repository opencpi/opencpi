
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
 * SCA Base Proxy
 *
 * Revision History:
 *
 *     06/03/2009 - Frank Pilhofer
 *                  Track worker state -- the SCA quietly ignores start and
 *                  stop when the resource is already started and stopped,
 *                  respectively, while the container would throw an error.
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2.
 *
 *     02/25/2009 - Frank Pilhofer
 *                  Merged common code from the RCC and RPL generic proxy.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include <OcpiUtilAutoMutex.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerNullOutput.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiCFUtilLegacyErrorNumbers.h>
#include <CF.h>
#include <CF_s.h>
#include "OcpiBaseProxy.h"

namespace OCPI {
  namespace SCA {
/*
 * ----------------------------------------------------------------------
 * Constructor and Destructor
 * ----------------------------------------------------------------------
 */
BaseProxy::
BaseProxy (CORBA::ORB_ptr orb,
              PortableServer::POA_ptr poa,
              const std::string & aIdentifier,
              OCPI::Logger::Logger * logger,
              bool adoptLogger,
              bool shutdownOrbOnRelease)
  throw (std::string)
  : m_disabled (false),
    m_orb (CORBA::ORB::_duplicate (orb)),
    m_poa (PortableServer::POA::_duplicate (poa)),
    m_logger (logger),
    m_logProducerName (aIdentifier),
    m_state (InitialState),
    m_identifier (aIdentifier),
    m_adoptLogger (adoptLogger),
    m_shutdownOrbOnRelease (shutdownOrbOnRelease)
{
  if (!m_logger) {
    try {
      m_logger = new OCPI::Logger::NullOutput;
    }
    catch (const std::bad_alloc & oops) {
      throw std::string (oops.what());
    }

    m_adoptLogger = true;
  }

  OCPI::Logger::DebugLogger debug (*m_logger);
  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Constructed."
        << std::flush;
}

BaseProxy::
~BaseProxy ()
  throw ()
{
  OCPI::Logger::DebugLogger debug (*m_logger);
  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Destructed."
        << std::flush;

}

/*
 * ----------------------------------------------------------------------
 * CF::LifeCycle
 * ----------------------------------------------------------------------
 */

void
BaseProxy::
initialize ()
  throw (CF::LifeCycle::InitializeError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  OCPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "initialize ()"
        << std::flush;

  m_state = InitialState; // redundant with constructor?
}

void
BaseProxy::
releaseObject ()
  throw (CF::LifeCycle::ReleaseError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  OCPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "releaseObject ()"
        << std::flush;

  /*
   * We might want to exit at this point.
   */

  if (!CORBA::is_nil (m_poa)) {
    try {
      PortableServer::ObjectId_var oid = m_poa->servant_to_id (this);
      m_poa->deactivate_object (oid);
    }
    catch (...) {
    }
  }

  if (m_shutdownOrbOnRelease) {
    if (!CORBA::is_nil (m_orb)) {
      try {
        m_orb->shutdown (0);
      }
      catch (...) {
      }
    }
  }

  /*
   * Release the worker.
   */

  m_disabled = true;

  try {
    releaseWorker();//controlWorker (WCI_CONTROL_RELEASE);
  }
  catch (const std::string & oops) {
    *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to release: "
              << oops
              << "."
              << std::flush;

    CF::LifeCycle::ReleaseError re;
    re.errorMessages.length (1);
    re.errorMessages[0] = oops.c_str();
    throw re;
  }

  m_state = InitialState;
}

/*
 * ----------------------------------------------------------------------
 * CF::TestableObject
 * ----------------------------------------------------------------------
 */

void
BaseProxy::
runTest (CORBA::ULong testId, CF::Properties & testValues)
  throw (CF::TestableObject::UnknownTest,
         CF::UnknownProperties,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  OCPI::Logger::DebugLogger debug (*m_logger);
  CF::UnknownProperties up;
  CORBA::ULong numProps = testValues.length ();
#if 0
  CORBA::ULong numInvalidProps = 0;
  bool needSync = false;
  bool haveSync = true;
#endif

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "testId ("
        << testId;

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    debug << ", ";
    debug << testValues[dpi].id;
  }

  debug << ")" << std::flush;

#ifdef TEST
  const Test * td;

  try {
    td = findTest (testId);
  }
  catch (const CF::TestableObject::UnknownTest &) {
    *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Unknown test "
              << testId << "."
              << std::flush;
    throw;
  }

  try {
    /*
     * Set testId property.
     */

    CORBA::Any aTestId;
    aTestId <<= testId;

    configureWorker ("testId", aTestId, false, needSync);

    /*
     * Get list of properties.
     */

    unsigned int numProperties;
    const OCPI::Metadata::Property * props = getProperties (numProperties);

    /*
     * Set input values.
     */

    StringSet expectedInputs;

    for (unsigned int tivi=0; tivi<td->numInputs; tivi++) {
      unsigned int pn = td->inputValues[tivi];
      ocpiAssert (pn < numProperties);
      expectedInputs.insert (props[pn].name);
    }

    for (CORBA::ULong pi=0; pi<numProps; pi++) {
      const CF::DataType & property = testValues[pi];
      StringSet::iterator eii = expectedInputs.find (property.id.in());

      if (eii == expectedInputs.end()) {
        up.invalidProperties.length (numInvalidProps + 1);
        up.invalidProperties[numInvalidProps++] = property;
        *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                  << m_logProducerName
                  << "Error: \""
                  << property.id
                  << "\" is not an input value to test "
                  << testId
                  << "."
                  << std::flush;
        continue;
      }

      expectedInputs.erase (eii);

      try {
        configureWorker (property.id,
                         property.value,
                         false,
                         needSync);
      }
      catch (const std::string & oops) {
        up.invalidProperties.length (numInvalidProps + 1);
        up.invalidProperties[numInvalidProps++] = property;
        *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                  << m_logProducerName
                  << "Error configuring test input value \""
                  << property.id
                  << "\": "
                  << oops
                  << "."
                  << std::flush;
      }
    }

    // FIXME: allow default values of input properties for tests?
    /*
     * Did the test get all input values that it expects?
     */

    if (!expectedInputs.empty()) {
      *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                << m_logProducerName
                << "Missing input value";

      if (expectedInputs.size() != 1) {
        *m_logger << "s";
      }

      *m_logger << " ";

      for (StringSet::iterator eii = expectedInputs.begin();
           eii != expectedInputs.end(); eii++) {
        if (eii != expectedInputs.begin()) {
          *m_logger << ", ";
        }
        *m_logger << *eii;

        up.invalidProperties.length (numInvalidProps + 1);
        up.invalidProperties[numInvalidProps++].id = (*eii).c_str ();
      }

      *m_logger << " for test " << testId << "." << std::flush;
    }

    if (numInvalidProps) {
      throw up;
    }

    /*
     * Run test.
     */

    try {
      controlWorker (WCI_CONTROL_TEST);
    }
    catch (const std::string & oops) {
      *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                << m_logProducerName
                << "Failed to run test \""
                << testId << ": "
                << oops
                << "."
                << std::flush;

      throw CF::TestableObject::UnknownTest ();
    }

    /*
     * Read result values.
     */

    numProps = td->numResults;
    testValues.length (numProps);

    for (CORBA::ULong pi=0; pi<numProps; pi++) {
      unsigned int pn = td->resultValues[pi];
      ocpiAssert (pn < numProperties);

      CF::DataType & property = testValues[pi];
      const char * propName = props[pn].name;
      property.id = propName;

      queryWorker (propName, property.value, haveSync);
    }
  }
  catch (const std::string & oops) {
    *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to run test "
              << testId << ": "
              << oops << "."
              << std::flush;
    throw CF::TestableObject::UnknownTest ();
  }
#endif
}

/*
 * ----------------------------------------------------------------------
 * CF::PropertySet
 * ----------------------------------------------------------------------
 */

void
BaseProxy::
configure (const CF::Properties & props)
  throw (CF::PropertySet::InvalidConfiguration,
         CF::PropertySet::PartialConfiguration,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  OCPI::Logger::DebugLogger debug (*m_logger);
  CF::Properties invalidProperties;
  CORBA::ULong numProps = props.length ();
  CORBA::ULong numInvalidProps = 0;
  bool needSync = false;

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "configure (";

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    if (dpi > 0) {
      debug << ", ";
    }

    debug << props[dpi].id;
  }

  debug << ")" << std::flush;

  for (CORBA::ULong pi=0; pi<numProps; pi++) {
    const CF::DataType & property = props[pi];
    bool last = ((pi == numProps-1) ? true : false);

    try {
      configureWorker (property.id,
                       property.value,
                       last,
                       needSync);
    }
    catch (const std::string & oops) {
      invalidProperties.length (numInvalidProps + 1);
      invalidProperties[numInvalidProps++] = property;
      *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                << m_logProducerName
                << "Error configuring property \""
                << property.id
                << "\": "
                << oops
                << "."
                << std::flush;
    }
  }

  if (numInvalidProps && numInvalidProps == numProps) {
    // No property was successfully configured.
    CF::PropertySet::InvalidConfiguration ic;
    ic.msg = "configure failed";
    ic.invalidProperties = invalidProperties;
    throw ic;
  }
  else if (numInvalidProps) {
    // Some, but not all properties were successfully configured.
    CF::PropertySet::PartialConfiguration pc;
    pc.invalidProperties = invalidProperties;
    throw pc;
  }
}

void
BaseProxy::
query (CF::Properties & props)
  throw (CF::UnknownProperties,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  OCPI::Logger::DebugLogger debug (*m_logger);
  CORBA::ULong numProps = props.length ();
  CORBA::ULong numInvalidProps = 0;
  CF::UnknownProperties up;
  bool haveSync = false;

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "query (";

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    if (dpi > 0) {
      debug << ", ";
    }

    debug << props[dpi].id;
  }

  debug << ")" << std::flush;

  if (!numProps) {
    /*
     * We're meant to return all properties.  Initialize props with all
     * property names, so that the code below fills them in.  Ignore
     * properties that are not readable, else the code below would fail
     * to query them.
     */

    unsigned int numProperties;



    const OCPI::Metadata::Property * cprops = getProperties (numProperties);

    numProps = numProperties;
    props.length (numProps);
    CORBA::ULong pc = 0;

    for (CORBA::ULong pi=0; pi<numProps; pi++) {
      if (cprops[pi].m_isReadable && !cprops[pi].isTest) {
        props[pc++].id = CORBA::string_dup (cprops[pi].m_name);
      }
    }

    props.length (pc);
    numProps = pc;
  }

  for (CORBA::ULong pi=0; pi<numProps; pi++) {
    CF::DataType & property = props[pi];

    try {
      queryWorker (property.id,
                   property.value,
                   haveSync);
    }
    catch (const std::string & oops) {
      up.invalidProperties.length (numInvalidProps + 1);
      up.invalidProperties[numInvalidProps++] = property;
      *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                << m_logProducerName
                << "Error querying property \""
                << property.id
                << "\": "
                << oops
                << "."
                << std::flush;
    }
  }

  if (numInvalidProps) {
    throw up;
  }
}

/*
 * ----------------------------------------------------------------------
 * CF::Resource
 * ----------------------------------------------------------------------
 */

char *
BaseProxy::
identifier ()
  throw (CORBA::SystemException)
{
  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  return CORBA::string_dup (m_identifier.c_str());
}

void
BaseProxy::
start ()
  throw (CF::Resource::StartError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  OCPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "start ()"
        << std::flush;

  if (m_state == StartedState) {
    // Ignore "start" when already started.
    return;
  }

  try {
    startWorker();//controlWorker (WCI_CONTROL_START);
  }
  catch (const std::string & oops) {
    *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to start: "
              << oops
              << "."
              << std::flush;

    CF::Resource::StartError se;
    se.msg = oops.c_str();
    throw se;
  }

  m_state = StartedState;
}

void
BaseProxy::
stop ()
  throw (CF::Resource::StopError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  OCPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "stop ()"
        << std::flush;

  if (m_state == StoppedState) {
    // Ignore "stop" when already stopped.
    return;
  }

  try {
    stopWorker(); //controlWorker (WCI_CONTROL_STOP);
  }
  catch (const std::string & oops) {
    *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to start: "
              << oops
              << "."
              << std::flush;

    CF::Resource::StopError se;
    se.msg = oops.c_str();
    throw se;
  }

  m_state = StoppedState;
}

/*
 * ----------------------------------------------------------------------
 * Helper for connection management.
 * ----------------------------------------------------------------------
 */

BaseProxyPort::
BaseProxyPort (const std::string & portName,
                  BaseProxy * proxy)
  throw ()
  : m_portName (portName),
    m_proxy (proxy)
{
  m_proxy->_add_ref ();
}

// This is NOT the SCA release, but a lot like it.
// SCA ports do not implement releaseObject
void BaseProxyPort::
release()
{
  m_proxy->m_poa->deactivate_object(*m_proxy->m_poa->servant_to_id (this));
}

BaseProxyPort::
~BaseProxyPort ()
  throw ()
{
  m_proxy->_remove_ref ();
}

void
BaseProxyPort::
connectPort (CORBA::Object_ptr connection, const char * connectionId)
  throw (CF::Port::InvalidPort,
         CF::Port::OccupiedPort,
         CORBA::SystemException)
{
#if 0
  m_proxy->connectPort (m_portName, connectionId, connection);
#else
  (void)connection;(void)connectionId;
  ocpiAssert(0);
#endif
}

void
BaseProxyPort::
disconnectPort (const char * connectionId)
  throw (CF::Port::InvalidPort,
         CORBA::SystemException)
{
#if 0
  m_proxy->disconnectPort (m_portName, connectionId);
#else
  (void)connectionId;
  ocpiAssert(0);
#endif
}
  }
}
