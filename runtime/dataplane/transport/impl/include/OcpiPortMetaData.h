
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
 *   This file contains the Interface for the Ocpi port meta data class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_PortMetaData_H_
#define OCPI_DataTransport_PortMetaData_H_

#include "OcpiParentChild.h"
#include "DtHandshakeControl.h"
#include "OcpiTransportConstants.h"
#include "OcpiRDTInterface.h"

namespace OU = OCPI::Util;

namespace DataTransfer {
  class XferFactory;
  struct EndPoint;
}

namespace OCPI {

  namespace DataTransport {

    // Forward references
    struct PortSetMetaData;
    class PortSet;

    // Port Ordinal
    typedef DataTransfer::PortId PortOrdinal;

    struct PortMetaData : public OU::Child<PortSetMetaData,PortMetaData>
    {

      // Are we a shadow port >
      bool m_shadow;

      // Location of the real port
      std::string real_location_string;

      // Our location if we are a shadow port
      std::string shadow_location_string;

      // Remote Circuit id if we are a shadow
      int32_t       remoteCircuitId;
      PortOrdinal   remotePortId;

      // port id
      PortOrdinal id;

      // Our rank in the port set
      uint32_t rank;

      // Output port
      bool output;

      // user data
      void* user_data;

      // Our port set metadata
      PortSetMetaData* m_portSetMd;

      // Data initialized
      bool m_init;

      // Our location, host, etc.
      DataTransfer::EndPoint    * m_real_location;
      DataTransfer::XferFactory * m_real_tfactory;
      DataTransfer::EndPoint    * m_shadow_location;
      DataTransfer::XferFactory * m_shadow_tfactory;

      // Ports local "port set" control structure offset
      OU::ResAddr m_localPortSetControl;

      // Offsets for port communication structures
      struct OutputPortBufferControlMap {
        OU::ResAddr bufferOffset;          // offset to our buffer
	OU::ResAddr bufferSize;            // Buffer size in bytes
        OU::ResAddr localStateOffset;      // offset to our state structure
        OU::ResAddr metaDataOffset;        // offset to our meta-data structure
        OU::ResAddr portSetControlOffset;  // offset to our port set control structure
      };

      // In this structure, the number of offsets for the remote state and meta data
      // is equal to the number of ports in the output port set.
      struct InputPortBufferControlMap {
        OU::ResAddr bufferOffset;          // offset to our buffer
	OU::ResAddr bufferSize;            // Buffer size in bytes
        /*
         *  The input buffers need N number of local states where N is the number of 
         *  output ports that can write to the input.  We will create a contiguous array
         *  of states so we only need 1 offset
         */
        OU::ResAddr localStateOffset; // offset to our state structure

        // Each output that can write data to our buffer will also write its meta-data here.
        // This array is also indexed by the output port id.  We will create a contiguous array
        // so we only need 1 offset
        OU::ResAddr metaDataOffset;
        /*
         *  The remote state structure contains the offsets to all of our remote
         *  states.  The remote states exist in the "shadow" input buffers in our
         *  represented port for each circuit instance that exists.  The number of 
         *  circuit instances that exist is equal to the number of output ports * 
         *  the number of input ports, however the only ones that we need to be concerned
         *  with are the ones that exist in circuits that have "real" output ports.
         *
         *  If this is a shadow port, these are not initialized.
         */

        // Offsets to our remote "shadow" input ports states.  When we locally indicate 
        // that an input buffer is empty, we need to also inform all of the shadows (producers) that have
        // a "real" output port.   This array is indexed by the port id, so only the output
        // port id's are valid.
	// It is the remotely mappable offset in an endpoint, hence the type is DtOsDataTypes::Offset
	DtOsDataTypes::Offset myShadowsRemoteStateOffsets[MAX_PCONTRIBS]; 
      };

      union BufferOffsets {
        struct OutputPortBufferControlMap outputOffsets;
        struct InputPortBufferControlMap inputOffsets;
      };

      // Our local "real" descriptor
      OCPI::RDT::Descriptors m_descriptor;

      // This is our shadow ports descriptor that we will pass to allow external ports to connect to us.
      OCPI::RDT::Descriptors m_shadowPortDescriptor;

      // This is the descriptor that we get from an external port that we are attempting to connect to.
      OCPI::RDT::Descriptors m_externPortDependencyData;

      struct OcpiPortDependencyData  {
        BufferOffsets *offsets;  // buffer offsets
      };

      // Here is our buffer offset information.  This is an array that is "N" buffers deep
      BufferOffsets *m_bufferData;

      // Standard constructors
      PortMetaData( PortOrdinal pid, 
                    bool s, 
		    DataTransfer::EndPoint *ep,
                    const OCPI::RDT::Descriptors& sPort,
                    PortSetMetaData* psmd );

      PortMetaData( PortOrdinal pid, 
                    bool output,
                    DataTransfer::EndPoint *ep, 
                    DataTransfer::EndPoint *shadow_ep,
                    PortSetMetaData* psmd );



      PortMetaData( PortOrdinal pid, 
                    DataTransfer::EndPoint &ep, 
                    const OCPI::RDT::Descriptors& portDesc,
                    PortSetMetaData* psmd );

      // Dependency constructor
      PortMetaData( PortOrdinal pid, 
                    DataTransfer::EndPoint &ep, 
                    DataTransfer::EndPoint &shadow_ep, 
                    const OCPI::RDT::Descriptors& pd, 
		    //                    uint32_t circuitId,
                    PortSetMetaData* psmd );

      virtual ~PortMetaData();

      /**********************************
       * Common init
       **********************************/
      void init();

    };

    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

  }

}

#endif

