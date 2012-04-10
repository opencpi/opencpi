
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

#ifndef OCPI_RCC_CONTAINER_H_
#define OCPI_RCC_CONTAINER_H_

#include <OcpiList.h>
#include <OcpiOsMutex.h>
#include <OcpiOsTimer.h>
#include <OcpiContainerManager.h>
#include <RCC_Worker.h>
#include <OcpiTransport.h>
#include <OcpiRccDriver.h>
#include <OcpiRccApplication.h>
#include <OcpiTimeEmit.h>

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
    const OCPI::OS::uint32_t CP289_EX_SOURCE_ID          = 10;
    const OCPI::OS::uint32_t BAD_ARTIFACT_URL            = (CP289_EX_SOURCE_ID << 16) + 1;
    const OCPI::OS::uint32_t REQUIRED_PROPERTY_NOT_SET   = (CP289_EX_SOURCE_ID << 16) + 2;
    const OCPI::OS::uint32_t WORKER_INIT_FAILED          = (CP289_EX_SOURCE_ID << 16) + 3;
    const OCPI::OS::uint32_t WORKER_ENABLE_FAILED          = (CP289_EX_SOURCE_ID << 16) + 4;
    const OCPI::OS::uint32_t WORKER_DISABLE_FAILED          = (CP289_EX_SOURCE_ID << 16) + 5;
    const OCPI::OS::uint32_t CP289_CSINTERNAL_ERROR          = (CP289_EX_SOURCE_ID << 16) + 6;

    class Container
      : public OCPI::Container::ContainerBase<Driver,Container,Application,Artifact>,
      public OCPI::Time::Emit
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
		  OCPI::DataTransport::TransportGlobal* tpg, 
		  const OCPI::API::PValue* props )
          throw ( OCPI::Util::EmbeddedException );

        virtual ~Container()
          throw ();

	OCPI::Container::Container::DispatchRetCode dispatch(DataTransfer::EventManager* event_manager=NULL);

        OCPI::API::ContainerApplication*
	createApplication(const char *name, const OCPI::Util::PValue *props)
          throw ( OCPI::Util::EmbeddedException );

	OCPI::Container::Artifact &createArtifact(OCPI::Library::Artifact &lart,
						  const OCPI::API::PValue *artifactParams);
      
        //!< Start/Stop the container
        void start(DataTransfer::EventManager* event_manager)
          throw();
        void stop(DataTransfer::EventManager* event_manager)
          throw();

        //! get the event manager for this container
        DataTransfer::EventManager*  getEventManager();

	inline OCPI::DataTransport::Transport& getTransport() { return *m_transport; }

	bool needThread() { return true; }
      protected:

        // Ocpi transport 
        OCPI::DataTransport::Transport *m_transport;


      };
  }
}

#endif

