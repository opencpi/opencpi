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

#include "lzma.h"                   // just for linkage hooks
#include "zlib.h"                   // just for linkage hooks
#include "pthread_workqueue.h"      // just for linkage hooks
#include "ocpi-config.h"
#include "OcpiOsSocket.h"           // just for linkage hooks
#include "OcpiOsServerSocket.h"     // just for linkage hooks
#include "OcpiOsSemaphore.h"        // just for linkage hooks
#include "OcpiUuid.h"               // just for linkage hooks
#include "OcpiThread.h"             // just for linkage hooks
#include "OcpiUtilPci.h"            // just for linkage hooks
#include "ContainerPort.h"          // just for linkage hooks
#include "OcpiContainerRunConditionApi.h"
#include "XferAccess.h"
#include "XferManager.h"

#include "ContainerManager.h"
#include "ContainerLauncher.h"
namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace OD = OCPI::Driver;
    namespace OU = OCPI::Util;
    namespace OT = OCPI::DataTransport;
    const char *container = "container";

    unsigned Manager::s_nContainers = 0;
    // TODO: Move this to a vector to manage its own memory...?
    Container **Manager::s_containers;
    unsigned Manager::s_maxContainer;
    static OCPI::Driver::Registration<Manager> cm;
    Manager::Manager() : m_tpg_events(NULL), m_tpg_no_events(NULL) {
    }

    Manager::~Manager() {
      // Delete my children before the transportGlobals they depend on.
      delete LocalLauncher::singleton();
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
    bool Manager::findContainersX(Callback &cb, OU::Worker &i, const char *a_name) {
      ocpiDebug("Finding containers for worker %s container name %s",
		i.cname(), a_name ? a_name : "<none specified>");
      parent().configureOnce();
      for (Driver *d = firstChild(); d; d = d->nextChild())
	for (Container *c = d->firstContainer(); c; c = c->nextContainer()) {
	  ocpiDebug("Trying container c->name: %s ord %u",
		    c->name().c_str(), c->ordinal());
	  bool decimal = a_name && a_name[strspn(a_name, "0123456789")] == '\0';
	  if ((!a_name ||
	       (decimal && (unsigned)atoi(a_name) == c->ordinal()) ||
	       (!decimal && a_name == c->name())) &&
	      c->supportsImplementation(i))
	    cb.foundContainer(*c);
	}
      return false;
    }
    bool Manager::
    dynamic() {
      return OCPI_DYNAMIC;
    }
    void Manager::
    cleanForContextX(void *context) {
      for (Driver *d = firstChild(); d; d = d->nextChild())
	for (Container *c = d->firstContainer(); c; c = c->nextContainer())
	  c->getTransport().cleanForContext(context);
      DataTransfer::XferManager::getFactoryManager().cleanForContext(context);
    }
    Driver::Driver(const char *a_name) 
      : OD::DriverType<Manager,Driver>(a_name, *this) {
    }
    const char
      *application = "application",
      *artifact ="artifact",
      *worker = "worker",
      *portBase = "port", // named differently to avoid shadowing issues
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
      ocpiDebug("ContainerManager::get(): Calling configureOnce");
      OCPI::Container::Manager::getSingleton().parent().configureOnce();
      ocpiDebug("ContainerManager::get(): Back with %d containers", OCPI::Container::Manager::s_nContainers);
      return
	n >= OCPI::Container::Manager::s_nContainers ? NULL : 
	&OCPI::Container::Container::nthContainer(n);
    }
    // List the containers available
    void ContainerManager::
    list(bool onlyPlatforms) {
      Container *c;
      if (onlyPlatforms) {
	std::set<std::string> plats;
	for (unsigned n = 0; (c = ContainerManager::get(n)); n++)
	  plats.insert(c->model() + "-" + (c->dynamic() ? "1" : "0") + "-" + c->platform());
	for (std::set<std::string>::const_iterator i = plats.begin(); i != plats.end(); ++i)
	  printf("%s\n", i->c_str());
      } else {
	printf("Available containers:\n"
	       " #  Model Platform       OS     OS-Version  Arch     Name\n");
	for (unsigned n = 0; (c = ContainerManager::get(n)); n++)
	  printf("%2u  %-5s %-14s %-6s %-11s %-8s %s\n",
		 n,  c->model().c_str(), c->platform().c_str(), c->os().c_str(),
		 c->osVersion().c_str(), c->arch().c_str(), c->name().c_str());
      }
      fflush(stdout);
    }



  }
  /*
   * This ensures the following functions are linked into the final ocpirun/ACI executables when
   * the functions are used by driver plugin(s) (which are dynamically loaded, but linked against
   * dynamic libraries that do not exist at runtime, e.g. uuid.so) but nowhere else in the
   * framework infrastructure, forcing them to be statically linked here:
   */
  namespace Container {
    intptr_t linkme() {
      ((DataTransfer::Access *)linkme)->closeAccess();
      ((OA::RunCondition *)linkme)->setPortMasks((OA::OcpiPortMask *)NULL);
      ((Container*)linkme)->run();
      ((DataTransfer::XferServices*)linkme)->DataTransfer::XferServices::send(0, NULL, 0);
      ((OCPI::Util::Thread*)linkme)->join();
      OCPI::Util::Uuid uuid;
      OCPI::Util::UuidString us;
      OCPI::Util::uuid2string(uuid, us);
      std::string str;
      OCPI::Util::searchPath(NULL, NULL, str, NULL, NULL);
      (void)OCPI::Util::getCDK();
      size_t dum2;
      (void)((BasicPort*)linkme)->BasicPort::getOperationInfo(0, dum2);
      unsigned dum3;
      (void)OCPI::Util::probePci(NULL, 0, 0, 0, 0, 0, NULL, dum3, str);
      // Msg::XferFactoryManager::getFactoryManager();
      OCPI::OS::Socket s;
      OCPI::OS::ServerSocket ss;
      OCPI::OS::Semaphore sem;
      gzerror(NULL, (int*)0);
      // p.applyConnectParams(NULL, NULL);
      ((OCPI::Container::Application*)0)->createWorker(NULL, NULL, NULL, NULL, NULL, NULL);
      pthread_workqueue_create_np(NULL, NULL);
      return (intptr_t)&lzma_stream_buffer_decode;
    }
  }
}

