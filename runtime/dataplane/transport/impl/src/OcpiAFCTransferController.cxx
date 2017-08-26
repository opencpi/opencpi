/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
////  ActiveFlowControle Transfer controller for pattern 1 
///
TransferController1AFCShadow::
TransferController1AFCShadow( PortSet* output, PortSet* input, bool whole_ss) 
{
  init( output, input, whole_ss);
  m_nextTid = 0;

}

TransferController* 
TransferController1AFCShadow::
createController( 
                 PortSet* output, 
                 PortSet* input,
                 bool whole_output_set)
{
  return new TransferController1AFCShadow( output, input, whole_output_set );
}

bool 
TransferController1AFCShadow::
canProduce( Buffer* buffer )
{
  ( void ) buffer;
  return true;
}

void 
TransferController1AFCShadow::
modifyOutputOffsets( Buffer* me, Buffer* new_buffer, bool reverse )
{
  ( void ) me;
  ( void ) new_buffer;
  ( void ) reverse;
  ocpiAssert("AFCShadowTransferController::modifyOutputOffsets() Should never be called !!\n"==0);
}


int 
TransferController1AFCShadow::
produce( Buffer* b, bool bcast )
{
  ocpiDebug("In TransferController1AFCShadow::produce");


  Buffer* buffer = static_cast<Buffer*>(b);
  int bcast_idx = bcast ? 1 : 0;
  if ( bcast_idx == 1 ) {
    ocpiDebug("*** producing via broadcast, rank == %d !!", b->getPort()->getRank());

    if ( ! buffer->getMetaData()->endOfStream ) {
      ocpiDebug("*** ERROR *** EOS not set via broadcast !!");
    }
  }

  // With this pattern, only port 0 of the output port set can produce
  if ( m_wholeOutputSet && b->getPort()->getRank() != 0 ) {
    ocpiDebug("My rank != 0 so i am not producing !!!");

    // Next input buffer 
    int tmp = ((m_nextTid++)%m_input->getBufferCount()) - 1;
    m_nextTid = tmp;
    if ( m_nextTid < 0 ) {
      m_nextTid = 0;
    }
    ocpiDebug("AFCTransferController:: m_nextTid = %d", m_nextTid );

    //#define DELAY_FOR_TEST
#ifdef DELAY_FOR_TEST
    Sleep( 500 );
#endif

    return 0;
  }

  // Broadcast if requested
  if ( bcast_idx ) {
    broadCastOutput( buffer );
    return 0;
  }

  /*
   *  For this pattern the output port and input port are constants when we look 
   *  up the template that we need to produce.  So, since the output tid is a given,
   *  the only calculation is the input tid that we are going to produce to.
   */

#ifdef DEBUG_L2
  ocpiDebug("output port id = %d, buffer id = %d, input id = %d", 
         buffer->getPort()->getPortId(), buffer->getTid(), m_nextTid);
  ocpiDebug("Template address = 0x%x", m_templates [buffer->getPort()->getPortId()][buffer->getTid()][0][m_nextTid]);
#endif

  // We need to mark the local buffer as free
  buffer->markBufferFull();

  // Start producing, this may be asynchronous
  OcpiTransferTemplate *temp = 
    m_templates [buffer->getPort()->getPortId()][buffer->getTid()][0][m_nextTid][bcast_idx][OUTPUT];
  temp->produce();

  // Add the template to our list
  insert_to_list(&buffer->getPendingTxList(), m_templates [buffer->getPort()->getPortId()][buffer->getTid()][0][m_nextTid][bcast_idx][OUTPUT], 64, 8);

  // Next input buffer 
  m_nextTid = (m_nextTid + 1) % m_input->getBufferCount();
  if ( m_nextTid < 0 ) {
    m_nextTid = 0;
  }

#ifdef DEBUG_L2
  ocpiDebug("next tid = %d, num buf = %d", m_nextTid, m_input->getBufferCount());
  ocpiDebug("Returning max gated sequence = %d", temp->getMaxGatedSequence());
#endif

  return temp->getMaxGatedSequence();
}

/**********************************
 * This marks the input buffer as "Empty" and informs all interested outputs that
 * the input is now available.
 *********************************/
Buffer* 
TransferController1AFCShadow::
consume( Buffer* input )
{
  ocpiAssert( 1 );


  Buffer* buffer = static_cast<Buffer*>(input);

  // We need to mark the local buffer as free
  buffer->markBufferEmpty();

#ifdef DTI_PORT_COMPLETE
  buffer->setBusyFactor( buffer->getPort()->getCircuit()->getRelativeLoadFactor() );
#endif


#ifdef DEBUG_L2
  ocpiDebug("Set load factor to %d", buffer->getState()->pad);
  ocpiDebug("Consuming using tpid = %d, ttid = %d, template = 0x%x",input->getPort()->getPortId(),
         input->getTid(), m_templates [0][0][input->getPort()->getPortId()][input->getTid()][0][INPUT] );
#endif

  // Tell everyone that we are empty
  return m_templates [0][0][input->getPort()->getPortId()][input->getTid()][0][INPUT]->consume();

}



#if 0
/**********************************
 * This method gets the next available buffer from the specified output port
 *********************************/
Buffer* 
TransferController1AFCShadow::
getNextEmptyOutputBuffer( 
                                                     OCPI::DataTransport::Port* src_port        
                                                     )
{
  OutputBuffer* boi=NULL;        
  OCPI::OS::uint32_t &n = src_port->getLastBufferTidProcessed();
  boi = static_cast<OutputBuffer*>(src_port->getBuffer(n));
  n = (n+1) % src_port->getBufferCount();
  return boi;
}
#endif


Buffer* 
TransferController1AFCShadow::
getNextFullInputBuffer( 
                    OCPI::DataTransport::Port* input_port 
                    )
{

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
TransferController1AFCShadow::
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
