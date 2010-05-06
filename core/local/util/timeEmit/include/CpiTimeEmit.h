#define CPI_TIME_EMIT_SUPPORT

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
    "CPI_TIME_EMIT_Q_SIZE"

    // Stop collecting events when Q is Full
    "CPI_TIME_EMIT_Q_SWF"
    
    // Emit event on construction/destruction of inherited classes
    "CPI_TIME_EMIT_TRACE_CD"

    // Dump on exit
    "CPI_TIME_EMIT_DUMP_ON_EXIT"

    // Dump mode
    "CPI_TIME_EMIT_DUMP_FORMAT"
       "RAW"
       "READABLE"
       "VCD"

   // File name to dump time data into
   "CPI_TIME_EMIT_DUMP_FILENAME"

    Make options:

    // compile in the support for the emit macros
    CPI_TIME_EMIT_SUPPORT   

    // turn on multi-threaded support
    CPI_TIME_EMIT_MULTI_THREADED    

*/


#ifndef CPI_TIME_ANALYZER_H_
#define CPI_TIME_ANALYZER_H_

#include <CpiOsDataTypes.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <CpiPValue.h>
#include <CpiOsMutex.h>
#include <CpiUtilAutoMutex.h>
#include <CpiUtilException.h>


// Support macros that are not used by the end user
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


#ifdef CPI_TIME_EMIT_SUPPORT

#define CPI_EMIT_REGISTER(  name )        \
  static CPI::Time::Emit::RegisterEvent re(name);

#define CPI_EMIT_REGISTER_FULL(  name, dtype, width, etype )        \
  static CPI::Time::Emit::RegisterEvent re(name,width,etype,dtype);

#define CPI_EMIT_REGISTER_P( p )        \
  static CPI::Time::Emit::RegisterEvent re(p);

// The following macros are used to emit events within a class that has inherited from Time::Emit
// They assume that they have access to "this" and they will preserve the class heirarchy information.

#define CPI_EMIT_HERE                \
{ \
  CPI_EMIT_REGISTER_FULL( __FILE__ "_line_"  TOSTRING(__LINE__), CPI::Time::Emit::u, 1, CPI::Time::Emit::Transient); \
  CPI::Time::Emit::getSEmit().emit(re);                        \
}
#define CPI_EMIT_HERE_                \
{ \
  CPI_EMIT_REGISTER_FULL( __FILE__ "_line_"  TOSTRING(__LINE__), CPI::Time::Emit::u, 1, CPI::Time::Emit::Transient); \
  this->emit(re);                \
}

#define CPI_EMIT_REGISTERED( re,v )                \
{ \
  CPI::Time::Emit::getSEmit().emit(re,v);        \
}
#define CPI_EMIT_REGISTERED_( re,v )                \
{ \
  this->emit(re,v);                                \
}

#define CPI_EMIT( name ) \
{ \
  CPI_EMIT_REGISTER_FULL( name, CPI::Time::Emit::u, 1, CPI::Time::Emit::Transient); \
  CPI::Time::Emit::getSEmit().emit(re);                        \
}
#define CPI_EMIT_( name ) \
{ \
  CPI_EMIT_REGISTER_FULL( name, CPI::Time::Emit::u, 1, CPI::Time::Emit::Transient); \
  this->emit(re);           \
}

#define CPI_EMIT_STATE( name, state )                        \
{ \
  CPI_EMIT_REGISTER_FULL( name, CPI::Time::Emit::u, 1, CPI::Time::Emit::Value); \
  CPI::Time::Emit::getSEmit().emit(re,static_cast<CPI::OS::uint64_t>(state)); \
}
#define CPI_EMIT_STATE_( name, state )                \
{ \
  CPI_EMIT_REGISTER_FULL( name, CPI::Time::Emit::u, 1, CPI::Time::Emit::Value); \
  this->emit(re,static_cast<CPI::OS::uint64_t>(state)); \
}

#define CPI_EMIT_PVALUE_( p ) \
{ \
  CPI_EMIT_REGISTER_P( p ) \
  this->emit(re, p);        \
}

#define CPI_EMIT_PVALUE( p ) \
{ \
  CPI_EMIT_REGISTER_P( p ) \
  CPI::Time::Emit::getSEmit().emit(re, p);        \
}


#define CPI_EMIT_UINT64_( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,64,CPI::Time::Emit::Value); \
  this->emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}
#define CPI_EMIT_UINT64( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,64,CPI::Time::Emit::Value); \
  CPI::Time::Emit::getSEmit().emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}
#define CPI_EMIT_UINT32_( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,32,CPI::Time::Emit::Value); \
  this->emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}
#define CPI_EMIT_UINT32( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,32,CPI::Time::Emit::Value); \
  CPI::Time::Emit::getSEmit().emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}
#define CPI_EMIT_UINT16_( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,16,CPI::Time::Emit::Value); \
  this->emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}
#define CPI_EMIT_UINT16( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,16,CPI::Time::Emit::Value); \
  CPI::Time::Emit::getSEmit().emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}
#define CPI_EMIT_UINT8_( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,8,CPI::Time::Emit::Value); \
  this->emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}
#define CPI_EMIT_UINT8( name, value ) \
{ \
  CPI_EMIT_REGISTER_FULL(name,CPI::Time::Emit::u,8,CPI::Time::Emit::Value); \
  CPI::Time::Emit::getSEmit().emit(re,static_cast<CPI::OS::uint64_t>(value));                        \
}

#else

#define CPI_EMIT_REGISTER(  name )
#define CPI_EMIT_REGISTER_FULL(  name, dtype, width, etype )
#define CPI_EMIT_REGISTER_P( p )

#define CPI_EMIT_HERE
#define CPI_EMIT_HERE_

#define CPI_EMIT_REGISTERED( re,v )
#define CPI_EMIT_REGISTERED_( re,v )

#define CPI_EMIT( name )
#define CPI_EMIT_( name )

#define CPI_EMIT_STATE( name, state )
#define CPI_EMIT_STATE_( name, state )


#define CPI_EMIT_PVALUE( p )
#define CPI_EMIT_PVALUE_( p )

#define CPI_EMIT_UINT64_( name, value )
#define CPI_EMIT_UINT64( name, value )
#define CPI_EMIT_UINT32_( name, value )
#define CPI_EMIT_UINT32( name, value )
#define CPI_EMIT_UINT16_( name, value )
#define CPI_EMIT_UINT16( name, value )
#define CPI_EMIT_UINT8_( name, value )
#define CPI_EMIT_UINT8( name, value )

#endif




namespace CPI {

  namespace Time {

    class Emit {

    public:
      typedef CPI::OS::uint16_t EventId;
      typedef CPI::OS::uint16_t OwnerId;
      typedef CPI::OS::uint64_t Time;

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
                       EventType type=CPI::Time::Emit::Transient,
                       DataType dt=CPI::Time::Emit::u
                       );
        RegisterEvent( CPI::Util::PValue& pvstr );
        static int registerEvent( const char* event_name,
                           int width=1, 
                           EventType type=CPI::Time::Emit::Transient,
                           DataType dt=CPI::Time::Emit::u
                           );

        operator EventId () const {return m_eid;}

      private:
        EventId m_eid;

        CPI::OS::Mutex m_mutex;
        
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
        throw ( CPI::Util::EmbeddedException );

      /*
       * This constructor is used by a top-level traceable object.
       */
      Emit( TimeSource& ts, const char* class_name=NULL, 
                 const char* instance_name=NULL, QConfig* config=NULL )
        throw ( CPI::Util::EmbeddedException );

      /*
       * This constructor is used by sub-ordinate objects.
       */
      Emit( Emit* parent, const char* class_name=NULL, 
                 const char* instance_name=NULL, QConfig* config=NULL )
        throw ( CPI::Util::EmbeddedException ) ;
      ~Emit()
        throw ( );

      // Header utility methods
      OwnerId addHeader( Emit* t );

      // Trace method
      void emit( EventId id, CPI::OS::uint64_t v, EventTriggerRole role=NoTrigger );
      void emit( EventId id, EventTriggerRole role=NoTrigger );
      void emit( EventId id, CPI::Util::PValue& p, EventTriggerRole role=NoTrigger );

      static Emit& getSEmit();
      static void sEmit( EventId id, CPI::OS::uint64_t v, EventTriggerRole role=NoTrigger );
      static void sEmit( EventId id, EventTriggerRole role=NoTrigger );

      // Stop collecting events now
      void stop( bool globally = true );

      // Member access
      inline CPI::OS::uint32_t getLevel(){return m_level;};
      inline std::string& getClassName(){return m_className; }
      inline Emit* getParent(){return m_parent;}

      // Get Headers
      static Header& getHeader();
      // This mutex is used to protect the static header data
      static inline CPI::OS::Mutex& getGMutex() {
        static CPI::OS::Mutex m_g_mutex(true);
        return m_g_mutex;
      }

      // Shutdown, deletes all global resources 
      static void shutdown()
        throw ( );

    private:

      void pre_init( const char* class_name, 
                const char* instance_name, 
                QConfig* config )
        throw ( CPI::Util::EmbeddedException );

      void init()
        throw ( CPI::Util::EmbeddedException );

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
      CPI::OS::Mutex m_mutex;
      TimeSource*    m_ts;
    };


    /*
     * Base class for formatting the trace output.
     */
    class EmitFormatter {

    public:

      enum DumpFormat {
        CPIReadable,
        CPIRaw,
        VCDFormat
      };

      EmitFormatter( DumpFormat df=CPIReadable  );

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

#define CPI_TIME_ANALYZER_INLINE_VALID_USE__
#include <CpiTimeEmit_inlines.h>
#undef CPI_TIME_ANALYZER_INLINE_VALID_USE__


#endif
