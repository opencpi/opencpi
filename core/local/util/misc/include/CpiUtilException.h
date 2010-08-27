/*
 * Abstact:
 *   This class contains an exception class that can be used as a base class for all exceptions
 *   thrown in the CPIG2 system.
 *
 * Author: John F. Miller
 *
 * Date: 7/6/99
 *
 */

#ifndef CpiUtilException_h
#define CpiUtilException_h

#include <string.h>
#include <CpiOsDataTypes.h>
#include <string>

namespace CPI {

  namespace Util {


    /*
     * To control footprint, the embedded system uses error codes to report well known
     * error conditions. 
     *
     * Error code 0 is reserved for string errors.
     */
    class EmbeddedException {
    public:


      // Constructor
      EmbeddedException( 
                        const CPI::OS::uint32_t errorCode,                // Embedded error code
                        const char* auxInfo,                                // Aux information
                        CPI::OS::uint32_t errorLevel=0 );                // Error Level

      // String error only
      EmbeddedException( const char* auxInfo );

      EmbeddedException( const EmbeddedException& cpy );

      virtual ~EmbeddedException();

      CPI::OS::uint32_t getErrorCode() const;
      const char* getAuxInfo() const;
      void  setAuxInfo( const char* info );

      CPI::OS::uint32_t  m_errorCode;
      std::string        m_auxInfo;
      CPI::OS::uint32_t  m_errorLevel;
    };


    /*
     * This is the exception monitor class.  This class is used in the embedded system
     * portion of the CPI code where exceptions are not allowed.  This class is a member
     * of most of the embedded classes and is used to monitor error conditions after methods
     * are called.
     */
    class ExceptionMonitor : public EmbeddedException {
    public:
      bool error();
      void setError( EmbeddedException* ex );
      void setError( const EmbeddedException& ex );
      void setError( CPI::OS::uint32_t ec, const char* aux_info );
      EmbeddedException& getError();
      bool clearError();
      bool m_ex;
      ExceptionMonitor();
      ExceptionMonitor( const ExceptionMonitor& rhs);
      EmbeddedException& operator =(EmbeddedException& ex);

    protected:
    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline EmbeddedException::EmbeddedException( 
                                                CPI::OS::uint32_t errorCode, 
                                                const char* auxInfo,
                                                CPI::OS::uint32_t errorLevel )
      : m_errorCode(errorCode), m_auxInfo(auxInfo), m_errorLevel(errorLevel)
      {
      }
      // String error only (error code zero)
    inline EmbeddedException::EmbeddedException( const char* auxInfo )
      : m_errorCode(0), m_auxInfo(auxInfo), m_errorLevel(0)
      {
      }
    inline EmbeddedException::~EmbeddedException(){}
    inline CPI::OS::uint32_t EmbeddedException::getErrorCode() const {return m_errorCode;}
    inline const char* EmbeddedException::getAuxInfo() const {return m_auxInfo.c_str();}
    inline void  EmbeddedException::setAuxInfo( const char* info ){m_auxInfo=info;}
    inline         EmbeddedException::EmbeddedException( const EmbeddedException& cpy )
      : m_errorCode(cpy.m_errorCode), m_auxInfo(cpy.m_auxInfo), m_errorLevel(cpy.m_errorLevel)
      {
      }


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
    inline void ExceptionMonitor::setError( CPI::OS::uint32_t ec, const char* aux_info )
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

#define CPI_EXCEPTION_OCCURED( x ) x->m_exceptionMonitor.error()
#define INHERIT_EXCEPTION( x ) m_exceptionMonitor = x->m_exceptionMonitor; \
    x->m_exceptionMonitor.clearError();

#define USE_REAL_EXCEPTIONS
#ifdef USE_REAL_EXCEPTIONS  // DONT TURN THIS FLAG ON, IT IS UNTESTED !!!!

#define LEVEL1 1
#define CPI_TRY try
#define CPI_CATCH( mon ) catch( ... )
#define CPI_CATCH_LEVEL( mon, level ) catch ( DataTransferEx &)
#define CPI_THROW_TO_NEXT_LEVEL(ex, level) throw ex;
#define CPI_THROWVOID(ex) throw ex;
#define CPI_THROWNULL(ex) throw ex;
#define CPI_THROWNEG(ex) throw ex;
#define CPI_RETHROWVOID  throw;
#define CPI_RETHROWNULL  throw;
#define CPI_RETHROWNEG   throw;
#define CPI_SET_RETHROWVOID(ex) throw;
#define CPI_SET_RETHROWNULL(ex) throw;
#define CPI_SET_RETHROWNEG(ex)  throw;
#define CPI_RETHROW_TO_NEXT_LEVEL(ex) throw;

#else

#define CPI_TRY \

#define CPI_CATCH( mon )if ( mon.clearError() )
#define CPI_CATCH_LEVEL( mon, level )                \
        level:                                        \
          if ( mon.clearError() )

#define CPI_THROW_TO_NEXT_LEVEL(ex, level) CPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; goto level;
#define CPI_RETHROW_TO_NEXT_LEVEL(level) goto level;

#define CPI_THROWVOID( ex) CPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; return;
#define CPI_THROWNULL( ex)  CPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; return NULL;
#define CPI_THROWNEG( ex)  CPI::Util::EmbeddedException* exp; m_exceptionMonitor.setError( exp = new ex ); delete exp; return -1;

#define CPI_RETHROWVOID  m_exceptionMonitor.m_ex=true; return;
#define CPI_RETHROWNULL  m_exceptionMonitor.m_ex=true; return NULL;
#define CPI_RETHROWNEG   m_exceptionMonitor.m_ex=true; return -1;

#define CPI_SET_RETHROWVOID(ex) m_exceptionMonitor=ex; m_exceptionMonitor.m_ex=true; return;
#define CPI_SET_RETHROWNULL(ex) m_exceptionMonitor=ex; m_exceptionMonitor.m_ex=true; return NULL;
#define CPI_SET_RETHROWNEG(ex)  m_exceptionMonitor=ex; m_exceptionMonitor.m_ex=true; return -1;

#endif

  }


}

#endif

