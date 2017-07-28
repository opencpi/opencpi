
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


