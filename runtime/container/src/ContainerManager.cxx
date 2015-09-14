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
#include "ContainerManager.h"
#include "ContainerLauncher.h"
#include "ContainerPort.h"          // just for linkage hooks
#include "DtSharedMemoryInternal.h" // just for linkage hooks
#include "OcpiUuid.h"               // just for linkage hooks
#include "DtMsgDriver.h"            // just for linkage hooks
#include "OcpiOsSocket.h"           // just for linkage hooks
#include "lzma.h"                   // just for linkage hooks
namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace OD = OCPI::Driver;
    namespace OU = OCPI::Util;
    namespace OT = OCPI::DataTransport;
    const char *container = "container";

    unsigned Manager::s_nContainers = 0;
    Container **Manager::s_containers;
    unsigned Manager::s_maxContainer;
    LocalLauncher *Manager::s_localLauncher;
    static OCPI::Driver::Registration<Manager> cm;
    Manager::Manager() : m_tpg_events(NULL), m_tpg_no_events(NULL) {
    }

    Manager::~Manager() {
      // Delete my children before the transportGlobals they depend on.
      delete s_localLauncher;
      deleteChildren();
      if ( m_tpg_no_events ) delete m_tpg_no_events;
      if ( m_tpg_events ) delete m_tpg_events;
      delete [] s_containers;
    }

    // Note this is not dependent on configuration.
    // It is currently used in lieu of a generic data transport shutdowm.
    OCPI::DataTransport::TransportGlobal &Manager::
    getTransportGlobalInternal(const OU::PValue *params) {
      static unsigned event_range_start = 0;
      bool polled = true;
      OU::findBool(params, "polled", polled);
      OT::TransportGlobal **tpg = polled ? &m_tpg_no_events : &m_tpg_events;
      if (!*tpg)
	*tpg = new OT::TransportGlobal( event_range_start++, !polled );
      return **tpg;
    }

#if 0
    // The manager of all container drivers gets the "containers" element
    void Manager::configure(ezxml_t x, bool debug) {
      // So by this time all drivers will be loaded and registered.
      // Find elements that match the container types.
      for (ezxml_t dx = x->child; dx; dx = dx->sibling) {
	for (DriverBase *d = firstChild(); d; d = d->nextChild())
	  if (!strcasecmp(d->name(), dx->name))
	    break;
	if (d)
	  d->configure(dx);
	else
	  OD::ManagerManager::
	    configError(x, "element '%s' doesn't match any loaded container driver");
      }
    }
#endif
    // Make sure we cleanup first since we are "on top"
    unsigned Manager::cleanupPosition() { return 0; }
    // FIXME: allow the caller to get errors. Perhaps another overloaded version
    OCPI::API::Container *Manager::find(const char *model, const char *which,
					const OA::PValue *params) {
      parent().configureOnce();
      for (Driver *d = firstChild(); d; d = d->nextChild()) {
	if (!strcmp(model, d->name().c_str())) {
	  OA::Container *c = d->findContainer(which);
	  std::string error;
	  return c ? c : d->probeContainer(which, error, params);
	}
      }
      return NULL;
    }
    Container *Manager::
    findX(const char *which) {
      parent().configureOnce();
      for (Driver *d = firstChild(); d; d = d->nextChild()) {
	Container *c = d->findContainer(which);
	if (c)
	  return c;
      }
      return NULL;
    }
    void Manager::shutdown() {
      deleteChildren();
    }
    bool Manager::findContainersX(Callback &cb, OU::Worker &i, const char *name) {
      parent().configureOnce();
      for (Driver *d = firstChild(); d; d = d->nextChild())
	for (Container *c = d->firstContainer(); c; c = c->nextContainer())
	  if ((!name ||
	       isdigit(*name) && (unsigned)atoi(name) == c->ordinal() ||
	       !isdigit(*name) && name == c->name()) &&
	      c->supportsImplementation(i))
	    cb.foundContainer(*c);
      return false;
    }
    bool Manager::
    dynamic() {
      return OCPI_DYNAMIC;
    }
    Driver::Driver(const char *name) 
      : OD::DriverType<Manager,Driver>(name, *this) {
    }
    const char
      *application = "application",
      *artifact ="artifact",
      *worker = "worker",
      *port = "port",
      *externalPort = "externalPort";
  }
  namespace API {
    Container *ContainerManager::
    find(const char *model, const char *which, const PValue *props) {
      return OCPI::Container::Manager::getSingleton().find(model, which, props);
    }
    void ContainerManager::shutdown() {
      OCPI::Container::Manager::getSingleton().shutdown();
    }
    Container *ContainerManager::
    get(unsigned n) {
      OCPI::Container::Manager::getSingleton().parent().configureOnce();
      return
	n >= OCPI::Container::Manager::s_nContainers ? NULL : 
	&OCPI::Container::Container::nthContainer(n);
    }
  }
  namespace Container {
    // Hooks to ensure that if we are linking statically, everything is pulled in
    // to support drivers and workers.
    void dumb1(BasicPort &p) { p.applyConnectParams(NULL, NULL); }
  }
}
namespace DataTransfer {
  intptr_t dumb2(EndPoint &loc) {
    OCPI::Util::Uuid uuid;
    OCPI::Util::UuidString us;
    OCPI::Util::uuid2string(uuid, us);
    createHostSmemServices(loc);
    Msg::XferFactoryManager::getFactoryManager();
    OCPI::OS::Socket s;
    return (intptr_t)&lzma_stream_buffer_decode;
  }
}

