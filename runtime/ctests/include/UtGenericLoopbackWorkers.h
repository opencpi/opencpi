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
 * Include file for generic loopback workers.
 *
 * 06/01/09 - John Miller
 * Initial Version
 *
 */


#ifndef UT_GENERIC_ZEROCOPY_IO_WORKERS_H
#define UT_GENERIC_ZEROCOPY_IO_WORKERS_H

#include <RCC_Worker.h>

#ifdef __cplusplus
using namespace OCPI::RCC;
#endif

#define ERROR_TEST_STRING "Forced Error string for test"


struct  UTGProducerWorkerProperties_ {
  uint32_t run2BufferCount;
  uint32_t buffersProcessed;
  uint32_t bytesProcessed;
  uint32_t testControlErrors;
  uint32_t testConfigErrors;

  uint32_t lastProp;
};
typedef struct UTGProducerWorkerProperties_ UTGProducerWorkerProperties;

struct  UTGConsumerWorkerProperties_ {
  uint32_t passfail;
  uint32_t run2BufferCount;
  uint32_t buffersProcessed;
  uint32_t droppedBuffers;
  uint32_t bytesProcessed;
  uint32_t testControlErrors;
  uint32_t testConfigErrors;

  uint32_t lastProp;
};
typedef struct UTGConsumerWorkerProperties_ UTGConsumerWorkerProperties;

struct UTGConsumerWorkerStaticMemory_ {
        uint32_t     place_holder;
#ifdef TIME_TP
        Timespec        startTime;
#endif
};
typedef struct UTGConsumerWorkerStaticMemory_ UTGConsumerWorkerStaticMemory;

struct UTGLoopbackWorkerProperties_ {
  int32_t transferMode;
  uint32_t testControlErrors;
  uint32_t testConfigErrors;

  uint32_t lastProp;
};

typedef struct UTGLoopbackWorkerProperties_ UTGLoopbackWorkerProperties;
struct UTGLoopbackWorkerStaticMemory_ {
    uint32_t ph;
};
typedef struct UTGLoopbackWorkerStaticMemory_ UTGLoopbackWorkerStaticMemory;



#ifdef __cplusplus
extern "C" {
extern RCCDispatch UTGProducerWorkerDispatchTable;
extern RCCDispatch UTGConsumerWorkerDispatchTable;
extern RCCDispatch UTGLoopbackWorkerDispatchTable;
};
#else
extern RCCDispatch UTGProducerWorkerDispatchTable;
extern RCCDispatch UTGConsumerWorkerDispatchTable;
extern RCCDispatch UTGLoopbackWorkerDispatchTable;
#endif



#endif
