
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

#include <DtOsDataTypes.h>
#include <OcpiBuffer.h>
#include <OcpiOsMutex.h>
#include <OcpiOsAssert.h>
#include <OcpiPort.h>

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
      volatile DataTransfer::OutputPortSetControl* getControlBlock();

        
      /**********************************
       * Get this buffers local state structure
       **********************************/              
      volatile DataTransfer::BufferState* getState(); 

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
      volatile DataTransfer::BufferMetaData* getMetaData();
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
      volatile DataTransfer::OutputPortSetControl* m_spsControl;

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
	// This is a local operation
        m_state[0][m_pid].bufferIsEmpty = EF_FULL_VALUE;
        setInUse(false);
      }

    /**********************************
     * Marks buffer as empty
     *********************************/
    inline void OutputBuffer::markBufferEmpty()
      {
	// this really only happens in cleanup or shadow-pull
        m_state[0][m_pid].bufferIsEmpty = EF_EMPTY_VALUE;
      }


    /**********************************
     * Get output control block
     *********************************/
    inline volatile DataTransfer::OutputPortSetControl* OutputBuffer::getControlBlock(){return m_spsControl;}

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
