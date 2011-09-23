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
#include <OcpiRccContainer.h>
#include <OcpiRccController.h>
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
    RDMAPort::MyConnection::
    init( PortData * src,
	  bool       slocal,
	  PortData * input,
	  bool       tlocal)
    {
      if ( slocal ) {
	lsrc = static_cast<RDMAPort*>(src);
	lsrc->external = false;
	if ( tlocal ) {
	  nosrcd = true;
	}
      }
      else {
	rsrc = *src;
	lsrc = NULL;
	rsrc.external = true;
      }
      if ( tlocal ) {
	linput = static_cast<RDMAPort*>(input);
	linput->external = false;
      }
      else {
	rinput = *input;
	rinput.external = true;
	linput = NULL;
      }
    }

    void 
    RDMAPort::
    initOutputPort()
    {
      if ( m_circuit )  {
	m_circuit->finalize( getData().data.desc.oob.oep );
      }
      else {
	m_circuit = parent().getTransport().createCircuit( getData().data );
	parent().parent().addCircuit( m_circuit );
      }
      m_dtPort = m_circuit->getOutputPort();
      parent().updatePort( *this );
    }


    void
    RDMAPort::
    processPortProperties(const OU::PValue* props )
    {
      const char *role = 0;
      OU::findString(props, "xferRole", role);
      if (!role)
	OU::findString(props, "role", role);
      if (role && 
	  (strcasecmp(role,"flowcontrol") == 0 ||
	   strcasecmp(role,"activeflowcontrol") == 0))
	getData().data.role = OCPI::RDT::ActiveFlowControl;
      OA::ULong ul;
      if (OU::findULong(props, "bufferCount", ul))
	getData().data.desc.nBuffers = ul;

      if (OU::findULong(props, "bufferSize", ul))
	getData().data.desc.dataBufferSize = ul;
    }

    void 
    RDMAPort::
    initInputPort()
    {

      if ( m_dtPort ) {
	return;
      }
      if ( m_circuit ) {
	m_dtPort = parent().getTransport().createInputPort( m_circuit, getData().data );
      }
      else {
	m_dtPort = parent().getTransport().createInputPort( m_circuit, getData().data );
	parent().parent().addCircuit( m_circuit );
      }

      // We need to get the port descriptor
      m_dtPort->getPortDescriptor( getData().data, NULL );

    }



    RDMAPort::
    RDMAPort(Worker& w, const OU::PValue *props,
	     const OCPI::Metadata::Port & pmd,  const char * endpoint )
      :  PortDelegator(w,props,pmd,endpoint),m_circuit(NULL)
    {

      getData().port = (OC::PortDesc)this;
      getData().container_id = m_container.getId();  
      getData().data.desc.nBuffers = (pmd.minBufferCount == 0) ? OM::Port::DEFAULT_NBUFFERS : pmd.minBufferCount;
      getData().data.desc.dataBufferSize = (pmd.minBufferSize == 0)  ? OM::Port::DEFAULT_BUFFER_SIZE : pmd.minBufferSize;
      getData().data.role = OCPI::RDT::ActiveMessage;
      getData().data.options =
	(1 << OCPI::RDT::ActiveFlowControl) |
	(1 << OCPI::RDT::ActiveMessage);


      processPortProperties( props );
      if ( endpoint ) {
	strcpy( getData().data.desc.oob.oep, endpoint );
      }

    }

    RDMAPort::
    ~RDMAPort()
    {
      disconnect();
    }

    void 
    RDMAPort::
    connectInside(OC::Port & other, const OU::PValue * my_props,  const OU::PValue * /* other_props */ )
    {
      ( void ) my_props;
      std::string ref;
      connectInputPort( &other, ref, my_props);
    }


    const std::string& 
    RDMAPort::
    getInitialProviderInfo(const OU::PValue* props)
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( isProvider() );
      applyConnectParams(props);
      getData().data.desc.nBuffers = myDesc.nBuffers;
      getData().data.desc.dataBufferSize = myDesc.dataBufferSize;
      initInputPort();
      m_ourFinalDescriptor = OC::Container::packPortDesc( *this );

      // url only, no cc
      return m_ourFinalDescriptor;
    }

    const std::string& 
    RDMAPort::
    setInitialProviderInfo(const OU::PValue* props, const std::string & user )
    {
      //      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( ! isProvider() );
      applyConnectParams(props);
      PortData otherPortData;
      OC::Container::unpackPortDesc(user, &otherPortData);
      establishRoles(otherPortData.getData().data);
      finishConnection(otherPortData.getData().data);

      PortData tpdata;
      // FIXME: why is this unpacking done twice?
      OC::Container::unpackPortDesc( user, &tpdata );
      connectInputPort( &tpdata, m_localShadowPort, NULL  );
      if ( getData().data.role == OCPI::RDT::ActiveMessage ) {
	return m_localShadowPort;
      }
      else {
	PortData desc;
	m_dtPort->getPortDescriptor( desc.getData().data, &tpdata.getData().data );
	desc.getData().container_id = m_container.getId();
	m_initialPortInfo = OC::Container::packPortDesc( desc );
      }
      // FIXME: why is this packing done twice?
      m_initialPortInfo = OC::Container::packPortDesc( *this );
      return m_initialPortInfo;
    }

    const std::string& 
    RDMAPort::
    setFinalProviderInfo(const std::string & input_port )
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( ! isProvider() );
      PortData tpdata;
      OC::Container::unpackPortDesc( input_port, &tpdata );

      if ( m_dtPort ) 
	m_dtPort->finalize( tpdata.getData().data );

      m_ourFinalDescriptor = OC::Container::packPortDesc( *this );
      return m_ourFinalDescriptor;
    }


    const std::string& 
    RDMAPort::
    setInitialUserInfo(const std::string& user)
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( isProvider() );

      initInputPort();
      PortData src;
      PortData * pd;
      if ( ! (pd=OC::Container::unpackPortDesc( user, &src ))) {
	throw OU::EmbeddedException("Input Port descriptor is invalid");
      }
      setOutputFlowControl( pd );  
      m_dtPort->finalize( src.getData().data );
      PortData desc;
      strncpy( desc.getData().data.desc.oob.oep,  getData().data.desc.oob.oep, OCPI::RDT::MAX_EPS_SIZE);
      m_dtPort->getPortDescriptor( desc.getData().data, &src.getData().data );
      getData().data.desc.oob.cookie = desc.getData().data.desc.oob.cookie;
      m_initialPortInfo = OC::Container::packPortDesc( *this );
      return m_initialPortInfo;
    }

    void 
    RDMAPort::
    setFinalUserInfo(const std::string& srcPort )
    {
      OU::AutoMutex guard ( m_mutex,  true ); 
      ocpiAssert( isProvider() );
      PortData src;
      PortData * pd;
      if ( ! (pd=OC::Container::unpackPortDesc( srcPort, &src ))) {
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
#ifdef NEEDED
      OCPI::DataTransport::Port            *m_dtPort;
#endif
      Port                                 *m_port;
      OS::Mutex                            *m_mutex; // not a reference since we construct as an array
    };

    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
    public:
      friend class ExternalBuffer;
      ExternalPort(Port &p, const char* name, const OU::PValue *props, const OM::Port &mPort, OS::Mutex & mutex ) 
	: OC::ExternalPortBase<Port,ExternalPort>(p, name, props, mPort, !p.isProvider()),
	  m_mutex(mutex)
      {
	unsigned nBuffers = p.getBufferCount();
	// Use an array here for scalability
	m_exBuffers = new ExternalBuffer[nBuffers];
	for (unsigned int n = 0; n < nBuffers; n++ ) {
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
      OCPI::OS::Mutex & m_mutex;     
    };

    void 
    ExternalBuffer::
    release()
    {
      OU::AutoMutex guard (*m_mutex, true); 
      m_port->advance(m_buffer,0);
    }

    // For producer buffers
    void 
    ExternalBuffer::
    put(uint32_t length, uint8_t opCode, bool endOfData)
    {        
      OU::AutoMutex guard ( *m_mutex, true ); 
      // FIXME:  check for value opcode and length values.
      if ( endOfData ) length = 0;
      m_port->advance(m_buffer,opCode,length);
    }


    // This port is an input port to the external user and an output port to the worker,
    // so the worker produces data and the external user gets it
    OC::ExternalBuffer *
    ExternalPort::
    getBuffer(uint8_t *&data, uint32_t &length, uint8_t &opCode, bool &endOfData)
    {
      OU::AutoMutex guard ( m_mutex, true ); 
      OCPI::DataTransport::BufferUserFacet * b = NULL;
      if ( parent().hasFullInputBuffer() ) {
	b = parent().getNextFullInputBuffer();
	int oc = b->opcode();
	opCode = oc;
	data = (uint8_t*)b->getBuffer();
	length = b->getDataLength();
	endOfData = (length == 0) ? true : false;
	return &m_exBuffers[b->getTid()];
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
      OCPI::DataTransport::BufferUserFacet * b = NULL;
      if ( parent().hasEmptyOutputBuffer() ) {
	b = parent().getNextEmptyOutputBuffer();
	data = (uint8_t*)b->getBuffer();
	length = b->getLength();
	return &m_exBuffers[b->getTid()];
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
    connectExternal(const char * name, const OU::PValue * myprops, const OU::PValue * oprops)
    {

      OU::AutoMutex guard ( m_mutex,  true ); 
      
      // We will create a single port worker to do this so that we can use the mechanics of the 
      // underlying system
      OC::Worker & w = *new Worker(parent().parent(), NULL,
				   (const char *)&stub_dispatch, NULL, NULL, NULL);
      OCPI::RCC::Port * p;
      if ( isProvider() ) {
	p = dynamic_cast<OCPI::RCC::Port*>
	  (&w.createOutputPort(0, getData().data.desc.nBuffers,
			       getData().data.desc.dataBufferSize,NULL ));
	p->setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	p->connect( *this, myprops, oprops );
	

	
      }
      else {
	p = dynamic_cast<OCPI::RCC::Port*>
	  (&w.createInputPort(0, 
			      getData().data.desc.nBuffers,
			      getData().data.desc.dataBufferSize,NULL ));
	p->setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	this->connect( *p, myprops, oprops );
      }
      return *new ExternalPort(*p, name, NULL, metaPort(), m_mutex);
    }

    void
    RDMAPort::
    advance( OCPI::DataTransport::BufferUserFacet* b, uint32_t opcode, uint32_t length )
    {
      OCPI::DataTransport::Buffer * buffer = static_cast<OCPI::DataTransport::Buffer*>(b);
      buffer->getMetaData()->ocpiMetaDataWord.opCode = opcode;
      m_dtPort->advance( buffer, length);
    }

    void 
    RDMAPort::
    disconnect(    OC::PortData* sp,                 
		   OC::PortData* input
		   )
    {
      OU::AutoMutex guard (parent().mutex(), true);
      OCPI::DataTransport::Circuit* scircuit=NULL;
      OCPI::DataTransport::Circuit* tcircuit=NULL;
      OCPI::DataTransport::Port* port=NULL;

      if ( ! sp->external ) {
	RDMAPort* p = reinterpret_cast<RDMAPort*>(sp->getData().port);
	scircuit = p->m_circuit;
	port = p->m_dtPort;

	if ( !scircuit || scircuit->isCircuitOpen() ) {
	  // Not connected.
	  return;
	}
	if ( p->parent().enabled ) {
	  throw OU::EmbeddedException( OU::ONP_WORKER_STARTED, NULL, OU::ApplicationRecoverable);
	}
	port->reset();
	p->parent().m_context->connectedPorts &= ~(1<<p->m_portOrdinal);
	p->m_circuit->release();
	p->m_circuit = NULL;
      }


      // Now the input, but only if it is local
      if ( ! input->external ) {
	RDMAPort * p = reinterpret_cast<RDMAPort*>(input->getData().port);
	tcircuit = p->m_circuit;
	port = p->m_dtPort;

	if ( p->parent().enabled ) {
	  throw OU::EmbeddedException( OU::ONP_WORKER_STARTED, NULL, OU::ApplicationRecoverable);
	}
	if ( !tcircuit || tcircuit->isCircuitOpen() ) {
	  throw OU::EmbeddedException( OU::BAD_CONNECTION_COOKIE, "Worker not found for input port",
				       OU::ApplicationRecoverable);
	}
	port->reset();
	p->parent().m_context->connectedPorts &= ~(1<<p->m_portOrdinal);

	p->m_circuit->release();
	p->m_circuit = NULL;
      }


    }

    void 
    RDMAPort::
    disconnect( )
      throw ( OU::EmbeddedException )
    {
      TRACE(" OCPI::RCC::Container::disconnectPorts()");
      OU::AutoMutex guard ( m_mutex,  true ); 
      if ( ! m_connectionCookie.connected ) {
	return;
      }
      PortData* rtpd;
      if (! m_connectionCookie.linput  ) {  // external
	rtpd = &m_connectionCookie.rinput;
      }
      else {
	rtpd = m_connectionCookie.linput;
      }

      if ( !m_connectionCookie.lsrc   ) {  // external
	disconnect( &m_connectionCookie.rsrc, rtpd );
      }
      else {
	if ( m_connectionCookie.nosrcd ) {
	  return;
	}
	disconnect(m_connectionCookie.lsrc, rtpd );    
      }
      m_connectionCookie.connected = false;

    }


    OC::PortConnectionDesc
    RDMAPort::
    connectExternalInputPort( PortData *           inputPort,    
			      const OU::PValue*       props
			      )
    {
      ( void ) props;
      OU::AutoMutex guard ( m_mutex,  true ); 

      // Make sure the the input port endpoint is registered, this may be the first time that
      // we talk to this endpoint
      parent().getTransport().addRemoteEndpoint( inputPort->getData().data.desc.oob.oep );

      // Initialize the port
      initOutputPort();
  
      // Add the ports to the circuit
      OC::PortConnectionDesc  flowControl;
      m_circuit->addInputPort( inputPort->getData().data, getData().data.desc.oob.oep, &flowControl.data );
      flowControl.port = (OC::PortDesc)inputPort;
      parent().m_context->connectedPorts |= (1<<m_portOrdinal);
      flowControl.container_id = m_container.getId();

      OC::PortData desc;
      strncpy( desc.getData().data.desc.oob.oep,  getData().data.desc.oob.oep, OCPI::RDT::MAX_EPS_SIZE );
      m_dtPort->getPortDescriptor( desc.getData().data, &inputPort->getData().data );
      flowControl.data.desc.oob.cookie = desc.getData().data.desc.oob.cookie;

      return flowControl;
    }


    void
    RDMAPort::
    connectInputPort( PortData *    inputPort,    
		      std::string&  lPort,
		      const OU::PValue*       props
		      )
      throw ( OU::EmbeddedException )
    {
      TRACE("OCPI::RCC::Container::connectInputPort()");
      OU::AutoMutex guard ( m_mutex,  true ); 

      PortData          localShadowPort;

      // At this point the output ports reoutputs are not yet created, we need to 
      // create a local endpoint that is compatible with the remote.
      std::string s = parent().getTransport().getLocalCompatibleEndpoint( inputPort->getData().data.desc.oob.oep );
      s = parent().getTransport().addLocalEndpoint( s.c_str() )->sMemServices->endpoint()->end_point;
      strcpy( getData().data.desc.oob.oep, s.c_str());  

      // At some point we may want to make this smarter, but we want to make sure
      // that we dont overrun the inputs buffers.
      if ( getData().data.desc.dataBufferSize > inputPort->getData().data.desc.dataBufferSize ) {
	getData().data.desc.dataBufferSize = inputPort->getData().data.desc.dataBufferSize;
      }

      OC::PortData* rtpd=0;
      RDMAPort* rp=NULL;
      if ( inputPort->getData().container_id != m_container.getId()  ) {  // external
	inputPort->external = true;
	OC::PortConnectionDesc fcd = connectExternalInputPort(inputPort,props);
	localShadowPort.getData() = fcd;
	localShadowPort.external = true;
	rtpd = inputPort;
      }
      else {
	inputPort->external = false;
	rp = reinterpret_cast<RDMAPort*>(inputPort->getData().port);
	connectInternalInputPort(rp,props);
	localShadowPort.getData() = inputPort->getData();
	localShadowPort.external = false;
      }

      try {
	if ( rp ) {  // local input port
	  m_connectionCookie.init( this,true,rp,true );
	}
	else {
	  m_connectionCookie.init( this,true,rtpd,false );
	}
      }
      catch( std::bad_alloc ) {
	throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "new", OU::ContainerFatal);
      }
      lPort = OC::Container::packPortDesc( localShadowPort );
    }


    void
    RDMAPort::
    setOutputFlowControl( PortData * srcPort )
      throw ( OU::EmbeddedException )
    {
      ocpiAssert( isProvider() );
      OU::AutoMutex guard ( m_mutex,  true ); 

      // Local connection
      if ( getData().container_id == srcPort->getData().container_id ) {
	try {
	  RDMAPort* p = reinterpret_cast<RDMAPort*>(srcPort->getData().port);
	  p->m_circuit = m_circuit;
	  m_connectionCookie.init( srcPort,true, this,true );
	}
	catch( std::bad_alloc ) {
	  throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "new", OU::ContainerFatal);
	}
	return;
      }
      m_dtPort->setFlowControlDescriptor( srcPort->getData().data );
      try {
	m_connectionCookie.init( srcPort,false,this,true );
      }
      catch( std::bad_alloc ) {
	throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "new", OU::ContainerFatal);
      }
    }


    /**********************************
     * This method is used to connect the external ports decribed in the
     * the PortDependencyData structure to output port for the given worker.
     *********************************/
    void
    RDMAPort::
    connectInternalInputPort( OC::Port *  tPort,
			      const OU::PValue*            props  )
    {
      ( void ) props;
      OU::AutoMutex guard ( m_mutex,  true ); 

      RDMAPort *  inputPort = static_cast<RDMAPort*>(tPort);

      inputPort->initInputPort();

      // Allocate a connection
      m_circuit = inputPort->m_circuit;
      m_circuit->attach();

      strcpy( getData().data.desc.oob.oep, inputPort->getData().data.desc.oob.oep  );

      // Initialize the port
      initOutputPort();
    }

    void 
    RDMAPort::
    sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* buf, unsigned int len )
    {
      m_dtPort->sendZcopyInputBuffer( static_cast<OCPI::DataTransport::Buffer*>(buf), len);
    }

    bool 
    RDMAPort::
    hasEmptyOutputBuffer()
    {
      return m_dtPort->hasEmptyOutputBuffer();
    }

    OCPI::DataTransport::BufferUserFacet*
    RDMAPort::
    getNextEmptyOutputBuffer()
    {
      return m_dtPort->getNextEmptyOutputBuffer();
    }


    bool  
    RDMAPort::
    hasFullInputBuffer()
    {
      return m_dtPort->hasFullInputBuffer();
    }

    OCPI::DataTransport::BufferUserFacet*
    RDMAPort::
    getNextFullInputBuffer()
    {
      return m_dtPort->getNextFullInputBuffer();
    }

    bool 
    RDMAPort::
    definitionComplete()
    {
      OCPI::DataTransport::Circuit* c = this->circuit();
      if ( !c || c->isCircuitOpen() ) {
	return false;
      }
      else {
	return true;
      }
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

    uint32_t 
    RDMAPort::
    getBufferLength()
    {
      return m_dtPort->getBufferLength();
    }

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
    Port( Worker& w, const OCPI::Util::PValue *props, const OCPI::Metadata::Port & pmd,
	 const char * endpoint)
      : PortDelegator( w, props, pmd, endpoint),m_props(props), m_delegateTo(NULL)
    {
      if ( endpoint ){
	m_endpoint = endpoint;
      }
    }

    void 
    Port::
    setMode( ConnectionMode mode )
    {
      if ( mode == CON_TYPE_RDMA ) {
	if ( m_delegateTo ) delete m_delegateTo;
	m_delegateTo = new RDMAPort( parent(), m_props, metaPort(), m_endpoint.c_str() );
      }
      else {
	m_delegateTo = new MessagePort( parent(), m_props, metaPort() );
      }

    }

    void 
    Port::
    connect( OCPI::Container::Port &other, const OCPI::API::PValue *myProps,
	     const OCPI::API::PValue *otherProps)
    {
      if ( ! m_delegateTo ) setMode( CON_TYPE_RDMA );
      m_delegateTo->connect(other,myProps,otherProps);
    }

    Port::
    ~Port()
    {
      // Empty
    }

    PortDelegator::
    PortDelegator( Worker& w, const OCPI::Util::PValue *props, const OCPI::Metadata::Port & pmd, const char* /*endpoint*/ )
      :  OCPI::Container::PortBase< Worker, OCPI::RCC::Port, ExternalPort>(w, props, pmd, pmd.provider),
     m_dtPort(NULL), m_portOrdinal(pmd.ordinal), m_mutex(m_container) 
      
    {
      
    }

    uint32_t 
    PortDelegator::    
    getBufferCount() 
    {
      return (m_metaPort.minBufferCount == 0) ? OM::Port::DEFAULT_NBUFFERS : m_metaPort.minBufferCount;
    };
  
    PortDelegator::    
    ~PortDelegator()
    {
      // Empty
    }

  }
}
