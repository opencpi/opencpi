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
      : public OCPI::Container::ContainerBase<Driver,Container,Application,Artifact>
    {

      public:
        friend class Port;
        friend class RDMAPort;
        friend class Worker;
        friend class Application;
	friend class PortDelegator;

        /**********************************
         * Constructors
         *********************************/
        Container(const char *name,
		  //		  OCPI::DataTransport::TransportGlobal* tpg, 
		  const OCPI::API::PValue* props )
          throw ( OCPI::Util::EmbeddedException );

        virtual ~Container()
          throw ();

	OCPI::Container::Container::DispatchRetCode
	dispatch(DataTransfer::EventManager* event_manager=NULL);

        OCPI::API::ContainerApplication*
	createApplication(const char *name, const OCPI::Util::PValue *props)
          throw ( OCPI::Util::EmbeddedException );

	OCPI::Container::Artifact &createArtifact(OCPI::Library::Artifact &lart,
						  const OCPI::API::PValue *artifactParams);
        void start(DataTransfer::EventManager* event_manager) throw();
        void stop(DataTransfer::EventManager* event_manager) throw();

        //! get the event manager for this container
        DataTransfer::EventManager*  getEventManager();
	bool needThread() { return true; }
      };
  }
}

#endif

