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
 *   This file contains the predefined error codes for the container implementations.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_CONTAINER_ERROR_CODES_H__
#define CPI_CONTAINER_ERROR_CODES_H__

#include <CpiOsDataTypes.h>
#include <CpiUtilException.h>

namespace CPI {

  namespace Container {

    const CPI::OS::uint32_t NO_ERROR_                   = 0;
    const CPI::OS::uint32_t ARTIFACT_LOAD_ERROR         = 1;
    const CPI::OS::uint32_t WORKER_CREATE_ERROR         = 2;   
    const CPI::OS::uint32_t PROPERTY_NOT_SET            = 3;  
    const CPI::OS::uint32_t MAIL_BOX_NOT_ALLOCATED      = 4;  
    const CPI::OS::uint32_t RESOURCE_EXCEPTION		    = 5;
    const CPI::OS::uint32_t APPLICATION_EXCEPTION       = 6;
    const CPI::OS::uint32_t WORKER_NOT_FOUND		    = 7;
    const CPI::OS::uint32_t PORT_NOT_FOUND              = 8;
    const CPI::OS::uint32_t BAD_CONNECTION_COOKIE       = 9;
    const CPI::OS::uint32_t ARTIFACT_NOT_FOUND          = 10;
    const CPI::OS::uint32_t PORT_ALREADY_CONNECTED      = 11;
    const CPI::OS::uint32_t BAD_CONNECTION_REQUEST      = 12;
    const CPI::OS::uint32_t PORT_NOT_CONNECTED          = 13;
    const CPI::OS::uint32_t PORT_CONFIG_ERROR           = 14;
    const CPI::OS::uint32_t NOT_YET_IMPLEMENTED         = 15;
    const CPI::OS::uint32_t NO_MORE_MEMORY              = 16;
    const CPI::OS::uint32_t CONTROL_PLANE_EXCEPTION     = 17;
    const CPI::OS::uint32_t PROPERTY_SET_EXCEPTION      = 18;
    const CPI::OS::uint32_t PROPERTY_GET_EXCEPTION      = 19;
    const CPI::OS::uint32_t CIRCUIT_NOT_FOUND		    = 20;
    const CPI::OS::uint32_t ONP_WORKER_STARTED          = 21;  // Operation not permitted while worker is started
    const CPI::OS::uint32_t BAD_PORT_CONFIGURATION      = 22;


    const CPI::OS::uint32_t INTERNAL_PROGRAMMING_ERROR  = 50;

    const CPI::OS::uint32_t LAST_ERROR_ID               = 100;


    // Our error levels
    enum ContainerErrorLevel {
      ApplicationRecoverable,
      ApplicationFatal,
      ContainerFatal
    };


    // Container exception types
    class ResourceEx : public CPI::Util::EmbeddedException {
    public:
    ResourceEx( const char* ex, ContainerErrorLevel level )
      :EmbeddedException(RESOURCE_EXCEPTION, ex,(CPI::OS::uint32_t)level) {
	setAuxInfo(ex);
      }
    };
    class ApplicationEx : public CPI::Util::EmbeddedException {
    public:
    ApplicationEx( const char* ex, ContainerErrorLevel level )
      :EmbeddedException(APPLICATION_EXCEPTION, ex,(CPI::OS::uint32_t)level) {
	setAuxInfo(ex);
      }
    };

    class InternalProgrammingErrorEx : public CPI::Util::EmbeddedException {
    public:
    InternalProgrammingErrorEx( const char* ex, ContainerErrorLevel level )
      :EmbeddedException(INTERNAL_PROGRAMMING_ERROR, ex,(CPI::OS::uint32_t)level) {
	setAuxInfo(ex);
      }
    };
  }
}

#endif

