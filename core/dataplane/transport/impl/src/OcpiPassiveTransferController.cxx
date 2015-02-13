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
 *   This file contains the OCPI transfer controller implementation.
 *
 * Revision History: 
 *
 *    John Miller - 6/15/09
 *    Fixed Coverity issues.
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <OcpiTransferController.h>
#include <OcpiCircuit.h>
#include <OcpiPortSet.h>
#include <OcpiBuffer.h>
#include <OcpiOutputBuffer.h>
#include <OcpiInputBuffer.h>
#include <OcpiIntDataDistribution.h>
#include <OcpiList.h>
#include <OcpiOsAssert.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace OCPI::OS;


///
////  Passive Transfer controller for pattern 1 
///
TransferController1Passive::
TransferController1Passive( PortSet* output, PortSet* input, bool whole_ss) 
{
  init( output, input, whole_ss);
  m_nextTid = 0;

}

TransferController* 
TransferController1Passive::
createController( 
                 PortSet* output, 
                 PortSet* input,
                 bool whole_output_set)
{
  return new TransferController1Passive( output, input, whole_output_set );
}

bool 
TransferController1Passive::
canProduce( Buffer* buffer )
{
  ( void ) buffer;
  return true;
}

void 
TransferController1Passive::
modifyOutputOffsets( Buffer* me, Buffer* new_buffer, bool reverse )
{
  ( void ) me;
  ( void ) new_buffer;
  ( void ) reverse;
  ocpiAssert("PassiveTransferController::modifyOutputOffsets() Should never be called !!\n"==0);
}


int TransferController1Passive::
produce(Buffer* b, bool bcast) {
  ocpiDebug("In TransferController1Passive::produce");
  if (++m_nextTid == m_input->getBufferCount())
    m_nextTid = 0;
  b->markBufferFull();
  return 0;
}

/**********************************
 * This marks the input buffer as "Empty" and informs all interested outputs that
 * the input is now available.
 *********************************/
Buffer* TransferController1Passive::
consume(Buffer* input) {
  input->markBufferEmpty();
  return NULL;
}

#if 0
Buffer* TransferController1Passive::
getNextFullInputBuffer(OCPI::DataTransport::Port* input_port) {
  InputBuffer* buffer;
  if ( hasFullInputBuffer( input_port, &buffer ) ) {
    BufferOrdinal
      &lo = input_port->getLastBufferOrd(),
      tlo = ((lo+1)%input_port->getBufferCount());
    lo = tlo;    
    buffer->setInUse( true );
    buffer->m_pullTransferInProgress = NULL;
    return buffer;
  }
  return NULL;
}



// A input buffer with a AFC output has the following states:
//  
//   Empty - No data available
//   Empty - Data available at output
//   Empty - Data transfer in progress
//   Full  - Not in use
//   Full  - In use
//   Empty - No data available

bool 
TransferController1Passive::
hasFullInputBuffer(
                    OCPI::DataTransport::Port* input_port,
                    InputBuffer** retb
                    )const
{
  InputBuffer* buffer;
  BufferOrdinal
    &lo = input_port->getLastBufferOrd(),
    tlo = ((lo+1)%input_port->getBufferCount());
  *retb = buffer = static_cast<InputBuffer*>(input_port->getBuffer(tlo));
  volatile BufferState* state = buffer->getState();
  //  ocpiAssert(!"AFC buffer check");
  if ((state->bufferIsFull & FF_MASK) == FF_EMPTY_VALUE ||
       buffer->inUse() ) {
    return false;
  }


  // At this point we have determined that the output port has a buffer available for us. 
  if ( !buffer->m_pullTransferInProgress ) {
    // Start the pull transfer now
    buffer->m_pullTransferInProgress = 
      input_port->getCircuit()->getOutputPortSet()->pullData( buffer );
  }
  else {
    if (  buffer->m_pullTransferInProgress->isEmpty() ) {
      return true;
    }
  }

  return false;
}
#endif
