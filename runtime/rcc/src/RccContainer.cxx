/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Abstract:
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

#include "RccContainer.h"

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;

namespace OCPI {
  namespace RCC {


DataTransfer::EventManager*  
Container::
getEventManager()
{
  return getTransport().m_transportGlobal->getEventManager();
}

class Driver;
Container::
Container(const char *a_name, const OA::PValue* /* params */)
  throw ( OU::EmbeddedException )
  : OC::ContainerBase<Driver,Container,Application,Artifact>(*this, a_name)
{
  m_model = "rcc";
  m_dynamic = OC::Manager::dynamic();
  if (parent().m_platform.size())
    m_platform = parent().m_platform;
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
#if 0
  try {
    delete m_transport;
  }
  catch( ... ) {
    printf("ERROR: Got an exception in OCPI::RCC::Container::~Container)\n");
  }
#endif
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
    getTransport().dispatch( event_manager );
  } catch(...) {
    ocpiBad("RCC Container dispatch thread encountered transport exception.  Shutting down.");
    // Release all the current workers if there is a transport failure.
    for (Application *a = OU::Parent<Application>::firstChild(); a; a = a->nextChild()) {
      a->release(true, false);
      a->release(true, true);
      a->release(false, false);
    }
    return DispatchNoMore;
  }
  //#define VECTOR_BUFFERS_FROM_EVENTS
#ifdef VECTOR_BUFFERS_FROM_EVENTS
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
createApplication(const char *a_name, const OCPI::Util::PValue *props)
  throw ( OU::EmbeddedException )
{
  TRACE( "OCPI::RCC::Container::createApplication()");
  OU::SelfAutoMutex guard (this);

  Application* ca;
  try {
    ca = new Application(*this, a_name, props);
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
      DataTransfer::EndPoint* ep = getTransport().getEndpoint();
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
      DataTransfer::EndPoint* ep = getTransport().getEndpoint();
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
