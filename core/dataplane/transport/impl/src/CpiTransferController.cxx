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


/**********************************
 * Constructor
 *********************************/
TransferController::TransferController()
  : m_FillQPtr(0), m_EmptyQPtr(0)
{                         

  for ( CPI::OS::uint32_t x=0; x<MAX_PCONTRIBS; x++ ) {
    for ( CPI::OS::uint32_t y=0; y<MAX_BUFFERS; y++ ) {
      for ( CPI::OS::uint32_t z=0; z<MAX_PCONTRIBS; z++ ) {
        for ( CPI::OS::uint32_t zz=0; zz<MAX_BUFFERS; zz++ ) {
          for ( CPI::OS::uint32_t bc=0; bc<2; bc++) {
            for ( CPI::OS::uint32_t ts=0; ts<2; ts++) {
              m_templates[x][y][z][zz][bc][ts] = NULL;
            }
          }
        }
      }
    }
  }

  m_wholeOutputSet=true;
}

/**********************************
 * Destructor
 *********************************/
TransferController::~TransferController()
{

  for ( CPI::OS::uint32_t x=0; x<MAX_PCONTRIBS; x++ ) {
    for ( CPI::OS::uint32_t y=0; y<MAX_BUFFERS; y++ ) {
      for ( CPI::OS::uint32_t z=0; z<MAX_PCONTRIBS; z++ ) {
        for ( CPI::OS::uint32_t zz=0; zz<MAX_BUFFERS; zz++ ) {
          for ( CPI::OS::uint32_t bc=0; bc<2; bc++) {
            for ( CPI::OS::uint32_t ts=0; ts<2; ts++) {
              delete m_templates[x][y][z][zz][bc][ts];
            }
          }
        }
      }
    }
  }

}

bool TransferController::hasEmptyOutputBuffer(
                                                  CPI::DataTransport::Port* src_port
                                                  )const
{
  CPI::OS::uint32_t &n = src_port->getLastBufferTidProcessed();
  OutputBuffer* buffer =  src_port->getOutputBuffer(n);
  if (  buffer->isEmpty() && ! buffer->inUse() ) {
    return true;
  }
  return false;
}

bool TransferController::hasFullInputBuffer(
                                             CPI::DataTransport::Port* input_port,
                                             InputBuffer** retb
                                             )const
{
  InputBuffer* buffer;
  int& lo = input_port->getLastBufferOrd();
  int tlo = ((lo+1)%input_port->getBufferCount());
  *retb = buffer = input_port->getInputBuffer(tlo);
  if (  ! buffer->isEmpty() && ! buffer->inUse() ) {
    return true;
  }
  return false;

}


void TransferController::bufferFull(
                                    CPI::DataTransport::Port* port                                                
                                    )
{
  //        We treat the input buffers as a circular queue, so we only need to check
  // the next buffer 
  port->getBuffer(m_FillQPtr)->markBufferFull();
  m_FillQPtr++ ;
  if ( m_FillQPtr >= (CPI::OS::int32_t)port->getBufferCount() ) {
    m_FillQPtr = 0;
  }
}
                                                

void  TransferController::freeBuffer(
                                     CPI::DataTransport::Port* port                                                
                                     )
{
  //        We treat the input buffers as a circular queue, so we only need to check
  // the next buffer 
  port->getBuffer(m_EmptyQPtr)->markBufferEmpty();
  m_EmptyQPtr++;
  if ( m_EmptyQPtr >= (CPI::OS::int32_t)port->getBufferCount() ) {
    m_EmptyQPtr = 0;
  }
}



/**********************************
 * This method gets the next available buffer from the specified output port
 *********************************/
Buffer* TransferController::getNextEmptyOutputBuffer( 
                                                     CPI::DataTransport::Port* src_port        
                                                     )
{
  // This default implementation simply finds the first available output buffer
  OutputBuffer* boi=NULL;        
  CPI::OS::uint32_t &n = src_port->getLastBufferTidProcessed();
  OutputBuffer* buffer = src_port->getOutputBuffer(n);
  if (  buffer->isEmpty() && ! buffer->inUse() ) {
    boi = buffer;
  }
  if ( boi ) {
    boi->setInUse( true );
    n = (n+1) % src_port->getBufferCount();
  }
  return boi;
}


/**********************************
 * This method gets the next available buffer from the specified input port
 *********************************/
Buffer* TransferController::getNextFullInputBuffer( 
                                                     CPI::DataTransport::Port* input_port 
                                                     )
{

#ifdef SEQ_RETURN
  // This default implementation simply makes sure that each output buffer
  // gets processed in sequence.

  DataDistributionMetaData::DistributionType d_type = 
    input_port->getCircuit()->getConnection()->getDataDistribution()->getMetaData()->distType;

  InputBuffer** buffers = input_port->getInputBuffers();
  InputBuffer* boi=NULL;

  int &seq = input_port->getLastBufferTidProcessed();

#ifndef NDEBUG
  printf("getNextFullInputBuffer, last seq processed = %d\n", seq );
#endif

  InputBuffer *low_seq = NULL;
  int full_count=0;
  for ( int n=0; n<input_port->getBufferCount(); n++ ) {
    if (  ! buffers[n]->isEmpty() && ! buffers[n]->inUse() ) {
      full_count++;

      // If we have a parellel distribution on the connection, all of the buffers need 
      // to be in order
      if ( d_type == DataDistributionMetaData::sequential ) {

        int inc = input_port->getCircuit()->getInputPortSetCount();
        if ( seq == 0 ) {
          seq = buffers[n]->getMetaData()->sequence;
          boi = buffers[n];
          seq += inc;
          break;
        }
        else if ( buffers[n]->getMetaData()->sequence == seq ) {
          boi = buffers[n];
          seq += inc;
          break;
        }

        // A broadcast may be out of sequence
        else if ( buffers[n]->getMetaData()->broadCast == 1 ) {   
          boi = buffers[n];
          seq += inc;
          break;
        }

      }
      else if ( buffers[n]->getMetaData()->sequence == seq ) {
        boi = buffers[n];
        seq++;
        break;
      }

    }
  }
  if ( boi ) {
    boi->setInUse( true );
  }


  // Check for programming error
  if ( (full_count == input_port->getBufferCount()) && ! boi ) {
#ifndef NDEBUG
    printf("*** INTERNAL ERROR ***, got a full set of input buffers, but cant find expected sequence\n");
    cpiAssert(0)
#endif
      }

  return boi;
#else


#define SIMPLE_AND_FAST
#ifdef SIMPLE_AND_FAST

  InputBuffer* buffer=NULL; 
  int& lo = input_port->getLastBufferOrd();
  int tlo = ((lo+1)%input_port->getBufferCount());
  buffer = input_port->getInputBuffer(tlo);
  if ( ! buffer->isEmpty() && ! buffer->inUse() ) {
    lo = tlo;
    buffer->setInUse( true );
    return buffer;
  }
  return NULL;
        
#else
  // With this pattern, the data buffers are not deterministic, so we will always hand back 
  // the buffer with the lowest sequence
  InputBuffer* buffer; 
  InputBuffer *low_seq = NULL;
        
  for ( CPI::OS::uint32_t n=0; n<input_port->getBufferCount(); n++ ) {
    buffer = input_port->getInputBuffer(n);
    if (  ! buffer->isEmpty() && ! buffer->inUse() ) {
      if ( low_seq ) {
        if ( buffer->getMetaData()->sequence < low_seq->getMetaData()->sequence  ) {
          low_seq = buffer;
        }
        else if ( (buffer->getMetaData()->sequence == low_seq->getMetaData()->sequence) &&
                  (buffer->getMetaData()->partsSequence < low_seq->getMetaData()->partsSequence) ) {
          low_seq = buffer;
        }
      }
      else {
        low_seq = buffer;
      }
    }
                
  }
  if ( low_seq ) {
    low_seq->setInUse( true );
  }

#ifndef NDEBUG
  if ( ! low_seq ) {
    printf("No Input buffers avail\n");
  }
#endif
        
  return low_seq;
#endif

#endif
        
}

/**********************************
 * Init
 *********************************/
void TransferController::init( PortSet* output, PortSet* input, bool whole_ss ) {
  m_output = output;
  m_input = input;
  m_wholeOutputSet = whole_ss;
}



/**********************************
 * This method determines if we can produce from the indicated buffer
 *********************************/
bool TransferController::canBroadcast( 
                                      Buffer* buffer                        // InOut - Buffer to produce from
                                      )
{

  // When s DD = whole only port 0 of the output port set can produce
  if (  m_wholeOutputSet && buffer->getPort()->getRank() != 0 ) {
    return true;
  }

  bool produce = false;

  // We will go to each of our shadows and figure out if they are empty

  // With this pattern, we sequence the output buffers to ensure that the input always
  // has the next buffer of data in one of its buffers, but we dont have to worry about 
  // sequencing through its buffers in order because the inputs have the logic needed
  // to re-sequence there own buffers

  for ( CPI::OS::uint32_t p=0; p<m_input->getBufferCount(); p++ ) {
    for ( CPI::OS::uint32_t n=0; n<m_input->getPortCount(); n++ ) {
      CPI::DataTransport::Port* port = m_input->getPort(n);
      if ( port->getBuffer(p)->isEmpty() ) {
        produce = true;
      }
      else {
        produce = false;
        break;
      }
    }

    // All inputs have a free buffer
    if ( produce ) {
      m_nextTid = p;
#ifndef NDEBUG
      printf("TransferController:: m_nextTid = %d\n", m_nextTid );
#endif
      break;
    }
  }
  return produce;
}




/**********************************
 * This initiates a broadcastdata transfer from the output buffer.
 *********************************/
void TransferController::broadCastOutput( Buffer* b )
{
  OutputBuffer* buffer = static_cast<OutputBuffer*>(b);

#ifndef NDEBUG
  printf("TransferController::broadCastOutput( Buffer* b ), setting EOS !!\n");
#endif

  // In the case of a braodcast, we will copy some of the buffers meta-data
  // attributes to the output control structure
  buffer->getControlBlock()->endOfStream = buffer->getMetaData()->endOfStream;
  buffer->getControlBlock()->endOfWhole = buffer->getMetaData()->endOfWhole;

  // We need to mark the buffer as full
  buffer->markBufferFull();
        
  for ( CPI::OS::uint32_t n=0; n<m_input->getPortCount(); n++ ) {
#ifndef NDEBUG
    printf("TransferController:: 1 m_nextTid = %d\n", m_nextTid );
#endif
    Buffer* tbuf = static_cast<Buffer*>(m_input->getPort(n)->getBuffer(m_nextTid));
    tbuf->markBufferFull();

  }
        
  /*
   *  For this pattern the output port and input port are constants when we look 
   *  up the template that we need to produce.  So, since the output tid is a given,
   *  the only calculation is the input tid that we are going to produce to.
   */
#ifndef NDEBUG
  printf("output port id = %d, buffer id = %d, input id = %d\n", 
         buffer->getPort()->getPortId(), buffer->getTid(), m_nextTid);
  printf("Template address = %p\n", m_templates [buffer->getPort()->getPortId()][buffer->getTid()][0][m_nextTid][1][OUTPUT]);
#endif

  // Start producing, this may be asynchronous
  m_templates [buffer->getPort()->getPortId()][buffer->getTid()][0][m_nextTid][1][OUTPUT]->produce();
        
  // Add the template to our list
  insert_to_list(&buffer->getPendingTxList(), m_templates [buffer->getPort()->getPortId()][buffer->getTid()][0][m_nextTid][1][OUTPUT], 64, 8);
        
}





///
////  Transfer controller for pattern 1 
///
TransferController1::TransferController1( PortSet* output, PortSet* input, bool whole_ss) 
{
  init( output, input, whole_ss);
  m_nextTid = 0;

}

TransferController* TransferController1::createController( 
                                                          PortSet* output, 
                                                          PortSet* input,
                                                          bool whole_output_set)
{
  return new TransferController1( output, input, whole_output_set );
}




/**********************************
 * This method determines if we can produce from the indicated buffer
 *********************************/
bool 
TransferController1::
canProduce( Buffer* buffer )
{

#ifndef NDEBUG
  printf(" In TransferController1::canProduce \n");
#endif

  // When s DD = whole only port 0 of the output port set can produce
  if (  m_wholeOutputSet && buffer->getPort()->getRank() != 0 ) {
    return true;
  }

  // Broadcast is a special case
  if ( buffer->getMetaData()->endOfStream ) {
    return canBroadcast( buffer );
  }

  bool produce = false;

  // We will go to each of our shadows and figure out if they are empty

#ifdef RANDOM_INPUTS
  // With this pattern, we sequence the output buffers to ensure that the input always
  // has the next buffer of data in one of its buffers, but we dont have to worry about 
  // sequencing through its buffers in order because the inputs have the logic needed
  // to re-sequence there own buffers

  for ( CPI::OS::uint32_t p=0; p<m_input->getBufferCount(); p++ ) {
    for ( CPI::OS::uint32_t n=0; n<m_input->getPortCount(); n++ ) {
      CPI::DataTransport::Port* port = m_input->getPort(n);
      if ( port->getBuffer(p)->isEmpty() ) {
        produce = true;
      }
      else {
        produce = false;
        break;
      }
    }

    // All inputs have a free buffer
    if ( produce ) {
      m_nextTid = p;

#ifndef NDEBUG
      printf("TransferController:: m_nextTid = %d\n", m_nextTid );
#endif
      break;
    }
  }
#else


  // We treat the input buffers as a circular queue, so we only need to check
  // the next buffer 
  for ( CPI::OS::uint32_t n=0; n<m_input->getPortCount(); n++ ) {
    CPI::DataTransport::Port* port = m_input->getPort(n);
    if ( port->getBuffer(m_nextTid)->isEmpty() ) {
      produce = true;
    }
    else {
      produce = false;
      break;
    }
  }


#endif


  return produce;

}

void TransferController::modifyOutputOffsets( Buffer* me, Buffer* new_buffer, bool reverse )
{
  int mt = m_nextTid;
  CpiTransferTemplate *temp = 
    m_templates [me->getPort()->getPortId()][me->getTid()][0][mt][0][OUTPUT];

  // If this is already a zero copy from output to the next input we need to deal with that
  if ( temp->m_zCopy ) {
    Buffer *cme =  static_cast<Buffer*>(me);
    Buffer *cnew_buffer =  static_cast<Buffer*>(new_buffer);
    if ( ! reverse ) {
      cme->attachZeroCopy( cnew_buffer );
    }
    else {
      cme->detachZeroCopy();
    }
  }
  else {

    // Note: this needs to me augmented for DRI
    CPI::OS::uint32_t new_offsets[2];
    new_offsets[1] = 0;
    CPI::OS::uint32_t old_offsets[2];
    Buffer* nb;
    Buffer *cnew_buffer =  static_cast<Buffer*>(new_buffer);
    if ( cnew_buffer->m_zeroCopyFromBuffer ) {
      nb = cnew_buffer->m_zeroCopyFromBuffer;
    }
    else {
      nb = cnew_buffer;
    }
    if ( ! reverse ) {
      new_offsets[0] = nb->m_startOffset; 
      temp->modify( new_offsets, old_offsets );
    }
    else {
      new_offsets[0] = me->m_startOffset; 
      temp->modify( new_offsets, old_offsets );
    }
  }
}


/**********************************
 * Implementation specific transfer from the output side.
 **********************************/
int TransferController1::produce( Buffer* b, bool bcast )
{
  Buffer* buffer = static_cast<Buffer*>(b);

  int bcast_idx = bcast ? 1 : 0;

  if ( bcast_idx == 1 ) {
#ifdef DEBUG_L2
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

#ifdef DEBUG_L2
    printf("My rank != 0 so i am not producing !!! \n");
#endif

    // We need to mark the local buffer as free
    buffer->markBufferEmpty();

    // Next input buffer 
    int tmp = ((m_nextTid++)%m_input->getBufferCount()) - 1;
    m_nextTid = tmp;
    if ( m_nextTid < 0 ) {
      m_nextTid = 0;
    }
#ifndef NDEBUG
    printf("TransferController:: m_nextTid = %d\n", m_nextTid );
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

  // We need to mark the buffer as full
  buffer->markBufferFull();

  for ( CPI::OS::uint32_t n=0; n<m_input->getPortCount(); n++ ) {

#ifndef NDEBUG
    printf("TransferController:: m_nextTid = %d\n", m_nextTid );
#endif
    Buffer* tbuf = static_cast<Buffer*>(m_input->getPort(n)->getBuffer(m_nextTid) );
    tbuf->markBufferFull();

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

void TransferController::freeAllBuffersLocal( CPI::DataTransport::Port* port  )
{
  cpiAssert( !port->isOutput() );
  m_EmptyQPtr = 0;
  for ( unsigned int n=0; n<port->getBufferCount(); n++ ) {
    port->getBuffer(n)->markBufferEmpty();
  }
}


void TransferController::consumeAllBuffersLocal( CPI::DataTransport::Port* port )
{
  cpiAssert( port->isOutput() );
  for ( unsigned int n=0; n<port->getBufferCount(); n++ ) {
    port->getBuffer(n)->markBufferEmpty();
  }
}


    
/**********************************
 * This marks the input buffer as "Empty" and informs all interested outputs that
 * the input is now available.
 *********************************/
Buffer* TransferController1::consume( Buffer* input )
{
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





///
////  Transfer controller for pattern 2
///

TransferController2::TransferController2( PortSet* output, PortSet* input, bool whole_ss )
{
  init( output, input, whole_ss);
  m_nextTid = 0;
}



TransferController* TransferController2::createController( 
                                                          PortSet* output, 
                                                          PortSet* input,
                                                          bool whole_output_set)
{

  TransferController* tc=NULL;

  // We handle sequential input distribution, but we specialize on the distribution type
  switch ( input->getDataDistribution()->getMetaData()->distSubType ) {
  case DataDistributionMetaData::round_robin:
    break;
                
  case DataDistributionMetaData::random_even:
    break;

  case DataDistributionMetaData::random_statistical:
    break;
                
  case DataDistributionMetaData::first_available:
    break;
                
  case DataDistributionMetaData::least_busy:
    tc = new TransferController2(output,input,whole_output_set);
    break;
  }

  cpiAssert( tc );

  return tc;
}



/**********************************
 * This method determines if we can produce from the indicated buffer
 *********************************/
bool TransferController2::canProduce( 
                                     Buffer* b                                // InOut - Buffer to produce from
                                     )
{

  Buffer* buffer = static_cast<Buffer*>(b);

  // With this pattern, only port 0 of the output port set can produce
  if ( m_wholeOutputSet && buffer->getPort()->getRank() != 0 ) {
    return true;
  }

  // Broadcast is a special case
  if ( buffer->getMetaData()->endOfStream ) {
#ifdef DEBUG_L2
    printf("*** Testing canproduce via broacast !!\n");
#endif
    return canBroadcast( b );
  }

  // Make sure we have the barrier token
  OutputBuffer *s_buf = static_cast<OutputBuffer*>(buffer);
  if ( ! haveOutputBarrierToken( s_buf ) ) {
    return false;
  }

  m_inputPort = NULL;

  // We will go to each of our shadows and figure out if they are empty

  // With this pattern, we sequence the output buffers to ensure that the input always
  // has the next buffer of data in one of its buffers, but we dont have to worry about 
  // sequencing through its buffers in order because the inputs have the logic needed
  // to re-sequence there own buffers
        
  for ( CPI::OS::uint32_t n=0; n<m_input->getPortCount(); n++ ) {
    CPI::DataTransport::Port* port = m_input->getPort(n);
    for ( CPI::OS::uint32_t p=0; p<m_input->getBufferCount(); p++ ) {

#ifdef DEBUG_L2
      printf("canProduce:: busy factor for port %d = %d\n", n, port->getBusyFactor() ) ;
#endif
      if ( port->getBuffer(p)->isEmpty() ) {
        if ( m_inputPort ) {
          if ( port->getBusyFactor() < m_inputPort->getBusyFactor() ) {
            m_nextTid = p;

#ifndef NDEBUG
            printf("TransferController:: m_nextTid = %d\n", m_nextTid );
#endif
            m_inputPort = port;
          }
        }
        else {
          m_nextTid = p;
#ifndef NDEBUG
          printf("TransferController:: m_nextTid = %d\n", m_nextTid );
#endif
          m_inputPort = port;
        }
        break;
      }
    }
  }

#ifdef DEBUG_L2
  if ( m_inputPort ) 
    printf("Selected Port %d with BF = %d\n", m_inputPort->getPortId(), m_inputPort->getBusyFactor() );
#endif

  return m_inputPort?true:false;

}


/**********************************
 * Implementation specific transfer from the output side.
 **********************************/
int TransferController2::produce( Buffer* b, bool bcast )
{
  OutputBuffer* buffer = static_cast<OutputBuffer*>(b);

  int bcast_idx = bcast ? 1 : 0;
  if ( bcast_idx == 1 ) {

#ifdef DEBUG_L2
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

#ifdef DEBUG_L2
    printf("My rank != 0 so i am not producing !!! \n");
#endif
    // We need to mark the local buffer as free
    buffer->markBufferEmpty();

    return 0;
  }


  // Broadcast if requested
  if ( bcast_idx ) {
    broadCastOutput( buffer );
    return 0;
  }

  // We need to mark our buffer as full
  buffer->markBufferFull();

  // Here we will mark the lstate of the input port as full

#ifndef NDEBUG
  printf("TransferController:: m_nextTid = %d\n", m_nextTid );
#endif

  Buffer* tbuf = static_cast<Buffer*>(m_inputPort->getBuffer(m_nextTid));
  tbuf->markBufferFull();

  // We need to increment the token value to enable the next output port
  buffer->getControlBlock()->sequentialControlToken = 
    (buffer->getControlBlock()->sequentialControlToken + 1) %
    buffer->getPort()->getPortSet()->getPortCount();

  /*
   *  For this pattern the output port and input port are constants when we look 
   *  up the template that we need to produce.  So, since the output tid is a given,
   *  the only calculation is the input tid that we are going to produce to.
   */
#ifdef DEBUG_L2
  printf("output port id = %d, buffer id = %d, input id = %d\n", 
         buffer->getPort()->getPortId(), buffer->getTid(), m_nextTid);
  printf("Template address = 0x%x\n", m_templates [buffer->getPort()->getPortId()][buffer->getTid()][m_inputPort->getPortId()][m_nextTid][bcast_idx][OUTPUT]);
#endif

  // Start producing, this may be asynchronous
  m_templates [buffer->getPort()->getPortId()][buffer->getTid()][m_inputPort->getPortId()][m_nextTid][bcast_idx][OUTPUT]->produce();

  // Add the template to our list
  insert_to_list(&buffer->getPendingTxList(), m_templates [buffer->getPort()->getPortId()][buffer->getTid()][m_inputPort->getPortId()][m_nextTid][bcast_idx][OUTPUT], 64, 8);

  return 0;
}

    
/**********************************
 * This marks the input buffer as "Empty" and informs all interested outputs that
 * the input is now available.
 *********************************/
Buffer* TransferController2::consume( Buffer* input )
{
  Buffer* buffer = static_cast<Buffer*>(input);

  // We need to mark the local buffer as free
  buffer->markBufferEmpty();

#ifdef DTI_PORT_COMPLETE
  buffer->setBusyFactor( buffer->getPort()->getCircuit()->getRelativeLoadFactor() );
#endif


#ifdef DEBUG_L2
  printf("Set load factor to %d\n", buffer->getState()->pad);
  printf("Consuming [0][0][%d][%d][0][1]\n",input->getPort()->getPortId(),input->getTid());
#endif

  // Tell everyone that we are empty
  return m_templates [0][0][input->getPort()->getPortId()][input->getTid()][0][INPUT]->consume();
}


/**********************************
 * This method gets the next available buffer from the specified input port
 *********************************/
Buffer* TransferController2::getNextFullInputBuffer( 
                                                      CPI::DataTransport::Port* input_port   
                                                      )
{
  // With this pattern, the data buffers are not deterministic, so we will always hand back 
  // the buffer with the lowest sequence
  InputBuffer* buffer; 
  InputBuffer *low_seq = NULL;
        
  for ( CPI::OS::uint32_t n=0; n<input_port->getBufferCount(); n++ ) {
    buffer = input_port->getInputBuffer(n);
    if (  ! buffer->isEmpty() && ! buffer->inUse() ) {
      if ( low_seq ) {
        if ( buffer->getMetaData()->sequence < low_seq->getMetaData()->sequence  ) {
          low_seq = buffer;
        }
      }
      else {
        low_seq = buffer;
      }
    }
                
  }
  if ( low_seq ) {
    low_seq->setInUse( true );
  }

#ifdef DEBUG_L2
  if ( ! low_seq ) {
    printf("No Input buffers avail\n");
  }
#endif

  return low_seq;
}


///
////  Transfer controller for pattern 3
///

/**********************************
 * This method determines if we have the output barrier token
 *********************************/
bool TransferController3::haveOutputBarrierToken( OutputBuffer* src_buf )
{
  bool tok=false;
  int our_port_id = src_buf->getPort()->getPortId();

  if ( src_buf->getControlBlock()->sequentialControlToken == our_port_id ) {
                
    tok = true;
                
#ifdef DEBUG_L2
    printf("Checking barrier token for port %d, token = %d, returning %d\n", our_port_id ,
           src_buf->getControlBlock()->sequentialControlToken, tok);
#endif
                
  }

#ifdef DEBUG_L2
  printf("Checking barrier token for port %d, token = %d, returning %d\n", our_port_id ,
         src_buf->getControlBlock()->sequentialControlToken, tok);
#endif

  return tok;
}



///
////  Transfer controller for pattern 4
///

/**********************************
 * This method determines if we can produce from the indicated buffer
 *********************************/
bool TransferController4::canProduce( Buffer* b)
{
  Buffer* buffer = static_cast<Buffer*>(b);
  // When s DD = whole only port 0 of the output port set can produce
  if (  m_wholeOutputSet && (buffer->getPort()->getRank() != 0) ) {
    return true;
  }
  // If there are no pending transfers, controller 1 will work.
  bool cp = TransferController1::canProduce(b);
  return cp;
}


/**********************************
 * Implementation specific transfer from the output side.
 **********************************/

TransferController4::TransferController4( PortSet* output, PortSet* input, bool whole_ss )
{
  init( output, input, whole_ss);
  m_nextTid = 0;
}


TransferController* TransferController4::createController( 
                                                          PortSet* output, 
                                                          PortSet* input,
                                                          bool whole_output_set)
{
  return new TransferController4( output, input, whole_output_set );
}


int TransferController4::produce( Buffer* b, bool bcast )
{
  Buffer* buffer = static_cast<Buffer*>(b);


  // Determine if this is a gated transfer, we have to note that any controller
  // can add a transfer template to the output buffer and we can only handle ours
  CPI::OS::int32_t n_pending = buffer->getPendingTransferCount();
  if ( ! n_pending ) {
    return TransferController1::produce(b,bcast);
  }

  // We have some pending transfers on this buffer
  List& l_pending = buffer->getPendingTxList();

#ifdef DEBUG_L2
  printf("pending transfers on buffer = %d\n", n_pending );
#endif

  int total=0;
  int n=0;
  for (n=0; n<n_pending; n++ ) {
                
    CpiTransferTemplate* temp = static_cast<CpiTransferTemplate*>(get_entry(&l_pending, n));
                
    // If this is one ouf ours, produce and then break, we only get to produce once each time
    // "canProduce" is called.
    if ( temp && temp->getTypeId() == 4 ) {

      // This is effectivly a broadcst to all port buffers, so we need to mark them as full
      for ( CPI::OS::uint32_t n=0; n<m_input->getPortCount(); n++ ) {
        Buffer* tbuf = static_cast<Buffer*>(m_input->getPort(n)->getBuffer(m_nextTid));
        tbuf->markBufferFull();
      }

      total += temp->produceGated( 0, m_nextTid );
      break;
    }
  }

#ifdef DEBUG_L2
  printf("TransferController4::produce returning %d\n", total );
#endif

  return total;
}



/**********************************
 * This method gets the next available buffer from the specified input port
 *********************************/
Buffer* TransferController4::getNextFullInputBuffer( 
                                                      CPI::DataTransport::Port* input_port 
                                                      )
{

#ifdef DEBUG_L2
  printf("In TransferController4::getNextFullInputBuffer\n");
#endif

  // With this pattern, the data buffers are not deterministic, so we will always hand back 
  // the buffer with the lowest output sequence AND the lowest whole sequence
  InputBuffer* buffer;
  InputBuffer *low_seq = NULL;
        
  for ( CPI::OS::uint32_t n=0; n<input_port->getBufferCount(); n++ ) {
    buffer = input_port->getInputBuffer(n);
    if (  ! buffer->isEmpty() && ! buffer->inUse() ) {
      if ( low_seq ) {
        if ( buffer->getMetaData()->sequence < low_seq->getMetaData()->sequence) {
          low_seq = buffer;
        }
        else if ( (buffer->getMetaData()->sequence == low_seq->getMetaData()->sequence) &&
                  (buffer->getMetaData()->partsSequence < low_seq->getMetaData()->partsSequence) ) {
          low_seq = buffer;
        }
      }
      else {
        low_seq = buffer;
      }
    }
  }
  if ( low_seq ) {
    low_seq->setInUse( true );
  }

#ifdef DEBUG_L2
  if ( ! low_seq ) {
    printf("No Input buffers avail\n");
  }
#endif
        
  return low_seq;

}







