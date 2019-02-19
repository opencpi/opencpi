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
#include <OcpiTimeEmitCategories.h>


using namespace OCPI::DataTransport;
using namespace DataTransfer;


/**********************************
 * Constructors
 *********************************/
Buffer::Buffer( OCPI::DataTransport::Port* port, OCPI::OS::uint32_t tid )
  : OCPI::Time::Emit( port, "Buffer", ""),
  m_zeroCopyFromBuffer(NULL), m_startOffset(0), m_length(0), m_port(port), m_zCopyPort(0),
  m_attachedZBuffer(0), m_tid(tid), m_pullTransferInProgress(NULL), m_pid(0), m_sbMd(NULL),
  m_bmdVaddr(NULL), m_state(NULL), m_bsVaddr(NULL), m_buffer(0), m_bVaddr(NULL),
  m_dependentZeroCopyPorts(1), m_dependentZeroCopyCount(0), m_InUse(false), m_remoteZCopy(false),
  m_threadSafeMutex(true), m_noTransfer(false)
{
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
      for ( PortOrdinal n=0; n<m_port->getCircuit()->getMaxPortOrd() &&
              n<m_dependentZeroCopyCount && n<(PortOrdinal)m_dependentZeroCopyPorts.size();
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
    ocpiAssert(s_port);
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
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex, true ); 
  OCPI_EMIT_CAT_("Attaching ZCopy buffer",OCPI_EMIT_CAT_TUNING,OCPI_EMIT_CAT_TUNING_DP);

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
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex, true ); 

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

  OCPI_EMIT_CAT_("Detaching ZCopy buffer",OCPI_EMIT_CAT_TUNING,OCPI_EMIT_CAT_TUNING_DP);

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
    OCPI_EMIT_CAT_("Posting Buffer Transfer",OCPI_EMIT_CAT_WORKER_DEV,OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW);
    m_port->getCircuit()->startBufferTransfer( this );
  }
  else {
    OCPI_EMIT_CAT_("Queing Buffer Transfer",OCPI_EMIT_CAT_WORKER_DEV,OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW);
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



