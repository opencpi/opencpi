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
#include <OcpiBuffer.h>
#include <OcpiRDTInterface.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilCDR.h>
#include <OcpiPortMetaData.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiContainerErrorCodes.h>
#include <OcpiContainerMisc.h>
#include <DtTransferInternal.h>
#include <OcpiIntParallelDataDistribution.h>
#include <OcpiPValue.h>
#include <OcpiUtilMisc.h>
#include <OcpiParentChild.h>
#include <OcpiMetadataWorker.h>

namespace OM = OCPI::Metadata;
namespace OP = OCPI::Util::Prop;
namespace OC = OCPI::Container;
namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OA = OCPI::API;

namespace OCPI {
  namespace RCC {
#if 0
#define SET_LAST_ERROR_TO_WORKER_ERROR_STRING(x)                        \
  if ( x->m_rcc_worker->m_context->errorString ) {                                \
    x->m_lastError=x->m_rcc_worker->m_context->errorString;                        \
  }                                                                        \
  else {                                                                \
    x->m_lastError = "Worked returned failure, but did not set errorString"; \
  }                                                                        \
  x->m_rcc_worker->m_context->errorString=NULL;

#define SET_LAST_ERROR_UNKNOWN_WORKER(x) x->m_lastError = "Unknown Worker Type"
#define SET_LAST_ERROR_WORKER_IN_UNUSABLE_STATE(x) x->m_lastError = "Worker in Unusable State"
#define SET_LAST_ERROR_INVALID_SEQUENCE(x) x->m_lastError = "Invalid Control Sequence"
#define SET_LAST_ERROR_ALL_REQUIRED_PORTS_NOT_CONNECTED(x) x->m_lastError = "Not all required ports are connected"
#define SET_LAST_ERROR_PORT_COUNT_MISMATCH(x) x->m_lastError = "Input/Output port count mismatch between worker and meta-data"
#define SET_LAST_ERROR_TO_TEST_NOT_IMPLEMENTED(x) x->m_lastError = "Test Not Implemented"
#define SET_LAST_ERROR_TO_PROPTERY_OVERRUN(x) x->m_lastError = "Property Overrun error"
#endif

void 
Worker::
overRidePortInfo( OM::Port & portData )
{
  RCCPortInfo* pi = NULL;
  if ( m_dispatch->portInfo == NULL ) {
    return;
  }
  int n=0;
  while ( m_dispatch->portInfo[n].port != RCC_NO_ORDINAL ) {
    if ( m_dispatch->portInfo[n].port == portData.ordinal  ) {
      pi = &m_dispatch->portInfo[n];
      break;
    }
    n++;
  }
  if ( pi ) {
#ifndef NDEBUG
    printf("\nWorker Port info is non NULL, overriding port defaults from s,bc -> %d,%d to %d,%d\n",
           portData.minBufferCount, portData.minBufferSize, pi->minBuffers, pi->maxLength );
#endif
    portData.minBufferCount = ( portData.minBufferCount > pi->minBuffers ) ? portData.minBufferCount : pi->minBuffers;
    portData.minBufferSize  = portData.maxBufferSize  = pi->maxLength;

  }
}

Worker::
Worker( Application & app, Artifact *art, const char *name,
	ezxml_t impl, ezxml_t inst, const OU::PValue *wParams)
  : OC::WorkerBase<Application,Worker,Port>(app, art, name, impl, inst, wParams),
    // Note the "hack" to use "name" as dispatch when artifact is not present
    m_context(0), m_mutex(app.container()), enabled(false),sourcePortCount(0),targetPortCount(0),
    sourcePorts(1), targetPorts(1), runConditionSS(0), worker_run_count(0),
    m_transport(app.parent().getTransport())
{
  if (art)
    m_dispatch = art->getDispatch(ezxml_cattr(impl, "name"));
  else
    m_dispatch = (RCCDispatch *)name;

  initializeContext();
  // If we have an event handler, we need to inform it about the timeout
  if (  m_dispatch->runCondition && m_dispatch->runCondition->timeout ) {
    runTimeout.seconds = m_dispatch->runCondition->usecs / 1000000;
    runTimeout.nanoseconds = (m_dispatch->runCondition->usecs%1000000) * 1000;
    if ( m_transport.m_transportGlobal->getEventManager() ) {

#ifdef EM_PORT_COMPLETE
      parent().myparent->m_transport->m_transportGlobal->getEventManager()->setMinTimeout( workerId, 
                                                                        wd->runCondition->usecs );
#endif

    }
  }
}

void 
Worker::
read(uint32_t offset, 
     uint32_t nBytes, 
     void * p_data  )
{
  OU::AutoMutex guard(m_mutex);
  if ( (offset+nBytes) > m_dispatch->propertySize ) {
    throw OU::EmbeddedException( OC::PROPERTY_GET_EXCEPTION, NULL, OC::ApplicationRecoverable);
  }
  memcpy( p_data, (char*)m_context->properties+offset, nBytes );
}

void 
Worker::
write(uint32_t offset, 
      uint32_t nBytes, 
      const void * p_data  )
{
  OU::AutoMutex guard (m_mutex);
  if ( (offset+nBytes) > m_dispatch->propertySize ) {
    throw OU::EmbeddedException( OC::PROPERTY_SET_EXCEPTION, NULL, OC::ApplicationRecoverable);
  }
  memcpy( (char*)m_context->properties+offset, p_data, nBytes );
}

Worker::
~Worker()
{
  // FIXME - this sort of thing should be generic
  if (enabled) {
    enabled = false;
    controlOperation(OM::Worker::OpStop);
    controlOperation(OM::Worker::OpRelease);
  }
#ifdef EM_PORT_COMPLETE
    RCCDispatch* wd = m_dispatch;
    // If we have an event handler, we need to inform it about the timeout
    if ( wd->runCondition && wd->runCondition->timeout ) {
      if ( parent().m_transport->m_transportGlobal->getEventManager() ) {
        parent().m_transport->m_transportGlobal->getEventManager()->removeMinTimeout( w->workerId );
      }
    }
#endif

  unsigned nports = m_dispatch->numInputs + m_dispatch->numOutputs;
  for (unsigned n=0; n<nports; n++ ) {
    OpaquePortData *opd = static_cast<OpaquePortData*>(m_context->ports[n].opaque);
    delete opd;
  }

  OS::uint32_t m = 0;
  while ( m_context->memories && m_context->memories[m] ) {
    delete [] (char*)m_context->memories[m];
    m++;
  }
  delete[] m_context->memories;
  delete[] m_context->runCondition->portMasks;
  delete m_context->runCondition;
  delete[] (char*)m_context->properties;
  delete[] m_context;
}

// FIXME:  recover memory on exceptions
void Worker::
initializeContext()
{        
  RCCDispatch *wd = m_dispatch;
  // Create our memory spaces
  int idx=0;
  while( wd->memSizes && wd->memSizes[idx] ) {
#ifndef NDEBUG
    printf("Allocating %d bytes of data for worker memory\n", wd->memSizes[idx] );
#endif
    idx++;
  }
  int mcount = idx;

  // check masks for bad bits
  unsigned nports = wd->numInputs+wd->numOutputs;
  int32_t mask = -1 << nports;
  if (mask & wd->optionallyConnectedPorts)
    throw OU::EmbeddedException( OC::PORT_COUNT_MISMATCH,
				 "optional port mask is invalid",
				 OC::ApplicationRecoverable);
  unsigned rc_count = 0;
  if ( wd->runCondition && wd->runCondition->portMasks )
    while (wd->runCondition->portMasks[rc_count] ) {
      if (mask & wd->runCondition->portMasks[rc_count])
	throw OU::EmbeddedException( OC::PORT_COUNT_MISMATCH,
				     "run condition mask is invalid",
				     OC::ApplicationRecoverable);
      rc_count++;
    }

  // Create our context
  m_context = (RCCWorker *)new char[sizeof(RCCWorker)+(nports)*sizeof(RCCPort)];
  m_context->properties = 0;
  m_context->container = &parent().parent();
  if ( mcount ) {
    try {
      m_context->memories = new void*[mcount+1];
    }
    catch( std::bad_alloc ) {
      delete[] m_context;
      throw OU::EmbeddedException( OC::NO_MORE_MEMORY, "worker requested too much memory", OC::ApplicationRecoverable);
    }
    m_context->memories[mcount] = NULL;
  }
  else {
    m_context->memories = NULL;
  }
  
  m_context->connectedPorts = 0;
  m_context->runCondition = new RCCRunCondition;
 
  if ( wd->runCondition ) {
    if ( wd->runCondition->portMasks ) {
      // Add one for the zero based terminator
      rc_count++;

      m_context->runCondition->portMasks = new RCCPortMask[rc_count];
      memcpy( m_context->runCondition->portMasks, wd->runCondition->portMasks, sizeof(RCCPortMask)*rc_count);
    }  
    else { // portMask is NULL, per CP289 spec, timeout only run condition
      m_context->runCondition->portMasks = new RCCPortMask[1];
      m_context->runCondition->portMasks[0] = 0;
    }
    
  }
  else {
    rc_count = 1;
    m_context->runCondition->portMasks = new RCCPortMask[2];
    m_context->runCondition->portMasks[0] = 0;
    m_context->runCondition->portMasks[1] = 0;

    for ( int n=0; n<(wd->numInputs+wd->numOutputs); n++ ) {
      m_context->runCondition->portMasks[0] |=  (1<<n);
    }
  }

  // This is the superset of the run conditions
  for (unsigned n=0; n< rc_count; n++ )
    runConditionSS |= m_context->runCondition->portMasks[n];

  // We create one of these structures for all of the ports that are defined in the worker.  However,
  // the actual data ports may be optional at runtime.
  for (unsigned n=0; n<nports; n++ ) {
    m_context->ports[n].opaque = new OpaquePortData();
    m_context->ports[n].current.data = 0;
    m_context->ports[n].current.maxLength = 0;
    m_context->ports[n].callBack = 0;
    //    static_cast<OpaquePortData*>(m_context->ports[n].opaque)->worker = wi;
    static_cast<OpaquePortData*>(m_context->ports[n].opaque)->cp289Port = &m_context->ports[n];
  }

  idx=0;
  while( wd->memSizes && wd->memSizes[idx] ) {
    try {
      m_context->memories[idx] = new char*[wd->memSizes[idx]];
    }
    catch( std::bad_alloc ) {
      delete[] m_context;
      throw OU::EmbeddedException( OC::NO_MORE_MEMORY, "worker requested too much memory", OC::ApplicationRecoverable);
    }
    idx++;
  }
  
  // Now create and initialize the worker properties
  m_context->properties = new char[wd->propertySize + 4];

  memset(m_context->properties, 0, sizeof(char)*wd->propertySize);
        
}

// FIXME:  parse buffer counts (at least) from PValue
OC::Port & 
Worker::
createPort(OCPI::Metadata::Port& mp, const OCPI::Util::PValue * )
{

  int bSize = ( mp.minBufferSize == 0 ) ? OCPI::Metadata::Port::DEFAULT_BUFFER_SIZE : mp.minBufferSize;
  int bCount = ( mp.minBufferCount == 0 ) ? OCPI::Metadata::Port::DEFAULT_NBUFFERS : mp.minBufferCount;
  if ( mp.provider ) {
    return createInputPort( mp.ordinal, bCount, bSize, NULL );
  }
  else {
    return createOutputPort( mp.ordinal, bCount, bSize, NULL );
  }

}


 OC::Port &
Worker::
createOutputPort( 
                 OM::PortOrdinal     portId,
                 OS::uint32_t    bufferCount,
                 OS::uint32_t    bufferSize, 
                 const OU::PValue*              props            
                 )
  throw ( OU::EmbeddedException )
{
  ( void ) props;
  TRACE(" OCPI::RCC::Container::createOutputPort()");
  OU::AutoMutex guard (m_mutex);

  sourcePortCount++;

  if ( sourcePortCount > m_dispatch->numOutputs  ) {
    throw OU::EmbeddedException( OC::PORT_NOT_FOUND, "Source Port count exceeds configuration", OC::ApplicationFatal);
  }
  if ( portId >= (m_dispatch->numInputs + m_dispatch->numOutputs) ) {
    throw OU::EmbeddedException( OC::PORT_NOT_FOUND, "Port id exceeds port count", OC::ApplicationFatal);
  }
  if ( portId > 32 ) {
    throw OU::EmbeddedException( OC::PORT_NOT_FOUND, "Port id exceeds max port count of 32", OC::ApplicationFatal);
  }

  OCPI::Metadata::Port pmd;
  pmd.ordinal = portId;
  pmd.provider = false;
  pmd.minBufferCount = bufferCount;
  pmd.minBufferSize = bufferSize;

  // Check to see if the worker requested specific port information
  overRidePortInfo( pmd );

  Port *port;
  try {
    port = new Port(*this, props, pmd, NULL);
  }
  catch( std::bad_alloc ) {
    throw OU::EmbeddedException( OC::NO_MORE_MEMORY, "new", OC::ContainerFatal);
  }

  m_context->ports[portId].current.maxLength = 
    (pmd.minBufferSize > pmd.maxBufferSize) ? pmd.minBufferSize : pmd.maxBufferSize;

  // Defer the real work until the port is connected.
  sourcePorts.insertToPosition(port, portId);

  return *port;
}




OC::Port &
Worker::
createInputPort( 
		OM::PortOrdinal    portId,   
                 OS::uint32_t   bufferCount,
                 OS::uint32_t   bufferSize, 
                 const OU::PValue*             props            
                 )
  throw ( OU::EmbeddedException )
{
  TRACE("OCPI::RCC::Container::createInputPort()");
  OU::AutoMutex guard (m_mutex); 
  if ( (bufferSize == 0) || (bufferCount == 0) ) {
    throw OU::EmbeddedException( OC::BAD_PORT_CONFIGURATION, "0 buffer count or size", OC::ApplicationRecoverable);
  }
  targetPortCount++;
  if ( targetPortCount > m_dispatch->numInputs  ) {
    throw OU::EmbeddedException( OC::PORT_NOT_FOUND, "Target Port count exceeds configuration", OC::ApplicationFatal);
  }
  if ( portId >= (m_dispatch->numInputs + m_dispatch->numOutputs) ) {
    throw OU::EmbeddedException( OC::PORT_NOT_FOUND, "Port id exceeds port count", OC::ApplicationFatal);
  }
  if ( portId > 32 ) {
    throw OU::EmbeddedException( OC::PORT_NOT_FOUND, "Port id exceeds max port count of 32", OC::ApplicationFatal);
  }

  // The caller can also be less specific and just specify the protocol 
  const char *protocol = NULL, *endpoint = NULL;
  if (OU::findString(props, "protocol", protocol))
    endpoint = m_transport.getEndpointFromProtocol( protocol).c_str();
  else if ((protocol = getenv("OCPI_DEFAULT_PROTOCOL"))) {
    printf("Forcing protocol = %s because OCPI_DEFAULT_PROTOCOL set in environment\n", protocol);
    endpoint = m_transport.getEndpointFromProtocol( protocol ).c_str();
  }
  else {
    // It is up to the caller to specify the endpoint which implicitly defines
    // the QOS.  If the caller does not provide the endpoint, we will pick one
    // by default.
    if (!OU::findString(props, "endpoint", endpoint))
      endpoint = m_transport.getDefaultEndPoint().c_str();
    endpoint = m_transport.addLocalEndpoint( endpoint)->sMemServices->endpoint()->end_point.c_str();
  }


  OCPI::Metadata::Port pmd;
  pmd.ordinal = portId;
  pmd.provider = true;
  pmd.minBufferCount = bufferCount;
  pmd.minBufferSize = bufferSize;

  // Check to see if the worker requested specific port information
  overRidePortInfo( pmd );

  Port *port;
  try {
    port = new Port(*this, props, pmd, endpoint);
  }
  catch( std::bad_alloc ) {
    throw OU::EmbeddedException( OC::NO_MORE_MEMORY, "new", OC::ContainerFatal);
  }

  // Add the port to the worker
  OpaquePortData * opaque =   
    static_cast<OpaquePortData*>(m_context->ports[portId].opaque);  
  port->opaque() = opaque;
  opaque->port = port;
  m_context->ports[portId].current.maxLength = 
    (pmd.minBufferSize > pmd.maxBufferSize) ? pmd.minBufferSize : pmd.maxBufferSize;
  targetPorts.insertToPosition(port, portId);

  return *port;
}


void 
Worker::
updatePort( Port &port )
{
  OM::PortOrdinal ord = port.portOrdinal();
  // Add the port to the worker
  OpaquePortData* opaque =
    static_cast<OpaquePortData*>(m_context->ports[ord].opaque);
  port.opaque() = opaque;
  opaque->port = &port;
  opaque->buffer = NULL;
  opaque->readyToAdvance=1;
  m_context->ports[ord].current.data = NULL;
  m_context->ports[ord].current.maxLength = 
    opaque->port->dtPort()->getPortSet()->getBufferLength();
  m_context->connectedPorts |= (1<<ord);
}

void 
Worker::
prepareProperty(OM::Property& md , OA::Property& cp)
{
  if (!md.isStruct && !md.members->type.isSequence && md.members->type.scalar != OP::Scalar::OCPI_String &&
      OP::Scalar::sizes[md.members->type.scalar] <= 32 &&
      !md.m_writeError) {

    if ( (md.m_offset+sizeof(md.members->type.scalar)) > m_dispatch->propertySize ) {
      throw OU::EmbeddedException( OC::PROPERTY_SET_EXCEPTION, NULL, OC::ApplicationRecoverable);
    }
    cp.m_writeVaddr = (uint8_t*)m_context->properties + md.m_offset;
  }
}

RCCPortMask Worker::getReadyPorts() {
  // Create the active mask
  ::RCCPortMask readyMask = 0;
  for (Port *ocpiport = firstChild(); ocpiport; ocpiport = ocpiport->nextChild()) {
    ::RCCPortMask myMask = 1 << ocpiport->portOrdinal();
    if (runConditionSS & myMask && // if port has an active run condition
	ocpiport->dtPort() ) {	   // if port is connected
      RCCPort *wport = &m_context->ports[ocpiport->portOrdinal()];
      OCPI::DataTransport::Port* port = ocpiport->dtPort();
      OpaquePortData *opd = static_cast<OpaquePortData*>(wport->opaque);

      // quick check that applies to input or output
      if (wport->current.data )
	readyMask |= myMask;
      else if (port->isOutput()) {
	if (port->hasEmptyOutputBuffer() ) {
	  // If this is an output port, set the mask if it has a free buffer
	  if ( opd->readyToAdvance ) {
	    opd->buffer = port->getNextEmptyOutputBuffer();
	    if ( opd->buffer ) {
	      opd->buffer->opaque = opd;
	      wport->current.data = (void*)opd->buffer->getBuffer();
	      wport->current.id_ = opd->buffer;
	      wport->output.length =         
		(OS::uint32_t)opd->buffer->getMetaData()->ocpiMetaDataWord.length;
	      wport->output.u.operation =         
		opd->buffer->getMetaData()->ocpiMetaDataWord.opCode;
	    }
	    else {
	      wport->current.data = NULL;
	    }
	    opd->readyToAdvance = false;
	  }
	  readyMask |= myMask;
	}
      }
      else if ( port->hasFullInputBuffer() ) {
	ocpiAssert( opd->readyToAdvance );
	if ( opd->readyToAdvance ) {
	  opd->buffer = port->getNextFullInputBuffer();
	  if ( opd->buffer ) {
	    wport->current.data = (void*)opd->buffer->getBuffer();
	    opd->buffer->opaque = opd;
	    wport->current.id_ = opd->buffer;
	    wport->input.length = opd->buffer->getMetaData()->ocpiMetaDataWord.length;
	    wport->input.u.operation = opd->buffer->getMetaData()->ocpiMetaDataWord.opCode;

	    ocpiAssert( wport->input.length <= wport->current.maxLength );

#ifndef NDEBUG
	    printf("max = %d, actual = %d\n", wport->current.maxLength, wport->input.length );
#endif
	    readyMask |= myMask;
	    opd->readyToAdvance = false;
	  }
	  else {
	    wport->current.data = NULL;
	  }
	}
      }
    } // End for each port
  }
  return readyMask;
}

extern volatile int ocpi_dbg_run;
void Worker::run(bool &anyone_run) {
  OU::AutoMutex guard (mutex(), true);
  
  if (!enabled)
    return;
  bool run_condition_met = false;
  RCCBoolean timeout = false;
  RCCPortMask readyMask = 0;

  // Break from this "do" when we know whether we are running or not
  do {
    // No run condition at all means run
    if (!m_context->runCondition) {
      run_condition_met = true;
      break;
    }
    // Check if this worker has a timer for a run condition
    if (m_dispatch->runCondition && m_dispatch->runCondition->timeout ) {
      OS::Timer::ElapsedTime et;
      runTimer.stop();
      runTimer.getValue( et );
      runTimer.start();
      if ( et > runTimeout ) {          
#ifndef NDEBUG
	printf("WORKER TIMED OUT, timer time = %d,%d -- run timer = %d,%d\n", 
	       et.seconds, et.nanoseconds, runTimeout.seconds, runTimeout.nanoseconds );
#endif
	run_condition_met = true;
	timeout = true;
	break;
      }
    }
    // If no port masks, then we won't run
    if (!m_context->runCondition->portMasks ||
	!m_context->runCondition->portMasks[0])
      break;
    // Ok, do the work to find out which ports are ready
    readyMask = getReadyPorts();
#ifndef NDEBUG
    if ( ocpi_dbg_run ) {
      printf("WORKER RUN: worker ready mask = %d\n", readyMask );
    }
#endif
    // We need to ignore optional ports that are NOT connected,
    // So we make believe they are ready
    readyMask |=
      m_dispatch->optionallyConnectedPorts &
      ~m_context->connectedPorts;
    if (!readyMask)
      break;
    // See if any of our masks are satisfied
    for (::RCCPortMask *pmp = m_context->runCondition->portMasks;
	 *pmp; pmp++)
      if ((*pmp & readyMask) == *pmp) {
	run_condition_met = true;
	break;
      }

  } while (0);
  assert(enabled);
  if (run_condition_met) {

    // If the worker has defined a port method, we will call it instead of the run method
    // for each port that has one [FIXME should these just run if the port is ready in any case?]
    OM::PortOrdinal pord = 0;
    bool execute_run = true;
    for (uint32_t mtest = 1; mtest < readyMask; mtest <<= 1, pord++) {
      if (mtest & readyMask &&
	  m_context->ports[pord].callBack ) {
	execute_run = false;
	if ( m_context->ports[pord].callBack(m_context,
					     &m_context->ports[pord], 
					     RCC_OK) != RCC_OK) {
	  enabled = false;
	  // FIXME: do anything with timers?  release?
	  setControlState(OM::Worker::UNUSABLE);
	}
      }
    }

    assert(enabled);
    if (m_dispatch->run && execute_run ) {
      anyone_run = true;
      OCPI_EMIT_("End Worker Evaluation");
      RCCBoolean newRunCondition = false;
      // FIXME: implement new runcondition!!!
      switch (m_dispatch->run(m_context, timeout, &newRunCondition)) {
      case RCC_ADVANCE:
	advanceAll();
	break;
      case RCC_DONE:
	// FIXME:  release all current buffers
	enabled = false;
	break;
      case RCC_OK:
	assert(enabled);
	runTimer.stop();
	runTimer.reset();
	runTimer.start();
	break;
      default:
	enabled = false;
	setControlState(OM::Worker::UNUSABLE);
	runTimer.stop();
      }
      checkDeadLock();
      worker_run_count++;
    }
  }
}
     void Worker::checkDeadLock() {}
     void Worker::advanceAll() {
       OCPI_EMIT_( "Start Advance All" );
       int numPorts = m_dispatch->numInputs + m_dispatch->numOutputs;
       for (int n=0; n<numPorts; n++ ) {
	 RCCPort *wport = &(m_context->ports[n]);
	 OpaquePortData *opd = static_cast<OpaquePortData*>(wport->opaque); 
	 if ( wport->current.data == NULL ) {
	   continue;
	 }
	 opd->readyToAdvance = true;    
	 if ( opd->buffer ) {
	   opd->port->dtPort()->advance( opd->buffer, opd->cp289Port->output.length );
	   wport->current.data = NULL;
	 }
       }
       OCPI_EMIT_( "End Advance All" );
     }

// Note we are already under a mutex here
void Worker::controlOperation(OM::Worker::ControlOperation op) {
  RCCResult rc = RCC_OK;
  OU::AutoMutex guard (mutex(), true);
  switch (op) {
  case OM::Worker::OpInitialize:
    if (m_dispatch->initialize)
      rc = m_dispatch->initialize(m_context);
    break;
  case OM::Worker::OpStart:
    // If a worker gets started before all of its ports are created: error
    if ( (int)(targetPortCount + sourcePortCount) != 
	 (int)(m_dispatch->numInputs + m_dispatch->numOutputs ) )
      throw OU::EmbeddedException( OC::PORT_COUNT_MISMATCH,
				   "Port count different than metadata",
				   OC::ApplicationRecoverable);
    // If the worker does not have all the required ports connected: error
    for (Port *port = firstChild(); port; port = port->nextChild())
      if (!(1<<port->portOrdinal() & m_dispatch->optionallyConnectedPorts)) {
	OCPI::DataTransport::Circuit* c = port->circuit();
	if ( !c || c->isCircuitOpen() )
	  throw OU::EmbeddedException( OC::PORT_NOT_CONNECTED, NULL, OC::ApplicationRecoverable);
      }
    if (!m_dispatch->start ||
	(rc = m_dispatch->start(m_context)) == RCC_OK) {
      enabled = true;
      runTimer.start();// FIXME: this this right for re-start too?
    }
    break;
  case OM::Worker::OpStop:
    if (enabled) {
      enabled = false;
      runTimer.stop();
      runTimer.reset();
    }
    if (m_dispatch->stop)
      rc = m_dispatch->stop(m_context);
    break;
    // like stop, except don't call stop
  case OM::Worker::OpRelease:
    if (enabled) {
      enabled = false;
      runTimer.stop();
      runTimer.reset();
    }
    if (m_dispatch->release)
      rc = m_dispatch->release(m_context);
    break;
  case OM::Worker::OpTest:
    if (m_dispatch->test)
      rc = m_dispatch->test(m_context);
    else
      throw OU::EmbeddedException( OC::TEST_NOT_IMPLEMENTED,
				   "Worker has no test implementation",
				   OC::ApplicationRecoverable);
    break;
  case OM::Worker::OpBeforeQuery:
    if (m_dispatch->beforeQuery)
      rc = m_dispatch->beforeQuery(m_context);
    break;
  case OM::Worker::OpAfterConfigure:
    if (m_dispatch->afterConfigure)
      rc = m_dispatch->afterConfigure(m_context);
  case OM::Worker::OpsLimit:
    break;
  }
  switch (rc) {
  case RCC_OK:
    break;
  case RCC_ERROR:
    throw OU::EmbeddedException( OC::WORKER_ERROR, m_context->errorString,
				 OC::ApplicationRecoverable);
    break;
  case RCC_FATAL:
    enabled = false;
    setControlState(OM::Worker::UNUSABLE);
    throw OU::EmbeddedException( OC::WORKER_FATAL, m_context->errorString,
				 OC::ApplicationFatal);
    break;
  default:
    enabled = false;
    throw OU::EmbeddedException( OC::WORKER_API_ERROR, "Control operation returned invalid RCCResult",
				 OC::ApplicationFatal);
  }    
}

}
}
