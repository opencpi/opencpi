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
 *   This file contains the exception definitions for the data transport system.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_TRANSPORT_EXCEPTIONS_H_
#define CPI_TRANSPORT_EXCEPTIONS_H_

#include <CpiUtilException.h>
#include <CpiIntTransportExceptions.h>

namespace CPI  {

  namespace DataTransport {

    // Our error code defintiions
    const CPI::OS::uint32_t CPI_EX_SOURCE_ID = 07;
    const CPI::OS::uint32_t UNSUPPORTED_ENDPOINT         = (CPI_EX_SOURCE_ID << 16) + 1;
    const CPI::OS::uint32_t CIRCUIT_DISCONNECTING         = (CPI_EX_SOURCE_ID << 16) + 2;
    const CPI::OS::uint32_t INTERNAL_PROGRAMMING_ERROR   = (CPI_EX_SOURCE_ID << 16) + 3;
    const CPI::OS::uint32_t NO_MORE_BUFFER_AVAILABLE      = (CPI_EX_SOURCE_ID << 16) + 4;
    const CPI::OS::uint32_t UNABLE_TO_CREATE_TX_REQUEST  = (CPI_EX_SOURCE_ID << 16) + 5;
    const CPI::OS::uint32_t INTERNAL_PROGRAMMING_ERROR1  = (CPI_EX_SOURCE_ID << 16) + 6;
    const CPI::OS::uint32_t MAX_ENDPOINT_COUNT_EXCEEDED  = (CPI_EX_SOURCE_ID << 16) + 7;
                
    /**********************************
     * Exception definitions
     *********************************/
                
    class UnsupportedEndpointEx : public  DT::Interface::TransportExcept {
    public:
      UnsupportedEndpointEx( const char* aux)
        : DT::Interface::TransportExcept(UNSUPPORTED_ENDPOINT, aux)
        {setAuxInfo(aux);}
    };
                
    class CircuitDisconnectingEx : public DT::Interface::TransportExcept {
    public:
      CircuitDisconnectingEx( const char* aux="")
        : DT::Interface::TransportExcept(CIRCUIT_DISCONNECTING, aux )
        {setAuxInfo(aux);}
    };

  }
}


#endif


