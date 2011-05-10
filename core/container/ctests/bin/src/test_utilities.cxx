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
 * Container test utilities
 *
 * 06/23/09 - John Miller
 * Added support for RPL workers.
 *
 * 06/10/09 - John Miller
 * Initial Version
 *
 */
#include <stdio.h>
#include <sstream>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilMisc.h>
#include <DtIntEventHandler.h>
#include <OcpiRDTInterface.h>
#include <OcpiPValue.h>
#include <OcpiWorker.h>
#include <OcpiContainerPort.h>
#include <test_utilities.h>

using namespace OCPI::DataTransport;
using namespace DataTransport::Interface;
using namespace OCPI::Container;
using namespace OCPI;


SignalCb * SignalHandler::m_cb;
bool SignalHandler::once;
bool g_testUtilVerbose=true;
static int   OCPI_RCC_DEFAULT_DATA_BUFFER_SIZE   = 2048;

namespace OCPI {
  namespace CONTAINER_TEST {
    void  dumpPortData( OCPI::Container::Port *  p )
    {
#ifdef PORT_COMPLETE
      OCPI::Container::PortData& pd = reinterpret_cast<PortData&>(p);
      printf("Input = %d\n",  pd.input );
      printf("Buffer count = %d\n",  pd.connectionData.data.desc.nBuffers);
      printf("Buffer size = %d\n", pd.connectionData.data.desc.dataBufferSize );
      printf("External = %d\n", pd.external);
      printf( "   Connection Date:\n");
      printf( "  port = %lld\n", (long long)pd.connectionData.port );
      printf( "  cid = %d\n", pd.connectionData.cid );

      if ( pd.connectionData.data.mode == OCPI::RDT::ProducerDescType ) {
        printf( " desc.emptyFlagBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.emptyFlagBaseAddr );
        printf( " desc.emptyFlagSize = %d\n",  pd.connectionData.data.desc.emptyFlagSize );
        printf( " desc.emptyFlagPitch = %d\n",  pd.connectionData.data.desc.emptyFlagPitch );
        printf( " desc.emptyFlagValue = 0x%llx\n",  (long long)pd.connectionData.data.desc.emptyFlagValue );
        printf( " desc.oob.oep = %s\n", pd.connectionData.data.desc.oob.oep );

      }
      else {
        printf( " desc.nBuffers = %d\n",  pd.connectionData.data.desc.nBuffers );
        printf( " desc.dataBufferBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.dataBufferBaseAddr );
        printf( " desc.dataBufferPitch = %d\n",  pd.connectionData.data.desc.dataBufferPitch );
        printf( " desc.dataBufferSize = %d\n",  pd.connectionData.data.desc.dataBufferSize );
        printf( " desc.metaDataBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.metaDataBaseAddr );
        printf( " desc.metaDataPitch = %d\n",  pd.connectionData.data.desc.metaDataPitch );
        printf( " desc.fullFlagBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.fullFlagBaseAddr );
        printf( " desc.fullFlagSize = %d\n",  pd.connectionData.data.desc.fullFlagSize );
        printf( " desc.fullFlagPitch = %d\n",  pd.connectionData.data.desc.fullFlagPitch );
        printf( " desc.fullFlagValue = 0x%llx\n",  (long long)pd.connectionData.data.desc.fullFlagValue );

        printf( " desc.oob.oep = %s\n", pd.connectionData.data.desc.oob.oep );
      }
#else
      ( void ) p;
#endif

    }
  }
}


void OCPI::CONTAINER_TEST::connectWorkers(std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  ( void ) ca;
  TUPRINTF( "In  OCPI::CONTAINER_TEST::connectWorkers\n");

  std::string localShadowPort;

  std::vector<CWorker*>::iterator wit;
  for ( wit=workers.begin(); wit!=workers.end(); wit++ ) {
    for ( int n=0; n<(*wit)->tPortCount+(*wit)->sPortCount; n++ ) {
      if ( (*wit)->pdata[n].input ){
        continue;
      }
      if ( (*wit)->pdata[n].down_stream_connection.worker ) {

#ifndef NDEBUG
        dumpPortData( (*wit)->pdata[n].down_stream_connection.worker->pdata[(*wit)->pdata[n].down_stream_connection.pid].port);
#endif

	(*wit)->pdata[n].port->connect( *(*wit)->pdata[n].down_stream_connection.worker->pdata[(*wit)->pdata[n].down_stream_connection.pid].port, NULL,NULL);


      }
    }
  }

  TUPRINTF( "Leaving  OCPI::CONTAINER_TEST::connectWorkers\n");
}



void  OCPI::CONTAINER_TEST::testDispatch(OCPI::API::Container* rcc_container)
{

  rcc_container->run(100, true);
#if 0
  // Block here until we get a wakeup notification
  // We will get a notification for the following reasons
  // 1) One of our target buffers has been filled
  // 2) A DMA has completed and we have a output buffer that is now empty
  // 3) A timeout has occured
  if ( event_manager ) {
    do {
      rcc_container->dispatch(event_manager);
      if ( event_manager->waitForEvent( 100  ) == DataTransfer::EventTimeout ) {
        printf("We have not recieved an event for 100 uSec.\n");
      }
      else {
        rcc_container->dispatch( event_manager );
        break;
      }
      
      OCPI::OS::sleep( 1000 );

    } while(1);
  }
  else {
    rcc_container->dispatch( event_manager );
  }
#endif
}



namespace {

  class  DThread : public OCPI::Util::Thread
  {
  public:

    DThread(  OCPI::CONTAINER_TEST::DThreadData* dtd )
      :m_dtd(dtd){};

    void run() {
      while(m_dtd->run) {
        std::vector<CApp>::iterator it;
        for ( it=m_dtd->containers.begin(); it != m_dtd->containers.end(); it++ ) {
          OCPI::CONTAINER_TEST::testDispatch( (*it).container );
        }
        OCPI::OS::sleep(0);
      }
    }

  private:
    OCPI::CONTAINER_TEST::DThreadData* m_dtd;

  };

}


void OCPI::CONTAINER_TEST::createPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  ( void ) ca;
  TUPRINTF( "In  OCPI::CONTAINER_TEST::createPorts\n");

  std::vector<CWorker*>::iterator wit;
  int n;
  for ( wit=workers.begin(); wit!=workers.end(); wit++ ) {
    TUPRINTF( "Worker target port count = %d\n", (*wit)->tPortCount );

    for ( n=0; n<(*wit)->tPortCount; n++ ) {
      (*wit)->pdata[n].port = &
        (*wit)->worker->createInputPort( n,
                                      (*wit)->pdata[n].bufferCount,
                                      OCPI_RCC_DEFAULT_DATA_BUFFER_SIZE, (*wit)->pdata[n].props );
      (*wit)->pdata[n].input = true;
    }

    TUPRINTF( "Worker output port count = %d\n", (*wit)->sPortCount );

    for ( ; n<(*wit)->tPortCount+(*wit)->sPortCount; n++ ) {
      (*wit)->pdata[n].port = &
        (*wit)->worker->createOutputPort( n,
                                       (*wit)->pdata[n].bufferCount,
                                       OCPI_RCC_DEFAULT_DATA_BUFFER_SIZE,(*wit)->pdata[n].props );
      (*wit)->pdata[n].input = false;
    }
  }

  TUPRINTF( "leaving  OCPI::CONTAINER_TEST::createPorts\n");
}


OCPI::Util::Thread*  OCPI::CONTAINER_TEST::runTestDispatch(  OCPI::CONTAINER_TEST::DThreadData& tdata )
{
  DThread* dThread = new DThread(&tdata);
  dThread->start();
  return dThread;
}



std::vector<CApp> OCPI::CONTAINER_TEST::createContainers( std::vector<ContainerDesc>& eps, 
                                                         DataTransfer::EventManager*& ,
                                                         bool )
{
  TUPRINTF( "In  OCPI::CONTAINER_TEST::createContainers\n");

  CApp ca;
  std::vector<CApp> containers;

  static OCPI::Util::PValue cprops[] = {
                                       OCPI::Util::PVBool("polling",1),
                                       OCPI::Util::PVEnd };
  // Create the container
  try { 
    for ( unsigned int n=0; n<eps.size();n++) { 

      OCPI::API::Container *c = OCPI::API::ContainerManager::find("rcc", NULL, cprops);
      if ( ! c )
	throw OCPI::Util::EmbeddedException("No Containers found\n");
      ca.container = static_cast<OCPI::Container::Container*>(c);
      ca.worker = dynamic_cast<OCPI::Container::Worker*>(ca.container);
      ca.app = ca.container->createApplication();
      containers.push_back(ca);
      //event_manager = ca.container->getEventManager();
    }
  }
  CATCH_ALL_RETHROW( "creating containers");
  TUPRINTF( "Leaving  OCPI::CONTAINER_TEST::createContainers\n");
  return containers;
}

std::vector<CApp> OCPI::CONTAINER_TEST::createContainers( std::vector<const char*>& eps, 
                                                         DataTransfer::EventManager*& event_manager, bool polling )
{
  ( void ) eps;
  ( void ) polling;
  ( void ) event_manager;
  CApp ca;
  int ccount=3;


  std::vector<CApp> containers;

  static OCPI::Util::PValue cprops[] = {
                                       OCPI::Util::PVBool("polling",1),
                                       OCPI::Util::PVEnd };

  // Create the container
  try { 
    for ( int n=0; n<ccount;n++) { 
      std::string name;
      OCPI::Util::Misc::formatString(name, "rcc%u", n);
      OCPI::API::Container *c = OCPI::API::ContainerManager::find("rcc", name.c_str(), cprops);
      if ( ! c )
	throw OCPI::Util::EmbeddedException("No Containers found\n");
      ca.container = c;
      ca.worker = dynamic_cast<OCPI::Container::Worker*>(ca.container);
      ca.app = ca.container->createApplication();
      containers.push_back(ca);
      //      event_manager = ca.container->getEventManager();
    }
  }
  CATCH_ALL_RETHROW( "creating containers");
  return containers;
}



void OCPI::CONTAINER_TEST::disconnectPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  ( void ) ca;
  std::vector<CWorker*>::iterator wit;

  for ( wit=workers.begin(); wit!=workers.end(); wit++ ) {
    for ( int n=0; n<((*wit)->sPortCount+(*wit)->tPortCount); n++ ) {
      if ( (*wit)->pdata[n].port ) {
        (*wit)->pdata[n].port->disconnect();
      }
    }
  }
}

void OCPI::CONTAINER_TEST::destroyWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  ( void ) workers;
  // Destroy the app contexts
  std::vector<CApp>::iterator cait;
  for ( cait=ca.begin(); cait!= ca.end(); cait++ ) {
    if ( cait->container ) {
      delete cait->app;
      cait->app = NULL;
      //      cait->app = cait->container->createApplication();
    }
  }
  std::vector<CWorker*>::iterator it;
  for ( it = workers.begin(); it != workers.end(); it++ ) {
    (*it)->worker = NULL;
  }
}




void OCPI::CONTAINER_TEST::destroyContainers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  ( void ) workers;
  // Destroy the app contexts
  std::vector<CApp>::iterator cait;
  for ( cait=ca.begin(); cait!= ca.end(); cait++ ) {
    if ( cait->container ) {
      if (cait->app)
	delete cait->app;
      delete cait->container;
    }
  }
  ca.clear();
}

void OCPI::CONTAINER_TEST::enableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )

{
  ( void ) ca;
  std::vector<CWorker*>::iterator it;
  for ( it = workers.begin(); it != workers.end(); it++ ) {
    (*it)->worker->start();
  }
}


void OCPI::CONTAINER_TEST::disableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  ( void ) ca;
  std::vector<CWorker*>::iterator it;
  for ( it = workers.begin(); it != workers.end(); it++ ) {
    (*it)->worker->stop();
  }
}
OCPI::Container::Worker *OCPI::CONTAINER_TEST::createWorker(OCPI::API::ContainerApplication *app, RCCDispatch *rccd) {
  OCPI::API::Worker &w = app->createWorker(NULL, NULL, (const char *)rccd, NULL, NULL, NULL);
  return static_cast<OCPI::Container::Worker *>(&w);
}

OCPI::Container::Worker *OCPI::CONTAINER_TEST::createWorker(CApp &ca, RCCDispatch *rccd) {
  if (!ca.app)
      ca.app = ca.container->createApplication();
  return createWorker(ca.app, rccd);
}





