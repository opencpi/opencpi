
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

#include <string>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <signal.h>
#include <OcpiOsAssert.h>
#include <OcpiOsMisc.h>
#include "OcpiUtilCppMacros.h"
#include <OcpiUtilCDR.h>
#include <OcpiRDTInterface.h>
#include <OcpiPortMetaData.h>
#include <OcpiLibraryManager.h>
#include <OcpiContainerInterface.h>
#include <OcpiContainerPort.h>
#include <OcpiContainerApplication.h>
#include <OcpiContainerArtifact.h>
#include "OcpiContainerErrorCodes.h"



namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OL = OCPI::Library;

using namespace OCPI::RDT;


namespace OCPI {
  namespace Container {

    std::string
    packDescriptor (OCPI::Util::CDR::Encoder& packer, const Descriptors & desc)
      throw ()
    {
      const OutOfBandData * oob=NULL;
      packer.putULong (desc.type);
      packer.putULong (desc.role);
      packer.putULong (desc.options);

      switch (desc.type) {
      case ConsumerDescT:
      case ConsumerFlowControlDescT:
      case ProducerDescT:
      default:
        {
          const Desc_t & d = desc.desc;
          oob = &d.oob;
          packer.putULong     (d.nBuffers);
          packer.putULongLong (d.dataBufferBaseAddr);
          packer.putULong     (d.dataBufferPitch);
          packer.putULong     (d.dataBufferSize);
          packer.putULongLong (d.metaDataBaseAddr);
          packer.putULong     (d.metaDataPitch);
          packer.putULongLong (d.fullFlagBaseAddr);
          packer.putULong     (d.fullFlagSize);
          packer.putULong     (d.fullFlagPitch);
          packer.putULongLong (d.fullFlagValue);
          packer.putULongLong (d.emptyFlagBaseAddr);
          packer.putULong     (d.emptyFlagSize);
          packer.putULong     (d.emptyFlagPitch);
          packer.putULongLong (d.emptyFlagValue);
        }
        break;
      }

      /*
       * OutOfBandData is the same for consumer and producer.
       */
      if ( oob ) { 
        packer.putULongLong (oob->port_id);
        packer.putString (oob->oep);
        packer.putULongLong (oob->cookie);
      }
      else {
        return false;
      }
      /*
       * Return marshaled data.
       */

      return packer.data ();
    }

    bool
    unpackDescriptor ( OCPI::Util::CDR::Decoder& unpacker,
                       Descriptors & desc)
      throw ()
    {
      OutOfBandData * oob=NULL;

      try {
        unpacker.getULong (desc.type);
        unpacker.getLong (desc.role);
        unpacker.getULong (desc.options);

        switch (desc.type) {
        case ConsumerDescT:
        case ConsumerFlowControlDescT:
        case ProducerDescT:
        default:
          {
            Desc_t & d = desc.desc;
            oob = &d.oob;
            unpacker.getULong     (d.nBuffers);
            unpacker.getULongLong (d.dataBufferBaseAddr);
            unpacker.getULong     (d.dataBufferPitch);
            unpacker.getULong     (d.dataBufferSize);
            unpacker.getULongLong (d.metaDataBaseAddr);
            unpacker.getULong     (d.metaDataPitch);
            unpacker.getULongLong (d.fullFlagBaseAddr);
            unpacker.getULong     (d.fullFlagSize);
            unpacker.getULong     (d.fullFlagPitch);
            unpacker.getULongLong (d.fullFlagValue);
            unpacker.getULongLong (d.emptyFlagBaseAddr);
            unpacker.getULong     (d.emptyFlagSize);
            unpacker.getULong     (d.emptyFlagPitch);
            unpacker.getULongLong (d.emptyFlagValue);
          }
          break;
        }

        /*
         * OutOfBandData is the same for consumer and producer.
         */

        std::string oep;
        if ( oob ) {
          unpacker.getULongLong (oob->port_id);
          unpacker.getString (oep);
          unpacker.getULongLong (oob->cookie);
        }
        else {
          return false;
        }

        if (oep.length()+1 > 128) {
          return false;
        }

        std::strncpy (oob->oep, oep.c_str(), 128);
      }
      catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
        return false;
      }

      return true;
    }

    static uint32_t mkUID() {
      static uint32_t id = 1;
      return id++ + getpid();
    }

    Container::Container(const OCPI::Util::PValue*props)
      throw ( OCPI::Util::EmbeddedException )
      : m_ourUID(mkUID()), m_enabled(false), m_ownThread(true), m_thread(NULL)
    {
      OU::findBool(props, "ownthread", m_ownThread);
      if (getenv("OCPI_NO_THREADS"))
	m_ownThread = false;
      m_os = OCPI_CPP_STRINGIFY(OCPI_OS) + strlen("OCPI");
      m_osVersion = OCPI_CPP_STRINGIFY(OCPI_OS_VERSION);
      m_platform = OCPI_CPP_STRINGIFY(OCPI_PLATFORM);
#if 0
      m_runtime = 0;
      m_runtimeVersion = 0;
#endif
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
      return createArtifact(OL::Manager::getArtifact(url, artifactParams),
			    artifactParams);
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
      stop(NULL);
      if (m_thread)
	m_thread->join();
    }
    Container::~Container() {
      if (m_thread)
	delete m_thread;
    }

    bool m_start;



    /*
     * ----------------------------------------------------------------------
     * A simple test.
     * ----------------------------------------------------------------------
     */
    /*
      static int
      pack_unpack_test (int argc, char *argv[])
      {
      OCPI::RDT::Descriptors d;
      std::string data;
      bool good;

      std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
      d.mode = OCPI::RDT::ConsumerDescType;
      d.desc.c.fullFlagValue = 42;
      std::strcpy (d.desc.c.oob.oep, "Hello World");
      data = packDescriptor (d);
      std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
      good = unpackDescriptor (data, d);
      ocpiAssert (good);
      ocpiAssert (d.mode == OCPI::RDT::ConsumerDescType);
      ocpiAssert (d.desc.c.fullFlagValue == 42);
      ocpiAssert (std::strcmp (d.desc.c.oob.oep, "Hello World") == 0);

      std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
      d.mode = OCPI::RDT::ProducerDescType;
      d.desc.p.emptyFlagValue = 42;
      std::strcpy (d.desc.p.oob.oep, "Hello World");
      data = packDescriptor (d);
      std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
      good = unpackDescriptor (data, d);
      ocpiAssert (good);
      ocpiAssert (d.mode == OCPI::RDT::ProducerDescType);
      ocpiAssert (d.desc.p.emptyFlagValue == 42);
      ocpiAssert (std::strcmp (d.desc.p.oob.oep, "Hello World") == 0);

      data[0] = ((data[0] == '\0') ? '\1' : '\0'); // Hack: flip byteorder
      good = unpackDescriptor (data, d);
      ocpiAssert (!good);

      return 0;
      }
    */


    std::string Container::packPortDesc(  PortData & port  )
      throw()
    {

      std::string s;
      OCPI::Util::CDR::Encoder packer;
      packer.putBoolean (OCPI::Util::CDR::nativeByteorder());
      packer.putULong (port.connectionData.container_id);
      packer.putULongLong( port.connectionData.port );
  
      packDescriptor( packer, port.connectionData.data );
      return packer.data();
    }


    int Container::portDescSize(){return sizeof(PortData);}

    PortData * Container::unpackPortDesc( const std::string& data, PortData* port )
      throw ()
    {
      OCPI::Util::CDR::Decoder unpacker (data);
      Descriptors *desc = &port->connectionData.data;
      bool bo;

      try { 
	unpacker.getBoolean (bo);
	unpacker.byteorder (bo);
	unpacker.getULong (port->connectionData.container_id);
	unpacker.getULongLong ((uint64_t&)port->connectionData.port);
	bool good = unpackDescriptor ( unpacker, *desc);
	if ( ! good ) {
	  return 0;
	}

      }
      catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
	return false;
      }

      return port;
    }



    void Container::start(DataTransfer::EventManager* event_manager)
      throw()
    {
      (void)event_manager;
      m_enabled = true;
    }

    void Container::stop(DataTransfer::EventManager* event_manager)
      throw()
    {
      (void)event_manager;
      m_enabled = false;
    }

    std::vector<std::string> 
    Container::
    getSupportedEndpoints()
      throw ()
    {
      std::vector<std::string> l;
      return l;
    }

    Container::DispatchRetCode Container::dispatch(DataTransfer::EventManager*)
      throw ( OCPI::Util::EmbeddedException )
    {
      return Container::DispatchNoMore;
    }
    bool Container::run(uint32_t usecs, bool verbose) {
      if (m_ownThread)
	throw OU::EmbeddedException( CONTAINER_HAS_OWN_THREAD,
				     "Can't use container->run when container has own thread",
				     ApplicationRecoverable);
      return runInternal();
    }

    bool Container::runInternal(uint32_t usecs, bool verbose) {
      if (!m_enabled)
	return false;
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
	break;

      case Spin:
	/*
	 * If we have an event manager, ask it to go to sleep and wait for
	 * an event.  If we are not event driven, the event manager will
	 * tell us that it is spinning.  In that case, yield to give other
	 * threads a chance to run.
	 */
	if (em &&
	    em->waitForEvent(usecs) == DataTransfer::EventTimeout && verbose)
	  printf("Timeout after %u usecs waiting for event\n", usecs);
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
      stop(getEventManager());
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
      ((Container *)arg)->thread();
    }
    void Container::start() {
      if (!m_enabled) {
	m_enabled = true;
	if (!m_thread && m_ownThread && needThread()) {
	  m_thread = new OCPI::OS::ThreadManager;
	  m_thread->start(runContainer, (void*)this);
	}
	start(getEventManager());
      }
    }
  }
  namespace API {
    Container::~Container(){}
  }
}
