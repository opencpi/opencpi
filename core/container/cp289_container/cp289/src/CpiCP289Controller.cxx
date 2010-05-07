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
 *   This file contains the interface for the CP289 Component Execution controller.
 *
   Revision History: 

   06/23/09 - John Miller
   Integrated and debugged worker timeout.

   06/14/09 - John Miller
   Integrated and debugged the advanced buffer features.

   05/18/09 - John Miller
   Updated to support revised CP289U RCC specification.
 
   03/01/05 - John Miller
   Initial version.   

 *
 *********************************************************/


#define WORKER_INTERNAL
#include <CpiCP289Port.h>
#include <CpiCP289Container.h>
#include <CpiCP289Controller.h>
#include <CpiOsMisc.h>
#include <CpiList.h>
#include <CpiOsAssert.h>
#include <DtHandshakeControl.h>
#include <DtIntEventHandler.h>
#include <CpiBuffer.h>
#include <CpiTransport.h>
#include <CpiTimeEmit.h>
#include <CpiOsTimer.h>


// Number of times that we will allow the run method to loop if there 
// is still work to do
#define MAX_RUNS_PER_CALL 10


using namespace CPI::Util;
using namespace CPI::Container;
using namespace CPI::CP289;
using namespace DataTransport;
using namespace CPI::OS;
using namespace DtI;


CPI::CP289::Controller::
~Controller ()
{
}


CPI::OS::int32_t        
CPI::CP289::Controller::
markWorkersPolled( CPI::CP289::Worker* worker )
{
  // Create the active mask
  CPI::OS::int32_t active_mask=0;
  CPI::CP289::Port * cpiport = static_cast<CPI::CP289::Port*>(worker->firstChild());

  while ( cpiport ) {

    // If we dont care about this port, ignore it
    CPI::OS::uint32_t portMask = (1<<cpiport->portId());
    if ( !(worker->runConditionSS & portMask) ) {
      cpiport = static_cast<CPI::CP289::Port*>(worker->nextChild(cpiport));
      continue;
    }

    // If this is an output port, set the mask if it has a free buffer
    if ( cpiport->dtPort()->isOutput() ) {        
      CPI::DataTransport::Port* port = cpiport->dtPort();
      RCCPort *wport = &worker->m_rcc_worker->m_context->ports[cpiport->portId()];
      if ( wport->current.data ) {
        active_mask |= (1<<cpiport->portId());
      }
      else if ( port->hasEmptyOutputBuffer() ) {
        CPI::CP289::OpaquePortData *opd = static_cast<CPI::CP289::OpaquePortData*>(wport->opaque);
        if ( opd->readyToAdvance ) {
          opd->buffer = port->getNextEmptyOutputBuffer();
          if ( opd->buffer ) {
            opd->buffer->opaque = opd;
            wport->current.data = (void*)opd->buffer->getBuffer();
            wport->current.id_ = opd->buffer;
            wport->output.length =         
              (CPI::OS::uint32_t)N_BYTES_TRANSFERED((CPI::OS::uint32_t)opd->buffer->getMetaData()->cpiMetaDataWord);
            wport->output.u.operation =         
              (CPI::OS::uint32_t)DECODE_OPCODE(opd->buffer->getMetaData()->cpiMetaDataWord);
          }
          else {
            wport->current.data = NULL;
          }
          opd->readyToAdvance = false;
        }
        active_mask |= (1<<cpiport->portId());
      }
    }

    else {  // This is an input port, so we are looking for data
      CPI::DataTransport::Port* port = cpiport->dtPort();
      RCCPort *wport = &worker->m_rcc_worker->m_context->ports[cpiport->portId()];
      CPI::CP289::OpaquePortData *opd = static_cast<CPI::CP289::OpaquePortData*>(wport->opaque);

      if ( wport->current.data ) {
        active_mask |= (1<<cpiport->portId());
      }
      else if ( port->hasFullInputBuffer() ) {
        cpiAssert( opd->readyToAdvance );
        if ( opd->readyToAdvance ) {
          opd->buffer = port->getNextFullInputBuffer();
          if ( opd->buffer ) {
            wport->current.data = (void*)opd->buffer->getBuffer();
            opd->buffer->opaque = opd;
            wport->current.id_ = opd->buffer;
            wport->input.length = 
              (CPI::OS::uint32_t)N_BYTES_TRANSFERED((CPI::OS::uint32_t)opd->buffer->getMetaData()->cpiMetaDataWord);
            wport->input.u.operation =         
              (CPI::OS::uint32_t)DECODE_OPCODE(opd->buffer->getMetaData()->cpiMetaDataWord);
            cpiAssert( wport->input.length <= wport->current.maxLength );

#ifndef NDEBUG
            printf("max = %d, actual = %d\n", wport->current.maxLength, wport->input.length );
#endif
            active_mask |= (1<<cpiport->portId());
            opd->readyToAdvance = false;
          }
          else {
            wport->current.data = NULL;
          }
        }
      }
    }
          
    cpiport = static_cast<CPI::CP289::Port*>(worker->nextChild(cpiport));

  } // End for each port


  return active_mask;
}


void checkWorkerDeadLock( CPI::CP289::Worker* worker )
{
  // Not yet implemented !!
}




volatile int cpi_dbg_run=0;

bool 
CPI::CP289::Controller::
run(DataTransfer::EventManager* event_manager )
{
  CPI::OS::uint32_t active_mask;
  CPI::CP289::Application* app;

  CPI_EMIT_("Start Worker Evaluation");

#ifndef NDEBUG
  if ( cpi_dbg_run ) {
    printf("WORKER RUN: Entry\n");
  }
#endif

  // Give our transport some time
  m_container->m_transport->dispatch( event_manager );


  //#define VECTOR_BUFFERS_FROM_EVENTS
#ifndef VECTOR_BUFFERS_FROM_EVENTS
  if ( event_manager ) {
    event_manager->consumeEvents();
  }
#endif


  // Process the workers
  bool anyone_run = false;
  int run_count=0;
  
  
  app = 
    static_cast<CPI::CP289::Application*>( m_container-> Parent<CPI::Container::Application>::firstChild() );
  if ( ! app ) {
    return false;
  }
  do {

    CPI::CP289::Worker * worker = static_cast<CPI::CP289::Worker*>(app->firstChild());      
    while ( worker ) {


      RCCBoolean timeout = false;

      // Give our transport some time
      m_container->m_transport->dispatch( event_manager );

      // Make sure the worker is enabled
      if ( ! worker->enabled ) {
        worker = static_cast<CPI::CP289::Worker*>(app->nextChild(worker));
        continue;
      }
      else {
        worker->run_condition_met = false;
      }

      // Check if this worker has a timer for a run condition
      if ( worker->m_rcc_worker->m_dispatch->runCondition && worker->m_rcc_worker->m_dispatch->runCondition->timeout ) {
        CPI::OS::Timer::ElapsedTime et;
        worker->runTimer.stop();
        worker->runTimer.getValue( et );
        worker->runTimer.start();
        if ( et > worker->runTimeout ) {          
#ifndef NDEBUG
          printf("WORKER TIMED OUT, timer time = %d,%d -- run timer = %d,%d\n", 
                 et.seconds, et.nanoseconds, worker->runTimeout.seconds, worker->runTimeout.nanoseconds );
#endif
          worker->run_condition_met = true;
          timeout = true;
        }
      }
      active_mask = markWorkersPolled( worker );

#ifndef NDEBUG
      if ( cpi_dbg_run ) {
        printf("WORKER RUN: worker active mask = %d\n", active_mask );
      }
#endif

      // Now see if the mask matches any of the workers run conditions
      int n=0;
      if ( worker->m_rcc_worker->m_context->runCondition ) {

        // We need to ignore optional ports that are NOT connected
        CPI::OS::uint32_t op_nc = 
          (worker->m_rcc_worker->m_dispatch->optionallyConnectedPorts &  ~worker->m_rcc_worker->m_context->connectedPorts);

        CPI::OS::uint32_t t_active_mask = active_mask | op_nc;

        while ( worker->m_rcc_worker->m_context->runCondition->portMasks && 
                worker->m_rcc_worker->m_context->runCondition->portMasks[n] ) {
          if ( t_active_mask && ((worker->m_rcc_worker->m_context->runCondition->portMasks[n]&t_active_mask ) == 
                                 worker->m_rcc_worker->m_context->runCondition->portMasks[n]) ) {
            worker->run_condition_met = true;
            break;
          }
          n++;
        }
      }
      else {
        worker->run_condition_met = true;
      }

      // Run the worker if needed
      if ( worker->run_condition_met ) {
        RCCResult wr;

        // If the worker has defined a port method, we will call it instead of the run method
        // for each port that has one
        CPI::OS::int32_t pord=0;
        bool execute_run = true;
        CPI::OS::uint32_t mtest=0;
        while ( mtest<active_mask  ) {
          mtest = 1<<pord;
          if ( (mtest&active_mask)==mtest) {
            if ( worker->m_rcc_worker->m_context->ports[pord].callBack ) {
              execute_run = false;
              RCCResult wr = 
                worker->m_rcc_worker->m_context->ports[pord].callBack( 
                                                                      worker->m_rcc_worker->m_context,
                                                                      &worker->m_rcc_worker->m_context->ports[pord], 
                                                                      RCC_OK);
              if ( wr != RCC_OK ) {
                worker->enabled = false;
                worker->m_rcc_worker->m_state = RCCWorkerInterface::WorkerUnusable;
              }
            }
          }
          pord++;
        }

        if ( execute_run ) {
          anyone_run = true;
          CPI_EMIT_("End Worker Evaluation");
          if ( (wr = worker->m_rcc_worker->run( timeout, &timeout)) == RCC_ADVANCE ) {
            advanceAll( worker );
          }
          else if ( wr == RCC_DONE ) {
            worker->enabled = false;
          }
          else if ( wr != RCC_OK ) {
            worker->enabled = false;
            worker->m_rcc_worker->m_state = RCCWorkerInterface::WorkerUnusable;
            worker->runTimer.stop();
          }
          else {
            worker->runTimer.stop();
            worker->runTimer.reset();
            worker->runTimer.start();
          }
          checkWorkerDeadLock( worker );
          worker->worker_run_count++;
        }
      }

      worker = static_cast<CPI::CP289::Worker*>(app->nextChild(worker));      

    } // End for each worker

    run_count++;
    app = static_cast<CPI::CP289::Application*>(m_container->  Parent<CPI::Container::Application>::nextChild(app));

  }  while ( app && anyone_run && (run_count < MAX_RUNS_PER_CALL) );

  CPI_EMIT_("End Worker Evaluation");

  return anyone_run;
}

// Advance all ports 
void Controller::advanceAll( CPI::CP289::Worker * worker )
{
  CPI_EMIT_( "Start Advance All" );
  int numPorts = worker->m_rcc_worker->m_dispatch->numInputs + worker->m_rcc_worker->m_dispatch->numOutputs;
  for (int n=0; n<numPorts; n++ ) {
    RCCPort *wport = &(worker->m_rcc_worker->m_context->ports[n]);
    CPI::CP289::OpaquePortData *opd = static_cast<CPI::CP289::OpaquePortData*>(wport->opaque); 
    if ( wport->current.data == NULL ) {
      continue;
    }
    opd->readyToAdvance = true;    
    if ( opd->buffer ) {
      opd->port->dtPort()->advance( opd->buffer, opd->cp289Port->output.length );
      wport->current.data = NULL;
    }
  }
  CPI_EMIT_( "End Advance All" );
}


void CP289Release( RCCBuffer* buffer )
{
  CPI::DataTransport::Buffer* dti_buffer = static_cast<CPI::DataTransport::Buffer*>(buffer->id_);
  CPI::CP289::OpaquePortData * id = static_cast<CPI::CP289::OpaquePortData*>( dti_buffer->opaque );
  cpiAssert( ! id->port->dtPort()->isOutput() );
  id->readyToAdvance = true;
  if ( dti_buffer ) {
    id->port->dtPort()->advance(dti_buffer);    
  }
  if ( id->buffer == dti_buffer ) {
    id->cp289Port->current.data = NULL;
  }

}


// This routine is used to allow an input buffer to be sent out an output port with zero copy.
// To avoid having to create complex DD+P transfer lists for all possible transers, we use one of
// the output ports buffer configurations to make the transfer by modifying the output pointers
// to point to the input buffer data.
void CP289SendZCopy( ::RCCPort* port, ::RCCBuffer* buffer, ::RCCOrdinal op, ::uint32_t len )
{
  CPI::CP289::OpaquePortData*  bopq = 
    static_cast<CPI::CP289::OpaquePortData*>( static_cast<CPI::DataTransport::Buffer*>(buffer->id_)->opaque );
  CPI::CP289::OpaquePortData* popq = static_cast<CPI::CP289::OpaquePortData*>(port->opaque);
  popq->readyToAdvance = true;
  popq->port->dtPort()->sendZcopyInputBuffer(  bopq->buffer, len);
}


void CP289Send( ::RCCPort* port, ::RCCBuffer* buffer, ::RCCOrdinal op, ::uint32_t len )
{
  // Cant send data out a input port.
  if ( ! static_cast<CPI::CP289::OpaquePortData*>(port->opaque)->port->dtPort()->isOutput() ) {
    return;
  }

  // If this is one of our input buffers, it means that the worker wants to 
  // send it instead of a output buffer to eliminate a copy.
  CPI::CP289::OpaquePortData*  bopq = 
    static_cast<CPI::CP289::OpaquePortData*>( static_cast<CPI::DataTransport::Buffer*>(buffer->id_)->opaque );
  CPI::DataTransport::Port* t_port = static_cast<CPI::CP289::OpaquePortData*>(port->opaque)->port->dtPort();

  static_cast<CPI::CP289::OpaquePortData*>(port->opaque)->readyToAdvance = true;
  bopq->readyToAdvance = true;
  if ( ! bopq->port->dtPort()->isOutput() ) {
    CP289SendZCopy( port, buffer, op, len );
    port->current.data = NULL;
    bopq->cp289Port->current.data = NULL;
    return;
  }

  static_cast<CPI::CP289::OpaquePortData*>(port->opaque)->buffer->
    getMetaData()->cpiMetaDataWord = (uint64_t)((uint64_t)op<<32);
  t_port->advance( static_cast<CPI::CP289::OpaquePortData*>(port->opaque)->buffer, len );
  port->current.data = NULL;
}

RCCBoolean CP289Advance( ::RCCPort* wport, ::uint32_t max )
{
  CPI::CP289::OpaquePortData *opd = static_cast<CPI::CP289::OpaquePortData*>(wport->opaque); 
  opd->readyToAdvance = true;
  if ( opd->buffer ) {
    opd->buffer->getMetaData()->cpiMetaDataWord = (uint64_t)((uint64_t)opd->cp289Port->output.u.operation<<32);
    opd->port->dtPort()->advance( opd->buffer, opd->cp289Port->output.length );
    wport->current.data = NULL;
    return true;
  }
  else {
    return false;
  }
}

RCCBoolean CP289Request( ::RCCPort* port, ::uint32_t max )
{
  CPI::CP289::OpaquePortData* opq = static_cast<CPI::CP289::OpaquePortData*>(port->opaque);

  // If this is an output port, set the mask if it has a free buffer
  if ( opq->port->dtPort()->isOutput() ) {

    if ( port->current.data ) {
      return CP289Advance( port, max);
    }
    else if ( opq->port->dtPort()->hasEmptyOutputBuffer() ) {
      if ( opq->readyToAdvance ) {
        opq->buffer = opq->port->dtPort()->getNextEmptyOutputBuffer();
        if ( opq->buffer ) {
          port->current.data = (void*)opq->buffer->getBuffer();
          opq->buffer->opaque = opq;
          port->current.id_ = opq->buffer;
          port->output.length = 
            (CPI::OS::uint32_t)N_BYTES_TRANSFERED((CPI::OS::uint32_t)opq->buffer->getMetaData()->cpiMetaDataWord);
          port->output.u.operation =         
              (CPI::OS::uint32_t)DECODE_OPCODE(opq->buffer->getMetaData()->cpiMetaDataWord);

        }
        else {
          port->current.data = NULL;
        }
        opq->readyToAdvance = false;
      }
    }
    else {
      return false;
    }
  }
  else {  // This is an input port, so we are looking for data
    if ( port->current.data ) {
      return CP289Advance( port, max);
    }
    else if ( opq->port->dtPort()->hasFullInputBuffer() ) {
      if ( opq->readyToAdvance ) {
        opq->buffer = opq->port->dtPort()->getNextFullInputBuffer();
        if ( opq->buffer ) {

          port->current.data = (void*)opq->buffer->getBuffer();
          opq->buffer->opaque = opq;
          port->current.id_ = opq->buffer;
          port->input.length = 
            (CPI::OS::uint32_t)N_BYTES_TRANSFERED((CPI::OS::uint32_t)opq->buffer->getMetaData()->cpiMetaDataWord);
          port->input.u.operation =         
              (CPI::OS::uint32_t)DECODE_OPCODE(opq->buffer->getMetaData()->cpiMetaDataWord);

        }
        else {
          port->current.data = NULL;
        }
        opq->readyToAdvance = false;
      }
    }
    else { 
      return false;
    }
  }
  return true;
}


RCCBoolean CP289Wait( ::RCCPort* port, ::uint32_t max, ::uint32_t usec )
{
  // Not implemented yet

  return false;
}

void CP289Take(RCCPort *rccPort, RCCBuffer *oldBuffer, RCCBuffer *newBuffer)
{
  CPI::CP289::OpaquePortData *opq = static_cast<CPI::CP289::OpaquePortData*>(rccPort->opaque);
  if ( opq->port->dtPort()->isOutput() ) {
    return;
  }
 
  *newBuffer = opq->cp289Port->current;
  rccPort->current.data = NULL;
  opq->buffer = NULL;
  opq->readyToAdvance = true;

  if ( oldBuffer ) {
    CP289Release( oldBuffer );
  }
  CP289Request( rccPort,0);
}



CPI::CP289::Controller::
Controller(Container* c, const char * monitorIPAddress)
  : m_container(c)
{
  release = CP289Release;
  send    = CP289Send;
  request = CP289Request;
  advance = CP289Advance;
  wait    = CP289Wait;
  take    = CP289Take;
}







