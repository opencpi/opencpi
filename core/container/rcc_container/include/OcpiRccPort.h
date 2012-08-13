
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
 *   This file contains the interface and data structures for the JTRS DSP Worker.
 *
 * Revision History: 
 *
 *    06/23/09  John Miller
 *    Added code to handle RCC_ERROR and RCC_FATAL return codes.
 * 
 *    06/01/05  John Miller
 *    Initial revision
 *
 */


#ifndef OCPI_RCC_PORT_H_
#define OCPI_RCC_PORT_H_

#ifndef WORKER_INTERNAL
#define WORKER_INTERNAL
#endif

#include <OcpiOsDataTypes.h>
#include <OcpiContainerDataTypes.h>
#include <OcpiContainerPort.h>
#include <OcpiRccApplication.h>
#include <OcpiBuffer.h>
#include <RCC_Worker.h>


namespace DataTransfer {
  namespace Msg {
    class MsgChannel;
  }
}

namespace OCPI {

  namespace DataTransport {
    class Buffer;
    class Port;
  }


  namespace RCC {

    class Application;
    class Worker;
    class ExternalPort;
    class ExternalBuffer;


    // This port does the actual work;
    class Port;
    class PortDelegator : public OCPI::Container::PortBase<Worker, Port, ExternalPort> {
    public:
      PortDelegator(  Worker& w, const OCPI::Metadata::Port & pmd, unsigned xferOptions,
		      Port *delegator, const OCPI::Util::PValue *params);
      virtual ~PortDelegator();

      virtual inline OCPI::Container::PortConnectionDesc &  getData() {
	return m_delegator ? m_delegator->getData() : OCPI::Container::PortData::getData();
      }
      virtual void startConnect(const OCPI::RDT::Descriptors *, const OCPI::Util::PValue *) {};
      virtual void setMode( ConnectionMode ){}
      virtual void connectInside( OCPI::Container::Port &, const OCPI::Util::PValue *){}
      const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors &/*other*/,
		    OCPI::RDT::Descriptors &/*feedback*/) { return NULL;}
      virtual bool definitionComplete(){return true;}

      virtual inline OCPI::OS::Mutex               &mutex() { return m_mutex;}
      inline OCPI::Metadata::PortOrdinal           portOrdinal(){return m_portOrdinal;}
      inline bool isOutput() {return !m_metaPort.provider;}

      // Port control methods // FIXME: make these pure virtual, but that requires other changes...
      virtual void releaseInputBuffer( OCPI::DataTransport::BufferUserFacet *)
      { ocpiAssert(0); }
      virtual void sendOutputBuffer( OCPI::DataTransport::BufferUserFacet* , uint32_t, uint8_t )
      {ocpiAssert(0);}
      virtual OCPI::DataTransport::BufferUserFacet* getNextEmptyOutputBuffer(void *&, uint32_t &)
      {ocpiAssert(0); return NULL;}	
      virtual OCPI::DataTransport::BufferUserFacet* getNextFullInputBuffer(void *&, uint32_t &, uint8_t &)
      {ocpiAssert(0); return NULL;}	
      virtual void sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* ,
					 unsigned int , uint8_t)
      {ocpiAssert(0);}
      virtual uint32_t getBufferCount()=0;
      virtual OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t tid )=0;
    protected:
      OCPI::Metadata::PortOrdinal  m_portOrdinal;
      OCPI::OS::Mutex &m_mutex;
      Port *m_delegator;

    };

    class RDMAPort;
    class Port : public PortDelegator  {
      friend class ExternalPort;
    public:

      Port( Worker& w, const OCPI::Metadata::Port & pmd, const OCPI::Util::PValue *params, RCCPort *rp);
      virtual ~Port();

      virtual inline OCPI::OS::Mutex &mutex() 
      { 
	if (m_delegateTo) {
	  return m_delegateTo->mutex();
	}
	return mutex();
      }

#if 0
      // JEK: inverted the sense of this: Port has the descriptor, delegatee does not.
      virtual inline OCPI::Container::PortConnectionDesc &  getData()
      {
	// FIXME:  we are relying on m_params being reparsed after delegation, which is weak...
	// ocpiAssert( m_delegateTo );
	return m_delegateTo ? m_delegateTo->getData() : PortData::getData();
      }
#endif
      bool isLocal() const { return true; }
      void setMode( ConnectionMode mode );
      virtual void connect( OCPI::Container::Port &other, const OCPI::API::PValue *myProps=NULL,
			    const OCPI::API::PValue *otherProps=NULL);

      virtual inline void connectURL( const char* url, const OCPI::Util::PValue *myProps,
				      const OCPI::Util::PValue * otherProps)
      {

	setMode( OCPI::Container::Port::CON_TYPE_MESSAGE );
	m_delegateTo->connectURL( url, myProps, otherProps );
      }

      virtual inline void disconnect()
        throw ( OCPI::Util::EmbeddedException )
      {
	if(m_delegateTo) {
	  m_delegateTo->disconnect();
	}
      }

#if 1
      OCPI::Container::ExternalPort &createExternal(const char *extName, bool provider,
						    const OCPI::Util::PValue *extParams,
						    const OCPI::Util::PValue *connParams);
#else
      virtual OCPI::API::ExternalPort& connectExternal(const char* n, const OCPI::Util::PValue* p,
						       const OCPI::Util::PValue* po);
#endif

      virtual inline bool definitionComplete() {
	if (m_delegateTo) {
	  return m_delegateTo->definitionComplete();
	}
	return false;
      }
      void localConnect(OCPI::DataTransport::Port &input)
      {
      if ( ! m_delegateTo ) setMode( CON_TYPE_RDMA );
	if (m_delegateTo) {
	  m_delegateTo->localConnect(input);
	}
      }
      OCPI::DataTransport::Port &dtPort() { 
      if ( ! m_delegateTo ) setMode( CON_TYPE_RDMA );
      ocpiAssert(m_delegateTo);
      return m_delegateTo->dtPort();
      }
      virtual inline void startConnect(const OCPI::RDT::Descriptors *other, const OCPI::Util::PValue* params)
      {
      if ( ! m_delegateTo ) setMode( CON_TYPE_RDMA );
	if (m_delegateTo) {
	  m_delegateTo->startConnect(other, params);
	}
      }
      virtual inline void connectInside(OCPI::Container::Port & other, const OCPI::Util::PValue * my_props)
      {
	ocpiAssert ( m_delegateTo);
	m_delegateTo->connectInside(other,my_props);
      }
      const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors &other,
		    OCPI::RDT::Descriptors &feedback)
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->finishConnect(other, feedback);
      }

#if 0
      // Connection routines
      virtual inline void getInitialProviderInfo(const OCPI::API::PValue*p, std::string &out)
      {
	// only supported in RDMA mode
	if ( ! m_delegateTo ) setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	return m_delegateTo->getInitialProviderInfo(p, out);
      }
      virtual inline void setInitialProviderInfo(const OCPI::API::PValue*p, const std::string&s, std::string &out)
      {
	// only supported in RDMA mode
	if ( ! m_delegateTo ) setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	return m_delegateTo->setInitialProviderInfo(p,s, out);
      }
      virtual inline void setInitialUserInfo(const std::string&s, std::string &out) 
      {
	// only supported in RDMA mode
	if ( ! m_delegateTo ) setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	return m_delegateTo->setInitialUserInfo(s, out);
      }
      virtual inline void setFinalProviderInfo(const std::string&s, std::string &out) 
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->setFinalProviderInfo(s, out);
      }
      virtual inline void setFinalUserInfo(const std::string&s)
      {
	ocpiAssert ( m_delegateTo);
	m_delegateTo->setFinalUserInfo(s);
      }
#endif
      // Port control methods
      virtual inline OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t tid )
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->getBuffer( tid );
      }

      virtual inline uint32_t getBufferCount()
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->getBufferCount();
      }

      void release( OCPI::DataTransport::BufferUserFacet* buffer);

      // Inlined since there is only one caller, that cannot be inline
      inline void take( RCCBuffer *oldBuffer, RCCBuffer *newBuffer)
      {
	*newBuffer = m_rccPort->current;
	m_rccPort->current.data = NULL;
	m_buffer = NULL;
	if ( oldBuffer ) {
	  OCPI::DataTransport::BufferUserFacet* old = oldBuffer->containerBuffer;
	  Port *bp = static_cast<Port *>(old->m_ud);
	  bp->release(old);
	}
	request();
      }
      // return true if we are ready, and try to make us ready in the process
      inline bool checkReady() {
	return m_buffer ? true : (m_wantsBuffer ? request() : false);
      }
      bool request();

      bool advance();

      void send( OCPI::DataTransport::BufferUserFacet* buffer, uint32_t length,
		 uint8_t opcode = 0 );
      inline RDMAPort &getRDMAPort();
    protected:


      // Our initial properties
      const OCPI::Util::PValue * m_params;

      PortDelegator *  m_delegateTo;

      ConnectionMode m_mode;
      // This flag indicates that this port wants a new buffer, i.e. has requested one and not gotten one
      // Thus there are three valid states:
      //   m_wantsBuffer == true: we don't have a buffer, and we want one. m_buffer is false
      //   m_wantsBuffer == false && !m_buffer: we don't have a buffer, and don't want one
      //   m_wantsBuffer == false && m_buffer: we have a buffer
      //   invalid: m_wantsBuffer && m_buffer
      //  The initial state is m_wantsBuffer == true, which implies that there is no way for a worker
      //  to start out NOT requesting any buffers...Someday that should be an option:  i.e. like
      //  optionally connected ports, you have optionally requested ports so that no buffer resources
      //  are used on a port until you specifically request buffers.
      bool                                  m_wantsBuffer;
      OCPI::DataTransport::BufferUserFacet *m_buffer;
      RCCPort                              *m_rccPort;
    };


    class MessagePort : public PortDelegator  {    

    public:
      MessagePort( Worker &w, Port& p, const OCPI::Util::PValue *params);
      
      void connectURL( const char* url, const OCPI::Util::PValue *myParams,
		       const OCPI::Util::PValue *otherParams);
      inline void disconnect()
        throw ( OCPI::Util::EmbeddedException );
      bool isLocal() const { return false; }
#if 1
      OCPI::Container::ExternalPort &createExternal(const char *extName, bool provider,
						    const OCPI::Util::PValue *extParams,
						    const OCPI::Util::PValue *connParams);
#else
      OCPI::Container::ExternalPort& connectExternal(const char*, const OCPI::Util::PValue*,
						     const OCPI::Util::PValue*);
#endif
      void releaseInputBuffer(OCPI::DataTransport::BufferUserFacet * buffer);
      void sendOutputBuffer( OCPI::DataTransport::BufferUserFacet* buffer, uint32_t length, uint8_t opcode );
      OCPI::DataTransport::BufferUserFacet*  getNextEmptyOutputBuffer(void *&data, uint32_t &length);
      OCPI::DataTransport::BufferUserFacet*  getNextFullInputBuffer(void *&data,
								    uint32_t &length, uint8_t &opcode);
      void sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* src_buf, unsigned int len, uint8_t op );
      OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t index );
      //      uint32_t getBufferLength();
      uint32_t getBufferCount();

      virtual ~MessagePort();
    private:
      DataTransfer::Msg::MsgChannel *m_msgChannel;
    };


    class RDMAPort : public PortDelegator  {
    public:

      friend class Port;

      RDMAPort( Worker &w, Port& p, const OCPI::Util::PValue *params);
      virtual ~RDMAPort();

      void disconnect()
        throw ( OCPI::Util::EmbeddedException );

      // Connection routines
#if 0
      void
	getInitialProviderInfo(const OCPI::API::PValue*, std::string &out),
	setInitialProviderInfo(const OCPI::API::PValue*, const std::string&, std::string &out),
	setInitialUserInfo(const std::string&, std::string &out),
	setFinalProviderInfo(const std::string&, std::string &out),
	setFinalUserInfo(const std::string&);
#endif
      bool isLocal() const { return true; }
      void localConnect(OCPI::DataTransport::Port &input);
      OCPI::DataTransport::Port &dtPort(){ ocpiAssert(m_dtPort); return *m_dtPort;}
      void startConnect(const OCPI::RDT::Descriptors *, const OCPI::Util::PValue * my_props);
      void connectInside(OCPI::Container::Port & other, const OCPI::Util::PValue * my_props);
      const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors &/*other*/,
		    OCPI::RDT::Descriptors &/*feedback*/);
      OCPI::Container::ExternalPort &createExternal(const char *extName, bool provider,
						    const OCPI::Util::PValue *extParams,
						    const OCPI::Util::PValue *connParams);
#if 0
      OCPI::Container::ExternalPort& connectExternal(const char*, const OCPI::Util::PValue*,
      						     const OCPI::Util::PValue*);
#endif
      virtual void connect( OCPI::Container::Port &other, const OCPI::API::PValue *myProps=NULL,
			    const OCPI::API::PValue *otherProps=NULL);        
      virtual void connectURL( const char* url, const OCPI::Util::PValue *myProps,
			       const OCPI::Util::PValue *otherProps);
      bool definitionComplete();
      
      // Port Control methods      
      void releaseInputBuffer(OCPI::DataTransport::BufferUserFacet * buffer);
      void sendOutputBuffer(OCPI::DataTransport::BufferUserFacet * buffer, uint32_t length, uint8_t opcode );
      OCPI::DataTransport::BufferUserFacet* getNextEmptyOutputBuffer(void *&data, uint32_t &length);
      OCPI::DataTransport::BufferUserFacet* getNextFullInputBuffer(void *&data, uint32_t &length,
									   uint8_t &opcode);
      void sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* src_buf, unsigned int len, uint8_t op );
      OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t index );
      //      uint32_t getBufferLength();
      uint32_t getBufferCount();
      
    private:

      //      void initInputPort(const OCPI::Util::PValue *);
      void processPortProperties( const OCPI::Util::PValue* props );
      void disconnectInternal();

      OCPI::DataTransport::Port *m_dtPort;
      RDMAPort *m_localOther; // a connected local (same container) port.
    };

    inline RDMAPort &Port::getRDMAPort() {
      ocpiAssert(m_delegateTo && m_mode == CON_TYPE_RDMA);
      return *static_cast<RDMAPort*>(m_delegateTo);
    }

  }

}
#endif
