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
 * Abstact:
 *   This file contains the Interface for the Ocpi transport thread hook class.
 *
 * Author: John F. Miller
 *
 * Date: 3/1/05
 *
 */

#ifndef OCPI_Transport_Thread_Hook_H_
#define OCPI_Transport_Thread_Hook_H_

namespace DataTransfer {
  class EventManager;
}

namespace OCPI {
  namespace DataTransport {
    class ThreadHook
    {
    public:

      /**********************************
       *  This method gets called to provide the circuit with execution time.
       **********************************/        
      virtual void dispatch(DataTransfer::EventManager* event_manager) = 0;
      virtual ~ThreadHook(){};
    };
  }
}


#endif

