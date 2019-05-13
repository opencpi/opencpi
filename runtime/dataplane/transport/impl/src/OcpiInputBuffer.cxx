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
 * Abstract:
 *   This file contains the implementation for the OCPI input buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <OcpiInputBuffer.h>
#include <OcpiPort.h>
#include <OcpiCircuit.h>
#include <OcpiOutputBuffer.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiTransferController.h>
#include <OcpiPullDataDriver.h>
#include <OcpiTimeEmitCategories.h>
#include "XferEndPoint.h"

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace OCPI::OS;

/**********************************
 * Constructors
 *********************************/
InputBuffer::InputBuffer( OCPI::DataTransport::Port* port, OCPI::OS::uint32_t tid )
  : Buffer( port, tid ),m_feedbackDesc(port->getMetaData()->m_externPortDependencyData),
    m_produced(true)
{
  m_feedbackDesc.type = OCPI::RDT::ProducerDescT;
  m_bVaddr = 0;
  m_bsVaddr = 0;
  m_bmdVaddr = 0;
  memset(m_rssVaddr,0,sizeof(void*)*MAX_PCONTRIBS);

  // update our offsets
  update(0);
}


/**********************************
 * Get/Set the number of bytes transfered
 *********************************/
OCPI::OS::uint32_t InputBuffer::getNumberOfBytesTransfered()
{
  return (OCPI::OS::uint32_t) getMetaDataByIndex(0)->ocpiMetaDataWord.length;
}



/**********************************
 * Update offsets
 *********************************/
void InputBuffer::update(bool critical)
{
  ( void ) critical;
  OCPI::OS::uint32_t n, tid;

  ocpiDebug("InputBuffer:update:start");

  tid = getTid();

  // We need to know the number of ports in the output port set;
  m_outputPortCount = getPort()->getCircuit()->getOutputPortSet()->getPortCount();
 
  struct PortMetaData::InputPortBufferControlMap *input_offsets = 
    &getPort()->getMetaData()->m_bufferData[tid].inputOffsets;

  // First we will map our buffer
  if ( !this->m_port->isShadow() && !m_bVaddr && input_offsets->bufferOffset ) {

    ocpiDebug("InputBuffer:update:mapping buffer offset");

    m_bVaddr = getPort()->getLocalShemServices()->mapRx
      (input_offsets->bufferOffset, 
       input_offsets->bufferSize );
    m_startOffset = input_offsets->bufferOffset;
                  
    m_length = input_offsets->bufferSize;
    memset(m_bVaddr, 0, input_offsets->bufferSize);
    m_buffer = /*m_baseAddress =*/ m_bVaddr;

#ifdef DEBUG_L2
    ocpiDebug("Input buffer addr = 0x%x, size = %d", m_buffer, input_offsets->bufferSize );
#endif

  }

  // map our states
  if ( !this->m_port->isShadow() && !m_bsVaddr && input_offsets->localStateOffset ) {

    ocpiDebug("InputBuffer %p:update: mapping states", this);

    m_bsVaddr = getPort()->getLocalShemServices()->mapRx
      (input_offsets->localStateOffset, 
       sizeof(BufferState)*MAX_PCONTRIBS*2);

    m_state = static_cast<volatile BufferState (*)[MAX_PCONTRIBS]>(m_bsVaddr);
    // These FULL flags will be set by the output side to FULL
    for ( unsigned int y=0; y<MAX_PCONTRIBS; y++ ) {
      m_state[0][y].bufferIsFull = FF_EMPTY_VALUE;
    }

    // These EMPTY flags will be set by the input side to EMPTY and sent to the shadows
    // This separates the flag that we get from the output and the flag that we send to
    // our shadow buffer. 
    for ( unsigned int y=MAX_PCONTRIBS; y<MAX_PCONTRIBS*2; y++ ) {
      m_state[0][y].bufferIsEmpty = EF_EMPTY_VALUE;
    }


  }

  // Now map the meta-data
  if ( !this->m_port->isShadow() && !m_bmdVaddr && input_offsets->metaDataOffset ) {

    ocpiDebug("InputBuffer:update: mapping meta data");

    m_bmdVaddr = getPort()->getLocalShemServices()->mapRx
      (input_offsets->metaDataOffset, 
       sizeof(BufferMetaData)*MAX_PCONTRIBS);

    memset(m_bmdVaddr,0,sizeof(BufferMetaData)*MAX_PCONTRIBS);
    m_sbMd = static_cast<volatile BufferMetaData (*)[MAX_PCONTRIBS]>(m_bmdVaddr);
  }
  

  // These are our shadow buffers remote states
  for ( n=0; n<m_outputPortCount; n++ ) {

    PortSet* s_ps = static_cast<PortSet*>(getPort()->getCircuit()->getOutputPortSet());
    OCPI::DataTransport::Port* shadow_port = static_cast<OCPI::DataTransport::Port*>(s_ps->getPortFromIndex(n));

    // At this point our shadows may not be completely defined, so we may have to delay
    if ( !shadow_port || !shadow_port->getRealShemServices() ) {
      continue;
    }

    int idx = shadow_port->getRealShemServices()->endPoint().mailBox();

    // A shadow for a output may not exist if they are co-located
    if ( !m_rssVaddr[idx] && input_offsets->myShadowsRemoteStateOffsets[idx] ) {

#ifdef DEBUG_L2
      ocpiDebug("&&&& mapping shadow offset to 0x%x", 
             input_offsets->myShadowsRemoteStateOffsets[idx]);
#endif

      ocpiDebug("InputBuffer:update: mapping shadows");

      m_rssVaddr[idx] = shadow_port->getRealShemServices()->mapRx
        (input_offsets->myShadowsRemoteStateOffsets[idx], 
         sizeof(BufferState));

      // Now format our descriptor
      this->m_feedbackDesc.type = OCPI::RDT::ConsumerFlowControlDescT;
      this->m_feedbackDesc.desc.emptyFlagBaseAddr = 
        input_offsets->myShadowsRemoteStateOffsets[idx];

      // Our flag is 4 bytes
      this->m_feedbackDesc.desc.emptyFlagPitch = sizeof(BufferState);
      this->m_feedbackDesc.desc.emptyFlagSize = sizeof(BufferState);
      this->m_feedbackDesc.desc.emptyFlagValue = EF_EMPTY_VALUE; // 0x1000; 
        
      ocpiDebug("Requested Emptyflag port value = 0x%llx", 
             (long long)this->m_feedbackDesc.desc.emptyFlagValue);
      ocpiDebug("InputBuffer:update: map returned %p", m_rssVaddr[idx]);


      ocpiAssert(getPort()->isShadow());
      m_myShadowsRemoteStates[idx] = 
        static_cast<volatile BufferState*>(m_rssVaddr[idx]);


      // Initially empty
      m_myShadowsRemoteStates[idx]->bufferIsEmpty = EF_EMPTY_VALUE;

#ifdef DEBUG_L2
      ocpiDebug("Mapped shadow buffer for idx %d = 0x%x", idx, m_myShadowsRemoteStates[idx] );
#endif

    }

  }
  ocpiDebug("InputBuffer:update:finish");
}


void InputBuffer::useTidForFlowControl( bool )
{
  for ( unsigned int y=MAX_PCONTRIBS; y<MAX_PCONTRIBS*2; y++ ) {
    m_state[0][y].bufferIsEmpty = EF_EMPTY_VALUE;
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
void InputBuffer::setBusyFactor( OCPI::OS::uint32_t bf )
{
#ifdef LEAST_BUSY
  m_state[0][m_pid].pad = bf;
#else
  ( void ) bf;
#endif
}


/**********************************
 * Get buffer state
 *********************************/
volatile BufferState* InputBuffer::getState()
{
  if ( !this->getPort()->isShadow() && m_zeroCopyFromBuffer ) {
    ocpiAssert(m_zeroCopyFromBuffer->getPort()->isOutput());
    // Output buffer's state is an empty flag
    uint32_t state = m_zeroCopyFromBuffer->getState()->bufferIsEmpty & EF_MASK;
    // Invert the sense in our state
    m_tState.bufferIsFull = state == EF_FULL_VALUE ? FF_FULL_VALUE : FF_EMPTY_VALUE;
    return &m_tState;
    // old/wrong? return m_zeroCopyFromBuffer->getState();
  }

#ifdef DEBUG_L2
  ocpiDebug("In input get state, state = 0x%x", m_state[0]);
#endif


  if ( ! this->getPort()->isShadow() ) {

#ifdef DEBUG_L2
    ocpiDebug("Getting load factor of %d", m_state[0]->pad );
#endif
    m_tState.bufferIsFull = m_state[0][m_pid].bufferIsFull;

#ifdef LEAST_BUSY
    m_tState.pad = m_state[0][m_pid].pad;
#endif


#define NEED_FULL_STATE_CONTROL
#ifdef NEED_FULL_STATE_CONTROL

    // This method belongs in the controller !!!!!


    // If we are a shadow port (a local state of a real remote port), then we only
    // use m_state[0] to determine if the remote buffer is free.  Otherwise, we need
    // to look at all of the other states to determine if all outputs have written to us.
    // NOT A SHADOW HERE
    for ( OCPI::OS::uint32_t n=0; n<MAX_PCONTRIBS; n++ ) {
      //      ocpiDebug("input %p getstate m_pid: %u s: %u n %u  m_state[0][n].bufferFull %u",
      //      		this, m_pid, m_tState.bufferIsFull, n, m_state[0][n].bufferIsFull);
      //fflush(stderr); fflush(stdout);
      uint32_t full = m_state[0][n].bufferIsFull & FF_MASK;
      ocpiAssert(full == FF_EMPTY_VALUE ||
		 full == FF_FULL_VALUE);
      if (full != FF_EMPTY_VALUE) {
        m_tState.bufferIsFull = m_state[0][n].bufferIsFull;


	ocpiDebug("&&&&&&&&   found state value at %d", n );



	break;
      }
    }
#endif


  }
  else {

    int mb = getPort()->getMailbox();

    m_tState.bufferIsFull = m_myShadowsRemoteStates[mb]->bufferIsEmpty;
    uint32_t full = m_tState.bufferIsFull & FF_MASK;
    ocpiAssert(full == EF_EMPTY_VALUE ||
	       full == EF_FULL_VALUE);

#ifdef LEAST_BUSY
    m_tState.pad = m_myShadowsRemoteStates[mb]->pad;
#endif

#ifdef DEBUG_L2
    ocpiDebug("Load factor in shadow pad = %d",  m_tState.pad );
#endif

  }

  return static_cast<volatile BufferState*>(&m_tState);
}


/**********************************
 * Marks buffer as full
 *********************************/
void InputBuffer::markBufferFull()
{
  assert(getPort()->isShadow() || !(getPort()->getMetaData()->m_descriptor.options & (1 << FlagIsMeta)));
  m_produced = true;
  OCPI_EMIT_CAT_("Mark Buffer Full",OCPI_EMIT_CAT_WORKER_DEV, OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW);

  if ( ! this->getPort()->isShadow() ) {
    // This can happen when in the same container
    uint32_t full = m_state[0][0].bufferIsFull & FF_MASK;
    ocpiAssert(full == FF_EMPTY_VALUE || full == FF_FULL_VALUE);
    m_state[0][0].bufferIsFull = FF_FULL_VALUE;
  }
  else {
    // This flag is being set locally with the expectation that the other side will write back to it
    // to tell us it has become empty
    ocpiDebug("empty value: %x", m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferIsEmpty);
    ocpiAssert(m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferIsEmpty == EF_EMPTY_VALUE ||
	       m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferIsEmpty == EF_FULL_VALUE);
    m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferIsEmpty = EF_FULL_VALUE;
  }

}


/**********************************
 * Marks buffer as empty
 *********************************/
void InputBuffer::markBufferEmpty()
{
  OCPI_EMIT_CAT_("Mark Buffer Empty",OCPI_EMIT_CAT_WORKER_DEV, OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW);
  if ( ! this->getPort()->isShadow() ) {
    for ( unsigned int n=0; n<MAX_PCONTRIBS; n++ ) {
      uint32_t full = m_state[0][n].bufferIsFull & FF_MASK;
      ocpiAssert(full == FF_EMPTY_VALUE || full == FF_FULL_VALUE);
      m_state[0][n].bufferIsFull =  FF_EMPTY_VALUE;
    }
  }
  else {
    ocpiAssert("marking shadow input empty?"==0);
    m_myShadowsRemoteStates[getPort()->getMailbox()]->bufferIsFull = EF_EMPTY_VALUE;
    volatile BufferState* state = this->getState();
    state->bufferIsFull = EF_EMPTY_VALUE;
  }        
}


/**********************************
 * Get buffer state
 *********************************/
volatile BufferState* InputBuffer::getState(OCPI::OS::uint32_t rank)
{
  return &m_state[0][rank];
}


/**********************************
 * Is buffer End Of Whole EOS
 *********************************/
bool InputBuffer::isShadow()
{
  return reinterpret_cast<OCPI::DataTransport::Port*>(getPort())->isShadow();
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

         OCPI::OS::uint64_t mdata;
         bool empty = getPort()->getPullDriver()->checkBufferEmpty( (OCPI::OS::uint8_t*)getBuffer(),getLength(),mdata);
         
       //         bool empty = getPort()->getPullDriver()->checkBufferEmpty(this);         
         if ( empty ) {
           m_produced = false;
           return true;
         }
       }       
       return false;
     }
     else {  // We are a real input
       uint32_t full = state->bufferIsFull & FF_MASK;
      ocpiAssert(full == FF_EMPTY_VALUE || full == FF_FULL_VALUE);
      bool empty = full == FF_EMPTY_VALUE;
       if ( empty ) {

         OCPI::OS::uint64_t mdata;
         empty = getPort()->getPullDriver()->checkBufferEmpty((OCPI::OS::uint8_t*)getBuffer(),getLength(),mdata);
         if ( !empty ) {
           setInUse(true);
           markBufferFull();
           memcpy((void*)&(getMetaData()->ocpiMetaDataWord), &mdata, sizeof(RplMetaData) );
           setInUse(false);            
         }
       }
       return empty;
     }
   }

#ifdef DEBUG_L2
  ocpiDebug("Input buffer state = %d", m_state[0]->bufferIsFull);
#endif

#ifdef LEAST_BUSY
  OCPI::DataTransport::Port* sp = static_cast<OCPI::DataTransport::Port*>(this->getPort());
  if ( state->pad != 0) {
    sp->setBusyFactor( state->pad );
  }
#endif

#ifdef DEBUG_L2
  ocpiDebug("Shadow(%d), Buffer state = %x", (isShadow() == true) ? 1:0, state->bufferIsFull );
#endif

  uint32_t flag = state->bufferIsFull & FF_MASK;
  if (isShadow()) {
      ocpiAssert(flag == EF_EMPTY_VALUE ||
		 flag == EF_FULL_VALUE);
  } else {
      ocpiAssert(flag == FF_EMPTY_VALUE ||
		 flag == FF_FULL_VALUE);
  }
  bool empty;
  empty = (flag == (isShadow() ? EF_EMPTY_VALUE : FF_EMPTY_VALUE)) ? true : false;


#ifdef DEBUG_L2
  ocpiDebug("TB(%p) port = %p empty = %d", this, getPort(), empty );
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
  PortSet* s_port = static_cast<OCPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  ocpiAssert(!isShadow());
  for ( OCPI::OS::uint32_t n=0; n<m_outputPortCount; n++ ) {
    int id = s_port->getPortFromIndex(n)->getPortId();
    uint32_t full = m_state[0][id].bufferIsFull & FF_MASK;
    ocpiAssert(full == FF_EMPTY_VALUE || full == FF_FULL_VALUE);
    if (full != FF_EMPTY_VALUE) {
      return &m_sbMd[0][id];
    }
  }
  return &m_sbMd[0][m_pid];

}


#if 0
/**********************************
 * Get number of outputs that have written to this buffer
 *********************************/
OCPI::OS::uint32_t InputBuffer::getNumOutputsThatHaveProduced()
{
  int count=0;
  PortSet* s_port = static_cast<OCPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  ocpiAssert(!isShadow());
  for ( OCPI::OS::uint32_t n=0; n<m_outputPortCount; n++ ) {
    int id = s_port->getPortFromIndex(n)->getPortId();
    ocpiAssert(m_state[0][id].bufferIsFull == FF_EMPTY_VALUE ||
	       m_state[0][id].bufferIsFull == FF_FULL_VALUE);
    if ( m_state[0][id].bufferIsFull != FF_EMPTY_VALUE  ) {
      count++;
    }
  }
  return count;
}
#endif


/**********************************
 * Get the Output produced meta data by index
 *********************************/
volatile BufferMetaData* InputBuffer::getMetaDataByIndex( OCPI::OS::uint32_t idx )
{
  PortSet* s_port = static_cast<OCPI::DataTransport::PortSet*>(getPort()->getCircuit()->getOutputPortSet());
  int id = s_port->getPortFromIndex(idx)->getPortId();
  return &m_sbMd[0][id];
}

void InputBuffer::
setInUse(bool in_use) {
  if (in_use && getPort()->getMetaData()->m_descriptor.options & (1 << FlagIsMeta)) {
    BufferMetaData &md = *(BufferMetaData*)getMetaData(); // cast to lose volatile
    bool dummy, end;
    size_t length;
    unpackXferMetaData(getState()->bufferIsFull, length, md.ocpiMetaDataWord.opCode,
		       end, dummy);
    md.ocpiMetaDataWord.end = end ? 1 : 0;
    md.ocpiMetaDataWord.length = OCPI_UTRUNCATE(uint32_t, length);
  }
  Buffer::setInUse(in_use);
}
/**********************************
 * Destructor
 *********************************/
InputBuffer::~InputBuffer()
{


}


