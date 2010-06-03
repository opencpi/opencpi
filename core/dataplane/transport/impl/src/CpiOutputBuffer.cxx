#define NDEBUG

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
 *   This file contains the implementation for the CPI output buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 */

#ifdef NDEBUG
#define NDEBUG
#endif

#include <DtHandshakeControl.h>
#include <CpiOutputBuffer.h>
#include <CpiInputBuffer.h>
#include <CpiPort.h>
#include <CpiTransferTemplate.h>
#include <CpiCircuit.h>
#include <CpiOsAssert.h>
#include <CpiUtilAutoMutex.h>

using namespace CPI::DataTransport;
using namespace DataTransfer;
using namespace CPI::OS;

/**********************************
 * Constructors
 *********************************/
OutputBuffer::OutputBuffer( CPI::DataTransport::Port* port, CPI::OS::uint32_t tid )
  : CPI::DataTransport::Buffer( port, tid ),m_slave(false)
{
  m_bVaddr = 0;
  m_bsVaddr = 0;
  m_bmdVaddr = 0;
  m_bcsVaddr = 0;

  // update
  update(0);
}


/**********************************
 * Get/Set the number of bytes transfered
 *********************************/
void OutputBuffer::setNumberOfBytes2Transfer(CPI::OS::uint32_t length)
{
  getMetaData()->cpiMetaDataWord.length = length;
}


/**********************************
 * Update offsets
 *********************************/
void OutputBuffer::update(bool critical)
{
  CPI::Util::AutoMutex guard ( m_threadSafeMutex, true ); 
  int tid = getTid();
  struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
    &getPort()->getMetaData()->m_bufferData[tid].outputOffsets;

  if ( this->m_port->isShadow()) {
    return;
  }

  // First we will map our buffer
  if ( !m_bVaddr && output_offsets->bufferOffset ) {

#ifndef NDEBUG
    printf("OutputBuffer:update: mapping buffer\n");
#endif

    m_bVaddr = getPort()->getLocalShemServices()->map
      (output_offsets->bufferOffset, 
       output_offsets->bufferSize);
    m_startOffset = output_offsets->bufferOffset;
    m_length = output_offsets->bufferSize;
    memset(m_bVaddr, 0, output_offsets->bufferSize);
    m_buffer = m_baseAddress = m_bVaddr;

#ifdef DEBUG_L2
    printf("*** Output buffer addr = 0x%x, size = %d\n", m_buffer, output_offsets->bufferSize );
#endif

  }
 
  // map our states
  if ( !m_bsVaddr && output_offsets->localStateOffset ) {

#ifndef NDEBUG
    printf("OutputBuffer:update: mapping states\n");
#endif

    m_bsVaddr = getPort()->getLocalShemServices()->map
      (output_offsets->localStateOffset, 
       sizeof(BufferState)*MAX_PCONTRIBS);

    m_state = static_cast<volatile BufferState (*)[MAX_PCONTRIBS]>(m_bsVaddr);
    for ( unsigned int y=0; y<MAX_PCONTRIBS; y++ ) {
      m_state[0][y].bufferFull = 1;
    }
  }
 
  // map our meta-data
  if ( !m_bmdVaddr && output_offsets->metaDataOffset ) {

#ifndef NDEBUG
    printf("OutputBuffer:update: mapping metadata\n");
#endif

    m_bmdVaddr = getPort()->getLocalShemServices()->map
      (output_offsets->metaDataOffset, 
       sizeof(BufferMetaData)*MAX_PCONTRIBS);

    memset(m_bmdVaddr, 0, sizeof(BufferMetaData)*MAX_PCONTRIBS);
    m_sbMd = static_cast<volatile BufferMetaData (*)[MAX_PCONTRIBS]>(m_bmdVaddr);
  }


  // map our output control structure
  if ( !m_bcsVaddr && output_offsets->portSetControlOffset ) {

#ifndef NDEBUG
    printf("OutputBuffer: mapping control structure\n");
#endif

    m_bcsVaddr = getPort()->getLocalShemServices()->map
      (output_offsets->portSetControlOffset, 
       sizeof(OutputPortSetControl));

#ifndef NDEBUG
    printf("m_bcsVaddr %p, portSetControlOffset %lld\n",
           m_bcsVaddr, (long long)output_offsets->portSetControlOffset);
#endif

                 
    memset(m_bcsVaddr, 0, sizeof(OutputPortSetControl));
    m_spsControl = static_cast<OutputPortSetControl*>(m_bcsVaddr);
    getPort()->setOutputControlBlock( m_spsControl );
  }

}

/**********************************
 * Get this buffers local state structure
 **********************************/              
volatile BufferState* OutputBuffer::getState()
{
  if ( m_dependentZeroCopyCount ) {
    CPI::OS::uint32_t c=0;
    for ( CPI::OS::uint32_t n=0;
          c<m_dependentZeroCopyCount && n<m_dependentZeroCopyPorts.size(); n++) {
      if ( m_dependentZeroCopyPorts[n] ) {
                       
        if (m_state[0][static_cast<InputBuffer*>(m_dependentZeroCopyPorts[n])->getPort()->getPortId()].bufferFull != 0 ) {
	    return &m_state[0][static_cast<InputBuffer*>(m_dependentZeroCopyPorts[n])->getPort()->getPortId()];
        }
        c++;
      }
    }
  }

#ifndef NDEBUG
  printf("Output buffer state = 0x%llx\n", (long long)m_state[0][m_pid].bufferFull );
#endif

  return &m_state[0][m_pid];
}




/**********************************
 * Is this buffer empty
 *********************************/
bool OutputBuffer::isEmpty()
{
  if ( Buffer::isEmpty() == false ) {
    return false;
  }
  CPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

#ifdef DEBUG_L2
  printf("Checking output isEmpty, m_dependentZeroCopyCount = %d, NOT CHECKING THEM !!\n", m_dependentZeroCopyCount);
#endif

  // If we have any pending transfers, we will check them here to determine if
  // they are done
  CPI::OS::int32_t n_pending = get_nentries(&m_pendingTransfers);

#ifdef DEBUG_L2
  printf("** there are %d pending output transfers\n", n_pending );
#endif

  for (CPI::OS::int32_t i=0; i < n_pending; i++) {
    TransferTemplate* temp = static_cast<TransferTemplate*>(get_entry(&m_pendingTransfers, i));
    if ( temp->isComplete() ) {
      remove_from_list( &m_pendingTransfers, temp );
      n_pending = get_nentries(&m_pendingTransfers);
      i = 0;
    }
    else {
      return false;
    }
  }

  if ( ! m_slave ) {

#ifndef NDEBUG
    printf("Not Slave port, manually setting DMA complete flag\n");
#endif

    if ( n_pending == 0 ) {
      m_state[0][m_pid].bufferFull = 1;
    }
  }

  if ( getPort()->isShadow() ) {
    return true;
  }

  volatile BufferState* state = this->getState();

  //  printf("Buffer state = %d\n", state->bufferFull );

  return (state->bufferFull != 0) ? true:false;
}

/**********************************
 * Get the offset to this ports meta-data
 **********************************/
volatile BufferMetaData* OutputBuffer::getMetaData()
{
  if ( m_zeroCopyFromBuffer ) {
    return m_zeroCopyFromBuffer->getMetaData();
  }

  return &m_sbMd[0][m_pid];
}


/**********************************
 * Get/Set the number of bytes transfered
 *********************************/
CPI::OS::uint32_t OutputBuffer::getNumberOfBytesTransfered()
{
  return (CPI::OS::uint32_t) getMetaDataByIndex(0)->cpiMetaDataWord.length;
}

void OutputBuffer::setMetaData()
{
  for ( unsigned int b=0; b<reinterpret_cast<CPI::DataTransport::Circuit*>(m_port->getCircuit())->getMaxPortOrd(); b++ ) {
    memcpy((void*) &m_sbMd[0][b].cpiMetaDataWord, (void*)&m_sbMd[0][m_pid].cpiMetaDataWord, sizeof(RplMetaData) );

    m_sbMd[0][b].zcopy           = m_sbMd[0][m_pid].zcopy;
    m_sbMd[0][b].sequence       = m_sbMd[0][m_pid].sequence;
    m_sbMd[0][b].broadCast      = m_sbMd[0][m_pid].broadCast;
    m_sbMd[0][b].srcRank        = m_sbMd[0][m_pid].srcRank;
    m_sbMd[0][b].srcTemporalId  = m_sbMd[0][m_pid].srcTemporalId;
    m_sbMd[0][b].endOfWhole     = m_sbMd[0][m_pid].endOfWhole;
    m_sbMd[0][b].endOfStream    = m_sbMd[0][m_pid].endOfStream;
  }
}

OutputBuffer::~OutputBuffer(){}


