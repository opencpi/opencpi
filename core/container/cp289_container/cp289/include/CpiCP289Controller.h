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
 *   This file contains the interface for the JTRS DSP Component Execution controller interface.
 *

   Revision History: 
 
     3/2005 - John Miller
     Initial version.   
	     
     5/18/2009 - John Miller
     Updated to support revised CP289U RCC specification.


 ****************************************************************************/

#ifndef CPI_NM_CP289_COMP__CONTROLLER_H
#define CPI_NM_CP289_COMP__CONTROLLER_H

#define WORKER_INTERNAL
#include <CpiCP289Worker.h>
#include <DtIntEventHandler.h>
#include <CpiTimeEmit.h>
#include <RCC_Worker.h>

namespace CPI {

    namespace CP289 {

      class Controller;


#ifdef WAS
      struct RCCWorkerContainer : public RCCContainer
      {
	RCCWorkerContainer( Controller* controller );
	private:
	Controller* m_controller;
      };
#endif



      /**********************************
       * This class is used to support the CP289 component model in the CPI
       * environment.
       *********************************/
      class Container;
      class Controller :  public ::RCCContainer, public CPI::Time::Emit
      {

      public:

	Controller( CPI::CP289::Container* c,
		    const char * monitorIPAddress = 0 );
	virtual ~Controller();

	/**********************************
	 * When a component  requests to run within the containers thread, the container calls
	 * this method periodically to allow the s scheduled execution.  This method MUST NOT
	 * BE BLOCKED.
	 *********************************/      
	bool run( DataTransfer::EventManager* event_manager );

	// Advance all ports 
	void advanceAll( Worker * worker );

      protected:

	CPI::OS::int32_t	
	  markWorkersPolled( CPI::CP289::Worker* worker );

	// Our component controller
	CPI::CP289::Container * m_container;

      };
    }
}

#endif
