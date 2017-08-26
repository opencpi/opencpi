#define NDEBUG 1
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

/*
 * Abstact:
 *   This file contains the Interface for the OCPI DDS port.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 8/2011
 *    Revision Detail: Created
 *
 */

#include <OcpiDDSTransport.h>
#include <ezxml.h>
#include <OcpiOsDataTypes.h>
#include <OcpiOsMutex.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiUtilEzxml.h>
#include <OcpiPValue.h>
#include <list>
#include <stack>
#include <map>
#include <XferException.h>
#include <iostream>
#include <OcpiBuffer.h>
#include <OpenSpliceBindings.h>
#include <OcpiUtilProtocol.h>
#include "OcpiUtilValue.h"

#define BUF_SIZE 1024

namespace OX = OCPI::Util::EzXml;
using namespace std;
using namespace OpenSpliceBindings;
namespace OU = OCPI::Util;
namespace OA = OCPI::API;

namespace OCPI {  
  namespace Msg {
    namespace DDS {

      struct EndPoint  {
	EndPoint( const char * ep ) 
	  : m_url(ep)
	{
	  char ms[128];
	  char k[128];
	  char t[128];

	  /* ocpi-dds-msg:topic-name:key */
	  //	  int c = sscanf(ep,"ocpi-dds-msg:%[^;];%[^;];%[^;];%s", t, m,s, k );
	  int c = sscanf(ep,"ocpi-dds-msg:%[^;];%[^;];%s", t, ms, k );
	  if ( c == 1 ) {
	    topic = t;	      
	  }
	  else if ( c != 3 ) {
	    fprintf( stderr, "DDS::EndPoint  ERROR: Bad DDS endpoint format (%s)\n", ep );
	    throw DataTransfer::DataTransferEx( DataTransfer::UNSUPPORTED_ENDPOINT, ep );	  
	  }
	  else {
	    c = strlen(ms)-1;
	    if ( ms[c] == ',' ) ms[c] = 0;
	    while ( c > 0 ) {
	      if ( ms[c] == ':' ) {
		struct_name = &ms[c+1];
		ms[c-1] = 0;
		module_name = ms;
		break;
	      }		   
	      c--;
	    }
	    topic = t;
	    type = module_name;
	    type += "::";
	    type += struct_name;
	    key = k;
	  }
	}
	std::string type;
	std::string topic;
	std::string module_name;
	std::string struct_name;
	std::string key;	  
	std::string m_url;
      };


      //       class TopicData : public OCPI::Util::Protocol {
      static std::string formatErrorMsg("Unable to generate DDS format string");
      class TopicData {

      private:
	int m_enumid;

      public:
	std::string name;
	std::string module_name;
	std::string struct_name;
	std::string type;
	std::string key;
	std::string format;
	std::string participant;
	static const int DEFAULT_MAX_MSG_SIZE=60000;
	OCPI::Util::Protocol * m_proto;


	TopicData()
	  :m_enumid(0),participant("OCPI"),m_currentOffset(0),m_OffsetOffset(0),m_unbounded(false){}

	void setEp( EndPoint & ep) {


	  
	  // Override the user data if available
	  if (  ! ep.key.empty() ) {
	    key = ep.key;
	  }
	  if ( ! ep.type.empty() ) {
	    type = ep.type;
	  }
	  if ( ! ep.module_name.empty() ) {
	    module_name = ep.module_name;
	  }
	  if ( ! ep.struct_name.empty() ) {
	    struct_name = ep.struct_name;
	  }

	  if ( ! ep.topic.empty() ) {
	    name = ep.topic;
	  }
	  else {
	    name = struct_name;
	  }
	}


	
	TopicData & operator=( const OCPI::Util::Protocol * p )
	{
#ifdef G
	  static_cast<OCPI::Util::Protocol&>(*this) = p;
#else
	  m_proto = (OCPI::Util::Protocol *)p;
#endif
	  for ( int n=0; n<nMembers(); n++ ) {
	    OCPI::Util::Member & m = member(n);
	    if ( m.m_isSequence && ( m.m_sequenceLength == 0 ) ) {
	      m_unbounded = true;
	      break;
	    }
	  }
	  for ( int n=0; n<nMembers(); n++ ) {
	    OCPI::Util::Member & m = member(n);
	    if ( m.m_isKey ) {
	      if ( ! key.empty() ) {
		key += ",";
	      }
	      key += m.m_name;
	    }
	  }
	  char * mms = getenv("OCPI_MAX_MESSAGE_SIZE");
	  if ( mms ) {
	    m_maxMsgSize = atoi(mms);
	  }
	  else {
	    m_maxMsgSize = DEFAULT_MAX_MSG_SIZE;
	  }

	  
	  // Get defaults from protocol
	  std::string tmp(p->m_qualifiedName);
	  size_t pos = tmp.find_last_of("::");
	  struct_name = tmp.substr(pos+1,tmp.length());
	  module_name = tmp.substr(0,pos-1);

	 
	  return *this;
	}


	const char * ddsType( OU::Member & m ) {

	  OCPI::API::BaseType t = m.m_baseType;
	  if ( m.m_baseType == OCPI::API::OCPI_Type ) {
	    OU::Member * tm = m.m_type;
	    while ( tm ) {
	      if ( tm->m_type == NULL ) {
		t = tm->m_baseType;
		break;
	      }
	      tm = tm->m_type;
	    }
	  }


	  switch ( t ) {

	  case OCPI::API::OCPI_Type:
	    ocpiAssert(!"Not a valid type");
	    break;
	  case OCPI::API::OCPI_Struct:
	    return "Struct";
	  case OCPI::API::OCPI_Enum:
	    return "Enum";
	  case OCPI::API::OCPI_Long:
	    return "Long";
	  case OCPI::API::OCPI_Short:
	    return "Short";
	  case OCPI::API::OCPI_Bool:
	    return "Boolean";
	  case OCPI::API::OCPI_Char:
	    return "Char";
	  case OCPI::API::OCPI_Double:
	    return "Double";
	  case OCPI::API::OCPI_Float:
	    return "Float";
	  case OCPI::API::OCPI_UChar:
	    return "Octet";
	  case OCPI::API::OCPI_ULong:
	    return "ULong";
	  case OCPI::API::OCPI_UShort:
	    return "UShort";
	  case OCPI::API::OCPI_LongLong:
	    return "LongLong";
	  case OCPI::API::OCPI_ULongLong:
	    return "ULongLong";
	  case OCPI::API::OCPI_String:
	    return "String";
	  case OCPI::API::OCPI_none:
	  case OCPI::API::OCPI_scalar_type_limit:
	    ocpiAssert(!"These type should never be defined in the protocol");
	    return "BadBad";

	  }
	  return NULL;
	}



	std::string & formatType( OU::Member& m, std::string & format, bool typname=true ) {
	  char buf[BUF_SIZE];

	  if ( typname && m.m_isSequence && (m.m_arrayRank>0) ) { 
	    if ( snprintf( buf, BUF_SIZE, "<Type name=\"%s::TypeDef%s\"/>", module_name.c_str(), m.m_name.c_str() ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    format += buf;
	    if ( snprintf( buf, BUF_SIZE, "%s::TypeDef%s", module_name.c_str(), m.m_name.c_str() ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    m.m_typeDef = buf;
	    return format;
	  }

	  if (  m.m_baseType == OCPI::API::OCPI_Struct ) {
	    if ( snprintf( buf, BUF_SIZE, "<Type name=\"%s::Struct%s\"/>", module_name.c_str(), m.m_name.c_str() ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    format += buf;
	  }
	  else if ( m.m_baseType == OCPI::API::OCPI_Type) {
	    //	    ocpiAssert(!"types ??");

	  }
	  else {
	    sprintf( buf, "<%s/>", ddsType(m) );
	    format += buf;
	  }	
	  return format;
	}


	std::string & formatSequence( OU::Member& m, std::string & format ) 
	{
	  char buf[BUF_SIZE];
	  if ( m.m_sequenceLength != 0 ) {
	    if ( snprintf( buf, BUF_SIZE, "<Sequence size=\"%zu\">", m.m_sequenceLength ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    format += buf;
	  }
	  else {
	    format += "<Sequence>";
	  }
	  formatType(m,format);
	  format += "</Sequence>";
	  return format;
	}

	std::string & formatMember( OU::Member& m, std::string & format, bool named=true ) 
	{
	  char buf[BUF_SIZE];

	  if ( named ) {
	    if ( snprintf( buf, BUF_SIZE, "<Member name=\"%s\">", m.m_name.c_str() ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    format += buf;
	  }

	  // In this case the sequence is the "outer" structure
	  if ( m.m_isSequence && (m.m_arrayRank > 0) ) {
	    formatSequence( m,format);
	  }
	  else if ( m.m_nEnums ) {
	    if ( snprintf( buf, BUF_SIZE, "<Enum name=\"enum_%d\">", m_enumid ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    format += buf;
	    for (unsigned int n=0; n<m.m_nEnums; n++ ) {
	      if ( snprintf( buf, BUF_SIZE, "<Element name=\"%s\"/>", m.m_enums[n] ) < 0 ) {
		throw formatErrorMsg;
	      }
	      format += buf;
	    }	    
	    format += "</Enum>";	    
	  }
	  else if ( m.m_arrayRank > 0 ) {
	    for ( unsigned int y=0; y<m.m_arrayRank; y++ ) {
	      if ( snprintf( buf, BUF_SIZE, "<Array size=\"%zu\">", m.m_arrayDimensions[y] ) < 0 ) {
		throw formatErrorMsg;
	      }
	      format += buf;
	    }

	    // Special case, and array of sequences
	    if ( m.m_isSequence ) {
	      formatSequence( m, format );
	    }
	    else {
	      formatType(m,format);
	    }
	    if ( m.m_type ) {
	      formatMember( *m.m_type, format, false );
	    }
	    for ( unsigned int y=0; y<m.m_arrayRank; y++ ) {
	      format += "</Array>";
	    }
	  }
	  else if ( m.m_isSequence ) {
	    formatSequence( m,format);
	  }
	  else {
	    formatType( m, format);
	  }	  
	  if ( named ) {
	    format += "</Member>";
	  }
	  return format;
	}



	std::string & defineStruct( OU::Member &m, const char * name, std::string & format ) 
	{
	  char buf[BUF_SIZE];
	    
	  if (  m.m_baseType == OCPI::API::OCPI_Struct ) {

	    if ( snprintf( buf, BUF_SIZE, "<Struct name=\"Struct%s\">", name  ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    format += buf;
	    if ( m.m_name.empty() ) {
	      if ( snprintf( buf, BUF_SIZE, "%s", name  ) < 0 ) {
		throw formatErrorMsg;
	      }
	      m.m_name = buf;
	      if ( snprintf( buf, BUF_SIZE, "%s::Struct%s", module_name.c_str(), name  ) < 0 ) {
		throw formatErrorMsg;
	      }
	      m.m_typeDef = buf;
	    }


	    for ( unsigned int n=0; n<m.m_nMembers; n++ ) {
	      formatMember( m.m_members[n], format );
	    }
	    format += "</Struct>";
	  }

	  // We also need to define Typedefs
	  else if ( m.m_isSequence && (m.m_arrayRank>0) ) { 

	    if ( snprintf( buf, BUF_SIZE, "<TypeDef name=\"TypeDef%s\">", name  ) < 0 ) {
	      throw formatErrorMsg;
	    }
	    format += buf;
	    if ( m.m_name.empty() ) {
	      if ( snprintf( buf, BUF_SIZE, "%s", name  ) < 0 ) {
		throw formatErrorMsg;
	      }
	      m.m_name = buf;
	      if ( snprintf( buf, BUF_SIZE, "%s::TypeDef%s", module_name.c_str(), name  ) < 0 ) {
		throw formatErrorMsg;
	      }
	      m.m_typeDef = buf;
	    }

	    for ( unsigned int y=0; y<m.m_arrayRank; y++ ) {
	      if ( snprintf( buf, BUF_SIZE, "<Array size=\"%zu\">", m.m_arrayDimensions[y] ) < 0 ) {
		throw formatErrorMsg;
	      }
	      format += buf;
	    }

	    formatType( m,format, false);

	    for ( unsigned int y=0; y<m.m_arrayRank; y++ ) {
	      format += "</Array>";
	    }
	      
	    format += "</TypeDef>";
	  }

	  return format;
	}

	std::string & defineStructs( std::string & format ) 
	{
	  char name[BUF_SIZE];
	  for ( int n=0; n<nMembers(); n++ ) {	    
	    OCPI::Util::Member & m = member(n);
	    defineStruct ( m, m.m_name.c_str(), format );	    
	    if ( m.m_type ) {
	      OU::Member * mt = m.m_type;
	      int count=0;
	      while( mt ) {
		if ( snprintf( name, BUF_SIZE, "%s%d", m.m_name.c_str(),count ) < 0 ) {
		  throw formatErrorMsg;
		}
		defineStruct ( *mt, name, format );		
		mt = mt->m_type;
	      }
	    }
	  }
	  return format;
	}



	void formatDDSMetaData()
	{

	  // Header
	  format = "<MetaData version=\"1.0.0\">";
	  char buf[BUF_SIZE];
	  sprintf( buf, "<Module name=\"%s\">", module_name.c_str() );
	  format += buf;

	  defineStructs(format);

	  sprintf( buf, "<Struct name=\"%s\">", struct_name.c_str() );
	  format += buf;
	  
	  for ( int n=0; n<nMembers(); n++ ) {
	    OCPI::Util::Member & m = member(n);
	    std::string fdata = formatMember( m, format );
	  }

	  // End Header
	  format += "</Struct></Module></MetaData>";

#ifndef NDEBUG
	  cout << endl << endl << endl << format << endl << endl << endl;
#endif	  
	}

	int maxMsgSize() {

#ifdef DONE
	  if ( ! m_unbounded ) {
	    return msgFixedSize();
	  }
#endif

	  return m_maxMsgSize;
	}


	int msgFixedSize()
	{
	  int size=0;
	  for ( int n=0; n<nMembers(); n++ ) {
	    OCPI::Util::Member & m = member(n);
	    if ( m.m_isSequence || (m.m_arrayRank>0) ) {
	      if  (m.m_sequenceLength == 0) {
		return -1;		
	      }
	      else {
		size += m.m_nBytes * m.m_sequenceLength;
	      }
	    }
	    size += m.m_nBytes;
	  }
	  return size;
	}

	inline int nMembers(){return m_proto->operations()[0].nArgs();}

	inline OCPI::Util::Member & member(int index) { 
	  OCPI::Util::Member &m = m_proto->operations()[0].args()[index];
	  if ( m.m_isSequence && (m.m_sequenceLength == 0) ) {  // Unbounded sequence
	    m_unbounded = true;
	  }
	  return m;
	}
	inline int mLength( int index, int vLen=0 ) {
	  int len;
	  OCPI::Util::Member &m = m_proto->operations()[0].args()[index];
	  if ( m.m_isSequence && (m.m_sequenceLength == 0) ) {  
	    len = vLen * m.m_nBytes;
	  }
	  else {
	    len = m.m_nBytes;
	  }
	  return len;
	}
	inline int mOffset( int index, int vLen=0 ) {
	  OCPI::Util::Member &m = m_proto->operations()[0].args()[index];

	  if ( m_unbounded ) {
	    int off = m_OffsetOffset;
	    if ( m.m_isSequence && (m.m_sequenceLength == 0) ) {  
	      off += vLen * m.m_nBytes;
	    }
	    else {
	      off += m.m_nBytes;
	    }
	    int align = (m.m_nBits + CHAR_BIT - 1) / CHAR_BIT;
	    m_currentOffset += align;
	    m_OffsetOffset = off;
	  }
	  else {
	    m_currentOffset = m.m_offset;	    
	  }
	  if ( m_unbounded ) {
	    if ( m_currentOffset >= m_maxMsgSize ) {
	      throw std::string("DDS Message size exceeds maximum");
	    }
	  }
	  return m_currentOffset;

	}
      
      private:
	int         m_maxMsgSize;
	int m_currentOffset;
	int m_OffsetOffset;
	bool m_unbounded;
      };


      class TopicManager;
      struct CbFuncs {
	gapi_copyOut co;
	gapi_copyIn  ci;
	gapi_readerCopy rc;
      };


      class RWUtil {
      public:
	void align( int n,  uint8_t *& p )
	{
	  p  = (uint8_t *)(((uintptr_t)p + (n - 1)) & ~((uintptr_t)(n)-1));
	}

	const char * ddsSubType( OU::Member &m ) {

	  if ( ! m.m_typeDef.empty() ) {
	    return  m.m_typeDef.c_str();
	  }

	  switch ( m.m_baseType ) {

	  case OCPI::API::OCPI_Type:
	    ocpiAssert(!"Not a valid type");
	    break;
	  case OCPI::API::OCPI_Struct:
	    return "c_struct";
	  case OCPI::API::OCPI_Enum:
	    return "c_enum";
	  case OCPI::API::OCPI_Long:
	    return "c_long";
	  case OCPI::API::OCPI_Short:
	    return "c_short";
	  case OCPI::API::OCPI_Bool:
	    return "c_boolean";
	  case OCPI::API::OCPI_Char:
	    return "c_char";
	  case OCPI::API::OCPI_Double:
	    return "c_double";
	  case OCPI::API::OCPI_Float:
	    return "c_float";
	  case OCPI::API::OCPI_UChar:
	    return "c_octet";
	  case OCPI::API::OCPI_ULong:
	    return "c_ulong";
	  case OCPI::API::OCPI_UShort:
	    return "c_ushort";
	  case OCPI::API::OCPI_LongLong:
	    return "c_longlong";
	  case OCPI::API::OCPI_ULongLong:
	    return "c_ulonglong";
	  case OCPI::API::OCPI_String:
	    return "c_string";
	  case OCPI::API::OCPI_none:
	  case OCPI::API::OCPI_scalar_type_limit:
	    ocpiAssert(!"These type should never be defined in the protocol");
	    return "BadBad";
	  }
	  return NULL;
	}



	c_long * toSequence( c_base base, OU::Member& m , int size, c_long length, void * buf, std::vector<std::string> & str)
	{
	  static c_type type0 = NULL;
	  c_type subtype0;
	  c_long *dest0;
	  const char* subt=ddsSubType(m);
	  subtype0 = c_type(c_metaResolve (c_metaObject(base),subt));
	  char  seq_type[128];
	  snprintf(seq_type,128,"C_SEQUENCE<%s>", subt);

#ifndef NDEBUG	
	  printf("To SEQ type = %s, sub_type = %s, len = %d, size = %d\n", seq_type, subt, length, size  );
#endif
	  
	  type0 = c_metaSequenceTypeNew(c_metaObject(base), seq_type,subtype0,0);
	  c_free(subtype0);
	  dest0 = (c_long *)c_newSequence(c_collectionType(type0),length);

	  if ( m.m_baseType == OCPI::API::OCPI_String ) {
	    c_string *dest1 = (c_string*)dest0;
	    std::vector<std::string>::iterator it;
	    int n=0;
	    for ( it=str.begin(); it!=str.end(); it++,n++ ) {
	      dest1[n] = c_stringNew( base, (*it).c_str() );
	    }
	  }
	  else {
	    if ( size ) 
	      memcpy (dest0,buf,length*size);
	  }
	  return dest0;
	}

      };



      
      class Reader : public OU::Reader , public RWUtil {

	TopicData & m_td;
	uint8_t * m_orig;
	std::stack<uint8_t*> m_source;

	struct Seq {
	  Seq( int n, int i, uint8_t** s )
	    :nElements(n),idx(i),source(s){}
	  int nElements;
	  int idx;
	  uint8_t ** source;
	};
	std::stack<Seq> m_seq;

      public:

	  Reader( TopicData & td, uint8_t * s )
	    :m_td(td),m_orig(s)  {
	    m_source.push(s);
	  }
	
	unsigned beginSequence(OU::Member &m) {

#ifndef NDEBUG
	  cout << endl << "**** In Begin Sequence !! " << m.m_name << " *** " << m_seq.size() << endl;
#else
	  (void)m;
#endif
	  align( 8,m_source.top());
	  long size;
	  c_long ** seq  = (c_long**)m_source.top();
	  size = c_arraySize(c_sequence(*seq));		    
#ifndef NDEBUG
	  cout << "Actual Sequence size = " << size << endl;
#endif
	  m_source.top() += sizeof(c_sequence);
	  m_seq.push( Seq(size,0,(uint8_t**)seq ) );
	  m_source.push( (uint8_t*)*seq );
	  return size;
	}

	void endSequence( OU::Member &) {
	  m_seq.pop();
	  m_source.pop();
	}

	void beginStruct(OU::Member &m) {
	  align( m.m_align,m_source.top());
	}

	void beginArray(OU::Member &m, uint32_t /* nItems */ ) {
	  align( m.m_align,m_source.top());
	}

	void endArray(OU::Member & ) {
	}

	unsigned beginString(OU::Member &, const char *&chars, bool /* start */ ) {
#ifndef NDEBUG
	  cout << "In beginString !!" << endl;
#endif

	  uint32_t slen;
	  if ( m_seq.size() == 0 ) { // Not within a sequence
	    align(sizeof(void*),m_source.top());
	    char** src = (char**)m_source.top();	
#ifndef NDEBUG
	  cout << "Returning " << *src << endl;
#endif
	    slen = strlen(*src);
	    chars = *src;
	    m_source.top() += sizeof(c_string*);
	  }
	  else {
	    int idx = m_seq.top().idx;
	    c_string * src = (c_string*)(*m_seq.top().source);	
	    slen = strlen(src[idx]);
	    chars = src[idx];
	    m_seq.top().idx++;
	  }
	  return slen;;
	}


	void readData(OU::Member &m, OU::ReadDataPtr p, uint32_t nBytes, uint32_t /* nElements */) {
	    uint8_t * t = p.data;

	    switch (m.m_baseType ) {
	    case OCPI::API::OCPI_Struct:
	      ocpiAssert(!"unsuporrted type for atomic read");	    
	    case OCPI::API::OCPI_Bool:
	    case OCPI::API::OCPI_Char:
	    case OCPI::API::OCPI_UChar:
	      {align(1,m_source.top()); goto processScalar;}  // for completeness
	    case OCPI::API::OCPI_Double:
	    case OCPI::API::OCPI_Float:
	    case OCPI::API::OCPI_LongLong:
	    case OCPI::API::OCPI_ULongLong:
	      {align(8,m_source.top()); goto processScalar;}
	    case OCPI::API::OCPI_Short:
	    case OCPI::API::OCPI_UShort:
	      {align(2,m_source.top()); goto processScalar;}
	    case OCPI::API::OCPI_Enum:
	    case OCPI::API::OCPI_Long:
	    case OCPI::API::OCPI_ULong:
	      {align(4,m_source.top()); goto processScalar;}

	    processScalar:
	      {
		memcpy( t, m_source.top(), nBytes); 
		m_source.top() += nBytes;
	      }
	      break;

	    case OCPI::API::OCPI_String:
	      {
		ocpiAssert(!"Should not be processing strings here");
	      }
	      break;

	      // Satisfy the compiler
	    case OCPI::API::OCPI_none:
	    case OCPI::API::OCPI_Type:
	    case OCPI::API::OCPI_scalar_type_limit:
	      ocpiAssert( !"Should never see these types in protocol spec");
	      break;
	    }
	  }
	};



      class Writer : public OU::Writer, public RWUtil  {

	TopicData & m_td;
	uint8_t * m_orig;
	uint8_t * m_target; // DDS
	c_base    m_base;
	uint32_t  m_inSequence;
	uint32_t  m_nElements;
	std::vector<std::string> m_seqStrings;
	int       m_inArray;
	struct    Seq {
	  Seq( int e, uint8_t*p)
	    :nElements(e),target(p){}
	  int     nElements;
	  uint8_t *target;
	};
	std::stack<Seq> m_seq;
	

      public:

	Writer( TopicData & td, uint8_t * t, c_base base )
	  :m_td(td),m_orig(t),m_target(t),m_base(base),m_inSequence(0),m_inArray(0){}

	// Writer
	void beginSequence(OU::Member &m, uint32_t nElements) {

	  m_inSequence++;
	  m_nElements = nElements;
#ifndef NDEBUG
	  cout << "In beginSequence: name = " << m.m_name << " count = " << nElements << endl;
	  cout << "Typdef = " << m.m_typeDef << " m_types = " << m.m_type << " members = " << m.m_nMembers << endl;
#endif
	  align(8,m_target);
	  c_long * dest=NULL;
	  if ( m.m_nMembers ) {
	    dest = toSequence( m_base, m , 0, m_nElements, NULL, m_seqStrings );
	    c_sequence * tseq = (c_sequence*)m_target;
	    *tseq = (c_sequence)dest;
	    m_target += sizeof(c_sequence);
	    m_seq.push(Seq(nElements,(uint8_t*)dest));
	  }


	}

	void beginArray(OU::Member &m, uint32_t nItems) {
	  align(m.m_align,m_target);
#ifndef NDEBUG
	  printf("Array member offset = %d, items = %d, align = %d\n", m.m_offset, nItems, m.m_align );
#else
	  (void)nItems;
#endif

	}

	void endArray(OU::Member & ) {
	  // no op
	}

	void beginStruct(OU::Member &m) {
	  align(m.m_align,m_target);
	}

	void endSequence(OU::Member &m ) {	
	  if ( m_seqStrings.size() ) {
	    ocpiAssert(  m_seqStrings.size() == m_nElements );
	    align(8,m_target);
	    c_string * dest = (c_string*)toSequence( m_base, m ,1, m_nElements, NULL, m_seqStrings );
	    c_sequence * tseq = (c_sequence*)m_target;
	    *tseq = (c_sequence)dest;
	    m_target += sizeof(c_sequence);
	    m_seqStrings.clear();
	  }
	  if ( m.m_nMembers ) {
	    m_seq.pop();
	  }
	  m_inSequence--;
	}


	void writeString(OU::Member &, OU::WriteDataPtr p, uint32_t strLen , bool /* start */, bool /* top */) {
	  (void)strLen;
	  c_char * msg = (c_char*)(p.data);
	  if ( m_inSequence ) {
	    m_seqStrings.push_back( std::string( msg ) );
	    return;	    
	  }	  
	  align(8,m_target);
	  c_string * st = (c_string*)m_target;
	  *st = c_stringNew(m_base, (c_char*)msg);	  
	  m_target += sizeof(c_string);
	}

	// Copying from p (worker buffer) to DDS
	void writeData(OU::Member &m, OU::WriteDataPtr p, uint32_t nBytes, uint32_t ) {

	  uint8_t * tf = (uint8_t*)p.data;

#ifndef NDEBUG
	  cout << "******* Writing in writeData nBytes = " << nBytes << endl;
#endif

	  uint8_t ** target;
	  if ( m_seq.empty() ) {
	    target = &m_target;
	  }
	  else {
	    target = &m_seq.top().target;
	  }
	  int bytes=0;
	  switch (m.m_baseType ) {
	  case OCPI::API::OCPI_Type:
	    ocpiAssert(!"unsuporrted type for atomic write");
	  case OCPI::API::OCPI_Bool:
	  case OCPI::API::OCPI_Char:
	  case OCPI::API::OCPI_UChar:
	    bytes = 1;
	    if ( ! m_inArray ) 
	      {align(1,*target); goto processScalar;}  // for completeness
	  case OCPI::API::OCPI_Struct:
	  case OCPI::API::OCPI_Double:
	  case OCPI::API::OCPI_Float:
	  case OCPI::API::OCPI_LongLong:
	  case OCPI::API::OCPI_ULongLong:
	    bytes = 8;
	    if ( ! m_inArray ) 
	      {align(8,*target); goto processScalar;}
	  case OCPI::API::OCPI_Short:
	  case OCPI::API::OCPI_UShort:
	    bytes = 2;
	    if ( ! m_inArray ) 
	      {align(2,*target); goto processScalar;}
	  case OCPI::API::OCPI_Enum:
	  case OCPI::API::OCPI_Long:
	  case OCPI::API::OCPI_ULong:
	    bytes = 4;
	    if ( ! m_inArray ) 
	      {align(4,*target); goto processScalar;}


	  processScalar:
	    {

	      if ( m.m_isSequence ) {

		align(m.m_align,*target);
#ifndef NDEBUG
		cout << "Seq:: bytes = " << nBytes << " elems = " << m_nElements << endl;
		cout << "RANK = " << m.m_arrayRank << endl;
#endif
		int f=1;
		for ( unsigned int y=0; y<m.m_arrayRank; y++ ) {
		  f *= m.m_arrayDimensions[y];
		}
		c_long * dest = toSequence( m_base, m , bytes*f, m_nElements, tf, m_seqStrings );
		c_sequence * tseq = (c_sequence*)*target;
		*tseq = (c_sequence)dest;
		(*target) += sizeof(c_sequence);

	      }
	      else {  // also process's arrays here
#ifndef NDEBUG
		printf("Copying data, byes align = %d, nBytes = %d\n", bytes, nBytes );
#endif
		memcpy( *target, tf, nBytes); (*target)+=nBytes;
	      }
	    }
	    break;

	  case OCPI::API::OCPI_String:
	    {
	      ocpiAssert(!"Strings should not be wrtten here");
	    }
	    break;

	    // Satisfy the compiler
	  case OCPI::API::OCPI_none:
	  case OCPI::API::OCPI_scalar_type_limit:
	    ocpiAssert( !"Should never see these types in protocol spec");
	    break;
	  }
	}
      };


      class Topic : public ::OCPI::Util::Child<TopicManager,Topic>, public  DDSEntityManager
      {
      private:
	TopicData          m_data;
	CbFuncs            m_cbFuncs;
	MsgTypeSupport_var m_mt;
	std::list<char*>   m_freeBuffers;

      public:	 
	uint8_t * align( int align, uint8_t * orig, uint8_t * cur )
	{
	  int offset = cur-orig;
	  int noffset = (offset + align - 1) & ~(align - 1);
	  return orig + noffset;
	}

	Topic( TopicManager & tm, TopicData & data  );
	inline TopicData & data(){return m_data;}
	inline CbFuncs & cbFunc(){return m_cbFuncs;} 



	// Copy from DDS
	void   copyOut( void* from, void * to ) 
	{
#ifndef NDEBUG
	  OU::Protocol & p = *m_data.m_proto;
	  p.printXML(stdout);
	  printf("Min Buffer Size: %u %u %u\n", p.m_minBufferSize, p.m_dataValueWidth, p.m_minMessageValues);

	  fflush(stdout);
#endif
	  Reader r(m_data,(uint8_t*)from);
	  m_data.m_proto->read( r, (uint8_t*)to, m_data.maxMsgSize(), 0);
	  return;
	}


	// Copy to DDS
	c_bool copyIn( c_base base, void* from, void * to )
	{
#ifndef NDEBUG
	  OU::Protocol & p = *m_data.m_proto;
	  p.printXML(stdout);
	  printf("Min Buffer Size: %u %u %u\n", p.m_minBufferSize, p.m_dataValueWidth, p.m_minMessageValues);
	  fflush(stdout);
#endif
	  Writer w(m_data,(uint8_t*)to, base);
	  m_data.m_proto->write( w, (uint8_t*)from, m_data.maxMsgSize(), 0);
	  return true;
	}

	char * getFreeBuffer() {
	  char* buf=NULL;
	  if ( ! m_freeBuffers.empty() ) {
	    buf = m_freeBuffers.front();
	    m_freeBuffers.pop_front();
	  }
	  else {
	    buf = new char[m_data.maxMsgSize()];
	  }
	  return buf;
	}

	void   dataReaderCopy (gapi_dataSampleSeq *samples, gapi_readerInfo *info)
	{
	  unsigned int i, len;
	  MsgSeq * data_seq = reinterpret_cast<MsgSeq *>(info->data_buffer);
	  ::DDS::SampleInfoSeq *info_seq = reinterpret_cast< ::DDS::SampleInfoSeq * >(info->info_buffer);
	  if (samples) {
	    len = samples->_length;
	  }
	  else {
	    len = 0;
	    data_seq->length(len);
	    info_seq->length(len);
	  }

	  if ( (info->max_samples != (gapi_unsigned_long)GAPI_LENGTH_UNLIMITED) && (len > info->max_samples) ) {
	    len = info->max_samples;
	  }
	  else if ( data_seq->maximum() > 0 && data_seq->maximum() < len ) {
	    len = data_seq->maximum();
	  }

	  if ( len > 0 ) {
	    if ( data_seq->maximum() == 0 ) {

	      //		char *dataBuf = new char[80000];

	      char *dataBuf = getFreeBuffer();

	      ::DDS::SampleInfo *infoBuf = ::DDS::SampleInfoSeq::allocbuf(len);
	      data_seq->replace(len, len, ( OpenSpliceBindings::Msg *)dataBuf, false);
	      info_seq->replace(len, len, infoBuf, false);
	      if (*(info->loan_registry) == NULL) {
		*(info->loan_registry) = gapi_loanRegistry_new();
	      }
	      gapi_loanRegistry_register((gapi_loanRegistry)*(info->loan_registry),
					 dataBuf,
					 infoBuf);
	    }
	    else
	      {
		data_seq->length(len);
		info_seq->length(len);
	      }

	    for ( i = 0; i < len; i++ ) {
	      info->copy_out ( samples->_buffer[i].data, &(*data_seq)[i] );
	      ccpp_SampleInfo_copyOut( samples->_buffer[i].info, (*info_seq)[i] );
	    }
	  }
	  info->num_samples = len;

	}

	inline std::string & module_name(){return m_data.module_name;}
	inline std::string & struct_name(){return m_data.struct_name;}
	// child class does this:	inline std::string & name(){return m_data.name;}	    

      };





      static const int CB_COUNT=50;  
      static OCPI::Msg::DDS::Topic * g_t[CB_COUNT];
#define C(v) static void co##v ( void * a1, void * a2 ){g_t[v]->copyOut(a1,a2);}
#define S(a,b,c,d,e,f,g,h,i,j)C(a)C(b)C(c)C(d)C(e)C(f)C(g)C(h) C(i) C(j)
#define M(a) S(a##0,a##1,a##2,a##3,a##4,a##5,a##6, a##7, a##71, a##72 )
      M(0)M(1)M(2)M(3)M(4)
#undef C
#undef S
#define S(a,b,c,d,e,f,g,h,i,j)C(a),C(b),C(c),C(d),C(e),C(f),C(g),C(h),C(i),C(j)
#define C(v) co##v    
	static const gapi_copyOut g_coPointers[CB_COUNT]={M(0),M(1),M(2),M(3),M(4) };
#undef C
#undef S
#undef M

#define C(v) static c_bool ci##v ( c_base base, void * a1, void * a2 ){return g_t[v]->copyIn(base,a1,a2);}
#define S(a,b,c,d,e,f,g,h,i,j)C(a)C(b)C(c)C(d)C(e)C(f)C(g)C(h) C(i) C(j)
#define M(a) S(a##0,a##1,a##2,a##3,a##4,a##5,a##6, a##7, a##71, a##72 )
      M(0)M(1)M(2)M(3)M(4)
#undef C
#undef S
#define S(a,b,c,d,e,f,g,h,i,j)C(a),C(b),C(c),C(d),C(e),C(f),C(g),C(h),C(i),C(j)
#define C(v) ci##v    
	static const gapi_copyIn g_ciPointers[CB_COUNT]={M(0),M(1),M(2),M(3),M(4) };
#undef C
#undef S
#undef M


#define C(v) static void rc##v ( gapi_dataSampleSeq * samples, gapi_readerInfo * info ){g_t[v]->dataReaderCopy(samples,info);}
#define S(a,b,c,d,e,f,g,h,i,j)C(a)C(b)C(c)C(d)C(e)C(f)C(g)C(h) C(i) C(j)
#define M(a) S(a##0,a##1,a##2,a##3,a##4,a##5,a##6, a##7, a##71, a##72 )
      M(0)M(1)M(2)M(3)M(4)
#undef C
#undef S
#define S(a,b,c,d,e,f,g,h,i,j)C(a),C(b),C(c),C(d),C(e),C(f),C(g),C(h),C(i),C(j)
#define C(v) rc##v    
	static const gapi_readerCopy g_rcPointers[CB_COUNT]={M(0),M(1),M(2),M(3),M(4) };
      int freeList[CB_COUNT];
#undef C
#undef S
#undef M

      class FuncPoolManager {
      private:

	static int nextFree() 
	{
	  for (int n=0; n<CB_COUNT; n++ ) {
	    if ( freeList[n]==0 ) {
	      freeList[n]=1;
	      return n;
	    }
	  }
	  return -1;
	}
      public:
	FuncPoolManager()
	{
	  for (int n=0; n<CB_COUNT; n++ ) {
	    freeList[n]=0;
	  }
	}

	void unRegisterCB( OCPI::Msg::DDS::Topic *  ) 
	{

	}

	static CbFuncs registerCB( OCPI::Msg::DDS::Topic * t ) 
	{
	  int index = nextFree();
	  if ( index < 0 ) {
	    throw std::string("No more DDS callbacks available");
	  }
	  g_t[index] = t;
	  CbFuncs f;
	  f.co = g_coPointers[index];
	  f.ci = g_ciPointers[index];
	  f.rc = g_rcPointers[index];
	  return f;
	}

      };

      // Singlton
      class TopicManager : public OCPI::Util::Parent<Topic> {
      private:
	std::list<Topic*> m_topics;
      public:
	TopicManager() {}
	~TopicManager() 
	{
	  m_topics.clear();
	}
	  
	static TopicManager & manager() {
	  static TopicManager * tp = NULL;
	  if ( tp == NULL ) {
	    tp = new TopicManager();
	  }
	  return *tp;
	};

	Topic * getTopic( TopicData & data ) {
	  std::list<Topic*>::iterator it;
	  for ( it=m_topics.begin(); it!=m_topics.end(); it++ ) {
	    if ( (*it)->name() == data.name ) {
	      return (*it);
	    }
	  }
	  Topic * t = new Topic( *this, data );
	  m_topics.push_back( t );
	  return t;	      
	}
      };


      Topic::Topic( TopicManager & tm, TopicData & data ) 
	: OCPI::Util::Child<TopicManager,Topic>(tm, *this, data.name.c_str()),m_data(data)
      {	
	m_cbFuncs = FuncPoolManager::registerCB( this );

	// create domain participant
	createParticipant( m_data.participant.c_str());

	// Register type
	if ( m_data.type == "" ) {
	  m_data.type = m_data.name;
	}
	m_mt = new MsgTypeSupport( this );
	registerType(m_mt.in());

	//create Topic
	createTopic(m_data.name.c_str());	  
      };


      class Buffer : public OCPI::DataTransport::BufferUserFacet {
	friend class MsgChannel;
      public:
	Buffer( void* data, uint32_t len, uint32_t tid)
	  :m_buf(data),m_bLen(len),m_dLen(0),m_tid(tid){}
	Buffer(uint32_t tid)
	  :m_buf(NULL),m_bLen(0),m_dLen(0),m_tid(tid){}
	virtual ~Buffer(){}
	inline volatile void * getBuffer(){return m_buf;}
	inline uint32_t getLength(){return m_bLen;}
	inline uint32_t getDataLength(){return m_dLen;}
	inline uint32_t opcode(){return 0;}
	inline uint32_t getTid(){return m_tid;}
      private:
	void *   m_buf;
	uint32_t m_bLen;
	uint32_t m_dLen;
	uint32_t m_tid;
	OpenSpliceBindings::MsgSeq  m_msgList;
	::DDS::SampleInfoSeq m_infoSeq;
      };

      class XferFactory;
      class XferServices;	
      class MsgChannel : public DataTransfer::Msg::TransferBase<XferServices,MsgChannel> 
      {
      private:
	Topic *   m_topic;
	EndPoint  m_ep;
	TopicData & m_td;	  
	MsgDataReader_var  m_reader;
	MsgDataWriter_var  m_writer;
	int m_bufCount;
	
	::DDS::Publisher_ptr  m_publisher;
	::DDS::Subscriber_ptr m_subscriber;
	::DDS::DataWriter_ptr m_writer_ptr;
	::DDS::DataReader_ptr m_reader_ptr;

	// Our receive buffers
	std::list<Buffer*> m_freeRcvBuffers;
	std::list<Buffer*> m_fullRcvBuffers;
	Buffer * m_currentBuffer;
	std::list<Buffer*> m_freeTxBuffers;	

      public:

	MsgChannel(XferServices & xf, const char  * url, TopicData & td,
		   const OCPI::Util::PValue *our_props=0,
		   const OCPI::Util::PValue *other_props=0 )
	  : DataTransfer::Msg::TransferBase<XferServices,MsgChannel>( xf, *this),
	    m_ep(url),m_td(td),m_bufCount(0)
	{
	  (void)our_props;
	  (void)other_props;
	  m_td.setEp( m_ep );
	  m_td.formatDDSMetaData();
	  m_topic = TopicManager::manager().getTopic( m_td );


	  //create Subscriber
	  m_subscriber = m_topic->createSubscriber();

	  // create DataReader
	  m_reader_ptr = m_subscriber->create_datareader(m_topic->getTopic().in(),
							 DATAREADER_QOS_USE_TOPIC_QOS, NULL, ::DDS::STATUS_MASK_NONE);
	  checkHandle(m_reader_ptr, "DDS::Subscriber::create_datareader ()");
	  m_reader = MsgDataReader::_narrow(m_reader_ptr);
	  checkHandle( m_reader, "MsgDataReader::_narrow");

	  // Create our message channel reader and writer
	  //create Publisher
	  m_publisher = m_topic->createPublisher();

	  // create DataWriter :
	  // If autodispose_unregistered_instances is set to true (default value),
	  // you will have to start the subscriber before the publisher
	  bool autodispose_unregistered_instances = true;
	  ::DDS::DataWriterQos dw_qos;
	  ::DDS::ReturnCode_t status;

	  status = m_publisher->get_default_datawriter_qos(dw_qos);
	  checkStatus(status, "DDS::DomainParticipant::get_default_publisher_qos");
	  status = m_publisher->copy_from_topic_qos(dw_qos, m_topic->topicQos());
	  checkStatus(status, "DDS::Publisher::copy_from_topic_qos");

	  // Set autodispose to false so that you can start
	  // the subscriber after the publisher
	  dw_qos.writer_data_lifecycle.autodispose_unregistered_instances =
	    autodispose_unregistered_instances;
	  m_writer_ptr = m_publisher->create_datawriter(m_topic->getTopic().in(), dw_qos, NULL,
							::DDS::STATUS_MASK_NONE);
	  checkHandle(m_writer_ptr, "DDS::Publisher::create_datawriter");
	  m_writer = MsgDataWriter::_narrow(m_writer_ptr);
	    
	}

	virtual ~MsgChannel()
	{

	  /*
	  try {
	    m_subscriber->delete_datareader(m_reader);
	    m_publisher->delete_datawriter(m_writer);
	    m_topic->getParticipant()->delete_subscriber(m_subscriber);
	    m_topic->getParticipant()->delete_publisher(m_publisher);

	    m_subscriber = NULL;
	    m_publisher = NULL;

	  }
	  catch ( ... ) {}
	  */
	}
	  
	void sendOutputBuffer (OCPI::DataTransport::BufferUserFacet* b, uint32_t /* msg_size */,
			       uint8_t /* opcode */)
	{
	  Buffer * buffer = static_cast<Buffer*>(b);
	  m_writer->write( buffer->m_buf, NULL);
	  m_freeTxBuffers.push_back( buffer );
	}

	bool hasFreeBuffer()
	{
	  return true;
	}

	OCPI::DataTransport::BufferUserFacet*
	getNextEmptyOutputBuffer(void *&data, uint32_t &length)
	{
	  if (!hasFreeBuffer())
	    return NULL;
	  Buffer * buf;
	  if ( ! m_freeTxBuffers.empty() ) {
	    buf = m_freeTxBuffers.front();
	    m_freeTxBuffers.pop_front();
	  }
	  else {
	    buf = new Buffer(new char[m_topic->data().maxMsgSize()], m_topic->data().maxMsgSize(), m_bufCount++);
	  }
	  if (buf) {
	    data = (void*)buf->getBuffer(); // cast off volatile
	    length = buf->getDataLength();
	  }
	  return buf;
	}


	Buffer * getFreeRcvBuffer()
	{
	  Buffer * buf;
	  if ( ! m_freeRcvBuffers.empty() ) {
	    buf = m_freeRcvBuffers.front();
	    m_freeRcvBuffers.pop_front();
	  }
	  else {
	    buf = new Buffer(new char[m_topic->data().maxMsgSize()], m_topic->data().maxMsgSize(), m_bufCount++);
	  }
	  return buf;
	}

	bool msgReady()
	{
	  if ( m_fullRcvBuffers.size() ) {
	    return true;
	  }
	  Buffer * buf = getFreeRcvBuffer();
	  ocpiAssert( buf->m_msgList.length() == 0 );
	  ::DDS::ReturnCode_t status;
	  status = m_reader->read( &buf->m_msgList, buf->m_infoSeq, 1,
				   ::DDS::NOT_READ_SAMPLE_STATE, ::DDS::ANY_VIEW_STATE, ::DDS::ALIVE_INSTANCE_STATE);
	  checkStatus(status, "msgDataReader::take");
	  if ( buf->m_msgList.length() ) {
	    ocpiAssert( buf->m_msgList.length() == 1 );
	    buf->m_dLen = buf->m_bLen = m_td.maxMsgSize();
	    buf->m_buf = &buf->m_msgList[0].data;
	    m_fullRcvBuffers.push_back( buf );
	    return true;
	  }
	  status = m_reader->return_loan(&buf->m_msgList, buf->m_infoSeq);
	  checkStatus(status, "MsgDataReader::return_loan");
	  m_freeRcvBuffers.push_back( buf );
	  return false;
	}

	void releaseInputBuffer( OCPI::DataTransport::BufferUserFacet*  buffer)
	{
	  ::DDS::ReturnCode_t status;
	  Buffer * buf = static_cast<Buffer*>(buffer);
	  status = m_reader->return_loan(&buf->m_msgList, buf->m_infoSeq);
	  checkStatus(status, "MsgDataReader::return_loan");
	  m_freeRcvBuffers.push_back( buf  );
	}
	  
	OCPI::DataTransport::BufferUserFacet*  getNextFullInputBuffer(void *&data, uint32_t &length,
								      uint8_t &opcode)
	{
	  if (!msgReady())
	    return NULL;
	  Buffer * buf = m_fullRcvBuffers.front();
	  ocpiAssert( buf );
	  m_fullRcvBuffers.pop_front();	  
	  length = buf->m_dLen;
	  opcode = 0;
	  data = (void*)buf->getBuffer(); // cast off volatile
	  return buf;
	}      

      };


      class XferServices : public DataTransfer::Msg::ConnectionBase<XferFactory,XferServices,MsgChannel>
      {
      public:
	XferServices ( const OCPI::Util::Protocol & protocol , const char  * other_url, 
		       const OCPI::Util::PValue *our_props=0,
		       const OCPI::Util::PValue *other_props=0 );

	void finalize( uint64_t cookie )
	{
	  (void)cookie;
	  // Empty
	}
	MsgChannel* getMsgChannel( const char  * url,
				   const OCPI::Util::PValue *our_props,
				   const OCPI::Util::PValue *other_props)
	{
	  return new MsgChannel( *this, url, m_td, our_props,other_props);
	}
	virtual ~XferServices ()
	{
	  // Empty 
	}

      private:
	TopicData m_td;
      };


#define DDS_DEVICE_ATTRS			\
      "ph"

      // Configuration for both the driver (defaults) and the devices
      class FactoryConfig
      {
      public:
	// Constructor happens after the driver is configured so we have
	// any XML as well as the parent's configuration available
	// The xml passed in is for the driver or device
	FactoryConfig()
	  : m_ph(1)
	{}
	void parse(FactoryConfig *defs, ezxml_t x)
	{
	  if (defs)
	    *this = *defs;
	  if (!x)
	    return;
#ifndef NDEBUG
	  printf("Processing device %s\n", ezxml_attr(x,"name") );
#endif
	  const char *err;
	  if ((err = OX::checkAttrs(x, DDS_DEVICE_ATTRS, NULL)) ||
	      (err = OX::getNumber(x, "ph", &m_ph, NULL, 0, false)) )
	    {
	      throw err; 
	    }
	}
 
	uint32_t m_ph;
      };


      class Device
	: public DataTransfer::Msg::DeviceBase<XferFactory,Device>, public FactoryConfig
      {
      public:
	Device(const char* name)
	  : DataTransfer::Msg::DeviceBase<XferFactory,Device>(name, *this)
	{

	}
	void configure(ezxml_t x);
	virtual ~Device(){}
      };

     
      class XferFactory
	: public DataTransfer::Msg::DriverBase<XferFactory, Device,XferServices,DataTransfer::Msg::msg_transfer>, public FactoryConfig
      {

      public:
	inline const char* getProtocol(){return "ocpi-dds-msg";};
	XferFactory()throw ();
	virtual ~XferFactory()throw ();
 
	void configure(ezxml_t)
	{
	  // Empty
	}
	  
	bool supportsTx( const char* url,
			 const OCPI::Util::PValue *our_props,
			 const OCPI::Util::PValue *other_props )
	{
	  (void)our_props;
	  (void)other_props;
	  if ( strncmp(getProtocol(), url, strlen(getProtocol())) == 0 ) {
	    return true;
	  }
	  return false;
	}


	virtual XferServices* getXferServices( const OCPI::Util::Protocol & protocol,
					       const char* url,
					       const OCPI::Util::PValue *our_props=0,
					       const OCPI::Util::PValue *other_props=0 )
	{

#ifndef NDEBUG
	  if ( protocol.m_name.length() )
	    cout << "Protocol name = " << protocol.m_name << endl;
	  cout << "  Operation count = " << protocol.nOperations() << endl;
	  cout << "  Op 1 info " << endl;
	  cout << "    name = " << protocol.operations()[0].name() << endl;
	  cout << "    num args = " <<  protocol.operations()[0].nArgs() << endl;
	  for ( unsigned n=0; n<protocol.operations()[0].nArgs(); n++ ) {

	    if ( protocol.operations()[0].args()[n].m_name.length() )
	      cout << n << " Name = " << protocol.operations()[0].args()[n].m_name << endl;
	    cout << "      offset =  " << protocol.operations()[0].args()[n].m_offset << endl;
	    cout << "      bits =  " << protocol.operations()[0].args()[n].m_nBits << endl;
	    cout << "      align =  " << protocol.operations()[0].args()[n].m_align << endl;
	    cout << "      nBytes =  " << protocol.operations()[0].args()[n].m_nBytes << endl;

	    cout << "      type =  " << protocol.operations()[0].args()[n].m_baseType << endl;	      
	    cout << "       seq =  " << protocol.operations()[0].args()[n].m_isSequence << endl;	      
	    cout << "       ary =  " << protocol.operations()[0].args()[n].m_arrayRank << endl;
	    cout << "       strlen =  " << protocol.operations()[0].args()[n].m_stringLength << endl;	      
	    cout << "       sequence length =  " << protocol.operations()[0].args()[n].m_sequenceLength << endl;	      
	  }
#endif

	  // We can share XferServices among threads in the same process
	  std::list<XferServices*>::iterator it;
	  for ( it=m_services.begin(); it!=m_services.end(); it++) {
	    if ( ((*it)->url() == url ) && ((*it)->protocol().m_name == protocol.m_name  ) ) {
	      return (*it);

	    }
	  }
	  XferServices * xs = new XferServices( protocol, url, our_props, other_props);
	  m_services.push_back(xs);
	  return xs;
	}

      private:
	std::list<XferServices*> m_services;

      };

	
      XferServices::
      XferServices ( const OCPI::Util::Protocol & protocol , const char  * other_url,
		     const OCPI::Util::PValue *our_props,
		     const OCPI::Util::PValue *other_props)
	: DataTransfer::Msg::ConnectionBase<XferFactory,XferServices,MsgChannel>
	  (*this, protocol,other_url,our_props,other_props)
      {
	m_td = &protocol;
      }

      XferFactory::
      XferFactory()
	throw ()
      {
	// Empty
      }

      XferFactory::	
      ~XferFactory()
	throw ()
      {
	

      }

      void
      Device::
      configure(ezxml_t x) {
	DataTransfer::Msg::Device::configure(x);
	DDS::FactoryConfig::parse(&parent(), x);
      }

      DataTransfer::Msg::RegisterTransferDriver<XferFactory> driver;

    }
  }
}



using namespace DDS;
namespace OpenSpliceBindings {

  // DDS Msg TypeSupport Object Body
  MsgTypeSupport::MsgTypeSupport( OCPI::Msg::DDS::Topic * topic ) 
    : TypeSupport_impl(
		       topic->data().type.c_str(),
		       topic->data().key.c_str(),
		       topic->data().format.c_str(),
		       (gapi_copyIn) topic->cbFunc().ci,
		       (gapi_copyOut) topic->cbFunc().co,
		       (gapi_readerCopy) topic->cbFunc().rc,
		       new  MsgTypeSupportFactory() )
  {
    // Parent constructor takes care of everything.
  }



}

#endif
