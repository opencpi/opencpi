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

#include "ocpi-config.h"
#include "OcpiOsMisc.h"
#include "RccContainer.h"
#include "RCC_Worker.h"

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OR = OCPI::RDT;

namespace OCPI {
  namespace RCC {

    bool Container::m_wqInit = false;
    pthread_workqueue_t Container::m_workqueues[2];

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
  const char *system = OU::getSystemId().c_str();
  m_model = "rcc";
  addTransport("ocpi-dma-pio", system, OR::ActiveMessage, OR::ActiveMessage,
	       //	       (1 << OR::FlagIsCounting) | // ask for counting flags
	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive),
	       //	       (1 << OR::FlagIsCounting) | // ask for counting flags
	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::FlagIsMetaOptional));
  addTransport("ocpi-smb-pio", system, OR::ActiveMessage, OR::ActiveMessage,
	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive),
	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive));
  addTransport("ocpi-scif-dma", system, OR::ActiveMessage, OR::ActiveMessage,
	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive),
	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive));
  addTransport("ocpi-socket-rdma", NULL, OR::ActiveFlowControl, OR::ActiveMessage,
	       (1 << OR::ActiveFlowControl) | (1 << OR::FlagIsMeta),
	       (1 << OR::ActiveMessage) | (1 << OR::FlagIsMeta));
  //	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive),
  //	       (1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::FlagIsMetaOptional));
  addTransport("ocpi-udp-rdma", NULL, OR::ActiveFlowControl, OR::ActiveMessage,
	       (1 << OR::ActiveFlowControl) | (1 << OR::FlagIsMeta),
	       (1 << OR::ActiveMessage) | (1 << OR::FlagIsMeta)),
  addTransport("ocpi-ether-rdma", NULL, OR::ActiveFlowControl, OR::ActiveMessage,
	       (1 << OR::ActiveFlowControl) | (1 << OR::FlagIsMeta),
	       (1 << OR::ActiveMessage) | (1 << OR::FlagIsMeta));
  m_dynamic = OC::Manager::dynamic();
  if (parent().m_platform.size())
    m_platform = parent().m_platform;
  initWorkQueues();
}

void Container::
initWorkQueues() {
#ifdef OCPI_OS_macos
  return;
#endif
  OU::SelfAutoMutex guard(this);
  if (m_wqInit == false ) {
    m_wqInit = true;
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

#ifdef NEEDED
    if (pthread_workqueue_attr_init_np(&attr) != 0)
      throw  OU::Error("Worker static initialization: Could not init workqueue attributes");
    if (pthread_workqueue_attr_setqueuepriority_np(&attr, WORKQ_LOW_PRIOQUEUE) != 0) 
      throw  OU::Error("Worker static initialization: Could not set workqueue priorities");        
    if (pthread_workqueue_create_np(&m_workqueues[LOW_PRI_Q], &attr) != 0)
      throw  OU::Error("Worker static initialization: Could not create workqueue ");
#endif

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
  this->lock();
  OC::Container::shutdown();
  // We need to shut down the apps and workers since they
  // depend on artifacts and transport.
  OU::Parent<Application>::deleteChildren();
}


struct Wargs {
  RCCUserTask *taskc;
  void (*task)(void *);
  void * args;
};

static volatile int task_count = 0;
void wait_join(void *args) {
  OCPI::OS::Semaphore *sem = (OCPI::OS::Semaphore *)args;
  while (task_count > 0) {
    OCPI::OS::sleep(0);
  }
  sem->post();  
}

bool Container::
join( bool block, OCPI::OS::Semaphore & sem ) {
  if (task_count == 0)
    return true;
  if (!block)
    return false;
  pthread_workqueue_additem_np(m_workqueues[LOW_PRI_Q], wait_join, (void*)&sem, NULL, NULL);    

  ocpiDebug("IN Con join about to wait for sem");
  sem.wait();
  ocpiDebug("IN Con join, joined");
  return true;
}

static OCPI::OS::Mutex mutex;
static void 
taskWrapper(void *args) {
  Wargs *wargs = (Wargs*)args;

  if (wargs->taskc == NULL)
    wargs->task(wargs->args);
  else
    wargs->taskc->run();
  mutex.lock();
  task_count--;
  mutex.unlock();
  delete wargs;
}

void Container::
addTask(void (*task)(void *), void *args) {
#ifdef OCPI_OS_macos
  assert("RCC task support not available"==0);
#endif
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



void Container::
addTask(OCPI::RCC::RCCUserTask * task) {
#ifdef OCPI_OS_macos
  assert("RCC task support not available"==0);
#endif
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
OC::Container::DispatchRetCode Container::
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

#if 0
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
#endif
  }
}
