
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



#ifndef CONSUMER_WORKER_H
#define CONSUMER_WORKER_H


#include <RCC_Worker.h>

#ifndef WIN32
//#define TIME_TP
#ifdef TIME_TP
#include <time_utils.h>
#endif
#endif

#ifdef RAND_MAX
#undef RAND_MAX
#endif

#define RAND_MAX 31

struct  ConsumerWorkerProperties_ {
  uint32_t startIndex;
  uint32_t longProperty;
};
typedef struct ConsumerWorkerProperties_ ConsumerWorkerProperties;


typedef enum  {
        C_Old_Input,
        C_New_Input
} C_MyWorkerState;

struct ConsumerWorkerStaticMemory_ {
        uint32_t     startIndex;
    C_MyWorkerState state;
        uint32_t     b_count;
        uint32_t      longProperty;

#ifdef TIME_TP
        Timespec        startTime;
#endif

};

typedef struct ConsumerWorkerStaticMemory_ ConsumerWorkerStaticMemory;


#ifdef __cplusplus
extern "C" {
extern RCCDispatch ConsumerWorkerDispatchTable;
};
#else
extern RCCDispatch ConsumerWorkerDispatchTable;
#endif






#endif
