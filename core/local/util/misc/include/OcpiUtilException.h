
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
 *   This class contains an exception class that can be used as a base class for all exceptions
 *   thrown in the OCPIG2 system.
 *
 * Author: John F. Miller
 *
 * Date: 7/6/99
 *
 */

#ifndef OcpiUtilException_h
#define OcpiUtilException_h

#include <string.h>
#include <stdarg.h>
#include <string>
#include "OcpiOsDataTypes.h"
#include "OcpiUtilExceptionApi.h"

namespace OCPI {

  namespace Util {


      // This is the implementation class of the API error class
      class Error : public OCPI::API::Error, public std::string {
      public:
	Error(const char *err, ...) __attribute__((format(printf, 2, 3)));
	virtual ~Error();
      protected:
	Error();
	void setFormatV(const char *fmt, va_list ap);
	void setFormat(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
	void setConcatenateV(const char *fmt, va_list ap);
	inline const std::string &error() const { return *this; }
      };

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
    enum ErrorLevel {
      ApplicationRecoverable,
      ApplicationFatal,
      ContainerFatal
    };
    /*
     * To control footprint, the embedded system uses error codes to report well known
     * error conditions. 
     *
     * Error code 0 is reserved for string errors.
     */
    class EmbeddedException : public Error {
    public:


      // Constructor
      EmbeddedException( 
                        const OCPI::OS::uint32_t errorCode,                // Embedded error code
                        const char* auxInfo,                                // Aux information
                        OCPI::OS::uint32_t errorLevel=0 );                // Error Level

      // String error only
      EmbeddedException( const char* auxInfo );

      //      EmbeddedException( const EmbeddedException& cpy );

      virtual ~EmbeddedException();

      OCPI::OS::uint32_t getErrorCode() const;
      const char* getAuxInfo() const;
      void  setAuxInfo( const char* info );

      OCPI::OS::uint32_t  m_errorCode;
      std::string        m_auxInfo;
      OCPI::OS::uint32_t  m_errorLevel;
    };


    /*
     * This is the exception monitor class.  This class is used in the embedded system
     * portion of the OCPI code where exceptions are not allowed.  This class is a member
     * of most of the embedded classes and is used to monitor error conditions after methods
     * are called.
     */
    class ExceptionMonitor : public EmbeddedException {
    public:
      bool error();
      void setError( EmbeddedException* ex );
      void setError( const EmbeddedException& ex );
      void setError( OCPI::OS::uint32_t ec, const char* aux_info );
      EmbeddedException& getError();
      bool clearError();
      bool m_ex;
      ExceptionMonitor();
      ExceptionMonitor( const ExceptionMonitor& rhs);
      EmbeddedException& operator =(EmbeddedException& ex);

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline OCPI::OS::uint32_t EmbeddedException::getErrorCode() const {return m_errorCode;}
    inline const char* EmbeddedException::getAuxInfo() const {return m_auxInfo.c_str();}
    inline void  EmbeddedException::setAuxInfo( const char* info ){m_auxInfo=info;}
#if 0
    inline         EmbeddedException::EmbeddedException( const EmbeddedException& cpy )
      : m_errorCode(cpy.m_errorCode), m_auxInfo(cpy.m_auxInfo), m_errorLevel(cpy.m_errorLevel)
      {
      }
#endif

    inline bool ExceptionMonitor::error(){return m_ex;}
    inline void ExceptionMonitor::setError( EmbeddedException* ex )
      {
        m_errorCode = ex->m_errorCode;
        m_auxInfo = ex->m_auxInfo;
        m_ex = true;
      }
    inline void ExceptionMonitor::setError( const EmbeddedException& ex )
      {
        m_errorCode = ex.m_errorCode;
        m_auxInfo = ex.m_auxInfo;
        m_ex = true;
      }
    inline void ExceptionMonitor::setError( OCPI::OS::uint32_t ec, const char* aux_info )
       {
        m_errorCode = ec;
        m_auxInfo = aux_info;
        m_ex = true;
      }
    inline EmbeddedException& ExceptionMonitor::getError(){m_ex=false; return *this;}
    inline bool ExceptionMonitor::clearError()
      {
        bool er = m_ex;
        m_ex=false;
        return er;
      }
    inline ExceptionMonitor::ExceptionMonitor():EmbeddedException(0,NULL),m_ex(false){}
      inline EmbeddedException& ExceptionMonitor::operator =(EmbeddedException& ex)
        {
          setError(ex); 
          return *this;
        }
      inline  ExceptionMonitor::ExceptionMonitor( const ExceptionMonitor& rhs)
        :EmbeddedException(0,NULL), m_ex(rhs.m_ex)
        {
          m_errorCode = rhs.m_errorCode;
          m_auxInfo = rhs.m_auxInfo;
        }



        /**********************************
         * This set of macros are used in the embedded system code to 
         * support platforms that do not support exceptions
         *********************************/

#define OCPI_EXCEPTION_OCCURED( x ) x->m_exceptionMonitor.error()
#define INHERIT_EXCEPTION( x ) m_exceptionMonitor = x->m_exceptionMonitor; \
    x->m_exceptionMonitor.clearError();

#define USE_REAL_EXCEPTIONS
#ifdef USE_REAL_EXCEPTIONS  // DONT TURN THIS FLAG ON, IT IS UNTESTED !!!!

#define LEVEL1 1
#define OCPI_TRY try
#define OCPI_CATCH( mon ) catch( ... )
#define OCPI_CATCH_LEVEL( mon, level ) catch ( DataTransferEx &)
#define OCPI_THROW_TO_NEXT_LEVEL(ex, level) throw ex;
#define OCPI_THROWVOID(ex) throw ex;
#define OCPI_THROWNULL(ex) throw ex;
#define OCPI_THROWNEG(ex) throw ex;
#define OCPI_RETHROWVOID  throw;
#define OCPI_RETHROWNULL  throw;
#define OCPI_RETHROWNEG   throw;
#define OCPI_SET_RETHROWVOID(ex) throw;
#define OCPI_SET_RETHROWNULL(ex) throw;
#define OCPI_SET_RETHROWNEG(ex)  throw;
#define OCPI_RETHROW_TO_NEXT_LEVEL(ex) throw;

#else

#define OCPI_TRY \

#define OCPI_CATCH( mon )if ( mon.clearError() )
#define OCPI_CATCH_LEVEL( mon, level )                \
        level:                                        \
          if ( mon.clearError() )

#define OCPI_THROW_TO_NEXT_LEVEL(ex, level) OCPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; goto level;
#define OCPI_RETHROW_TO_NEXT_LEVEL(level) goto level;

#define OCPI_THROWVOID( ex) OCPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; return;
#define OCPI_THROWNULL( ex)  OCPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; return NULL;
#define OCPI_THROWNEG( ex)  OCPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; return -1;

#define OCPI_RETHROWVOID  m_exceptionMonitor.m_ex=true; return;
#define OCPI_RETHROWNULL  m_exceptionMonitor.m_ex=true; return NULL;
#define OCPI_RETHROWNEG   m_exceptionMonitor.m_ex=true; return -1;

#define OCPI_SET_RETHROWVOID(ex) m_exceptionMonitor=ex; m_exceptionMonitor.m_ex=true; return;
#define OCPI_SET_RETHROWNULL(ex) m_exceptionMonitor=ex; m_exceptionMonitor.m_ex=true; return NULL;
#define OCPI_SET_RETHROWNEG(ex)  m_exceptionMonitor=ex; m_exceptionMonitor.m_ex=true; return -1;

#endif

      // Class that appends error strings rather than sprintfs them, sort of "legacy"
      class ApiError : public Error {
      public:
	std::string m_auxInfo; // legacy compatibility FIXME
	ApiError(const char *err, ...); // FIXME: remote this entirely to get better error checking
	virtual ~ApiError();
      };
  }
}

#endif

