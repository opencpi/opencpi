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

#ifdef __OPENCL_VERSION__
  #define OCL_CONST const
  #define OCL_GLOBAL __global
//  #if __OPENCL_VERSION__ < CL_VERSION_1_2
  #pragma OPENCL EXTENSION cl_khr_fp64: enable
//  #endif
  typedef char int8_t;
  typedef unsigned char uint8_t;
  typedef short int16_t;
  typedef unsigned short uint16_t;
  typedef int int32_t;
  typedef unsigned int uint32_t;
  typedef unsigned long uint64_t;
  typedef long int64_t;
#else
  #include <stdint.h>
  #define OCL_CONST
  #define OCL_GLOBAL
#endif

#ifdef __cplusplus
namespace OCPI {
  namespace OCL {
#endif

typedef char OCLChar;
typedef float OCLFloat;
//typedef double OCLDouble;
typedef uint32_t OCLOrdinal;
typedef uint8_t OCLBoolean;
typedef uint8_t OCLOpCode;

typedef enum {
  OCL_OK,
  OCL_ERROR,
  OCL_FATAL,
  OCL_DONE,
  OCL_ADVANCE,
  OCL_ADVANCE_DONE
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
#define OCL_ALL_PORTS (~(OCLPortMask)0)
#define OCL_NO_PORTS ((OCLPortMask)0)

// This structure must consist of portably defined sizes
typedef struct {
  OCLBoolean  useDefault; // when "new", change to default
  OCLBoolean  useTimeout; // wait for usecs for period
  OCLBoolean  usePorts;   // wait for port masks
  OCLPortMask portMasks[4];
  OCLTime     usecs;
} OCLRunCondition;

// This structure is copied from the worker back to the container after execution
// This structure must consist of portably defined sizes
typedef struct {
  uint32_t         result;
  OCLBoolean       newRunCondition;
  OCLRunCondition  runCondition;
  /* then are properties */
  /* then are memories */
} OCLReturned;

#define OCL_SELF \
  uint16_t        crew_size; \
  uint16_t        member; \
  OCLBoolean      firstRun; \
  OCLBoolean      timedOut; \
  OCLOpCode       controlOp; \
  OCLPortMask     connectedPorts; \
  uint8_t         runCount; \
  uint8_t         nPorts; \
  uint8_t         logLevel; \
  uint8_t         kernelLogLevel;
// This structure is send to the worker as a kernel argument
typedef struct {
  OCL_SELF
} OCLWorker;

typedef struct {
  uint32_t length; // the number of elements
  uint32_t offset; // offset in the "whole" argument in the "whole message".
  uint32_t left; // left overlap
  uint32_t right; // right overlap
} OCLPartInfo;

typedef struct {
  uint8_t          m_opCode; // Opcode for the message
  uint8_t          m_eof;
  uint8_t          m_data;   // Offset of data from this struct. 0 is for standalone eof
  uint8_t          m_direct;
  uint32_t         m_length; // size in bytes of the message
} OCLHeader;

typedef struct {
  union {
    OCL_GLOBAL void      *data;
    uint64_t              pad;      // ensure alignment on all architectures
  };
  uint32_t                length;   // const on input, set by worker on output
  OCLOpCode               opCode;   // const on input, set by worker on output
  OCLBoolean              end;      // const on input, set by worker on output
  union {
    OCL_CONST OCLHeader  *partInfo; // part info associated with message, if any
    uint64_t              pad1;     // ensure alignment on all architectures
  };
  union {
    OCL_GLOBAL OCLHeader *header;
    uint64_t              pad2; // ensure alignment on all architectures
  };
} OCLBuffer;

typedef struct {
  OCLBuffer            current;               // The current buffer (not a pointer to it)
  OCL_CONST uint32_t   maxLength;             // max for all buffers and messages at this port
  OCL_CONST OCLBoolean isConnected;           // is this (optional) port connected to something?
  OCL_CONST OCLBoolean isOutput;              // is this (optional) port connected to something?
  uint32_t             defaultLength;         // set by worker to avoid setting it all the time
  OCLOpCode            defaultOpCode;         // set by worker to avoid setting it all the time
  OCL_CONST uint32_t   connectedCrewSize;     // if connected, now is the other side scaled
  OCL_CONST uint32_t   dataValueWidthInBytes; // for work group computations sometimes...
  OCL_CONST uint32_t   bufferStride;
  uint32_t             readyOffset;
  OCL_CONST uint32_t   endOffset;
} OCLPort;

#if 0
// Not yet.
void OCLRelease ( OCLBuffer* buffer );
void OCLSend(OCLPort* port, OCLBuffer* buffer, OCLOrdinal op, unsigned int length);
OCLBoolean OCLRequest(OCLPort* port, unsigned int max);
OCLBoolean OCLAdvance(OCLPort* port, unsigned int max);
void OCLTake(OCLPort* port, OCLBuffer* releaseBuffer, OCLBuffer* takenBuffer);
#endif

#ifdef __cplusplus
  } }
#endif

#endif /* End: #ifndef INCLUDED_OCPI_OCL_WORKER_INTERFACE_H */
