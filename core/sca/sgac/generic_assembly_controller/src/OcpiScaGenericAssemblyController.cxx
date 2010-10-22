
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
 * Generic Assembly Controller
 *
 * Revision History:
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2.
 *
 *     03/17/2009 - Frank Pilhofer
 *                  Bug fix: Invalid properties returned from a query() with
 *                  a non-empty list of properties.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <CF_s.h>
#include <OcpiOsAssert.h>
#include <OcpiLoggerTee.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiLwLogLoggerOutput.h>
#include <OcpiCloneSystemException.h>
#include <OcpiStringifyCorbaException.h>
#include <OcpiCFUtilMisc.h>
#include <OcpiCFUtilReceptacleHelper.h>
#include <OcpiCFUtilStringifyCFException.h>
#include <OcpiCFUtilLegacyErrorNumbers.h>
#include "OcpiScaGenericAssemblyController.h"

OCPI::SCA::GenericAssemblyController::
GenericAssemblyController (CORBA::ORB_ptr orb,
                           PortableServer::POA_ptr poa,
                           const std::string & componentIdentifier,
                           OCPI::Logger::Logger * logger,
                           bool adoptLogger,
                           bool shutdownOrbOnRelease)
  throw (std::string)
  : m_orb (CORBA::ORB::_duplicate (orb)),
    m_poa (PortableServer::POA::_duplicate (poa)),
    m_shutdownOrbOnRelease (shutdownOrbOnRelease),
    m_componentIdentifier (componentIdentifier),
    m_logPort (m_poa, "LogOut", this),
    m_logProducerName ("")
{
  ocpiAssert (!CORBA::is_nil (m_orb));
  ocpiAssert (!CORBA::is_nil (m_poa));

  std::string::size_type colon = m_componentIdentifier.rfind (':');

  if (colon != std::string::npos) {
    m_instantiationIdentifier = m_componentIdentifier.substr (0, colon);
    m_applicationName = m_componentIdentifier.substr (colon + 1);
  }
  else {
    m_instantiationIdentifier = m_componentIdentifier;
    m_applicationName = m_componentIdentifier;
  }

  m_out.addOutput (m_logOut, false, true);

  if (logger) {
    m_out.addOutput (logger, adoptLogger);
  }

  m_logProducerName = OCPI::Logger::ProducerName (m_applicationName);
  m_out.setProducerId (m_instantiationIdentifier.c_str());

  m_producerLogLevel.length (26);
  for (CORBA::ULong pli=0; pli<26; pli++) {
    m_producerLogLevel[pli] = static_cast<OCPI::CORBAUtil::LwLogLoggerOutput::LogLevelType> (pli+1);
  }
}

OCPI::SCA::GenericAssemblyController::
~GenericAssemblyController ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * OCPI::CFUtil::ReceptacleHelperCallback
 * ----------------------------------------------------------------------
 */

void
OCPI::SCA::GenericAssemblyController::
connectPort (const std::string & portName,
             const std::string & connectionId)
  throw (CF::Port::InvalidPort,
         CF::Port::OccupiedPort,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);

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
OCPI::SCA::GenericAssemblyController::
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
OCPI::SCA::GenericAssemblyController::
initialize ()
  throw (CF::LifeCycle::InitializeError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Initializing."
        << std::flush;
}

void
OCPI::SCA::GenericAssemblyController::
releaseObject ()
  throw (CF::LifeCycle::ReleaseError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

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
}

/*
 * ----------------------------------------------------------------------
 * CF::TestableObject
 * ----------------------------------------------------------------------
 */

void
OCPI::SCA::GenericAssemblyController::
runTest (CORBA::ULong testId,
         CF::Properties & testValues)
  throw (CF::TestableObject::UnknownTest,
         CF::UnknownProperties,
         CORBA::SystemException)
{
  ( void ) testId;
  ( void ) testValues;
  throw CF::TestableObject::UnknownTest ();
}

/*
 * ----------------------------------------------------------------------
 * CF::PropertySet
 * ----------------------------------------------------------------------
 */

void
OCPI::SCA::GenericAssemblyController::
mergeProperties (CF::Properties & set,
                 CORBA::ULong & setSize,
                 const CF::Properties & toMerge,
                 const std::string & propPrefix)
  throw ()
{
  CORBA::ULong tms = toMerge.length ();

  set.length (setSize + tms);

  for (CORBA::ULong tmi=0; tmi<tms; tmi++) {
    std::string propName = propPrefix;
    propName += ".";
    propName += toMerge[tmi].id;
    set[setSize + tmi].id = propName.c_str ();
    set[setSize + tmi].value = toMerge[tmi].value;
  }

  setSize += tms;
}

void
OCPI::SCA::GenericAssemblyController::
configure (const CF::Properties & configProperties)
  throw (CF::PropertySet::InvalidConfiguration,
         CF::PropertySet::PartialConfiguration,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  CORBA::ULong numProps = configProperties.length ();
  CF::Properties invalidProperties;
  CORBA::ULong numInvalidProperties = 0;

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "configure (";

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    if (dpi > 0) {
      debug << ", ";
    }

    debug << configProperties[dpi].id;
  }

  debug << ")" << std::flush;

  /*
   * Properties shall have the component name prefixed with a ".".  Sort
   * them into per-component bins.
   */

  SetOfProperties sop;

  for (CORBA::ULong pi=0; pi<numProps; pi++) {
    const CF::DataType & property = configProperties[pi];
    std::string propertyId = property.id.in ();
    std::string::size_type dot = propertyId.find ('.');

    if (dot != std::string::npos) {
      std::string componentName = propertyId.substr (0, dot);
      std::string propertyName = propertyId.substr (dot+1);

      CF::Properties & ccp = sop[componentName];
      CORBA::ULong ccpl = ccp.length ();
      ccp.length (ccpl+1);
      ccp[ccpl].id = propertyName.c_str ();
      ccp[ccpl].value = property.value;
    }
    else if (propertyId == "PRODUCER_LOG_LEVEL") {
      if (!configureProducerLogLevel (property)) {
        invalidProperties.length (numInvalidProperties + 1);
        invalidProperties[numInvalidProperties++] = property;
      }
    }
    else {
      invalidProperties.length (numInvalidProperties + 1);
      invalidProperties[numInvalidProperties++] = property;
    }
  }

  /*
   * Now call configure() on each component.
   */

  SetOfProperties::iterator sit, end;

  for (sit = sop.begin(), end = sop.end(); sit != end; sit++) {
    const std::string & componentName = (*sit).first;
    const CF::Properties & ccp = (*sit).second;
    const CORBA::ULong ccpl = ccp.length ();

    debug << m_logProducerName
          << OCPI::Logger::Verbosity (1)
          << "Calling configure (";

    for (CORBA::ULong ocpi=0; ocpi<ccpl; ocpi++) {
      if (ocpi > 0) {
        debug << ", ";
      }

      debug << ccp[ocpi].id;
    }

    debug << ") on component \""
          << componentName
          << "\"."
          << std::flush;

    PortMap::iterator pmi = m_portMap.find (componentName);

    if (pmi == m_portMap.end()) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Attempt to configure non-existent component \""
            << componentName
            << "\"."
            << std::flush;
      mergeProperties (invalidProperties, numInvalidProperties,
                       ccp, componentName);
      continue;
    }

    CF::Resource_var cr = (*pmi).second->getConnection ();

    if (CORBA::is_nil (cr)) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Can not configure component \""
            << componentName
            << "\" because it is not connected."
            << std::flush;
      mergeProperties (invalidProperties, numInvalidProperties,
                       ccp, componentName);
      continue;
    }

    try {
      cr->configure (ccp);
    }
    catch (const CF::PropertySet::InvalidConfiguration & icex) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Failed to configure component \""
            << componentName
            << "\":"
            << OCPI::CFUtil::stringifyCFException (icex)
            << "."
            << std::flush;
      mergeProperties (invalidProperties, numInvalidProperties,
                       icex.invalidProperties, componentName);
    }
    catch (const CF::PropertySet::PartialConfiguration & pcex) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Failed to configure component \""
            << componentName
            << "\":"
            << OCPI::CFUtil::stringifyCFException (pcex)
            << "."
            << std::flush;
      mergeProperties (invalidProperties, numInvalidProperties,
                       pcex.invalidProperties, componentName);
    }
    catch (const CORBA::SystemException & ex) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Failed to configure component \""
            << componentName
            << "\":"
            << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
            << "."
            << std::flush;
      mergeProperties (invalidProperties, numInvalidProperties,
                       ccp, componentName);
    }
  }

  if (numInvalidProperties > 0 && numInvalidProperties == numProps) {
    // No property was successfully configured.
    CF::PropertySet::InvalidConfiguration ic;
    ic.msg = "configure failed";
    ic.invalidProperties = invalidProperties;
    throw ic;
  }
  else if (numInvalidProperties) {
    // Some, but not all properties were successfully configured.
    CF::PropertySet::PartialConfiguration pc;
    pc.invalidProperties = invalidProperties;
    throw pc;
  }

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "configured."
        << std::flush;
}

void
OCPI::SCA::GenericAssemblyController::
query (CF::Properties & configProperties)
  throw (CF::UnknownProperties,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);
  CORBA::ULong numProps = configProperties.length ();

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "query (";

  for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
    if (dpi > 0) {
      debug << ", ";
    }

    debug << configProperties[dpi].id;
  }

  debug << ")" << std::flush;

  /*
   * If the list of properties is empty, we're meant to return all available
   * properties.  So iterate over every component and ask it for its set.
   */

  if (!numProps) {
    PortMap::iterator pit, end;

    for (pit = m_portMap.begin(), end = m_portMap.end(); pit != end; pit++) {
      const std::string & componentName = (*pit).first;

      debug << m_logProducerName
            << OCPI::Logger::Verbosity (1)
            << "Calling query() on component \""
            << componentName
            << "\"."
            << std::flush;

      CF::Resource_var cr = (*pit).second->getConnection ();

      if (CORBA::is_nil (cr)) {
        m_out << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Can not query component \""
              << componentName
              << "\" because it is not connected."
              << std::flush;
        continue;
      }

      CF::Properties ccp;

      try {
        cr->query (ccp);
      }
      catch (const CORBA::Exception & ex) {
        m_out << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Failed to query component \""
              << componentName
              << "\": "
              << OCPI::CFUtil::stringifyCFException (ex)
              << "."
              << std::flush;
      }

      mergeProperties (configProperties, numProps,
                       ccp, componentName);
    }

    configProperties.length (numProps + 1);
    configProperties[numProps].id = "PRODUCER_LOG_LEVEL";
    queryProducerLogLevel (configProperties[numProps]);
    return;
  }

  /*
   * The rest is more or less the same as for configure() above.
   */

  CF::Properties outProperties;
  CORBA::ULong numOutProperties = 0;

  CF::UnknownProperties up;
  CORBA::ULong numInvalidProperties = 0;
  SetOfProperties sop;

  for (CORBA::ULong pi=0; pi<numProps; pi++) {
    CF::DataType & property = configProperties[pi];
    std::string propertyId = property.id.in ();
    std::string::size_type dot = propertyId.find ('.');

    if (dot != std::string::npos) {
      std::string componentName = propertyId.substr (0, dot);
      std::string propertyName = propertyId.substr (dot+1);

      CF::Properties & ccp = sop[componentName];
      CORBA::ULong ccpl = ccp.length ();
      ccp.length (ccpl+1);
      ccp[ccpl].id = propertyName.c_str ();
    }
    else if (propertyId == "PRODUCER_LOG_LEVEL") {
      outProperties.length (numOutProperties + 1);
      outProperties[numOutProperties].id = propertyId.c_str ();
      queryProducerLogLevel (outProperties[numOutProperties]);
      numOutProperties++;
    }
    else {
      up.invalidProperties.length (numInvalidProperties + 1);
      up.invalidProperties[numInvalidProperties++] = property;
    }
  }

  /*
   * Now call query() on each component.
   */

  SetOfProperties::iterator sit, end;

  configProperties = outProperties;
  numProps = numOutProperties;

  for (sit = sop.begin(), end = sop.end(); sit != end; sit++) {
    const std::string & componentName = (*sit).first;
    CF::Properties & ccp = (*sit).second;
    const CORBA::ULong ccpl = ccp.length ();

    debug << m_logProducerName
          << OCPI::Logger::Verbosity (1)
          << "Calling configure (";

    for (CORBA::ULong ocpi=0; ocpi<ccpl; ocpi++) {
      if (ocpi > 0) {
        debug << ", ";
      }

      debug << ccp[ocpi].id;
    }

    debug << ") on component \""
          << componentName
          << "\"."
          << std::flush;

    PortMap::iterator pmi = m_portMap.find (componentName);

    if (pmi == m_portMap.end()) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Attempt to query non-existent component \""
            << componentName
            << "\"."
            << std::flush;
      mergeProperties (up.invalidProperties, numInvalidProperties,
                       ccp, componentName);
      continue;
    }

    CF::Resource_var cr = (*pmi).second->getConnection ();

    if (CORBA::is_nil (cr)) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Can not query component \""
            << componentName
            << "\" because it is not connected."
            << std::flush;
      mergeProperties (up.invalidProperties, numInvalidProperties,
                       ccp, componentName);
      continue;
    }

    try {
      cr->query (ccp);
      mergeProperties (configProperties, numProps,
                       ccp, componentName);
    }
    catch (const CF::UnknownProperties & upex) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Failed to query component \""
            << componentName
            << "\":"
            << OCPI::CFUtil::stringifyCFException (upex)
            << "."
            << std::flush;
      mergeProperties (up.invalidProperties, numInvalidProperties,
                       upex.invalidProperties, componentName);
    }
    catch (const CORBA::SystemException & ex) {
      m_out << OCPI::Logger::Level::EXCEPTION_ERROR
            << m_logProducerName
            << "Failed to query component \""
            << componentName
            << "\":"
            << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
            << "."
            << std::flush;
      mergeProperties (up.invalidProperties, numInvalidProperties,
                       ccp, componentName);
    }
  }

  if (numInvalidProperties) {
    m_out << OCPI::Logger::Level::EXCEPTION_ERROR
          << m_logProducerName
          << "query failed for "
          << ((numInvalidProperties != 1) ? "properties " : "property ");

    for (CORBA::ULong dpi=0; dpi<numInvalidProperties; dpi++) {
      if (dpi > 0 && dpi+1 == numInvalidProperties) {
        m_out << " and ";
      }
      else if (dpi > 0) {
        m_out << ", ";
      }

      m_out << up.invalidProperties[dpi].id;
    }

    m_out << "." << std::flush;
    throw up;
  }

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "queried."
        << std::flush;
}

/*
 * ----------------------------------------------------------------------
 * CF::PortSupplier
 * ----------------------------------------------------------------------
 */

CORBA::Object_ptr
OCPI::SCA::GenericAssemblyController::
getPort (const char * name)
  throw (CF::PortSupplier::UnknownPort,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Query for port \""
        << name
        << "\"."
        << std::flush;

  if (std::strcmp (name, "LogOut") == 0) {
    return m_logPort.getPort ();
  }

  PortMap::iterator it = m_portMap.find (name);

  if (it != m_portMap.end()) {
    return (*it).second->getPort ();
  }

  OCPI::CFUtil::ReceptacleHelper<CF::Resource> * np =
    new OCPI::CFUtil::ReceptacleHelper<CF::Resource> (m_poa, name, this);
  m_portMap[name] = np;

  return np->getPort ();
}

/*
 * ----------------------------------------------------------------------
 * CF::Resource
 * ----------------------------------------------------------------------
 */

char *
OCPI::SCA::GenericAssemblyController::
identifier ()
  throw (CORBA::SystemException)
{ 
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "Identifier attribute queried: \""
        << m_componentIdentifier
        << "\"."
        << std::flush;

  return CORBA::string_dup (m_componentIdentifier.c_str());
}

void
OCPI::SCA::GenericAssemblyController::
start ()
  throw (CF::Resource::StartError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  PortMap::size_type nc = m_portMap.size ();

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Starting "
        << nc
        << " componentns."
        << std::flush;

  /*
   * Call start() on every connected component.  If one of them fails to
   * start, stop the components that were already started, then propagate
   * the exception.
   */

  PortMap::const_iterator it, end;

  for (it = m_portMap.begin (), end = m_portMap.end (); it != end; it++) {
    CF::Resource_var cr = (*it).second->getConnection ();

    if (!CORBA::is_nil (cr)) {
      debug << m_logProducerName
            << OCPI::Logger::Verbosity (1)
            << "Starting component \""
            << (*it).first
            << "\"."
            << std::flush;

      try {
        cr->start ();
      }
      catch (const CORBA::Exception & ex) {
        m_out << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Component \""
              << (*it).first
              << "\" failed to start: "
              << OCPI::CFUtil::stringifyCFException (ex)
              << "."
              << std::flush;

        for (PortMap::const_iterator i2 = m_portMap.begin (); i2 != it; i2++) {
          cr = (*it).second->getConnection ();

          if (!CORBA::is_nil (cr)) {
            debug << m_logProducerName
                  << OCPI::Logger::Verbosity (1)
                  << "Stopping component \""
                  << (*it).first
                  << "\"."
                  << std::flush;

            try {
              cr->stop ();
            }
            catch (const CORBA::Exception & e2) {
              m_out << OCPI::Logger::Level::EXCEPTION_ERROR
                    << m_logProducerName
                    << "Failed to stop component \""
                    << (*it).first
                    << "\": "
                    << OCPI::CFUtil::stringifyCFException (ex)
                    << ".  (Ignored.)"
                    << std::flush;
            }
          }
        }

        throw;
      }
    }
    else {
      debug << m_logProducerName
            << OCPI::Logger::Verbosity (1)
            << "Can not start component \""
            << (*it).first
            << "\" because is not connected yet."
            << std::flush;
    }
  }

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "Started."
        << std::flush;
}

void
OCPI::SCA::GenericAssemblyController::
stop ()
  throw (CF::Resource::StopError,
         CORBA::SystemException)
{
  OCPI::Util::AutoMutex mutex (m_mutex);
  OCPI::Logger::DebugLogger debug (m_out);

  PortMap::size_type nc = m_portMap.size ();

  debug << m_logProducerName
        << OCPI::Logger::Verbosity (1)
        << "Starting "
        << nc
        << " componentns."
        << std::flush;

  /*
   * Call stop() on every connected component.  If one of them fails to
   * stop, proceed to stop the remaining components, then propagate the
   * exception.
   */

  PortMap::const_iterator it, end;
  bool gotException = false;
  CF::Resource::StopError se;
  CORBA::SystemException * cse = 0;

  for (it = m_portMap.begin (), end = m_portMap.end (); it != end; it++) {
    CF::Resource_var cr = (*it).second->getConnection ();

    if (!CORBA::is_nil (cr)) {
      debug << m_logProducerName
            << OCPI::Logger::Verbosity (1)
            << "Stopping component \""
            << (*it).first
            << "\"."
            << std::flush;

      try {
        cr->stop ();
      }
      catch (const CF::Resource::StopError & ex) {
        m_out << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Component \""
              << (*it).first
              << "\" failed to stop: "
              << OCPI::CFUtil::stringifyCFException (ex)
              << "."
              << std::flush;

        if (!gotException) {
          gotException = true;
          se = ex;
        }
      }
      catch (const CORBA::SystemException & ex) {
        m_out << OCPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Component \""
              << (*it).first
              << "\" failed to stop: "
              << OCPI::CFUtil::stringifyCFException (ex)
              << "."
              << std::flush;

        if (!gotException) {
          gotException = true;
          cse = OCPI::CORBAUtil::Misc::cloneSystemException (ex);
        }
      }
    }
    else {
      debug << m_logProducerName
            << OCPI::Logger::Verbosity (1)
            << "Can not stop component \""
            << (*it).first
            << "\" because is not connected yet."
            << std::flush;
    }
  }

  if (gotException) {
    if (cse) {
      /* Leak */
      cse->_raise ();
    }
    else {
      throw se;
    }
  }

  m_out << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
        << m_logProducerName
        << "Stopped."
        << std::flush;
}

/*
 * ----------------------------------------------------------------------
 * Helper functions to configure and query PRODUCER_LOG_LEVEL.
 * ----------------------------------------------------------------------
 */

bool
OCPI::SCA::GenericAssemblyController::
configureProducerLogLevel (const CF::DataType & property)
  throw ()
{
  /*
   * Logically, this PRODUCER_LOG_LEVEL property should be of type
   * CosLwLog::LogLevelSequence.  However, this type can not be
   * expressed in an SCA property file, which allows for sequences
   * of basic types only.  JTAP complains if this property's type
   * is different than a sequence of unsigned short.
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
OCPI::SCA::GenericAssemblyController::
queryProducerLogLevel (CF::DataType & property)
  throw ()
{
  CORBA::ULong nls = m_producerLogLevel.length();
  CORBA::UShortSeq levels (nls);
  levels.length (nls);

  for (CORBA::ULong li=0; li<nls; li++) {
    levels[li] = m_producerLogLevel[li];
  }

  property.value <<= levels;
}
