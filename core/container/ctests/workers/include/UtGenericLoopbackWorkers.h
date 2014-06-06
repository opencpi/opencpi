
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
