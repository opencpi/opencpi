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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#include "RccWorker.h"
#include "RccContainer.h"
#include "RccApplication.h"

namespace OCPI {
  namespace RCC {
    namespace OC = OCPI::Container;
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;

Artifact::
Artifact(Container &c, OCPI::Library::Artifact &lart, const OA::PValue *props)
  : OC::ArtifactBase<Container,Artifact>(c, *this, lart, props),
    m_entryTable(NULL), m_open(false), m_workerCount(0)
{
  const char *url = lart.name().c_str();
  std::string err;
  const char* entryPoint = "ocpi_EntryTable";
  try {
    ocpiInfo("Loading RCC worker artifact %s", url);
    m_loader.open(url);
    m_open = true;
    OU::findString(props, "DLLEntryPoint", entryPoint);
    m_entryTable = (RCCEntryTable *)m_loader.getSymbol(entryPoint);
  } catch (std::string &error) {
    OU::format(err, "Could not open RCC worker file %s: %s", url, error.c_str());
    throw err;
  } catch (...) {
    OU::format(err, "Unknown exception opening RCC worker file %s", url);
    throw err;
  }
  if (!m_entryTable) {
    OU::format(err, "Worker DLL entry point not found: %s", entryPoint);
    throw err;
  }
}

RCCEntryTable *Artifact::
getDispatch(const char *implName) {
  for (RCCEntryTable *et = m_entryTable; et->name; et++)
    if (!strcmp(et->name, implName))
      return et;
  std::string error;
  OU::format(error, "Worker \"%s\" not found in artifact file \"%s\" (found",
	     implName, name().c_str());
  for (RCCEntryTable *et = m_entryTable; et->name; et++)
    OU::formatAdd(error, "%s \"%s\"", et == m_entryTable ? "" : ",", et->name);
  error += ")";
  throw OU::Error(error);
}      

Artifact::
~Artifact()

{
  m_loader.close();
  m_open = false;
}

#if 0
bool 
Artifact::
hasUrl(const char *url)
{ 
  ( void ) url;
  return myUrl ? true : false;
}
#endif

/**********************************
 * Constructor
 *********************************/  
Application::
Application(Container &c, const char *a_name, const OU::PValue *props)
  : OC::ApplicationBase<Container,Application,Worker>(c, *this, a_name, props)
{
  // Empty
}

/**********************************
 * Destructors
 *********************************/  
Application::
~Application()
{
  // Lock out the dispatch loop until all of our children
  // are removed.
  OU::SelfAutoMutex guard (&parent());

  // We do this explicitly so we can do it under our mutex to lock out
  // the dispatch thread, since we may be destroyed outside the
  // context of our parent's destructor (from the API).
  // The container's dispatch thread doesn't deal with applications, only
  // workers, so it is ok that the rest of the destruction of the application
  // object happens outside the mutex (assuming we don't support mutiple
  // user threads dealing with application objects).
  deleteChildren();
  releaseFromParent();
}

OC::Worker &
Application::
createWorker(OC::Artifact *art, const char *appInstName,
	     ezxml_t impl, ezxml_t inst, OC::Worker *slave, bool hasMaster,
	     const OCPI::Util::PValue *wParams)
{
  OU::SelfAutoMutex guard (&container());
  if ( appInstName ) 
    return *new Worker(*this, art ? static_cast<Artifact*>(art) : NULL, appInstName, impl, inst, slave, hasMaster, wParams);
  else 
    return *new Worker(*this, art ? static_cast<Artifact*>(art) : NULL, "unnamed-worker", impl, inst, slave, hasMaster, wParams);
}

void Application::
run(DataTransfer::EventManager* event_manager, bool &more_to_do) {
  for (Worker *w = firstChild(); w; w = w->nextChild()) {
    // Give our transport some time
    parent().getTransport().dispatch( event_manager );
    w->run(more_to_do);
  }
}

  }
}
