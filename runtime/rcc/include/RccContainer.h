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
 *   This file contains the interface for the OCPI Node Manager Container.  
 *
 * Revision History: 
 *
 *  06/14/09 - John Miller
 *  Added support to get last worker control error.
 * 
 *  03/01/05 - John Miller
 *  Initial Version
 *
 *************************************************************************/

#ifndef RCC_CONTAINER_H_
#define RCC_CONTAINER_H_

#include "pthread_workqueue.h"
#include "OcpiOsSemaphore.h"
#include "RccApplication.h"
#include "RccDriver.h"

namespace OCPI {

  namespace Container {
    class PortData;
  }

  namespace RCC {
    class Port;
    class Component;     
    class RCCWorkerInterface;
    class Worker;
    class RCCUserTask;
    struct RCCPortData;

    // Our Custom exception definitions
    const uint32_t CP289_EX_SOURCE_ID          = 10;
    const uint32_t BAD_ARTIFACT_URL            = (CP289_EX_SOURCE_ID << 16) + 1;
    const uint32_t REQUIRED_PROPERTY_NOT_SET   = (CP289_EX_SOURCE_ID << 16) + 2;
    const uint32_t WORKER_INIT_FAILED          = (CP289_EX_SOURCE_ID << 16) + 3;
    const uint32_t WORKER_ENABLE_FAILED          = (CP289_EX_SOURCE_ID << 16) + 4;
    const uint32_t WORKER_DISABLE_FAILED          = (CP289_EX_SOURCE_ID << 16) + 5;
    const uint32_t CP289_CSINTERNAL_ERROR          = (CP289_EX_SOURCE_ID << 16) + 6;

    class Container
      : public OCPI::Container::ContainerBase<Driver,Container,Application,Artifact> {
    private:
      // FIXME:  someday this thread pool will be private to the container
      static bool m_wqInit;
      static const int WORKQUEUE_COUNT = 2;
      static const int LOW_PRI_Q = 0;
      static const int HIGH_PRI_Q = 1;
      static pthread_workqueue_t m_workqueues[WORKQUEUE_COUNT]; 

    public:
      friend class Port;
      friend class RDMAPort;
      friend class Worker;
      friend class Application;
      friend class PortDelegator;

      Container(const char *name, const OCPI::API::PValue* params)
	throw (OCPI::Util::EmbeddedException);
      virtual ~Container()
	throw ();
      void initWorkQueues();
      bool portsInProcess() { return true; }
      OCPI::Container::Container::DispatchRetCode
      dispatch(DataTransfer::EventManager *event_manager = NULL);
      OCPI::API::ContainerApplication*
      createApplication(const char *name, const OCPI::Util::PValue *params)
	throw (OCPI::Util::EmbeddedException);
      OCPI::Container::Artifact &
      createArtifact(OCPI::Library::Artifact &lart, const OCPI::API::PValue *artifactParams);

      // worker task management
      void addTask( void (*workitem_func)(void *), void * args );
      void addTask( RCCUserTask * task );
      bool join( bool block, OCPI::OS::Semaphore & sem );

      void start(DataTransfer::EventManager* event_manager) throw();
      void stop(DataTransfer::EventManager* event_manager) throw();
      DataTransfer::EventManager*  getEventManager();
      bool needThread() { return true; }
    };
  }
}

#endif

