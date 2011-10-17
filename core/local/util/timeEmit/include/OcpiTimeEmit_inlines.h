
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



/**
 * \file
 * \brief Time Analyzer classes
 *
 * Revision History:
 *
 *     08/19/2009 - John F. Miller
 *                  Initial version.
 */

#include <string>
#include <fstream>
#include <iostream>
#include <OcpiOsAssert.h>
#include <OcpiUtilDataTypesApi.h>

#ifndef OCPI_TIME_ANALYZER_INLINE_VALID_USE__
#error "You are not permitted to include this file standalone"
#endif

#define OCPI_TIME_EMIT_MULTI_THREADED
#ifdef OCPI_TIME_EMIT_MULTI_THREADED
#define AUTO_MUTEX(m) OCPI::Util::AutoMutex guard ( m, true ); 
#else
#define AUTO_MUTEX(m)
#endif

// Required to be defined here for inlines
namespace OCPI {

  namespace Time {
    union SValue {
      OCPI::OS::uint64_t uvalue;
      OCPI::OS::int64_t  ivalue;
      double            dvalue;
    };

    struct Emit::EventQEntry {
      Time     time;
      EventId  eid;
      OwnerId  owner;
      OCPI::OS::uint32_t   size;   // In bytes
      // payload goes here
    };

    struct Emit::EventQ {
      QConfig      config;
      EventQEntry*  start;
      OCPI::OS::uint8_t* base;
      OCPI::OS::uint8_t* end;
      EventQEntry* current;
      bool         full;
      bool         done;
      EventTriggerRole    role;
    EventQ():start(NULL),base(NULL),end(NULL),current(NULL),full(false),done(false),role(NoTrigger){}
      void allocate()
      {
        base = new uint8_t[config.size];
        start = (EventQEntry*) base;
        end   = base + config.size;
        memset(base,0,config.size);
      };
      ~EventQ() 
      {
	if (base)
	  delete [] base;
      }
    };

    struct Emit::HeaderEntry {
      std::string     className;
      std::string     instanceName;
      int             instanceId;
      int             parentIndex;   // Index into this vector
      std::string     outputPostFix;
      HeaderEntry( std::string& cn, std::string& in, int i, int pi  )
        :className(cn),instanceName(in),instanceId(i),parentIndex(pi){};
    };

    struct Emit::EventMap {
      EventId     id;
      std::string eventName;
      int         width;
      EventType   type;
      DataType dtype;
      EventMap( EventId pid, const char* en, int w, EventType t, DataType dt)
        :id(pid),eventName(en),width(w),type(t),dtype(dt){}
    };

    struct Emit::Header {
      bool                                 init;
      EventId                              nextEventId;
      Time                                 startTime;
      std::vector<HeaderEntry>             classDefs;
      std::vector<EventQ*>                 eventQ;
      std::vector<EventMap>                eventMap;
      bool                                 shuttingDown;
      bool                                 dumpOnExit;
      bool                                 traceCD;      // trace class construction/destruction
      EmitFormatter::DumpFormat            dumpFormat;
      std::string                          dumpFileName;
      std::fstream                         dumpFileStream;
      Header():init(false),nextEventId(1),shuttingDown(false),dumpOnExit(false){};
    };

  }
}

inline OCPI::Time::Emit::Time OCPI::Time::Emit::getTime()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  Time t = (tv.tv_sec * 1000 * 1000) + tv.tv_usec;
  return t;
};


inline void OCPI::Time::Emit::processTrigger( EventTriggerRole role ) {
  switch( role ) {
    
  case LocalQGroupTrigger:
    {
      if ( (m_q->role == LocalQGroupTrigger) || (m_q->role == GlobalQGroupTrigger)) {
        // nothing to do
        break;
      }
      else {
        m_q->role = LocalQGroupTrigger;
        m_q->current = NULL;
        m_q->full = false;
      }
    }
    break;
  case LocalQMasterTrigger:
    {
      m_q->role = LocalQMasterTrigger;
      m_q->current = NULL;
      m_q->full = false;
    }
    break;

  case GlobalQGroupTrigger:
  case GlobalQMasterTrigger:
    {


      std::vector<OCPI::Time::Emit::EventQ*>::iterator it;
      for( it=OCPI::Time::Emit::getHeader().eventQ.begin();
           it!=OCPI::Time::Emit::getHeader().eventQ.end(); it++ ) {  
    
      }
    }
    break;

  case NoTrigger:
    break;  // Nothing to do
  }
}

#define  ADJUST_CURRENT( m_q,size)  \
  if ( !m_q->current ) {  \
    m_q->current = m_q->start; \
  } \
  else { \
  if ( (((OCPI::OS::uint8_t*)m_q->current) + size ) >= m_q->end ) { \
    m_q->current = m_q->start; \
    m_q->full = true; \
  } \
}


#define INIT_EVENT( id, role, s ) \
  AUTO_MUTEX( m_mutex ); \
  if ( role != NoTrigger ) \
    processTrigger(role); \
  if ( m_q->done ) { \
    return; \
  } \
  ADJUST_CURRENT( m_q, s ); \
  m_q->current->size = s; \
  m_q->current->time  = getTime();        \
  m_q->current->eid   = id;                \
  m_q->current->owner = m_myId;



#define FINI_EVENT \
  m_q->current = reinterpret_cast<EventQEntry*>(((OCPI::OS::uint8_t*)m_q->current +  m_q->current->size )); \
  m_q->current++; \
  if ( (OCPI::OS::uint8_t*)m_q->current >= m_q->end ) {                        \
    m_q->full = true; \
    if ( m_q->config.stopWhenFull ) { \
      m_q->done = true; \
      return; \
    } \
    m_q->current = m_q->start; \
  }



inline void OCPI::Time::Emit::emit( OCPI::Time::Emit::EventId id, 
                                           OCPI::OS::uint64_t v,
                                           EventTriggerRole role)
{        
  //  INIT_EVENT(id, role, sizeof(OCPI::OS::uint64_t) );

  int size = sizeof(OCPI::OS::uint64_t);
  AUTO_MUTEX( m_mutex ); 
  if ( role != NoTrigger ) 
    processTrigger(role); 
  if ( m_q->done ) { 
    return; 
  } 

  if ( !m_q->current ) {  
    m_q->current = m_q->start; 
  } 
  else { 
    if ( (((OCPI::OS::uint8_t*)m_q->current) + size ) >= m_q->end ) { 
      m_q->current = m_q->start; 
      m_q->full = true; 
    } 
  }

  m_q->current->size = size; 
  m_q->current->time  = getTime();        
  m_q->current->eid   = id;                
  m_q->current->owner = m_myId;



  OCPI::OS::uint64_t* dp = (OCPI::OS::uint64_t*)(m_q->current + 1);
  *dp = v;


  m_q->current++; 
  m_q->current = reinterpret_cast<EventQEntry*>(((OCPI::OS::uint8_t*)m_q->current + size )); 
  if ( (OCPI::OS::uint8_t*)m_q->current >= m_q->end ) {                        
    m_q->full = true; 
    if ( m_q->config.stopWhenFull ) { 
      m_q->done = true; 
      return; 
    } 
    m_q->current = m_q->start; 
  }


  //  FINI_EVENT;
}

inline void OCPI::Time::Emit::emit( EventId id, OCPI::API::PValue& p, EventTriggerRole role )
{
  INIT_EVENT(id, role, sizeof(OCPI::OS::uint64_t) );

  OCPI::Time::SValue* dp = (OCPI::Time::SValue*)(m_q->current + 1);

  switch ( p.type ) {

  case OCPI::API::OCPI_Short:
    dp->ivalue = p.vShort;    
    break;
  case OCPI::API::OCPI_Long:
    dp->ivalue = p.vLong;
    break;
  case OCPI::API::OCPI_Char:
    dp->ivalue = p.vChar;
    break;    
  case OCPI::API::OCPI_LongLong:
    dp->ivalue = p.vLongLong;
    break;    
  case OCPI::API::OCPI_Bool:
    dp->uvalue = p.vBool;
    break;    
  case OCPI::API::OCPI_ULong:
    dp->uvalue = p.vULong;
    break;    
  case OCPI::API::OCPI_UShort:
    dp->uvalue = p.vUShort;
    break;    
  case OCPI::API::OCPI_ULongLong:
    dp->uvalue = p.vULongLong;
    break;    
  case OCPI::API::OCPI_UChar:
    dp->uvalue = p.vUShort;
    break;    
  case OCPI::API::OCPI_Double:
    dp->dvalue = p.vDouble;
    break;    
  case OCPI::API::OCPI_Float:
    dp->dvalue = p.vFloat;
    break;    

  case OCPI::API::OCPI_String:
    // NOTE:   NOT REALLY SURE WHAT TO DO WITH THIS YET !!
    //    memcpy( &m_q->current->uvalue, &p.vString, sizeof(OCPI::OS::uint64_t) );
    break;    

  case OCPI::API::OCPI_none:
  case OCPI::API::OCPI_Struct:
  case OCPI::API::OCPI_Type:
  case OCPI::API::OCPI_Enum:
  case OCPI::API::OCPI_scalar_type_limit:
    ocpiAssert(0);
  }


  FINI_EVENT;
}

inline void OCPI::Time::Emit::emit( OCPI::Time::Emit::EventId id,
                                           EventTriggerRole role )
{        
  INIT_EVENT(id, role, sizeof(OCPI::OS::uint64_t) );
  FINI_EVENT;
}

namespace {
  class SEmit : public OCPI::Time::Emit {
  public:
    SEmit()
      : OCPI::Time::Emit("Global"){}
    ~SEmit(){shutdown();}
  };
}

inline void OCPI::Time::Emit::sEmit( EventId id,
                                            EventTriggerRole role )
{
  getSEmit().emit( id, role );
}

inline void OCPI::Time::Emit::sEmit( EventId id, OCPI::OS::uint64_t v, EventTriggerRole role )
{
  getSEmit().emit( id, v, role );
}

inline std::ostream &
operator<< (std::ostream& out, OCPI::Time::EmitFormatter& t ) {
  t.formatDumpToStream(out);
  return out;
}


