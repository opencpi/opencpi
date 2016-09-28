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

#include <climits>
#include "OcpiTimeEmitCategories.h"
#include "RccApplication.h"
#include "RccPort.h"
#include "RccWorker.h"

namespace OC = OCPI::Container;
namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OA = OCPI::API;

namespace OCPI {
  namespace RCC {

 Worker::
 Worker(Application & app, Artifact *art, const char *name,
	ezxml_t impl, ezxml_t inst, OC::Worker *slave, bool hasMaster, const OU::PValue *wParams)
   : OC::WorkerBase<Application,Worker,Port>(app, *this, art, name, impl, inst, slave, hasMaster,
					     wParams),
   OCPI::Time::Emit( &parent().parent(), "Worker", name), 
   m_entry(art ? art->getDispatch(ezxml_cattr(impl, "name")) : NULL), m_user(NULL),
   m_dispatch(NULL), m_portInit(0), m_context(NULL), m_mutex(app.container()),
   m_runCondition(NULL), m_errorString(NULL), enabled(false), hasRun(false),
   sourcePortCount(0), targetPortCount(0), m_nPorts(nPorts()), worker_run_count(0),
   m_transport(app.parent().getTransport())
 {
   memset(&m_info, 0, sizeof(m_info));
   if (art)
     if (!strcasecmp(m_entry->type, "c"))
       m_dispatch = m_entry->dispatch;
     else
       // Get the info, constructing the worker.
       ((RCCConstruct *)m_entry->dispatch)(NULL, m_info);
   else {
     // Note the "hack" to use "name" as dispatch when artifact is not present
     m_dispatch = (RCCDispatch *)name;
     uint32_t mask = 0;
     // For these test workers, initialize the control mask from the dispatch table
#define CONTROL_OP(x, c, t, s1, s2, s3, s4) \
     if (m_dispatch->x)                     \
	  mask |= 1 << OU::Worker::Op##c;
     OCPI_CONTROL_OPS
#undef CONTROL_OP
     setControlMask(mask);
   }
   if (m_dispatch) {
     m_info.memSize = m_dispatch->memSize;
     m_info.memSizes = m_dispatch->memSizes;
     m_info.portInfo = m_dispatch->portInfo;
     m_info.propertySize = m_dispatch->propertySize;
     m_info.optionallyConnectedPorts = m_dispatch->optionallyConnectedPorts;
   }
   initializeContext();
#if 0
   // If we have an event handler, we need to inform it about the timeout
   if (m_runCondition->m_timeout ) {
     runTimeout.set(m_runCondition->m_usecs / 1000000,
		    (m_runCondition->m_usecs % 1000000) * 1000);
     if ( m_transport.m_transportGlobal->getEventManager() ) {

 #ifdef EM_PORT_COMPLETE
       parent().myparent->m_transport->m_transportGlobal->getEventManager()->setMinTimeout( workerId, 
									 m_runCondition->m_usecs );
 #endif

     }
   }
#endif
 }

 void 
 Worker::
 read(size_t offset, 
      size_t nBytes, 
      void * p_data  )
 {
   OU::AutoMutex guard(m_mutex);
   if (!m_context->properties || (offset+nBytes) > m_info.propertySize)
     throw OU::EmbeddedException( OU::PROPERTY_GET_EXCEPTION, NULL, OU::ApplicationRecoverable);
   memcpy( p_data, (char*)m_context->properties+offset, nBytes );
 }

 void 
 Worker::
 write(size_t offset, 
       size_t nBytes, 
       const void * p_data  )
 {
   OU::AutoMutex guard (m_mutex);
   if (!m_context->properties || (offset+nBytes) > m_info.propertySize)
     throw OU::EmbeddedException( OU::PROPERTY_SET_EXCEPTION, NULL, OU::ApplicationRecoverable);
   memcpy( (char*)m_context->properties+offset, p_data, nBytes );
 }

 RCCResult Worker::
 setError(const char *fmt, va_list ap) {
   if (m_errorString) {
     free(m_errorString);
     m_errorString = NULL;
   }
   vasprintf(&m_errorString, fmt, ap);
   return RCC_ERROR;
 }

 OC::Worker &Worker::
 getSlave() {
   if (!slave())
     throw OU::Error("No slave has been set for this worker");
   return *slave();
 }

Worker::
~Worker()
{
  // FIXME - this sort of thing should be generic and be reused in portError
  try {
    if (enabled) {
      enabled = false;
      controlOperation(OU::Worker::OpStop);
    }
    controlOperation(OU::Worker::OpRelease);
  } catch(...) {
  }
#ifdef EM_PORT_COMPLETE
  // If we have an event handler, we need to inform it about the timeout
  if ( m_runCondition && m_runCondition->m_timeout ) {
    if ( parent().m_transport->m_transportGlobal->getEventManager() ) {
      parent().m_transport->m_transportGlobal->getEventManager()->removeMinTimeout( w->workerId );
    }
  }
#endif
  delete m_user;
  deleteChildren();
  uint32_t m = 0;
  while ( m_context->memories && m_context->memories[m] ) {
    delete [] (char*)m_context->memories[m];
    m++;
  }
  delete[] m_context->memories;
  delete[] (char*)m_context->memory;
  if (m_dispatch && m_context->properties)
    delete[] (char*)m_context->properties;
  delete[] m_context;
  if (m_errorString)
    free(m_errorString);
  while (!m_testPmds.empty()) {
    OU::Port *pmd = m_testPmds.front();
    m_testPmds.pop_front();
    delete pmd;
  }
}

 static RCCResult
   rccSetError(const char *fmt, ...);
 static void
   rccRelease(RCCBuffer *),
   cSend(RCCPort *, RCCBuffer*, RCCOpCode op, size_t length),
   rccSend(RCCPort *, RCCBuffer*),
   rccTake(RCCPort *,RCCBuffer *old_buffer, RCCBuffer *new_buffer);
 static RCCBoolean
   rccRequest(RCCPort *port, size_t maxlength),
   cAdvance(RCCPort *port, size_t maxlength),
   rccAdvance(RCCPort *port, size_t maxlength),
   rccWait(RCCPort *, size_t max, unsigned usecs);

 static RCCResult
 rccSetError(const char *fmt, ...)
 {
   va_list ap;
   va_start(ap, fmt);
   RCCResult rc = ((Worker *)pthread_getspecific(Driver::s_threadKey))->setError(fmt, ap);
   va_end(ap);
   return rc;
 }

 static void
 rccRelease(RCCBuffer* buffer)
 {
   OCPI::DataTransport::BufferUserFacet* dti_buffer = buffer->containerBuffer;
   ocpiAssert(dti_buffer);
   Port * port = static_cast<Port*>( dti_buffer->m_ud );
   ocpiAssert(port);
   // An API might be called incorrectly.
   if (port->isOutput())
     throw OU::Error("RCC release container function called on an output port, which is not supported");
   port->release(dti_buffer);    
 }
 // C language only
 static void
 cSend(RCCPort* rccPort, RCCBuffer* rccBuffer, RCCOpCode op, size_t len)
 {
   rccBuffer->length_ = len;
   rccBuffer->opCode_ = op;
   rccSend(rccPort, rccBuffer);
 }
 static void
 rccSend(RCCPort* rccPort, RCCBuffer* rccBuffer)
 {
   Port* port = rccPort->containerPort;
   ocpiAssert(port);
   if (!port->isOutput() )
     throw OU::Error("The 'send' container function cannot be called on an input port");
   OCPI::DataTransport::BufferUserFacet *buffer = rccBuffer->containerBuffer;
   Port *bufferPort = static_cast<Port*>(buffer->m_ud);
   if (bufferPort != port && bufferPort->isOutput())
     throw OU::Error("Cannot send a buffer from a different output port");
   port->send(buffer, rccBuffer->length_, rccBuffer->opCode_);
 }
 // C language only
 static RCCBoolean
 cAdvance(RCCPort* rccPort, size_t max)
 {
   // This is only useful for output buffers
   rccPort->current.length_ = rccPort->output.length;
   rccPort->current.opCode_ = rccPort->output.u.operation;
   return rccAdvance(rccPort, max);
 }
 static RCCBoolean
 rccAdvance(RCCPort* rccPort, size_t max)
 {
   Port *port = rccPort->containerPort; 
   ocpiAssert(port);
   bool ready = port->advance();
   if (ready && max && max > rccPort->current.maxLength)
     throw OU::Error("Output buffer request/advance (size %zu) greater than buffer size (%zu)",
		     max, rccPort->current.maxLength);
   return ready;
 }

 static RCCBoolean
 rccRequest(RCCPort* rccPort, size_t max )
 {
   if (rccPort->current.data )
     return true;
   Port* port = rccPort->containerPort;
   ocpiAssert(port);
   bool ready = port->request();
   if (ready && max && port->isOutput() && max < rccPort->output.length)
     throw OU::Error("Requested output buffer size is unavailable");
   return ready;
 }

 static RCCBoolean
 rccWait(RCCPort* port, size_t max, unsigned usec )
 {
   // Not implemented yet
   ( void ) port;
   ( void ) max;
   ( void ) usec;
   return false;
 }
 static void rccTake(RCCPort *rccPort, RCCBuffer *oldBuffer, RCCBuffer *newBuffer)
 {
   Port *port = rccPort->containerPort;
   if ( port->isOutput() )
     throw OU::Error("The 'take' container function cannot be used on an output port");
   if (!rccPort->current.data)
     throw OU::Error("The 'take' container function cannot be called when there is no current buffer");
   port->take(oldBuffer, newBuffer);
 }

 // FIXME:  recover memory on exceptions
 void Worker::
 initializeContext()
 {
   RCCDispatch *wd = m_dispatch;

   // check masks for bad bits
   OU::Port *ports = getPorts();
   if (m_nPorts) {
     if (wd && m_nPorts != wd->numInputs + wd->numOutputs)
       throw OU::Error("metadata port count (%u) and dispatch port count (in: %u + out: %u) differ",
		       m_nPorts, wd->numInputs, wd->numOutputs);
     RCCPortMask optional = 0;
     for (unsigned n = 0; n < m_nPorts; n++)
       if (ports[n].m_optional)
	 optional |= 1 << n;
     if (m_dispatch) {
       if (m_info.optionallyConnectedPorts != optional)
	 throw OU::Error("metadata optional ports (%x) and dispatch optional ports (%x) differ",
			 optional, m_info.optionallyConnectedPorts);
     } else
       m_info.optionallyConnectedPorts = optional;
   } else {
     if (wd && (wd->numInputs || wd->numOutputs))
       m_nPorts = wd->numInputs + wd->numOutputs;
   }

   RCCPortMask ourMask = ~(-1 << m_nPorts);
   if (~ourMask & m_info.optionallyConnectedPorts)
     throw OU::EmbeddedException( OU::PORT_COUNT_MISMATCH,
				  "optional port mask is invalid",
				  OU::ApplicationRecoverable);
   // Note here that the default constructor for m_defaultRunCondition makes it the default
   // run condition
   if (wd && wd->runCondition) {
     for (RCCPortMask *pm = wd->runCondition->portMasks; pm && *pm; pm++)
       if (~ourMask & *pm)
	 throw OU::EmbeddedException(OU::PORT_COUNT_MISMATCH, "run condition mask is invalid",
				     OU::ApplicationRecoverable);
     // Found a C-language run condition
     m_cRunCondition.setRunCondition(*wd->runCondition);
     setRunCondition(m_cRunCondition);
   } else // Use our default rundition
     setRunCondition(m_defaultRunCondition);

   // Now after error checking we start to allocate resources
   // Create our context
   size_t n = sizeof(RCCWorker) + m_nPorts * sizeof(RCCPort);
   m_context = (RCCWorker *)new char[n];
   memset(m_context, 0, n);
   static RCCContainer rccContainer =
     { rccRelease, cSend, rccRequest, cAdvance, rccWait, rccTake, rccSetError};
   m_context->container = rccContainer;
   m_context->runCondition = wd ? wd->runCondition : NULL;

   // We initialize one of these structures for all of the ports that are defined in the
   // worker. However, the actual data ports may be optional at runtime and not be created.
   for (unsigned n=0;  n< m_nPorts; n++) {
     m_context->ports[n].containerPort = NULL;
     m_context->ports[n].current.data = NULL;
     m_context->ports[n].current.maxLength = 0;
     m_context->ports[n].callBack = 0;
     m_context->ports[n].userPort = NULL;
     m_context->ports[n].sequence = NULL;
   }

   // Create our memory spaces
   unsigned mcount;
   for (mcount = 0; m_info.memSizes && m_info.memSizes[mcount]; mcount++)
     ocpiDebug("Allocating %zu bytes of data for worker memory %u\n",
	       m_info.memSizes[mcount], mcount);
   if (m_info.memSize)
     ocpiDebug("Allocating %zu bytes of data for worker memory\n", m_info.memSize);
   if (m_info.memSizes || m_info.memSize) {
     try {
       if (m_info.memSizes) {
	 m_context->memories = new void*[mcount+1];
	 memset(m_context->memories, 0, sizeof(void*)*(mcount+1));
	 for (unsigned n = 0; n < mcount; n++) {
	   m_context->memories[n] = new char[m_info.memSizes[n]];
	   memset(m_context->memories[n], 0, m_info.memSizes[n]);
	 }
       }
       if (m_info.memSize) {
	 m_context->memory = new char[m_info.memSize];
	 memset(m_context->memory, 0, m_info.memSize);
       }
     } catch( std::bad_alloc ) {
       throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "worker requested too much memory",
				    OU::ApplicationRecoverable);
     }
   }  
   // Now create and initialize the worker properties
   if (m_dispatch) {
     if (m_info.propertySize)
       m_context->properties = new char[m_info.propertySize + 4];
   } else
     try {
       pthread_setspecific(Driver::s_threadKey, this);
       m_user = ((RCCConstruct *)m_entry->dispatch)(m_entry, m_info);
       m_context->properties = m_user->rawProperties(m_info.propertySize);
     } catch (std::string &e) {
       throw OU::Error("Worker C++ constructor failed with an unknown exception: %s",
		       e.c_str());
     } catch (...) {
       throw OU::Error("Worker C++ constructor failed with an unknown exception");
     }
   if (m_context->properties)
     memset(m_context->properties, 0, sizeof(char)*m_info.propertySize);
 }

 // Called from the generic getPort when the port is not found.
 // Also called from createInputPort and createOutputPort locally
 OC::Port & 
 Worker::
 createPort(const OU::Port& mp, const OCPI::Util::PValue *params)
 {
   TRACE(" OCPI::RCC::Worker::createPort()");
   if ( mp.m_minBufferCount == 0)
     throw OU::EmbeddedException( OU::BAD_PORT_CONFIGURATION, "0 buffer count",
				  OU::ApplicationRecoverable);
   OU::AutoMutex guard (m_mutex);
   if (m_dispatch && mp.m_ordinal >= m_dispatch->numInputs + m_dispatch->numOutputs)
     throw OU::EmbeddedException(OU::PORT_NOT_FOUND,
				 "Port id exceeds port count", OU::ApplicationFatal);
   if (mp.m_ordinal > 32)
     throw OU::EmbeddedException(OU::PORT_NOT_FOUND,
				 "Port id exceeds max port count of 32", OU::ApplicationFatal);
   // If the worker binary has port info, check it against the metadata for consistency
   if (m_info.portInfo)
     for (RCCPortInfo* pi = m_info.portInfo; pi->port != RCC_NO_ORDINAL; pi++)
       if (pi->port == mp.m_ordinal) {
	 ocpiDebug("Worker PortInfo for port %d, bc,s: %d,%zu, metadata is %zu,%zu",
		   mp.m_ordinal, pi->minBuffers, pi->maxLength, mp.m_minBufferCount,
		   mp.m_minBufferSize);
	 if (pi->minBuffers > mp.m_minBufferCount) // bad: worker needs more than metadata   
	   throw OU::EmbeddedException(OU::PORT_NOT_FOUND,
				       "Worker metadata inconsistent with RCC PortInfo",
				       OU::ContainerFatal);
       }
   if (mp.m_provider) { // input
     if (m_dispatch && ++targetPortCount > m_dispatch->numInputs)
       throw OU::EmbeddedException(OU::PORT_NOT_FOUND,
				   "Target Port count exceeds configuration", OU::ApplicationFatal);
   } else if (m_dispatch && ++sourcePortCount > m_dispatch->numOutputs )
     throw OU::EmbeddedException(OU::PORT_NOT_FOUND,
				 "Source Port count exceeds configuration", OU::ApplicationFatal);
   Port *port;
   try {
     port = new Port(*this, mp, params, m_context->ports[mp.m_ordinal]);
   }
   catch(std::bad_alloc) {
     throw OU::EmbeddedException( OU::NO_MORE_MEMORY, "new", OU::ContainerFatal);
   }
   // We know the metadata is more contrained than port info
   // FIXME: RccPort Object should know about its C RCCPort and do this itself
   // FIXME: this can change on connections
   m_context->ports[mp.m_ordinal].current.maxLength = port->getData().data.desc.dataBufferSize;
   m_context->ports[mp.m_ordinal].containerPort = port;
   return *port;
 }


 // Common code for the test API to create ports by ordinal
 // We add the explicitly specified buffer count and size to the
 // list of properties.
 static  OC::Port &
 createTestPort( Worker *w, OU::PortOrdinal portId,
		 size_t bufferCount,
		 size_t bufferSize, 
		 bool isProvider,
		 const OU::PValue* props) {
   OU::Port *pmd = new OU::Port;;
   pmd->m_name = isProvider ? "unnamed input" : "unnamed output";
   pmd->m_ordinal = portId;
   pmd->m_provider = isProvider;
   pmd->m_bufferSize = bufferSize;
   pmd->m_minBufferCount = bufferCount;
   w->m_testPmds.push_back(pmd);
   return w->createPort(*pmd, props);
 }
  OC::Port &
 Worker::
 createOutputPort( 
		  OU::PortOrdinal     portId,
		  size_t    bufferCount,
		  size_t    bufferSize, 
		  const OU::PValue*              props            
		  )
   throw ( OU::EmbeddedException )
 {
   TRACE(" OCPI::RCC::Container::createOutputPort()");

   return createTestPort(this, portId, bufferCount, bufferSize, false, props);
 }




 OC::Port &
 Worker::
 createInputPort( 
		 OU::PortOrdinal    portId,   
		  size_t   bufferCount,
		  size_t   bufferSize, 
		  const OU::PValue*             props            
		  )
   throw ( OU::EmbeddedException )
 {
   TRACE("OCPI::RCC::Container::createInputPort()");

   return createTestPort(this, portId, bufferCount, bufferSize, true, props);
 }


 void 
 Worker::
 portIsConnected( unsigned ordinal )
 {
   m_context->connectedPorts |= (1<<ordinal);
 }

 void
 Worker::
 portError(std::string &error) {
   enabled = false;
   controlOperation(OU::Worker::OpStop);
   controlOperation(OU::Worker::OpRelease);
   setControlState(OU::Worker::UNUSABLE);
   ocpiBad("Worker %s received port error: %s", name().c_str(), error.c_str());
 }

void Worker::
propertyWritten(unsigned ordinal) const {
  if (m_user) {
    m_user->propertyWritten(ordinal);
    checkError();
  }
}
void Worker::
propertyRead(unsigned ordinal) const {
  if (m_user) {
    m_user->propertyRead(ordinal);
    checkError();
  }
}

void Worker::
prepareProperty(OU::Property& md , 
		volatile void *&writeVaddr,
		const volatile void *&readVaddr) {
  (void)readVaddr;
  if (md.m_baseType != OA::OCPI_Struct && !md.m_isSequence && md.m_baseType != OA::OCPI_String &&
      OU::baseTypeSizes[md.m_baseType] <= 32 &&
      !md.m_writeError) {
    if (!m_context->properties ||
	(md.m_offset+OU::baseTypeSizes[md.m_baseType]/8) > m_info.propertySize ) {
      throw OU::EmbeddedException( OU::PROPERTY_SET_EXCEPTION, NULL, OU::ApplicationRecoverable);
    }
    writeVaddr = (uint8_t*)m_context->properties + md.m_offset;
  }
}

void Worker::
checkError() const {
  char *err = m_context->errorString ? m_context->errorString : m_errorString;
  if (err) {
    std::string e;
    OU::format(e, "Worker %s produced error during execution: %s", name().c_str(), err);
    m_context->errorString = NULL;
    if (m_errorString) {
      free(m_errorString);
      m_errorString = NULL;
    }
    throw OU::Error("%s", e.c_str());
  }
}

void Worker::
run(bool &anyone_run) {
  checkControl();
  if (!enabled)
    return;
  OU::AutoMutex guard (mutex(), true);
  if (!enabled)
    return;
  // Before run condition processing happens, perform callbacks, and, if we did any,
  // skip runcondition processing
  // FIXME: have a bit mask of these
  bool didCallBack = false;
  RCCPort *rccPort = m_context->ports;
  for (unsigned n = 0; n < m_nPorts; n++, rccPort++)
    if (rccPort->callBack && m_context->connectedPorts & (1 << n) &&
	rccPort->containerPort->checkReady())
      if (rccPort->callBack(m_context, rccPort, RCC_OK) == RCC_OK)
	didCallBack = true;
      else {
	enabled = false;
	setControlState(OU::Worker::UNUSABLE);
	return;
      }
  if (didCallBack)
    return;
  // OCPI_EMIT_REGISTER_FULL_VAR( "Worker Evaluation", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, were ); 
  // OCPI_EMIT_STATE_CAT_NR_(were, 1, OCPI_EMIT_CAT_TUNING, OCPI_EMIT_CAT_TUNING_WC);

  // Run condition processing: we break if we're going to run, and return if not
  ocpiAssert(m_runCondition);
  bool timedOut = false;
  do {
    if (!m_runCondition->m_portMasks) // no port mask array means run all the time
      break;
    if (m_runCondition->m_timeout && m_runTimer.expired()) {
      ocpiInfo("WORKER TIMED OUT, elapsed time = %u,%u", 
	       m_runTimer.getElapsed().seconds(), m_runTimer.getElapsed().nanoseconds());
      timedOut = true;
      break;
    }
    // If no port masks, then we don't run except for timeouts, checked above
    if (m_runCondition->m_portMasks[0] == RCC_NO_PORTS)
      if (m_runCondition->m_timeout && !hasRun) {
	hasRun = true;
	break; // run if we're in period execution and haven't run at all yet
      } else
	return;
    // Start out assuming optional unconnected ports are "ready"
    RCCPortMask readyMask = m_info.optionallyConnectedPorts & ~m_context->connectedPorts;
    // Only examine connected ports that are in the run condition
    RCCPortMask relevantMask = m_context->connectedPorts & m_runCondition->m_allMasks;
    RCCPort *rccPort = m_context->ports;
    RCCPortMask portBit = 1;
    for (unsigned n = m_nPorts; n; n--, rccPort++, portBit <<= 1)
      if ((portBit & relevantMask) && rccPort->containerPort->checkReady())
	readyMask |= portBit;
    if (!readyMask)
      return;
    // See if any of our masks are satisfied
    RCCPortMask *pmp, pm = 0;
    for (pmp = m_runCondition->m_portMasks; (pm = *pmp); pmp++)
      if ((pm & readyMask) == (pm & ~(RCC_ALL_PORTS << m_nPorts)))
	break;
    if (!pm)
      return;
  } while (0);
  assert(enabled);
  if (!m_dispatch || m_dispatch->run) {
    anyone_run = true;
    //      OCPI_EMIT_STATE_CAT_NR_(were, 0, OCPI_EMIT_CAT_TUNING, OCPI_EMIT_CAT_TUNING_WC);
    RCCBoolean newRunCondition = false;

    pthread_setspecific(Driver::s_threadKey, this);
    if (m_runCondition->m_timeout)
      m_runTimer.restart();
    OCPI_EMIT_REGISTER_FULL_VAR( "Worker Run", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, wre ); \
    OCPI_EMIT_STATE_CAT_NR_(wre, 1, OCPI_EMIT_CAT_WORKER_DEV, OCPI_EMIT_CAT_WORKER_DEV_RUN_TIME);
    RCCResult rc = m_dispatch ?
      m_dispatch->run(m_context, timedOut, &newRunCondition) : m_user->run(timedOut);
    OCPI_EMIT_STATE_CAT_NR_(wre, 0, OCPI_EMIT_CAT_WORKER_DEV, OCPI_EMIT_CAT_WORKER_DEV_RUN_TIME);
    m_context->firstRun = false;
    if (m_user)
      m_user->m_first = false;
    checkError();
    if (newRunCondition) {
      if (m_runCondition->m_timeout)
	m_runTimer.reset();
      if (m_context->runCondition) {
	m_cRunCondition.setRunCondition(*m_context->runCondition);
	setRunCondition(m_cRunCondition);
      } else
	setRunCondition(m_defaultRunCondition);
      // FIXME: cache this silly calcation using OS::Time
      if (m_runCondition->m_timeout)
	m_runTimer.restart(m_runCondition->m_usecs / 1000000,
			   (m_runCondition->m_usecs % 1000000) * 1000);
    }
    // The state might have changed behind our back: e.g. in port exceptions
    if (getState() != OU::Worker::UNUSABLE)
      switch (rc) {
      case RCC_ADVANCE:
	advanceAll();
	break;
      case RCC_ADVANCE_DONE:
	advanceAll();
      case RCC_DONE:
	// FIXME:  release all current buffers
	enabled = false;
	setControlState(OU::Worker::FINISHED);
	break;
      case RCC_OK:
	break;
      default:
	enabled = false;
	setControlState(OU::Worker::UNUSABLE);
	m_runTimer.reset();
      }
    worker_run_count++;
  }
}

void Worker::
advanceAll() {
  OCPI_EMIT_REGISTER_FULL_VAR( "Advance All", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, aare ); 
  OCPI_EMIT_STATE_CAT_NR_(aare, 1, OCPI_EMIT_CAT_TUNING, OCPI_EMIT_CAT_TUNING_WC);
  RCCPort *rccPort = m_context->ports;
  for (unsigned n = 0; n < m_nPorts; n++, rccPort++)
    if (rccPort->current.data)
      if (m_dispatch)
	cAdvance(rccPort, 0);
      else
	rccPort->userPort->advance();
  OCPI_EMIT_STATE_CAT_NR_(aare, 0, OCPI_EMIT_CAT_TUNING, OCPI_EMIT_CAT_TUNING_WC);
}

// Note we are already under a mutex here
void Worker::
controlOperation(OU::Worker::ControlOperation op) {
  RCCResult rc = RCC_OK;
  OU::AutoMutex guard (mutex(), true);
  pthread_setspecific(Driver::s_threadKey, this);
  enum {
#define CONTROL_OP(x, c, t, s1, s2, s3, s4) My_##x = 1 << OU::Worker::Op##c,
OCPI_CONTROL_OPS
#undef CONTROL_OP
  };
#define DISPATCH(op) \
  (!(getControlMask() & My_##op) ? RCC_OK : \
   (m_dispatch ? (m_dispatch->op ? m_dispatch->op(m_context) : RCC_OK) : m_user->op()))
  switch (op) {
  case OU::Worker::OpInitialize:
    rc = DISPATCH(initialize);
    break;
  case OU::Worker::OpStart:
    {
      // If a worker gets started before all of its required ports are created: error
      RCCPortMask mandatory = ~(-1 << m_nPorts) & ~m_info.optionallyConnectedPorts;
      // FIXME - this should be in generic code, not RCC
      if ((mandatory & m_context->connectedPorts) != mandatory) {
	const char *inst = instTag().c_str();
	throw OU::Error("A port of%s%s%s worker '%s' is not connected",
			inst[0] ? " instance '" : "", inst, inst[0] ? "'" : "",
			implTag().c_str());
      }
    }
    if ((rc = DISPATCH(start)) == RCC_OK) {
      enabled = true;
      hasRun = false; // allow immediate execution after suspension for period execution
    }
    break;
  case OU::Worker::OpStop:
    // If the worker says that stop failed, we're not stopped.
    if ((rc = DISPATCH(stop)) != RCC_OK)
      break;
    if (enabled) {
      enabled = false;
      m_runTimer.reset(); // just in case there is overhead with running the timer
    }
    setControlState(OU::Worker::SUSPENDED);
    break;
    // like stop, except don't call stop
  case OU::Worker::OpRelease:
    if (enabled) {
      enabled = false;
      m_runTimer.reset();
    }
    rc = DISPATCH(release);
    setControlState(OU::Worker::UNUSABLE);
    break;
  case OU::Worker::OpTest:
    if (m_dispatch && !m_dispatch->test)
      throw OU::EmbeddedException( OU::TEST_NOT_IMPLEMENTED,
				   "Worker has no test implementation",
				   OU::ApplicationRecoverable);
    rc = DISPATCH(test);
    break;
  case OU::Worker::OpBeforeQuery:
    rc = DISPATCH(beforeQuery);
    break;
  case OU::Worker::OpAfterConfigure:
    rc = DISPATCH(afterConfigure);
    break;
  case OU::Worker::OpsLimit:
    break;
  }
  const char *err = m_context->errorString ? m_context->errorString : m_errorString;
  std::string serr;
  if (err) {
    OU::format(serr, "Worker '%s' produced error during the '%s' control operation: %s",
	       name().c_str(), OU::Worker::s_controlOpNames[op], err);
    m_context->errorString = NULL;
    if (m_errorString) {
      free(m_errorString);
      m_errorString = NULL;
    }
    err = serr.c_str();
  }
  switch (rc) {
  case RCC_OK:
    break;
  case RCC_ERROR:
    throw OU::EmbeddedException( OU::WORKER_ERROR, err,
				 OU::ApplicationRecoverable);
    break;
  case RCC_FATAL:
    enabled = false;
    setControlState(OU::Worker::UNUSABLE);
    throw OU::EmbeddedException( OU::WORKER_FATAL, err,
				 OU::ApplicationFatal);
    break;
  default:
    enabled = false;
    throw
      OU::Error("Control operation \"%s\" on RCC worker \"%s\" returned invalid "
		"RCCResult value: 0x%x", OU::Worker::s_controlOpNames[op],
		name().c_str(), rc);
  }    
}

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors. 
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
    void Worker::set##pretty##Property(unsigned ordinal, const run val, unsigned idx) const { \
      OA::PropertyInfo &info = properties()[ordinal];	       \
    assert(info.m_baseType == OCPI::API::OCPI_##pretty);       \
    if (info.m_writeError)                                     \
      throw; /*"worker has errors before write */              \
    store *pp = (store *)(getPropertyVaddr() + info.m_offset + \
			  info.m_elementBytes * idx);          \
    if (bits > 32) {                                           \
      assert(bits == 64);                                      \
      uint32_t *p32 = (uint32_t *)pp;                          \
      p32[1] = ((const uint32_t *)&val)[1];                    \
      p32[0] = ((const uint32_t *)&val)[0];                    \
    } else						       \
      *pp = *(const store *)&val;			       \
    if (info.m_writeError)				       \
      throw; /*"worker has errors after write */	       \
    if (info.m_writeSync)                                      \
     propertyWritten(ordinal);                                 \
  }                                                            \
  void Worker::set##pretty##SequenceProperty(const OA::Property &p, const run *vals, \
					     size_t length) const {	\
    if (p.m_info.m_writeError)						\
      throw; /*"worker has errors before write */			\
    if (p.m_info.m_isSequence) {     					\
      memcpy((void *)(getPropertyVaddr() + p.m_info.m_offset + p.m_info.m_align), vals, \
	     length * sizeof(run));					\
      *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset) =           \
	(uint32_t)(length/p.m_info.m_nItems);				\
    } else								\
      memcpy((void *)(getPropertyVaddr() + p.m_info.m_offset), vals,	\
	     length * sizeof(run));					\
    if (p.m_info.m_writeError)						\
      throw; /*"worker has errors after write */			\
    if (p.m_info.m_writeSync)						\
      propertyWritten(p.m_ordinal); 				        \
  }
// Set a string property value
// ASSUMPTION:  strings always occupy at least 4 bytes, and
// are aligned on 4 byte boundaries.  The offset calculations
// and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)      \
     void Worker::set##pretty##Property(unsigned ordinal, const run val, unsigned idx) const { \
    OA::PropertyInfo &info = properties()[ordinal];                   \
    size_t ocpi_length;                                               \
    if (!val || (ocpi_length = strlen(val)) > info.m_stringLength)    \
      throw; /*"string property too long"*/;			      \
    if (info.m_writeError)					      \
      throw; /*"worker has errors before write */		      \
    uint32_t *p32 = (uint32_t *)(getPropertyVaddr() + info.m_offset + \
				 info.m_elementBytes * idx);	      \
    /* if length to be written is more than 32 bits */		      \
    if (++ocpi_length > 32/CHAR_BIT)				      \
      memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT);  \
    uint32_t i;							      \
    memcpy(&i, val, 32/CHAR_BIT);				      \
    p32[0] = i;							      \
    if (info.m_writeError)					      \
      throw; /*"worker has errors after write */		      \
    if (info.m_writeSync)					      \
      propertyWritten(ordinal); 				      \
  }								      \
  void Worker::set##pretty##SequenceProperty(const OA::Property &p,const run *vals,\
					     size_t length) const {	\
    if (p.m_info.m_writeError)						\
      throw; /*"worker has errors before write */			\
    char *cp = (char *)(getPropertyVaddr() + p.m_info.m_offset + 32/CHAR_BIT); \
    for (unsigned i = 0; i < length; i++) {				\
      size_t len = strlen(vals[i]);					\
      if (len > p.m_info.m_stringLength)				\
	throw; /* "string in sequence too long" */			\
      memcpy(cp, vals[i], len+1);					\
    }									\
    *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset) = (uint32_t)length; \
    if (p.m_info.m_writeError)						\
      throw; /*"worker has errors after write */			\
    if (p.m_info.m_writeSync)						\
      propertyWritten(p.m_ordinal); 				        \
  }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		    \
      run Worker::get##pretty##Property(unsigned ordinal, unsigned idx) const { \
        OA::PropertyInfo &info = properties()[ordinal];                     \
	if (info.m_readSync)						    \
	  propertyRead(ordinal);					    \
        if (info.m_readError )					            \
          throw; /*"worker has errors before read "*/			    \
        uint32_t *pp = (uint32_t *)(getPropertyVaddr() + info.m_offset +    \
				    info.m_elementBytes * idx);		    \
        union {								    \
	  run r;							    \
	  uint32_t u32[bits/32];                                            \
        } u;								    \
        if (bits > 32)							    \
          u.u32[1] = pp[1];						    \
        u.u32[0] = pp[0];						    \
        if (info.m_readError )				        	    \
          throw; /*"worker has errors after read */			    \
        return u.r;							    \
      }									    \
      unsigned Worker::get##pretty##SequenceProperty(const OA::Property &p, \
					     run *vals,			    \
					     size_t length) const {	    \
	if (p.m_info.m_readSync)					    \
	  propertyRead(p.m_info.m_ordinal);				    \
        if (p.m_info.m_readError )					    \
          throw; /*"worker has errors before read "*/			    \
	if (p.m_info.m_isSequence) {					    \
	  uint32_t nSeq = *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset); \
	  size_t n = nSeq * p.m_info.m_nItems;				    \
	  if (n > length)						    \
	    throw "sequence longer than provided buffer";		    \
	  memcpy(vals,							    \
		 (void*)(getPropertyVaddr() + p.m_info.m_offset + p.m_info.m_align), \
		 n * sizeof(run));					    \
	  length = n;							    \
	} else								    \
	  memcpy(vals,							    \
		 (void*)(getPropertyVaddr() + p.m_info.m_offset),	    \
		 length * sizeof(run));					    \
	if (p.m_info.m_readError )					    \
	  throw; /*"worker has errors after read */			    \
	return (unsigned)length;         				    \
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)	            \
      void Worker::get##pretty##Property(unsigned ordinal, char *cp, size_t length, \
					 unsigned idx) const {		            \
          OA::PropertyInfo &info = properties()[ordinal];                           \
	  if (info.m_readSync)	   				                    \
	    propertyRead(ordinal);				                    \
	  size_t stringLength = info.m_stringLength;                                \
	  if (length < stringLength+1)			                            \
	    throw; /*"string buffer smaller than property"*/;		            \
	  if (info.m_readError)					                    \
	    throw; /*"worker has errors before write */			            \
	  uint32_t i32, *p32 = (uint32_t *)(getPropertyVaddr() + info.m_offset +    \
					    info.m_elementBytes * idx);		    \
	  memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT);        \
	  i32 = *p32;							            \
	  memcpy(cp, &i32, 32/CHAR_BIT);				            \
	  if (info.m_readError)					                    \
	    throw; /*"worker has errors after write */			            \
	}								            \
      unsigned Worker::get##pretty##SequenceProperty			    \
	(const OA::Property &p, char **vals, size_t length, char *buf,      \
	 size_t space) const {					            \
	if (p.m_info.m_readSync)	  				    \
	    propertyRead(p.m_info.m_ordinal);				    \
        if (p.m_info.m_readError)					    \
          throw; /*"worker has errors before read */                        \
        uint32_t                                                            \
          n = *(uint32_t *)(getPropertyVaddr() + p.m_info.m_offset);	    \
	size_t wlen = p.m_info.m_stringLength + 1;			    \
        if (n > length)                                                     \
          throw; /* sequence longer than provided buffer */                 \
        char *cp = (char *)(getPropertyVaddr() + p.m_info.m_offset + 32/CHAR_BIT); \
        for (unsigned i = 0; i < n; i++) {                                  \
          if (space < wlen)                                                 \
            throw;                                                          \
          memcpy(buf, cp, wlen);                                            \
          cp += wlen;                                                       \
          vals[i] = buf;                                                    \
          size_t slen = strlen(buf) + 1;                                  \
          buf += slen;                                                      \
          space -= slen;                                                    \
        }                                                                   \
        if (p.m_info.m_readError)                                           \
          throw; /*"worker has errors after read */                         \
        return n;                                                           \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
      void Worker::setPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
			    const uint8_t *data, size_t nBytes, unsigned idx) const {
        memcpy((void *)(getPropertyVaddr() + offset + idx * info.m_elementBytes), data, nBytes);
      }
      void Worker::setProperty8(const OCPI::API::PropertyInfo &info, uint8_t data,
				unsigned idx) const {
        ((uint8_t *)(getPropertyVaddr() + info.m_offset))[idx] = data;
      }
     void Worker::setProperty16(const OCPI::API::PropertyInfo &info, uint16_t data,
				unsigned idx) const {
       ((uint16_t *)(getPropertyVaddr() + info.m_offset))[idx] = data;
      }
     void Worker::setProperty32(const OCPI::API::PropertyInfo &info, uint32_t data,
				unsigned idx) const {
       ((uint32_t *)(getPropertyVaddr() + info.m_offset))[idx] = data;
      };
     void Worker::setProperty64(const OCPI::API::PropertyInfo &info, uint64_t data,
				unsigned idx) const {
       ((uint64_t *)(getPropertyVaddr() + info.m_offset))[idx] = data;
      }
      void Worker::getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
				    uint8_t *data, size_t nBytes, unsigned idx) const {
        memcpy(data, (void *)(getPropertyVaddr() + offset + idx * info.m_elementBytes), nBytes);
      }
     uint8_t Worker::getProperty8(const OCPI::API::PropertyInfo &info, unsigned idx) const {
       return ((uint8_t *)(getPropertyVaddr() + info.m_offset))[idx];
      }
     uint16_t Worker::getProperty16(const OCPI::API::PropertyInfo &info, unsigned idx) const {
       return ((uint16_t *)(getPropertyVaddr() + info.m_offset))[idx];

      }
     uint32_t Worker::getProperty32(const OCPI::API::PropertyInfo &info, unsigned idx) const {
       return ((uint32_t *)(getPropertyVaddr() + info.m_offset))[idx];
      };
     uint64_t Worker::getProperty64(const OCPI::API::PropertyInfo &info, unsigned idx) const {
       return ((uint64_t *)(getPropertyVaddr() + info.m_offset))[idx];
      }

   RCCUserWorker::RCCUserWorker()
     : m_worker(*(Worker *)pthread_getspecific(Driver::s_threadKey)), m_first(true),
       m_rcc(m_worker.context()) {
   }
   RCCUserWorker::~RCCUserWorker() {
   }
   const RunCondition *RCCUserWorker::getRunCondition() const {
     return
       m_worker.m_runCondition == &m_worker.m_defaultRunCondition ?
       NULL : m_worker.m_runCondition;
   }
   void RCCUserWorker::setRunCondition(RunCondition *rc) {
     m_worker.setRunCondition(rc ? *rc : m_worker.m_defaultRunCondition);
   }
   OCPI::API::Application &RCCUserWorker::getApplication() {
     OCPI::API::Application *app = m_worker.parent().getApplication();
     if (!app)
       throw
	 OU::Error("Worker \"%s\" is accessing the top level application when it doesn't exist",
		   m_worker.name().c_str());
     return *app;
   }

   // Default worker methods
   RCCResult RCCUserWorker::initialize() { return RCC_OK;}
   RCCResult RCCUserWorker::start() { return RCC_OK;}
   RCCResult RCCUserWorker::stop() { return RCC_OK;}
   RCCResult RCCUserWorker::release() { return RCC_OK;}
   RCCResult RCCUserWorker::test() { return RCC_OK;}
   RCCResult RCCUserWorker::beforeQuery() { return RCC_OK;}
   RCCResult RCCUserWorker::afterConfigure() { return RCC_OK;}
   uint8_t *RCCUserWorker::rawProperties(size_t &size) const { size = 0; return NULL; }
   RCCResult RCCUserWorker::setError(const char *fmt, ...) {
     va_list ap;
     va_start(ap, fmt);
     RCCResult rc = m_worker.setError(fmt, ap);
     va_end(ap);
     return rc;
   }
   RCCTime RCCUserWorker::getTime() {
     return OS::Time::now().bits();
   }
   bool RCCUserWorker::isInitialized() const {
     return m_worker.getControlState() == OU::Worker::INITIALIZED;
   }
   bool RCCUserWorker::isOperating() const {
     return m_worker.getControlState() == OU::Worker::OPERATING;
   }
   bool RCCUserWorker::isSuspended() const {
     return m_worker.getControlState() == OU::Worker::SUSPENDED;
   }
   bool RCCUserWorker::isFinished() const {
     return m_worker.getControlState() == OU::Worker::FINISHED;
   }
   bool RCCUserWorker::isUnusable() const {
     return m_worker.getControlState() == OU::Worker::UNUSABLE;
   }
   RCCUserPort::
   RCCUserPort()
     : m_rccPort(((Worker *)pthread_getspecific(Driver::s_threadKey))->portInit()) {
     m_rccBuffer = &m_rccPort.current;
     m_rccPort.userPort = this;
   };
   // C++ specific buffer initialization.  When C is better integrated, can be common.
   // Opcode is initialized so we can both detect mismatches (opcode vs opcode-specific
   // accessors) and automatically infer opcodes from the use of opcode-specific accessors
   void RCCUserBuffer::
   initBuffer(bool output) {
     m_opCodeSet = !output;
     m_lengthSet = !output;
     m_resized = !output;
     m_rccBuffer->isNew_ = false;
   }
   void RCCUserPort::
   checkOpCode(RCCUserBuffer &buf, unsigned op) const {
     assert(m_rccPort.containerPort);
     assert(op < m_rccPort.containerPort->metaPort().m_nOperations);
     if (buf.m_opCodeSet && op != buf.m_rccBuffer->opCode_)
       throw OU::Error("opcode accessor for %u used when port opcode set to %u",
		       op, buf.m_rccBuffer->opCode_);
     buf.m_opCodeSet = true;
     buf.m_rccBuffer->opCode_ = OCPI_UTRUNCATE(uint8_t, op);
   }
   void *RCCUserPort::
   getArgAddress(RCCUserBuffer &buf, unsigned op, unsigned arg, size_t *length,
		 size_t *capacity) const {
     if (buf.m_rccBuffer->isNew_)
       buf.initBuffer(m_rccPort.containerPort->isOutput());
     checkOpCode(buf, op);
     OU::Operation &o = m_rccPort.containerPort->metaPort().m_operations[op];
     OU::Member &m = o.m_args[arg];
     uint8_t *p = (uint8_t *)buf.m_rccBuffer->data + m.m_offset;
     if (m.m_isSequence) {
       assert(length);
       if (o.m_nArgs == 1) {
	 assert(buf.m_rccBuffer->length_ % m.m_elementBytes == 0);
	 if (!buf.m_lengthSet && !m_rccPort.useDefaultLength_ && !buf.m_resized) {
	   buf.m_rccBuffer->length_ = 0;
	   buf.m_resized = true;
	 }
	 *length = buf.m_rccBuffer->length_ / m.m_elementBytes;
	 if (capacity)
	   *capacity = buf.m_rccBuffer->maxLength / m.m_elementBytes;
       } else {
	 uint32_t *p32 = (uint32_t *)p;
	 if (!buf.m_lengthSet && !m_rccPort.useDefaultLength_ && !buf.m_resized) {
	   *p32 = 0;
	   buf.m_rccBuffer->length_ = m.m_offset + m.m_align;
	   buf.m_resized = true;
	 }
	 *length = *p32;
	 assert(!m.m_sequenceLength || *length <= m.m_sequenceLength);
	 if (capacity)
	   *capacity = (buf.m_rccBuffer->maxLength - m.m_offset) / m.m_elementBytes;
	 return p + m.m_align;
       }
     }
     return p;
   }
   void RCCUserPort::
   setArgSize(RCCUserBuffer &buf, unsigned op, unsigned arg, size_t size) const {
     assert(m_rccPort.containerPort);
     checkOpCode(buf, op);
     OU::Operation &o = m_rccPort.containerPort->metaPort().m_operations[op];
     OU::Member &m = o.m_args[arg];
     assert(m.m_isSequence);
     assert(!m.m_sequenceLength || size <= m.m_sequenceLength);
     size_t
       nBytes = size * m.m_elementBytes,
       limit = buf.m_rccBuffer->maxLength - (o.m_nArgs == 1 ? 0 : m.m_offset + m.m_align);
     if (buf.m_lengthSet)
       throw OU::Error("resize of %zu specified after length set to %zu bytes", size, 
		       buf.m_rccBuffer->length_);
     if (nBytes > limit)
       throw OU::Error("for protocol \"%s\" operation \"%s\" argument \"%s\": "
		       "sequence size %zu (%zu bytes) exceeds remaining buffer size (%zu)",
		       m_rccPort.containerPort->metaPort().OU::Protocol::m_name.c_str(),
		       o.name().c_str(), m.name().c_str(), size, nBytes, limit);
     if (o.m_nArgs == 1)
       buf.m_rccBuffer->length_ = nBytes;
     else {
       *(uint32_t *)((uint8_t*)buf.m_rccBuffer->data + m.m_offset) =
	 OCPI_UTRUNCATE(uint32_t, size);
       buf.m_rccBuffer->length_ = m.m_offset + m.m_align + nBytes;
     }     
     buf.m_resized = true;
   }
   void RCCUserPort::
   send(RCCUserBuffer&buf) {
     rccSend(&m_rccPort, buf.getRccBuffer());
   }
   RCCUserBuffer &RCCUserPort::
   take(RCCUserBuffer *oldBuffer) {
     RCCUserBuffer *nb = new RCCUserBuffer;
     rccTake(&m_rccPort, oldBuffer ? &oldBuffer->m_taken : NULL, &nb->m_taken);
     delete oldBuffer; // FIXME: when C and C++ are more integrated, this will go away
     return *nb;
   }
   bool RCCUserPort::
   request(size_t maxlength) {
     return rccRequest(&m_rccPort, maxlength);
   }
   bool RCCUserPort::
   advance(size_t maxlength) {
     assert(m_rccPort.containerPort);
     if (m_rccPort.containerPort->isOutput()) {
       if (!m_opCodeSet && !m_rccPort.useDefaultOpCode_)
	 throw
	   OU::Error("port \"%s\" advanced without setting opcode or setting default opcode",
		     m_rccPort.containerPort->name().c_str());
       if (!m_lengthSet && !m_resized) {
	 if (!m_rccPort.useDefaultLength_)
	   throw OU::Error("port \"%s\" advanced without setting length or resizing sequence",
			   m_rccPort.containerPort->name().c_str());
	 // If we are allowed to default the length due to there being only one operation
	 // and the last argument is a sequence that is not the first argument, make sure
	 // to set the embedded sequence length to maintain the integrity of the message
	 if (m_rccPort.sequence) {
	   OU::Member &m = *m_rccPort.sequence;
	   *(uint32_t *)((uint8_t*)m_rccPort.current.data + m.m_offset) =
	     OCPI_UTRUNCATE(uint32_t,
			    m_rccPort.current.length_ - (m.m_offset + m.m_align)) /
	     m.m_elementBytes;
	 }
       }
     }
     return rccAdvance(&m_rccPort, maxlength);
   }
   // FIXME: the connectivity indication should be cached somewhere better...
   bool RCCUserPort::
   isConnected() {
     return m_rccPort.containerPort->parent().m_context->connectedPorts &
       (1 << m_rccPort.containerPort->ordinal());
   }
   RCCOrdinal RCCUserPort::
   ordinal() const { return (RCCOrdinal)m_rccPort.containerPort->ordinal(); }

   bool RCCUserPort::
   wait(size_t max, unsigned usecs) {
     return rccWait(&m_rccPort, max, usecs);
   }
   void RCCUserPort::
   checkLength(size_t length) {
     if (!hasBuffer())
       throw OU::Error("Port has no buffer.  The checkLength method is invalid.");
     if (length > maxLength())
       throw OU::Error("Checked length %zu on port \"%s\" of worker \"%s\" exceeds buffer size: %zu",
		       length, m_rccPort.containerPort->name().c_str(),
		       m_rccPort.containerPort->parent().name().c_str(), maxLength());
   }
   size_t RCCUserPort::
   topLength(size_t eLength) {
     if (!hasBuffer())
       throw OU::Error("Port has no buffer.  The topLength method is invalid.");
     if (length() % eLength)
       throw OU::Error("Message length (%zu) is not a multiple of the top level sequence element size (%zu)",
		       length(), eLength);
     return length()/eLength;
   }
   void RCCUserPort::
   setDefaultLength(size_t length) {
     // FIXME check for input port
     m_rccPort.defaultLength_ = length;
     m_rccPort.useDefaultLength_ = true;
   }
   void RCCUserPort::
   setDefaultOpCode(RCCOpCode op) {
     // FIXME check for input port
     m_rccPort.defaultOpCode_ = op;
     m_rccPort.useDefaultOpCode_ = true;
   }

   RCCPortOperationArg::
   RCCPortOperationArg(RCCPortOperation &po, unsigned arg)
     : m_arg(arg), m_op(po) {
   }

   RCCUserBuffer::
   RCCUserBuffer() : m_rccBuffer(&m_taken), m_opCodeSet(false), m_lengthSet(false) {
   }
   RCCUserBuffer::
   ~RCCUserBuffer() {}
   void RCCUserBuffer::
   setRccBuffer(RCCBuffer *b) {
     m_rccBuffer = b;
   }
   void RCCUserBuffer::
   release() {
     rccRelease(m_rccBuffer);
     delete this; // this will go away when we integrate C and C++ better
   }
   void RCCUserBuffer::
   setOpCode(RCCOpCode op) {
     if (m_rccBuffer->isNew_)
       initBuffer();
     if (m_opCodeSet && op != m_rccBuffer->opCode_)
       throw OU::Error("opcodes cannot be changed after being set, explicitly or implicitly "
		       "(was %d, requested %d)", m_rccBuffer->opCode_, op);
     m_rccBuffer->opCode_ = op;
     m_opCodeSet = true;
   }


   RCCUserSlave::
   RCCUserSlave()
     : m_worker(((OCPI::RCC::Worker *)pthread_getspecific(Driver::s_threadKey))->getSlave())
   {
   }
#if 0
   RCCUserSlave::
   ~RCCUserSlave() {
   }
#endif
#ifndef NBBY
#define NBBY 8
#endif
   RunCondition::
   RunCondition()
     : m_portMasks(m_myMasks), m_timeout(false), m_usecs(0), m_allocated(NULL), m_allMasks(0) {
     m_myMasks[0] = RCC_ALL_PORTS; // all connected ports must be ready
     m_myMasks[1] = 0;
     m_allMasks = m_myMasks[0];
   }
   RunCondition::
   RunCondition(RCCPortMask pm, ...) :
     m_timeout(false), m_usecs(0), m_allocated(NULL), m_allMasks(0) {
     va_list ap;
     va_start(ap, pm);
     initMasks(ap);
     va_end(ap);
     va_start(ap, pm);
     setMasks(pm, ap);
     va_end(ap);
   }
   RunCondition::
   RunCondition(RCCPortMask *rpm, uint32_t usecs, bool timeout)
     : m_portMasks(NULL), m_timeout(timeout), m_usecs(usecs), m_allocated(NULL), m_allMasks(0) {
     setPortMasks(rpm);
   }
   RunCondition::
   ~RunCondition() {
     delete [] m_allocated;
   }
   void RunCondition::
   initMasks(va_list ap) {
     unsigned n;
     RCCPortMask m;
     for (n = 2; (m = va_arg(ap, RCCPortMask)); n++)
	;
     if (n < sizeof(m_myMasks)/sizeof(RCCPortMask))
       m_portMasks = m_allocated = new RCCPortMask[n];
     else
       m_portMasks = m_myMasks;
   }
   void RunCondition::
   setMasks(RCCPortMask first, va_list ap) {
     RCCPortMask
       *pms = m_portMasks,
       m = first;
     do {
       *pms++ = m;
       m_allMasks |= m;
     } while ((m = va_arg(ap, RCCPortMask)));
     *pms++ = 0;
   }
   void RunCondition::
   setPortMasks(RCCPortMask pm, ...) {
     delete [] m_allocated;
     m_allocated = NULL;
     m_allMasks = 0;
     va_list ap;
     va_start(ap, pm);
     initMasks(ap);
     va_end(ap);
     va_start(ap, pm);
     setMasks(pm, ap);
     va_end(ap);
   }
   void RunCondition::
   setPortMasks(RCCPortMask *rpm) {
     delete [] m_allocated;
     m_allocated = NULL;
     m_allMasks = 0;
     m_portMasks = NULL;
     if (rpm) {
       unsigned n;
       for (n = 0; rpm[n]; n++)
	 ;
       if (n >= sizeof(m_myMasks)/sizeof(RCCPortMask))
	 m_portMasks = m_allocated = new RCCPortMask[n + 1];
       else
	 m_portMasks = m_myMasks;
       RCCPortMask m;
       RCCPortMask *pms = m_portMasks;
       do {
	 *pms++ = m = *rpm++;
	 m_allMasks |= m;
       } while (m);
     }
   }
  }
}
