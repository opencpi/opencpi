
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



/**
   @file

   @brief
   The OCPI::Container::Container class as the base class for Container implementations.

   Revision History:

   06/15/09 - John Miller
   Added getLastControlError to interface.

   10/13/08 - John Miller
   Initial version.

************************************************************************** */

#ifndef OCPI_CONTAINER_INTERFACE_H_
#define OCPI_CONTAINER_INTERFACE_H_

#include <vector>
#include <OcpiOsDataTypes.h>
#include <OcpiParentChild.h>
#include <OcpiOsThreadManager.h>
#include "OcpiUtilSelfMutex.h"
#include "OcpiTransport.h"
#include <DtIntEventHandler.h>
#include <OcpiContainerDataTypes.h>
#include <OcpiContainerApi.h>
#include <OcpiLibraryManager.h>

namespace OCPI {

  namespace Container {

    class Driver;
    class PortData;
    class Application;
    class Artifact;

    /**
       @class Container

       @brief
       Container base class.

       This class provides the interface definition for container implementations.

       @sa OCPI::Container::Container

       ********************************************************************** */
    class Artifact; // the class representing a loaded artifact on the container
    class Container
      : public OCPI::API::Container,
	public OCPI::Library::Capabilities,
	public OCPI::Time::Emit,
	virtual public OCPI::Util::SelfMutex
    {

      bool runInternal(uint32_t usecs = 0, bool verbose = false);
    public:
      virtual Driver &driver() = 0;

      //!< Dispatch thread return codes
      enum DispatchRetCode {
        MoreWorkNeeded,   // Dispatch returned, but there is more work needed
        Spin,             // Dispatch completed it current tasks
        DispatchNoMore,   // No more dispatching required
        Stopped           // Container is stopped
      };

      /**
         @brief
         Constructor

         @param [ in ] properties
         Container startup properties

         @throw  OCPI::Util::EmbeddedException  If an error is detected during construction

         ****************************************************************** */
      Container(const char *name, const ezxml_t config = NULL, const OCPI::Util::PValue* props = NULL)
        throw ( OCPI::Util::EmbeddedException );

      ~Container();

      virtual Container *nextContainer() = 0;
      bool supportsImplementation(OCPI::Util::Implementation &);
      /**
         @brief
         createApplication

         Creates an application  for all subsequent method calls.

         @throw OCPI::Util::EmbeddedException  If an error is detected during the creation of the .

         ****************************************************************** */        
      virtual OCPI::API::ContainerApplication *
	createApplication(const char *name = NULL, const OCPI::Util::PValue *props = NULL)
        throw ( OCPI::Util::EmbeddedException ) = 0;

      OCPI::Util::PValue *getProperties();
      OCPI::Util::PValue *getProperty(const char *);

      /**
         @brief
         dispatch

         This is the method that gets called by the creator to provide thread time to the container.  If this method
         returns "true" the caller must continue to call this method.  If the return is "false" the method no longer needs
         to be called.

         @param [ in ] event_manager
         Event Manager object that is associated with this container.  This parameter can be NULL if the container is
         being used in polled mode.

         @throw OCPI::Util::EmbeddedException  If an error is detected during dispatch

         ****************************************************************** */        
      virtual DispatchRetCode dispatch(DataTransfer::EventManager*);
      bool run(uint32_t usecs = 0, bool verbose = false);
      void thread();
      virtual bool needThread() = 0;

      // Load from url
      Artifact & loadArtifact(const char *url,
			      const OCPI::Util::PValue *artifactParams = NULL);
      // Load from library artifact
      Artifact & loadArtifact(OCPI::Library::Artifact &art,
			      const OCPI::Util::PValue *artifactParams = NULL);
      
      virtual Artifact *findLoadedArtifact(const char *url) = 0;
      virtual Artifact *findLoadedArtifact(const OCPI::Library::Artifact &a) = 0;
      virtual Artifact &createArtifact(OCPI::Library::Artifact &,
				       const OCPI::API::PValue *props = NULL) = 0;
     
      //!< Start/Stop the container
      virtual void start(DataTransfer::EventManager* event_manager)
        throw();
      virtual void stop(DataTransfer::EventManager* event_manager)
        throw();

      virtual void stop();
      // FIXME: default start behavior is for software containers.
      virtual void start();
      //! get the event manager for this container
      virtual DataTransfer::EventManager*  getEventManager(){return NULL;}

      bool hasName(const char *name);

      //      inline OCPI::OS::uint32_t getId(){return m_ourUID;}
      inline unsigned ordinal() const { return m_ordinal; }
      static Container &nthContainer(unsigned n);
      typedef uint32_t CMap;
      static const unsigned maxContainer = sizeof(CMap) * 8;
      static unsigned s_nContainers;
      // Server - if a container is remote, it has a server.
      class Server;
      Server *m_server;
      inline Server *server() const { return m_server; }
      inline OCPI::DataTransport::Transport &getTransport() { return m_transport; }
    protected:
      void shutdown();
      //      const std::string m_name;

      //! This containers unique id
      //      OCPI::OS::uint32_t m_ourUID;
      unsigned m_ordinal;

      // Start/Stop flag for this container
      bool m_enabled;
      bool m_ownThread;
      OCPI::OS::ThreadManager *m_thread;
      // This is not an embedded member to potentially control lifecycle better...
      OCPI::DataTransport::Transport &m_transport;
    private:
      static Container **s_containers;
      static unsigned s_maxContainer;
    };
  }
}

#endif

