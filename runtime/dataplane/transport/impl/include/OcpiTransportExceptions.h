
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
 * Abstract:
 *   This file contains the exception definitions for the data transport system.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_TRANSPORT_EXCEPTIONS_H_
#define OCPI_TRANSPORT_EXCEPTIONS_H_

#include "OcpiIntTransportExceptions.h"

namespace OCPI  {

  namespace DataTransport {

    // Our error code defintiions
    const uint32_t OCPI_EX_SOURCE_ID = 07;
    const uint32_t UNSUPPORTED_ENDPOINT         = (OCPI_EX_SOURCE_ID << 16) + 1;
    const uint32_t CIRCUIT_DISCONNECTING         = (OCPI_EX_SOURCE_ID << 16) + 2;
    const uint32_t INTERNAL_PROGRAMMING_ERROR   = (OCPI_EX_SOURCE_ID << 16) + 3;
    const uint32_t NO_MORE_BUFFER_AVAILABLE      = (OCPI_EX_SOURCE_ID << 16) + 4;
    const uint32_t UNABLE_TO_CREATE_TX_REQUEST  = (OCPI_EX_SOURCE_ID << 16) + 5;
    const uint32_t INTERNAL_PROGRAMMING_ERROR1  = (OCPI_EX_SOURCE_ID << 16) + 6;
    const uint32_t MAX_ENDPOINT_COUNT_EXCEEDED  = (OCPI_EX_SOURCE_ID << 16) + 7;
                
    /**********************************
     * Exception definitions
     *********************************/
                
    class UnsupportedEndpointEx : public  TransportExcept {
    public:
      UnsupportedEndpointEx( const char* aux)
        : TransportExcept(UNSUPPORTED_ENDPOINT, aux)
        {setAuxInfo(aux);}
    };
                
    class CircuitDisconnectingEx : public TransportExcept {
    public:
      CircuitDisconnectingEx( const char* aux="")
        : TransportExcept(CIRCUIT_DISCONNECTING, aux )
        {setAuxInfo(aux);}
    };

  }
}


#endif


