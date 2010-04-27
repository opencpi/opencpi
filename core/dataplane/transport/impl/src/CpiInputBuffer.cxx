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
 *   This file contains the implementation for the CPI input buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <CpiInputBuffer.h>
#include <CpiPort.h>
#include <CpiCircuit.h>
#include <CpiOutputBuffer.h>
#include <CpiOsAssert.h>
#include <CpiUtilAutoMutex.h>
#include <CpiTransferController.h>
#include <CpiPullDataDriver.h>

using namespace CPI::DataTransport;
using namespace DataTransfer;
using namespace CPI::OS;

/**********************************
 * Constructors
 *********************************/
InputBuffer::InputBuffer( CPI::DataTransport::Port* port, CPI::OS::uint32_t tid )
  : Buffer( port, tid ),m_feedbackDesc(port->getMetaData()->m_externPortDependencyData),
    m_useEmptyFlagForFlowControl(true),m_produced(true)
{
  m_feedbackDesc.type = CPI::RDT::ProducerDescT;
  m_bVaddr = 0;
  m_bsVaddr = 0;
  m_bmdVaddr = 0;
  memset(m_rssVaddr,0,sizeof(void*)*MAX_PORT_COUNT);

  // update our offsets
  update(0);
}


/**********************************
 * Get/Set the number of bytes transfered
 *********************************/
CPI::OS::uint32_t InputBuffer::getNumberOfBytesTransfered()
{
  return (CPI::OS::uint32_t)  N_BYTES_TRANSFERED(  getMetaDataByIndex(0)->cpiMetaDataWord  );
}



/**********************************
 * Update offsets
 *********************************/
void InputBuffer::update(bool critical)
{

  CPI::OS::uint32_t n, tid;

#ifndef NDEBUG
  printf("InputBuffer:update:start\n");
#endif

  tid = getTid();

  // We need to know the number of ports in the output port set;
  m_outputPortCount = getPort()->getCircuit()->getOutputPortSet()->getPortCount();
 
  struct PortMetaData::InputPortBufferControlMap *input_offsets = 
    &getPort()->getMetaData()->m_bufferData[tid].inputOffsets;

  // First we will map our buffer
  if ( !this->m_port->isShadow() && !m_bVaddr && input_offsets->bufferOffset ) {

#ifndef NDEBUG
    printf("InputBuffer:update:mapping buffer offset\n");
#endif

    m_bVaddr = getPort()->getLocalShemServices()->map
      (input_offsets->bufferOffset, 
       input_offsets->bufferSize );
    m_startOffset = input_offsets->bufferOffset;
		  
    m_length = input_offsets->bufferSize;
    memset(m_bVaddr, 0, input_offsets->bufferSize);
    m_buffer = m_baseAddress = m_bVaddr;

#ifdef DEBUG_L2
    printf("Input buffer addr = 0x%x, size = %d\n", m_buffer, input_offsets->bufferSize );
#endif

  }

  // map our states
  if ( !this->m_port->isShadow() && !m_bsVaddr && input_offsets->localStateOffset ) {

#ifndef NDEBUG
    printf("InputBuffer:update: mapping states\n");
#endif

    m_bsVaddr = getPort()->getLocalShemServices()->map
      (input_offsets->localStateOffset, 
       sizeof(BufferState)*MAX_PORT_COUNT*2);

    m_state = static_cast<volatile BufferState (*)[MAX_PORT_COUNT]>(m_bsVaddr);
    for ( unsigned int y=0; y<MAX_PORT_COUNT; y++ ) {
      m_state[0][y].bufferFull = DataTransfer::BufferEmptyFlag;
    }

    // This separates the flag that we get from the output and the flag that we send to
    // our shadow buffer. 
    if ( m_useEmptyFlagForFlowControl ) {
      for ( unsigned int y=MAX_PORT_COUNT; y<MAX_PORT_COUNT*2; y++ ) {
	m_state[0][y].bufferFull = DataTransfer::BufferEmptyFlag;
      }
    }
    else {
#ifdef USE_TID_FOR_DB
      for (unsigned  int y=MAX_PORT_COUNT; y<MAX_PORT_COUNT*2; y++ ) {
	m_state[0][y].bufferFull = m_tid;
      }
#else 
      for (unsigned  int y=MAX_PORT_COUNT; y<MAX_PORT_COUNT*2; y++ ) {
	m_state[0][y].bufferFull = m_port->getMetaData()->m_externPortDependencyData.desc.emptyFlagValue;
      }
#endif
    }

	  
  }

  // Now map the meta-data
  if ( !this->m_port->isShadow() && !m_bmdVaddr && input_offsets->metaDataOffset ) {

#ifndef NDEBUG
    printf("InputBuffer:update: mapping meta data\n");
#endif

    m_bmdVaddr = getPort()->getLocalShemServices()->map
      (input_offsets->metaDataOffset, 
       sizeof(BufferMetaData)*MAX_PORT_COUNT);

    memset(m_bmdVaddr,0,sizeof(BufferMetaData)*MAX_PORT_COUNT);
    m_sbMd = static_cast<volatile BufferMetaData (*)[MAX_PORT_COUNT]>(m_bmdVaddr);
  }
  

  // These are our shadow buffers remote states
  for ( n=0; n<m_outputPortCount; n++ ) {

    PortSet* s_ps = static_cast<PortSet*>(getPort()->getCircuit()->getOutputPortSet());
    CPI::DataTransport::Port* shadow_port = static_cast<CPI::DataTransport::Port*>(s_ps->getPortFromIndex(n));

    // At this point our shadows may not be completely defined, so we may have to delay
    if ( !shadow_port || !shadow_port->getRealShemServices() ) {
      continue;
    }

    int idx = shadow_port->getRealShemServices()->getEndPoint()->mailbox;

    // A shadow for a output may not exist if they are co-located
    if ( !m_rssVaddr[idx] && input_offsets->myShadowsRemoteStateOffsets[idx] ) {

#ifdef DEBUG_L2
      printf("&&&& mapping shadow offset to 0x%x\n", 
	     input_offsets->myShadowsRemoteStateOffsets[idx]);
#endif

#ifndef NDEBUG
      printf("InputBuffer:update: mapping shadows\n");
#endif

      m_rssVaddr[idx] = shadow_port->getRealShemServices()->map
	(input_offsets->myShadowsRemoteStateOffsets[idx], 
	 sizeof(BufferState));

      // Now format our descriptor
      this->m_feedbackDesc.type = CPI::RDT::ConsumerFlowControlDescT;
      this->m_feedbackDesc.desc.emptyFlagBaseAddr = 
	input_offsets->myShadowsRemoteStateOffsets[idx];

      // Our flag is 4 bytes
      this->m_feedbackDesc.desc.emptyFlagPitch = sizeof(BufferState);
      this->m_feedbackDesc.desc.emptyFlagSize = sizeof(BufferState);
      this->m_feedbackDesc.desc.emptyFlagValue = 0x1000; 
        
#ifndef NDEBUG		
      printf("Requested Emptyflag port value = 0x%llx\n", 
	     (long long)this->m_feedbackDesc.desc.emptyFlagValue);
#endif
		

#ifndef NDEBUG
      printf("InputBuffer:update: map returned %p\n", m_rssVaddr[idx]);
#endif


      m_myShadowsRemoteStates[idx] = 
	static_cast<volatile BufferState*>(m_rssVaddr[idx]);


      m_myShadowsRemoteStates[idx]->bufferFull = -1;

#ifdef DEBUG_L2
      printf("Mapped shadow buffer for idx %d = 0x%x\n", idx, m_myShadowsRemoteStates[idx] );
#endif

    }

  }

#ifndef NDEBUG
  printf("InputBuffer:update:finish\n");
#endif

}


void InputBuffer::useTidForFlowControl( bool ut )
{
  m_useEmptyFlagForFlowControl = ut;
  if ( m_useEmptyFlagForFlowControl ) {
    for ( unsigned int y=MAX_PORT_COUNT; y<MAX_PORT_COUNT*2; y++ ) {
      m_state[0][y].bufferFull = DataTransfer::BufferEmptyFlag;
    }
  }
  else {
#ifdef USE_TID_FOR_DB
    for ( int y=MAX_PORT_COUNT; y<MAX_PORT_COUNT*2; y++ ) {
      m_state[0][y].bufferFull = m_tid | ((CPI::OS::uint64_t)m_tid<<32);
    }
#else
    for ( unsigned int y=MAX_PORT_COUNT; y<MAX_PORT_COUNT*2; y++ ) {
      m_state[0][y].bufferFull = m_port->getMetaData()->m_externPortDependencyData.desc.emptyFlagValue;
    }
#endif
  }
}


void InputBuffer::produce( Buffer* src_buf )
{
  if ( m_noTransfer ) return;
  this->getPort()->getPortSet()->getTxController()->produce( src_buf );
  m_produced = true;  
}

/**********************************
 * Sets the buffers busy factor
 **********************************/
void InputBuffer::setBusyFactor( CPI::OS::uint32_t bf )
{
#ifdef LEAST_BUSY
  m_state[0][m_pid].pad = bf;
#endif
}


/**********************************
 * Get buffer state
 *********************************/
volatile BufferState* InputBuffer::getState()
{
  if ( !this->getPort()->isShadow() && m_zeroCopyFromBuffer ) {
    return m_zeroCopyFromBuffer->getState();
  }

#ifdef DEBUG_L2
  printf("In input get state, state = 0x%x\n", m_state[0]);
#endif


  if ( ! this->getPort()->isShadow() ) {

#ifdef DEBUG_L2
    printf("Getting load factor of %d\n", m_state[0]->pad );
#endif
    m_tState.bufferFull = m_state[0][m_pid].bufferFull;

#ifdef LEAST_BUSY
    m_tState.pad = m_state[0][m_pid].pad;
#endif


#define NEED_FULL_STATE_CONTROL
#ifdef NEED_FULL_STATE_CONTROL

    // This method belongs in the controller !!!!!


    // If we are a shadow port (a local state of a real remote port), then we only
    // use m_state[0] to determine if the remote buffer is free.  Otherwise, we need
    // to look at all of the other states to determine if all outputs have written to us.
    for ( CPI::OS::uint32_t n=0; n<MAX_PORT_COUNT; n++ ) {
      if ( (m_state[0][n].bufferFull & DataTransfer::BufferEmptyFlag) != DataTransfer::BufferEmptyFlag ) {
	m_tState.bufferFull = m_state[0][n].bufferFull;
       	break;
      }
    }
#endif


  }
  else {

    int mb = getPort()->getMailbox();

    m_tState.bufferFull = m_myShadowsRemoteStates[mb]->bufferFull;

#ifdef LEAST_BUSY
    m_tState.pad = m_myShadowsRemoteStates[mb]->pad;
#endif

#ifdef DEBUG_L2
    printf("Load factor in shadow pad = %d\n",  m_tState.pad );
#endif

  }

  return static_cast<volatile BufferState*>(&m_tState);
}


/**********************************
 * Marks buffer as full
 *********************************/
void InputBuffer::markBufferFull()
{
  m_produced = true;

#ifdef USE_TID_FOR_DB
  if ( ! this->getPort()->isShadow() ) {
    m_state[0][0].bufferFull = m_tid;
  }
  else {
    m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferFull = m_tid;
  }
#else
  if ( ! this->getPort()->isShadow() ) {
    m_state[0][0].bufferFull = 1;
  }
  else {
    m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferFull = 1;
  }
#endif
	
}


/**********************************
 * Marks buffer as empty
 *********************************/
void InputBuffer::markBufferEmpty()
{
  if ( ! this->getPort()->isShadow() ) {
    for ( unsigned int n=0; n<MAX_PORT_COUNT; n++ ) {
      m_state[0][n].bufferFull =  DataTransfer::BufferEmptyFlag;
    }
  }
  else {
    m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferFull =  DataTransfer::BufferEmptyFlag;
    volatile BufferState* state = this->getState();
    state->bufferFull = DataTransfer::BufferEmptyFlag;
  }	
}


/**********************************
 * Get buffer state
 *********************************/
volatile BufferState* InputBuffer::getState(CPI::OS::uint32_t rank)
{
  return &m_state[0][rank];
}


/**********************************
 * Is buffer End Of Whole EOS
 *********************************/
bool InputBuffer::isShadow()
{
  return reinterpret_cast<CPI::DataTransport::Port*>(getPort())->isShadow();
}



/**********************************
 * Is this buffer empty
 *********************************/
bool InputBuffer::isEmpty()
{

  volatile BufferState* state = this->getState();
  if ( Buffer::isEmpty() == false ) {
    return false;
  }

  if ( getPort()->getPullDriver() ) {
     if ( isShadow() ) {
       // m_produced is our barrier sync.  
       if ( m_produced ) {

	 CPI::OS::uint64_t mdata;
	 bool empty = getPort()->getPullDriver()->checkBufferEmpty( (CPI::OS::uint8_t*)getBuffer(),getLength(),mdata);
	 
       //	 bool empty = getPort()->getPullDriver()->checkBufferEmpty(this);	 
	 if ( empty ) {
	   m_produced = false;
	   return true;
	 }
       }       
       return false;
     }
     else {  // We are a real input
       bool empty = 
	 ((state->bufferFull & DataTransfer::BufferEmptyFlag) == DataTransfer::BufferEmptyFlag) ? true : false;       
       if ( empty ) {

	 CPI::OS::uint64_t mdata;
	 empty = getPort()->getPullDriver()->checkBufferEmpty((CPI::OS::uint8_t*)getBuffer(),getLength(),mdata);
	 if ( !empty ) {
	   setInUse(true);
	   markBufferFull();
	   getMetaData()->cpiMetaDataWord = mdata;
	   setInUse(false);	    
	 }
       }
       return empty;
     }
   }

#ifdef DEBUG_L2
  printf("Input buffer state = %d\n", m_state[0]->bufferFull);
#endif

#ifdef LEAST_BUSY
  CPI::DataTransport::Port* sp = static_cast<CPI::DataTransport::Port*>(this->getPort());
  if ( state->pad != 0) {
    sp->setBusyFactor( state->pad );
  }
#endif

  bool empty = ((state->bufferFull & DataTransfer::BufferEmptyFlag) == DataTransfer::BufferEmptyFlag )  ? true : false;

#ifndef NDEBUG
  //  printf("TB(%p) port = %p empty = %d\n", this, getPort(), empty );
#endif

  return empty;
}




/**********************************
 * Get the offset to this ports meta-data
 **********************************/
volatile BufferMetaData* InputBuffer::getMetaData()
{
  if ( m_zeroCopyFromBuffer ) {
    return m_zeroCopyFromBuffer->getMetaData();
  }

  // This returns the first MD that has been written
  PortSet* s_port = static_cast<CPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  for ( CPI::OS::uint32_t n=0; n<m_outputPortCount; n++ ) {
    int id = s_port->getPortFromIndex(n)->getPortId();
    if ( (m_state[0][id].bufferFull & DataTransfer::BufferEmptyFlag) != DataTransfer::BufferEmptyFlag ) {
      return &m_sbMd[0][id];
    }
  }
  return &m_sbMd[0][m_pid];

}


/**********************************
 * Get number of outputs that have written to this buffer
 *********************************/
CPI::OS::uint32_t InputBuffer::getNumOutputsThatHaveProduced()
{
  int count=0;
  PortSet* s_port = static_cast<CPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  for ( CPI::OS::uint32_t n=0; n<m_outputPortCount; n++ ) {
    int id = s_port->getPortFromIndex(n)->getPortId();
    if ( (m_state[0][id].bufferFull & DataTransfer::BufferEmptyFlag) != DataTransfer::BufferEmptyFlag  ) {
      count++;
    }
  }
  return count;
}



/**********************************
 * Get the Output produced meta data by index
 *********************************/
volatile BufferMetaData* InputBuffer::getMetaDataByIndex( CPI::OS::uint32_t idx )
{
  PortSet* s_port = static_cast<CPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  int id = s_port->getPortFromIndex(idx)->getPortId();
  return &m_sbMd[0][id];
}




      
/**********************************
 * Destructor
 *********************************/
InputBuffer::~InputBuffer()
{


}


