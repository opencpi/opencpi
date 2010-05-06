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
 * Abstact:
 *   This file contains the interface for the CPI RCC Container.  
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
#include <CpiOsMisc.h>
#include <CpiTransport.h>
#include <CpiOsAssert.h>
#include <CpiUtilCDR.h>
#include <CpiRDTInterface.h>
#include <CpiCP289Port.h>
#include <CpiCP289Container.h>
#include <CpiCP289Controller.h>
#include <CpiCP289Application.h>
#include <CpiPortMetaData.h>
#include <CpiUtilAutoMutex.h>
#include <CpiContainerErrorCodes.h>
#include <DtTransferInternal.h>
#include <CpiIntParallelDataDistribution.h>
#include <CpiPValue.h>
#include <CpiUtilMisc.h>
#include <RCC_Worker.h>

using namespace CPI::Container;
using namespace CPI::Util;
using namespace CPI::OS;
using namespace CPI::RDT;


DataTransfer::EventManager*  
CPI::CP289::Container::
getEventManager()
{
  return m_transport->m_transportGlobal->getEventManager();
}

CPI::CP289::Container::
Container(  CPI::Util::Driver &d,
            CPI::OS::uint32_t g_unique_id, CPI::DataTransport::TransportGlobal *tpg, 
            const CPI::Util::PValue* props )
  throw ( CPI::Util::EmbeddedException )
  :CPI::Container::Interface( d, "RCC Container Instance", props), 
   CPI::CP289::Controller( this ),
   m_threadSafeMutex(true)
{
  m_ourUID = g_unique_id;

  // The underlying tranport system has some number of endpoints registered.  Lets make sure
  // that their is at least 1 available or we will complain.
  // Initialize the underlying transport system
  try {
    m_transport = new CPI::DataTransport::Transport( tpg, false );
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }

  const CPI::Util::PValue*  p = CPI::Util::PValue::find(props, "monitorIPAddress");
  const char* monitorIPAddress = NULL;
  if ( p ) {
    if ( p->type != CPI::Metadata::Property::CPI_String) {
      throw CPI::Util::EmbeddedException("\"monitorIPAddress\" property has wrong type, should be String");
    }
    monitorIPAddress = p->vString;
  }

  start(NULL);
}



CPI::Container::Artifact & 
CPI::CP289::Container::
createArtifact(const char *url, CPI::Util::PValue *artifactParams)
{
  return *(new Artifact(*this, url));
}


/**********************************
 * Destructor
 *********************************/
CPI::CP289::Container::
~Container()
  throw ()
{
  TRACE( "CPI::CP289::Container::~Container()");

  try {
    delete m_transport;
  }
  catch( ... ) {
    printf("ERROR: Got an exception in CPI::CP289::Container::~Container)\n");
  }


  // Because we own and share our mutex with our children, we need to remove them while the
  // mutex is still valid.
  CPI::CP289::Application * app = 
    static_cast<CPI::CP289::Application*>( this->Parent<CPI::Container::Application>::firstChild() );
  while ( app ) {
    CPI::CP289::Application * next_app =  
      static_cast<CPI::CP289::Application*>( this->Parent<CPI::Container::Application>::nextChild(app) );
    delete app;
    app = next_app;
  }

}

/**********************************
 * For single threaded containers, this is the dispatch hook
 *********************************/
CPI::Container::Interface::DispatchRetCode 
CPI::CP289::Container::
dispatch(DataTransfer::EventManager* event_manager)
  throw ( CPI::Util::EmbeddedException )
{
  bool more_to_do = false;
  if ( ! m_start ) {
    return Stopped;
  }
  CPI::Util::AutoMutex guard ( m_threadSafeMutex, true ); 

  if ( run( event_manager ) ) {
    more_to_do = true;
  }
  return more_to_do ? MoreWorkNeeded : Spin;
}


/**********************************
 * Creates an application 
 *********************************/
Application * 
CPI::CP289::Container::
createApplication()
  throw ( CPI::Util::EmbeddedException )
{
  TRACE( "CPI::CP289::Container::createApplication()");
  CPI::Util::AutoMutex guard ( m_threadSafeMutex, true ); 

  CPI::CP289::Application* ca;
  try {
    ca = new CPI::CP289::Application(*this,m_threadSafeMutex);
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }
  return ca;
}


std::vector<std::string> 
CPI::CP289::Container::
getSupportedEndpoints()
        throw ()
{
  return m_transport->getListOfSupportedEndpoints();
}


void 
CPI::CP289::Container::
start(DataTransfer::EventManager* event_manager)
  throw()
{
  try {
    m_start = true;

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
CPI::CP289::Container::
stop(DataTransfer::EventManager* event_manager)
  throw()
{
  try {
    m_start = false;
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

