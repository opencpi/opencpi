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

#include <signal.h>
#include "ocpi-config.h"
#include "OcpiOsMisc.h"
#include "OcpiUtilCppMacros.h"
#include "XferManager.h"
#include "ContainerManager.h"
#include "ContainerLauncher.h"
#include "Container.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OL = OCPI::Library;
namespace OR = OCPI::RDT;
namespace XF = DataTransfer;

namespace OCPI {
  namespace Container {

    Container::Container(const char *a_name, const ezxml_t config,
			 const OCPI::Util::PValue *params)
      throw ( OU::EmbeddedException )
      : //m_ourUID(mkUID()),
      OCPI::Time::Emit("Container", a_name ),
      m_enabled(false), m_ownThread(true), m_verbose(false), m_thread(NULL),
      m_transport(*new OCPI::DataTransport::Transport(&Manager::getTransportGlobal(params), false, this))
    {
      OU::findBool(params, "verbose", m_verbose);
      OU::SelfAutoMutex guard (this);
      m_ordinal = Manager::s_nContainers++;
      if (m_ordinal >= Manager::s_maxContainer) {
	Container **old = Manager::s_containers;
	Manager::s_containers = new Container *[Manager::s_maxContainer + 10];
	if (old) {
	  memcpy(Manager::s_containers, old, Manager::s_maxContainer * sizeof(Container *));
	  delete [] old;
	}
	Manager::s_maxContainer += 10;
      }
      Manager::s_containers[m_ordinal] = this;
      (void)config; // nothing to parse (yet)
      // FIXME:  this should really be in a baseclass inherited by software containers
      // It works because stuff can be overriden and no threads are created until
      // "start", which is
      OU::findBool(params, "ownthread", m_ownThread);
      if (getenv("OCPI_NO_THREADS"))
	m_ownThread = false;
      m_os = OCPI_CPP_STRINGIFY(OCPI_OS) + strlen("OCPI");
      m_osVersion = OCPI_CPP_STRINGIFY(OCPI_OS_VERSION);
      m_platform = OCPI_CPP_STRINGIFY(OCPI_PLATFORM);
      m_arch = OCPI_CPP_STRINGIFY(OCPI_ARCH);
    }

    bool Container::supportsImplementation(OU::Worker &i) {
      static const char *opencpiVersion; // AV-2453
      static bool allowVersionMismatch = false;

      if (!opencpiVersion) {
	opencpiVersion = 
	  OCPI_CPP_STRINGIFY(OCPI_VERSION_MAJOR) "." OCPI_CPP_STRINGIFY(OCPI_VERSION_MINOR);
	ocpiInfo("OpenCPI version is: %s", opencpiVersion);
	const char *env = getenv("OCPI_ALLOW_VERSION_MISMATCH");
	allowVersionMismatch = env && env[0] == '1';
	ocpiInfo("Artifact version checking is %sin effect.%s",
                allowVersionMismatch ? "NOT " : "",
                allowVersionMismatch ? "" : " Set OCPI_ALLOW_VERSION_MISMATCH to 1 to allow.");
      }
      bool ok =
	m_model == i.model() &&
	m_os == i.attributes().os() &&
	m_osVersion == i.attributes().osVersion() &&
	// if all three are present and match, on rcc, platform does not need to match.
	// this eases the transition to software platforms having proper names
	((m_model == "rcc" &&
	  m_os.length() && m_os == i.attributes().os() &&
	  m_osVersion.length() && m_osVersion == i.attributes().osVersion() &&
	  m_arch.length() && m_arch == i.attributes().arch()) ||
	 (i.attributes().platform().length() && m_platform == i.attributes().platform()) ||
	 (i.attributes().platform().empty() && m_arch == i.attributes().arch())) &&
	m_dynamic == i.attributes().dynamic();
      // Checking OpenCPI version is not part of the "ok" above because we can override with
      // the environment and want to warn.
      if (ok && !allowVersionMismatch &&
	  !(ok = opencpiVersion == i.attributes().opencpiVersion()))
	ocpiBad("Rejected '%s' for platform %s ONLY because of artifact version mismatch "
		"('%s' vs. expected '%s')%s",
		i.cname(), i.attributes().platform().c_str(),
		i.attributes().opencpiVersion().c_str(), opencpiVersion,
		OCPI::OS::logWillLog(OCPI_LOG_INFO) ? "" : " (try increasing log level)");
      ocpiInfo("vs. container %s (%u) model %s os %s version %s arch %s platform %s dynamic %u "
	       "opencpi version %s ==> %s",
	       name().c_str(), m_ordinal, m_model.c_str(), m_os.c_str(), m_osVersion.c_str(),
	       m_arch.c_str(), m_platform.c_str(), m_dynamic, opencpiVersion,
	       ok ? "accepted" : "rejected");
      return ok;
    }

    Artifact & Container::
    loadArtifact(const char *url, const OA::PValue *artifactParams) {
      // First check if it is loaded on in this container
      // FIXME: canonicalize the URL here?
      Artifact *art = findLoadedArtifact(url);

      if (art)
	return *art;
      // If it is not loaded, let's get it from the library system,
      // and load it ourselves.
      return createArtifact(OL::Manager::getArtifact(url, artifactParams), artifactParams);
    }
    Artifact & Container::
    loadArtifact(OL::Artifact &libArt, const OA::PValue *artifactParams) {
      // First check if it is loaded on in this container
      // FIXME: canonicalize the URL here?
      Artifact *art = findLoadedArtifact(libArt);

      if (art)
	return *art;
      // If it is not loaded, let's get it from the library system,
      // and load it ourselves.
      return createArtifact(libArt, artifactParams);
    }

    // Ultimately there would be a set of "base class" generic properties
    // and the derived class would merge them.
    // FIXME: define base class properties for all apps
    OCPI::Util::PValue *Container::getProperties() {
      return 0;
    }
    OCPI::Util::PValue *Container::getProperty(const char *) {
      return 0;
    }
    // This is for the derived class's destructor to call
    void Container::shutdown() {
      stop();
      if (m_thread)
	m_thread->join();
    }
    Container::~Container() {
      m_enabled = false;
      if (m_thread) {
	m_thread->join();
	delete m_thread;
      }
      Manager::s_containers[m_ordinal] = 0;
      delete &m_transport;
    }

#if 0
    //    bool m_start;

    void Container::start(DataTransfer::EventManager* event_manager)
      throw()
    {
      (void)event_manager;
      start();
      m_enabled = true;
    }

    void Container::stop(DataTransfer::EventManager* event_manager)
      throw()
    {
      (void)event_manager;
      m_enabled = false;
    }
#endif
    Container::DispatchRetCode Container::dispatch(DataTransfer::EventManager*)
    {
      return Container::DispatchNoMore;
    }
    bool Container::run(uint32_t usecs) {
      if (m_ownThread)
	throw OU::EmbeddedException( OU::CONTAINER_HAS_OWN_THREAD,
				     "Can't use container->run when container has own thread",
				     OU::ApplicationRecoverable);
      return runInternal(usecs);
    }

    bool Container::runInternal(uint32_t usecs) {
      if (!m_enabled)
	return false;
      //OS::sleep(0);
      {
	OU::SelfAutoMutex guard(this);
	for (BridgedPortsIter bpi = m_bridgedPorts.begin(); bpi != m_bridgedPorts.end(); bpi++)
	  (*bpi)->runBridge();
      }
      DataTransfer::EventManager *em = getEventManager();
      switch (dispatch(em)) {
      case DispatchNoMore:
	// All done, exit from dispatch thread.
	return false;

      case MoreWorkNeeded:
	// No-op. To prevent blocking the CPU, yield.
	OCPI::OS::sleep (0);
	return true;

      case Stopped:
	// Exit from dispatch thread, it will be restarted.
	return false;

      case Spin:
	/*
	 * If we have an event manager, ask it to go to sleep and wait for
	 * an event.  If we are not event driven, the event manager will
	 * tell us that it is spinning.  In that case, yield to give other
	 * threads a chance to run.
	 */
	if (em &&
	    em->waitForEvent(usecs) == DataTransfer::EventTimeout && m_verbose)
	  ocpiBad("Timeout after %u usecs waiting for event", usecs);
	OCPI::OS::sleep (0);
      }
      return true;
    }
    // This will be called inside a separate thread for this container.
    void Container::thread() {
      while (m_enabled && runInternal())
	;
    }
    void Container::stop() {
      //      stop(getEventManager());
      m_enabled = false;
    }
    void runContainer(void*arg) {

      // Disable signals on these background threads.
      // Any non-exception/non-inline signal handling should be in the main program.
      // (signals originating outside the program)
      // FIXME: we need a good theory here about how signals in container threads
      // should be handled and/or controlled.  For now we just know that container
      // threads don't know what the control app wants from signals so it stays away
      // FIXME: use OcpiOs for this
      sigset_t set;
      sigemptyset(&set);
      static int sigs[] = {SIGHUP, SIGINT, SIGQUIT, SIGKILL, SIGTERM, SIGSTOP, SIGTSTP,
			   SIGCHLD, 0};
      for (int *sp = sigs; *sp; sp++)
	sigaddset(&set, *sp);
      ocpiCheck(pthread_sigmask(SIG_BLOCK, &set, NULL) == 0);
      try {
	((Container *)arg)->thread();
      } catch (const std::string &s) {
	std::cerr << "Container \"" << ((Container *)arg)->name()
		  << "\" background thread exception:  " << s << std::endl;
	abort();
      } catch (...) {
	std::cerr << "Container background thread unknown exception" << std::endl;
	throw;
      }
    }
    void Container::start() {
      Container &base = baseContainer();
      if (this != &base && base.m_bridgedPorts.size())
	base.start();
      if (!m_enabled) {
	if (this != &base && base.m_bridgedPorts.size())
	  base.start();
	m_enabled = true;
	ocpiDebug("Starting container %s(%u): %p", name().c_str(), m_ordinal, this);
	if (!m_thread && m_ownThread && needThread()) {
	  m_thread = new OCPI::OS::ThreadManager;
	  m_thread->start(runContainer, (void*)this);
	}
	//	start(getEventManager());
      }
    }
    Container &Container::nthContainer(unsigned n) {
      if (n >= Manager::s_maxContainer)
	throw OU::Error("Invalid container %u", n);
      if (!Manager::s_containers[n])
	throw OU::Error("Missing container %u", n);
      return *Manager::s_containers[n];
    }
    Container &Container::baseContainer() {
      for (unsigned i = 0; i <= Manager::s_maxContainer; i++) {
	Container &c = Container::nthContainer(i);
	if (!strncmp("rcc", c.name().c_str(), 3))
	  return c;
      }
      throw OU::Error("No RCC container found when looking for the base container");
    }
    void Container::registerBridgedPort(LocalPort &p) {
      OU::SelfAutoMutex guard (this);
      ocpiDebug("BridgePort %p of container %p registered with container %p",
		&p, &p.container(), this);
      m_bridgedPorts.insert(&p);
    }
    void Container::unregisterBridgedPort(LocalPort &p) {
      OU::SelfAutoMutex guard (this);
      ocpiDebug("BridgePort %p of container %p unregistered from container %p",
		&p, &p.container(), this);
      m_bridgedPorts.erase(&p);
    }
    Launcher &Container::launcher() const {
      return LocalLauncher::getSingleton();
    }
    void Container::
    addTransport(const char *a_name, const char *id, OR::PortRole roleIn,  OR::PortRole roleOut,
		 uint32_t inOptions, uint32_t outOptions) {
      if (XF::getManager().find(a_name)) {
	Transport t;
	t.transport = a_name;
	t.id = id ? id : "";
	t.roleIn = roleIn;
	t.roleOut = roleOut;
	t.optionsIn = inOptions;
	t.optionsOut = outOptions;
	m_transports.push_back(t);
	ocpiLog(9, "Adding transport %s(%s) to container %s",
		a_name, t.id.c_str(), name().c_str());
      } else
	ocpiInfo("Transport %s not supported in this process.  Not loaded/spec'd in system.xml?",
		 a_name);
    }
  }
  namespace API {
    Container::~Container(){}
  }
}
