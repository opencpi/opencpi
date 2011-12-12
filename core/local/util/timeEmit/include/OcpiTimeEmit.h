
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

//#define OCPI_TIME_EMIT_SUPPORT



/**
 * \file
 * \brief Time Analyzer classes
 *
 * Revision History:
 *
 *     08/19/2009 - John F. Miller
 *                  Initial version.
 */





/*

    Environment options:

    // Q size in bytes
    "OCPI_TIME_EMIT_Q_SIZE"

    // Stop collecting events when Q is Full
    "OCPI_TIME_EMIT_Q_SWF"
    
    // Emit event on construction/destruction of inherited classes
    "OCPI_TIME_EMIT_TRACE_CD"

    // Dump on exit
    "OCPI_TIME_EMIT_DUMP_ON_EXIT"

    // Dump mode
    "OCPI_TIME_EMIT_DUMP_FORMAT"
       "RAW"
       "READABLE"
       "VCD"

   // File name to dump time data into
   "OCPI_TIME_EMIT_DUMP_FILENAME"

    Make options:

    // compile in the support for the emit macros
    OCPI_TIME_EMIT_SUPPORT   

    // turn on multi-threaded support
    OCPI_TIME_EMIT_MULTI_THREADED    

*/


#ifndef OCPI_TIME_ANALYZER_H_
#define OCPI_TIME_ANALYZER_H_

#include <OcpiOsDataTypes.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <OcpiPValue.h>
#include <OcpiOsMutex.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiUtilException.h>


// Support macros that are not used by the end user
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


#ifdef OCPI_TIME_EMIT_SUPPORT

#define OCPI_EMIT_REGISTER(  name )        \
  static OCPI::Time::Emit::RegisterEvent re(name)

#define OCPI_EMIT_REGISTER_FULL(  name, dtype, width, etype )        \
  static OCPI::Time::Emit::RegisterEvent re(name,width,etype,dtype)

#define OCPI_EMIT_REGISTER_P( p )        \
  static OCPI::Time::Emit::RegisterEvent re(p)

// The following macros are used to emit events within a class that has inherited from Time::Emit
// They assume that they have access to "this" and they will preserve the class heirarchy information.

#define OCPI_EMIT_HERE                \
{ \
  OCPI_EMIT_REGISTER_FULL( __FILE__ "_line_"  TOSTRING(__LINE__), OCPI::Time::Emit::u, 1, OCPI::Time::Emit::Transient); \
  OCPI::Time::Emit::getSEmit().emit(re);                        \
}
#define OCPI_EMIT_HERE_                \
{ \
  OCPI_EMIT_REGISTER_FULL( __FILE__ "_line_"  TOSTRING(__LINE__), OCPI::Time::Emit::u, 1, OCPI::Time::Emit::Transient); \
  this->emit(re);                \
}

#define OCPI_EMIT_REGISTERED( re,v )                \
do { \
  OCPI::Time::Emit::getSEmit().emit(re,v);        \
 } while(0)
#define OCPI_EMIT_REGISTERED_( re,v )                \
do { \
  this->emit(re,v);                                \
} while(0)

#define OCPI_EMIT( name ) \
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::u, 1, OCPI::Time::Emit::Transient); \
  OCPI::Time::Emit::getSEmit().emit(re);                        \
} while(0)

#define OCPI_EMIT_( name ) \
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::u, 1, OCPI::Time::Emit::Transient); \
  this->emit(re);           \
 } while(0)

#define OCPI_EMIT_STATE( name, state )                        \
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::u, 1, OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(state)); \
} while(0)
#define OCPI_EMIT_STATE_( name, state )                \
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::u, 1, OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(state)); \
 } while(0)

#define OCPI_EMIT_PVALUE_( p ) \
do { \
  OCPI_EMIT_REGISTER_P( p ) \
  this->emit(re, p);        \
 } while(0)

#define OCPI_EMIT_PVALUE( p ) \
do { \
  OCPI_EMIT_REGISTER_P( p ) \
  OCPI::Time::Emit::getSEmit().emit(re, p);        \
} while(0)


#define OCPI_EMIT_UINT64_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,64,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT64( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,64,OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT32_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,32,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT32( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,32,OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT16_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,16,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT16( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,16,OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT8_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,8,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT8( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::u,8,OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)

#else

#define OCPI_EMIT_REGISTER(  name )
#define OCPI_EMIT_REGISTER_FULL(  name, dtype, width, etype )
#define OCPI_EMIT_REGISTER_P( p )

#define OCPI_EMIT_HERE
#define OCPI_EMIT_HERE_

#define OCPI_EMIT_REGISTERED( re,v )
#define OCPI_EMIT_REGISTERED_( re,v )

#define OCPI_EMIT( name )
#define OCPI_EMIT_( name )

#define OCPI_EMIT_STATE( name, state )
#define OCPI_EMIT_STATE_( name, state )


#define OCPI_EMIT_PVALUE( p )
#define OCPI_EMIT_PVALUE_( p )

#define OCPI_EMIT_UINT64_( name, value )
#define OCPI_EMIT_UINT64( name, value )
#define OCPI_EMIT_UINT32_( name, value )
#define OCPI_EMIT_UINT32( name, value )
#define OCPI_EMIT_UINT16_( name, value )
#define OCPI_EMIT_UINT16( name, value )
#define OCPI_EMIT_UINT8_( name, value )
#define OCPI_EMIT_UINT8( name, value )

#endif




namespace OCPI {

  namespace Time {

    class Emit {

    public:
      typedef OCPI::OS::uint16_t EventId;
      typedef OCPI::OS::uint16_t OwnerId;
      typedef OCPI::OS::uint64_t Time;

      enum EventType {
        Transient,
        State,
        Value
      };

      // These types are used by the formatter class to determine how to 
      // display the event values.
      enum DataType {
        u,   // unsigned
        i,   // signed
        d,   // double
        c    // character
      };

      struct QConfig {
        unsigned int size;
        bool         stopWhenFull;
      };
      
      enum EventTriggerRole {

        // This allows anyone with this role to trigger the collection of events in the local Q.
        // The first event with this role starts collection.
        LocalQGroupTrigger,  

        // This role allows an event to start event collection.  If more than 1 event use this role,
        // The last event with this role "restarts" event collection in the local Q.  
        LocalQMasterTrigger,

        // This allows anyone with this role to trigger the collection of events in all the Q's
        // The first event with this role starts collection.
        GlobalQGroupTrigger,

        // This role allows an event to start event collection.  If more than 1 event use this role,
        // The last event with this role "restarts" event collection in all the Q's.  
        GlobalQMasterTrigger,

        NoTrigger
      };
      
      // Forward references
      struct EventQEntry;
      struct EventQ;
      struct Header;
      struct HeaderEntry;
      struct EventMap;

    public:
      friend class EmitFormatter;

      /*
       *  This class provides a convienient way to register a named event statically
       *  with little runtime overhead.
       */
      class RegisterEvent {
      public:
        RegisterEvent( const char* event_name,
                       int width=1, 
                       EventType type=OCPI::Time::Emit::Transient,
                       DataType dt=OCPI::Time::Emit::u
                       );
        RegisterEvent( OCPI::API::PValue& pvstr );
        static int registerEvent( const char* event_name,
                           int width=1, 
                           EventType type=OCPI::Time::Emit::Transient,
                           DataType dt=OCPI::Time::Emit::u
                           );

        operator EventId () const {return m_eid;}

      private:
        EventId m_eid;

	// FIXME:  we can't possibly create a mutex for every event???
	// FIXME:  which calls separate static destructors per event?
        OCPI::OS::Mutex m_mutex;
        
      };

      // This is the base class for the time source that gets used by the emit class for time stamping events.
      class TimeSource {
      public:
        TimeSource();
        virtual Time getTime()=0;
        virtual ~TimeSource(){}
      };

      // This class uses "gettimeofday" to get the time tag
      class SimpleSystemTime : public TimeSource {
      public:
        Time getTime();
      private:
        virtual ~SimpleSystemTime(){};
      };

      // This class uses both "gettimeofday" and the CPU free runnging clock to minimize the 
      // "call site" latency.
      class FastSystemTime : public TimeSource {
      public:
        FastSystemTime();
        Time getTime();
      private:
          int m_method;
          virtual ~FastSystemTime(){};
      };
      

      /*
       * This constructor is used by a top-level traceable object.
       */
      Emit( const char* class_name=NULL, 
                 const char* instance_name=NULL, QConfig* config=NULL )
        throw ( OCPI::Util::EmbeddedException );

      /*
       * This constructor is used by a top-level traceable object.
       */
      Emit( TimeSource& ts, const char* class_name=NULL, 
                 const char* instance_name=NULL, QConfig* config=NULL )
        throw ( OCPI::Util::EmbeddedException );

      /*
       * This constructor is used by sub-ordinate objects.
       */
      Emit( Emit* parent, const char* class_name=NULL, 
                 const char* instance_name=NULL, QConfig* config=NULL )
        throw ( OCPI::Util::EmbeddedException ) ;
      ~Emit()
        throw ( );

      // Header utility methods
      OwnerId addHeader( Emit* t );

      // Trace method
      void emit( EventId id, OCPI::OS::uint64_t v, EventTriggerRole role=NoTrigger );
      void emit( EventId id, EventTriggerRole role=NoTrigger );
      void emit( EventId id, OCPI::API::PValue& p, EventTriggerRole role=NoTrigger );

      static Emit& getSEmit();
      static void sEmit( EventId id, OCPI::OS::uint64_t v, EventTriggerRole role=NoTrigger );
      static void sEmit( EventId id, EventTriggerRole role=NoTrigger );

      // Stop collecting events now
      void stop( bool globally = true );

      // Member access
      inline OCPI::OS::uint32_t getLevel(){return m_level;};
      inline std::string& getClassName(){return m_className; }
      inline Emit* getParent(){return m_parent;}

      // Get Headers
      static Header& getHeader();
      // This mutex is used to protect the static header data
      static inline OCPI::OS::Mutex& getGMutex() {
        static OCPI::OS::Mutex m_g_mutex(true);
        return m_g_mutex;
      }

      // Shutdown, deletes all global resources 
      static void shutdown()
        throw ( );

    private:

      void pre_init( const char* class_name, 
                const char* instance_name, 
                QConfig* config )
        throw ( OCPI::Util::EmbeddedException );

      void init()
        throw ( OCPI::Util::EmbeddedException );

      // static class methods
      static Time getTime();

      // Get the header information
      void getHeaderInfo( Emit* t, int& instance  );

      // Process event trigger
      inline void processTrigger( EventTriggerRole role );

      // Determines if this id is a child of this class
      bool isChild( Emit::OwnerId id );

      unsigned int   m_level;
      std::string    m_className;
      std::string    m_instanceName;
      Emit*  m_parent;
      EventQ*        m_q;
      OwnerId        m_myId;
      int            m_parentIndex;
      OCPI::OS::Mutex m_mutex;
      TimeSource*    m_ts;
    };


    /*
     * Base class for formatting the trace output.
     */
    class EmitFormatter {

    public:

      enum DumpFormat {
        OCPIReadable,
        OCPIRaw,
        VCDFormat
      };

      EmitFormatter( DumpFormat df=OCPIReadable  );

      // Returns the "pretty" description of an event
      const char* getEventDescription( Emit::EventId id );

      // Returns the hierachical string describing the owning class
      static void formatOwnerString( Emit::OwnerId id, std::string& str, bool full_path=true );

      // Utility format methods
      std::string formatEventString ( Emit::EventQEntry& eqe,
                                      Emit::Time time_ref );
      std::string formatEventStringRaw ( Emit::EventQEntry& eqe,
                                         Emit::Time time_ref );

      // Top level formatter methods
      std::ostream& formatDumpToStream( std::ostream& out );
      std::ostream& formatDumpToStreamRaw( std::ostream& out );
      std::ostream& formatDumpToStreamReadable( std::ostream& out );
      std::ostream& formatDumpToStreamVCD( std::ostream& out );

    private:
      Emit* m_traceable;
      DumpFormat m_dumpFormat;

    };

  }
}

#define OCPI_TIME_ANALYZER_INLINE_VALID_USE__
#include <OcpiTimeEmit_inlines.h>
#undef OCPI_TIME_ANALYZER_INLINE_VALID_USE__


#endif
