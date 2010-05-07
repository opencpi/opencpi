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
 * Include file for zero copy workers.
 *
 * 06/01/09 - John Miller
 * Initial Version
 *
 */



#ifndef UT_ZEROCOPY_IO_WORKERS_H
#define UT_ZEROCOPY_IO_WORKERS_H

#include <RCC_Worker.h>


#define ProducerSend    0
#define ProducerAdvance 1

struct  ProducerWorkerProperties_ {
  uint32_t run2BufferCount;
  uint32_t buffersProcessed;
  uint32_t bytesProcessed;
  int32_t transferMode;
};




typedef struct ProducerWorkerProperties_ ProducerWorkerProperties;


#define ConsumerConsume 0 
#define ConsumerTake    1

#define MIN_CONSUMER_BUFFERS 4

/*
struct  ConsumerWorkerProperties_ {
  uint32_t passfail;
  uint32_t run2BufferCount;
  uint32_t buffersProcessed;
  uint32_t droppedBuffers;
  uint32_t bytesProcessed;
  int32_t transferMode;
  RCCBuffer takenBuffers[MIN_CONSUMER_BUFFERS];
  uint32_t takenBufferIndex;
  uint32_t releaseBufferIndex;
};
*/

struct  ConsumerWorkerProperties_ {
  RCCDouble    doubleT;
  uint32_t     passfail;
  uint32_t     run2BufferCount;
  uint64_t     longlongT;
  uint32_t     buffersProcessed;
  uint32_t     droppedBuffers;
  uint32_t     floatCountT;
  RCCFloat     floatST[32];
  uint32_t     bytesProcessed;
  int32_t      transferMode;
  uint32_t     stringCountST;
  RCCChar      stringST[256][50];
  uint32_t     takenBufferIndex;
  uint32_t     releaseBufferIndex;
  RCCBoolean   boolT;
  RCCBuffer    takenBuffers[MIN_CONSUMER_BUFFERS];
};

typedef struct ConsumerWorkerProperties_ ConsumerWorkerProperties;

struct ConsumerWorkerStaticMemory_ {
  uint32_t     place_holder;
#ifdef TIME_TP
  Timespec        startTime;
#endif
};
typedef struct ConsumerWorkerStaticMemory_ ConsumerWorkerStaticMemory;


#define LBZCopyOnly    0
#define        LBAdvanceOnly  1
#define        LBSendOnly     2
#define        LBMix          3


struct LoopbackWorkerProperties_ {
  int32_t transferMode;
};

typedef struct LoopbackWorkerProperties_ LoopbackWorkerProperties;
struct LoopbackWorkerStaticMemory_ {
  uint32_t ph;
};
typedef struct LoopbackWorkerStaticMemory_ LoopbackWorkerStaticMemory;



#ifdef __cplusplus
extern "C" {
  extern RCCDispatch UTZCopyProducerWorkerDispatchTable;
  extern RCCDispatch UTZCopyConsumerWorkerDispatchTable;
  extern RCCDispatch UTZCopyLoopbackWorkerDispatchTable;
};
#else
extern RCCDispatch UTZCopyProducerWorkerDispatchTable;
extern RCCDispatch UTZCopyConsumerWorkerDispatchTable;
extern RCCDispatch UTZCopyLoopbackWorkerDispatchTable;
#endif



#endif
