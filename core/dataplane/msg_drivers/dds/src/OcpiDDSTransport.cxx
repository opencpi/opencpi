#ifdef OPENSPLICE_MSG_SUPPORT

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
#include <map>
#include <DtExceptions.h>
#include <iostream>
#include <OcpiBuffer.h>
#include <OpenSpliceBindings.h>

namespace OX = OCPI::Util::EzXml;
using namespace std;
using namespace OpenSpliceBindings;

namespace OCPI {  
  namespace Msg {
    namespace DDS {

      class TopicData : public OCPI::Metadata::Protocol {
      private:
	int         m_maxMsgSize;
      public:
	std::string name;
	std::string module_name;
	std::string struct_name;
	std::string type;
	std::string key;
	std::string format;
	std::string participant;

	TopicData()
	  :participant("OCPI"),m_currentOffset(0),m_OffsetOffset(0),m_unbounded(false){}

	TopicData & operator=( const OCPI::Metadata::Protocol * p )
	{
	  static_cast<OCPI::Metadata::Protocol&>(*this) = *p;
	  for ( int n=0; n<nMembers(); n++ ) {
	    OCPI::Util::Prop::Member & m = member(n);
	    if ( m.type.isSequence && ( m.type.length == 0 ) ) {
	      m_unbounded = true;
	      break;
	    }
	  }
	  char * mms = getenv("OCPI_MAX_MESSAGE_SIZE");
	  if ( mms ) {
	    m_maxMsgSize = atoi(mms);
	  }
	  else {
	    m_maxMsgSize = 30000;
	  }
	  return *this;
	}


	const char * ddsType( OCPI::API::ScalarType t ) {
	  switch ( t ) {

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



	const char * ddsSubType( OCPI::API::ScalarType t ) {
	  switch ( t ) {

	  case OCPI::API::OCPI_Long:
	    return "c_long";
	  case OCPI::API::OCPI_Short:
	    return "c_short";
	  case OCPI::API::OCPI_Bool:
	    return "c_boolean";
	  case OCPI::API::OCPI_Char:
	    return "c_char";
	  case OCPI::API::OCPI_Double:
	    return "c_ouble";
	  case OCPI::API::OCPI_Float:
	    return "c_loat";
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

	c_long * toSequence( c_base base, OCPI::API::ScalarType type, int size, c_long length, void * buf )
	{
	  static c_type type0 = NULL;
	  c_type subtype0;
	  c_long *dest0;
	  const char* subt=ddsSubType(type);
	  subtype0 = c_type(c_metaResolve (c_metaObject(base),subt));
	  char  seq_type[128];
	  snprintf(seq_type,128,"C_SEQUENCE<%s>", subt);
	  type0 = c_metaSequenceTypeNew(c_metaObject(base), seq_type,subtype0,0);
	  c_free(subtype0);
	  dest0 = (c_long *)c_newSequence(c_collectionType(type0),length);
	  memcpy (dest0,buf,length*size);
	  return dest0;
	}

	void formatDDSMetaData()
	{

	  format += "<MetaData version=\"1.0.0\">";
	  char buf[1024];
	  sprintf( buf, "<Module name=\"%s\">", module_name.c_str() );
	  format += buf;
	  sprintf( buf, "<Struct name=\"%s\">", struct_name.c_str() );
	  format += buf;
	  
	  for ( int n=0; n<nMembers(); n++ ) {
	    OCPI::Util::Prop::Member & m = member(n);
	    if ( m.type.isSequence ) {
	      sprintf( buf, "<Member name=\"%s\"><Sequence><%s/></Sequence></Member>", m.name, ddsType(m.type.scalar) );
	    }
	    else if ( m.type.isArray ) {
	      sprintf( buf, "<Member name=\"%s\"><Array size=\"%d\"><%s/></Array></Member>", m.name, m.type.length, ddsType(m.type.scalar) );
	    }
	    else {
	      sprintf( buf, "<Member name=\"%s\"><%s/></Member>", m.name, ddsType(m.type.scalar) );
	    }
	    format+= buf;
	  }
	  format += "</Struct></Module></MetaData>";

#ifndef NDEBUG
	  cout << format << endl;
#endif
	}

	int maxMsgSize() {
	  if ( ! m_unbounded ) {
	    return msgFixedSize();
	  }
	  return m_maxMsgSize;
	}


	int msgFixedSize()
	{
	  int size=0;
	  for ( int n=0; n<nMembers(); n++ ) {
	    OCPI::Util::Prop::Member & m = member(n);
	    if ( m.type.isSequence || m.type.isArray ) {
	      if  (m.type.length == 0) {
		return -1;		
	      }
	      else {
		size += m.nBytes * m.type.length;
	      }
	    }
	    size += m.nBytes;
	  }
	  return size;
	}

	inline int nMembers(){return operations()[0].nArgs();}

	inline OCPI::Util::Prop::Member & member(int index) { 
	  OCPI::Util::Prop::Member &m = operations()[0].args()[index];
	  if ( m.type.isSequence && (m.type.length == 0) ) {  // Unbounded sequence
	    m_unbounded = true;
	  }
	  return m;
	}
	inline int mLength( int index, int vLen=0 ) {
	  int len;
	  OCPI::Util::Prop::Member &m = operations()[0].args()[index];
	  if ( m.type.isSequence && (m.type.length == 0) ) {  
	    len = vLen * m.nBytes;
	  }
	  else {
	    len = m.nBytes;
	  }
	  return len;
	}
	inline int mOffset( int index, int vLen=0 ) {
	  OCPI::Util::Prop::Member &m = operations()[0].args()[index];

	  if ( m_unbounded ) {
	    int off = m_OffsetOffset;
	    if ( m.type.isSequence && (m.type.length == 0) ) {  
	      off += vLen * m.nBytes;
	    }
	    else {
	      off += m.nBytes;
	    }
	    int align = (m.bits + CHAR_BIT - 1) / CHAR_BIT;
	    m_currentOffset += align;
	    m_OffsetOffset = off;
	  }
	  else {
	    m_currentOffset = m.offset;	    
	  }
	  return m_currentOffset;

	}
      
      private:
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
	  cout << offset << endl;
	  int padding = align - (offset & (align - 1));
	  cout << padding << endl;
	  int noffset = (offset + align - 1) & ~(align - 1);
	  cout << noffset << endl;
	  return orig + noffset;
	}

	Topic( TopicManager & tm, TopicData & data  );
	inline TopicData & data(){return m_data;}
	inline CbFuncs & cbFunc(){return m_cbFuncs;} 


	void   copyOut( void* from, void * to ) 
	{
	  uint8_t * orig;
	  uint8_t * tf = orig = (uint8_t*)from;
	  uint8_t * tt = (uint8_t*)to;

#ifndef NDEBUG
	  printf("Op name = %s\n", m_data.operations()[0].name().c_str() );
#endif

	  for ( int n=0; n<m_data.nMembers(); n++ ) {

#ifdef DEBUG_MEMBERS
	    if (m_data.member(n).name )
	      cout << n << " Name = " <<m_data.member(n).name << endl;
	    cout << "      offset =  " <<m_data.member(n).offset << endl;
	    cout << "      bits =  " <<m_data.member(n).bits << endl;
	    cout << "      align =  " <<m_data.member(n).align << endl;
	    cout << "      nBytes =  " <<m_data.member(n).nBytes << endl;
	    cout << "      type =  " <<m_data.member(n).type.scalar << endl;	      
	    cout << "       seq =  " <<m_data.member(n).type.isSequence << endl;	      
	    cout << "       ary =  " <<m_data.member(n).type.isArray << endl;	      
	    cout << "       strlen =  " <<m_data.member(n).type.stringLength << endl;	      
	    cout << "       length =  " <<m_data.member(n).type.length << endl;	      
#endif

	    switch (m_data.member(n).type.scalar ) {

	    case OCPI::API::OCPI_Bool:
	    case OCPI::API::OCPI_Char:
	    case OCPI::API::OCPI_UChar:
	      {tf = align(1,orig,tf); goto processScalar;}  // for completeness
	    case OCPI::API::OCPI_Double:
	    case OCPI::API::OCPI_Float:
	    case OCPI::API::OCPI_LongLong:
	    case OCPI::API::OCPI_ULongLong:
	      {tf = align(8,orig,tf); goto processScalar;}
	    case OCPI::API::OCPI_Short:
	    case OCPI::API::OCPI_UShort:
	      {tf = align(2,orig,tf); goto processScalar;}
	    case OCPI::API::OCPI_Long:
	    case OCPI::API::OCPI_ULong:
	      {tf = align(4,orig,tf); goto processScalar;}

	    processScalar:
	      {

		if ( m_data.member(n).type.isSequence ) {
		  tf = align(sizeof(void*),orig,tf);
		  long size;
		  c_long ** src = (c_long**)tf;
		  size = c_arraySize(c_sequence(*src));	
		  cout << size << endl;
		  *((uint32_t*)(tt + m_data.mOffset(n,size))) = size; // set length
		  int len =m_data.mLength(n,size);
		  int clen = size < len ? size : len;
		  clen *= m_data.mLength(n);
		  memcpy( tt + m_data.mOffset(n,size) + 4, *src, clen); 
		  tf += sizeof(c_sequence);
		}
		else if ( m_data.member(n).type.isArray ) {
		  c_long ** src = (c_long**)tf;
		  int len =m_data.mLength(n);
		  memcpy( tt + m_data.mOffset(n), *src, len); 
		  tf += sizeof(c_array);
		}
		else {
		  int len =m_data.mLength(n);
		  cout << len << endl;
		  memcpy( tt + m_data.mOffset(n) , tf, len); tf+=len;
		}

	      }
	      break;

	    case OCPI::API::OCPI_String:
	      {
		tf = align(sizeof(void*),orig,tf);
		char* dst = (char*)tt + m_data.mOffset(n);
		char** src = (char**)tf;	
		cout << *src << endl;
		uint32_t slen = strlen(*src) + 1;
		ocpiAssert( (uint32_t)m_data.mLength(n) > slen );		  
		strncpy((char*)dst,*src,slen);
		tf += sizeof(c_string);
	      }
	      break;

	      // Satisfy the compiler
	    case OCPI::API::OCPI_none:
	    case OCPI::API::OCPI_scalar_type_limit:
	      ocpiAssert( !"Should never see these types in protocol spec");
	      break;

	    }
	  }
	}

	c_bool copyIn( c_base base, void* from, void * to )
	{
	  c_bool ret = true;
	  uint8_t * orig;
	  uint8_t * tf = (uint8_t*)from;
	  uint8_t * tt = orig = (uint8_t*)to;

#ifndef NDEBUG
	  printf("Op name = %s\n", m_data.operations()[0].name().c_str() );
#endif

	  for ( int n=0; n<m_data.nMembers(); n++ ) {

#ifdef DEBUG_MEMBERS
	    if (m_data.member(n).name )
	      cout << n << " Name = " <<m_data.member(n).name << endl;
	    cout << "      offset =  " <<m_data.member(n).offset << endl;
	    cout << "      bits =  " <<m_data.member(n).bits << endl;
	    cout << "      align =  " <<m_data.member(n).align << endl;
	    cout << "      nBytes =  " <<m_data.member(n).nBytes << endl;
	    cout << "      type =  " <<m_data.member(n).type.scalar << endl;	      
	    cout << "       seq =  " <<m_data.member(n).type.isSequence << endl;	      
	    cout << "       ary =  " <<m_data.member(n).type.isArray << endl;	      
	    cout << "       strlen =  " <<m_data.member(n).type.stringLength << endl;	      
	    cout << "       length =  " <<m_data.member(n).type.length << endl;	      
#endif

	    switch (m_data.member(n).type.scalar ) {

	    case OCPI::API::OCPI_Bool:
	    case OCPI::API::OCPI_Char:
	    case OCPI::API::OCPI_UChar:
	      {tt = align(1,orig,tt); goto processScalar;}  // for completeness
	    case OCPI::API::OCPI_Double:
	    case OCPI::API::OCPI_Float:
	    case OCPI::API::OCPI_LongLong:
	    case OCPI::API::OCPI_ULongLong:
	      {tt = align(8,orig,tt); goto processScalar;}
	    case OCPI::API::OCPI_Short:
	    case OCPI::API::OCPI_UShort:
	      {tt = align(2,orig,tt); goto processScalar;}
	    case OCPI::API::OCPI_Long:
	    case OCPI::API::OCPI_ULong:
	      {tt = align(4,orig,tt); goto processScalar;}

	    processScalar:
	      {
		if ( m_data.member(n).type.isSequence ) {
		  tt = align(8,orig,tt);
		  c_long len  = (c_long)*(tf+m_data.mOffset(n));
		  cout << len <<endl;
		  ocpiAssert( len < m_data.mLength(n) );
		  c_long * dest = m_data.toSequence( base, m_data.member(n).type.scalar , sizeof(c_long), len, tf+m_data.mOffset(n)+4 );
		  c_sequence * tseq = (c_sequence*)tt;
		  *tseq = (c_sequence)dest;
		  tt += sizeof(c_sequence);
		}
		else {  // also process's arrays here
		  int len =m_data.mLength(n);
		  cout << len << endl;
		  memcpy( tt, tf + m_data.mOffset(n) ,len); tt+=len;
		}
	      }
	      break;

	    case OCPI::API::OCPI_String:
	      {
		tt = align(8,orig,tt);
		//		  uint32_t * len =  (uint32_t*)tf + m_data.mOffset(n);
		// c_char * msg = (c_char*)(len+1);
		c_char * msg = (c_char*)(tf + m_data.mOffset(n));
		c_string * st = (c_string*)tt;
		*st = c_stringNew(base, (c_char*)msg);	  
		tt += sizeof(c_string);
	      }
	      break;

	      // Satisfy the compiler
	    case OCPI::API::OCPI_none:
	    case OCPI::API::OCPI_scalar_type_limit:
	      ocpiAssert( !"Should never see these types in protocol spec");
	      break;
	    }
	  }
	  return ret;
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
	inline std::string & name(){return m_data.name;}	    

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
	: OCPI::Util::Child<TopicManager,Topic>(tm),m_data(data)
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

      struct EndPoint  {
	EndPoint( const char * ep ) 
	  : m_url(ep)
	{
	  char m[128];
	  char k[128];
	  char t[128];
	  char s[128];

	  /* ocpi-dds-msg://topic-name:key */
	  int c = sscanf(ep,"ocpi-dds-msg://%[^;];%[^;];%[^;];%s", t, m,s, k );
	  if ( c == 1 ) {
	    topic = t;	      
	  }
	  else if ( c != 4 ) {
	    fprintf( stderr, "DDS::EndPoint  ERROR: Bad DDS endpoint format (%s)\n", ep );
	    throw DataTransfer::DataTransferEx( DataTransfer::UNSUPPORTED_ENDPOINT, ep );	  
	  }
	  else {
	    topic = t;
	    module_name = m;
	    struct_name = s;
	    type = m;
	    type += "::";
	    type += s;
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
	
	::DDS::Publisher_var  m_publisher;
	::DDS::Subscriber_var m_subscriber;
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
	  : DataTransfer::Msg::TransferBase<XferServices,MsgChannel>( xf ),m_ep(url),m_td(td),m_bufCount(0)
	{
	  (void)our_props;
	  (void)other_props;
	  m_td.name = m_ep.topic;
	  m_td.key = m_ep.key;
	  m_td.type = m_ep.type;
	  m_td.module_name = m_ep.module_name;
	  m_td.struct_name = m_ep.struct_name;
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
	  m_subscriber->delete_datareader(m_reader);
	  m_publisher->delete_datawriter(m_writer);
	  m_topic->getParticipant()->delete_publisher(m_publisher.in());
	  m_topic->getParticipant()->delete_subscriber(m_subscriber);
	}
	  
	void post (OCPI::DataTransport::BufferUserFacet* b, uint32_t msg_size )
	{
	  (void)msg_size;
	  Buffer * buffer = static_cast<Buffer*>(b);
	  m_writer->write( buffer->m_buf, NULL);
	  m_freeTxBuffers.push_back( buffer );
	}

	bool hasFreeBuffer()
	{
	  return true;
	}

	OCPI::DataTransport::BufferUserFacet*  getFreeBuffer()
	{
	  Buffer * buf;
	  if ( ! m_freeTxBuffers.empty() ) {
	    buf = m_freeTxBuffers.front();
	    m_freeTxBuffers.pop_front();
	  }
	  else {
	    buf = new Buffer(new char[m_topic->data().maxMsgSize()], m_topic->data().maxMsgSize(), m_bufCount++);
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
	  status = m_reader->take( &buf->m_msgList, buf->m_infoSeq, 1,
				   ::DDS::ANY_SAMPLE_STATE, ::DDS::ANY_VIEW_STATE, ::DDS::ANY_INSTANCE_STATE);
	  checkStatus(status, "msgDataReader::take");
	  if ( buf->m_msgList.length() ) {
	    ocpiAssert( buf->m_msgList.length() == 1 );
	    buf->m_dLen = buf->m_bLen = m_td.msgFixedSize();
	    buf->m_buf = &buf->m_msgList[0].data;
	    m_fullRcvBuffers.push_back( buf );
	    return true;
	  }
	  m_freeRcvBuffers.push_back( buf );

	  // OpenSlice bug work around, same work around in all of their examples
	  os_time delay_200ms = { 0, 200000000 };
	  os_nanoSleep(delay_200ms);

	  return false;
	}

	void release( OCPI::DataTransport::BufferUserFacet*  buffer)
	{
	  ::DDS::ReturnCode_t status;
	  Buffer * buf = static_cast<Buffer*>(buffer);
	  status = m_reader->return_loan(&buf->m_msgList, buf->m_infoSeq);
	  checkStatus(status, "MsgDataReader::return_loan");
	  m_freeRcvBuffers.push_back( buf  );
	}
	  
	OCPI::DataTransport::BufferUserFacet* getNextMsg( uint32_t & length )
	{
	  if ( ! msgReady() ) {
	    return NULL;
	  }
	  Buffer * buf = m_fullRcvBuffers.front();
	  ocpiAssert( buf );
	  m_fullRcvBuffers.pop_front();	  
	  length = buf->m_dLen;
	  return buf;
	}      

      };


      class XferServices : public DataTransfer::Msg::ConnectionBase<XferFactory,XferServices,MsgChannel>
      {
      public:
	XferServices ( OCPI::Metadata::Protocol * protocol , const char  * other_url, 
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
	  : DataTransfer::Msg::DeviceBase<XferFactory,Device>(name)
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


	virtual XferServices* getXferServices( OCPI::Metadata::Protocol * protocol,
					       const char* url,
					       const OCPI::Util::PValue *our_props=0,
					       const OCPI::Util::PValue *other_props=0 )
	{

#ifndef NDEBUG
	  if ( protocol->m_name )
	    cout << "Protocol name = " << protocol->m_name << endl;
	  cout << "  Operation count = " << protocol->nOperations() << endl;
	  cout << "  Op 1 info " << endl;
	  cout << "    name = " << protocol->operations()[0].name() << endl;
	  cout << "    num args = " <<  protocol->operations()[0].nArgs() << endl;
	  for ( unsigned n=0; n<protocol->operations()[0].nArgs(); n++ ) {

	    if ( protocol->operations()[0].args()[n].name )
	      cout << n << " Name = " << protocol->operations()[0].args()[n].name << endl;
	    cout << "      offset =  " << protocol->operations()[0].args()[n].offset << endl;
	    cout << "      bits =  " << protocol->operations()[0].args()[n].bits << endl;
	    cout << "      align =  " << protocol->operations()[0].args()[n].align << endl;
	    cout << "      nBytes =  " << protocol->operations()[0].args()[n].nBytes << endl;

	    cout << "      type =  " << protocol->operations()[0].args()[n].type.scalar << endl;	      
	    cout << "       seq =  " << protocol->operations()[0].args()[n].type.isSequence << endl;	      
	    cout << "       ary =  " << protocol->operations()[0].args()[n].type.isArray << endl;	      
	    cout << "       strlen =  " << protocol->operations()[0].args()[n].type.stringLength << endl;	      
	    cout << "       length =  " << protocol->operations()[0].args()[n].type.length << endl;	      
	  }
#endif

	  // We can share XferServices among threads in the same process
	  std::list<XferServices*>::iterator it;
	  for ( it=m_services.begin(); it!=m_services.end(); it++) {
	    if ( ((*it)->url() == url ) && ((*it)->protocol()->m_name == protocol->m_name  ) ) {
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
      XferServices ( OCPI::Metadata::Protocol * protocol , const char  * other_url,
		     const OCPI::Util::PValue *our_props,
		     const OCPI::Util::PValue *other_props)
	: DataTransfer::Msg::ConnectionBase<XferFactory,XferServices,MsgChannel>(protocol,other_url,our_props,other_props)
      {
	m_td = protocol;
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
	FactoryConfig::parse(&parent(), x);
      }


#ifndef OCPI_OS_darwin
      DataTransfer::Msg::RegisterTransferDriver<XferFactory> driver;
#endif

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
