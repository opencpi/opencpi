
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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
  #define OCL_GLOBAL __global
  #pragma OPENCL EXTENSION cl_khr_fp64: enable
  typedef char int8_t;
  typedef unsigned char uint8_t;
  typedef short int16_t;
  typedef unsigned short uint16_t;
  typedef int int32_t;
  typedef unsigned int uint32_t;
#else
  #define OCL_GLOBAL
#endif

typedef char OCLChar;
typedef float OCLFloat;
typedef double OCLDouble;
typedef unsigned int OCLOrdinal; /* Must be 32-bit to be a kernel argument */
typedef unsigned int OCLBoolean; /* Must be 32-bit to be a kernel argument */

typedef enum
{
  OCL_OK,
  OCL_ERROR,
  OCL_FATAL,
  OCL_DONE,
  OCL_ADVANCE

} OCLResult;

typedef unsigned int OCLTime;
typedef unsigned int OCLPortMask;
typedef struct OCLPort OCLPort;
typedef struct OCLWorker OCLWorker;

typedef struct
{
  OCLPortMask portMasks;
  OCLBoolean timeout;
  OCLTime usecs;

} OCLRunCondition;

typedef struct
{
  OCL_GLOBAL void* data;
  unsigned int maxLength;

} OCLBuffer;

typedef struct
{
  OCLBoolean optional;
  OCLBoolean connected;
  unsigned int length;
  union
  {
    OCLOrdinal operation;
    OCLOrdinal exception;
  } u;

} OCLPortAttr;

struct OCLPort
{
  OCLPortAttr attr;
  OCLBuffer current;

} /* OCLPort */;

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
