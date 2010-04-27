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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_CONTAINER_APP_H__
#define CPI_CONTAINER_APP_H__

#include <vector>
#include <wci.h>
#include <CpiList.h>
#include <CpiOsDataTypes.h>
#include <CpiContainerDataTypes.h>
#include <CpiApplication.h>
#include <RCC_Worker.h>


namespace CPI {

  namespace DataTransport {
    class Transport;
    class Circuit;
  }

    namespace CP289 {

      class Worker;
      class Container;

      /**********************************
       * Containers represented Node Application class
       *********************************/
      class Application : public CPI::Container::Application
      {

    public:
	friend class Container;
	friend class Controller;

	CPI::Container::Artifact & createArtifact(const char *url, CPI::Util::PValue *params);
	CPI::Container::Worker & createWorker(const char *url, CPI::Util::PValue *aparams,
				   const void *entryPoint, const char *inst=NULL,
				   CPI::Util::PValue *wparams = NULL);


      /**********************************
       * Constructor
       *********************************/  
	Application(CPI::CP289::Container &, CPI::OS::Mutex& );

      /**********************************
       * Destructor
       *********************************/  
	virtual ~Application();

      /**********************************
       * Get the worker info from the worker id
       *********************************/ 
      CPI::CP289::Worker* getWorker( CPI::Container::WorkerId& worker );


      /**********************************
       * Get the transport object
       *********************************/ 
      CPI::DataTransport::Transport & getTransport();

      /**********************************
       * Add a circuit to our managed list
       *********************************/ 
      inline void addCircuit( CPI::DataTransport::Circuit* c ){m_circuits.push_back(c);}
      
      /**********************************
       * Get our shared mutex
       *********************************/ 
      inline CPI::OS::Mutex& mutex(){return m_mutex;}

      private:

      /**********************************
       * Add a worker
       *********************************/  
      CPI::CP289::Worker* createWorker();
      void removeWorker( CPI::Container::WorkerId worker );

      // Our thread safe mutex
      CPI::OS::Mutex& m_mutex;

      // RCC container
      RCCContainer *  m_rccContainer;

      // All circuits in this application
      std::vector<CPI::DataTransport::Circuit*> m_circuits;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
      inline  Worker* Application::getWorker( CPI::Container::WorkerId& worker )
      {
	return reinterpret_cast<Worker*>(worker);
      }

    }

}

#endif

