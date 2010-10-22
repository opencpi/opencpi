
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
#include <OcpiCP289Port.h>
#include <OcpiCP289Container.h>
#include <OcpiCP289Controller.h>
#include <OcpiCP289Application.h>
#include <OcpiPortMetaData.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiContainerErrorCodes.h>
#include <DtTransferInternal.h>
#include <OcpiIntParallelDataDistribution.h>
#include <OcpiPValue.h>
#include <OcpiUtilMisc.h>
#include <RCC_Worker.h>

using namespace OCPI::Container;
using namespace OCPI::Util;
using namespace OCPI::OS;
using namespace OCPI::RDT;


DataTransfer::EventManager*  
OCPI::CP289::Container::
getEventManager()
{
  return m_transport->m_transportGlobal->getEventManager();
}

OCPI::CP289::Container::
Container(  OCPI::Util::Driver &drv,
            OCPI::OS::uint32_t g_unique_id, OCPI::DataTransport::TransportGlobal *tpg, 
            const OCPI::Util::PValue* props )
  throw ( OCPI::Util::EmbeddedException )
  :OCPI::Container::Interface( drv, "RCC Container Instance", props), 
   OCPI::CP289::Controller( this ),
   m_threadSafeMutex(true)
{
  m_ourUID = g_unique_id;

  // The underlying tranport system has some number of endpoints registered.  Lets make sure
  // that their is at least 1 available or we will complain.
  // Initialize the underlying transport system
  try {
    m_transport = new OCPI::DataTransport::Transport( tpg, false );
  }
  catch( std::bad_alloc ) {
    throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }

  const OCPI::Util::PValue*  p = OCPI::Util::PValue::find(props, "monitorIPAddress");
  const char* monitorIPAddress = NULL;
  if ( p ) {
    if ( p->type != OCPI::Util::Prop::Scalar::OCPI_String) {
      throw OCPI::Util::EmbeddedException("\"monitorIPAddress\" property has wrong type, should be String");
    }
    monitorIPAddress = p->vString;
  }

  start(NULL);
}



OCPI::Container::Artifact & 
OCPI::CP289::Container::
createArtifact(const char *url, OCPI::Util::PValue *artifactParams)
{
  ( void ) artifactParams;
  return *(new Artifact(*this, url));
}


/**********************************
 * Destructor
 *********************************/
OCPI::CP289::Container::
~Container()
  throw ()
{
  TRACE( "OCPI::CP289::Container::~Container()");

  try {
    delete m_transport;
  }
  catch( ... ) {
    printf("ERROR: Got an exception in OCPI::CP289::Container::~Container)\n");
  }


  // Because we own and share our mutex with our children, we need to remove them while the
  // mutex is still valid.
  OCPI::CP289::Application * app = 
    static_cast<OCPI::CP289::Application*>( this->Parent<OCPI::Container::Application>::firstChild() );
  while ( app ) {
    OCPI::CP289::Application * next_app =  
      static_cast<OCPI::CP289::Application*>( this->Parent<OCPI::Container::Application>::nextChild(app) );
    delete app;
    app = next_app;
  }

}

/**********************************
 * For single threaded containers, this is the dispatch hook
 *********************************/
OCPI::Container::Interface::DispatchRetCode 
OCPI::CP289::Container::
dispatch(DataTransfer::EventManager* event_manager)
  throw ( OCPI::Util::EmbeddedException )
{
  bool more_to_do = false;
  if ( ! m_start ) {
    return Stopped;
  }
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex, true ); 

  if ( run( event_manager ) ) {
    more_to_do = true;
  }
  return more_to_do ? MoreWorkNeeded : Spin;
}


/**********************************
 * Creates an application 
 *********************************/
Application * 
OCPI::CP289::Container::
createApplication()
  throw ( OCPI::Util::EmbeddedException )
{
  TRACE( "OCPI::CP289::Container::createApplication()");
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex, true ); 

  OCPI::CP289::Application* ca;
  try {
    ca = new OCPI::CP289::Application(*this,m_threadSafeMutex);
  }
  catch( std::bad_alloc ) {
    throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }
  return ca;
}


std::vector<std::string> 
OCPI::CP289::Container::
getSupportedEndpoints()
        throw ()
{
  return m_transport->getListOfSupportedEndpoints();
}


void 
OCPI::CP289::Container::
start(DataTransfer::EventManager* event_manager)
  throw()
{
 ( void ) event_manager;
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
OCPI::CP289::Container::
stop(DataTransfer::EventManager* event_manager)
  throw()
{
  ( void ) event_manager;
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

