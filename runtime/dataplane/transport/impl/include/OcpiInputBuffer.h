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
 *   This file contains the Interface for the input buffer class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_InputBuffer
#define OCPI_DataTransport_InputBuffer

#include <OcpiBuffer.h>
#include <OcpiRDTInterface.h>
#include <OcpiOsMutex.h>

namespace OCPI {
  namespace DataTransport {
    class Port;
  }
}


namespace OCPI {

  namespace DataTransport {

    class OutputBuffer;

    class InputBuffer : public Buffer
    {

    public:

      /**********************************
       * Our dependency desctriptor
       **********************************/
      OCPI::RDT::Descriptors& m_feedbackDesc;

      /**********************************
       * Constructors
       *********************************/
      InputBuffer( OCPI::DataTransport::Port* input_port, OCPI::OS::uint32_t tid );

      /**********************************
       * Destructor
       *********************************/
      virtual ~InputBuffer();

      /**********************************
       * Update offsets
       *********************************/
      void update(bool critical);

      /**********************************
       * Get this buffers remote state offset
       **********************************/
      DtOsDataTypes::Offset getRemoteStateOffset( 
            OCPI::OS::uint32_t s_port_id );  // There is a remote state for every output port in the circuit

      /**********************************
       * Is this buffer empty
       *********************************/
      virtual bool isEmpty();



      /**********************************
       * Use the buffer id for flow control
       *********************************/
      void useTidForFlowControl( bool ut );



      /**********************************
       * Get/Set the number of bytes transfered
       *********************************/
      OCPI::OS::uint32_t getNumberOfBytesTransfered();
      void setNumberOfBytes2Transfer(OCPI::OS::uint32_t){};

      /**********************************
       * Marks buffer as empty
       *********************************/
      virtual void markBufferEmpty();

      /**********************************
       * Marks buffer as full
       *********************************/
      virtual void markBufferFull();

      /**********************************
       * Produce 
       *********************************/
      virtual void produce( OCPI::DataTransport::Buffer* src_buf );

      /**********************************
       * Sets the buffers busy factor
       **********************************/
      virtual void setBusyFactor( OCPI::OS::uint32_t bf );

      /**********************************
       * Is buffer End Of Stream EOS
       *********************************/
      virtual bool isEOS();

      /**********************************
       * Is buffer End Of Whole EOS
       *********************************/
      virtual bool isEOW();

      /**********************************
       * Gets the input state structure. This is a "smart" routine that returns the 
       * the correct state structure depending on whether this buffer is the real input
       * buffer or if it is the "shadow".  The parameter rank is used for debug to get the 
       * real inputs state that can consist of N states in sequentail distribution, where N
       * is equal to the number of output ports.
       **********************************/
      virtual volatile BufferState* getState();
      volatile BufferState* getState(OCPI::OS::uint32_t rank);

      /**********************************
       * Get the offset to this ports meta-data
       **********************************/
      volatile BufferMetaData* getMetaData();

                
      /**********************************
       * Get number of outputs that have written to this buffer
       *********************************/
      //      virtual OCPI::OS::uint32_t getNumOutputsThatHaveProduced();

      /**********************************
       * Is this a shadow buffer ?
       *********************************/
      virtual bool isShadow();


      /**********************************
       * Get the Output produced meta data by index
       *********************************/
      volatile BufferMetaData* getMetaDataByIndex( OCPI::OS::uint32_t idx );

      void setInUse(bool in_use); // override default method in Buffer class
    protected:

      // Number of ports int the output port set
      OCPI::OS::uint32_t m_outputPortCount;

      // represents the "AND" of all input buffer states
      BufferState m_tState;

      // Mapped pointer to our state
      volatile BufferState*  m_myShadowsRemoteStates[MAX_PCONTRIBS];
      void          *(m_rssVaddr[MAX_PCONTRIBS]);                // buffer state virtual address

      // Keeps track of when it produces
      bool m_produced;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

    /**********************************
     * Is buffer End Of Stream EOS
     *********************************/
    inline bool InputBuffer::isEOS(){return getMetaData()->endOfStream == 1 ? true : false;}

    /**********************************
     * Is buffer End Of Whole EOS
     *********************************/
    inline bool InputBuffer::isEOW(){return getMetaData()->endOfWhole == 1 ? true : false;}

  }

}


#endif
