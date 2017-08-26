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
 * Abstract:
 *   This file contains the exceptions that are generated in the transport code;
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DATATRANSPORT_EXCEPTIONS_H_
#define OCPI_DATATRANSPORT_EXCEPTIONS_H_

#include <stdint.h>
#include "OcpiUtilException.h"

namespace OCPI {

  namespace DataTransport {

    // Our error code defintiions
    const uint32_t DTP_EX_SOURCE_ID = 05;
    const uint32_t CIRCUIT_ALREADY_EXISTS      = (DTP_EX_SOURCE_ID << 16) + 1;
    const uint32_t SERVER_NOT_RESPONDING       = (DTP_EX_SOURCE_ID << 16) + 2;
    const uint32_t FAILED_TO_CREATE_CIRCUIT    = (DTP_EX_SOURCE_ID << 16) + 3;


    /**********************************
     * Transport exception base class
     *********************************/
    class TransportExcept : public OCPI::Util::EmbeddedException {
    public:
    TransportExcept(const uint32_t errorCode, const char* ex="")
      :OCPI::Util::EmbeddedException( errorCode, ex ){}
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


