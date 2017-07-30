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

/**
  @file
    Contains the declarations of the OCL Worker interface.

************************************************************************** */

#ifndef INCLUDED_OCPI_OCL_WORKER_INTERFACE_H
#define INCLUDED_OCPI_OCL_WORKER_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __OPENCL_VERSION__
  #define OCL_CONST const
  #define OCL_GLOBAL __global
  #pragma OPENCL EXTENSION cl_khr_fp64: enable
  typedef char int8_t;
  typedef unsigned char uint8_t;
  typedef short int16_t;
  typedef unsigned short uint16_t;
  typedef int int32_t;
  typedef unsigned int uint32_t;
  typedef unsigned long long uint64_t;
  typedef long long int64_t;
#else
  #include <stdint.h>
  #define OCL_CONST
  #define OCL_GLOBAL
#endif

typedef char OCLChar;
typedef float OCLFloat;
typedef double OCLDouble;
typedef unsigned int OCLOrdinal; /* Must be 32-bit to be a kernel argument */
typedef unsigned int OCLBoolean; /* Must be 32-bit to be a kernel argument */
typedef uint32_t OCLOpCode;

typedef enum {
  OCL_OK,
  OCL_ERROR,
  OCL_FATAL,
  OCL_DONE,
  OCL_ADVANCE
} OCLResult;

typedef enum {
  OCPI_OCL_INITIALIZE = 0,
  OCPI_OCL_START,
  OCPI_OCL_STOP,
  OCPI_OCL_RELEASE,
  OCPI_OCL_BEFORE_QUERY,
  OCPI_OCL_AFTER_CONFIGURE,
  OCPI_OCL_TEST,
  OCPI_OCL_RUN
} OCLControlOp;

typedef uint64_t OCLTime;
typedef uint32_t OCLPortMask;

typedef struct {
  OCLBoolean  timeout;
  OCLTime     usecs;
  OCLBoolean  usePorts;
  OCLPortMask portMasks[4];
} OCLRunCondition;

typedef struct {
  OCLResult        result;
  OCLRunCondition  runCondition;
  OCLBoolean       newRunCondition;
  uint16_t         crew_size;
  uint16_t         crew_rank;
  OCL_GLOBAL void *memory;
  /* then are memories */
  /* then are ports */
  /* then are properties */
} OCLWorker;

typedef struct {
  uint32_t length; // the number of elements
  uint32_t offset; // offset in the "whole" argument in the "whole message".
  uint32_t left; // left overlap
  uint32_t right; // right overlap
} OCLPartInfo;

typedef struct {
  OCL_GLOBAL void*       data;
  OCL_CONST uint32_t     maxLength;           // maximum message size that will fit
  uint32_t               length;              // const on input, set by worker on output
  OCLOpCode              opCode;              // const on input, set by worker on output
  OCLPartInfo           *partInfo;            // part info associated with message, if any
} OCLBuffer;

typedef struct {
  OCLBuffer            current;               // The current buffer (not a pointer to it)
  OCL_CONST uint32_t   maxLength;             // max for all buffers and messages at this port
  OCL_CONST OCLBoolean isConnected;           // is this (optional) port connected to something?
  uint32_t             defaultLength;         // set by worker to avoid setting it all the time
  OCLOpCode            defaultOpCode;         // set by worker to avoid setting it all the time
  OCL_CONST uint32_t   connectedCrewSize;     // if connected, now is the other side scaled
  OCL_CONST uint32_t   dataValueWidthInBytes; // for work group computations sometimes...
} OCLPort;

void OCLRelease ( OCLBuffer* buffer );

void OCLSend ( OCLPort* port,
               OCLBuffer* buffer,
               OCLOrdinal op,
               unsigned int length );

OCLBoolean OCLRequest ( OCLPort* port,
                        unsigned int max );

OCLBoolean OCLAdvance ( OCLPort* port,
                        unsigned int max );

void OCLTake ( OCLPort* port,
               OCLBuffer* releaseBuffer,
               OCLBuffer* takenBuffer);

#ifdef __cplusplus
}
#endif

#endif /* End: #ifndef INCLUDED_OCPI_OCL_WORKER_INTERFACE_H */
