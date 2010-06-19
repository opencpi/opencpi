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
#include <CpiCP289Port.h>
#include <CpiCP289Container.h>
#include <CpiCP289Controller.h>
#include <CpiCP289Application.h>
#include <CpiOsMisc.h>
#include <CpiTransport.h>
#include <CpiRDTInterface.h>
#include <CpiOsAssert.h>
#include <CpiUtilCDR.h>
#include <CpiPortMetaData.h>
#include <CpiUtilAutoMutex.h>
#include <CpiContainerErrorCodes.h>
#include <CpiPValue.h>
#include <CpiUtilMisc.h>
#include <CpiParentChild.h>
#include <CpiBuffer.h>

#include <DtTransferInternal.h>

using namespace CPI::Container;
using namespace CPI::Util;
using namespace CPI::CP289;

#define MyParent static_cast<CPI::CP289::Worker*>(myParent)
#define MyApp static_cast<CPI::CP289::Application*>(myParent->myParent)

void 
CPI::CP289::Port::MyConnection::
init( PortData * src,
      bool       slocal,
      PortData * input,
      bool       tlocal)
{
  if ( slocal ) {
    lsrc = static_cast<CPI::CP289::Port*>(src);
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
    linput = static_cast<CPI::CP289::Port*>(input);
    linput->external = false;
  }
  else {
    rinput = *input;
    rinput.external = true;
    linput = NULL;
  }
}

void 
CPI::CP289::Port::
initOutputPort()
{
  if ( m_circuit )  {
    m_circuit->finalize( connectionData.data.desc.oob.oep );
  }
  else {
    m_circuit = MyParent->getTransport().createCircuit( connectionData.data );
    MyApp->addCircuit( m_circuit );
  }
  m_dtPort = m_circuit->getOutputPort();
  MyParent->updatePort( *this );
}


void
CPI::CP289::Port::
processPortProperties(CPI::Util::PValue* props )
{
  const CPI::Util::PValue* p = CPI::Util::PValue::find(props, "role");
  if ( p ) {
    if ( (p->type != CPI::Metadata::Property::CPI_String)) {
      throw CPI::Util::EmbeddedException("\"role\" property has wrong type, should be String");
    }
    if ( strcmp(p->vString,"ActiveFlowControl") == 0 ) {
      connectionData.data.role = CPI::RDT::ActiveFlowControl;
    }
  }

  p = CPI::Util::PValue::find(props, "bufferCount");
  if ( p ) {
    if ( (p->type != CPI::Metadata::Property::CPI_ULong)) {
      throw CPI::Util::EmbeddedException("\"bufferCount\" property has wrong type, should be ULong");
    }
    connectionData.data.desc.nBuffers = p->vULong;
  }

  p = CPI::Util::PValue::find(props, "bufferSize");
  if ( p ) {
    if ( (p->type != CPI::Metadata::Property::CPI_ULong)) {
      throw CPI::Util::EmbeddedException("\"bufferSize\" property has wrong type, should be ULong");
    }
    connectionData.data.desc.dataBufferSize = p->vULong;
  }
}

void 
CPI::CP289::Port::
initInputPort()
{
  if ( m_dtPort ) {
    return;
  }
  if ( m_circuit ) {
    m_dtPort = MyParent->getTransport().createInputPort( m_circuit, connectionData.data );
  }
  else {
    m_dtPort = MyParent->getTransport().createInputPort( m_circuit, connectionData.data );
    MyApp->addCircuit( m_circuit );
  }

  // We need to get the port descriptor
  m_dtPort->getPortDescriptor( connectionData.data );

}



CPI::CP289::Port::
Port(CPI::Container::Worker& w, CPI::Metadata::Port & pmd,  const char * endpoint )
  : CPI::Container::Port(w,pmd),m_dtPort(NULL),m_portId(pmd.m_pid),m_circuit(NULL),m_props(NULL)
{

  connectionData.port = (PortDesc)this;
  connectionData.container_id = static_cast<CPI::CP289::Container*>(myParent->myParent->myParent)->getId();  
  myMetaPort.provider = pmd.provider;
  connectionData.data.desc.nBuffers = (pmd.minBufferCount == 0) ? pmd.DEFAULT_NBUFFERS : pmd.minBufferCount;
  connectionData.data.desc.dataBufferSize = (pmd.minBufferSize == 0)  ? pmd.DEFAULT_BUFFER_SIZE : pmd.minBufferSize;
  connectionData.data.role = CPI::RDT::ActiveMessage;
  connectionData.data.options =
    (1 << CPI::RDT::ActiveFlowControl) |
    (1 << CPI::RDT::ActiveMessage);


  processPortProperties( m_props );
  if ( endpoint ) {
    strcpy( connectionData.data.desc.oob.oep, endpoint );
  }

  if ( ! isProvider() ) {
    // The output port does not get initialized until it gets connected
  }
  else {


    //    initInputPort();
    //    cpiAssert( m_circuit );


  }
}




CPI::CP289::Port::
~Port()
{
  disconnect();
}


void 
CPI::CP289::Port::
connectInside(CPI::Container::Port & other, CPI::Util::PValue * my_props,  CPI::Util::PValue * /* other_props */ )
{
  std::string ref;
  connectInputPort( &other, ref, m_props);
}


const std::string& 
CPI::CP289::Port::
getInitialProviderInfo(CPI::Util::PValue* props)
{
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  cpiAssert( isProvider() );
  applyConnectParams(props);
  connectionData.data.desc.nBuffers = myDesc.nBuffers;
  connectionData.data.desc.dataBufferSize = myDesc.dataBufferSize;
  initInputPort();
  m_ourFinalDescriptor = MyParent->myParent->myParent->packPortDesc( *this );
  return m_ourFinalDescriptor;
}

const std::string& 
CPI::CP289::Port::
setInitialProviderInfo(CPI::Util::PValue* props, const std::string & user )
{
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  cpiAssert( ! isProvider() );
  applyConnectParams(props);
  PortData otherPortData;
  myContainer.unpackPortDesc(user, &otherPortData);
  establishRoles(otherPortData.connectionData.data);
  finishConnection(otherPortData.connectionData.data);
  return setFinalProviderInfo( user );
}

const std::string& 
CPI::CP289::Port::
setFinalProviderInfo(const std::string & input_port )
{
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  cpiAssert( ! isProvider() );
  PortData tpdata;
  MyParent->myParent->myParent->unpackPortDesc( input_port, &tpdata );
  connectInputPort( &tpdata, m_localShadowPort, NULL  );

  if ( connectionData.data.role == CPI::RDT::ActiveMessage ) {
    return m_localShadowPort;
  }
  else {
    PortData desc;
    m_dtPort->getPortDescriptor( desc.connectionData.data );
    desc.connectionData.container_id = MyParent->myParent->myParent->getId();
    myInitialPortInfo = MyParent->myParent->myParent->packPortDesc( desc );
  }
  return myInitialPortInfo;
}

static std::string nul_s;
const std::string& 
CPI::CP289::Port::
setInitialUserInfo(const std::string& user)
{
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  cpiAssert( isProvider() );
  setFinalUserInfo(user );
  // Nothing more to do
  return nul_s;
}


void 
CPI::CP289::Port::
setFinalUserInfo(const std::string& srcPort )
{
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  cpiAssert( isProvider() );
  initInputPort();
  PortData src;
  PortData * pd;
  if ( ! (pd=MyParent->myParent->myParent->unpackPortDesc( srcPort, &src ))) {
    throw CPI::Util::EmbeddedException("Input Port descriptor is invalid");
  }
  setOutputFlowControl( pd );
}


namespace CPI {
  namespace CP289 {

    class ExternalBuffer : public CPI::Container::ExternalBuffer {
    public:
      ExternalBuffer( ExternalPort& ep, Port & p, CPI::DataTransport::Buffer* buffer ) 
        :CPI::Container::ExternalBuffer(ep),m_buffer(buffer),m_port(p)
      {}

      virtual ~ExternalBuffer(){};

      // For consumer buffers
      void release();
      // For producer buffers
      void put(uint8_t opCode, uint32_t length, bool endOfData);

    private:
      CPI::DataTransport::Buffer* m_buffer;
      Port & m_port;
    };

    class ExternalPort : public CPI::Container::ExternalPort, CPI::Util::Child<Port,CPI::Container::ExternalPort> {    
    public:
      friend class ExternalBuffer;
      ExternalPort( const char* name, Port &p, CPI::OS::Mutex & mutex ) 
        : CPI::Container::ExternalPort(name), CPI::Util::Child<Port,CPI::Container::ExternalPort>(p),m_port(p),m_mutex(mutex)
      {
        for ( unsigned int n=0; n<p.dtPort()->getBufferCount(); n++ ) {
          m_exBuffers.push_back( new  ExternalBuffer( *this, p, p.dtPort()->getBuffer(n) ) );
        }
      }
      CPI::Container::ExternalBuffer * getBuffer(uint8_t &opCode, uint8_t *&data, uint32_t &length, bool &endOfData);
      CPI::Container::ExternalBuffer *getBuffer(uint8_t *&data, uint32_t &length);
      void endOfData();
      void tryFlush();


    private:
      // External Buffers
      std::vector<ExternalBuffer*> m_exBuffers;
      Port & m_port; 
      CPI::OS::Mutex & m_mutex;     

      virtual ~ExternalPort(){}
    };

  }
}




void 
CPI::CP289::ExternalBuffer::
release()
{
  CPI::Util::AutoMutex guard ( static_cast<CPI::CP289::ExternalPort*>(myParent)->m_mutex, true ); 
  m_port.dtPort()->advance(m_buffer);
}

// For producer buffers
void 
CPI::CP289::ExternalBuffer::
put(uint8_t opCode, uint32_t length, bool endOfData)
{        
  CPI::Util::AutoMutex guard ( static_cast<CPI::CP289::ExternalPort*>(myParent)->m_mutex, true ); 
  if ( endOfData ) length = 0;
  m_buffer->getMetaData()->cpiMetaDataWord.opCode = opCode;
  m_port.dtPort()->advance(m_buffer,length);
}


// This port is an input port to the external user and an output port to the worker,
// so the worker produces data and the external user gets it
CPI::Container::ExternalBuffer *
CPI::CP289::ExternalPort::
getBuffer(uint8_t &opCode, uint8_t *&data, uint32_t &length, bool &endOfData)
{
  CPI::Util::AutoMutex guard ( m_mutex, true ); 
  CPI::DataTransport::Buffer * b = NULL;
  if ( m_port.dtPort()->hasFullInputBuffer() ) {
    b = m_port.dtPort()->getNextFullInputBuffer();
    int oc = b->getMetaData()->cpiMetaDataWord.opCode;
    opCode = oc;
    data = (uint8_t*)b->getBuffer();
    length = b->getMetaData()->cpiMetaDataWord.length;
    endOfData = (length == 0) ? true : false;
    return m_exBuffers[b->getTid()];
  }
  return NULL;
}


// This port is an output port to the external user and an input port to the worker
// so the external user produces data and 
CPI::Container::ExternalBuffer * 
CPI::CP289::ExternalPort::
getBuffer(uint8_t *&data, uint32_t &length)
{
  CPI::Util::AutoMutex guard ( m_mutex, true ); 
  CPI::DataTransport::Buffer * b = NULL;
  if ( m_port.dtPort()->hasEmptyOutputBuffer() ) {
    b = m_port.dtPort()->getNextEmptyOutputBuffer();
    data = (uint8_t*)b->getBuffer();
    length = b->getLength();
    return m_exBuffers[b->getTid()];
  }
  return NULL;
}

void 
CPI::CP289::ExternalPort::
endOfData()
{
  // ??

}


void 
CPI::CP289::ExternalPort::
tryFlush()
{
  // NoOp
}




/*
 * The following code is generated by the tool
 */
#include <RCC_Worker.h>
static 
RCCResult stub_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
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


CPI::Container::ExternalPort& 
CPI::CP289::Port::
connectExternal(const char * name, CPI::Util::PValue * myprops, CPI::Util::PValue * oprops)
{

  // We will create a single port worker to do this so that we can use the mechanics of the 
  // underlying system
  CPI::Container::Worker & w = 
    MyApp->createWorker(NULL,NULL,&stub_dispatch);
  Port * p;
  if ( isProvider() ) {
    p = static_cast<CPI::CP289::Port*>(&w.createOutputPort( 0, 
                            connectionData.data.desc.nBuffers, connectionData.data.desc.dataBufferSize ));

    p->connect( *this, myprops, oprops );

    /*
    std::string fb = p->setFinalProviderInfo( getInitialProviderInfo(0) );
    setFinalUserInfo( fb );
    */
  }
  else {
    p = static_cast<CPI::CP289::Port*>(&w.createInputPort( 0, 
                            connectionData.data.desc.nBuffers, connectionData.data.desc.dataBufferSize ));

    this->connect( *p, myprops, oprops );

    /*
    std::string fb = setFinalProviderInfo( p->getInitialProviderInfo(0) );
    p->setFinalUserInfo( fb );
    */

  }
  ExternalPort* ep = new ExternalPort(name, *p, MyApp->mutex());
  return *ep;
}


static 
inline 
CPI::CP289::Worker* toWorker(WorkerId & id)
{
  if ( id == 0 ) {
    throw CPI::Util::EmbeddedException( "Worker Not Found" );
  }
  return reinterpret_cast<CPI::CP289::Worker*>(id);
}


void 
CPI::CP289::Port::
disconnect(    CPI::Container::PortData* sp,                 
               CPI::Container::PortData* input
               )
{

  CPI::CP289::Worker * swi=NULL;
  CPI::DataTransport::Circuit* scircuit=NULL;
  CPI::DataTransport::Circuit* tcircuit=NULL;
  CPI::DataTransport::Port* port=NULL;

  if ( ! sp->external ) {
    CPI::CP289::Port* p = reinterpret_cast<CPI::CP289::Port*>(sp->connectionData.port);
    scircuit = p->m_circuit;
    port = p->m_dtPort;
    swi = static_cast<CPI::CP289::Worker*>(p->myParent);

    if ( !scircuit || scircuit->isCircuitOpen() ) {
      // Not connected.
      return;
    }
    if ( swi->enabled ) {
      throw CPI::Util::EmbeddedException( ONP_WORKER_STARTED, NULL, ApplicationRecoverable);
    }
    port->reset();
    swi->m_rcc_worker->m_context->connectedPorts &= ~(1<<p->m_portId);
    p->m_circuit->release();
    p->m_circuit = NULL;
  }


  // Now the input, but only if it is local
  CPI::CP289::Worker * twi = NULL;
  if ( ! input->external ) {
    CPI::CP289::Port* p = reinterpret_cast<CPI::CP289::Port*>(input->connectionData.port);
    tcircuit = p->m_circuit;
    port = p->m_dtPort;
    twi = static_cast<CPI::CP289::Worker*>(p->myParent);

    if ( twi->enabled ) {
      throw CPI::Util::EmbeddedException( ONP_WORKER_STARTED, NULL, ApplicationRecoverable);
    }
    if ( !tcircuit || tcircuit->isCircuitOpen() ) {
      throw CPI::Util::EmbeddedException( BAD_CONNECTION_COOKIE, "Worker not found for input port", ApplicationRecoverable);
    }
    port->reset();
    twi->m_rcc_worker->m_context->connectedPorts &= ~(1<<p->m_portId);

    p->m_circuit->release();
    p->m_circuit = NULL;
  }

}





void 
CPI::CP289::Port::
disconnect( )
  throw ( CPI::Util::EmbeddedException )
{
  TRACE(" CPI::CP289::Container::disconnectPorts()");
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
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


PortConnectionDesc
CPI::CP289::Port::
connectExternalInputPort( PortData *           inputPort,    
                          CPI::Util::PValue*       props
                          )
{
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  // Make sure the the input port endpoint is registered, this may be the first time that
  // we talk to this endpoint
  MyParent->getTransport().addRemoteEndpoint( inputPort->connectionData.data.desc.oob.oep );

  // Initialize the port
  initOutputPort();
  
  // Add the ports to the circuit
  PortConnectionDesc  flowControl;
  m_circuit->addInputPort( inputPort->connectionData.data, connectionData.data.desc.oob.oep, &flowControl.data );
  flowControl.port = (PortDesc)inputPort;
  MyParent->m_rcc_worker->m_context->connectedPorts |= (1<<m_portId);
  flowControl.container_id = MyParent->myParent->myParent->getId();
  return flowControl;
}


void
CPI::CP289::Port::
connectInputPort( PortData *    inputPort,    
                   std::string&  lPort,
                  CPI::Util::PValue*       props
                  )
  throw ( CPI::Util::EmbeddedException )
{
  TRACE("CPI::CP289::Container::connectInputPort()");
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  PortData          localShadowPort;

  // At this point the output ports reoutputs are not yet created, we need to 
  // create a local endpoint that is compatible with the remote.
  std::string s = MyParent->getTransport().getLocalCompatibleEndpoint( inputPort->connectionData.data.desc.oob.oep );
  s = MyParent->getTransport().addLocalEndpoint( s.c_str() )->sMemServices->getEndPoint()->end_point;
  strcpy( connectionData.data.desc.oob.oep, s.c_str());  

  // At some point we may want to make this smarter, but we want to make sure
  // that we dont overrun the inputs buffers.
  if ( connectionData.data.desc.dataBufferSize > inputPort->connectionData.data.desc.dataBufferSize ) {
    connectionData.data.desc.dataBufferSize = inputPort->connectionData.data.desc.dataBufferSize;
  }

  PortData* rtpd;
  Port* rp=NULL;
  if ( inputPort->connectionData.container_id != MyParent->myParent->myParent->getId()  ) {  // external
    inputPort->external = true;
    PortConnectionDesc fcd = connectExternalInputPort(inputPort,props);
    localShadowPort.connectionData = fcd;
    localShadowPort.external = true;
    rtpd = inputPort;
  }
  else {
    inputPort->external = false;
    rp = reinterpret_cast<Port*>(inputPort->connectionData.port);
    connectInternalInputPort(rp,props);
    localShadowPort.connectionData = inputPort->connectionData;
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
    throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }
  lPort = MyParent->myParent->myParent->packPortDesc( localShadowPort );
}


void
CPI::CP289::Port::
setOutputFlowControl( PortData * srcPort )
  throw ( CPI::Util::EmbeddedException )
{
  cpiAssert( isProvider() );
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  // Local connection
  if ( connectionData.container_id == srcPort->connectionData.container_id ) {
    try {
      CPI::CP289::Port* p = reinterpret_cast<CPI::CP289::Port*>(srcPort->connectionData.port);
      p->m_circuit = m_circuit;
      m_connectionCookie.init( srcPort,true, this,true );
    }
    catch( std::bad_alloc ) {
      throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
    }
    return;
  }
  m_dtPort->setFlowControlDescriptor( srcPort->connectionData.data );
  try {
    m_connectionCookie.init( srcPort,false,this,true );
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }
}


/**********************************
 * This method is used to connect the external ports decribed in the
 * the PortDependencyData structure to output port for the given worker.
 *********************************/
void
CPI::CP289::Port::
connectInternalInputPort( CPI::Container::Port *  tPort,
                          CPI::Util::PValue*            props  )
{
  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  CPI::CP289::Port *    inputPort = static_cast<CPI::CP289::Port*>(tPort);

  inputPort->initInputPort();

  // Allocate a connection
  m_circuit = inputPort->m_circuit;
  m_circuit->attach();

  strcpy( connectionData.data.desc.oob.oep, inputPort->connectionData.data.desc.oob.oep  );

  // Initialize the port
  initOutputPort();


}


// The general case of connecting ports that are in the same process.
void 
CPI::CP289::Port::
connect( CPI::Container::Port &other, CPI::Util::PValue *myProps, CPI::Util::PValue *otherProps) {

  CPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  if (isProvider())
    if (other.isProvider())
      throw ApiError("Cannot connect two provider ports", NULL);
    else
      other.connect( *this, otherProps, myProps);
  else if (!other.isProvider()) {
    throw ApiError("Cannot connect to user ports", NULL);
  }
  else {
    Interface
      *myContainer = myParent->myParent->myParent,
      *pContainer = other.myParent->myParent->myParent;

    const std::string
      &ipi = other.getInitialProviderInfo(otherProps),
      &iui = setInitialProviderInfo(myProps, ipi);
    if (!iui.empty()) {
      const std::string &fpi = other.setInitialUserInfo(iui);
      if (!fpi.empty()) {
	const std::string &fui = setFinalProviderInfo(fpi);
	if (!fui.empty())
	  other.setFinalUserInfo(fui);
      }
    }
  }

}

