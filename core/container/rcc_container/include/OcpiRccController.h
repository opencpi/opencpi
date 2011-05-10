
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
 *   This file contains the interface for the JTRS DSP Component Execution controller interface.
 *

   Revision History: 
 
     3/2005 - John Miller
     Initial version.   
             
     5/18/2009 - John Miller
     Updated to support revised CP289U RCC specification.


 ****************************************************************************/

#ifndef OCPI_RCC_CONTROLLER_H
#define OCPI_RCC_CONTROLLER_H

#define WORKER_INTERNAL
#include <OcpiRccWorker.h>
#include <DtIntEventHandler.h>
#include <OcpiTimeEmit.h>
#include <RCC_Worker.h>

namespace OCPI {

    namespace RCC {

      class Controller;

      /**********************************
       * This class is used to support the RCC component model in the OCPI
       * environment.
       *********************************/
      class Container;
      class Controller :  public ::RCCContainer, public OCPI::Time::Emit
      {

      public:

        Controller(Container* c,
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

        OCPI::OS::int32_t        
          markWorkersPolled( Worker* worker );

        // Our component controller
        Container * m_container;

      };
    }
}

#endif
