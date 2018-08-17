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

#ifndef RCC_APPLICATION_H__
#define RCC_APPLICATION_H__

#ifndef WORKER_INTERNAL
#define WORKER_INTERNAL
#endif
#include "RCC_Worker.h"

#include "OcpiOsLoadableModule.h"

#include "ContainerManager.h"

namespace OCPI {

  namespace DataTransport {
    class Transport;
    class Circuit;
  }

    namespace RCC {

      class Worker;
      class Container;

      class Artifact : public OCPI::Container::ArtifactBase<Container,Artifact> {
	friend class Container;

	Artifact(Container &c, OCPI::Library::Artifact &lart, const OCPI::API::PValue *artifactParams);

      public:
	virtual ~Artifact();
	OCPI::Container::Worker &
	  createWorkerX(OCPI::Container::Application &a, const char *name, ezxml_t impl,
			ezxml_t inst, const OCPI::API::PValue *execProps);
	RCCEntryTable *getDispatch(const char *name);
      private:
	
        OCPI::OS::LoadableModule m_loader;
	RCCEntryTable           *m_entryTable;
	bool                     m_open;
        int                      m_workerCount;

      };


      /**********************************
       * Containers represented Node Application class
       *********************************/
      class Application
	: public OCPI::Container::ApplicationBase<Container,Application,Worker>
      {
        friend class Container;
        friend class Controller;
      protected:
	void run(DataTransfer::EventManager* event_manager, bool &more_to_do);
      public:
	OCPI::Container::Worker &
	createWorker(OCPI::Container::Artifact *art, const char *appInstName, ezxml_t impl,
		     ezxml_t inst, const OCPI::Container::Workers &slaves, bool hasMaster,
		     size_t member, size_t crewSize, const OCPI::Util::PValue *wParams);

      /**********************************
       * Constructor
       *********************************/  
        Application(Container &, const char *, const OCPI::Util::PValue *);

      /**********************************
       * Destructor
       *********************************/  
        virtual ~Application();

      private:
	RCCContainer *m_rccContainer;
    };

    }
}

#endif

