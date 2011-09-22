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
#include <OcpiRccPort.h>
#include <OcpiRccContainer.h>
#include <OcpiRccController.h>
#include <OcpiOsMisc.h>
#include <OcpiList.h>
#include <OcpiOsAssert.h>
#include <DtHandshakeControl.h>
#include <DtIntEventHandler.h>
#include <OcpiBuffer.h>
#include <OcpiTransport.h>
#include <OcpiTimeEmit.h>
#include <OcpiOsTimer.h>


// Number of times that we will allow the run method to loop if there 
// is still work to do
#define MAX_RUNS_PER_CALL 10

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace DTP = DataTransport;

namespace OCPI {
  namespace RCC {

Controller::
~Controller ()
{
}

void CP289Release( RCCBuffer* buffer )
{
  OCPI::DataTransport::BufferUserFacet* dti_buffer = static_cast<OCPI::DataTransport::BufferUserFacet*>(buffer->id_);
  OpaquePortData * id = static_cast<OpaquePortData*>( dti_buffer->m_ud );
  ocpiAssert( ! id->port->isOutput() );
  id->readyToAdvance = true;
  if ( dti_buffer ) {
    id->port->advance(dti_buffer);    
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
  ( void ) op;
  OpaquePortData*  bopq = 
    static_cast<OpaquePortData*>( static_cast<OCPI::DataTransport::BufferUserFacet*>(buffer->id_)->m_ud );
  OpaquePortData* popq = static_cast<OpaquePortData*>(port->opaque);
  popq->readyToAdvance = true;
  popq->port->sendZcopyInputBuffer(  bopq->buffer, len);
}


void CP289Send( ::RCCPort* port, ::RCCBuffer* buffer, ::RCCOrdinal op, ::uint32_t len )
{
  ( void ) op;
  // Cant send data out a input port.
  OpaquePortData* opq = static_cast<OpaquePortData*>(port->opaque);
  if ( !opq ) {
    printf ( "tatic_cast<OpaquePortData*>(port->opaque) is null\n" );
  }
  if ( ! static_cast<OpaquePortData*>(port->opaque)->port->isOutput() ) {
    return;
  }

  // If this is one of our input buffers, it means that the worker wants to 
  // send it instead of a output buffer to eliminate a copy.
  OpaquePortData*  bopq = static_cast<OpaquePortData*>
    ( static_cast<OCPI::DataTransport::BufferUserFacet*>(buffer->id_)->m_ud );

  static_cast<OpaquePortData*>(port->opaque)->readyToAdvance = true;
  bopq->readyToAdvance = true;
  if ( ! bopq->port->isOutput() ) {
    CP289SendZCopy( port, buffer, op, len );
    port->current.data = NULL;
    bopq->cp289Port->current.data = NULL;
    return;
  }
  opq->port->advance( static_cast<OpaquePortData*>(port->opaque)->buffer, op, len );
  port->current.data = NULL;
}

RCCBoolean CP289Advance( ::RCCPort* wport, ::uint32_t max )
{
  ( void ) max;
  OpaquePortData *opd = static_cast<OpaquePortData*>(wport->opaque); 
  opd->readyToAdvance = true;
  if ( opd->buffer ) {
    opd->port->advance( opd->buffer, opd->cp289Port->output.u.operation, opd->cp289Port->output.length );
    wport->current.data = NULL;
    return true;
  }
  else {
    return false;
  }
}

RCCBoolean CP289Request( ::RCCPort* port, ::uint32_t max )
{
  OpaquePortData* opq = static_cast<OpaquePortData*>(port->opaque);

  // If this is an output port, set the mask if it has a free buffer
  if ( opq->port->isOutput() ) {

    if ( port->current.data ) {
      return CP289Advance( port, max);
    }
    else if ( opq->port->hasEmptyOutputBuffer() ) {
      if ( opq->readyToAdvance ) {
        opq->buffer = opq->port->getNextEmptyOutputBuffer();
        if ( opq->buffer ) {
          port->current.data = (void*)opq->buffer->getBuffer();
          opq->buffer->m_ud = opq;
          port->current.id_ = opq->buffer;
#ifdef NEEDED
          port->output.length = opq->buffer->getMetaData()->ocpiMetaDataWord.length;
#endif
          port->output.length = opq->buffer->getDataLength();
          port->output.u.operation =  opq->buffer->opcode();
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
    else if ( opq->port->hasFullInputBuffer() ) {
      if ( opq->readyToAdvance ) {
        opq->buffer = opq->port->getNextFullInputBuffer();
        if ( opq->buffer ) {

          port->current.data = (void*)opq->buffer->getBuffer();
          opq->buffer->m_ud = opq;
          port->current.id_ = opq->buffer;
          port->input.length = opq->buffer->getDataLength();
          port->input.u.operation = opq->buffer->opcode();

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
  ( void ) port;
  ( void ) max;
  ( void ) usec;
  return false;
}

void CP289Take(RCCPort *rccPort, RCCBuffer *oldBuffer, RCCBuffer *newBuffer)
{
  OpaquePortData *opq = static_cast<OpaquePortData*>(rccPort->opaque);
  if ( opq->port->isOutput() ) {
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



Controller::
Controller(Container* c, const char * monitorIPAddress)
  : m_container(c)
{
  ( void ) monitorIPAddress;
  release = CP289Release;
  send    = CP289Send;
  request = CP289Request;
  advance = CP289Advance;
  wait    = CP289Wait;
  take    = CP289Take;
}
  }
}
