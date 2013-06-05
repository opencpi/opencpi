
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

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <fasttime.h>
#include <OcpiTimeEmit.h>
#include <OcpiOsAssert.h>
#include <OcpiOsMisc.h>
#include "OcpiOsDataTypes.h"
#include "OcpiUtilDataTypes.h"
#include <iostream>

#define HANDLE_CLOCK_WRAP 1

namespace OA = OCPI::API;
namespace OU = OCPI::Util;

namespace OCPI {
  namespace Time {

    uint32_t Emit::m_categories = 0;
    uint32_t Emit::m_sub_categories = 0;

    extern "C" {
      int OcpiTimeARegister( char* signal_name )
      {
	return Emit::RegisterEvent::registerEvent( signal_name );
      }
      void OcpiEmit( unsigned sig ) 
      {
	Emit::getSEmit().emit((Emit::EventId)sig);
      }
    }


    static 
    void
    exitHandler()
    {
      static bool once=false;
      if ( ! once ) {
	Emit::endQue();
	once = true;
	EmitFormatter ef( Emit::getHeader().dumpFormat  );
	Emit::getHeader().dumpFileStream << ef;
      }
    }

    void
    Emit::
    init()
      throw ( OU::EmbeddedException )
    {
      AUTO_MUTEX(Emit::getGMutex());
      if ( getHeader().init == true ) {
	return;
      }
  
      const char *tmp;
      if ( ( tmp = getenv("OCPI_TIME_EMIT_TRACE_CD") ) != NULL ) {
	getHeader().traceCD = atoi(tmp);    
      }
      else {
	getHeader().traceCD = 0;
      }

      if ( ( tmp = getenv("OCPI_TIME_EMIT_DUMP_ON_EXIT") ) != NULL ) {
	getHeader().dumpOnExit = atoi(tmp);
	if ( getHeader().dumpOnExit ) {
	  atexit( exitHandler );
	}
      }
      else {
	getHeader().dumpOnExit = 1;
      }
      getHeader().dumpFormat =  EmitFormatter::OCPIRAW;
      if ( ( tmp = getenv("OCPI_TIME_EMIT_DUMP_FILENAME") ) != NULL ) {
	getHeader().dumpFileName = tmp;
      }
      else {
	if ( getHeader().dumpOnExit ) {
	  getHeader().dumpFileName = "./timeData.raw";
	}
      }

      if ( ( tmp = getenv("OCPI_TIME_EMIT_CAT") ) != NULL ) {
	m_categories = atoi(tmp);
      }

      if ( ( tmp = getenv("OCPI_TIME_EMIT_SUB_CAT") ) != NULL ) {
	m_sub_categories = atoi(tmp);
      }

      // Try to open the stream now so that we can report any errors before exit
      if ( getHeader().dumpOnExit ) {
	getHeader().dumpFileStream.open( getHeader().dumpFileName.c_str(), std::ios::out | std::ios::trunc | std::ios::binary );
	if ( ! getHeader().dumpFileStream.is_open() ) {
	  std::string err("Unable to open Time::Emit dump file ");
	  err += getHeader().dumpFileName;
	  throw OU::EmbeddedException( err.c_str() );
	}
      }

      getHeader().init = true;
    }
    
    void 
    Emit::
    init_q( QConfig * config,  TimeSource * t )
    {
      m_q = new EventQ;      
      m_ts = m_q->ts = t;
      if ( config ) {
	m_q->config = *config;
      }
      else {
	char* qsize;
	if ( (qsize = getenv("OCPI_TIME_EMIT_Q_SIZE") ) != NULL ) {
	  m_q->config.size = atoi(qsize);
	}
	else {
	  m_q->config.size  = 50 * 1024;
	}
	char* swf;
	if ( (swf = getenv("OCPI_TIME_EMIT_Q_SWF") ) != NULL ) {
	  m_q->config.stopWhenFull = atoi(swf);
	}
	else {
	  m_q->config.stopWhenFull = false;
	}
      }
      m_q->allocate();
      getHeader().eventQ.push_back( m_q );
    }

    void 
    Emit::
    pre_init( const char* class_name, 
              const char* instance_name, 
              QConfig* config )
      throw ( OU::EmbeddedException )
    {
      if ( class_name ) {
	m_className = class_name;
      }
      if ( instance_name ) {
	m_instanceName = instance_name;
      }
      m_parentIndex = -1;
      m_myId = addHeader( this );
      init_q( config, m_ts );
    }

    OCPI::OS::Mutex& 
    Emit::getGMutex() 
    {
      return *getHeader().g_mutex;
    }

    Emit::TimeSource * 
    Emit::
    getDefaultTS()
    {
      return Emit::getHeader().ts;
    }

    Emit::
    Emit( TimeSource& ts, const char* class_name, 
	  const char* instance_name, QConfig* config )
      throw ( OU::EmbeddedException )
      : m_parent(NULL), m_q(NULL), m_ts(NULL)
    {
      AUTO_MUTEX(Emit::getGMutex() );
      m_ts = &ts;
      pre_init( class_name, instance_name, config );
      init();
      if ( getHeader().traceCD ) {
	OCPI_EMIT_("Object Exists");
      }
    }

    Emit::
    Emit( const char* class_name, 
	  const char* instance_name, 
	  QConfig* config )
      throw ( OU::EmbeddedException )
      : m_level(1),m_parent(NULL), m_q(NULL), m_ts(NULL)
    {
      AUTO_MUTEX(Emit::getGMutex() );
      m_ts = getDefaultTS();
      pre_init( class_name, instance_name, config );
      init();
      if ( getHeader().traceCD ) {
	OCPI_EMIT_("Object Exists");
      }
    }


    void 
    Emit::
    parent_init( Emit* parent, 
		 const char* class_name, 
		 const char* instance_name, 
		 QConfig* config,
		 bool parent_q )
    {
      if ( class_name ) {
	m_className = class_name;
      }
      if ( instance_name ) {
	m_instanceName = instance_name;
      }
      m_level = m_parent->m_level + 1;
      m_parentIndex = parent->m_myId;
      m_myId = addHeader( this );
      if ( !parent_q ) {
	init_q( config, m_ts );
      }
      else {
	m_q = m_parent->m_q;
      }

      init();

      if (   getHeader().traceCD ) {
	OCPI_EMIT_("Object Exists");
      }
    }


    Emit::
    Emit( Emit* parent, 
	  const char* class_name, 
	  const char* instance_name, 
	  QConfig* config )
      throw ( OU::EmbeddedException )
      :m_parent(parent), m_q(NULL), m_ts(NULL)
    {
      AUTO_MUTEX(Emit::getGMutex());
      m_ts = getDefaultTS();
      parent_init(parent,class_name,instance_name,config,config?true:false);
    }


    Emit::
    Emit( Emit* parent, 
	  TimeSource &ts,
	  const char* class_name, 
	  const char* instance_name, 
	  QConfig* config )
      throw ( OU::EmbeddedException )
      :m_parent(parent), m_q(NULL), m_ts(NULL)
    {
      AUTO_MUTEX(Emit::getGMutex());
      init_q( config, &ts );
      parent_init(parent,class_name,instance_name,NULL,false);
    }


    void 
    Emit::
    stop( bool globally )
    {
      AUTO_MUTEX(Emit::getGMutex());
  
      if ( globally ) {
	std::vector<EventQ*>::iterator it;    
	for( it=Emit::getHeader().eventQ.begin();
	     it!=Emit::getHeader().eventQ.end(); it++ ) {
	  (*it)->done = true;
	}
      }
      else {
	m_q->done = false;
      }
    }


    void 
    Emit::
    endQue()
    {
      AUTO_MUTEX(Emit::getGMutex());
  
      std::vector<EventQ*>::iterator it;    
      for( it=Emit::getHeader().eventQ.begin();
	   it!=Emit::getHeader().eventQ.end(); it++ ) {
	(*it)->done = true;
	(*it)->gTime.stopTime  = (*it)->ts->getTime();
	(*it)->gTime.stopTicks = (*it)->ts->ticks( (*it)->ts );
      }

    }



    Emit::OwnerId 
    Emit::
    addHeader( Emit* t ) 
    {
      // mutex protected at call site
      int instance;
      getHeaderInfo( t, instance );
      // Add the header for this class/instance
      OwnerId myIndex = (OwnerId)(getHeader().classDefs.size());
      getHeader().classDefs.push_back( HeaderEntry(m_className, m_instanceName, instance, (EventId)m_parentIndex ) );
      return myIndex;
    }

    void 
    Emit::
    setInstanceName( const char* name )
    {
      m_instanceName = name;
      getHeader().classDefs[m_myId].instanceName = name;
    }

    void 
    Emit::
    getHeaderInfo( Emit* t, 
		   int& instance  ) 
    {
      AUTO_MUTEX(Emit::getGMutex());
      std::vector<HeaderEntry>::iterator it;
      instance=1;
      for ( it=getHeader().classDefs.begin(); it != getHeader().classDefs.end(); it++ ) {
	if ( t->m_parent != NULL ) {
	  if ( (*it).className == t->m_className ) {
	    instance++;
	  }          
	}
	else {
	  if ( (*it).className == t->m_className  ) {
	    instance++;
	  }          
	}
      }
    }


    bool 
    Emit::
    isChild( Emit::OwnerId id ) {

      AUTO_MUTEX(Emit::getGMutex());
      if ( id == -1 || (size_t)id >= getHeader().classDefs.size() ) {
	return false;
      }
      Emit::HeaderEntry& child = getHeader().classDefs[id];
      if ( child.parentIndex == -1 ) {
	return false;
      }
      else if ( child.parentIndex == m_myId ) {
	return true;
      }
      else {
	return isChild( child.parentIndex );
      }
      return false;
    }

    Emit::~Emit()
      throw ()
    {

    }


    static Emit::Header * g_header = NULL;
    Emit::Header& Emit::getHeader() {
      if ( g_header == NULL ) {
	g_header = new Header;
      }
      return *g_header;
    }

    void
    Emit::
    shutdown()
      throw()
    {

      if ( getHeader().dumpOnExit && !getHeader().shuttingDown) {
	exitHandler();
      }

      try {
	getHeader().shuttingDown = true;
	delete g_header;
      }
      catch ( ... ) {
	// Ignore
      }
      g_header = NULL;
    }




    Emit::EventId 
    Emit::RegisterEvent::
    registerEvent( const char* event_name, int width,
		   EventType type,
		   DataType dtype)
    {
      EventId e;
      AUTO_MUTEX(Emit::getGMutex());
      // Make sure this event does not already exist
      std::vector<EventMap>::iterator it;
      for( it =Emit::getHeader().eventMap.begin();
	   it != Emit::getHeader().eventMap.end();
	   it++ ) {
	if ( (*it).eventName == event_name ) {
	  e = (*it).id;
	  return e;
	}
      }
      e = getHeader().nextEventId++;
      Emit::getHeader().eventMap.push_back( EventMap(e,event_name,width,type,dtype) );
      return e;
    }

    Emit::RegisterEvent::RegisterEvent( const char* event_name, int width,
					EventType type,
					DataType dtype)
    {
      m_eid = registerEvent( event_name, width, type, dtype);
    }



    Emit::RegisterEvent::RegisterEvent( OCPI::API::PValue& p )
    {
      AUTO_MUTEX(Emit::getGMutex());
      m_eid = getHeader().nextEventId++;
      int width = OU::baseTypeSizes[p.type];
      DataType dtype=Emit::i;
      switch( p.type ){
      case OA::OCPI_Short:
      case OA::OCPI_Long:
      case OA::OCPI_Char:
      case OA::OCPI_LongLong:
	dtype = Emit::i;
	break;

      case OA::OCPI_Bool:
      case OA::OCPI_ULong:
      case OA::OCPI_UShort:
      case OA::OCPI_ULongLong:
      case OA::OCPI_UChar:
	dtype = Emit::u;
	break;

      case OA::OCPI_Double:
      case OA::OCPI_Float:
	dtype = Emit::d;
	break;

      case OA::OCPI_String:
	dtype = Emit::c;
	break;

      case OA::OCPI_none:
      case OA::OCPI_Struct:
      case OA::OCPI_Enum:
      case OA::OCPI_Type:
      case OA::OCPI_scalar_type_limit:
	ocpiAssert(0);
	break;


      }
      Emit::getHeader().eventMap.push_back( EventMap(m_eid,p.name,width,
						     Emit::Value, dtype
						     ) );
    }

    EmitFormatter::EmitFormatter( DumpFormat format)
      :m_dumpFormat(format)
    {

    };

    const char* EmitFormatter::getEventDescription( Emit::EventId id ) {
      AUTO_MUTEX(Emit::getGMutex());
      std::vector<Emit::EventMap>::iterator it;
      for ( it=m_traceable->getHeader().eventMap.begin(); 
	    it!=m_traceable->getHeader().eventMap.end(); it++) {
	if ( (*it).id == id ) {
	  return (*it).eventName.c_str();
	}
      }
      return NULL;
    }

    static Emit::EventMap* getEventMap( Emit::EventQEntry* e ) 
    {
      AUTO_MUTEX(Emit::getGMutex());
      std::vector<Emit::EventMap>::iterator it;
      for ( it=Emit::getHeader().eventMap.begin();
	    it != Emit::getHeader().eventMap.end(); it++ ) {
	if ( (*it).id == e->eid ) {
	  return &(*it);
	}
      }  
      return NULL;
    }

    static inline Emit::EventQEntry* getNextEntry( Emit::EventQEntry * ce, Emit::EventQ * q )
    {
      Emit::EventQEntry * ne = reinterpret_cast<Emit::EventQEntry *>( 
								     ( (uint8_t*)((uint8_t*)ce + sizeof(Emit::EventQEntry) + ce->size) ));

      // Deal with wrap for the variable length payload
      uint8_t * end = (uint8_t*)q->end;
      if ( ((uint8_t*)ne>=end-sizeof(Emit::EventQEntry)) || ((uint8_t*)ne+ne->size)>=end) {
	ne = q->start;
      }
      return ne;
    }



    Emit& 
    Emit::
    getSEmit() 
    {
      static SEmit t;
      return t;
    }



    std::ostream& EmitFormatter::formatDumpToStreamRAW( std::ostream& out ) 
    {
      AUTO_MUTEX(Emit::getGMutex());

      // Now do the timed events
      std::vector<Emit::EventQ*>::iterator it;
      for( it=Emit::getHeader().eventQ.begin();
	   it!=Emit::getHeader().eventQ.end(); it++ ) {
	Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
	Emit::EventQEntry* begin = qe;
	do {
	  Emit::EventMap* emap = getEventMap( qe );
	  if ( !emap ) {
	    qe = getNextEntry( qe, (*it) );
	    continue;  // This can occur on wrap
	  }
	  if ( qe->time_ticks ) {
	    out << qe->eid << "," << qe->owner << "," << emap->dtype  << "," << (*it)->calcGTime( qe->time_ticks );
	    if ( Emit::getHeader().eventMap[qe->eid].type != Emit::Transient ) {
	      SValue* d = (SValue*)(qe + 1);
	      switch ( emap->dtype ) {
	      case Emit::u:
		out << "," << d->uvalue << std::endl;
		break;
	      case Emit::i:
		out << "," << d->ivalue << std::endl;
		break;
	      case Emit::c:
		out << "," << d->cvalue << std::endl;	  
		break;
	      case Emit::d:
		out << "," << d->dvalue << std::endl;	  
		break;
	      }      
	    }
	    else {
	      out << ",0" << std::endl;	  
	    } 
	  }
	  qe = getNextEntry( qe, (*it) );
	}  while( (qe!=begin) && qe->size );
      }


      // Descriptors
      out << "<EventData>" << std::endl;
      {
	out << "  <Descriptors>" << std::endl;
	std::vector<Emit::EventMap>::iterator it;
	for ( it=Emit::getHeader().eventMap.begin(); it!=Emit::getHeader().eventMap.end();  it++ ) {
	  std::string tn((*it).eventName);
	  std::replace(tn.begin(),tn.end(),' ','_');
	  std::string owner;
	  out << "    " << "<Class id=\"" << (*it).id << "\" description=\"" << getEventDescription( (*it).id ) 
	      << "\" etype=\"" << (*it).type << "\" width=\"" << (*it).width << "\"" 
	      << " dtype=\"" << (*it).dtype << "\""
	      << "/>" << std::endl;
	}
	out << "  </Descriptors>" << std::endl;
      }

      // Owners
      {
	out << "  <Owners>" << std::endl;
	for (Emit::EventId n=0; n<Emit::getHeader().classDefs.size(); n++ ) {
	  std::string owner;
	  formatOwnerString(n, owner, false);      
	  out << "    <Owner id=\"" << n << "\" name=\"" << owner << "\"" << " parent=\"" << Emit::getHeader().classDefs[n].parentIndex <<
	    "\"/>" << std::endl;
	}
	out << "  </Owners>" << std::endl;    
      }
      out << "</EventData>" << std::endl;

      return out;  
    }



    std::ostream& EmitFormatter::formatDumpToStream( std::ostream& out ) 
    {
      return formatDumpToStreamRAW(out);
    }
      

    void EmitFormatter::formatOwnerString( Emit::OwnerId id, std::string& str, bool full_path ) {
      if ( id == -1 || (size_t)id >= Emit::getHeader().classDefs.size() ) {
	return;
      }

      if ( full_path && Emit::getHeader().classDefs[id].parentIndex != -1 ) {
	formatOwnerString( Emit::getHeader().classDefs[id].parentIndex, str );
      }
      if ( !str.empty() ) str.append("::");
      if ( Emit::getHeader().classDefs[id].className != "" ) {
	str.append(  Emit::getHeader().classDefs[id].className + ":" );
      }
      else {
	str.append( "Class:" );
      }
      if ( Emit::getHeader().classDefs[id].instanceName != "" ) {
	str.append(  Emit::getHeader().classDefs[id].instanceName + ":" );          
      }
      else {
	str.append(  ":" );          
      }
      char buf[10];
      sprintf(buf,"%d",Emit::getHeader().classDefs[id].instanceId );
      str.append(buf);

    }

    struct timespec Emit::SimpleSystemTime::m_init_tv;
    bool  Emit::SimpleSystemTime::m_init=false;
    Emit::SimpleSystemTime::
    SimpleSystemTime()
    {
      if ( !m_init ) {
	ticks = myTicks;
	m_init_tv.tv_sec =0;
	m_init_tv.tv_nsec =0;
	clock_gettime(CLOCK_REALTIME, &m_init_tv );
	m_init = true;
      }
    }

    Emit::Time 
    Emit::SimpleSystemTime::
    getTimeOfDay( )
    {
      struct timespec tv;
      Time t;
      clock_gettime(CLOCK_REALTIME, &tv );
#ifdef HANDLE_CLOCK_WRAP
      static time_t delta=0;
      if ( m_init_tv.tv_sec > tv.tv_sec ) { // we wrapped, this handles 1 edge consition only
	time_t d = ~0;
	delta = d - m_init_tv.tv_sec;
      }
      else {
	t = ((Time) (tv.tv_sec - m_init_tv.tv_sec)) * 1000000000 +
	  (tv.tv_nsec - m_init_tv.tv_nsec);
	return t;
      }
      t = ((Time) (tv.tv_sec + delta)) * 1000000000 +
	(tv.tv_nsec - m_init_tv.tv_nsec);
#else
      t = ((Time) (tv.tv_sec - m_init_tv.tv_sec)) * 1000000000 +
	(tv.tv_nsec - m_init_tv.tv_nsec);
#endif
      return t;  
    }


    Emit::Time 
    Emit::SimpleSystemTime::
    myTicks( TimeSource * )
    {
      return fasttime_getticks();
    }



    Emit::FastSystemTime::
    FastSystemTime()
    {
      long int wait_time;
      int result;
      fasttime_statistics_t stats;
      struct timespec tp_fast, tp_actual;
      ticks = myTicks;

      m_method = fasttime_init_context(NULL, 
				       FASTTIME_METHOD_CLIENT | FASTTIME_METHOD_DAEMON);

      switch (m_method & ~FASTTIME_METHOD_SYSTEM)
	{
        case FASTTIME_METHOD_CLIENT:
#ifndef NDEBUG
	  printf("Using client calibration");
#endif
	  break;

        case FASTTIME_METHOD_DAEMON:
#ifdef NDEBUG
	  printf("Using daemon calibration");
#endif
	  break;

        default:
	  printf("Error in init; quitting\n");
	  ocpiAssert("Fasttime method not supported"==0);
	}

      /* Check availability */
      while (fasttime_getstatistics(NULL, &stats) != 0) {
	ocpiDebug("Waiting for fasttime to warm up !!");
	OCPI::OS::sleep( 1000 );
      };


      if (!stats.ready)
	{
	  clock_gettime(CLOCK_REALTIME, &tp_actual );
	  wait_time = stats.ready_time - tp_actual.tv_sec;
	  if (wait_time > 0)
	    {
	      ocpiDebug("Waiting %lu secs for fasttime to get ready...", wait_time);
	      OCPI::OS::sleep(wait_time*1000);
	    }
	}
    

      do {
	clock_gettime(CLOCK_REALTIME, &tp_actual ); 
	result = fasttime_gettime(&tp_fast);  
      } while (result);

#ifdef PRINT_ACCURACY
      printf("Check accuracy:\n");
      printf(" Fast:   %lu secs, %lu nsecs\n", tp_fast.tv_sec, tp_fast.tv_nsec);
      printf(" Actual: %lu secs, %lu nsecs\n", tp_actual.tv_sec, tp_actual.tv_nsec); 
      printf(" Delta = %lu,%lu\n", tp_actual.tv_sec-tp_fast.tv_sec, tp_actual.tv_nsec-tp_fast.tv_nsec);
#endif

      if ( ! m_init ) {
	m_init_tv.tv_sec =0;
	m_init_tv.tv_nsec =0;
	fasttime_gettime(&m_init_tv);
	m_init = true;
      }

    }


    Emit::Time 
    Emit::FastSystemTime::
    myTicks( TimeSource *)
    {
      return fasttime_getticks();
    }


    Emit::Time 
    Emit::FastSystemTime::
    getTimeOfDay()
    {
      return getTimeS();
    }


    struct timespec Emit::FastSystemTime::m_init_tv;
    bool  Emit::FastSystemTime::m_init=false;
    Emit::Time 
    Emit::FastSystemTime::
    getTimeS()
    {
      struct timespec tv;
      Time t;
      fasttime_gettime(&tv);
#ifdef HANDLE_CLOCK_WRAP
      static time_t delta=0;
      if ( m_init_tv.tv_sec > tv.tv_sec ) { // we wrapped, this handles 1 edge consition only
	time_t d = ~0;
	delta = d - m_init_tv.tv_sec;
      }
      else {
	t = ((Time) (tv.tv_sec - m_init_tv.tv_sec)) * 1000000000 +
	  (tv.tv_nsec - m_init_tv.tv_nsec);
	return t;
      }
      t = ((Time) (tv.tv_sec + delta)) * 1000000000 +
	(tv.tv_nsec - m_init_tv.tv_nsec);
#else
      t = ((Time) (tv.tv_sec - m_init_tv.tv_sec)) * 1000000000 +
	(tv.tv_nsec - m_init_tv.tv_nsec);
#endif
      return t;  
    }


    Emit::Time 
    Emit::TimeSource::
    getTime()
    {
      return Emit::SimpleSystemTime::getTimeOfDay();
    }



  }

}
