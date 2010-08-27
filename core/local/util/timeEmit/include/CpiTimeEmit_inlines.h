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

#include <string>
#include <fstream>
#include <iostream>
#include <CpiOsAssert.h>
#include <CpiUtilPropertyType.h>

#ifndef CPI_TIME_ANALYZER_INLINE_VALID_USE__
#error "You are not permitted to include this file standalone"
#endif

#define CPI_TIME_EMIT_MULTI_THREADED
#ifdef CPI_TIME_EMIT_MULTI_THREADED
#define AUTO_MUTEX(m) CPI::Util::AutoMutex guard ( m, true ); 
#else
#define AUTO_MUTEX(m)
#endif

// Required to be defined here for inlines
namespace CPI {

  namespace Time {

    union SValue {
      CPI::OS::uint64_t uvalue;
      CPI::OS::int64_t  ivalue;
      double            dvalue;
    };

    struct Emit::EventQEntry {
      Time     time;
      EventId  eid;
      OwnerId  owner;
      CPI::OS::uint32_t   size;   // In bytes
      // payload goes here
    };

    struct Emit::EventQ {
      QConfig      config;
      EventQEntry*  start;
      CPI::OS::uint8_t* end;
      EventQEntry* current;
      bool         full;
      bool         done;
      EventTriggerRole    role;
      EventQ():start(NULL),current(NULL),full(false),done(false),role(NoTrigger){}
      void allocate()
      {
        start = (EventQEntry*) new CPI::OS::uint8_t[config.size];
        end   = (CPI::OS::uint8_t*)start + config.size;
        memset( start,0,config.size);
      };
      ~EventQ() 
      {
        delete [] start;
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

inline CPI::Time::Emit::Time CPI::Time::Emit::getTime()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  Time t = (tv.tv_sec * 1000 * 1000) + tv.tv_usec;
  return t;
};


inline void CPI::Time::Emit::processTrigger( EventTriggerRole role ) {
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


      std::vector<CPI::Time::Emit::EventQ*>::iterator it;
      for( it=CPI::Time::Emit::getHeader().eventQ.begin();
           it!=CPI::Time::Emit::getHeader().eventQ.end(); it++ ) {  
    
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
  if ( (((CPI::OS::uint8_t*)m_q->current) + size ) >= m_q->end ) { \
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
  m_q->current = reinterpret_cast<EventQEntry*>(((CPI::OS::uint8_t*)m_q->current +  m_q->current->size )); \
  m_q->current++; \
  if ( (CPI::OS::uint8_t*)m_q->current >= m_q->end ) {                        \
    m_q->full = true; \
    if ( m_q->config.stopWhenFull ) { \
      m_q->done = true; \
      return; \
    } \
    m_q->current = m_q->start; \
  }



inline void CPI::Time::Emit::emit( CPI::Time::Emit::EventId id, 
                                           CPI::OS::uint64_t v,
                                           EventTriggerRole role)
{        
  //  INIT_EVENT(id, role, sizeof(CPI::OS::uint64_t) );

  int size = sizeof(CPI::OS::uint64_t);
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
    if ( (((CPI::OS::uint8_t*)m_q->current) + size ) >= m_q->end ) { 
      m_q->current = m_q->start; 
      m_q->full = true; 
    } 
  }

  m_q->current->size = size; 
  m_q->current->time  = getTime();        
  m_q->current->eid   = id;                
  m_q->current->owner = m_myId;



  CPI::OS::uint64_t* dp = (CPI::OS::uint64_t*)(m_q->current + 1);
  *dp = v;


  m_q->current++; 
  m_q->current = reinterpret_cast<EventQEntry*>(((CPI::OS::uint8_t*)m_q->current + size )); 
  if ( (CPI::OS::uint8_t*)m_q->current >= m_q->end ) {                        
    m_q->full = true; 
    if ( m_q->config.stopWhenFull ) { 
      m_q->done = true; 
      return; 
    } 
    m_q->current = m_q->start; 
  }


  //  FINI_EVENT;
}

inline void CPI::Time::Emit::emit( EventId id, CPI::Util::PValue& p, EventTriggerRole role )
{
  INIT_EVENT(id, role, sizeof(CPI::OS::uint64_t) );

  CPI::Time::SValue* dp = (CPI::Time::SValue*)(m_q->current + 1);

  switch ( p.type ) {

  case CPI::Util::Prop::Scalar::CPI_Short:
    dp->ivalue = p.vShort;    
    break;
  case CPI::Util::Prop::Scalar::CPI_Long:
    dp->ivalue = p.vLong;
    break;
  case CPI::Util::Prop::Scalar::CPI_Char:
    dp->ivalue = p.vChar;
    break;    
  case CPI::Util::Prop::Scalar::CPI_LongLong:
    dp->ivalue = p.vLongLong;
    break;    
  case CPI::Util::Prop::Scalar::CPI_Bool:
    dp->uvalue = p.vBool;
    break;    
  case CPI::Util::Prop::Scalar::CPI_ULong:
    dp->uvalue = p.vULong;
    break;    
  case CPI::Util::Prop::Scalar::CPI_UShort:
    dp->uvalue = p.vUShort;
    break;    
  case CPI::Util::Prop::Scalar::CPI_ULongLong:
    dp->uvalue = p.vULongLong;
    break;    
  case CPI::Util::Prop::Scalar::CPI_UChar:
    dp->uvalue = p.vUShort;
    break;    
  case CPI::Util::Prop::Scalar::CPI_Double:
    dp->dvalue = p.vDouble;
    break;    
  case CPI::Util::Prop::Scalar::CPI_Float:
    dp->dvalue = p.vFloat;
    break;    

  case CPI::Util::Prop::Scalar::CPI_String:
    // NOTE:   NOT REALLY SURE WHAT TO DO WITH THIS YET !!
    //    memcpy( &m_q->current->uvalue, &p.vString, sizeof(CPI::OS::uint64_t) );
    break;    

  case CPI::Util::Prop::Scalar::CPI_none:
  case CPI::Util::Prop::Scalar::CPI_scalar_type_limit:
    cpiAssert(0);
  }


  FINI_EVENT;
}

inline void CPI::Time::Emit::emit( CPI::Time::Emit::EventId id,
                                           EventTriggerRole role )
{        
  INIT_EVENT(id, role, sizeof(CPI::OS::uint64_t) );
  FINI_EVENT;
}

namespace {
  class SEmit : public CPI::Time::Emit {
  public:
    SEmit()
      : CPI::Time::Emit("Global"){}
    ~SEmit(){shutdown();}
  };
}

inline void CPI::Time::Emit::sEmit( EventId id,
                                            EventTriggerRole role )
{
  getSEmit().emit( id, role );
}

inline void CPI::Time::Emit::sEmit( EventId id, CPI::OS::uint64_t v, EventTriggerRole role )
{
  getSEmit().emit( id, v, role );
}

inline std::ostream &
operator<< (std::ostream& out, CPI::Time::EmitFormatter& t ) {
  t.formatDumpToStream(out);
  return out;
}


