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


#include <CpiTimeEmit.h>
#include <time.h>
#include <CpiOsAssert.h>
#include <fasttime.h>

extern "C" {
  int CpiTimeARegister( char* signal_name );
  void CpiEmit( int sig );
};

int CpiTimeARegister( char* signal_name )
{
  return CPI::Time::Emit::RegisterEvent::registerEvent( signal_name );
}
void CpiEmit( int sig ) 
{
  CPI::Time::Emit::getSEmit().emit(sig);
}



static 
void
exitHandler()
{
  static bool once=false;
  if ( ! once ) {
    once = true;
    CPI::Time::EmitFormatter ef( CPI::Time::Emit::getHeader().dumpFormat  );
    CPI::Time::Emit::getHeader().dumpFileStream << ef;
  }
}

void
CPI::Time::Emit::
init()
  throw ( CPI::Util::EmbeddedException )
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
  if ( getHeader().init == true ) {
    return;
  }
  
  const char *tmp;
  if ( ( tmp = getenv("CPI_TIME_EMIT_TRACE_CD") ) != NULL ) {
    getHeader().traceCD = atoi(tmp);    
  }

  if ( ( tmp = getenv("CPI_TIME_EMIT_DUMP_ON_EXIT") ) != NULL ) {
    getHeader().dumpOnExit = atoi(tmp);
    if ( getHeader().dumpOnExit ) {
      atexit( exitHandler );
    }
  }

  if ( ( tmp = getenv("CPI_TIME_EMIT_DUMP_FORMAT") ) != NULL ) {
    if ( strcasecmp( tmp, "READABLE" ) == 0 ) {
      getHeader().dumpFormat = EmitFormatter::CPIReadable;
    }
    else if ( strcasecmp( tmp, "RAW" ) == 0 ) {
      getHeader().dumpFormat =  EmitFormatter::CPIRaw;
    }
    else if ( strcasecmp( tmp, "VCD" ) == 0 ) {
      getHeader().dumpFormat =  EmitFormatter::VCDFormat;
    }
  }

  if ( ( tmp = getenv("CPI_TIME_EMIT_DUMP_FILENAME") ) != NULL ) {
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
      throw CPI::Util::EmbeddedException( err.c_str() );
    }
  }

  getHeader().init = true;
}

void 
CPI::Time::Emit::
pre_init( const char* class_name, 
              const char* instance_name, 
              QConfig* config )
  throw ( CPI::Util::EmbeddedException )
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
    if ( (qsize = getenv("CPI_TIME_EMIT_Q_SIZE") ) != NULL ) {
      m_q->config.size = atoi(qsize);
    }
    else {
      m_q->config.size  = 256 * 1024;
    }
    char* swf;
    if ( (swf = getenv("CPI_TIME_EMIT_Q_SWF") ) != NULL ) {
      m_q->config.stopWhenFull = atoi(swf);
    }
    else {
      m_q->config.stopWhenFull = false;
    }
  }
  m_q->allocate();

}


CPI::Time::Emit::
Emit( TimeSource& ts, const char* class_name, 
      const char* instance_name, QConfig* config )
  throw ( CPI::Util::EmbeddedException )
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex() );
  m_ts = &ts;
  pre_init( class_name, instance_name, config );
  init();
  if ( getHeader().traceCD ) {
    CPI_EMIT_("Object Exists");
  }
}

CPI::Time::Emit::
Emit( const char* class_name, 
              const char* instance_name, 
              QConfig* config )
  throw ( CPI::Util::EmbeddedException )
    : m_level(1),m_parent(NULL)
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex() );
  pre_init( class_name, instance_name, config );
  init();
  if ( getHeader().traceCD ) {
    CPI_EMIT_("Object Exists");
  }
}


CPI::Time::Emit::
Emit( Emit* parent, 
              const char* class_name, 
              const char* instance_name, 
              QConfig* config )
  throw ( CPI::Util::EmbeddedException )
  :m_parent(parent)
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
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
    CPI_EMIT_("Object Exists");
  }
}


void 
CPI::Time::Emit::
stop( bool globally )
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());

  if ( globally ) {
    std::vector<EventQ*>::iterator it;    
    for( it=CPI::Time::Emit::getHeader().eventQ.begin();
         it!=CPI::Time::Emit::getHeader().eventQ.end(); it++ ) {
      (*it)->done = true;
    }
  }
  else {
    m_q->done = false;
  }
}



CPI::Time::Emit::OwnerId 
CPI::Time::Emit::
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
CPI::Time::Emit::
getHeaderInfo( CPI::Time::Emit* t, 
               int& instance  ) 
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
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
CPI::Time::Emit::
isChild( Emit::OwnerId id ) {

  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
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


CPI::Time::Emit::~Emit()
  throw ()
{

  if (   getHeader().traceCD ) {
    if(!getHeader().shuttingDown) 
      CPI_EMIT_("Object Terminated");
  }
}

void
CPI::Time::Emit::
shutdown()
  throw()
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());

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
CPI::Time::Emit::RegisterEvent::
registerEvent( const char* event_name, int width,
               EventType type,
               DataType dtype)
{
  int e;
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
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

CPI::Time::Emit::RegisterEvent::RegisterEvent( const char* event_name, int width,
                                                       EventType type,
                                                       DataType dtype)
{
  m_eid = registerEvent( event_name, width, type, dtype);
}



CPI::Time::Emit::RegisterEvent::RegisterEvent( CPI::Util::PValue& p )
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
  m_eid = getHeader().nextEventId++;
  int width = p.width*8;
  DataType dtype=CPI::Time::Emit::i;
  switch( p.type ){
  case CPI::Metadata::Property::CPI_Short:
  case CPI::Metadata::Property::CPI_Long:
  case CPI::Metadata::Property::CPI_Char:
  case CPI::Metadata::Property::CPI_LongLong:
    dtype = CPI::Time::Emit::i;
    break;

  case CPI::Metadata::Property::CPI_Bool:
  case CPI::Metadata::Property::CPI_ULong:
  case CPI::Metadata::Property::CPI_UShort:
  case CPI::Metadata::Property::CPI_ULongLong:
  case CPI::Metadata::Property::CPI_UChar:
    dtype = CPI::Time::Emit::u;
    break;

  case CPI::Metadata::Property::CPI_Double:
  case CPI::Metadata::Property::CPI_Float:
    dtype = CPI::Time::Emit::d;
    break;

  case CPI::Metadata::Property::CPI_String:
    dtype = CPI::Time::Emit::c;
    break;

  case CPI::Metadata::Property::CPI_none:
  case CPI::Metadata::Property::CPI_data_type_limit:
    cpiAssert(0);
    break;


  }
  Emit::getHeader().eventMap.push_back( EventMap(m_eid,p.name,width,
                                                         CPI::Time::Emit::Value, dtype
                                                         ) );
}

CPI::Time::EmitFormatter::EmitFormatter( DumpFormat format)
  :m_dumpFormat(format)
{

};

const char* CPI::Time::EmitFormatter::getEventDescription( Emit::EventId id ) {
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
  std::vector<Emit::EventMap>::iterator it;
  for ( it=m_traceable->getHeader().eventMap.begin(); 
        it!=m_traceable->getHeader().eventMap.end(); it++) {
    if ( (*it).id == id ) {
      return (*it).eventName.c_str();
    }
  }
  return NULL;
}

std::string CPI::Time::EmitFormatter::formatEventString ( Emit::EventQEntry& eqe, 
                                                                  CPI::Time::Emit::Time time_ref ) 
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

std::string CPI::Time::EmitFormatter::formatEventStringRaw ( Emit::EventQEntry& eqe, 
                                                                  CPI::Time::Emit::Time time_ref ) 
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



static CPI::Time::Emit::EventMap* getEventMap( CPI::Time::Emit::EventQEntry* e ) 
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
  std::vector<CPI::Time::Emit::EventMap>::iterator it;
  for ( it=CPI::Time::Emit::getHeader().eventMap.begin();
        it != CPI::Time::Emit::getHeader().eventMap.end(); it++ ) {
    if ( (*it).id == e->eid ) {
      return &(*it);
    }
  }  
  return NULL;
}

static inline CPI::Time::Emit::EventQEntry* getNextEntry( CPI::Time::Emit::EventQEntry * ce, CPI::Time::Emit::EventQ * q )
{
  CPI::Time::Emit::EventQEntry * ne = reinterpret_cast<CPI::Time::Emit::EventQEntry *>( 
               ( (CPI::OS::uint8_t*)((CPI::OS::uint8_t*)ce + sizeof(CPI::Time::Emit::EventQEntry) + ce->size) ));
  return ne;
}



CPI::Time::Emit& 
CPI::Time::Emit::
getSEmit() 
{
  static SEmit t;
  return t;
}



// NOT mutex protected, the caller needs to handle this !!
CPI::Time::Emit::Time getStartTime() 
{
  std::vector<CPI::Time::Emit::EventQ*>::iterator it;
  CPI::Time::Emit::Time time=0;
  for( it=CPI::Time::Emit::getHeader().eventQ.begin();
       it!=CPI::Time::Emit::getHeader().eventQ.end(); it++ ) {

    CPI::Time::Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
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


std::ostream& CPI::Time::EmitFormatter::formatDumpToStreamRaw( std::ostream& out ) 
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());

  std::vector<CPI::Time::Emit::EventQ*>::iterator it;
  for( it=CPI::Time::Emit::getHeader().eventQ.begin();
       it!=CPI::Time::Emit::getHeader().eventQ.end(); it++ ) {

    CPI::Time::Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
    CPI::Time::Emit::Time time_ref = getStartTime();

    while( qe && qe->size ) {

      std::string str = formatEventStringRaw( *qe, time_ref );
      out << str.c_str() << ",";
      std::string ostr;
      formatOwnerString( qe->owner, ostr );
      out << ostr.c_str() << ",";

      CPI::Time::Emit::EventMap* emap = getEventMap( qe );
      CPI::Time::SValue* d = (CPI::Time::SValue*)(qe + 1);

      switch ( emap->dtype ) {
      case CPI::Time::Emit::u:
        out << d->uvalue  << std::endl;
        break;
      case CPI::Time::Emit::i:
        out << d->ivalue  << std::endl;
        break;
      case CPI::Time::Emit::c:
        out << d->ivalue  << std::endl;
        break;
      case CPI::Time::Emit::d:
        out << d->dvalue  << std::endl;
        break;
      }
      qe = getNextEntry( qe, (*it) );
    }

  }
  return out;  
}

CPI::Time::Emit::Header& CPI::Time::Emit::getHeader() {
  static Header h;
  return h;
}


std::ostream& CPI::Time::EmitFormatter::formatDumpToStreamReadable( std::ostream& out ) 
{
  AUTO_MUTEX(CPI::Time::Emit::getGMutex());
  std::vector<CPI::Time::Emit::EventQ*>::iterator it;
  for( it=CPI::Time::Emit::getHeader().eventQ.begin();
       it!=CPI::Time::Emit::getHeader().eventQ.end(); it++ ) {

    CPI::Time::Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
    CPI::Time::Emit::Time time_ref = getStartTime();
    while( qe && qe->size ) {

      std::string str = formatEventString( *qe, time_ref );
      out << str.c_str() << "     ";
      int mlen = 60 - str.size();
      for ( int n=0; n<mlen; n++) out << "";
   
      std::string ostr;
      formatOwnerString( qe->owner, ostr );
      out << ostr.c_str() << "  ";

      CPI::Time::Emit::EventMap* emap = getEventMap( qe );
      CPI::Time::SValue* d = (CPI::Time::SValue*)(qe + 1);

      switch ( emap->dtype ) {
      case CPI::Time::Emit::u:
        out << d->uvalue  << std::endl;
        break;
      case CPI::Time::Emit::i:
        out << d->ivalue  << std::endl;
        break;
      case CPI::Time::Emit::c:
        out << d->ivalue  << std::endl;
        break;
      case CPI::Time::Emit::d:
        out << d->dvalue  << std::endl;
        break;
      }
      qe = getNextEntry( qe, (*it) );
    }

  }
  return out;
}

std::ostream& CPI::Time::EmitFormatter::formatDumpToStream( std::ostream& out ) 
{
  switch ( m_dumpFormat ) {
  case CPIReadable:
    return formatDumpToStreamReadable(out);
    break;
          
  case CPIRaw:
    return formatDumpToStreamRaw(out);
    break;

  case VCDFormat:
    return formatDumpToStreamVCD(out);
    break;
  }

  return out;
}
      

void CPI::Time::EmitFormatter::formatOwnerString( Emit::OwnerId id, std::string& str, bool full_path ) {
  if ( id >= CPI::Time::Emit::getHeader().classDefs.size() ) {
    return;
  }
  
  if ( full_path && CPI::Time::Emit::getHeader().classDefs[id].parentIndex != -1 ) {
    formatOwnerString( CPI::Time::Emit::getHeader().classDefs[id].parentIndex, str );
  }

  if ( CPI::Time::Emit::getHeader().classDefs[id].className != "" ) {
    str.append(  CPI::Time::Emit::getHeader().classDefs[id].className + ":" );
  }
  else {
    str.append( "Class:" );
  }
  if ( CPI::Time::Emit::getHeader().classDefs[id].instanceName != "" ) {
    str.append(  CPI::Time::Emit::getHeader().classDefs[id].instanceName + ":" );          
  }
  else {
    str.append(  ":" );          
  }
  char buf[10];
  sprintf(buf,"%d::",CPI::Time::Emit::getHeader().classDefs[id].instanceId );
  str.append(buf);

}

namespace {
  struct ecmp
  {
    bool operator()(const CPI::Time::Emit::EventId e1, const  CPI::Time::Emit::EventId e2) const
    {
      return e1 < e2;
    }
  };
}

#define SYMSTART 33
#define SYMEND   126
#define SYMLEN (SYMEND-SYMSTART)
void getVCDVarSyms( CPI::Time::Emit& t,
                           std::map<CPI::Time::Emit::EventId,std::string,ecmp> & varsyms )
{
  unsigned int n;
  char syms[SYMLEN];
  for( n=0; n<SYMLEN; n++)syms[n]=static_cast<char>(n+SYMSTART);
  std::vector<CPI::Time::Emit::EventMap>::iterator it;
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

static bool eventProducedBy( CPI::Time::Emit::EventId event, CPI::Time::Emit::OwnerId owner )
{
  std::vector<CPI::Time::Emit::EventQ*>::iterator it;
  for( it=CPI::Time::Emit::getHeader().eventQ.begin();
       it!=CPI::Time::Emit::getHeader().eventQ.end(); it++ ) {
    CPI::Time::Emit::EventQEntry* qe = (*it)->full ? (*it)->current : (*it)->start;
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
  CPI::Time::Emit::OwnerId     owner;
  CPI::Time::Emit::EventId     id;
  std::string                       sym;
  EventInstance(CPI::Time::Emit::OwnerId     o,
                CPI::Time::Emit::EventId     i,
                std::string                       s):owner(0),id(i),sym(s){}

};



static void dumpVCDScope( std::ostream& out, CPI::Time::Emit::OwnerId owner,
                          std::map< CPI::Time::Emit::EventId, std::string, ecmp > & varsyms,
                          std::vector<EventInstance> & allEis )
{
    std::string pname;
    CPI::Time::EmitFormatter::formatOwnerString( owner, pname, false );
    std::replace(pname.begin(),pname.end(),' ','_');
    out << "$scope module " << pname.c_str() << " $end" << std::endl; 
    CPI::Time::Emit::HeaderEntry & he = CPI::Time::Emit::getHeader().classDefs[owner];
    for( int n=0; n<=owner/SYMLEN; n++) he.outputPostFix=static_cast<char>((SYMSTART+owner)%SYMLEN);
    // Dump the variables for this object
    std::vector<CPI::Time::Emit::EventMap>::iterator it;
    for ( it=CPI::Time::Emit::getHeader().eventMap.begin();
          it!=CPI::Time::Emit::getHeader().eventMap.end();  it++ ) {
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
    CPI::Time::Emit::OwnerId id;
    for ( id=0; id<CPI::Time::Emit::getHeader().classDefs.size(); id++ ) {
      if ( CPI::Time::Emit::getHeader().classDefs[id].parentIndex == owner ) {
        dumpVCDScope( out, id, varsyms, allEis );
      }
    }
    out << "$upscope $end" << std::endl;
}


static 
CPI::Time::Emit::EventType 
getEventType( CPI::Time::Emit::EventQEntry* e ) 
{
  CPI::Time::Emit::EventType rtype = CPI::Time::Emit::Transient;
  std::vector<CPI::Time::Emit::EventMap>::iterator it;
  for ( it=CPI::Time::Emit::getHeader().eventMap.begin();
        it != CPI::Time::Emit::getHeader().eventMap.end(); it++ ) {
    if ( (*it).id == e->eid ) {
      rtype = (*it).type;
    }
  }  
  return rtype;
}




struct TimeLineData {
  CPI::Time::Emit::Time t;
  std::string                time;
  std::string                values;
};
bool SortPredicate( const TimeLineData& tl1, const TimeLineData& tl2 )
{
  return tl1.t < tl2.t;
}

std::ostream& CPI::Time::EmitFormatter::formatDumpToStreamVCD( std::ostream& out )
{

  std::vector<Emit::HeaderEntry>::iterator hit;
  std::map< CPI::Time::Emit::EventId, std::string, ecmp > varsyms;
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
  out << "            CPI VCD Software Event Dumper V1.0" << std::endl;
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
  CPI::Time::Emit::Time start_time = 0;
  std::vector<CPI::Time::Emit::EventQ*>::iterator it;
  std::vector<TimeLineData> tldv;
  for ( it=CPI::Time::Emit::getHeader().eventQ.begin();
        it!=CPI::Time::Emit::getHeader().eventQ.end();  it++ ) {

    CPI::Time::Emit::EventQEntry* e = (*it)->full ? (*it)->current : (*it)->start;
    CPI::Time::SValue* d = (CPI::Time::SValue*)(e + 1);
    while( e && e->size ) {
    
      CPI::Time::Emit::HeaderEntry & he = CPI::Time::Emit::getHeader().classDefs[e->owner];
      if ( start_time == 0 ) {
        start_time = e->time;
      }

      TimeLineData tld;

      // event time
      char tbuf[256];
      CPI::Time::Emit::Time ctime = e->time-start_time;
      snprintf(tbuf,256,"\n#%lld\n",(long long)ctime);
      tld.t = ctime;
      tld.time = tbuf;

      switch ( getEventType( e )  ) {
      case CPI::Time::Emit::Transient:
        {
          tld.values += "1" + varsyms[e->eid] + he.outputPostFix.c_str() + "\n";
          snprintf(tbuf,256,"#%lld\n",(long long)(ctime+1));
          tld.values += tbuf;
          tld.values += "0" + varsyms[e->eid] + he.outputPostFix.c_str() + "\n";
        }
        break;
      case CPI::Time::Emit::State:
        {
          snprintf(tbuf,256,"%d ",(d->uvalue == 0) ? 0 : 1);
          tld.values += tbuf;
          tld.values += varsyms[e->eid] + he.outputPostFix.c_str() + "\n";
        }
        break;
      case CPI::Time::Emit::Value:
        {
          tld.values += "b";

          CPI::Time::Emit::EventMap* emap = getEventMap( e ) ;

            switch ( emap->dtype ) {
            case CPI::Time::Emit::u:
            case CPI::Time::Emit::i:
            case CPI::Time::Emit::c:
              {
                CPI::OS::uint32_t* ui = reinterpret_cast<CPI::OS::uint32_t*>(&d->uvalue);
                ui++;
                for (int n=0; n<2; n++ ) {
                  for ( CPI::OS::uint32_t i=(1<<31); i>=(CPI::OS::uint32_t)1; ) {
                    tld.values += ((i & *ui)==i) ? "1" : "0";
                    i = i>>1;
                  }
                  ui--;
                }
              }
              break;
            case CPI::Time::Emit::d:
              {
                CPI::OS::uint32_t* ui = reinterpret_cast<CPI::OS::uint32_t*>(&d->dvalue);
                ui++;
                for (int n=0; n<2; n++ ) {
                  for ( CPI::OS::uint32_t i=(1<<31); i>=(CPI::OS::uint32_t)1; ) {
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
  printf("size = %ld\n", tldv.size() );
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


CPI::Time::Emit::Time 
CPI::Time::Emit::SimpleSystemTime::
getTime()
{
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv );
  Time t = (tv.tv_sec * 1000 * 1000) + tv.tv_nsec;
  return t;  
}


CPI::Time::Emit::FastSystemTime::
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
            cpiAssert(!"Fasttime method not supported");
    }

    /* Check availability */
    while (fasttime_getstatistics(NULL, &stats) != 0);

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


CPI::Time::Emit::Time 
CPI::Time::Emit::FastSystemTime::
getTime()
{
  struct timespec tv;
  fasttime_gettime(&tv);
  Time t = (tv.tv_sec * 1000 * 1000) + tv.tv_nsec;
  return t;  
}


CPI::Time::Emit::TimeSource::
TimeSource()
{
  // Empty
}


