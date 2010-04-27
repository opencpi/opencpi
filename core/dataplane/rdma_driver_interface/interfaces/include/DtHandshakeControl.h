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
 *   This file contains the class that controls the setup for handshake within the CPI transfer protocol.
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
  const CPI::OS::uint64_t UpAndRunningMarker = 0x51abac;
  const CPI::OS::uint32_t BufferEmptyFlag    = 0x0fffffff; 

  // Token definition
  typedef CPI::OS::int32_t ControlToken;
  typedef CPI::OS::int32_t BooleanToken;

  
  /**********************************
   *  SMB communications structures
   *
   *	 This structure is used to make inband requests to other SMB's.
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
      CPI::OS::uint32_t  pad;
    };
	 
    struct RequestUpdateCircuit {
      ReqTypeIds         type;
      CPI::OS::uint32_t  pad;
      CPI::OS::uint32_t       senderCircuitId;	      // Id of circuit of interest
      CPI::OS::uint32_t       receiverCircuitId;      // Id of circuit of interest
      CPI::OS::uint64_t  receiverPortId;	    // Our new input port id
      CPI::OS::uint32_t	 tPortCount;	            // Number of input ports
      CPI::OS::uint64_t	 senderPortId;             // Sender port ordinal
      CPI::OS::uint64_t  senderOutputPortId;       // Our output port ordinal
      CPI::OS::uint32_t	 senderOutputControlOffset; // Control offset to output
      CPI::OS::uint32_t  pad1;
      char               output_end_point[128];   // Output endpoint
    };

    struct RequestShadowRstateOffset {
      ReqTypeIds         type;
      CPI::OS::uint32_t  pad;
      CPI::OS::uint32_t  circuitId;			            
      CPI::OS::uint32_t  portId;				
      CPI::OS::uint32_t  pad1;
      char               url[128];                   
    };

    struct RequestInputOffsets {
      ReqTypeIds         type;
      CPI::OS::uint32_t  pad;
      CPI::OS::uint32_t  circuitId;			            
      CPI::OS::uint32_t  portId;				
      CPI::OS::uint32_t       pad1;
      char               url[128];                   
    };

    struct RequestOutputControlOffset {
      ReqTypeIds                type;
      CPI::OS::uint32_t         pad;
      CPI::OS::uint32_t  circuitId;			            
      CPI::OS::uint32_t  portId;				
      char                      shadow_end_point[128];       // Output endpoint
    };

    struct RequestNewConnection {
      ReqTypeIds                type;
      CPI::OS::uint32_t         pad;
      CPI::OS::uint32_t         circuitId;
      CPI::OS::uint32_t         buffer_size;
      CPI::OS::uint32_t		  send;				// Send or recieve buffer
      CPI::OS::uint32_t         pad1;
      char                      output_end_point[128];
    };

    union RequestTypes {
      BasicReq				    reqBasic;
      RequestInputOffsets        reqInputOffsets;
      RequestShadowRstateOffset   reqShadowOffsets;
      RequestOutputControlOffset  reqOutputContOffset;
      RequestNewConnection        reqNewConnection;
      RequestUpdateCircuit        reqUpdateCircuit;
    };
	  
    struct MailBox {
      RequestTypes            request;
      CPI::OS::int32_t	      error_code;
      CPI::OS::uint32_t	      returnMailboxId;
      CPI::OS::int32_t	      return_offset;
      CPI::OS::uint32_t	      return_size;
    };
	  
    CPI::OS::int64_t	       upAndRunning;
    MailBox		       mailBox[MAX_SYSTEM_SMBS];
	  
  };


  /**********************************
   *  Output port control structure
   *********************************/
  struct OutputPortSetControl {

    // This token is used to control what output port can transfer
    ControlToken    sequentialControlToken;

    BooleanToken   endOfWhole;           // end of whole data distribution
    BooleanToken   endOfStream;          // end of data stream

    CPI::OS::uint32_t           numberOfBuffers;
  };


  struct BufferState {
    // buffer full token
    CPI::OS::int32_t   bufferFull;
  };
  

  struct BufferShape
  {
    /* Return error value */
    CPI::OS::uint32_t error;

    /* Buffer handle */
    CPI::OS::uint32_t buffer;

    /* Number of dimensions */
    CPI::OS::uint32_t ndims;
  
    /* Data shape size */
    CPI::OS::uint32_t dataShape[3];

    /* Whole shape size */
    CPI::OS::uint32_t wholeShape[3];

    /* Left overlap sizes */
    CPI::OS::uint32_t left[3];

    /* Right overlap sizes */
    CPI::OS::uint32_t right[3];

    /* Position of data in whole */
    CPI::OS::uint32_t wholePosition[3];

    /* Strides of data in buffer */
    CPI::OS::uint32_t dataInBuffer[3];

    /* Offset from start of buffer to start of data */
    CPI::OS::uint32_t dataBufferOffset;
  };



  /**********************************
   *  Buffer meta-data
   *********************************/
  const unsigned int ZeroCopyReady = 0x10000000;

#define N_BYTES_TRANSFERED(v) ( v & 0xffffffff )
#define DECODE_OPCODE(v) ( (v>>32) & 0xffffffff )
#define SET_BYTES_TRANSFERED(l,bytes) l = ((l & 0xffffffff00000000) | bytes);


  struct BufferMetaData {
    CPI::OS::uint64_t        cpiMetaDataWord;      // CPI compatible metadata word


    CPI::OS::uint32_t	   sequence;			    // Transfer sequence
    CPI::OS::int32_t	   userTag;			    // User defined buffer tag
    CPI::OS::int32_t	   endOfCircuit;		    // Circuit is being deleted
    CPI::OS::int32_t	   broadCast;			// This buffer was broadcast to all inputs
    CPI::OS::int32_t       metaDataOnlyTransfer; // Only meta data transfered
    CPI::OS::uint32_t      srcRank;              // rank of the output buffer
    CPI::OS::int32_t       srcTemporalId;        // temporal buffer id
    BooleanToken           endOfWhole;           // end of whole data distribution
    CPI::OS::uint32_t	   nPartsPerWhole;		// Number of parts to make up the whole data set
    CPI::OS::uint32_t	   partsSequence;	    // This buffers sequence in the whole
    BooleanToken           endOfStream;          // end of data stream
    CPI::OS::int32_t	   timeStamp;			// Buffer time stamp
    DtOsDataTypes::Offset  localStateOffset;     // offset back to local state
    CPI::OS::int32_t       outputSmbId;          // Output smb id
    BufferShape	           shape;				// buffer shape
    CPI::OS::uint32_t  	   zcopy;


    volatile BufferMetaData& operator =(volatile BufferMetaData*);
  };




  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

  inline volatile BufferMetaData& BufferMetaData::operator =(volatile BufferMetaData* t)
    {
      cpiMetaDataWord           = t->cpiMetaDataWord;
      endOfCircuit              = t->endOfCircuit;
      broadCast			= t->broadCast;
      metaDataOnlyTransfer	= t->metaDataOnlyTransfer;
      srcRank			= t->srcRank;
      endOfWhole		= t->endOfWhole;
      nPartsPerWhole		= t->nPartsPerWhole;
      partsSequence		= t->partsSequence;
      endOfStream		= t->endOfStream;
      userTag			= t->userTag;
      timeStamp			= t->timeStamp;
      localStateOffset		= t->localStateOffset;
      outputSmbId		= t->outputSmbId;
      sequence			= t->sequence;
      return *this;
    }

}

#endif
