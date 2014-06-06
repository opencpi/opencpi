
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



#ifndef PRODUCER_WORKER_H
#define PRODUCER_WORKER_H


#include <RCC_Worker.h>

#ifdef RAND_MAX
#undef RAND_MAX
#endif

#define RAND_MAX 31

typedef enum  {
        P_Old_Input,
        P_New_Input
}P_MyWorkerState;

struct ProducerWorkerStaticMemory_ {
        uint32_t startIndex;
    P_MyWorkerState state;
        char overlap[100];
        uint32_t b_count;
        int32_t longProperty;
};

typedef struct ProducerWorkerStaticMemory_ ProducerWorkerStaticMemory;


#ifdef __cplusplus
extern "C" {
  extern OCPI::RCC::RCCDispatch ProducerWorkerDispatchTable;
};
#else
extern RCCDispatch ProducerWorkerDispatchTable;
#endif



#endif
