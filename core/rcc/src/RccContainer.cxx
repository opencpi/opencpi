
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

#include "RccContainer.h"
#include "OcpiOsMisc.h"
#include "RCC_Worker.h"

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OR = OCPI::RDT;

namespace OCPI {
  namespace RCC {

    bool Container::m_wqInit = false;

DataTransfer::EventManager*  
Container::
getEventManager()
{
  return getTransport().m_transportGlobal->getEventManager();
}

class Driver;
Container::
Container(const char *name,
	  //	  OCPI::DataTransport::TransportGlobal *tpg, 
	  const OA::PValue* props )
  throw ( OU::EmbeddedException )
  : OC::ContainerBase<Driver,Container,Application,Artifact>(*this, name)
{
  m_model = "rcc";
}

void Container::
initWorkQueues() {
  OU::SelfAutoMutex guard(this);
  if (m_wqInit == false ) {

     pthread_workqueue_attr_t attr;

     memset(&m_workqueues, 0, sizeof(m_workqueues));     

    // Create the worker queues
     if (pthread_workqueue_attr_init_np(&attr) != 0)
       throw  OU::Error("Worker static initialization: Could not init workqueue attributes");

     //     if (pthread_attr_setinheritsched( &attr, 0) != 0 ) 
     //       throw  OU::Error("Worker static initialization: Could not set scheduler in  workqueue attributes");

     if (pthread_workqueue_attr_setqueuepriority_np(&attr, WORKQ_HIGH_PRIOQUEUE) != 0) 
       throw  OU::Error("Worker static initialization: Could not set workqueue priorities");        
     if (pthread_workqueue_create_np(&m_workqueues[HIGH_PRI_Q], &attr) != 0)
       throw  OU::Error("Worker static initialization: Could not create workqueue ");

     if (pthread_workqueue_attr_init_np(&attr) != 0)
       throw  OU::Error("Worker static initialization: Could not init workqueue attributes");
     if (pthread_workqueue_attr_setqueuepriority_np(&attr, WORKQ_LOW_PRIOQUEUE) != 0) 
       throw  OU::Error("Worker static initialization: Could not set workqueue priorities");        
     if (pthread_workqueue_create_np(&m_workqueues[LOW_PRI_Q], &attr) != 0)
       throw  OU::Error("Worker static initialization: Could not create workqueue ");

     m_wqInit = true;
   }

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
}


struct Wargs {
  RCCUserTask * taskc;
  void (*task)(void *);
  void * args;
};

static volatile int task_count = 0;
void wait_join( void *  args) {
  OCPI::OS::Semaphore * sem = (OCPI::OS::Semaphore *)args;
  while( task_count > 0 ) {
    OCPI::OS::sleep( 0 );
  }
  sem->post();  
}

bool
Container::
join( bool block, OCPI::OS::Semaphore & sem ) {

  //  printf("In join \n");
  if ( task_count == 0 ) {
    return true;
  }

  if ( ! block ) {
    return false;
  }
  else {
    pthread_workqueue_additem_np(m_workqueues[LOW_PRI_Q], wait_join, (void*)&sem, NULL, NULL);    
  }


  printf("IN Con join about to wait for sem\n");
  sem.wait();
  printf("IN Con join , joined \n");
  return true;
}

static OCPI::OS::Mutex mutex;
void 
taskWrapper(  void * args ) {
  Wargs * wargs = (Wargs*)args;

  if ( wargs->taskc == NULL ) {
    wargs->task(wargs->args);
  }
  else {
    wargs->taskc->run();
    wargs->taskc->done();
  }

  mutex.lock();
  task_count--;
  mutex.unlock();

  delete wargs;
}

void
Container::
addTask( void (*task)(void *), void * args ) {
  if (!m_wqInit)
    initWorkQueues();
  mutex.lock();
  task_count++;
  mutex.unlock();

  Wargs *wargs = new Wargs();
  wargs->taskc = NULL;  
  wargs->task = task;
  wargs->args = args;
  pthread_workqueue_additem_np(m_workqueues[HIGH_PRI_Q], taskWrapper, wargs, NULL, NULL);    
}



void
Container::
addTask( OCPI::RCC::RCCUserTask * task ) {
  if (!m_wqInit)
    initWorkQueues();
  mutex.lock();
  task_count++;
  mutex.unlock();

  Wargs *wargs = new Wargs();
  wargs->taskc = task;
  wargs->args = NULL;
  pthread_workqueue_additem_np(m_workqueues[HIGH_PRI_Q], taskWrapper, wargs, NULL, NULL);    
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
    for (Application *a = OU::Parent<Application>::firstChild(); a; a = a->nextChild())
      a->release();
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
