
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
#include <OcpiCP289Port.h>
#include <OcpiCP289Container.h>
#include <OcpiCP289Controller.h>
#include <OcpiCP289Application.h>
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

using namespace OCPI::Container;
using namespace OCPI::Util;
using namespace OCPI::CP289;

#define MyParent static_cast<OCPI::CP289::Worker*>(myParent)
#define MyApp static_cast<OCPI::CP289::Application*>(myParent->myParent)

void 
OCPI::CP289::Port::MyConnection::
init( PortData * src,
      bool       slocal,
      PortData * input,
      bool       tlocal)
{
  if ( slocal ) {
    lsrc = static_cast<OCPI::CP289::Port*>(src);
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
    linput = static_cast<OCPI::CP289::Port*>(input);
    linput->external = false;
  }
  else {
    rinput = *input;
    rinput.external = true;
    linput = NULL;
  }
}

void 
OCPI::CP289::Port::
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
OCPI::CP289::Port::
processPortProperties(OCPI::Util::PValue* props )
{
  const OCPI::Util::PValue* p = OCPI::Util::PValue::find(props, "xferRole");
  if (!p)
    p = OCPI::Util::PValue::find(props, "role");
  if ( p ) {
    if ( (p->type != OCPI::Util::Prop::Scalar::OCPI_String)) {
      throw OCPI::Util::EmbeddedException("\"xferrole\" property has wrong type, should be String");
    }
    if ( strcasecmp(p->vString,"flowcontrol") == 0 ||
	 strcasecmp(p->vString,"activeflowcontrol") == 0 ) {
      connectionData.data.role = OCPI::RDT::ActiveFlowControl;
    }
  }

  p = OCPI::Util::PValue::find(props, "bufferCount");
  if ( p ) {
    if ( (p->type != OCPI::Util::Prop::Scalar::OCPI_ULong)) {
      throw OCPI::Util::EmbeddedException("\"bufferCount\" property has wrong type, should be ULong");
    }
    connectionData.data.desc.nBuffers = p->vULong;
  }

  p = OCPI::Util::PValue::find(props, "bufferSize");
  if ( p ) {
    if ( (p->type != OCPI::Util::Prop::Scalar::OCPI_ULong)) {
      throw OCPI::Util::EmbeddedException("\"bufferSize\" property has wrong type, should be ULong");
    }
    connectionData.data.desc.dataBufferSize = p->vULong;
  }
}

void 
OCPI::CP289::Port::
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
  m_dtPort->getPortDescriptor( connectionData.data, NULL );

}



OCPI::CP289::Port::
Port(OCPI::Container::Worker& w, OCPI::Metadata::Port & pmd,  const char * endpoint )
  : OCPI::Container::Port(w,pmd),m_dtPort(NULL),m_portId(pmd.m_pid),m_circuit(NULL),m_props(NULL)
{

  connectionData.port = (PortDesc)this;
  connectionData.container_id = static_cast<OCPI::CP289::Container*>(myParent->myParent->myParent)->getId();  
  myMetaPort.provider = pmd.provider;
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




OCPI::CP289::Port::
~Port()
{
  disconnect();
}


void 
OCPI::CP289::Port::
connectInside(OCPI::Container::Port & other, OCPI::Util::PValue * my_props,  OCPI::Util::PValue * /* other_props */ )
{
  ( void ) my_props;
  std::string ref;
  connectInputPort( &other, ref, m_props);
}


const std::string& 
OCPI::CP289::Port::
getInitialProviderInfo(OCPI::Util::PValue* props)
{
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  ocpiAssert( isProvider() );
  applyConnectParams(props);
  connectionData.data.desc.nBuffers = myDesc.nBuffers;
  connectionData.data.desc.dataBufferSize = myDesc.dataBufferSize;
  initInputPort();
  m_ourFinalDescriptor = MyParent->myParent->myParent->packPortDesc( *this );

  // url only, no cc
  return m_ourFinalDescriptor;
}

const std::string& 
OCPI::CP289::Port::
setInitialProviderInfo(OCPI::Util::PValue* props, const std::string & user )
{
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  ocpiAssert( ! isProvider() );
  applyConnectParams(props);
  PortData otherPortData;
  myContainer.unpackPortDesc(user, &otherPortData);
  establishRoles(otherPortData.connectionData.data);
  finishConnection(otherPortData.connectionData.data);

  PortData tpdata;
  MyParent->myParent->myParent->unpackPortDesc( user, &tpdata );
  connectInputPort( &tpdata, m_localShadowPort, NULL  );
  if ( connectionData.data.role == OCPI::RDT::ActiveMessage ) {
    return m_localShadowPort;
  }
  else {
    PortData desc;
    m_dtPort->getPortDescriptor( desc.connectionData.data, &tpdata.connectionData.data );
    desc.connectionData.container_id = MyParent->myParent->myParent->getId();
    myInitialPortInfo = MyParent->myParent->myParent->packPortDesc( desc );
  }

  myInitialPortInfo = MyParent->myParent->myParent->packPortDesc( *this );
  return myInitialPortInfo;
}

const std::string& 
OCPI::CP289::Port::
setFinalProviderInfo(const std::string & input_port )
{
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  ocpiAssert( ! isProvider() );
  PortData tpdata;
  MyParent->myParent->myParent->unpackPortDesc( input_port, &tpdata );

  if ( m_dtPort ) 
    m_dtPort->finalize( tpdata.connectionData.data );

  m_ourFinalDescriptor = MyParent->myParent->myParent->packPortDesc( *this );
  return m_ourFinalDescriptor;
}


const std::string& 
OCPI::CP289::Port::
setInitialUserInfo(const std::string& user)
{
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  ocpiAssert( isProvider() );

  initInputPort();
  PortData src;
  PortData * pd;
  if ( ! (pd=MyParent->myParent->myParent->unpackPortDesc( user, &src ))) {
    throw OCPI::Util::EmbeddedException("Input Port descriptor is invalid");
  }
  setOutputFlowControl( pd );  
  m_dtPort->finalize( src.connectionData.data );
  PortData desc;
  strncpy( desc.connectionData.data.desc.oob.oep,  connectionData.data.desc.oob.oep, OCPI::RDT::MAX_EPS_SIZE);
  m_dtPort->getPortDescriptor( desc.connectionData.data, &src.connectionData.data );
  connectionData.data.desc.oob.cookie = desc.connectionData.data.desc.oob.cookie;
  myInitialPortInfo = MyParent->myParent->myParent->packPortDesc( *this );
  return myInitialPortInfo;
}

void 
OCPI::CP289::Port::
setFinalUserInfo(const std::string& srcPort )
{
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  ocpiAssert( isProvider() );
  PortData src;
  PortData * pd;
  if ( ! (pd=MyParent->myParent->myParent->unpackPortDesc( srcPort, &src ))) {
    throw OCPI::Util::EmbeddedException("Input Port descriptor is invalid");
  }

}

namespace OCPI {
  namespace CP289 {

    class ExternalBuffer : public OCPI::Container::ExternalBuffer {
    public:
      ExternalBuffer( ExternalPort& ep, Port & p, OCPI::DataTransport::Buffer* buffer ) 
        :OCPI::Container::ExternalBuffer(ep),m_buffer(buffer),m_port(p)
      {}

      virtual ~ExternalBuffer(){};

      // For consumer buffers
      void release();
      // For producer buffers
      void put(uint8_t opCode, uint32_t length, bool endOfData);

    private:
      OCPI::DataTransport::Buffer* m_buffer;
      Port & m_port;
    };

    class ExternalPort : public OCPI::Container::ExternalPort, OCPI::Util::Child<Port,OCPI::Container::ExternalPort> {    
    public:
      friend class ExternalBuffer;
      ExternalPort( const char* name, Port &p, OCPI::OS::Mutex & mutex ) 
        : OCPI::Container::ExternalPort(name), OCPI::Util::Child<Port,OCPI::Container::ExternalPort>(p),m_port(p),m_mutex(mutex)
      {
        for ( unsigned int n=0; n<p.dtPort()->getBufferCount(); n++ ) {
          m_exBuffers.push_back( new  ExternalBuffer( *this, p, p.dtPort()->getBuffer(n) ) );
        }
      }
      OCPI::Container::ExternalBuffer * getBuffer(uint8_t &opCode, uint8_t *&data, uint32_t &length, bool &endOfData);
      OCPI::Container::ExternalBuffer *getBuffer(uint8_t *&data, uint32_t &length);
      void endOfData();
      bool tryFlush();


    private:
      // External Buffers
      std::vector<ExternalBuffer*> m_exBuffers;
      Port & m_port; 
      OCPI::OS::Mutex & m_mutex;     

      virtual ~ExternalPort(){}
    };

  }
}




void 
OCPI::CP289::ExternalBuffer::
release()
{
  OCPI::Util::AutoMutex guard ( static_cast<OCPI::CP289::ExternalPort*>(myParent)->m_mutex, true ); 
  m_port.dtPort()->advance(m_buffer);
}

// For producer buffers
void 
OCPI::CP289::ExternalBuffer::
put(uint8_t opCode, uint32_t length, bool endOfData)
{        
  OCPI::Util::AutoMutex guard ( static_cast<OCPI::CP289::ExternalPort*>(myParent)->m_mutex, true ); 
  if ( endOfData ) length = 0;
  m_buffer->getMetaData()->ocpiMetaDataWord.opCode = opCode;
  m_port.dtPort()->advance(m_buffer,length);
}


// This port is an input port to the external user and an output port to the worker,
// so the worker produces data and the external user gets it
OCPI::Container::ExternalBuffer *
OCPI::CP289::ExternalPort::
getBuffer(uint8_t &opCode, uint8_t *&data, uint32_t &length, bool &endOfData)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  OCPI::DataTransport::Buffer * b = NULL;
  if ( m_port.dtPort()->hasFullInputBuffer() ) {
    b = m_port.dtPort()->getNextFullInputBuffer();
    int oc = b->getMetaData()->ocpiMetaDataWord.opCode;
    opCode = oc;
    data = (uint8_t*)b->getBuffer();
    length = b->getMetaData()->ocpiMetaDataWord.length;
    endOfData = (length == 0) ? true : false;
    return m_exBuffers[b->getTid()];
  }
  return NULL;
}


// This port is an output port to the external user and an input port to the worker
// so the external user produces data and 
OCPI::Container::ExternalBuffer * 
OCPI::CP289::ExternalPort::
getBuffer(uint8_t *&data, uint32_t &length)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  OCPI::DataTransport::Buffer * b = NULL;
  if ( m_port.dtPort()->hasEmptyOutputBuffer() ) {
    b = m_port.dtPort()->getNextEmptyOutputBuffer();
    data = (uint8_t*)b->getBuffer();
    length = b->getLength();
    return m_exBuffers[b->getTid()];
  }
  return NULL;
}

void 
OCPI::CP289::ExternalPort::
endOfData()
{
  // ??

}


bool 
OCPI::CP289::ExternalPort::
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


OCPI::Container::ExternalPort& 
OCPI::CP289::Port::
connectExternal(const char * name, OCPI::Util::PValue * myprops, OCPI::Util::PValue * oprops)
{

  // We will create a single port worker to do this so that we can use the mechanics of the 
  // underlying system
  OCPI::Container::Worker & w = 
#if 0
    MyApp->createWorker(NULL,NULL,&stub_dispatch);
#else
  // That application (above) method was being misused, but this isn't much better...
  *(new OCPI::CP289::Worker( *myParent->myParent, &stub_dispatch, NULL,
			    static_cast<Container *>(myParent->myParent->myParent), NULL, NULL ));
#endif
  Port * p;
  if ( isProvider() ) {
    p = static_cast<OCPI::CP289::Port*>(&w.createOutputPort( 0, 
                            connectionData.data.desc.nBuffers, connectionData.data.desc.dataBufferSize ));

    p->connect( *this, myprops, oprops );

    /*
    std::string fb = p->setFinalProviderInfo( getInitialProviderInfo(0) );
    setFinalUserInfo( fb );
    */
  }
  else {
    p = static_cast<OCPI::CP289::Port*>(&w.createInputPort( 0, 
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
OCPI::CP289::Worker* toWorker(WorkerId & id)
{
  if ( id == 0 ) {
    throw OCPI::Util::EmbeddedException( "Worker Not Found" );
  }
  return reinterpret_cast<OCPI::CP289::Worker*>(id);
}


void 
OCPI::CP289::Port::
disconnect(    OCPI::Container::PortData* sp,                 
               OCPI::Container::PortData* input
               )
{

  OCPI::CP289::Worker * swi=NULL;
  OCPI::DataTransport::Circuit* scircuit=NULL;
  OCPI::DataTransport::Circuit* tcircuit=NULL;
  OCPI::DataTransport::Port* port=NULL;

  if ( ! sp->external ) {
    OCPI::CP289::Port* p = reinterpret_cast<OCPI::CP289::Port*>(sp->connectionData.port);
    scircuit = p->m_circuit;
    port = p->m_dtPort;
    swi = static_cast<OCPI::CP289::Worker*>(p->myParent);

    if ( !scircuit || scircuit->isCircuitOpen() ) {
      // Not connected.
      return;
    }
    if ( swi->enabled ) {
      throw OCPI::Util::EmbeddedException( ONP_WORKER_STARTED, NULL, ApplicationRecoverable);
    }
    port->reset();
    swi->m_rcc_worker->m_context->connectedPorts &= ~(1<<p->m_portId);
    p->m_circuit->release();
    p->m_circuit = NULL;
  }


  // Now the input, but only if it is local
  OCPI::CP289::Worker * twi = NULL;
  if ( ! input->external ) {
    OCPI::CP289::Port* p = reinterpret_cast<OCPI::CP289::Port*>(input->connectionData.port);
    tcircuit = p->m_circuit;
    port = p->m_dtPort;
    twi = static_cast<OCPI::CP289::Worker*>(p->myParent);

    if ( twi->enabled ) {
      throw OCPI::Util::EmbeddedException( ONP_WORKER_STARTED, NULL, ApplicationRecoverable);
    }
    if ( !tcircuit || tcircuit->isCircuitOpen() ) {
      throw OCPI::Util::EmbeddedException( BAD_CONNECTION_COOKIE, "Worker not found for input port", ApplicationRecoverable);
    }
    port->reset();
    twi->m_rcc_worker->m_context->connectedPorts &= ~(1<<p->m_portId);

    p->m_circuit->release();
    p->m_circuit = NULL;
  }

}





void 
OCPI::CP289::Port::
disconnect( )
  throw ( OCPI::Util::EmbeddedException )
{
  TRACE(" OCPI::CP289::Container::disconnectPorts()");
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
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
OCPI::CP289::Port::
connectExternalInputPort( PortData *           inputPort,    
                          OCPI::Util::PValue*       props
                          )
{
  ( void ) props;
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

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

  PortData desc;
  strncpy( desc.connectionData.data.desc.oob.oep,  connectionData.data.desc.oob.oep, OCPI::RDT::MAX_EPS_SIZE );
  m_dtPort->getPortDescriptor( desc.connectionData.data, &inputPort->connectionData.data );
  flowControl.data.desc.oob.cookie = desc.connectionData.data.desc.oob.cookie;

  return flowControl;
}


void
OCPI::CP289::Port::
connectInputPort( PortData *    inputPort,    
                   std::string&  lPort,
                  OCPI::Util::PValue*       props
                  )
  throw ( OCPI::Util::EmbeddedException )
{
  TRACE("OCPI::CP289::Container::connectInputPort()");
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  PortData          localShadowPort;

  // At this point the output ports reoutputs are not yet created, we need to 
  // create a local endpoint that is compatible with the remote.
  std::string s = MyParent->getTransport().getLocalCompatibleEndpoint( inputPort->connectionData.data.desc.oob.oep );
  s = MyParent->getTransport().addLocalEndpoint( s.c_str() )->sMemServices->endpoint()->end_point;
  strcpy( connectionData.data.desc.oob.oep, s.c_str());  

  // At some point we may want to make this smarter, but we want to make sure
  // that we dont overrun the inputs buffers.
  if ( connectionData.data.desc.dataBufferSize > inputPort->connectionData.data.desc.dataBufferSize ) {
    connectionData.data.desc.dataBufferSize = inputPort->connectionData.data.desc.dataBufferSize;
  }

  PortData* rtpd=0;
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
    throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }
  lPort = MyParent->myParent->myParent->packPortDesc( localShadowPort );
}


void
OCPI::CP289::Port::
setOutputFlowControl( PortData * srcPort )
  throw ( OCPI::Util::EmbeddedException )
{
  ocpiAssert( isProvider() );
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  // Local connection
  if ( connectionData.container_id == srcPort->connectionData.container_id ) {
    try {
      OCPI::CP289::Port* p = reinterpret_cast<OCPI::CP289::Port*>(srcPort->connectionData.port);
      p->m_circuit = m_circuit;
      m_connectionCookie.init( srcPort,true, this,true );
    }
    catch( std::bad_alloc ) {
      throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
    }
    return;
  }
  m_dtPort->setFlowControlDescriptor( srcPort->connectionData.data );
  try {
    m_connectionCookie.init( srcPort,false,this,true );
  }
  catch( std::bad_alloc ) {
    throw OCPI::Util::EmbeddedException( NO_MORE_MEMORY, "new", ContainerFatal);
  }
}


/**********************************
 * This method is used to connect the external ports decribed in the
 * the PortDependencyData structure to output port for the given worker.
 *********************************/
void
OCPI::CP289::Port::
connectInternalInputPort( OCPI::Container::Port *  tPort,
                          OCPI::Util::PValue*            props  )
{
  ( void ) props;
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 

  OCPI::CP289::Port *    inputPort = static_cast<OCPI::CP289::Port*>(tPort);

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
OCPI::CP289::Port::
connect( OCPI::Container::Port &other, OCPI::Util::PValue *myProps, OCPI::Util::PValue *otherProps) {
  OCPI::Util::AutoMutex guard ( MyApp->mutex(),  true ); 
  return OCPI::Container::Port::connect(other, myProps, otherProps);
}


