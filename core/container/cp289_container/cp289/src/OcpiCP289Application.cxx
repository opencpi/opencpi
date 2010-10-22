
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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#define WORKER_INTERNAL
#include <OcpiCP289Port.h>
#include <OcpiCP289Container.h>
#include <OcpiCP289Controller.h>
#include <OcpiCP289Application.h>
#include <OcpiOsMisc.h>
#include <OcpiTransport.h>
#include <OcpiRDTInterface.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilCDR.h>
#include <OcpiPortMetaData.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiContainerErrorCodes.h>
#include <DtTransferInternal.h>
#include <OcpiIntParallelDataDistribution.h>
#include <OcpiPValue.h>
#include <OcpiUtilMisc.h>
#include <OcpiArtifact.h>
#include <OcpiOsAssert.h>


using namespace OCPI::Container;
using namespace OCPI::Util;
using namespace OCPI::CP289;

#define MyParent static_cast<OCPI::CP289::Container*>(myParent)
#define MyAppParent static_cast<OCPI::CP289::Application*>(myParent)



OCPI::CP289::Artifact::
Artifact(Interface & p, const char *url)
  : OCPI::Container::Artifact(p,url), m_open(false)
{
  ocpiAssert( url );
  try {
    m_loader.open( url );
    m_open = true;
  }
  catch( std::string & error ) {
    std::string my_err("Could not open RCC worker DLL ");
    my_err += url;
    OCPI::Util::EmbeddedException ex(my_err.c_str());
    ex.setAuxInfo( error.c_str() );
    throw ex;
  }
}


OCPI::Container::Worker &
OCPI::CP289::Artifact::
createWorkerX( OCPI::Container::Application &app, ezxml_t impl, ezxml_t inst, OCPI::Util::PValue *props)
{
  const char* entryPoint = "ocpi_EntryTable";

  // First get the entry point from the properties
  const OCPI::Util::PValue*  p = OCPI::Util::PValue::find(props, "DLLEntryPoint");
  if ( p ) {
    if ( p->type != OCPI::Util::Prop::Scalar::OCPI_String) {
      throw OCPI::Util::EmbeddedException("\"DLLEntryPoint\" property has wrong type, should be String");
    }
    entryPoint = p->vString;
  }

  ::RCCEntryTable * et = (::RCCEntryTable *)m_loader.getSymbol( entryPoint );
  if ( ! et ) {
    std::string error("Worker DLL entry point not found ");
    error += entryPoint;
    throw OCPI::Util::EmbeddedException( error.c_str() );
  }

  const char * implName = ezxml_attr(impl, "name");
  ocpiAssert(implName);

  for (; et->name; et++)
    if (!strcmp(et->name, implName))
      return *new OCPI::CP289::Worker( app, et->dispatch, props, MyParent, impl, inst );
  std::string error("Worker not found in DLL -> ");
  error += implName;
  throw OCPI::Util::EmbeddedException( error.c_str() );
}



OCPI::CP289::Artifact::
~Artifact()

{
  m_loader.close();
  m_open = false;
}

bool 
OCPI::CP289::Artifact::
hasUrl(const char *url)
{ 
  ( void ) url;
  return myUrl ? true : false;
}


/**********************************
 * Constructor
 *********************************/  
OCPI::CP289::Application::
Application(OCPI::CP289::Container &c, OCPI::OS::Mutex& m )
  : OCPI::Container::Application(c), m_mutex(m) 
{
  // Empty
}

/**********************************
 * Destructors
 *********************************/  
OCPI::CP289::Application::
~Application()
{
  // Lock out the dispatch loop until all of our children
  // are removed.
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  // If we let our base class destroy our childen, it cant lock out the dispatch thread :(
  Worker * worker = static_cast<OCPI::CP289::Worker*>(firstChild());
  while ( worker ) {
    Worker* next_worker =  static_cast<OCPI::CP289::Worker*>(nextChild(worker));
    delete worker;
    worker = next_worker;
  }
}


OCPI::Container::Artifact &
OCPI::CP289::Application::
createArtifact(const char *url, OCPI::Util::PValue *)
{
  return *(new OCPI::CP289::Artifact( *MyParent, url ));
}


OCPI::Container::Worker &
OCPI::CP289::Application::
createWorker(const char *url, OCPI::Util::PValue *aparams,
             const char *entryPoint, const char *inst, OCPI::Util::PValue *wparams )
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  if (!url)
    return *(new OCPI::CP289::Worker( *this, (RCCDispatch *)entryPoint, wparams, MyParent, NULL, NULL ));
  else
    return OCPI::Container::Application::createWorker(url, aparams, entryPoint, inst, wparams);
}

void 
OCPI::CP289::Application::
removeWorker( WorkerId worker )
{
  Worker *ws = reinterpret_cast<Worker*>(worker);
  delete ws;
}

OCPI::DataTransport::Transport & 
OCPI::CP289::Application::
getTransport()
{
  return *(MyParent->m_transport);
}
