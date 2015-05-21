
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
 * Include file for zero copy workers.
 *
 * 06/01/09 - John Miller
 * Initial Version
 *
 */



#ifndef UT_ZEROCOPY_IO_WORKERS_H
#define UT_ZEROCOPY_IO_WORKERS_H

#include <RCC_Worker.h>

#ifdef __cplusplus
using namespace OCPI::RCC;
#endif
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

// This must not exceed the minimum that is explicitly requested or defined in metadata
#define MIN_CONSUMER_BUFFERS 1
#define CONSUMER_TAKE_COUNT 2

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
  RCCBuffer    takenBuffers[CONSUMER_TAKE_COUNT];
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
