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

#ifndef RCC_WORKER_FILESINK_H__
#define RCC_WORKER_FILESINK_H__

/*
 * Header file for worker FileSink.
 * Generated at 2009-06-23 09:04:52
 * from FileSink.scd.xml
 */

#include <RCC_Worker.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*
 * Worker port ordinals.
 */

enum FileSinkPortOrdinal {
  FILESINK_DATAIN
};

#define FILESINK_N_INPUT_PORTS 1
#define FILESINK_N_OUTPUT_PORTS 0

/*
 * Worker property space.
 */

typedef struct {
  RCCChar portName[256];
  RCCChar fileName[256];
  uint32_t offset;
  uint32_t errnoValue;
  RCCBoolean verbose;
} FileSinkProperties;

#if defined (__cplusplus)
}
#endif
#endif
