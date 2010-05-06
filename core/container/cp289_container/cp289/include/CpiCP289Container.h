// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file contains the interface for the CPI Node Manager Container.  
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

#ifndef CPI_CONTAINER_IMPL_H_
#define CPI_CONTAINER_IMPL_H_

#include <CpiList.h>
#include <CpiOsMutex.h>
#include <CpiOsTimer.h>
#include <RCC_Worker.h>
#include <CpiContainerInterface.h>
#include <CpiTransport.h>
#include <CpiCP289Controller.h>


namespace CPI {

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
    const CPI::OS::uint32_t CP289_EX_SOURCE_ID          = 10;
    const CPI::OS::uint32_t BAD_ARTIFACT_URL            = (CP289_EX_SOURCE_ID << 16) + 1;
    const CPI::OS::uint32_t REQUIRED_PROPERTY_NOT_SET   = (CP289_EX_SOURCE_ID << 16) + 2;
    const CPI::OS::uint32_t WORKER_INIT_FAILED          = (CP289_EX_SOURCE_ID << 16) + 3;
    const CPI::OS::uint32_t WORKER_ENABLE_FAILED          = (CP289_EX_SOURCE_ID << 16) + 4;
    const CPI::OS::uint32_t WORKER_DISABLE_FAILED          = (CP289_EX_SOURCE_ID << 16) + 5;
    const CPI::OS::uint32_t CP289_CSINTERNAL_ERROR          = (CP289_EX_SOURCE_ID << 16) + 6;

    class Container : public CPI::Container::Interface, public Controller
      {

      public:
        friend class Controller;
        friend class Application;

        /**********************************
         * Constructors
         *********************************/
        Container(  CPI::Util::Driver &, CPI::OS::uint32_t g_unique_id,
                         CPI::DataTransport::TransportGlobal* tpg, 
                         const CPI::Util::PValue* props )
          throw ( CPI::Util::EmbeddedException );

        virtual ~Container()
          throw ();

        DispatchRetCode dispatch(DataTransfer::EventManager* event_manager=NULL)
          throw ( CPI::Util::EmbeddedException );

        CPI::Container::Application * createApplication()
          throw ( CPI::Util::EmbeddedException );

        CPI::Container::Artifact & createArtifact(const char *url, CPI::Util::PValue *artifactParams = 0);

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
        CPI::OS::Mutex m_threadSafeMutex;

        // Cpi transport 
        CPI::DataTransport::Transport* m_transport;


      };
  }
}

#endif

