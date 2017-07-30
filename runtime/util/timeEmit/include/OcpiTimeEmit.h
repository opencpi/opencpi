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

#define OCPI_TIME_EMIT_SUPPORT



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

#define OCPI_EMIT_REGISTER_FULL_VAR(  name, dtype, width, etype, var )	\
  static OCPI::Time::Emit::RegisterEvent var(name,width,etype,dtype)

#define OCPI_EMIT_REGISTER_P( p )        \
  static OCPI::Time::Emit::RegisterEvent re(p)

// The following macros are used to emit events within a class that has inherited from Time::Emit
// They assume that they have access to "this" and they will preserve the class heirarchy information.

#define OCPI_EMIT_HERE                \
do { \
  OCPI_EMIT_REGISTER_FULL( __FILE__ "_line_"  TOSTRING(__LINE__), OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Transient); \
  OCPI::Time::Emit::getSEmit().emit(re);                        \
 } while(0)
#define OCPI_EMIT_HERE_                \
do { \
  OCPI_EMIT_REGISTER_FULL( __FILE__ "_line_"  TOSTRING(__LINE__), OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Transient); \
  this->emit(re);                \
 } while(0)

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
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Transient); \
  OCPI::Time::Emit::getSEmit().emit(re);                        \
} while(0)

#define OCPI_EMIT_( name ) \
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Transient); \
  this->emit(re);           \
 } while(0)

#define OCPI_EMIT__( name, c )			\
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Transient); \
  c->emit(re);								\
 } while(0)


#define OCPI_EMIT_STATE( name, state )                        \
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(state)); \
} while(0)
#define OCPI_EMIT_STATE_( name, state )                \
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(state)); \
 } while(0)
#define OCPI_EMIT_STATE__( name, state, c )	\
do { \
  OCPI_EMIT_REGISTER_FULL( name, OCPI::Time::Emit::DT_u, 1, OCPI::Time::Emit::Value); \
  c->emit(re,static_cast<OCPI::OS::uint64_t>(state));		\
 } while(0)

#define OCPI_EMIT_STATE_NR( re, state )                        \
do { \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(state)); \
} while(0)
#define OCPI_EMIT_STATE_NR_( re, state )                \
do { \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(state)); \
 } while(0)
#define OCPI_EMIT_STATE_NR__( re, state, c )	\
do { \
  c->emit(re,static_cast<OCPI::OS::uint64_t>(state));		\
 } while(0)

#define OCPI_EMIT_PVALUE_( p ) \
do { \
  OCPI_EMIT_REGISTER_P( p );			\
  this->emit(re, p);        \
 } while(0)

#define OCPI_EMIT_PVALUE( p ) \
do { \
  OCPI_EMIT_REGISTER_P( p );			   \
  OCPI::Time::Emit::getSEmit().emit(re, p);        \
} while(0)


#define OCPI_EMIT_UINT64_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,64,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT64( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,64,OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT32_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,32,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT32( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,32,OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT16_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,16,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT16( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,16,OCPI::Time::Emit::Value); \
  OCPI::Time::Emit::getSEmit().emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT8_( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,8,OCPI::Time::Emit::Value); \
  this->emit(re,static_cast<OCPI::OS::uint64_t>(value));                        \
} while(0)
#define OCPI_EMIT_UINT8( name, value ) \
do { \
  OCPI_EMIT_REGISTER_FULL(name,OCPI::Time::Emit::DT_u,8,OCPI::Time::Emit::Value); \
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
      typedef uint16_t EventId;
      typedef int16_t OwnerId; // -1 is sentinel
      typedef uint64_t Time;

      enum EventType {
        Transient,
        State,
        Value
      };

      // These types are used by the formatter class to determine how to 
      // display the event values.
      enum DataType {
        DT_u,   // unsigned
        DT_i,   // signed
        DT_d,   // double
        DT_c    // character
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
                       DataType dt=OCPI::Time::Emit::DT_u
                       );
        RegisterEvent( OCPI::API::PValue& pvstr );
        static Emit::EventId registerEvent( const char* event_name,
					    int width=1, 
					    EventType type=OCPI::Time::Emit::Transient,
					    DataType dt=OCPI::Time::Emit::DT_u
					    );

        operator EventId () const {return m_eid;}

      private:
        EventId m_eid;

      };

      class TimeSource;
      typedef  Time (*TickFunc)(TimeSource*);

      // This is the base class for the time source that gets used by the emit class for time stamping events.
      class TimeSource {
      public:
        TimeSource(TickFunc tf = NULL) : ticks(tf) {};
	virtual Time getTime();
	TickFunc ticks;
	virtual ~TimeSource(){}
      };

      // This class uses "gettimeofday" to get the time tag
      class SimpleSystemTime : public TimeSource {
      public:
	SimpleSystemTime();
        static Time getTimeOfDay();
        virtual ~SimpleSystemTime(){};
      private:
	static struct timespec m_init_tv;
	static bool m_init;
	static uint64_t myTicks( TimeSource *);
      };

      // This class uses both "gettimeofday" and the CPU free runnging clock to minimize the 
      // "call site" latency.
      class FastSystemTime : public TimeSource {
      public:
        FastSystemTime();
        Time getTimeOfDay();
	static Time getTimeS();
      private:
	int m_method;
	static struct timespec m_init_tv;
	static bool m_init;
	virtual ~FastSystemTime(){};
	static uint64_t myTicks( TimeSource * );
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

      Emit( Emit* parent, TimeSource& ts, const char* class_name=NULL, 
	    const char* instance_name=NULL, QConfig* config=NULL )
        throw ( OCPI::Util::EmbeddedException ) ;
      ~Emit()
        throw ( );

      // Header utility methods
      OwnerId addHeader( Emit* t );

      // Gets the default time source
      static TimeSource * getDefaultTS();

      // Trace method
      void emit( EventId id, OCPI::OS::uint64_t v, EventTriggerRole role=NoTrigger );
      void emitT( EventId id, OCPI::OS::uint64_t v, Time t, EventTriggerRole role=NoTrigger );
      void emit( EventId id, EventTriggerRole role=NoTrigger );
      void emitT( EventId id, Time t, EventTriggerRole role=NoTrigger );
      void emit( EventId id, OCPI::API::PValue& p, EventTriggerRole role=NoTrigger );
      void emitT( EventId id,  OCPI::API::PValue& p, Time t, EventTriggerRole role=NoTrigger );

      static Emit& getSEmit();
      static void sEmit( EventId id, OCPI::OS::uint64_t v, EventTriggerRole role=NoTrigger );
      static void sEmitT( EventId id, OCPI::OS::uint64_t v, Time t, EventTriggerRole role=NoTrigger );
      static void sEmit( EventId id, EventTriggerRole role=NoTrigger );
      static void sEmitT( EventId id, Time t, EventTriggerRole role=NoTrigger );

      // Stop collecting events now
      void stop( bool globally = true );
      static void endQue();

      // Member access
      inline OCPI::OS::uint32_t getLevel(){return m_level;};
      inline std::string& getClassName(){return m_className; }
      void setInstanceName( const char* name );
      inline Emit* getParent(){return m_parent;}

      // Determines if a category and sub-category qualitfy for an emit
      inline static bool qualifyCategory( uint32_t category, uint32_t sub_cat )
	{
	  if ( (m_categories & category) && ( m_sub_categories & sub_cat) ) 
	    return true;
	  else 
	    return false;	      
	}

      // Get Headers
      static Header& getHeader();

      // This mutex is used to protect the static header data
      static OCPI::OS::Mutex& getGMutex();

      // Shutdown, deletes all global resources 
      static void shutdown()
        throw ( );

    private:
      void 
	init_q ( QConfig* config, TimeSource * ts );

      void 
	parent_init( Emit* parent, 
		     const char* class_name, 
		     const char* instance_name, 
		     QConfig* config,
		     bool parent_q=false);

      void pre_init( const char* class_name, 
                const char* instance_name, 
                QConfig* config )
        throw ( OCPI::Util::EmbeddedException );

      void init()
        throw ( OCPI::Util::EmbeddedException );

      // static class methods
      Time getTime();
      Time getTicks();

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
      static uint32_t m_categories;
      static uint32_t m_sub_categories;
    };


    /*
     * Base class for formatting the trace output.
     */
    class EmitFormatter {

    public:

      enum DumpFormat {
        OCPIReadable,
        OCPIRAW,
        VCDFormat,
	CSVFormat,
      };

      EmitFormatter( DumpFormat df=OCPIReadable  );

      // Returns the "pretty" description of an event
      const char* getEventDescription( Emit::EventId id );

      // Returns the hierachical string describing the owning class
      static void formatOwnerString( Emit::OwnerId id, std::string& str, bool full_path=true );

      // Utility format methods
      std::string formatEventString ( Emit::EventQEntry& eqe,
                                      Emit::Time time_ref );
      std::string formatEventStringRAW ( Emit::EventQEntry& eqe );


      // Top level formatter methods
      std::ostream& formatDumpToStream( std::ostream& out );
      std::ostream& formatDumpToStreamRAW( std::ostream& out );

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
