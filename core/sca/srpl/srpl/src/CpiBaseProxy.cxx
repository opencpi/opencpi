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
#include <CpiUtilAutoMutex.h>
#include <CpiLoggerLogger.h>
#include <CpiLoggerNullOutput.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiCFUtilLegacyErrorNumbers.h>
#include <CF.h>
#include <CF_s.h>
#include <wci.h>
#include "CpiBaseProxy.h"

/*
 * ----------------------------------------------------------------------
 * Constructor and Destructor
 * ----------------------------------------------------------------------
 */

CPI::SCA::BaseProxy::
BaseProxy (CORBA::ORB_ptr orb,
              PortableServer::POA_ptr poa,
              const std::string & aIdentifier,
              CPI::Logger::Logger * logger,
              bool adoptLogger,
              bool shutdownOrbOnRelease)
  throw (std::string)
  : m_disabled (false),
    m_orb (CORBA::ORB::_duplicate (orb)),
    m_poa (PortableServer::POA::_duplicate (poa)),
    m_logger (logger),
    m_logProducerName (aIdentifier),
    m_state (static_cast<WCI_control> (-1)),
    m_identifier (aIdentifier),
    m_adoptLogger (adoptLogger),
    m_shutdownOrbOnRelease (shutdownOrbOnRelease)
{
  if (!m_logger) {
    try {
      m_logger = new CPI::Logger::NullOutput;
    }
    catch (const std::bad_alloc & oops) {
      throw std::string (oops.what());
    }

    m_adoptLogger = true;
  }

  CPI::Logger::DebugLogger debug (*m_logger);
  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
        << "Constructed."
        << std::flush;
}

CPI::SCA::BaseProxy::
~BaseProxy ()
  throw ()
{
  CPI::Logger::DebugLogger debug (*m_logger);
  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
        << "Destructed."
        << std::flush;

}

/*
 * ----------------------------------------------------------------------
 * CF::LifeCycle
 * ----------------------------------------------------------------------
 */

void
CPI::SCA::BaseProxy::
initialize ()
  throw (CF::LifeCycle::InitializeError,
         CORBA::SystemException)
{
  CPI::Util::AutoMutex lock (m_mutex);
  CPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
        << "initialize ()"
        << std::flush;

  try {
    initializeWorker();//controlWorker (WCI_CONTROL_INITIALIZE);
  }
  catch (const std::string & oops) {
    *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to initialize: "
              << oops
              << "."
              << std::flush;

    CF::LifeCycle::InitializeError ie;
    ie.errorMessages.length (1);
    ie.errorMessages[0] = oops.c_str();
    throw ie;
  }

  m_state = WCI_CONTROL_INITIALIZE;
}

void
CPI::SCA::BaseProxy::
releaseObject ()
  throw (CF::LifeCycle::ReleaseError,
         CORBA::SystemException)
{
  CPI::Util::AutoMutex lock (m_mutex);
  CPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
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
    *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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

  m_state = WCI_CONTROL_RELEASE;
}

/*
 * ----------------------------------------------------------------------
 * CF::TestableObject
 * ----------------------------------------------------------------------
 */

void
CPI::SCA::BaseProxy::
runTest (CORBA::ULong testId, CF::Properties & testValues)
  throw (CF::TestableObject::UnknownTest,
         CF::UnknownProperties,
         CORBA::SystemException)
{
  CPI::Util::AutoMutex lock (m_mutex);
  CPI::Logger::DebugLogger debug (*m_logger);
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
        << CPI::Logger::Verbosity (2)
        << "testId ("
        << testId;

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    debug << ", ";
    debug << testValues[dpi].id;
  }

  debug << ")" << std::flush;

#ifdef TEST
  const CPI::SCA::Test * td;

  try {
    td = findTest (testId);
  }
  catch (const CF::TestableObject::UnknownTest &) {
    *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
    const CPI::Metadata::Property * props = getProperties (numProperties);

    /*
     * Set input values.
     */

    StringSet expectedInputs;

    for (unsigned int tivi=0; tivi<td->numInputs; tivi++) {
      unsigned int pn = td->inputValues[tivi];
      cpiAssert (pn < numProperties);
      expectedInputs.insert (props[pn].name);
    }

    for (CORBA::ULong pi=0; pi<numProps; pi++) {
      const CF::DataType & property = testValues[pi];
      StringSet::iterator eii = expectedInputs.find (property.id.in());

      if (eii == expectedInputs.end()) {
        up.invalidProperties.length (numInvalidProps + 1);
        up.invalidProperties[numInvalidProps++] = property;
        *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
        *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
                  << m_logProducerName
                  << "Error configuring test input value \""
                  << property.id
                  << "\": "
                  << oops
                  << "."
                  << std::flush;
      }
    }

    /*
     * Did the test get all input values that it expects?
     */

    if (!expectedInputs.empty()) {
      *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
      *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
      cpiAssert (pn < numProperties);

      CF::DataType & property = testValues[pi];
      const char * propName = props[pn].name;
      property.id = propName;

      queryWorker (propName, property.value, haveSync);
    }
  }
  catch (const std::string & oops) {
    *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
CPI::SCA::BaseProxy::
configure (const CF::Properties & props)
  throw (CF::PropertySet::InvalidConfiguration,
         CF::PropertySet::PartialConfiguration,
         CORBA::SystemException)
{
  CPI::Util::AutoMutex lock (m_mutex);
  CPI::Logger::DebugLogger debug (*m_logger);
  CF::Properties invalidProperties;
  CORBA::ULong numProps = props.length ();
  CORBA::ULong numInvalidProps = 0;
  bool needSync = false;

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
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
      *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
CPI::SCA::BaseProxy::
query (CF::Properties & props)
  throw (CF::UnknownProperties,
         CORBA::SystemException)
{
  CPI::Util::AutoMutex lock (m_mutex);
  CPI::Logger::DebugLogger debug (*m_logger);
  CORBA::ULong numProps = props.length ();
  CORBA::ULong numInvalidProps = 0;
  CF::UnknownProperties up;
  bool haveSync = false;

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
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
    const CPI::Metadata::Property * cprops = getProperties (numProperties);

    numProps = numProperties;
    props.length (numProps);
    CORBA::ULong pc = 0;

    for (CORBA::ULong pi=0; pi<numProps; pi++) {
      if (cprops[pi].isReadable && !cprops[pi].isTest) {
        props[pc++].id = CORBA::string_dup (cprops[pi].name);
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
      *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
CPI::SCA::BaseProxy::
identifier ()
  throw (CORBA::SystemException)
{
  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  return CORBA::string_dup (m_identifier.c_str());
}

void
CPI::SCA::BaseProxy::
start ()
  throw (CF::Resource::StartError,
         CORBA::SystemException)
{
  CPI::Util::AutoMutex lock (m_mutex);
  CPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
        << "start ()"
        << std::flush;

  if (m_state == WCI_CONTROL_START) {
    // Ignore "start" when already started.
    return;
  }

  try {
    startWorker();//controlWorker (WCI_CONTROL_START);
  }
  catch (const std::string & oops) {
    *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to start: "
              << oops
              << "."
              << std::flush;

    CF::Resource::StartError se;
    se.msg = oops.c_str();
    throw se;
  }

  m_state = WCI_CONTROL_START;
}

void
CPI::SCA::BaseProxy::
stop ()
  throw (CF::Resource::StopError,
         CORBA::SystemException)
{
  CPI::Util::AutoMutex lock (m_mutex);
  CPI::Logger::DebugLogger debug (*m_logger);

  if (m_disabled) {
    throw CORBA::BAD_INV_ORDER ();
  }

  debug << m_logProducerName
        << CPI::Logger::Verbosity (2)
        << "stop ()"
        << std::flush;

  if (m_state == WCI_CONTROL_STOP) {
    // Ignore "stop" when already stopped.
    return;
  }

  try {
    stopWorker(); //controlWorker (WCI_CONTROL_STOP);
  }
  catch (const std::string & oops) {
    *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to start: "
              << oops
              << "."
              << std::flush;

    CF::Resource::StopError se;
    se.msg = oops.c_str();
    throw se;
  }

  m_state = WCI_CONTROL_STOP;
}

/*
 * ----------------------------------------------------------------------
 * Helper for connection management.
 * ----------------------------------------------------------------------
 */

CPI::SCA::BaseProxyPort::
BaseProxyPort (const std::string & portName,
                  CPI::SCA::BaseProxy * proxy)
  throw ()
  : m_portName (portName),
    m_proxy (proxy)
{
  m_proxy->_add_ref ();
}

// This is NOT the SCA release, but a lot like it.
// SCA ports do not implement releaseObject
void CPI::SCA::BaseProxyPort::
release()
{
  m_proxy->m_poa->deactivate_object(*m_proxy->m_poa->servant_to_id (this));
}

CPI::SCA::BaseProxyPort::
~BaseProxyPort ()
  throw ()
{
  m_proxy->_remove_ref ();
}

void
CPI::SCA::BaseProxyPort::
connectPort (CORBA::Object_ptr connection, const char * connectionId)
  throw (CF::Port::InvalidPort,
         CF::Port::OccupiedPort,
         CORBA::SystemException)
{
#if 0
  m_proxy->connectPort (m_portName, connectionId, connection);
#else
  (void)connection;(void)connectionId;
  cpiAssert(0);
#endif
}

void
CPI::SCA::BaseProxyPort::
disconnectPort (const char * connectionId)
  throw (CF::Port::InvalidPort,
         CORBA::SystemException)
{
#if 0
  m_proxy->disconnectPort (m_portName, connectionId);
#else
  (void)connectionId;
  cpiAssert(0);
#endif
}
