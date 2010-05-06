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
 *   This file contains the CPI transfer controller implementation.
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

#include <CpiTransferController.h>
#include <CpiCircuit.h>
#include <CpiPortSet.h>
#include <CpiBuffer.h>
#include <CpiOutputBuffer.h>
#include <CpiInputBuffer.h>
#include <CpiIntDataDistribution.h>
#include <CpiList.h>
#include <CpiOsAssert.h>

using namespace CPI::DataTransport;
using namespace DataTransfer;
using namespace CPI::OS;


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
  cpiAssert( buffer->getPort()->isShadow() );
  return true;
}

void 
TransferController1AFCShadow::
modifyOutputOffsets( Buffer* me, Buffer* new_buffer, bool reverse )
{
  cpiAssert(!"AFCShadowTransferController::modifyOutputOffsets() Should never be called !!\n");
}


int 
TransferController1AFCShadow::
produce( Buffer* b, bool bcast )
{
  Buffer* buffer = static_cast<Buffer*>(b);
  int bcast_idx = bcast ? 1 : 0;
  if ( bcast_idx == 1 ) {
#ifndef NDEBUG
    printf("*** producing via broadcast, rank == %d !!\n", b->getPort()->getRank());
#endif

    if ( ! buffer->getMetaData()->endOfStream ) {
#ifndef NDEBUG
      printf("*** ERROR *** EOS not set via broadcast !!\n");
#endif
    }
  }

  // With this pattern, only port 0 of the output port set can produce
  if ( m_wholeOutputSet && b->getPort()->getRank() != 0 ) {
#ifndef DEBUG
    printf("My rank != 0 so i am not producing !!! \n");
#endif

    // Next input buffer 
    int tmp = ((m_nextTid++)%m_input->getBufferCount()) - 1;
    m_nextTid = tmp;
    if ( m_nextTid < 0 ) {
      m_nextTid = 0;
    }
#ifndef NDEBUG
    printf("AFCTransferController:: m_nextTid = %d\n", m_nextTid );
#endif

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
  printf("output port id = %d, buffer id = %d, input id = %d\n", 
         buffer->getPort()->getPortId(), buffer->getTid(), m_nextTid);
  printf("Template address = 0x%x\n", m_templates [buffer->getPort()->getPortId()][buffer->getTid()][0][m_nextTid]);
#endif

  // Start producing, this may be asynchronous
  CpiTransferTemplate *temp = 
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
  printf("next tid = %d, num buf = %d\n", m_nextTid, m_input->getBufferCount());
  printf("Returning max gated sequence = %d\n", temp->getMaxGatedSequence());
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
  cpiAssert( 1 );


  Buffer* buffer = static_cast<Buffer*>(input);

  // We need to mark the local buffer as free
  buffer->markBufferEmpty();

#ifdef DTI_PORT_COMPLETE
  buffer->setBusyFactor( buffer->getPort()->getCircuit()->getRelativeLoadFactor() );
#endif


#ifdef DEBUG_L2
  printf("Set load factor to %d\n", buffer->getState()->pad);
  printf("Consuming using tpid = %d, ttid = %d, template = 0x%x\n",input->getPort()->getPortId(),
         input->getTid(), m_templates [0][0][input->getPort()->getPortId()][input->getTid()][0][INPUT] );
#endif

  // Tell everyone that we are empty
  return m_templates [0][0][input->getPort()->getPortId()][input->getTid()][0][INPUT]->consume();

}



/**********************************
 * This method gets the next available buffer from the specified output port
 *********************************/
Buffer* 
TransferController1AFCShadow::
getNextEmptyOutputBuffer( 
                                                     CPI::DataTransport::Port* src_port        
                                                     )
{
  OutputBuffer* boi=NULL;        
  CPI::OS::uint32_t &n = src_port->getLastBufferTidProcessed();
  boi = static_cast<OutputBuffer*>(src_port->getBuffer(n));
  n = (n+1) % src_port->getBufferCount();
  return boi;
}



Buffer* 
TransferController1AFCShadow::
getNextFullInputBuffer( 
                    CPI::DataTransport::Port* input_port 
                    )
{

  InputBuffer* buffer;
  if ( hasFullInputBuffer( input_port, &buffer ) ) {
    int& lo = input_port->getLastBufferOrd();
    int tlo = ((lo+1)%input_port->getBufferCount());
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
                    CPI::DataTransport::Port* input_port,
                    InputBuffer** retb
                    )const
{
  InputBuffer* buffer;
  int& lo = input_port->getLastBufferOrd();
  int tlo = ((lo+1)%input_port->getBufferCount());
  *retb = buffer = static_cast<InputBuffer*>(input_port->getBuffer(tlo));
  volatile BufferState* state = buffer->getState();
  if ( ((state->bufferFull & DataTransfer::BufferEmptyFlag) == DataTransfer::BufferEmptyFlag ) ||
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
