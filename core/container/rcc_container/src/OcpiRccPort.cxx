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
Port::MyConnection::
init( PortData * src,
      bool       slocal,
      PortData * input,
      bool       tlocal)
{
  if ( slocal ) {
    lsrc = static_cast<Port*>(src);
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
    linput = static_cast<Port*>(input);
    linput->external = false;
  }
  else {
    rinput = *input;
    rinput.external = true;
    linput = NULL;
  }
}

void 
Port::
initOutputPort()
{
  if ( m_circuit )  {
    m_circuit->finalize( connectionData.data.desc.oob.oep );
  }
  else {
    m_circuit = parent().getTransport().createCircuit( connectionData.data );
    parent().parent().addCircuit( m_circuit );
  }
  m_dtPort = m_circuit->getOutputPort();
  parent().updatePort( *this );
}


void
Port::
processPortProperties(OU::PValue* props )
{
  const char *role = 0;
  OU::findString(props, "xferRole", role);
  if (!role)
    OU::findString(props, "role", role);
  if (role && 
       (strcasecmp(role,"flowcontrol") == 0 ||
	strcasecmp(role,"activeflowcontrol") == 0))
    connectionData.data.role = OCPI::RDT::ActiveFlowControl;
  OA::ULong ul;
  if (OU::findULong(props, "bufferCount", ul))
    connectionData.data.desc.nBuffers = ul;

  if (OU::findULong(props, "bufferSize", ul))
    connectionData.data.desc.dataBufferSize = ul;
}

void 
Port::
initInputPort()
{
  if ( m_dtPort ) {
    return;
  }
  if ( m_circuit ) {
    m_dtPort = parent().getTransport().createInputPort( m_circuit, connectionData.data );
  }
  else {
    m_dtPort = parent().getTransport().createInputPort( m_circuit, connectionData.data );
    parent().parent().addCircuit( m_circuit );
  }

  // We need to get the port descriptor
  m_dtPort->getPortDescriptor( connectionData.data, NULL );

}



Port::
Port(Worker& w, const OU::PValue *props,
     OCPI::Metadata::Port & pmd,  const char * endpoint)
  : OC::PortBase<Worker, Port, ExternalPort>(w, props, pmd),
    m_dtPort(NULL), m_portOrdinal(pmd.ordinal), m_circuit(NULL), m_props(NULL),
    m_mutex(m_container)
{

  connectionData.port = (OC::PortDesc)this;
  connectionData.container_id = m_container.getId();  
  connectionData.data.desc.nBuffers = (pmd.minBufferCount == 0) ? pmd.DEFAULT_NBUFFERS : pmd.minBufferCount;
  connectionData.data.desc.dataBufferSize = (pmd.minBufferSize == 0)  ? pmd.DEFAULT_BUFFER_SIZE : pmd.minBufferSize;
  connectionData.data.role = OCPI::RDT::ActiveMessage;
  connectionData.data.options =
    (1 << OCPI::RDT::ActiveFlowControl) |
    (1 << OCPI::RDT::ActiveMessage);


  processPortProperties( m_props );
  if ( endpoint ) {
    strcpy( connectionData.data.desc.oob.oep, endpoint );
  }

  if ( ! isProvider() ) {
    // The output port does not get initialized until it gets connected
  }
  else {


    //    initInputPort();
    //    ocpiAssert( m_circuit );


  }
}




Port::
~Port()
{
  disconnect();
}


void 
Port::
connectInside(OC::Port & other, const OU::PValue * my_props,  const OU::PValue * /* other_props */ )
{
  ( void ) my_props;
  std::string ref;
  connectInputPort( &other, ref, m_props);
}


const std::string& 
Port::
getInitialProviderInfo(const OU::PValue* props)
{
  OU::AutoMutex guard ( m_mutex,  true ); 
  ocpiAssert( isProvider() );
  applyConnectParams(props);
  connectionData.data.desc.nBuffers = myDesc.nBuffers;
  connectionData.data.desc.dataBufferSize = myDesc.dataBufferSize;
  initInputPort();
  m_ourFinalDescriptor = OC::Container::packPortDesc( *this );

  // url only, no cc
  return m_ourFinalDescriptor;
}

const std::string& 
Port::
setInitialProviderInfo(const OU::PValue* props, const std::string & user )
{
  OU::AutoMutex guard ( m_mutex,  true ); 
  ocpiAssert( ! isProvider() );
  applyConnectParams(props);
  PortData otherPortData;
  OC::Container::unpackPortDesc(user, &otherPortData);
  establishRoles(otherPortData.connectionData.data);
  finishConnection(otherPortData.connectionData.data);

  PortData tpdata;
  // FIXME: why is this unpacking done twice?
  OC::Container::unpackPortDesc( user, &tpdata );
  connectInputPort( &tpdata, m_localShadowPort, NULL  );
  if ( connectionData.data.role == OCPI::RDT::ActiveMessage ) {
    return m_localShadowPort;
  }
  else {
    PortData desc;
    m_dtPort->getPortDescriptor( desc.connectionData.data, &tpdata.connectionData.data );
    desc.connectionData.container_id = m_container.getId();
    m_initialPortInfo = OC::Container::packPortDesc( desc );
  }
  // FIXME: why is this packing done twice?
  m_initialPortInfo = OC::Container::packPortDesc( *this );
  return m_initialPortInfo;
}

const std::string& 
Port::
setFinalProviderInfo(const std::string & input_port )
{
  OU::AutoMutex guard ( m_mutex,  true ); 
  ocpiAssert( ! isProvider() );
  PortData tpdata;
  OC::Container::unpackPortDesc( input_port, &tpdata );

  if ( m_dtPort ) 
    m_dtPort->finalize( tpdata.connectionData.data );

  m_ourFinalDescriptor = OC::Container::packPortDesc( *this );
  return m_ourFinalDescriptor;
}


const std::string& 
Port::
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
  m_dtPort->finalize( src.connectionData.data );
  PortData desc;
  strncpy( desc.connectionData.data.desc.oob.oep,  connectionData.data.desc.oob.oep, OCPI::RDT::MAX_EPS_SIZE);
  m_dtPort->getPortDescriptor( desc.connectionData.data, &src.connectionData.data );
  connectionData.data.desc.oob.cookie = desc.connectionData.data.desc.oob.cookie;
  m_initialPortInfo = OC::Container::packPortDesc( *this );
  return m_initialPortInfo;
}

void 
Port::
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
  OCPI::DataTransport::Buffer *m_buffer;
  OCPI::DataTransport::Port   *m_dtPort;
  OS::Mutex                   *m_mutex; // not a reference since we construct as an array
};

class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
public:
  friend class ExternalBuffer;
  ExternalPort(Port &p, const char* name, const OU::PValue *props, const OM::Port &mPort, OS::Mutex & mutex ) 
    : OC::ExternalPortBase<Port,ExternalPort>(p, name, props, mPort), m_mutex(mutex)
  {
    unsigned nBuffers = p.dtPort()->getBufferCount();
    // Use an array here for scalability
    m_exBuffers = new ExternalBuffer[nBuffers];
    for (unsigned int n = 0; n < nBuffers; n++ ) {
	m_exBuffers[n].m_buffer = p.dtPort()->getBuffer(n);
	m_exBuffers[n].m_dtPort = p.dtPort();
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
  m_dtPort->advance(m_buffer);
}

// For producer buffers
void 
ExternalBuffer::
put(uint32_t length, uint8_t opCode, bool endOfData)
{        
  OU::AutoMutex guard ( *m_mutex, true ); 
  // FIXME:  check for value opcode and length values.
  if ( endOfData ) length = 0;
  m_buffer->getMetaData()->ocpiMetaDataWord.opCode = opCode;
  m_dtPort->advance(m_buffer,length);
}


// This port is an input port to the external user and an output port to the worker,
// so the worker produces data and the external user gets it
OC::ExternalBuffer *
ExternalPort::
getBuffer(uint8_t *&data, uint32_t &length, uint8_t &opCode, bool &endOfData)
{
  OU::AutoMutex guard ( m_mutex, true ); 
  OCPI::DataTransport::Buffer * b = NULL;
  if ( parent().dtPort()->hasFullInputBuffer() ) {
    b = parent().dtPort()->getNextFullInputBuffer();
    int oc = b->getMetaData()->ocpiMetaDataWord.opCode;
    opCode = oc;
    data = (uint8_t*)b->getBuffer();
    length = b->getMetaData()->ocpiMetaDataWord.length;
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
  OCPI::DataTransport::Buffer * b = NULL;
  if ( parent().dtPort()->hasEmptyOutputBuffer() ) {
    b = parent().dtPort()->getNextEmptyOutputBuffer();
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
Port::
connectExternal(const char * name, const OU::PValue * myprops, const OU::PValue * oprops)
{

  OU::AutoMutex guard ( m_mutex,  true ); 
  // We will create a single port worker to do this so that we can use the mechanics of the 
  // underlying system
  OC::Worker & w = *new Worker(parent().parent(), NULL,
			       (const char *)&stub_dispatch, NULL, NULL, NULL);
  Port * p;
  if ( isProvider() ) {
    p = static_cast<Port*>
      (&w.createOutputPort(0, connectionData.data.desc.nBuffers,
			   connectionData.data.desc.dataBufferSize ));
    p->connect( *this, myprops, oprops );
  }
  else {
    p = static_cast<Port*>
      (&w.createInputPort(0, 
			  connectionData.data.desc.nBuffers,
			  connectionData.data.desc.dataBufferSize ));
    this->connect( *p, myprops, oprops );
  }
  return *new ExternalPort(*p, name, NULL, metaPort(), m_mutex);
}

void 
Port::
disconnect(    OC::PortData* sp,                 
               OC::PortData* input
               )
{
  OU::AutoMutex guard (parent().mutex(), true);
  OCPI::DataTransport::Circuit* scircuit=NULL;
  OCPI::DataTransport::Circuit* tcircuit=NULL;
  OCPI::DataTransport::Port* port=NULL;

  if ( ! sp->external ) {
    Port* p = reinterpret_cast<Port*>(sp->connectionData.port);
    scircuit = p->m_circuit;
    port = p->m_dtPort;

    if ( !scircuit || scircuit->isCircuitOpen() ) {
      // Not connected.
      return;
    }
    if ( p->parent().enabled ) {
      throw OU::EmbeddedException( OC::ONP_WORKER_STARTED, NULL, OC::ApplicationRecoverable);
    }
    port->reset();
    p->parent().m_context->connectedPorts &= ~(1<<p->m_portOrdinal);
    p->m_circuit->release();
    p->m_circuit = NULL;
  }


  // Now the input, but only if it is local
  if ( ! input->external ) {
    Port* p = reinterpret_cast<Port*>(input->connectionData.port);
    tcircuit = p->m_circuit;
    port = p->m_dtPort;

    if ( p->parent().enabled ) {
      throw OU::EmbeddedException( OC::ONP_WORKER_STARTED, NULL, OC::ApplicationRecoverable);
    }
    if ( !tcircuit || tcircuit->isCircuitOpen() ) {
      throw OU::EmbeddedException( OC::BAD_CONNECTION_COOKIE, "Worker not found for input port",
				   OC::ApplicationRecoverable);
    }
    port->reset();
    p->parent().m_context->connectedPorts &= ~(1<<p->m_portOrdinal);

    p->m_circuit->release();
    p->m_circuit = NULL;
  }

}





void 
Port::
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
Port::
connectExternalInputPort( PortData *           inputPort,    
                          OU::PValue*       props
                          )
{
  ( void ) props;
  OU::AutoMutex guard ( m_mutex,  true ); 

  // Make sure the the input port endpoint is registered, this may be the first time that
  // we talk to this endpoint
  parent().getTransport().addRemoteEndpoint( inputPort->connectionData.data.desc.oob.oep );

  // Initialize the port
  initOutputPort();
  
  // Add the ports to the circuit
  OC::PortConnectionDesc  flowControl;
  m_circuit->addInputPort( inputPort->connectionData.data, connectionData.data.desc.oob.oep, &flowControl.data );
  flowControl.port = (OC::PortDesc)inputPort;
  parent().m_context->connectedPorts |= (1<<m_portOrdinal);
  flowControl.container_id = m_container.getId();

  OC::PortData desc;
  strncpy( desc.connectionData.data.desc.oob.oep,  connectionData.data.desc.oob.oep, OCPI::RDT::MAX_EPS_SIZE );
  m_dtPort->getPortDescriptor( desc.connectionData.data, &inputPort->connectionData.data );
  flowControl.data.desc.oob.cookie = desc.connectionData.data.desc.oob.cookie;

  return flowControl;
}


void
Port::
connectInputPort( PortData *    inputPort,    
                   std::string&  lPort,
                  OU::PValue*       props
                  )
  throw ( OU::EmbeddedException )
{
  TRACE("OCPI::RCC::Container::connectInputPort()");
  OU::AutoMutex guard ( m_mutex,  true ); 

  PortData          localShadowPort;

  // At this point the output ports reoutputs are not yet created, we need to 
  // create a local endpoint that is compatible with the remote.
  std::string s = parent().getTransport().getLocalCompatibleEndpoint( inputPort->connectionData.data.desc.oob.oep );
  s = parent().getTransport().addLocalEndpoint( s.c_str() )->sMemServices->endpoint()->end_point;
  strcpy( connectionData.data.desc.oob.oep, s.c_str());  

  // At some point we may want to make this smarter, but we want to make sure
  // that we dont overrun the inputs buffers.
  if ( connectionData.data.desc.dataBufferSize > inputPort->connectionData.data.desc.dataBufferSize ) {
    connectionData.data.desc.dataBufferSize = inputPort->connectionData.data.desc.dataBufferSize;
  }

  OC::PortData* rtpd=0;
  Port* rp=NULL;
  if ( inputPort->connectionData.container_id != m_container.getId()  ) {  // external
    inputPort->external = true;
    OC::PortConnectionDesc fcd = connectExternalInputPort(inputPort,props);
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
    throw OU::EmbeddedException( OC::NO_MORE_MEMORY, "new", OC::ContainerFatal);
  }
  lPort = OC::Container::packPortDesc( localShadowPort );
}


void
Port::
setOutputFlowControl( PortData * srcPort )
  throw ( OU::EmbeddedException )
{
  ocpiAssert( isProvider() );
  OU::AutoMutex guard ( m_mutex,  true ); 

  // Local connection
  if ( connectionData.container_id == srcPort->connectionData.container_id ) {
    try {
      Port* p = reinterpret_cast<Port*>(srcPort->connectionData.port);
      p->m_circuit = m_circuit;
      m_connectionCookie.init( srcPort,true, this,true );
    }
    catch( std::bad_alloc ) {
      throw OU::EmbeddedException( OC::NO_MORE_MEMORY, "new", OC::ContainerFatal);
    }
    return;
  }
  m_dtPort->setFlowControlDescriptor( srcPort->connectionData.data );
  try {
    m_connectionCookie.init( srcPort,false,this,true );
  }
  catch( std::bad_alloc ) {
    throw OU::EmbeddedException( OC::NO_MORE_MEMORY, "new", OC::ContainerFatal);
  }
}


/**********************************
 * This method is used to connect the external ports decribed in the
 * the PortDependencyData structure to output port for the given worker.
 *********************************/
void
Port::
connectInternalInputPort( OC::Port *  tPort,
                          OU::PValue*            props  )
{
  ( void ) props;
  OU::AutoMutex guard ( m_mutex,  true ); 

  Port *    inputPort = static_cast<Port*>(tPort);

  inputPort->initInputPort();

  // Allocate a connection
  m_circuit = inputPort->m_circuit;
  m_circuit->attach();

  strcpy( connectionData.data.desc.oob.oep, inputPort->connectionData.data.desc.oob.oep  );

  // Initialize the port
  initOutputPort();


}


// Just guard the generic connect method with our mutex
void 
Port::
connect( OC::Port &other, const OU::PValue *myProps, const OU::PValue *otherProps) {
  OU::AutoMutex guard ( m_mutex,  true ); 
  return OC::Port::connect(other, myProps, otherProps);
}


}
}
