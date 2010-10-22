
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
 *   This file contains the Interface for the OCPI port.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_Port_H_
#define OCPI_DataTransport_Port_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OcpiList.h>
#include <OcpiRDTInterface.h>
#include <OcpiTransportConstants.h>
#include <OcpiPortMetaData.h>
#include <OcpiPortSet.h>
#include <OcpiParentChild.h>
#include <DtSharedMemoryInternal.h>


namespace DataTransfer {
  struct OutputPortSetControl;
  struct SMBResources;
  struct EndPoint;
}

namespace OCPI {

  namespace DataTransport {

    class PullDataDriver;
    class PortSet;
    class Buffer;
    class OutputBuffer;
    class InputBuffer;
    class Circuit;

    // This is the OCPI specialized port class definition
    class Port : public OCPI::Util::Child<PortSet,Port>
    {

    public:

      friend class Circuit;

      /**********************************
       * Constructors
       *********************************/
      Port( PortMetaData* data, PortSet* ps );

      /**********************************
       * Destructor
       *********************************/
      virtual ~Port();


      /**********************************
       * Advance the ports buffer buffer
       *********************************/
      void advance( Buffer* buffer, unsigned int len=0);


      /**********************************
       * Get port descriptor.  This is the data that is needed by an
       * external port to connect to this port.
       *********************************/
      void getPortDescriptor( OCPI::RDT::Descriptors& );


      /**********************************
       * This method determines if there is an empty output buffer, but does not affect the
       * state of the object.
       *********************************/
      bool hasEmptyOutputBuffer();



      /**********************************
       * This method determines if there is data available, but does not affect the
       * state of the object.
       *********************************/
      bool hasFullInputBuffer();


      /**********************************
       * Reteives the next available input buffer.
       *********************************/
      Buffer* getNextFullInputBuffer();


      /**********************************
       * This method retreives the next available buffer from the local (our)
       * port set.  A NULL port indicates local context.
       *********************************/
      Buffer* getNextEmptyOutputBuffer();



      /**********************************
       * Get the port dependency data
       *********************************/
      PortMetaData::OcpiPortDependencyData& getPortDependencyData();

      /**********************************
       * Sets the feedback descriptor for this port.
       *********************************/
      virtual void setFlowControlDescriptor( OCPI::RDT::Descriptors& );

      /**************************************
       * Get the buffer by index
       ***************************************/
      inline Buffer* getBuffer( OCPI::OS::uint32_t index ){return m_buffers[index];}
      OCPI::DataTransport::OutputBuffer* getOutputBuffer(OCPI::OS::uint32_t idx);
      OCPI::DataTransport::InputBuffer* getInputBuffer(OCPI::OS::uint32_t idx);

      /**************************************
       * Are we a shadow port ?
       ***************************************/
      bool isShadow();

      /**************************************
       * Are we a Output port ?
       ***************************************/
      inline bool isOutput(){return m_data->output;}

      /**************************************
       * Get our ordinal id
       ***************************************/
      inline PortOrdinal getPortId(){return m_data->id;}

      /**************************************
       * Debug dump
       ***************************************/
      void debugDump();

      /**************************************
       * Get buffer count
       ***************************************/
      OCPI::OS::uint32_t getBufferCount();

      /**********************************
       * Get port data
       *********************************/
      inline PortMetaData* getMetaData(){return m_data;}

      /**********************************
       * Get our associated circuit
       *********************************/
      Circuit* getCircuit();


      /***********************************
       * This method is used to send an input buffer thru an output port with Zero copy, 
       * if possible.
       *********************************/
      void sendZcopyInputBuffer( Buffer* src_buf, unsigned int len );

      

      // Advanced buffer management
    protected:

      /**********************************
       * This method causes the specified input buffer to be marked
       * as available.
       *********************************/
      OCPI::OS::int32_t inputAvailable( Buffer* input_buf );


      /**********************************
       * Send an output buffer
       *********************************/
      void sendOutputBuffer( Buffer* b, unsigned int length );


      /**********************************
       * Advance the ports circular buffer
       *********************************/
      void advance( OCPI::OS::uint64_t value );


    private:

      /**********************************
       * Internal port initialization
       *********************************/
      void initialize();

      /**********************************
       * This routine is used to allocate the Output buffer offsets
       **********************************/
      virtual void createOutputOffsets();

      /**********************************
       * This routine is used to allocate the input buffer offsets
       **********************************/
      virtual void createInputOffsets();

      /**********************************
       * This method invokes the appropriate buffer allocation routine
       **********************************/
      void allocateBufferResources();

      // Our intialized flag
      bool m_initialized;

      // This ports meta data
      PortMetaData* m_data;

      // Our shared memory object
      DataTransfer::SMBResources* m_realSMemResources;
      DataTransfer::SMBResources* m_shadowSMemResources;
      DataTransfer::SMBResources* m_localSMemResources;

      // Handshake port control
      volatile DataTransfer::OutputPortSetControl* m_hsPortControl;

      // This routine creates our buffers from the meta-data
      void createBuffers();

      // Last buffer that was processed
      OCPI::OS::uint32_t m_lastBufferTidProcessed;

      // Sequence number of last buffer that was transfered
      OCPI::OS::uint32_t m_sequence;

      // Are we a shadow port
      bool m_shadow;

      // Our busy factor
      OCPI::OS::uint32_t m_busyFactor;

      // End of stream indicator
      bool m_eos;

      // Our mailbox
      OCPI::OS::uint32_t m_mailbox;

      // Our port dependency data
      PortMetaData::OcpiPortDependencyData m_portDependencyData;

      // Offset into the SMB to our offsets
      OCPI::OS::uint64_t m_offsetsOffset;

      // used to cycle through buffers
      int m_lastBufferOrd;

      // This port is externally connected
      enum ExternalConnectState {
        NotExternal,
        WaitingForUpdate,
        WaitingForShadowBuffer,
        DefinitionComplete
      };
      ExternalConnectState m_externalState;

      // Our pull driver
      PullDataDriver* m_pdDriver;

      // Buffers
      int m_localBufferCount;
      Buffer* m_buffers[MAX_BUFFERS];

      // Associated Port set
      PortSet* m_portSet;


      /**********************************
       * Sets the feedback descriptor for this port.
       *********************************/
      virtual void setFlowControlDescriptorInternal( OCPI::RDT::Descriptors& );


    public:
      
      // The following methods are public but are only used by internal port managment classes


      /**************************************
       * Attaches a pull data driver to this port
       ***************************************/
      void attachPullDriver( PullDataDriver* pd );
      PullDataDriver* getPullDriver();

      /**************************************
       * Get the ordinal of the last buffer 
       * processed
       ***************************************/
      int &getLastBufferOrd();

      // List of zCopy buffers to send to 
      OCPI::Util::VList m_zCopyBufferQ;

      /**************************************
       * Get the next full input buffer
       ***************************************/
      OCPI::OS::uint32_t& getLastBufferTidProcessed();

      /**************************************
       * Get the buffer transfer sequence
       ***************************************/
      OCPI::OS::uint32_t& getBufferSequence();

      /**********************************
       * Has an End Of Stream been detcted on this port
       *********************************/
      bool isEOS();
      void setEOS();
      void resetEOS();

      // Get/Set rank for scaled ports
      inline void setRank( OCPI::OS::uint32_t r ){m_data->rank=r;}
      inline OCPI::OS::uint32_t getRank(){return m_data->rank;}


      /**********************************
       * Get/Set the SMB name
       *********************************/
      const char* getSMBAddress();
      DataTransfer::EndPoint* getEndpoint();
      std::string& getShadowEndpoint();
      void setEndpoint( std::string& ep );

      /**********************************
       * Determines of a port is ready to go
       *********************************/
      bool ready();

      /**********************************
       * Once the circuit definition is complete, we need to update each port
       *********************************/
      void update();

      /**********************************
       * writes buffer offsets to address
       *********************************/
      void writeOffsets( PortMetaData::BufferOffsets* offset );


      /**********************************
       * get buffer offsets to dependent data
       *********************************/
      struct ToFrom_ {
        OCPI::OS::uint64_t from_offset;
        OCPI::OS::uint64_t to_offset;
      };
      typedef ToFrom_ ToFrom;
      void getOffsets( OCPI::OS::uint32_t to_base_offset, OCPI::Util::VList& offsets );
      void releaseOffsets( OCPI::Util::VList& offsets );


      /**********************************
       * Get the shared memory object
       *********************************/
      DataTransfer::SmemServices* getRealShemServices();
      DataTransfer::SmemServices* getShadowShemServices();
      DataTransfer::SmemServices* getLocalShemServices();
      OCPI::OS::uint32_t                            getMailbox();

      /**********************************
       * Get this source port's control structure
       *********************************/
      volatile DataTransfer::OutputPortSetControl* getOutputControlBlock();
      void setOutputControlBlock( volatile DataTransfer::OutputPortSetControl* scb );

      /**********************************
       * Get the offsets to the other Output ports control structures within the circuit
       *********************************/
      DtOsDataTypes::Offset getPortHSControl(PortOrdinal id);


      /**********************************
       * Can these two ports support Zero Copy transfers
       *********************************/
      virtual bool supportsZeroCopy( Port* port );

      /**************************************
       * Sets this ports busy factor
       ***************************************/
      void setBusyFactor(OCPI::OS::uint32_t bf );
      OCPI::OS::uint32_t getBusyFactor();

      /**************************************
       * Reset the port
       ***************************************/
      void reset();

      inline PortSet* getPortSet(){return m_portSet;}
      void addBuffer( Buffer* buf );


    private:

      int m_bufferCount;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline PortMetaData::OcpiPortDependencyData& Port::getPortDependencyData(){return m_portDependencyData;}
    inline OCPI::OS::uint32_t& Port::getBufferSequence(){return m_sequence;}
    inline bool Port::isShadow(){return m_shadow;}
    inline OCPI::OS::uint32_t& Port::getLastBufferTidProcessed(){return m_lastBufferTidProcessed;}
    inline const char* Port::getSMBAddress(){return m_data->m_real_location->getAddress();}
    inline DataTransfer::EndPoint* Port::getEndpoint(){return m_data->m_real_location;}
    inline std::string& Port::getShadowEndpoint(){return m_data->m_shadow_location->end_point;}
    inline volatile DataTransfer::OutputPortSetControl* Port::getOutputControlBlock(){return m_hsPortControl;}
    inline void Port::setOutputControlBlock( volatile DataTransfer::OutputPortSetControl* scb ){m_hsPortControl=scb;}
    inline OCPI::DataTransport::OutputBuffer* Port::getOutputBuffer(OCPI::OS::uint32_t idx)
      {return reinterpret_cast<OCPI::DataTransport::OutputBuffer*>(Port::getBuffer(idx));}
    inline OCPI::DataTransport::InputBuffer* Port::getInputBuffer(OCPI::OS::uint32_t idx)
      {return reinterpret_cast<OCPI::DataTransport::InputBuffer*>(Port::getBuffer(idx));}



    inline void Port::setBusyFactor(OCPI::OS::uint32_t bf ){m_busyFactor=bf;}
    inline OCPI::OS::uint32_t Port::getBusyFactor(){return m_busyFactor;}
    inline void Port::setEOS(){m_eos=true;}
    inline int& Port::getLastBufferOrd(){return m_lastBufferOrd;}
    inline void Port::attachPullDriver( PullDataDriver* pd ){m_pdDriver=pd;}
    inline PullDataDriver* Port::getPullDriver(){return m_pdDriver;}

    /**************************************
     * Our mailbox
     ***************************************/
    inline OCPI::OS::uint32_t        Port::getMailbox(){return m_mailbox;}

  }
}


#endif
