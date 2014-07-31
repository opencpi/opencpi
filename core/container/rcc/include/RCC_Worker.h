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



/**
  @file

  @brief
    Contains the declarations of the RCC Worker interface as specified in
    the CP289U specification.

************************************************************************** */


#ifndef OCPI_RCC_WORKER__INTERFACE_H
#define OCPI_RCC_WORKER__INTERFACE_H

#include <stddef.h>

#if defined (WIN32)
    /**
     * 8 bit signed integer data type.
     */

    typedef signed char int8_t;

    /**
     * 8 bit unsigned integer data type.
     */

    typedef unsigned char uint8_t;

    /**
     * 16 bit signed integer data type.
     */

    typedef short int16_t;

    /**
     * 16 bit unsigned integer data type.
     */

    typedef unsigned short uint16_t;

    /**
     * 32 bit signed integer data type.
     */

    typedef int int32_t;

    /**
     * 32 bit unsigned integer data type.
     */

    typedef unsigned int uint32_t;

    /**
     * 64 bit signed integer data type.
     */

    typedef long long int64_t;

    /**
     * 64 bit unsigned integer data type.
     */

    typedef unsigned long long uint64_t;
#else
#ifndef _WRS_KERNEL
#include <stdint.h>
#endif
#endif

#ifdef WORKER_INTERNAL
namespace OCPI { namespace RCC { class Port; } namespace DataTransport { class BufferUserFacet;}}
#define RCC_CONST
#else
#define RCC_CONST const
#endif
#ifdef __cplusplus
namespace OCPI { namespace RCC {
#endif

typedef uint16_t  RCCOrdinal;
typedef uint8_t   RCCOpCode;
#ifdef __cplusplus
typedef bool RCCBoolean;
#else
typedef uint8_t   RCCBoolean;
#endif
typedef void      *RCCBufferId;
typedef float     RCCFloat;
typedef double    RCCDouble;
typedef char      RCCChar;

// do compile time checks for float, double, and char
#define RCC_VERSION 1
#define RCC_NO_EXCEPTION (0)
#define RCC_SYSTEM_EXCEPTION (1)
#define RCC_NO_ORDINAL ((RCCOrdinal)(-1))

typedef enum {
  RCC_OK,
  RCC_ERROR,
  RCC_FATAL,
  RCC_DONE,
  RCC_ADVANCE,
  RCC_ADVANCE_DONE
} RCCResult;

typedef uint32_t RCCPortMask;
typedef struct RCCWorker RCCWorker;
typedef struct RCCPort RCCPort;

typedef struct {
    RCCPortMask *portMasks;
    RCCBoolean  timeout;
    uint32_t    usecs;
} RCCRunCondition;

typedef RCCResult RCCMethod(RCCWorker *_this);
typedef RCCResult RCCRunMethod(RCCWorker *_this,
			       RCCBoolean timedout,
			       RCCBoolean *newRunCondition);
typedef RCCResult RCCPortMethod(RCCWorker *_this,
				RCCPort *port,
				RCCResult reason);
typedef struct {
  void *data;
  size_t maxLength;

  /* private member for container use */
  size_t length_;
  RCCOpCode opCode_;
#ifdef WORKER_INTERNAL
  OCPI::DataTransport::BufferUserFacet *containerBuffer;
#else
  void *id_; 
#endif
} RCCBuffer;

struct RCCPort {
  RCCBuffer current;

  RCC_CONST struct {
    size_t length;
    union {
      RCCOpCode operation;
      RCCOpCode exception;
    } u;
  } input;
  struct {
    size_t length;
    union {
      RCCOpCode operation;
      RCCOpCode exception;
    } u;
  } output;
  RCCPortMethod *callBack;

  /* Used by the container */
  RCCBoolean useDefaultLength_; // for C++, use the length field as default
  size_t defaultLength_;
  RCCBoolean useDefaultOpCode_; // for C++, use the length field as default
  RCCOpCode defaultOpCode_;
#ifdef WORKER_INTERNAL
  OCPI::RCC::Port *containerPort;
#else
  void* opaque;
#endif
};

typedef struct {
  void (*release)(RCCBuffer *);
  void (*send)(RCCPort *, RCCBuffer*, RCCOpCode op, size_t length);
  RCCBoolean (*request)(RCCPort *port, size_t maxlength);
  RCCBoolean (*advance)(RCCPort *port, size_t maxlength);
  RCCBoolean (*wait)(RCCPort *, size_t max, unsigned usecs);
  void (*take)(RCCPort *,RCCBuffer *old_buffer, RCCBuffer *new_buffer);
  RCCResult (*setError)(const char *, ...);
} RCCContainer;

struct RCCWorker {
  void * RCC_CONST         properties;
  void * RCC_CONST       * RCC_CONST memories;
  void * RCC_CONST         memory;
  RCC_CONST RCCContainer   container;
  RCCRunCondition        * runCondition;
  RCCPortMask              connectedPorts;
  char                   * errorString;
  RCCPort                  ports[1];
};


typedef struct {
  RCCOrdinal port;
  size_t maxLength;
  unsigned minBuffers;
  // others...
} RCCPortInfo;


#ifndef __cplusplus
typedef struct {
  /* Information for consistency checking */
  unsigned version, numInputs, numOutputs;
  size_t propertySize, *memSizes;
  RCCBoolean threadProfile;
  /* Methods */
  RCCMethod *initialize, *start, *stop, *release, *test,
    *afterConfigure, *beforeQuery;
  RCCRunMethod *run;
  /* Implementation information for container behavior */
  RCCRunCondition *runCondition;
  RCCPortInfo *portInfo;

  /* Mask indicating which ports may be left unconnected */
  RCCPortMask optionallyConnectedPorts;
  size_t memSize;
} RCCDispatch;

// Info common to C and C++
typedef struct {
  size_t size, memSize, *memSizes, propertySize;
  RCCPortInfo *portInfo;
  RCCPortMask optionallyConnectedPorts; // usually initialized from metadata
} RCCWorkerInfo;

typedef struct {
  const char *name;
  RCCDispatch *dispatch;
  const char *type;
} RCCEntryTable;
#endif



#ifdef __cplusplus

struct RCCDispatch {
  /* Information for consistency checking */
  unsigned version, numInputs, numOutputs;
  size_t propertySize, *memSizes;
  RCCBoolean threadProfile;
  /* Methods */
  RCCMethod *initialize, *start, *stop, *release, *test,
    *afterConfigure, *beforeQuery;
  RCCRunMethod *run;
  /* Implementation information for container behavior */
  RCCRunCondition *runCondition;
  RCCPortInfo *portInfo;

  /* Mask indicating which ports may be left unconnected */
  RCCPortMask optionallyConnectedPorts;
  size_t memSize;
};

// Info common to C and C++
struct RCCWorkerInfo {
  size_t size, memSize, *memSizes, propertySize;
  RCCPortInfo *portInfo;
  RCCPortMask optionallyConnectedPorts; // usually initialized from metadata
};

struct RCCEntryTable {
  const char *name;
  RCCDispatch *dispatch;
  const char *type;
};





// This is a preliminary implementation that avoids reorganizing the classes for
// maximum commonality between C and C++ workers. FIXME
 class RCCUserWorker;
 typedef RCCUserWorker *RCCConstruct(void *place, RCCWorkerInfo &info);
 class RCCUserPort;
 class RCCUserBuffer {
   RCCBuffer *m_rccBuffer;
   RCCBuffer  m_taken;
   friend class RCCUserPort;
 protected:
   RCCUserBuffer();
   void setRccBuffer(RCCBuffer *b);
   inline RCCBuffer *getRccBuffer() const { return m_rccBuffer; }
 public:
   inline void *data() const { return m_rccBuffer->data; }
   inline size_t maxLength() const { return m_rccBuffer->maxLength; }
   // For input buffers
   inline size_t length() const { return m_rccBuffer->length_; }
   RCCOpCode opCode() const { return m_rccBuffer->opCode_; }
   // For output buffers
   void setLength(size_t length) { m_rccBuffer->length_ = length; }
   void setOpCode(RCCOpCode op) {m_rccBuffer->opCode_ = op; }
   void setInfo(RCCOpCode op, size_t len) {
     setOpCode(op);
     setLength(len);
   }
   void release();
 };
 // Port inherits the buffer class in order to act as current buffer
 class RCCUserPort : public RCCUserBuffer {
   RCCPort &m_rccPort;
   friend class RCCUserWorker;
 protected:
   RCCUserPort();
 public:
   size_t
     topLength(size_t elementLength);
   void
     checkLength(size_t length),
     setDefaultLength(size_t length),
     setDefaultOpCode(RCCOpCode op),
     send(RCCUserBuffer*);
   RCCUserBuffer *take(RCCUserBuffer *oldBuffer);
   bool
    request(size_t maxlength),
    advance(size_t maxlength),
    wait(size_t max, unsigned usecs);
 };
 class Worker;
 class RCCUserWorker {
   friend class Worker;
   Worker &m_worker;
   RCCUserPort *m_ports; // array of C++ port objects
 protected:
   RCCUserPort &getPort(unsigned n) const { return m_ports[n]; }
   RCCWorker &m_rcc; // pointer not reference due to initialization issues
   RCCUserWorker();
   virtual ~RCCUserWorker();
   virtual uint8_t *rawProperties(size_t &size) const;
   virtual RCCResult
     initialize(), start(), stop(), release(), test(), beforeQuery(), afterConfigure();
   RCCResult setError(const char *fmt, ...);
 public:
   virtual RCCResult run(bool timeout) = 0;
 };
}} // end of namespace OCPI::RCC
#endif
#endif
