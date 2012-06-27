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
 *   Container application context class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */
#define WORKER_INTERNAL
#include <OcpiRccPort.h>
#include <OcpiRccWorker.h>
#include <OcpiRccContainer.h>
#include <OcpiRccApplication.h>
#include <OcpiOsMisc.h>
#include <OcpiTransport.h>
#include <OcpiRDTInterface.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilCDR.h>
#include <OcpiPortMetaData.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiContainerErrorCodes.h>
#include <OcpiPValue.h>
#include <OcpiUtilMisc.h>
#include <OcpiParentChild.h>
#include <OcpiBuffer.h>

#include <DtTransferInternal.h>

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OM = OCPI::Metadata;
namespace OCPI {
  namespace RCC {

    void 
    RDMAPort::
    initInputPort(const OU::PValue *params)
    {
      ocpiAssert(!m_dtPort);
      m_dtPort = parent().getTransport().createInputPort(getData().data, params );
      std::string n = parent().implTag() + "_";
      n += parent().instTag() + "_";
      n +=  m_metaPort.name;
      m_dtPort->setInstanceName( n.c_str() );
      parent().portIsConnected(portOrdinal());
    }

    RDMAPort::
    RDMAPort(Worker &w, Port& p,  const OU::PValue *params)
      :  PortDelegator(w, p.metaPort(),
		       (1 << OCPI::RDT::ActiveFlowControl) | (1 << OCPI::RDT::ActiveMessage), // options
		       &p, params),
	 m_dtPort(NULL), m_localOther(NULL)
    {
    }

    RDMAPort::
    ~RDMAPort()
    {
      disconnect();
    }

    void 
    RDMAPort::
    connectInside(OC::Port & other, const OU::PValue */*myParams*/,  const OU::PValue *otherParams)
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      RDMAPort &inputPort = static_cast<OCPI::RCC::Port *>(&other)->getRDMAPort();

      // Setup the input port, given connect-time parameters for the transport subsystem
      // and enhancing (but NOT finalizing) the descriptor based on transport/protocol issues.
      inputPort.initInputPort(otherParams);
      ocpiAssert(!m_dtPort);
      // Setup the output port, providing input port info, but NOT finalizing
      m_dtPort = parent().getTransport().createOutputPort(getData().data, *inputPort.dtPort() );

      std::string n = parent().implTag() + "_";
      n += parent().instTag() + "_";
      n +=  m_metaPort.name;
      m_dtPort->setInstanceName( n.c_str() );

      // Perform the final negotiation between the input side with all its
      determineRoles(inputPort.getData().data);
      // Tell the input port to finalize
      inputPort.dtPort()->finalize(getData().data, inputPort.getData().data );
      // Tell the output port to finalize
      m_dtPort->finalize(inputPort.getData().data, getData().data );
      m_localOther = &inputPort;
      inputPort.m_localOther = this;
      parent().portIsConnected(portOrdinal());
    }


    void
    RDMAPort::
    getInitialProviderInfo(const OU::PValue* params, std::string &out)
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( isProvider() );
      applyConnectParams(params);
      initInputPort(params);
      OC::Container::packPortDesc(getData(), out);
    }

    // An output port is being told of an input port's initial (perhaps sufficient) info
    // FIXME: share more code with the generic implementation
    void
    RDMAPort::
    setInitialProviderInfo(const OU::PValue* params, const std::string & inputInfo, std::string &out )
    {
      OU::AutoMutex guard ( m_mutex,  true ); 

      ocpiAssert( ! isProvider() );
      applyConnectParams(params);
      OC::PortConnectionDesc inputPortData;
      OC::Container::unpackPortDesc(inputInfo, inputPortData);
      OC::PortConnectionDesc localShadowPort;
      ocpiAssert(!m_dtPort);
      m_dtPort = parent().getTransport().createOutputPort(getData().data, inputPortData.data );

      std::string n = parent().implTag() + "_";
      n += parent().instTag() + "_";
      n +=  m_metaPort.name;
      m_dtPort->setInstanceName( n.c_str() );

      determineRoles(inputPortData.data);
      finishConnection(inputPortData.data);
      m_dtPort->finalize(inputPortData.data, getData().data, &localShadowPort.data );
      parent().portIsConnected(portOrdinal());
      // Fill in our container-port reference and our container reference
      OC::Container::packPortDesc(getData().data.role == OCPI::RDT::ActiveMessage ?
				  localShadowPort : getData(), out);      
    }

    // An output port is being told about an input port's final info
    void
    RDMAPort::
    setFinalProviderInfo(const std::string & input_port, std::string &out )
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( ! isProvider() );
      OC::PortConnectionDesc tpdata;
      OC::Container::unpackPortDesc(input_port, tpdata);

      ocpiAssert(m_dtPort);
      m_dtPort->finalize(tpdata.data, getData().data );
      OC::Container::packPortDesc(getData(), out);
    }


    // An input/provider port is being told about the output/user port
    // getInitialProviderInfo has already been called on this provider/input
    void
    RDMAPort::
    setInitialUserInfo(const std::string& user, std::string &out)
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( isProvider() );
      OC::PortConnectionDesc src;
      if ( !OC::Container::unpackPortDesc( user, src )) {
	throw OU::EmbeddedException("Input Port descriptor is invalid");
      }
      m_dtPort->finalize( src.data, getData().data );
      out.clear();
      //OC::Container::packPortDesc(getData(), out);
    }

    void 
    RDMAPort::
    setFinalUserInfo(const std::string& srcPort )
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( isProvider() );
      OC::PortConnectionDesc src;
      if ( !OC::Container::unpackPortDesc( srcPort, src )) {
	throw OU::EmbeddedException("Input Port descriptor is invalid");
      }
    }

    class ExternalBuffer : public OC::ExternalBuffer {
    public:
      // For consumer buffers
      void release();
      // For producer buffers
      void put(uint32_t length, uint8_t opCode, bool endOfData);

    private:
      friend class ExternalPort;
      OCPI::DataTransport::BufferUserFacet  *m_buffer;
      Port                                 *m_port;
      OS::Mutex                            *m_mutex; // not a reference since we construct as an array
    };

    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
    public:
      friend class ExternalBuffer;
      ExternalPort(Port &p, const char* name, const OU::PValue *props, const OM::Port &mPort, OS::Mutex & mutex ) 
	: OC::ExternalPortBase<Port,ExternalPort>(p, name, props, mPort, !p.isProvider()),
	  m_nBuffers(p.getBufferCount()), m_currentIdx(0), m_mutex(mutex)
      {
	// Use an array here for scalability
	m_exBuffers = new ExternalBuffer[m_nBuffers];
	for (unsigned int n = 0; n < m_nBuffers; n++ ) {
	  m_exBuffers[n].m_buffer = p.getBuffer(n);
	  m_exBuffers[n].m_port = &p;
	  m_exBuffers[n].m_mutex = &m_mutex;
	}
      }
      OC::ExternalBuffer *getBuffer(uint8_t *&data, uint32_t &length, uint8_t &opCode, bool &endOfData);
      OC::ExternalBuffer *getBuffer(uint8_t *&data, uint32_t &length);
      void endOfData();
      bool tryFlush();
      virtual ~ExternalPort(){
	delete [] m_exBuffers;
      }


    private:
      // External Buffers
      ExternalBuffer* m_exBuffers; // an array
      unsigned m_nBuffers, m_currentIdx;
      OCPI::OS::Mutex & m_mutex;     
    };

    void 
    ExternalBuffer::
    release()
    {
      OU::AutoMutex guard (*m_mutex, true); 
      m_port->release(m_buffer);
    }

    // For producer buffers
    void 
    ExternalBuffer::
    put(uint32_t length, uint8_t opCode, bool endOfData)
    {        
      OU::AutoMutex guard ( *m_mutex, true ); 
      // FIXME:  check for value opcode and length values.
      if ( endOfData ) length = 0;
      m_port->send(m_buffer, length,  opCode);
    }


    // This port is an input port to the external user and an output port to the worker,
    // so the worker produces data and the external user gets it
    OC::ExternalBuffer *
    ExternalPort::
    getBuffer(uint8_t *&data, uint32_t &length, uint8_t &opCode, bool &endOfData)
    {
      OU::AutoMutex guard ( m_mutex, true ); 
      Port &p = parent();
      if (p.request()) {
	RCCPort &rp = *p.m_rccPort;
	data = (uint8_t*)rp.current.data;
	// start code different from getBuffer below
	opCode = rp.input.u.operation;
	length = rp.input.length;
	endOfData = (length == 0) ? true : false;
	// end code different from getbuffer above
	if (m_currentIdx >= m_nBuffers)
	  m_currentIdx = 0;
	ExternalBuffer &ex = m_exBuffers[m_currentIdx];
	ex.m_buffer = rp.current.containerBuffer;
	return &ex;
      }
      return NULL;
    }


    // This port is an output port to the external user and an input port to the worker
    // so the external user produces data and 
    OC::ExternalBuffer * 
    ExternalPort::
    getBuffer(uint8_t *&data, uint32_t &length)
    {
      OU::AutoMutex guard ( m_mutex, true ); 
      Port &p = parent();
      if (p.request()) {
	RCCPort &rp = *p.m_rccPort;
	data = (uint8_t*)rp.current.data;
	// start code different from getBuffer above
	length = rp.current.maxLength;
	// end code different from getbuffer above
	if (m_currentIdx >= m_nBuffers)
	  m_currentIdx = 0;
	ExternalBuffer &ex = m_exBuffers[m_currentIdx];
	ex.m_buffer = rp.current.containerBuffer;
	return &ex;
      }
      return NULL;
    }

    void 
    ExternalPort::
    endOfData()
    {
      // ??
    }

    bool 
    ExternalPort::
    tryFlush()
    {
      return false; // there are no more buffers to send on this external port that I can send...
    }


    /*
     * The following code is generated by the tool
     */
#include <RCC_Worker.h>
    static 
    RCCResult stub_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
    { 
      ( void ) this_;
      ( void ) timedout; 
      ( void ) newRunCondition; 
      return RCC_DONE;
    }
    static RCCDispatch stub_dispatch = { RCC_VERSION, 1, 1,
					 0, 0, 0,
					 0, 
					 0, 
					 0, 
					 0, 
					 0, 
					 0, 
					 0, 
					 stub_run,
					 0, 
					 0, 
					 0};

    OC::ExternalPort& 
    RDMAPort::
    connectExternal(const char *, const OU::PValue *, const OU::PValue *)
    {
      ocpiAssert(!"unexpected");
      return *(OC::ExternalPort *)0;
    }

    // Discard the buffer (input only for now)
    void
    RDMAPort::
    releaseInputBuffer( OCPI::DataTransport::BufferUserFacet* b)
    {
      ocpiAssert(b);
      ocpiAssert(isProvider()); // we don't support releasing (and not sending) output buffers
      ocpiAssert(m_dtPort);
      try {
	m_dtPort->releaseInputBuffer(b);
      } catch (std::string &e) {
	parent().portError(e);
      }
    }
    
    void
    RDMAPort::
    sendOutputBuffer(OCPI::DataTransport::BufferUserFacet* b, uint32_t length, uint8_t opCode )
    {
      try {
	m_dtPort->sendOutputBuffer(b, length, opCode);
      } catch (std::string &e) {
	parent().portError(e);
      }
    }

    OCPI::DataTransport::BufferUserFacet    *
    RDMAPort::
    getNextFullInputBuffer(void *&data, uint32_t &length, uint8_t &opcode )
    {
      OCPI::DataTransport::BufferUserFacet *b = NULL;
      try {
	b = m_dtPort->getNextFullInputBuffer(data, length, opcode);
      } catch (std::string &e) {
	parent().portError(e);
      }
      return b;
    }
    // We are being told by our local peer that they are being disconnected.
    void 
    RDMAPort::
    disconnectInternal( ) {
      TRACE(" OCPI::RCC::RDMAPort::disconnectInternal()");
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert(m_localOther);
      if (!isProvider() && m_dtPort)
	// If we are output, we are going first.  We take down our dtport.
	m_dtPort->reset();
      m_dtPort = NULL;
      parent().m_context->connectedPorts &= ~(1<<m_portOrdinal);
      m_localOther = NULL;
    }
    void 
    RDMAPort::
    disconnect( )
      throw ( OU::EmbeddedException )
    {
      TRACE(" OCPI::RCC::RDMAPort::disconnect()");
      OU::AutoMutex guard ( m_mutex,  true ); 
      
      // Always output before input
      if (!isProvider()) {
	// We are output
	if (m_dtPort)
	  m_dtPort->reset();
	if (m_localOther)
	  m_localOther->disconnectInternal();
      } else if (m_localOther)
	// Input with a peer - they go first
	m_localOther->disconnectInternal();
      else if (m_dtPort)
	// Input without a pear
	m_dtPort->reset();
      m_localOther = NULL;
      m_dtPort = 0;
      parent().m_context->connectedPorts &= ~(1<<m_portOrdinal);
    }

    void 
    RDMAPort::
    sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* buf, unsigned int len, uint8_t op )
    {
      try {
	m_dtPort->sendZcopyInputBuffer( static_cast<OCPI::DataTransport::Buffer*>(buf), len, op);
      } catch (std::string &e) {
	parent().portError(e);
      }
    }

    OCPI::DataTransport::BufferUserFacet*
    RDMAPort::
    getNextEmptyOutputBuffer(void *&data, uint32_t &length)
    {
      OCPI::DataTransport::BufferUserFacet *b = NULL;
      try {
	b = m_dtPort->getNextEmptyOutputBuffer(data, length);
      } catch (std::string &e) {
	parent().portError(e);
      }
      return b;
    }

    bool 
    RDMAPort::
    definitionComplete()
    {
      return m_dtPort && m_dtPort->isFinalized();
    }

    // Just guard the generic connect method with our mutex
    void 
    RDMAPort::
    connect( OC::Port &other, const OU::PValue *myProps, const OU::PValue *otherProps) {
      OU::AutoMutex guard ( m_mutex,  true ); 
      return OC::Port::connect(other, myProps, otherProps);
    }

    OCPI::DataTransport::BufferUserFacet* 
    RDMAPort::
    getBuffer( uint32_t index )
    {
      return m_dtPort->getBuffer(index);
    }

#if 0
    uint32_t 
    RDMAPort::
    getBufferLength()
    {
      return m_dtPort->getBufferLength();
    }
#endif
    uint32_t 
    RDMAPort::
    getBufferCount() 
    {
      return getData().data.desc.nBuffers;
    };

    void 
    RDMAPort::
    connectURL( const char*, const OCPI::Util::PValue *, const OCPI::Util::PValue *)
    {    
      ocpiAssert( 0 );
    }

    Port::
    Port( Worker& w, const OCPI::Metadata::Port & pmd, const OCPI::Util::PValue *params, RCCPort *rp)
      : PortDelegator( w, pmd, 0, NULL, params), m_params(params), m_delegateTo(NULL),
	m_mode(OC::Port::CON_TYPE_NONE), m_wantsBuffer(true), m_buffer(NULL), m_rccPort(rp)

    {
      // FIXME: deep copy params
    }

    void 
    Port::
    setMode( ConnectionMode mode )
    {
      if (mode != m_mode) {
	if ( m_delegateTo ) delete m_delegateTo;
	if ( mode == CON_TYPE_RDMA ) {
	  m_delegateTo = new RDMAPort( parent(), *this, m_params );
	}
	else {
	  m_delegateTo = new MessagePort( parent(), *this, m_params );
	}
	m_mode = mode;
      }
    }

    void 
    Port::
    connect( OCPI::Container::Port &other, const OCPI::API::PValue *myParams,
	     const OCPI::API::PValue *otherParams)
    {
      if ( ! m_delegateTo ) setMode( CON_TYPE_RDMA );
      m_delegateTo->connect(other,myParams,otherParams);
    }

    // NOTE: this cannot be delegated because it passes "this" to connect, and that must be
    // a top level port.
    OCPI::API::ExternalPort&
    Port::
    connectExternal(const char* name, const OCPI::Util::PValue* myParams, const OCPI::Util::PValue* otherParams)
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      // only supported in RDMA mode
      if ( ! m_delegateTo ) {
	setMode( OCPI::Container::Port::CON_TYPE_RDMA );
      }
      
      // We will create a single port worker to do this so that we can use the mechanics of the 
      // underlying system
      OC::Worker & w = *new Worker(parent().parent(), NULL,
				   (const char *)&stub_dispatch, NULL, NULL, NULL);
      OCPI::RCC::Port * p;
      if ( isProvider() ) {
	p = static_cast<OCPI::RCC::Port*>
	  (&w.createOutputPort(0, getData().data.desc.nBuffers,
			       getData().data.desc.dataBufferSize,NULL ));
	p->setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	p->connect( *this, myParams, otherParams );
      }
      else {
	p = static_cast<OCPI::RCC::Port*>
	  (&w.createInputPort(0, 
			      getData().data.desc.nBuffers,
			      getData().data.desc.dataBufferSize,NULL ));
	p->setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	this->connect( *p, myParams, otherParams );
      }
      return *new ExternalPort(*p, name, NULL, metaPort(), m_mutex);
    }

    void Port::
    release( OCPI::DataTransport::BufferUserFacet* buffer)
    {
      ocpiAssert(m_delegateTo);
      ocpiAssert(m_rccPort);
      ocpiAssert(isProvider());
      if (m_buffer == buffer) {
	m_buffer = NULL;
	m_rccPort->current.data = NULL;
      }
      m_delegateTo->releaseInputBuffer(buffer);
    }

    bool Port::
    request() {
      ocpiAssert(m_delegateTo);
      if (m_buffer)
	return true;
      m_wantsBuffer = true;
      if (!definitionComplete())
	return false;
      // We want a buffer and we don't have one
      if (isOutput()) {
	m_buffer = m_delegateTo->getNextEmptyOutputBuffer(m_rccPort->current.data,
							  m_rccPort->current.maxLength);
	m_rccPort->output.length = m_rccPort->current.maxLength;
      } else {
	uint8_t opcode;
	if ((m_buffer = m_delegateTo->getNextFullInputBuffer(m_rccPort->current.data,
							     m_rccPort->input.length,
							     opcode)))
	  m_rccPort->input.u.operation = opcode;
      }
      if (m_buffer) {
	m_rccPort->current.containerBuffer = m_buffer;
	m_buffer->m_ud = this;
	m_wantsBuffer = false;
	return true;
      }
      return false;
    }

    void Port::
    send(OCPI::DataTransport::BufferUserFacet* buffer, uint32_t length, uint8_t opcode)
    {
      Port *bufferPort = static_cast<Port*>(buffer->m_ud);
      ocpiAssert (m_delegateTo);
      if (bufferPort == this) {
	m_delegateTo->sendOutputBuffer(buffer, length, opcode);
	if (buffer == m_buffer) {
	  m_buffer = NULL;
	  m_rccPort->current.data = NULL;
	  // If we send the current buffer, it is an implicit advance
	  m_wantsBuffer = true;
	}
      } else {
	// Potential zero copy
	m_delegateTo->sendZcopyInputBuffer(buffer, length, opcode);
	if (bufferPort->m_buffer == buffer) {
	  bufferPort->m_buffer = NULL;
	  bufferPort->m_rccPort->current.data = NULL;
	  // If we send the current buffer, it is an implicit advance
	  bufferPort->m_wantsBuffer = true;
	}
      }
    }

    bool Port::advance()
    {
      ocpiAssert (m_delegateTo);
      if (m_buffer) {
	isOutput() ?
	  m_delegateTo->sendOutputBuffer(m_buffer, m_rccPort->output.length, m_rccPort->output.u.operation) :
	  m_delegateTo->releaseInputBuffer(m_buffer);
	m_rccPort->current.data = NULL;
	m_buffer = NULL;
      }
      return request();
    }


    Port::
    ~Port()
    {
      // Empty
    }

    PortDelegator::
    PortDelegator( Worker& w, const OCPI::Metadata::Port & pmd, unsigned xferOptions,
		   Port *delegator, const OCPI::Util::PValue *params)
      :  OCPI::Container::PortBase< Worker, OCPI::RCC::Port, ExternalPort>
	 (w, pmd, pmd.provider, xferOptions, params, delegator ? &delegator->getData() : NULL),
	 m_portOrdinal(pmd.ordinal), m_mutex(m_container), m_delegator(delegator)
    {
      
    }

    uint32_t 
    PortDelegator::    
    getBufferCount() 
    {
#if 1
      return getData().data.desc.nBuffers;
#else
    FIXME: refactor the parts of the RDMA descriptor that is common for message ports.
      return (m_metaPort.minBufferCount == 0) ? OM::Port::DEFAULT_NBUFFERS : m_metaPort.minBufferCount;
#endif
    };
  
    PortDelegator::    
    ~PortDelegator()
    {
      // Empty
    }

  }
}
