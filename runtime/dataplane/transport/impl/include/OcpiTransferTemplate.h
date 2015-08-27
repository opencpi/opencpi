
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
 *   This file contains the Interface for the OCPI transfer template class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_TransferTemplate_H_
#define OCPI_DataTransport_TransferTemplate_H_

#include <OcpiPortMetaData.h>
#include <DtTransferInternal.h>
#include <OcpiIntTransferTemplate.h>
#include <OcpiTransportConstants.h>
#include <OcpiOutputBuffer.h>
#include <OcpiInputBuffer.h>
#include <OcpiList.h>
#include <OcpiOsAssert.h>

#include <stdio.h>

namespace DataTransfer {
  struct BufferMetaData;
}

namespace OCPI {

  namespace DataTransport {

    class PortSet;
    class Port;

    class OcpiTransferTemplate : TransferTemplate
    {

    public:

      /**********************************
       * Constructors
       *********************************/
      OcpiTransferTemplate(OCPI::OS::uint32_t id);

      /**********************************
       * Destructor
       *********************************/
      virtual ~OcpiTransferTemplate();

      /**********************************
       * Is this transfer pending
       *********************************/
      virtual bool isPending();

      /**********************************
       * Is this transfer in use
       *********************************/
      virtual bool isComplete();


      /**********************************
       * Start the output/input transfer
       *********************************/
      virtual void produce();

      /**********************************
       * Start the input reply transfer
       *********************************/
      virtual OCPI::DataTransport::Buffer* consume();

      /**********************************
       * Start the input reply transfer
       *********************************/
      virtual void modify(DtOsDataTypes::Offset new_off[], DtOsDataTypes::Offset old_off[] );

      /**********************************
       * Get/Set transfer type id
       *********************************/
      void setTypeId(OCPI::OS::uint32_t id);
      OCPI::OS::uint32_t  getTypeId();

      /**********************************
       * Check for duplicates
       *********************************/
      bool isDuplicate( 
                       OutputBuffer* output,                        // In - Output buffer
                       InputBuffer* input );                // In - Input buffer

      /**********************************
       * Add a transfer request
       *********************************/
      void addTransfer( DataTransfer::XferRequest* tx_request );

      /**********************************
       * Add a xero copy transfer request
       *********************************/
      void addZeroCopyTransfer( OutputBuffer* output, InputBuffer* input );

      /**********************************
       * Add a gated transfer, gated transfers are additional transfers that 
       *********************************/
      void addGatedTransfer( 
                            OCPI::OS::uint32_t sequence,
                            OcpiTransferTemplate* gated_transfer,
                            PortOrdinal input_port_id,
                            OCPI::OS::uint32_t         buffer_tid);

      /**********************************
       * Get a gated transfer
       *********************************/
      OcpiTransferTemplate* getNextGatedTransfer(
                                                PortOrdinal input_port_id,
                                                OCPI::OS::uint32_t         buffer_tid);

      /**********************************
       * Get the maximum post produce sequence this class should transfer
       *********************************/
      OCPI::OS::uint32_t getMaxGatedSequence();

      /**********************************
       * Produce the next gated tansfer of this type
       *********************************/
      OCPI::OS::uint32_t produceGated( OCPI::OS::uint32_t port_id, OCPI::OS::uint32_t tid  );

      /**********************************
       * Intializes the presets values into the output meta-data prior to kicking off
       * a transfer
       *********************************/
      virtual void presetMetaData( 
                          volatile DataTransfer::BufferMetaData* data,    // In - Pointer to output meta-data
                          OCPI::OS::uint32_t    length,                                // In - Data transfer length
                          bool  end_of_whole,                // In - End of whole transfer
                          OCPI::OS::uint32_t    nPartsPerWhole,                // In - Number of parts that make up a whole
                          OCPI::OS::uint32_t    sequence                                // In - Whole sequence
                          );


      /**********************************
       * Sets the next input port and tid
       *********************************/
      void setInput( OCPI::DataTransport::Port* p, OCPI::OS::uint32_t tid);


      struct ZCopy {
        ZCopy():output(NULL),input(NULL),next(NULL){}
        ZCopy( OutputBuffer* s, InputBuffer* t )
          :output(s),input(t),next(NULL){}
        void add( ZCopy* );
        OutputBuffer* output;
        InputBuffer* input;
        ZCopy* next;

      };
      ZCopy *m_zCopy;


    private:

      /**********************************
       * Executes preset's
       *********************************/
      void presetMetaData();

      // Our transfer type id
      OCPI::OS::uint32_t m_id;

      // xfer request class
      OCPI::OS::uint32_t n_transfers;
      DataTransfer::XferRequest* m_xferReq[MAX_TRANSFERS];

      // For some templates, there are mutiple transfers that have to take place from
      // a single output buffer, such is the case for whole to parts when the number of
      // parts exceed the number of buffers+input ports.
      //                                                                           transfer sequence            input port    input buffer
      OcpiTransferTemplate* m_nextTransfer[MAX_TRANSFERS_PER_BUFFER] [MAX_PCONTRIBS] [MAX_BUFFERS];
      List m_gatedTransfersPending;
      OCPI::OS::uint32_t m_sequence;
      OCPI::OS::uint32_t m_maxSequence;


      // List of preset meta-data structures
      struct PresetMetaData {
        volatile DataTransfer::BufferMetaData* ptr;
        OCPI::OS::uint32_t length;
        OCPI::OS::uint32_t endOfWhole;
        OCPI::OS::uint32_t nPartsPerWhole;
        OCPI::OS::uint32_t sequence;
      };
      List m_PresetMetaData;


      // Next input
      OCPI::DataTransport::Port* m_nextPort;
      OCPI::OS::uint32_t   m_nextTid;

      // Private initialization
      void init();

    };


    /**********************************
     * inline declarations
     *********************************/

    /**********************************
     * Add a transfer request
     *********************************/
    inline void OcpiTransferTemplate::addTransfer( DataTransfer::XferRequest* tx_request ) {
      ocpiAssert( n_transfers < MAX_TRANSFERS);
      m_xferReq[n_transfers++] = tx_request;
    }

    /**********************************
     * Add a gated transfer, gated transfers are additional transfers that 
     *********************************/
    inline void OcpiTransferTemplate::addGatedTransfer( 
                                                      OCPI::OS::uint32_t      sequence,
                                                      OcpiTransferTemplate* gated_transfer, 
                                                      PortOrdinal     input_port_id,
                                                      OCPI::OS::uint32_t             buffer_tid)
      {
        if ( sequence > m_maxSequence ) {
          m_maxSequence = sequence;
        }
#ifndef NDEBUG
        printf("*** Adding a gated transfer to this[%d][%d][%d] \n",
               m_maxSequence,input_port_id, buffer_tid);
#endif
        m_nextTransfer[sequence][input_port_id][buffer_tid] = gated_transfer;
      }

    /**********************************
     * Get a gated transfer
     *********************************/
    inline OcpiTransferTemplate* OcpiTransferTemplate::getNextGatedTransfer(
                                                                          PortOrdinal input_port_id,
                                                                          OCPI::OS::uint32_t         buffer_tid)
      {return m_nextTransfer[m_sequence++][input_port_id][buffer_tid];}

    /**********************************
     * Get the maximum post produce sequence this class should transfer
     *********************************/
    inline OCPI::OS::uint32_t OcpiTransferTemplate::getMaxGatedSequence(){return m_maxSequence;}

    /**********************************
     * Get/Set transfer type id
     *********************************/
    inline void OcpiTransferTemplate::setTypeId(OCPI::OS::uint32_t id){m_id=id;}
    inline OCPI::OS::uint32_t  OcpiTransferTemplate::getTypeId(){return m_id;}

    /**********************************
     * Sets the next input port and tid
     *********************************/
    inline void OcpiTransferTemplate::setInput( OCPI::DataTransport::Port* p, 
                                                OCPI::OS::uint32_t tid){m_nextPort=p; m_nextTid=tid;}

  }

}


#endif
