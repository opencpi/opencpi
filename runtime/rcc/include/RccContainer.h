
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

#include "RccApplication.h"
#include "RccDriver.h"
#include "OcpiOsSemaphore.h"
#include "pthread_workqueue.h"

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

