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
 *   This file contains the Interface for the output buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_OutputBuffer_H_
#define OCPI_DataTransport_OutputBuffer_H_

#include <OcpiBuffer.h>
#include <OcpiOsMutex.h>
#include <OcpiOsAssert.h>
#include <OcpiPort.h>
#include <OcpiTimeEmitCategories.h>

namespace OCPI {

  namespace DataTransport {

    // Forward references
    class InputBuffer;

    class OutputBuffer : public OCPI::DataTransport::Buffer
    {

    public:

      /**********************************
       * Constructors
       *********************************/
      OutputBuffer( OCPI::DataTransport::Port* port, OCPI::OS::uint32_t tid );

      /**********************************
       * Destructor
       *********************************/
      virtual ~OutputBuffer();

      /**********************************
       * Update offsets
       *********************************/
      void update(bool critical);

      /**********************************
       * Get output control block
       *********************************/
      volatile OutputPortSetControl* getControlBlock();

        
      /**********************************
       * Get this buffers local state structure
       **********************************/              
      volatile BufferState* getState(); 

      /**********************************
       * Is this buffer empty
       *********************************/
      virtual bool isEmpty();

      /**********************************
       * Marks buffer as full
       *********************************/
      virtual void markBufferFull();

      /**********************************
       * Marks buffer as full
       *********************************/
      virtual void markBufferEmpty();

      /**********************************
       * Get/Set the number of bytes transfered
       *********************************/
      OCPI::OS::uint32_t getNumberOfBytesTransfered();
      void setNumberOfBytes2Transfer(OCPI::OS::uint32_t length);

      /**********************************
       * Get the offset to this ports meta-data
       **********************************/
      volatile BufferMetaData* getMetaData();
      void setMetaData();

      /**********************************
       * Is buffer End Of Stream EOS
       *********************************/
      virtual bool isEOS();

      /**********************************
       * Is buffer End Of Whole EOS
       *********************************/
      virtual bool isEOW();

      void setSlave(){m_slave=true;}

    protected:

      // Mapped pointer to our control structure
      volatile OutputPortSetControl* m_spsControl;

      // buffer state virtual address
      void     *m_bcsVaddr;                

      // I do not control DMA from my buffer
      bool m_slave;
    };



    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    /**********************************
     * Marks buffer as full
     *********************************/
    inline void OutputBuffer::markBufferFull()
      {
	ocpiAssert(!getPort()->isShadow());
	OCPI_EMIT_CAT_("Mark Buffer Full",OCPI_EMIT_CAT_WORKER_DEV, OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW);
	// This is a local operation
	ocpiDebug("Mark output buffer full: %p before %x", &m_state[0][m_pid].bufferIsEmpty, m_state[0][m_pid].bufferIsEmpty);
        m_state[0][m_pid].bufferIsEmpty = EF_FULL_VALUE;
	assert(inUse());
        setInUse(false);
	ocpiDebug("Mark output buffer full: after %x", m_state[0][m_pid].bufferIsEmpty);
      }

    /**********************************
     * Marks buffer as empty
     *********************************/
    inline void OutputBuffer::markBufferEmpty()
      {
	OCPI_EMIT_CAT_("Mark Buffer Empty",OCPI_EMIT_CAT_WORKER_DEV, OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW);
	// this really only happens in cleanup or shadow-pull
	ocpiDebug("Mark output buffer empty: %p before %x", &m_state[0][m_pid].bufferIsEmpty, m_state[0][m_pid].bufferIsEmpty);
	if ((m_state[0][m_pid].bufferIsEmpty & EF_MASK) != EF_EMPTY_VALUE)
	  m_state[0][m_pid].bufferIsEmpty = EF_EMPTY_VALUE;
	ocpiDebug("Mark output buffer empty: after %x", m_state[0][m_pid].bufferIsEmpty);
      }


    /**********************************
     * Get output control block
     *********************************/
    inline volatile OutputPortSetControl* OutputBuffer::getControlBlock(){return m_spsControl;}

    /**********************************
     * Is buffer End Of Stream EOS
     *********************************/
    inline bool OutputBuffer::isEOS(){return m_spsControl->endOfStream == 1 ? true : false;}

    /**********************************
     * Is buffer End Of Whole EOS
     *********************************/
    inline bool OutputBuffer::isEOW(){return m_spsControl->endOfWhole == 1 ? true : false;}

  }

}


#endif
