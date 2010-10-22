
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
 *   This file contains the class that controls the setup for handshake within the OCPI transfer protocol.
 *
 * Revision History:
 *
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef DataTransfer_HandShakeControl_H_
#define DataTransfer_HandShakeControl_H_

#include <DtSharedMemoryInterface.h>
#include <DtOsDataTypes.h>

namespace DataTransfer {

  // This is the const value that is used to determine if a SMB transport
  // is up and running
  const OCPI::OS::uint64_t UpAndRunningMarker = 0x51abac;

  // Token definition
  typedef OCPI::OS::int32_t ControlToken;
  typedef OCPI::OS::int32_t BooleanToken;


  /**********************************
   *  SMB communications structures
   *
   *         This structure is used to make inband requests to other SMB's.
   *    We use our mailbox id to index into our local SMB as the output
   *  of our request.  A non-zero in the request indicates that a request
   *  is in process.  If this local SMB is used in a multi-threaded environment
   *  this structure needs mutex protection.
   *********************************/
  const int GetPortOffsets = 1;

  struct ContainerComms {

    enum ReqTypeIds {
      NoRequest = 0,
      ReqShadowRstateOffset=1,
      ReqInputOffsets=2,
      ReqOutputControlOffset=3,
      ReqNewConnection = 4,
      ReqUpdateCircuit = 5
    };

    struct BasicReq {
      ReqTypeIds         type;
      OCPI::OS::uint32_t  pad;
    };

    struct RequestUpdateCircuit {
      ReqTypeIds         type;
      OCPI::OS::uint32_t  pad;
      OCPI::OS::uint32_t       senderCircuitId;              // Id of circuit of interest
      OCPI::OS::uint32_t       receiverCircuitId;      // Id of circuit of interest
      OCPI::OS::uint64_t  receiverPortId;            // Our new input port id
      OCPI::OS::uint32_t         tPortCount;                    // Number of input ports
      OCPI::OS::uint64_t         senderPortId;             // Sender port ordinal
      OCPI::OS::uint64_t  senderOutputPortId;       // Our output port ordinal
      OCPI::OS::uint32_t         senderOutputControlOffset; // Control offset to output
      OCPI::OS::uint32_t  pad1;
      char               output_end_point[128];   // Output endpoint
    };

    struct RequestShadowRstateOffset {
      ReqTypeIds         type;
      OCPI::OS::uint32_t  pad;
      OCPI::OS::uint32_t  circuitId;
      OCPI::OS::uint32_t  portId;
      OCPI::OS::uint32_t  pad1;
      char               url[128];
    };

    struct RequestInputOffsets {
      ReqTypeIds         type;
      OCPI::OS::uint32_t  pad;
      OCPI::OS::uint32_t  circuitId;
      OCPI::OS::uint32_t  portId;
      OCPI::OS::uint32_t       pad1;
      char               url[128];
    };

    struct RequestOutputControlOffset {
      ReqTypeIds                type;
      OCPI::OS::uint32_t         pad;
      OCPI::OS::uint32_t  circuitId;
      OCPI::OS::uint32_t  portId;
      char                      shadow_end_point[128];       // Output endpoint
    };

    struct RequestNewConnection {
      ReqTypeIds                type;
      OCPI::OS::uint32_t         pad;
      OCPI::OS::uint32_t         circuitId;
      OCPI::OS::uint32_t         buffer_size;
      OCPI::OS::uint32_t                  send;                                // Send or recieve buffer
      OCPI::OS::uint32_t         pad1;
      char                      output_end_point[128];
    };

    union RequestTypes {
      BasicReq                                    reqBasic;
      RequestInputOffsets        reqInputOffsets;
      RequestShadowRstateOffset   reqShadowOffsets;
      RequestOutputControlOffset  reqOutputContOffset;
      RequestNewConnection        reqNewConnection;
      RequestUpdateCircuit        reqUpdateCircuit;
    };

    struct MailBox {
      RequestTypes            request;
      OCPI::OS::int32_t              error_code;
      OCPI::OS::uint32_t              returnMailboxId;
      OCPI::OS::int32_t              return_offset;
      OCPI::OS::uint32_t              return_size;
    };

    OCPI::OS::int64_t               upAndRunning;
    MailBox                       mailBox[MAX_SYSTEM_SMBS];

  };


  /**********************************
   *  Output port control structure
   *********************************/
  struct OutputPortSetControl {

    // This token is used to control what output port can transfer
    ControlToken    sequentialControlToken;

    BooleanToken   endOfWhole;           // end of whole data distribution
    BooleanToken   endOfStream;          // end of data stream

    OCPI::OS::uint32_t           numberOfBuffers;
  };


  struct BufferState {
    // buffer full token
    OCPI::OS::int32_t   bufferFull;
  };


  struct BufferShape
  {
    /* Return error value */
    OCPI::OS::uint32_t error;

    /* Buffer handle */
    OCPI::OS::uint32_t buffer;

    /* Number of dimensions */
    OCPI::OS::uint32_t ndims;

    /* Data shape size */
    OCPI::OS::uint32_t dataShape[3];

    /* Whole shape size */
    OCPI::OS::uint32_t wholeShape[3];

    /* Left overlap sizes */
    OCPI::OS::uint32_t left[3];

    /* Right overlap sizes */
    OCPI::OS::uint32_t right[3];

    /* Position of data in whole */
    OCPI::OS::uint32_t wholePosition[3];

    /* Strides of data in buffer */
    OCPI::OS::uint32_t dataInBuffer[3];

    /* Offset from start of buffer to start of data */
    OCPI::OS::uint32_t dataBufferOffset;
  };



  /**********************************
   *  Buffer meta-data
   *********************************/
  const unsigned int ZeroCopyReady = 0x10000000;

  struct RplMetaData {
    uint32_t
    length,
      opCode,
      tag,
      interval;
  };


  struct BufferMetaData {
    RplMetaData              ocpiMetaDataWord;      // OCPI compatible metadata word

    OCPI::OS::uint32_t           sequence;                            // Transfer sequence
    OCPI::OS::int32_t           userTag;                            // User defined buffer tag
    OCPI::OS::int32_t           endOfCircuit;                    // Circuit is being deleted
    OCPI::OS::int32_t           broadCast;                        // This buffer was broadcast to all inputs
    OCPI::OS::int32_t       metaDataOnlyTransfer; // Only meta data transfered
    OCPI::OS::uint32_t      srcRank;              // rank of the output buffer
    OCPI::OS::int32_t       srcTemporalId;        // temporal buffer id
    BooleanToken           endOfWhole;           // end of whole data distribution
    OCPI::OS::uint32_t           nPartsPerWhole;                // Number of parts to make up the whole data set
    OCPI::OS::uint32_t           partsSequence;            // This buffers sequence in the whole
    BooleanToken           endOfStream;          // end of data stream
    OCPI::OS::int32_t           timeStamp;                        // Buffer time stamp
    DtOsDataTypes::Offset  localStateOffset;     // offset back to local state
    OCPI::OS::int32_t       outputSmbId;          // Output smb id
    BufferShape                   shape;                                // buffer shape
    OCPI::OS::uint32_t             zcopy;


  };




  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

}

#endif
