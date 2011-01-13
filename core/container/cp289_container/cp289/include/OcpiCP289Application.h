
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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_CONTAINER_APP_H__
#define OCPI_CONTAINER_APP_H__

#include <vector>
#include <wci.h>
#include <OcpiList.h>
#include <OcpiOsDataTypes.h>
#include <OcpiContainerDataTypes.h>
#include <OcpiContainerApplication.h>
#include <OcpiArtifact.h>
#include <RCC_Worker.h>
#include <OcpiOsLoadableModule.h>


namespace OCPI {

  namespace DataTransport {
    class Transport;
    class Circuit;
  }

    namespace CP289 {

      class Worker;
      class Container;

      class Artifact : public OCPI::Container::Artifact {

      public:
        Artifact(OCPI::Container::Interface &, const char *url);
        virtual OCPI::Container::Worker &createWorkerX( OCPI::Container::Application &a, ezxml_t impl, ezxml_t inst, OCPI::Util::PValue *);
        virtual ~Artifact();
        bool hasUrl(const char *url);

      private:
        OCPI::OS::LoadableModule m_loader;
        bool                    m_open;
        RCCDispatch          *  m_dispatch;
        int                     m_workerCount;

      };


      /**********************************
       * Containers represented Node Application class
       *********************************/
      class Application : public OCPI::Container::Application
      {

    public:
        friend class Container;
        friend class Controller;

        OCPI::Container::Artifact & createArtifact(const char *url, OCPI::Util::PValue *params);
	OCPI::Container::Worker &createWorker(const char *url, OCPI::Util::PValue *aparams,
					     const char *entryPoint, const char *inst, OCPI::Util::PValue *wparams );

      /**********************************
       * Constructor
       *********************************/  
        Application(OCPI::CP289::Container &, OCPI::OS::Mutex& );

      /**********************************
       * Destructor
       *********************************/  
        virtual ~Application();

      /**********************************
       * Get the worker info from the worker id
       *********************************/ 
      OCPI::CP289::Worker* getWorker( OCPI::Container::WorkerId& worker );


      /**********************************
       * Get the transport object
       *********************************/ 
      OCPI::DataTransport::Transport & getTransport();

      /**********************************
       * Add a circuit to our managed list
       *********************************/ 
      inline void addCircuit( OCPI::DataTransport::Circuit* c ){m_circuits.push_back(c);}
      
      /**********************************
       * Get our shared mutex
       *********************************/ 
      inline OCPI::OS::Mutex& mutex(){return m_mutex;}

      private:

      /**********************************
       * Add a worker
       *********************************/  
      OCPI::CP289::Worker* createWorker();
      void removeWorker( OCPI::Container::WorkerId worker );

      // Our thread safe mutex
      OCPI::OS::Mutex& m_mutex;

      // RCC container
      RCCContainer *  m_rccContainer;

      // All circuits in this application
      std::vector<OCPI::DataTransport::Circuit*> m_circuits;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
      inline  Worker* Application::getWorker( OCPI::Container::WorkerId& worker )
      {
        return reinterpret_cast<Worker*>(worker);
      }

    }

}

#endif

