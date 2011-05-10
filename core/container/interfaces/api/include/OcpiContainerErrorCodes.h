
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
 *   This file contains the predefined error codes for the container implementations.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_CONTAINER_ERROR_CODES_H__
#define OCPI_CONTAINER_ERROR_CODES_H__

#include <OcpiOsDataTypes.h>
#include <OcpiUtilException.h>

namespace OCPI {

  namespace Container {

    const OCPI::OS::uint32_t NO_ERROR_                   = 0;
    const OCPI::OS::uint32_t ARTIFACT_LOAD_ERROR         = 1;
    const OCPI::OS::uint32_t WORKER_CREATE_ERROR         = 2;   
    const OCPI::OS::uint32_t PROPERTY_NOT_SET            = 3;  
    const OCPI::OS::uint32_t MAIL_BOX_NOT_ALLOCATED      = 4;  
    const OCPI::OS::uint32_t RESOURCE_EXCEPTION          = 5;
    const OCPI::OS::uint32_t APPLICATION_EXCEPTION       = 6;
    const OCPI::OS::uint32_t WORKER_NOT_FOUND            = 7;
    const OCPI::OS::uint32_t PORT_NOT_FOUND              = 8;
    const OCPI::OS::uint32_t BAD_CONNECTION_COOKIE       = 9;
    const OCPI::OS::uint32_t ARTIFACT_NOT_FOUND          = 10;
    const OCPI::OS::uint32_t PORT_ALREADY_CONNECTED      = 11;
    const OCPI::OS::uint32_t BAD_CONNECTION_REQUEST      = 12;
    const OCPI::OS::uint32_t PORT_NOT_CONNECTED          = 13;
    const OCPI::OS::uint32_t PORT_CONFIG_ERROR           = 14;
    const OCPI::OS::uint32_t NOT_YET_IMPLEMENTED         = 15;
    const OCPI::OS::uint32_t NO_MORE_MEMORY              = 16;
    const OCPI::OS::uint32_t CONTROL_PLANE_EXCEPTION     = 17;
    const OCPI::OS::uint32_t PROPERTY_SET_EXCEPTION      = 18;
    const OCPI::OS::uint32_t PROPERTY_GET_EXCEPTION      = 19;
    const OCPI::OS::uint32_t CIRCUIT_NOT_FOUND           = 20;
    const OCPI::OS::uint32_t ONP_WORKER_STARTED          = 21; // Operation not permitted while worker started
    const OCPI::OS::uint32_t BAD_PORT_CONFIGURATION      = 22;
    const OCPI::OS::uint32_t WORKER_ERROR                = 23; // and maybe string from worker
    const OCPI::OS::uint32_t WORKER_FATAL                = 24; // and maybe string from worker
    const OCPI::OS::uint32_t WORKER_API_ERROR            = 25;
    const OCPI::OS::uint32_t TEST_NOT_IMPLEMENTED        = 26;
    const OCPI::OS::uint32_t INVALID_CONTROL_SEQUENCE    = 27;
    const OCPI::OS::uint32_t PORT_COUNT_MISMATCH         = 28;
    const OCPI::OS::uint32_t WORKER_UNUSABLE             = 29;
    const OCPI::OS::uint32_t ARTIFACT_UNSUPPORTED        = 30;
    const OCPI::OS::uint32_t NO_ARTIFACT_FOR_WORKER      = 31;
    const OCPI::OS::uint32_t CONTAINER_HAS_OWN_THREAD    = 32;


    const OCPI::OS::uint32_t INTERNAL_PROGRAMMING_ERROR  = 50;

    const OCPI::OS::uint32_t LAST_ERROR_ID               = 100;


    // Our error levels
    enum ContainerErrorLevel {
      ApplicationRecoverable,
      ApplicationFatal,
      ContainerFatal
    };


    // Container exception types
    class ResourceEx : public OCPI::Util::EmbeddedException {
    public:
    ResourceEx( const char* ex, ContainerErrorLevel level )
      :EmbeddedException(RESOURCE_EXCEPTION, ex,(OCPI::OS::uint32_t)level) {
        setAuxInfo(ex);
      }
    };
    class ApplicationEx : public OCPI::Util::EmbeddedException {
    public:
    ApplicationEx( const char* ex, ContainerErrorLevel level )
      :EmbeddedException(APPLICATION_EXCEPTION, ex,(OCPI::OS::uint32_t)level) {
        setAuxInfo(ex);
      }
    };

    class InternalProgrammingErrorEx : public OCPI::Util::EmbeddedException {
    public:
    InternalProgrammingErrorEx( const char* ex, ContainerErrorLevel level )
      :EmbeddedException(INTERNAL_PROGRAMMING_ERROR, ex,(OCPI::OS::uint32_t)level) {
        setAuxInfo(ex);
      }
    };
  }
}

#endif

