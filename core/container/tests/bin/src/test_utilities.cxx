// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

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
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <CpiRDTInterface.h>
#include <CpiDriver.h>
#include <CpiPValue.h>
#include <test_utilities.h>
#include <CpiWorker.h>
#include <CpiContainerPort.h>

using namespace CPI::DataTransport;
using namespace DataTransport::Interface;
using namespace CPI::Container;
using namespace CPI;


SignalCb * SignalHandler::m_cb;
bool SignalHandler::once;
bool g_testUtilVerbose=true;
static int   CPI_RCC_DEFAULT_DATA_BUFFER_SIZE   = 2048;

namespace CPI {
  namespace CONTAINER_TEST {
    void  dumpPortData( CPI::Container::Port *  p )
    {
#ifdef PORT_COMPLETE
      CPI::Container::PortData& pd = reinterpret_cast<PortData&>(p);
      printf("Input = %d\n",  pd.input );
      printf("Buffer count = %d\n",  pd.connectionData.data.desc.nBuffers);
      printf("Buffer size = %d\n", pd.connectionData.data.desc.dataBufferSize );
      printf("External = %d\n", pd.external);
      printf( "   Connection Date:\n");
      printf( "  port = %lld\n", (long long)pd.connectionData.port );
      printf( "  cid = %d\n", pd.connectionData.cid );

      if ( pd.connectionData.data.mode == CPI::RDT::ProducerDescType ) {
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
#endif

    }
  }
}


void CPI::CONTAINER_TEST::connectWorkers(std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  TUPRINTF( "In  CPI::CONTAINER_TEST::connectWorkers\n");

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

  TUPRINTF( "Leaving  CPI::CONTAINER_TEST::connectWorkers\n");
}



void  CPI::CONTAINER_TEST::testDispatch(CPI::Container::Interface* rcc_container, DataTransfer::EventManager* event_manager)
{

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
    } while(1);
  }
  else {
    rcc_container->dispatch( event_manager );
  }

}



namespace {

  class  DThread : public CPI::Util::Thread
  {
  public:

    DThread(  CPI::CONTAINER_TEST::DThreadData* dtd )
      :m_dtd(dtd){};

    void run() {
      while(m_dtd->run) {
        std::vector<CApp>::iterator it;
        for ( it=m_dtd->containers.begin(); it != m_dtd->containers.end(); it++ ) {
          CPI::CONTAINER_TEST::testDispatch( (*it).container, m_dtd->event_manager );
        }
        CPI::OS::sleep(1);
      }
    }

  private:
    CPI::CONTAINER_TEST::DThreadData* m_dtd;

  };

}


void CPI::CONTAINER_TEST::createPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{

  TUPRINTF( "In  CPI::CONTAINER_TEST::createPorts\n");

  std::vector<CWorker*>::iterator wit;
  int n;
  for ( wit=workers.begin(); wit!=workers.end(); wit++ ) {
    TUPRINTF( "Worker target port count = %d\n", (*wit)->tPortCount );

    for ( n=0; n<(*wit)->tPortCount; n++ ) {
      (*wit)->pdata[n].port = &
        (*wit)->worker->createInputPort( n,
                                      (*wit)->pdata[n].bufferCount,
                                      CPI_RCC_DEFAULT_DATA_BUFFER_SIZE, (*wit)->pdata[n].props );
      (*wit)->pdata[n].input = true;
    }

    TUPRINTF( "Worker output port count = %d\n", (*wit)->sPortCount );

    for ( ; n<(*wit)->tPortCount+(*wit)->sPortCount; n++ ) {
      (*wit)->pdata[n].port = &
        (*wit)->worker->createOutputPort( n,
                                       (*wit)->pdata[n].bufferCount,
                                       CPI_RCC_DEFAULT_DATA_BUFFER_SIZE,(*wit)->pdata[n].props );
      (*wit)->pdata[n].input = false;
    }
  }

  TUPRINTF( "leaving  CPI::CONTAINER_TEST::createPorts\n");
}


CPI::Util::Thread*  CPI::CONTAINER_TEST::runTestDispatch(  CPI::CONTAINER_TEST::DThreadData& tdata )
{
  DThread* dThread = new DThread(&tdata);
  dThread->start();
  return dThread;
}


static CPI::Util::DriverManager dm("Container");
std::vector<CApp> CPI::CONTAINER_TEST::createContainers( std::vector<ContainerDesc>& eps, 
                                                         DataTransfer::EventManager*& event_manager,
                                                         bool polling )
{

  TUPRINTF( "In  CPI::CONTAINER_TEST::createContainers\n");

  CApp ca;
  std::vector<CApp> containers;

  static CPI::Util::PValue cprops[] = {
                                       CPI::Util::PVBool("polling",1),
                                       CPI::Util::PVEnd };


  dm.discoverDevices(0,0);
  // Create the container
  try { 
    for ( unsigned int n=0; n<eps.size();n++) { 

      CPI::Util::Device* d = dm.getDevice( cprops, eps[n].type.c_str());
      if ( ! d ) {
        throw CPI::Util::EmbeddedException("No Containers found\n");
      }
      ca.container = static_cast<CPI::Container::Interface*>(d);
      ca.wci_worker = dynamic_cast<CPI::WCI::Worker*>(ca.container);
      ca.app = ca.container->createApplication();
      containers.push_back(ca);
      event_manager = ca.container->getEventManager();
    }
  }
  CATCH_ALL_RETHROW( "creating containers");
  TUPRINTF( "Leaving  CPI::CONTAINER_TEST::createContainers\n");
  return containers;
}

std::vector<CApp> CPI::CONTAINER_TEST::createContainers( std::vector<char*>& eps, 
                                                         DataTransfer::EventManager*& event_manager, bool polling )
{
  CApp ca;
  int ccount=3;


  std::vector<CApp> containers;

  static CPI::Util::PValue cprops[] = {
                                       CPI::Util::PVBool("polling",1),
                                       CPI::Util::PVEnd };

  dm.discoverDevices(0,0);
  // Create the container
  try { 
    for ( int n=0; n<ccount;n++) { 

      CPI::Util::Device* d = dm.getDevice( cprops, "RCC");
      if ( ! d ) {
        throw CPI::Util::EmbeddedException("No Containers found\n");
      }
      ca.container = static_cast<CPI::Container::Interface*>(d);
      ca.wci_worker = dynamic_cast<CPI::WCI::Worker*>(ca.container);
      ca.app = ca.container->createApplication();
      containers.push_back(ca);
      event_manager = ca.container->getEventManager();
    }
  }
  CATCH_ALL_RETHROW( "creating containers");
  return containers;
}



void CPI::CONTAINER_TEST::disconnectPorts( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  std::vector<CWorker*>::iterator wit;

  for ( wit=workers.begin(); wit!=workers.end(); wit++ ) {
    for ( int n=0; n<((*wit)->sPortCount+(*wit)->tPortCount); n++ ) {
      if ( (*wit)->pdata[n].port ) {
        (*wit)->pdata[n].port->disconnect();
      }
    }
  }
}

void CPI::CONTAINER_TEST::destroyWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  // Destroy the app contexts
  std::vector<CApp>::iterator cait;
  for ( cait=ca.begin(); cait!= ca.end(); cait++ ) {
    if ( cait->container ) {
      delete cait->app;
      cait->app = cait->container->createApplication();
    }
  }
}




void CPI::CONTAINER_TEST::destroyContainers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  // Destroy the app contexts
  std::vector<CApp>::iterator cait;
  for ( cait=ca.begin(); cait!= ca.end(); cait++ ) {
    if ( cait->container ) {
      delete cait->app;
      delete  cait->container;
      cait->container = NULL;
    }
  }
  ca.clear();
}


void CPI::CONTAINER_TEST::initWorkers(std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{
  WCI_error wcie;
  std::vector<CWorker*>::iterator it;
  for ( it = workers.begin(); it != workers.end(); it++ ) {
    CWorker& cw = *(*it);
    wcie = (*it)->worker->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
    CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_INITIALIZE, ca, cw );

  }
}

void CPI::CONTAINER_TEST::enableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )

{
  WCI_error wcie;
  std::vector<CWorker*>::iterator it;
  for ( it = workers.begin(); it != workers.end(); it++ ) {
    CWorker& cw = *(*it);
    wcie = (*it)->worker->control(  WCI_CONTROL_START, WCI_DEFAULT );
    CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_START, ca, cw );
  }
}


void CPI::CONTAINER_TEST::disableWorkers( std::vector<CApp>& ca, std::vector<CWorker*>& workers )
{

  WCI_error wcie;
  std::vector<CWorker*>::iterator it;
  for ( it = workers.begin(); it != workers.end(); it++ ) {
    CWorker& cw = *(*it);
    wcie = (*it)->worker->control(  WCI_CONTROL_STOP, WCI_DEFAULT );
    CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_STOP, ca, cw );
  }
}





