
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

#ifndef OCPI_CONTAINER_IMPL_H_
#define OCPI_CONTAINER_IMPL_H_

#include <OcpiList.h>
#include <OcpiOsMutex.h>
#include <OcpiOsTimer.h>
#include <RCC_Worker.h>
#include <OcpiContainerInterface.h>
#include <OcpiTransport.h>
#include <OcpiCP289Controller.h>


namespace OCPI {

  namespace Container {
    class PortData;
  }

  namespace CP289 {

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

    class Container : public OCPI::Container::Interface, public Controller
      {

      public:
        friend class Controller;
        friend class Application;

        /**********************************
         * Constructors
         *********************************/
        Container(  OCPI::Util::Driver &, OCPI::OS::uint32_t g_unique_id,
                         OCPI::DataTransport::TransportGlobal* tpg, 
                         const OCPI::Util::PValue* props )
          throw ( OCPI::Util::EmbeddedException );

        virtual ~Container()
          throw ();

        DispatchRetCode dispatch(DataTransfer::EventManager* event_manager=NULL)
          throw ( OCPI::Util::EmbeddedException );

        OCPI::Container::Application * createApplication()
          throw ( OCPI::Util::EmbeddedException );

        OCPI::Container::Artifact & createArtifact(const char *url, OCPI::Util::PValue *artifactParams = 0);

        std::vector<std::string> getSupportedEndpoints()
          throw ();
      
        //!< Start/Stop the container
        void start(DataTransfer::EventManager* event_manager)
          throw();
        void stop(DataTransfer::EventManager* event_manager)
          throw();

        //! get the event manager for this container
        DataTransfer::EventManager*  getEventManager();

      protected:

        // Our thread safe mutex
        OCPI::OS::Mutex m_threadSafeMutex;

        // Ocpi transport 
        OCPI::DataTransport::Transport* m_transport;


      };
  }
}

#endif

