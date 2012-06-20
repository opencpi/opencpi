
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
 * Abstact:
 *   This file contains the interface for the OCPI RCC Container.  
 *
 * Revision History: 
 * 
 *  06/15/09 - John Miller
 *  Added getLastControlError method.
 *
 *  03/01/05 -  John Miller
 *  Initial Version
 *
 ************************************************************************/

#include <string>
#include <cstring>
#include <cstdlib>
#include <memory>

#define WORKER_INTERNAL
#include <OcpiOsMisc.h>
#include <OcpiTransport.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilCDR.h>
#include <OcpiRDTInterface.h>
#include <OcpiRccPort.h>
#include <OcpiRccContainer.h>
#include <OcpiRccApplication.h>
#include <OcpiPortMetaData.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiContainerErrorCodes.h>
#include <DtTransferInternal.h>
#include <OcpiIntParallelDataDistribution.h>
#include <OcpiPValue.h>
#include <OcpiUtilMisc.h>
#include <RCC_Worker.h>

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;

namespace OCPI {
  namespace RCC {


DataTransfer::EventManager*  
Container::
getEventManager()
{
  return m_transport->m_transportGlobal->getEventManager();
}

class Driver;
Container::
Container(const char *name,
	  OCPI::DataTransport::TransportGlobal *tpg, 
	  const OA::PValue* props )
  throw ( OU::EmbeddedException )
  : OC::ContainerBase<Driver,Container,Application,Artifact>(name),
    OCPI::Time::Emit( "Container", name )
{
  m_model = "rcc";
  //temp  m_ourUID = g_unique_id;

  // The underlying tranport system has some number of endpoints registered.  Lets make sure
  // that their is at least 1 available or we will complain.
  // Initialize the underlying transport system
  try {
    m_transport = new OCPI::DataTransport::Transport( tpg, false, this );
  }
  catch( std::bad_alloc ) {
    throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "new", OU::ContainerFatal);
  }

  const char* monitorIPAddress = NULL;
  OU::findString(props, "monitorIPAddress", monitorIPAddress);
  //  start(NULL);
}



OC::Artifact & Container::
createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) {
  return *new Artifact(*this, lart, artifactParams);
}


/**********************************
 * Destructor
 *********************************/
Container::
~Container()
  throw ()
{
  // Lock our mutex.  It will be unlocked.
  TRACE( "OCPI::RCC::Container::~Container()");
  OC::Container::shutdown();
  this->lock();
  // We need to shut down the apps and workers since they
  // depend on artifacts and transport.
  OU::Parent<Application>::deleteChildren();
  try {
    delete m_transport;
  }
  catch( ... ) {
    printf("ERROR: Got an exception in OCPI::RCC::Container::~Container)\n");
  }
}

volatile int ocpi_dbg_run=0;

/**********************************
 * For single threaded containers, this is the dispatch hook
 *********************************/
OC::Container::DispatchRetCode 
Container::
dispatch(DataTransfer::EventManager* event_manager)
{
  bool more_to_do = false;
  if ( ! m_enabled ) {
    return Stopped;
  }
  OU::SelfAutoMutex guard(this);

#ifndef NDEBUG
  if ( ocpi_dbg_run ) {
    printf("WORKER RUN: Entry\n");
  }
#endif
  try {
    // Give our transport some time
    m_transport->dispatch( event_manager );
  } catch(...) {
    ocpiBad("RCC Container dispatch thread encountered transport exception.  Shutting down.");
    // Release all the current workers if there is a transport failure.
    for (Application *a = OU::Parent<Application>::firstChild(); a; a = a->nextChild())
      a->release();
    return DispatchNoMore;
  }
  //#define VECTOR_BUFFERS_FROM_EVENTS
#ifndef VECTOR_BUFFERS_FROM_EVENTS
  if ( event_manager ) {
    event_manager->consumeEvents();
  }
#endif
  // Process the workers
  for (Application *a = OU::Parent<Application>::firstChild(); a; a = a->nextChild())
    a->run(event_manager, more_to_do);

  return more_to_do ? MoreWorkNeeded : Spin;
}


/**********************************
 * Creates an application 
 *********************************/
OA::ContainerApplication * Container::
createApplication(const char *name, const OCPI::Util::PValue *props)
  throw ( OU::EmbeddedException )
{
  TRACE( "OCPI::RCC::Container::createApplication()");
  OU::SelfAutoMutex guard (this);

  Application* ca;
  try {
    ca = new Application(*this, name, props);
  }
  catch( std::bad_alloc ) {
    throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "new", OU::ContainerFatal);
  }
  return ca;
}


void 
Container::
start(DataTransfer::EventManager* event_manager)
  throw()
{
 ( void ) event_manager;
  try {
    m_enabled = true;

#ifdef EM_PORT_COMPLETE
    if ( event_manager ) {
      DataTransfer::EndPoint* ep = m_transport->getEndpoint();
      event_manager->spin(ep, false);
    }
#endif

  }
  catch( ... ) {
    // Ignore
  }
}


void 
Container::
stop(DataTransfer::EventManager* event_manager)
  throw()
{
  ( void ) event_manager;
  try {
    m_enabled = false;
#ifdef EM_PORT_COMPLETE
    if ( event_manager ) {
      DataTransfer::EndPoint* ep = m_transport->getEndpoint();
      event_manager->spin(ep, true);  
    }
#endif

  }
  catch( ... ) {
    // Ignore
  }
}

  }
}
