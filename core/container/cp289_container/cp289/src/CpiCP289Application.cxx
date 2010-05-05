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
#include <CpiCP289Port.h>
#include <CpiCP289Container.h>
#include <CpiCP289Controller.h>
#include <CpiCP289Application.h>
#include <CpiOsMisc.h>
#include <CpiTransport.h>
#include <CpiRDTInterface.h>
#include <CpiOsAssert.h>
#include <CpiUtilCDR.h>
#include <CpiPortMetaData.h>
#include <CpiUtilAutoMutex.h>
#include <CpiContainerErrorCodes.h>
#include <DtTransferInternal.h>
#include <CpiIntParallelDataDistribution.h>
#include <CpiPValue.h>
#include <CpiUtilMisc.h>
#include <CpiArtifact.h>


using namespace CPI::Container;
using namespace CPI::Util;
using namespace CPI::CP289;

#define MyParent static_cast<CPI::CP289::Container*>(myParent)
#define MyAppParent static_cast<CPI::CP289::Application*>(myParent)



CPI::CP289::Artifact::
Artifact(Interface & p, const char *url)
  : CPI::Container::Artifact(p,url), m_open(false)
{
  cpiAssert( url );
  try {
    m_loader.open( url );
    m_open = true;
  }
  catch( std::string & error ) {
    std::string my_err("Could not open RCC worker DLL ");
    my_err += url;
    CPI::Util::EmbeddedException ex(my_err.c_str());
    ex.setAuxInfo( error.c_str() );
    throw ex;
  }
}


CPI::Container::Worker &
CPI::CP289::Artifact::
createWorkerX( CPI::Container::Application &app, ezxml_t impl, ezxml_t inst, CPI::Util::PValue *props)
{
  const char* entryPoint = NULL;

  // First get the entry point from the properties
  const CPI::Util::PValue*  p = CPI::Util::PValue::find(props, "DLLEntryPoint");
  if ( p ) {
    if ( p->type != CPI::Metadata::Property::CPI_String) {
      throw CPI::Util::EmbeddedException("\"DLLEntryPoint\" property has wrong type, should be String");
    }
    entryPoint = p->vString;
  }
  else {  
    entryPoint = "WorkerDispatchTables";
  }

  void * ep = m_loader.getSymbol( entryPoint );
  if ( ! ep ) {
    std::string error("Worker DLL entry point not found ");
    error += entryPoint;
    throw CPI::Util::EmbeddedException( error.c_str() );
  }

  DllDispatchEntry * de = (DllDispatchEntry * )ep;
  
  //  RCCDispatch ** eps = (RCCDispatch **)ep;

  int wi=0;
  bool found = false;  
  const char * implName = ezxml_attr(impl, "name");
  while ( de[wi].worker_name ) {
    if ( implName && (strcmp( de[wi].worker_name, (char*)implName) == 0) ) {
      found = true;
      break;
    }
    wi++;
  }

  Worker * w;
  if ( found ) {
    w = new CPI::CP289::Worker( app, de[wi].dt, props, MyParent, impl, inst );
  }
  else {
    std::string error("Worker not found in DLL -> ");
    error += (char*)implName;
    throw CPI::Util::EmbeddedException( error.c_str() );
  }

  return *w;
}



CPI::CP289::Artifact::
~Artifact()

{
  m_loader.close();
  m_open = false;
}

bool 
CPI::CP289::Artifact::
hasUrl(const char *url)
{ 
  return myUrl ? true : false;
}


/**********************************
 * Constructor
 *********************************/  
CPI::CP289::Application::
Application(CPI::CP289::Container &c, CPI::OS::Mutex& m )
  : CPI::Container::Application(c), m_mutex(m) 
{
  // Empty
}

/**********************************
 * Destructors
 *********************************/  
CPI::CP289::Application::
~Application()
{
  // Lock out the dispatch loop until all of our children
  // are removed.
  CPI::Util::AutoMutex guard ( m_mutex, true ); 

  // If we let our base class destroy our childen, it cant lock out the dispatch thread :(
  Worker * worker = static_cast<CPI::CP289::Worker*>(firstChild());
  while ( worker ) {
    Worker* next_worker =  static_cast<CPI::CP289::Worker*>(nextChild(worker));
    delete worker;
    worker = next_worker;
  }
}


CPI::Container::Artifact &
CPI::CP289::Application::
createArtifact(const char *url, CPI::Util::PValue *)
{
  return *(new CPI::CP289::Artifact( *MyParent, url ));
}


CPI::Container::Worker &
CPI::CP289::Application::
createWorker(const char *url, CPI::Util::PValue *aparams,
	     const void *entryPoint, const char *inst, CPI::Util::PValue *wparams )
{
  CPI::Util::AutoMutex guard ( m_mutex, true ); 
  return *(new CPI::CP289::Worker( *this, entryPoint, wparams, MyParent, NULL, NULL ));
}


void 
CPI::CP289::Application::
removeWorker( WorkerId worker )
{
  Worker *ws = reinterpret_cast<Worker*>(worker);
  delete ws;
}

CPI::DataTransport::Transport & 
CPI::CP289::Application::
getTransport()
{
  return *(MyParent->m_transport);
}
