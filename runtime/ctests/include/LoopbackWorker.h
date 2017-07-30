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

#ifndef LOOPBACK_WORKER_H
#define LOOPBACK_WORKER_H


#include <RCC_Worker.h>


#ifdef RAND_MAX
#undef RAND_MAX
#endif

#define RAND_MAX 31

struct LoopbackWorkerProperties_ {
  uint32_t startIndex;
  uint32_t longProperty;

};

typedef struct LoopbackWorkerProperties_ LoopbackWorkerProperties;

typedef enum  {
        LB_Old_Input,
        LB_New_Input
}LB_MyWorkerState;

struct LoopbackWorkerStaticMemory_ {
        uint32_t startIndex;
    LB_MyWorkerState state;
        char overlap[100];
        uint32_t overlapSize;
        uint32_t longProperty;
};

typedef struct LoopbackWorkerStaticMemory_ LoopbackWorkerStaticMemory;


#ifdef __cplusplus
extern "C" {
  extern OCPI::RCC::RCCDispatch LoopbackWorkerDispatchTable;
};
#else
extern RCCDispatch LoopbackWorkerDispatchTable;
#endif

#endif
