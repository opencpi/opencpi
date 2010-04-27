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


#ifndef PRODUCER_WORKER_H
#define PRODUCER_WORKER_H


#include <RCCWorker.h>

#ifdef RAND_MAX
#undef RAND_MAX
#endif

#define RAND_MAX 31

struct ProducerWorkerProperties_ {
  WorkerLong startIndex;
  WorkerLong longProperty;

};

typedef struct ProducerWorkerProperties_ ProducerWorkerProperties;


struct ProducerWorkerStaticMemory_ {
	WorkerULong startIndex;
	char overlap[100];
	WorkerULong b_count;
	WorkerLong longProperty;
};

typedef struct ProducerWorkerStaticMemory_ ProducerWorkerStaticMemory;


#ifdef __cplusplus
extern "C" {
  extern WorkerDispatch CoverageProducerWorkerDispatchTable;
};
#else
  extern WorkerDispatch CoverageProducerWorkerDispatchTable;
#endif

#endif
