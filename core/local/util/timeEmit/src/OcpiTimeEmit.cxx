
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
#include "OcpiOsDataTypes.h"
#include "OcpiUtilDataTypes.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;

namespace OCPI {
  namespace Time {

    extern "C" {
      int OcpiTimeARegister( char* signal_name )
      {
	return Emit::RegisterEvent::registerEvent( signal_name );
      }
      void OcpiEmit( int sig ) 
      {
	Emit::getSEmit().emit(sig);
      }
    }


static 
void
exitHandler()
{
  static bool once=false;
  if ( ! once ) {
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

  if ( ( tmp = getenv("OCPI_TIME_EMIT_DUMP_ON_EXIT") ) != NULL ) {
    getHeader().dumpOnExit = atoi(tmp);
    if ( getHeader().dumpOnExit ) {
      atexit( exitHandler );
    }
  }

  if ( ( tmp = getenv("OCPI_TIME_EMIT_DUMP_FORMAT") ) != NULL ) {
    if ( strcasecmp( tmp, "READABLE" ) == 0 ) {
      getHeader().dumpFormat = EmitFormatter::OCPIReadable;
    }
    else if ( strcasecmp( tmp, "RAW" ) == 0 ) {
      getHeader().dumpFormat =  EmitFormatter::OCPIRaw;
    }
    else if ( strcasecmp( tmp, "VCD" ) == 0 ) {
      getHeader().dumpFormat =  EmitFormatter::VCDFormat;
    }
  }

  if ( ( tmp = getenv("OCPI_TIME_EMIT_DUMP_FILENAME") ) != NULL ) {
    getHeader().dumpFileName = tmp;
  }
  else {
    if ( getHeader().dumpOnExit ) {
      getHeader().dumpFileName = "./timeData.dmp";
    }
  }

  // Try to open the stream now so that we can report any errors before exit
  if ( getHeader().dumpOnExit ) {
    getHeader().dumpFileStream.open( getHeader().dumpFileName.c_str(), std::ios::out | std::ios::trunc );
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

  m_q = new EventQ;
  getHeader().eventQ.push_back( m_q );
  if ( config ) {
    m_q->config = *config;
  }
  else {
    char* qsize;
    if ( (qsize = getenv("OCPI_TIME_EMIT_Q_SIZE") ) != NULL ) {
      m_q->config.size = atoi(qsize);
    }
    else {
      m_q->config.size  = 256 * 1024;
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
  pre_init( class_name, instance_name, config );
  init();
  if ( getHeader().traceCD ) {
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
  if ( class_name ) {
    m_className = class_name;
  }
  if ( instance_name ) {
    m_instanceName = instance_name;
  }
  m_level = m_parent->m_level + 1;
  m_parentIndex = parent->m_myId;
  m_myId = addHeader( this );
  if ( config ) {
    m_q = new EventQ;
    getHeader().eventQ.push_back( m_q );
    m_q->config = *config;
    m_q->allocate();
  }
  else {
    m_q = m_parent->m_q;
  }

  init();

  if (   getHeader().traceCD ) {
    OCPI_EMIT_("Object Exists");
  }
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



Emit::OwnerId 
Emit::
addHeader( Emit* t ) 
{
  // mutex protected at call site
  int instance;
  getHeaderInfo( t, instance );
  // Add the header for this class/instance
  int myIndex = getHeader().classDefs.size();
  getHeader().classDefs.push_back( HeaderEntry(m_className, m_instanceName, instance, m_parentIndex ) );
  return myIndex;
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
  if ( id >= getHeader().classDefs.size() ) {
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

  if (   getHeader().traceCD ) {
    if(!getHeader().shuttingDown) {
      OCPI_EMIT_("Object Terminated");
    }
  }
}

void
Emit::
shutdown()
  throw()
{
  AUTO_MUTEX(Emit::getGMutex());

  if ( getHeader().dumpOnExit ) {
    exitHandler();
  }

  try {
    getHeader().shuttingDown = true;
    std::vector<EventQ*>::iterator it;
    int c=getHeader().eventQ.size();
    it=getHeader().eventQ.begin(); 
    while ( c ) {
      delete (*it);
      c--;
      it++;
    }
    getHeader().eventQ.clear();
    getHeader().shuttingDown = true;
  }
  catch ( ... ) {
    // Ignore
  }
}




int 
Emit::RegisterEvent::
registerEvent( const char* event_name, int width,
               EventType type,
               DataType dtype)
{
  int e;
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

std::string EmitFormatter::formatEventString ( Emit::EventQEntry& eqe, 
                                                                  Emit::Time time_ref ) 
{
  std::string str;
  const char* ed =  getEventDescription(eqe.eid);
  char buf[128];
  if ( ed ) {
    sprintf(buf,"%-6d   %lld \"%-20s\" ", eqe.eid, (long long)(eqe.time-time_ref), ed );
  }
  else {
    sprintf(buf,"%-6d   %lld \"%-20s\" ", eqe.eid, (long long)(eqe.time-time_ref), "" );
  }
  str.append(buf);

  return str;
}      

std::string EmitFormatter::formatEventStringRaw ( Emit::EventQEntry& eqe, 
                                                                  Emit::Time time_ref ) 
{
  std::string str;
  const char* ed =  getEventDescription(eqe.eid);
  char buf[128];
  if ( ed ) {
    sprintf(buf,"%d,%lld,\"%s\"", eqe.eid,(long long)(eqe.time-time_ref), ed );
  }
  else {
    sprintf(buf,"%d,%lld,\"%s\"", eqe.eid, (long long)(eqe.time-time_ref), "" );
  }
  str.append(buf);
  return str;
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
  ( void ) q;
  Emit::EventQEntry * ne = reinterpret_cast<Emit::EventQEntry *>( 
               ( (OCPI::OS::uint8_t*)((OCPI::OS::uint8_t*)ce + sizeof(Emit::EventQEntry) + ce->size) ));
  return ne;
}



Emit& 
Emit::
getSEmit() 
{
  static SEmit t;
  return t;
}



// NOT mutex protected, the caller needs to handle this !!
Emit::Time getStartTime() 
{
  std::vector<Emit::EventQ*>::iterator it;
  Emit::Time time=0;
  for( it=Emit::getHeader().eventQ.begin();
       it!=Emit::getHeader().eventQ.end(); it++ ) {

    Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
    while( qe && qe->size ) {

      if ( time == 0 ) {
        time = qe->time;
      }    
      if ( time > qe->time ) {
        time = qe->time;
      }
      qe = getNextEntry( qe, (*it) );
    }

  }
  return time;
}


std::ostream& EmitFormatter::formatDumpToStreamRaw( std::ostream& out ) 
{
  AUTO_MUTEX(Emit::getGMutex());

  std::vector<Emit::EventQ*>::iterator it;
  for( it=Emit::getHeader().eventQ.begin();
       it!=Emit::getHeader().eventQ.end(); it++ ) {

    Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
    Emit::Time time_ref = getStartTime();

    while( qe && qe->size ) {

      std::string str = formatEventStringRaw( *qe, time_ref );
      out << str.c_str() << ",";
      std::string ostr;
      formatOwnerString( qe->owner, ostr );
      out << ostr.c_str() << ",";

      Emit::EventMap* emap = getEventMap( qe );
      SValue* d = (SValue*)(qe + 1);

      switch ( emap->dtype ) {
      case Emit::u:
        out << d->uvalue  << std::endl;
        break;
      case Emit::i:
        out << d->ivalue  << std::endl;
        break;
      case Emit::c:
        out << d->ivalue  << std::endl;
        break;
      case Emit::d:
        out << d->dvalue  << std::endl;
        break;
      }
      qe = getNextEntry( qe, (*it) );
    }

  }
  return out;  
}

Emit::Header& Emit::getHeader() {
  static Header h;
  return h;
}


std::ostream& EmitFormatter::formatDumpToStreamReadable( std::ostream& out ) 
{
  AUTO_MUTEX(Emit::getGMutex());
  std::vector<Emit::EventQ*>::iterator it;
  for( it=Emit::getHeader().eventQ.begin();
       it!=Emit::getHeader().eventQ.end(); it++ ) {

    Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
    Emit::Time time_ref = getStartTime();
    while( qe && qe->size ) {

      std::string str = formatEventString( *qe, time_ref );
      out << str.c_str() << "     ";
      int mlen = 60 - str.size();
      for ( int n=0; n<mlen; n++) out << "";
   
      std::string ostr;
      formatOwnerString( qe->owner, ostr );
      out << ostr.c_str() << "  ";

      Emit::EventMap* emap = getEventMap( qe );
      SValue* d = (SValue*)(qe + 1);

      switch ( emap->dtype ) {
      case Emit::u:
        out << d->uvalue  << std::endl;
        break;
      case Emit::i:
        out << d->ivalue  << std::endl;
        break;
      case Emit::c:
        out << d->ivalue  << std::endl;
        break;
      case Emit::d:
        out << d->dvalue  << std::endl;
        break;
      }
      qe = getNextEntry( qe, (*it) );
    }

  }
  return out;
}

std::ostream& EmitFormatter::formatDumpToStream( std::ostream& out ) 
{
  switch ( m_dumpFormat ) {
  case OCPIReadable:
    return formatDumpToStreamReadable(out);
    break;
          
  case OCPIRaw:
    return formatDumpToStreamRaw(out);
    break;

  case VCDFormat:
    return formatDumpToStreamVCD(out);
    break;
  }

  return out;
}
      

void EmitFormatter::formatOwnerString( Emit::OwnerId id, std::string& str, bool full_path ) {
  if ( id >= Emit::getHeader().classDefs.size() ) {
    return;
  }
  
  if ( full_path && Emit::getHeader().classDefs[id].parentIndex != -1 ) {
    formatOwnerString( Emit::getHeader().classDefs[id].parentIndex, str );
  }

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
  sprintf(buf,"%d::",Emit::getHeader().classDefs[id].instanceId );
  str.append(buf);

}

namespace {
  struct ecmp
  {
    bool operator()(const Emit::EventId e1, const  Emit::EventId e2) const
    {
      return e1 < e2;
    }
  };
}

#define SYMSTART 33
#define SYMEND   126
#define SYMLEN (SYMEND-SYMSTART)
void getVCDVarSyms( Emit& t,
                           std::map<Emit::EventId,std::string,ecmp> & varsyms )
{
  unsigned int n;
  char syms[SYMLEN];
  for( n=0; n<SYMLEN; n++)syms[n]=static_cast<char>(n+SYMSTART);
  std::vector<Emit::EventMap>::iterator it;
  int scount=0;
  int * sindex = new int[t.getHeader().eventMap.size()/SYMLEN+1];
  std::auto_ptr<int> del(sindex);
  for ( n=0;n<t.getHeader().eventMap.size()/SYMLEN+1;n++)sindex[n]=0;

  for ( it=t.getHeader().eventMap.begin(); it!=t.getHeader().eventMap.end(); it++ ) {

#ifndef NDEBUG
    printf("Event name = %s\n", (*it).eventName.c_str() );
#endif    

    unsigned int ccount = scount/SYMLEN;
    std::string sym;
    for ( n=0; n<=ccount; n++ ) {
      sym += syms[sindex[n]%SYMLEN];
      sindex[n]++;
      scount++;
    }
    varsyms[(*it).id] = sym;

#ifndef NDEBUG
    printf("Adding symbol %s at index %d\n", sym.c_str(), (*it).id );
#endif

  }



}

static bool eventProducedBy( Emit::EventId event, Emit::OwnerId owner )
{
  std::vector<Emit::EventQ*>::iterator it;
  for( it=Emit::getHeader().eventQ.begin();
       it!=Emit::getHeader().eventQ.end(); it++ ) {
    Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
    while( qe && qe->size ) {
      if ( (event == qe->eid ) && (owner == qe->owner ) ) {
        return true;
      }
      qe = getNextEntry( qe, (*it) );
    }
  }
  return false;
}

struct EventInstance {
  Emit::OwnerId     owner;
  Emit::EventId     id;
  std::string                       sym;
  EventInstance(Emit::OwnerId     o,
                Emit::EventId     i,
                std::string                       s):owner(0),id(i),sym(s)
  {
    ( void ) o;
  }

};



static void dumpVCDScope( std::ostream& out, Emit::OwnerId owner,
                          std::map< Emit::EventId, std::string, ecmp > & varsyms,
                          std::vector<EventInstance> & allEis )
{
    std::string pname;
    EmitFormatter::formatOwnerString( owner, pname, false );
    std::replace(pname.begin(),pname.end(),' ','_');
    out << "$scope module " << pname.c_str() << " $end" << std::endl; 
    Emit::HeaderEntry & he = Emit::getHeader().classDefs[owner];
    for( int n=0; n<=owner/SYMLEN; n++) he.outputPostFix=static_cast<char>((SYMSTART+owner)%SYMLEN);
    // Dump the variables for this object
    std::vector<Emit::EventMap>::iterator it;
    for ( it=Emit::getHeader().eventMap.begin();
          it!=Emit::getHeader().eventMap.end();  it++ ) {
      if ( eventProducedBy( (*it).id, owner ) ) {
        std::string tn((*it).eventName);
        std::replace(tn.begin(),tn.end(),' ','_');
        out << "$var reg " << (*it).width << " " << varsyms[(*it).id] << he.outputPostFix.c_str() <<
          " " <<  tn.c_str() << " $end" << std::endl;

        std::cout << "$var reg " << (*it).width << " " << varsyms[(*it).id] << he.outputPostFix.c_str() <<
          " " <<  tn.c_str() << " $end" << std::endl;

        std::string sym = varsyms[(*it).id] + he.outputPostFix.c_str();
        allEis.push_back( EventInstance(owner,(*it).id,sym) );
      }
    }
    
    // Now dump our children
    Emit::OwnerId id;
    for ( id=0; id<Emit::getHeader().classDefs.size(); id++ ) {
      if ( Emit::getHeader().classDefs[id].parentIndex == owner ) {
        dumpVCDScope( out, id, varsyms, allEis );
      }
    }
    out << "$upscope $end" << std::endl;
}


static 
Emit::EventType 
getEventType( Emit::EventQEntry* e ) 
{
  Emit::EventType rtype = Emit::Transient;
  std::vector<Emit::EventMap>::iterator it;
  for ( it=Emit::getHeader().eventMap.begin();
        it != Emit::getHeader().eventMap.end(); it++ ) {
    if ( (*it).id == e->eid ) {
      rtype = (*it).type;
    }
  }  
  return rtype;
}




struct TimeLineData {
  Emit::Time t;
  std::string                time;
  std::string                values;
};
bool SortPredicate( const TimeLineData& tl1, const TimeLineData& tl2 )
{
  return tl1.t < tl2.t;
}

std::ostream& EmitFormatter::formatDumpToStreamVCD( std::ostream& out )
{

  std::vector<Emit::HeaderEntry>::iterator hit;
  std::map< Emit::EventId, std::string, ecmp > varsyms;
  std::vector<EventInstance> allEis;
  Emit::OwnerId owner;

  // Date
  char date[80];
  const char *fmt="%A, %B %d %Y %X";
  struct tm* pmt;
  time_t     raw_time;
  time ( &raw_time );
  pmt = gmtime( &raw_time );
  strftime(date,80,fmt,pmt);
  out << "$date" << std::endl;
  out << "         " << date << std::endl;
  out << "$end" << std::endl;  

  // Version
  out << "$version" << std::endl;  
  out << "            OCPI VCD Software Event Dumper V1.0" << std::endl;
  out << "$end" << std::endl;  
  
  // Timescale
  out << "$timescale" << std::endl;    
  out << "          1 us" << std::endl;
  out << "$end" << std::endl;

  // Now for the class definitions  
  out << "$scope module Software $end" << std::endl;    

  // For each top level object generate its $var defs and then dump its children
  getVCDVarSyms( *m_traceable, varsyms );
  for ( owner=0, hit=m_traceable->getHeader().classDefs.begin();
        hit!=m_traceable->getHeader().classDefs.end(); hit++,owner++ ) {
    if ( (*hit).parentIndex != -1 ) continue;
    dumpVCDScope( out, owner, varsyms, allEis );
  }
  out << "$upscope $end" << std::endl;
  out << "$enddefinitions $end" << std::endl;

  // Dump out the initial values
  out << "$dumpvars" << std::endl;
  std::vector<EventInstance>::iterator eisit;
  for ( eisit=allEis.begin(); eisit!=allEis.end(); eisit++ ) {
    out << "0" << (*eisit).sym.c_str() << std::endl;
  }
  out << "$end" << std::endl;  

  // Now emit the events
  Emit::Time start_time = 0;
  std::vector<Emit::EventQ*>::iterator it;
  std::vector<TimeLineData> tldv;
  for ( it=Emit::getHeader().eventQ.begin();
        it!=Emit::getHeader().eventQ.end();  it++ ) {

    Emit::EventQEntry* e = (*it)->full ? (*it)->current : (*it)->start;
    SValue* d = (SValue*)(e + 1);
    while( e && e->size ) {
    
      Emit::HeaderEntry & he = Emit::getHeader().classDefs[e->owner];
      if ( start_time == 0 ) {
        start_time = e->time;
      }

      TimeLineData tld;

      // event time
      char tbuf[256];
      Emit::Time ctime = e->time-start_time;
      snprintf(tbuf,256,"\n#%lld\n",(long long)ctime);
      tld.t = ctime;
      tld.time = tbuf;

      switch ( getEventType( e )  ) {
      case Emit::Transient:
        {
          tld.values += "1" + varsyms[e->eid] + he.outputPostFix.c_str() + "\n";
          snprintf(tbuf,256,"#%lld\n",(long long)(ctime+1));
          tld.values += tbuf;
          tld.values += "0" + varsyms[e->eid] + he.outputPostFix.c_str() + "\n";
        }
        break;
      case Emit::State:
        {
          snprintf(tbuf,256,"%d ",(d->uvalue == 0) ? 0 : 1);
          tld.values += tbuf;
          tld.values += varsyms[e->eid] + he.outputPostFix.c_str() + "\n";
        }
        break;
      case Emit::Value:
        {
          tld.values += "b";

          Emit::EventMap* emap = getEventMap( e ) ;

            switch ( emap->dtype ) {
            case Emit::u:
            case Emit::i:
            case Emit::c:
              {
                OCPI::OS::uint32_t* ui = reinterpret_cast<OCPI::OS::uint32_t*>(&d->uvalue);
                ui++;
                for (int n=0; n<2; n++ ) {
                  for ( OCPI::OS::uint32_t i=(1<<31); i>=(OCPI::OS::uint32_t)1; ) {
                    tld.values += ((i & *ui)==i) ? "1" : "0";
                    i = i>>1;
                  }
                  ui--;
                }
              }
              break;
            case Emit::d:
              {
                OCPI::OS::uint32_t* ui = reinterpret_cast<OCPI::OS::uint32_t*>(&d->dvalue);
                ui++;
                for (int n=0; n<2; n++ ) {
                  for ( OCPI::OS::uint32_t i=(1<<31); i>=(OCPI::OS::uint32_t)1; ) {
                    tld.values += ((i & *ui)==i) ? "1" : "0";
                    i = i>>1;
                  }
                  ui--;
                }
              }
              break;
            }
#ifdef GTK_VERSION_REQ_SPACE
          tld.values += " ";
#endif
          tld.values +=  varsyms[e->eid] + he.outputPostFix.c_str() + "\n";
        }
        break;
      }
      tldv.push_back(tld);
      e = getNextEntry( e, (*it) );
    }
  }
  std::sort( tldv.begin(), tldv.end(), SortPredicate );
  unsigned int n;
  // compress

#ifndef NDEBUG
  printf("size = %" PRIsize_t "\n", tldv.size() );
#endif

  if ( tldv.size() ) {
    for(n=0; n<tldv.size()-1; n++) {  
      if ( tldv[n].time == tldv[n+1].time ) {
        tldv[n].values = tldv[n+1].values;
        tldv.erase(tldv.begin()+n+1);
        n=0;
      }
    }
  }
  for(unsigned int n=0; n<tldv.size(); n++ ) {
    out << tldv[n].time;
    out << tldv[n].values;
  }

  // End of file
  out << std::endl << "$dumpoff" << std::endl;
  for ( eisit=allEis.begin(); eisit!=allEis.end(); eisit++ ) {
    //    out << "x" << (*eisit).sym.c_str() << std::endl;
  }
  out << "$end" << std::endl;  
 
  return out;
}


Emit::Time 
Emit::SimpleSystemTime::
getTime()
{
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv );
  Time t = (tv.tv_sec * 1000 * 1000) + tv.tv_nsec;
  return t;  
}


Emit::FastSystemTime::
FastSystemTime()
{
  int wait_time;
  int result;
  fasttime_statistics_t stats;
  struct timespec tp_fast, tp_actual;

  m_method = fasttime_init_context(NULL, 
                 FASTTIME_METHOD_CLIENT | FASTTIME_METHOD_DAEMON);

  switch (m_method & ~FASTTIME_METHOD_SYSTEM)
    {
        case FASTTIME_METHOD_CLIENT:
            printf("Using client calibration");
            break;

        case FASTTIME_METHOD_DAEMON:
            printf("Using daemon calibration");
            break;

        default:
            printf("Error in init; quitting\n");
            ocpiAssert(!"Fasttime method not supported");
    }

    /* Check availability */
    while (fasttime_getstatistics(NULL, &stats) != 0)
      ;

    if (!stats.ready)
    {
      clock_gettime(CLOCK_REALTIME, &tp_actual );
        wait_time = stats.ready_time - tp_actual.tv_sec;
        if (wait_time > 0)
        {
            printf("Waiting %d secs for fasttime to get ready...\n", wait_time);
            sleep(wait_time);
        }
    }
    
    printf("Check accuracy:\n");
    do {
      clock_gettime(CLOCK_REALTIME, &tp_actual ); 
      result = fasttime_gettime(&tp_fast);  
    } while (result);
    printf(" Fast:   %lu secs, %lu nsecs\n", tp_fast.tv_sec, tp_fast.tv_nsec);
    printf(" Actual: %lu secs, %lu nsecs\n", tp_actual.tv_sec, tp_actual.tv_nsec); 
    printf(" Delta = %lu,%lu\n", tp_actual.tv_sec-tp_fast.tv_sec, tp_actual.tv_nsec-tp_fast.tv_nsec);

}


Emit::Time 
Emit::FastSystemTime::
getTime()
{
  struct timespec tv;
  fasttime_gettime(&tv);
  Time t = (tv.tv_sec * 1000 * 1000) + tv.tv_nsec;
  return t;  
}


Emit::TimeSource::
TimeSource()
{
  // Empty
}


}
}
