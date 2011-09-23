
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
 *   This file contains the implementation for the OCPI buffer class.
 *
 * Revision History: 
 *
 *    05/18/09 
 *    John Miller
 *    Added zero copy I/O capability.
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 */

#include <DtHandshakeControl.h>
#include <OcpiBuffer.h>
#include <OcpiPort.h>
#include <OcpiOsAssert.h>
#include <OcpiCircuit.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;


/**********************************
 * Constructors
 *********************************/
Buffer::Buffer( OCPI::DataTransport::Port* port, OCPI::OS::uint32_t tid )
    : m_zeroCopyFromBuffer(NULL),m_port(port),
      m_zCopyPort(0),m_attachedZBuffer(0),m_tid(tid),m_buffer(0),
      m_dependentZeroCopyPorts(1),  m_dependentZeroCopyCount(0), 
      m_InUse(false),m_remoteZCopy(false), m_threadSafeMutex(true)
{
  m_pullTransferInProgress = NULL;
  m_pid=m_port->getPortId();
}


/**********************************
 * Marks/Unmarks the attached buffers that are using
 * this buffer for zero copy
 **********************************/ 
void Buffer::markZeroCopyDependent( Buffer* dependent_buffer )
{

  OCPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  ocpiAssert( dependent_buffer );
  if ( (OCPI::OS::uint32_t)dependent_buffer->getPort()->getPortId()<m_dependentZeroCopyPorts.size() &&
       m_dependentZeroCopyPorts[dependent_buffer->getPort()->getPortId()] ) {

#ifndef NDEBUG
    printf("Error detected in Buffer::markZeroCopyDependent\n");
#endif

  }
  m_dependentZeroCopyPorts.insertToPosition(dependent_buffer, dependent_buffer->getPort()->getPortId() ); 
  m_dependentZeroCopyCount++;
}

void Buffer::unMarkZeroCopyDependent( Buffer* dependent_buffer )
{
  ocpiAssert( dependent_buffer );
  ocpiAssert( (OCPI::OS::uint32_t)dependent_buffer->getPort()->getPortId() < m_dependentZeroCopyPorts.size() );

  if ( ! m_dependentZeroCopyPorts[dependent_buffer->getPort()->getPortId()] ) {
#ifndef NDEBUG
    printf("Error detected in Buffer::unMarkZeroCopyDependent\n");
#endif
  }
  m_dependentZeroCopyPorts.insertToPosition(NULL, dependent_buffer->getPort()->getPortId() );
  m_dependentZeroCopyCount--;

  if ( m_dependentZeroCopyCount == 0 ) {
    m_InUse = false;
    markBufferEmpty();
  }
}

void 
Port::
addBuffer( OCPI::DataTransport::Buffer* buf )
{
  m_buffers[buf->getTid()] = buf;
}

/**********************************
 * Is this buffer empty
 *********************************/
bool Buffer::isEmpty()
{

  OCPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  // There are two reasons why this would be non-zero, either there is a local "in-process"
  // zero copy buffer attached, or an OOP zer copy buffer attached.  Internal buffers are 
  // automatically detached, OOP buffers require a little more work.
  if ( m_dependentZeroCopyCount ) {
    if ( m_remoteZCopy ) {
      for ( unsigned int n=0; n<reinterpret_cast<OCPI::DataTransport::Circuit*>(m_port->getCircuit())->getMaxPortOrd() &&
              n<m_dependentZeroCopyCount && n<m_dependentZeroCopyPorts.size();
            n++ ) {
        if ( m_dependentZeroCopyPorts[n] &&
             static_cast<Buffer*>(m_dependentZeroCopyPorts[n])->getPort()->isShadow() && 
             static_cast<Buffer*>(m_dependentZeroCopyPorts[n])->isEmpty() ) {

#ifdef DEBUG_L2
          printf("OcpiNmOutputBuffer::isEmpty() Dependent input is empty, removing it\n");
#endif

          m_dependentZeroCopyCount--;
          m_dependentZeroCopyPorts.insertToPosition(NULL,n);
        }
      }
    }
    else {
      return false;
    }

    if ( m_dependentZeroCopyCount ) {
      return false;
    }

  }   

  bool shadow = getPort()->isShadow();

  /*
   * In the case of a input buffer, if we are co-located and are
   * sharing the output buffer, The output buffer needs to decide
   * if it is empty or not.
   */
  if ( m_zeroCopyFromBuffer && !shadow ) {
    return m_zeroCopyFromBuffer->isEmpty();
  }
  else if ( !shadow && getMetaDataByPortId(m_pid)->zcopy & ZeroCopyReady )  {
    unsigned int s_pid = (getMetaDataByPortId(m_pid)->zcopy & ~ZeroCopyReady);
    s_pid >>= 16;
    unsigned int tid = getMetaDataByPortId(m_pid)->zcopy & 0xff;
#ifdef DEBUG_L2
    printf("*&*&* We have a OOP Zercopy ready buffer, s_pid = %d, tid = %d\n",
           s_pid, tid);
#endif
    OCPI::DataTransport::Port* s_port = 
      static_cast<OCPI::DataTransport::Port*>(this->getPort()->getCircuit()->getOutputPortSet()->getPortFromOrdinal( s_pid ));
    Buffer* s_buf = static_cast<Buffer*>(s_port->getBuffer(tid));
    attachZeroCopy( reinterpret_cast<Buffer*>(s_buf) );
    return false;
  }
  return true;
}


/**********************************
 * Attach a zero copy buffer to this buffer
 **********************************/
void Buffer::attachZeroCopy( Buffer* from_buffer )
{
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  ocpiAssert( ! m_zeroCopyFromBuffer );

#ifdef DEBUG_L2
  printf("*** ATTACHING from 0x%x to this 0x%x\n", from_buffer, this);
#endif

  // If this is an out of process ZC
  if ( getPort()->isShadow() ) {

    // We are a shadow and the output is local
    getMetaDataByPortId(m_pid)->zcopy = 
      ZeroCopyReady | from_buffer->getPort()->getPortId()<<16 | from_buffer->getTid();
    from_buffer->m_remoteZCopy = true;
  }

  m_zeroCopyFromBuffer = from_buffer;
  from_buffer->markZeroCopyDependent( this );
 
}



/**********************************
 * Attach a zero copy buffer to this buffer
 **********************************/
Buffer* Buffer::detachZeroCopy()
{
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  Buffer* rb=m_zeroCopyFromBuffer;

#ifdef DEBUG_L2
  printf("*** DETATCHING 0x%x\n", m_zeroCopyFromBuffer);
#endif

  // Disable remote zero copy
  getMetaDataByPortId(m_pid)->zcopy = 0;

  // Since this buffer can be filled by co-located and non co-located outputs,
  // we only need to perform this method is the output was co-located
  if ( ! m_zeroCopyFromBuffer ) {
    return NULL;
  }

  if ( m_zeroCopyFromBuffer->m_zeroCopyFromBuffer ) {
    rb = m_zeroCopyFromBuffer->detachZeroCopy();
  }

  // We need to inform all links that this buffer is no longer a dependency
  m_zeroCopyFromBuffer->unMarkZeroCopyDependent( this );
  m_zeroCopyFromBuffer = NULL;
  return rb;

}



/**********************************
 * Get/Set the internal buffer 
 **********************************/
volatile void* Buffer::getBuffer()
{
  if ( m_zeroCopyFromBuffer ) {
    return m_zeroCopyFromBuffer->getBuffer();
  }
  return m_buffer;
}


/**********************************
 * Get the Output produced meta data by port Id
 *********************************/
volatile 
BufferMetaData* 
Buffer::
getMetaDataByPortId( OCPI::OS::uint32_t id )
{
  return &m_sbMd[0][id];
}


void 
Buffer::
send()
{
  if ( m_noTransfer ) return;
  if ( m_port->getCircuit()->canTransferBuffer( this ) ) {
    m_port->getCircuit()->startBufferTransfer( this );
  }
  else {
    m_port->getCircuit()->queTransfer( this );
  }
}


/**********************************
 * DEBUG: print offsets
 **********************************/
#ifndef NDEBUG
void 
Buffer::
dumpOffsets()
{
  printf("\n\n m_sbMd = %p\n", m_sbMd);
  printf("m_state = %p\n", m_state);
  printf("m_buffer = %p\n", m_buffer);
}
#else
void Buffer::dumpOffsets(){}
#endif



/**********************************
 * Destructor
 *********************************/
Buffer::~Buffer()
{
  destroy_list(&m_pendingTransfers);
}



