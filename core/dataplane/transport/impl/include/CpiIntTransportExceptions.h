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
 *   This file contains the exceptions that are generated in the transport code;
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DATATRANSPORT_EXCEPTIONS_H_
#define CPI_DATATRANSPORT_EXCEPTIONS_H_

#include <CpiUtilException.h>

namespace DT {

  namespace Interface {

    // Our error code defintiions
    const CPI::OS::uint32_t DTP_EX_SOURCE_ID = 05;
    const CPI::OS::uint32_t CIRCUIT_ALREADY_EXISTS      = (DTP_EX_SOURCE_ID << 16) + 1;
    const CPI::OS::uint32_t SERVER_NOT_RESPONDING       = (DTP_EX_SOURCE_ID << 16) + 2;
    const CPI::OS::uint32_t FAILED_TO_CREATE_CIRCUIT    = (DTP_EX_SOURCE_ID << 16) + 3;


    /**********************************
     * Transport exception base class
     *********************************/
    class TransportExcept : public CPI::Util::EmbeddedException {
    public:
    TransportExcept(const CPI::OS::uint32_t errorCode, const char* ex="")
      :CPI::Util::EmbeddedException( errorCode, ex ){}
    };

    class CircuitAlreadyExists : public TransportExcept {
    public:
    CircuitAlreadyExists( const char* aux="")
      :TransportExcept(CIRCUIT_ALREADY_EXISTS, aux)
	{setAuxInfo(aux);}

    };

    class ServerNotResponding : public TransportExcept {
    public:
    ServerNotResponding( const char* aux="")
      :TransportExcept(SERVER_NOT_RESPONDING, aux)
	{setAuxInfo(aux);}

    };

    class CircuitCreateFailed : public TransportExcept {
    public:
    CircuitCreateFailed( const char* aux="")
      :TransportExcept(FAILED_TO_CREATE_CIRCUIT, aux)
	{setAuxInfo(aux);}

    };

  }

}


#endif


