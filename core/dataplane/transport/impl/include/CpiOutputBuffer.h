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
 *   This file contains the Interface for the output buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_OutputBuffer_H_
#define CPI_DataTransport_OutputBuffer_H_

#include <DtOsDataTypes.h>
#include <CpiBuffer.h>
#include <CpiOsMutex.h>

namespace CPI {

  namespace DataTransport {

    // Forward references
    class InputBuffer;

    class OutputBuffer : public CPI::DataTransport::Buffer
    {

    public:

      /**********************************
       * Constructors
       *********************************/
      OutputBuffer( CPI::DataTransport::Port* port, CPI::OS::uint32_t tid );

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
      CPI::OS::uint32_t getNumberOfBytesTransfered();
      void setNumberOfBytes2Transfer(CPI::OS::uint32_t length);

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

    protected:

      // Mapped pointer to our control structure
      volatile DataTransfer::OutputPortSetControl* m_spsControl;

      // buffer state virtual address
      void     *m_bcsVaddr;		

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
#ifdef USE_TID_FOR_DB
	m_state[0][m_pid].bufferFull = m_tid;
#else
	m_state[0][m_pid].bufferFull = 1;
#endif
	setInUse(false);
      }

    /**********************************
     * Marks buffer as empty
     *********************************/
    inline void OutputBuffer::markBufferEmpty()
      {
	m_state[0][m_pid].bufferFull = DataTransfer::BufferEmptyFlag;
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
