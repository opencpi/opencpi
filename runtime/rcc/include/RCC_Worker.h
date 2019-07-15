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

  @brief
    Contains the declarations of the RCC Worker interface as specified in
      the CP289U specification.
    This file is included by user-defined RCC Workers to communicate with
      the framework.

************************************************************************** */


#ifndef OCPI_RCC_WORKER__INTERFACE_H
#define OCPI_RCC_WORKER__INTERFACE_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "OcpiContainerRunConditionApi.h"

#define RCC_CONST const // this definition applies to worker code
#ifdef __cplusplus
#include "OcpiContainerApi.h" // For proxy slaves
#ifdef WORKER_INTERNAL
namespace OCPI {
  namespace RCC { class Port; }
  namespace DataTransport { class BufferUserFacet; }
  namespace Util { class Member; class Port;}
}
#undef RCC_CONST
#define RCC_CONST // for (C++) framework code these things are not const
#endif
namespace OCPI {
  namespace API {
    class Application;
  }
  namespace OS {
    class Timer;
  }
  namespace OCL {
    class Worker;
  }
  namespace RCC {
    class Worker;
#endif // end of __cplusplus

typedef uint16_t  RCCOrdinal;
typedef uint8_t   RCCOpCode;
typedef enum {
  RCC_LITTLE,
  RCC_BIG,
  RCC_DYNAMIC
} RCCEndian;
#ifdef __cplusplus
  class RCCUserPort;
  typedef OCPI::API::OcpiBoolean RCCBoolean;
  typedef OCPI::API::OcpiPortMask RCCPortMask; // not documented
  typedef OCPI::API::OcpiPortMask PortMask;
  typedef OCPI::API::RunCondition RunCondition;
  const OCPI::API::OcpiPortMask RCC_ALL_PORTS = OCPI::API::OCPI_ALL_PORTS;
  const OCPI::API::OcpiPortMask RCC_NO_PORTS = OCPI::API::OCPI_NO_PORTS;
#else
  typedef OcpiBoolean  RCCBoolean;
  typedef OcpiPortMask RCCPortMask;
  #define RCC_ALL_PORTS OCPI_ALL_PORTS
  #define RCC_NO_PORTS OCPI_NO_PORTS
#endif
typedef void      *RCCBufferId;
typedef float     RCCFloat;
typedef double    RCCDouble;
typedef char      RCCChar;
typedef uint64_t  RCCTime;

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

typedef struct RCCWorker RCCWorker;
typedef struct RCCPort RCCPort;

typedef struct {
  RCCPortMask *portMasks;
  RCCBoolean   timeout;
  uint32_t     usecs;
} RCCRunCondition;

typedef RCCResult RCCMethod(RCCWorker *_this);
typedef RCCResult RCCRunMethod(RCCWorker *_this,
                               RCCBoolean timedout,
                               RCCBoolean *newRunCondition);
typedef RCCResult RCCPortMethod(RCCWorker *_this,
                                RCCPort *port,
                                RCCResult reason);
 typedef struct {
   size_t length;
   size_t offset;
   size_t left;
   size_t right;
 } RCCPartInfo;

typedef struct {
  void *data;
  size_t maxLength;
  RCCPartInfo *partInfo;

  /* private member for container use */
  size_t length_;
  RCCOpCode opCode_;
  size_t direct_;
  bool eof_;
  bool isNew_; // hook for upper level initializations
#ifdef WORKER_INTERNAL
  OCPI::RCC::Port *containerPort;
  OCPI::API::ExternalBuffer *portBuffer;
#else
  void *id_, *id1_;
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
    RCCBoolean eof;
  } input;
  struct {
    size_t length;
    union {
      RCCOpCode operation;
      RCCOpCode exception;
    } u;
    RCCBoolean eof;
  } output;
  RCCPortMethod *callBack;
  size_t connectedCrewSize;
  /* Used by the container */
  RCCBoolean useDefaultLength_; // for C++, use the length field as default
  size_t defaultLength_;
  RCCBoolean useDefaultOpCode_; // for C++, use the length field as default
  RCCOpCode defaultOpCode_;
#ifdef WORKER_INTERNAL
  OCPI::RCC::Port *containerPort;
  RCCUserPort *userPort;
  OCPI::Util::Member *sequence;
  OCPI::Util::Port *metaPort;
#else
  void *containerPort, *userPort, *sequence, *metaPort;
#endif
};

typedef struct {
  void *args[3];
} RCCTaskArgs;

typedef void (*RCCTask)(RCCTaskArgs *args);

typedef struct {
  void (*release)(RCCBuffer *);
  void (*send)(RCCPort *, RCCBuffer*, RCCOpCode op, size_t length);
  RCCBoolean (*request)(RCCPort *port, size_t minlength);
  RCCBoolean (*advance)(RCCPort *port, size_t minlength);
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
  size_t                   member;
  size_t                   crewSize;
  RCCBoolean               firstRun;
  RCCPort                  ports[1];
};


typedef struct {
  RCCOrdinal port;
  size_t maxLength;
  unsigned minBuffers;
  // others...
} RCCPortInfo;

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
} RCCWorkerInfo;

typedef struct {
  const char *name;
  RCCDispatch *dispatch;
  const char *type;
} RCCEntryTable;

#ifdef __cplusplus
// This is a preliminary implementation that avoids reorganizing the classes for
// maximum commonality between C and C++ workers. FIXME
 class RCCUserWorker;
 typedef RCCUserWorker *RCCConstruct(void *place, RCCWorkerInfo &info);

 class RCCUserBuffer { // : public RCCUserBufferInterface {
   RCCBuffer *m_rccBuffer;
   RCCBuffer  m_taken;
   bool m_opCodeSet, m_lengthSet, m_resized;
   friend class RCCUserPort;
   friend class RCCPortOperation;
 protected:
   RCCUserBuffer();
   virtual ~RCCUserBuffer();
   void initBuffer(bool output = true);
   void setRccBuffer(RCCBuffer *b);
   inline RCCBuffer *getRccBuffer() const { return m_rccBuffer; }
   inline void resize(size_t size) {
     m_rccBuffer->length_ = size;
     m_resized = true;
   }
 public:
   inline uint8_t *data() const { return (uint8_t *)m_rccBuffer->data; }
   inline size_t maxLength() const { return m_rccBuffer->maxLength; }
   // For input buffers
   inline size_t length() const { return m_rccBuffer->length_; }
   inline size_t getLength() const { return m_rccBuffer->length_; } // same as STL-style length() but complements setLength
   inline RCCOpCode opCode() const { return m_rccBuffer->opCode_; }
   inline RCCOpCode getOpCode() const { return m_rccBuffer->opCode_; } // same as opCode() but complements setOpCode
   inline bool eof() const { return m_rccBuffer->eof_; }
   inline bool getEOF() const { return eof(); }
   // For output buffers
   void setLength(size_t a_length) {
     if (m_rccBuffer->isNew_)
       initBuffer();
     m_rccBuffer->length_ = a_length;
     m_lengthSet = true;
   }
   void setOpCode(RCCOpCode op);
   void setDirect(size_t direct) {m_rccBuffer->direct_ = direct; }
   void setEOF() { m_rccBuffer->eof_ = true; }
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
   friend class RCCPortOperation;
 protected:
   RCCUserPort();
   // Note length is capacity for output buffers.
   void *getArgAddress(RCCUserBuffer &buf, unsigned op, unsigned arg, size_t *length,
                       size_t *capacity) const;
   void setArgSize(RCCUserBuffer &buf, unsigned op, unsigned arg, size_t length) const;
 private:
   void checkOpCode(RCCUserBuffer &buf, unsigned op, bool setting = true) const;
   void shouldBeOutput() const;
   bool m_isSendMode; // Port-level flag determining if user has done a direct send of a message
                      // that was received. Once they've done that, it affects output port buffers
                      // and you cannot use certain functions, e.g. setOpCode (AV-1610)
 public:
   // Test whether a buffer is available, and if not request one
   // There is no buffer is there is no container port for the rcc port (not connected).
   // FIXME: use connectedPorts rather than containerPort
   inline bool hasBuffer() {
     return m_rccBuffer && (m_rccBuffer->data || (m_rccPort.containerPort && request()));
   }
   size_t connectedCrewSize() { return m_rccPort.connectedCrewSize; }
   size_t
     topLength(size_t elementLength);
   void
     checkLength(size_t length),
     setDefaultLength(size_t length),
     setDefaultOpCode(RCCOpCode op),
     send(RCCUserBuffer&);
   RCCUserBuffer &take(RCCUserBuffer *oldBuffer = NULL);
   bool
    request(size_t minlength = 0),
    advance(size_t minlength = 0),
    isConnected(),
    wait(size_t max, unsigned usecs);
   RCCOrdinal ordinal() const;
   void setOpCode(RCCOpCode op);
 };

 class RCCPortOperation { // : public RCCUserBufferInterface {
   RCCUserPort &m_port;
   unsigned m_op;
   friend class RCCPortOperationArg;
 protected:
   RCCUserBuffer *m_buffer;
   RCCPortOperation(RCCUserPort &p, unsigned op) : m_port(p), m_op(op), m_buffer(&p) {}
   inline void setRccBuffer(RCCBuffer *b) { m_buffer->setRccBuffer(b); };
   inline RCCBuffer *getRccBuffer() const { return m_buffer->getRccBuffer(); }
   inline void *getArgAddress(unsigned arg, size_t *a_length, size_t *capacity) const {
     return m_port.getArgAddress(*m_buffer, m_op, arg, a_length, capacity);
   }
   inline void setArgSize(unsigned arg, size_t a_length) const {
     m_port.setArgSize(*m_buffer, m_op, arg, a_length);
   }
 public:
   inline void * data() const { return m_buffer->data(); }
   inline size_t maxLength() const { return m_buffer->maxLength(); }
   // For input buffers
   inline size_t length() const { return m_buffer->length(); }
   inline RCCOpCode opCode() const  { return m_buffer->opCode(); };
   // For output buffers
   inline void setLength(size_t a_length) {
     m_port.checkLength(a_length);
     m_buffer->setLength(a_length);
   }
   inline void setOpCode(RCCOpCode op) { m_buffer->setOpCode(op); }
   inline void setInfo(RCCOpCode op, size_t len) { m_buffer->setInfo(op, len); }
   inline void release() { m_buffer->release(); }

   unsigned  overlap() const;
   bool      endOfWhole() const;
   bool      endOfStream() const;
   unsigned  wholeSize() const;  // Size of the whole for this message

 };

 // A class that simply provides access to the arg
 class RCCPortOperationArg {
   unsigned m_arg;
   friend class RCCPortOperation;
 protected:
   RCCPortOperation &m_op;
   RCCPortOperationArg(RCCPortOperation &, unsigned arg);
   inline void *getArgAddress(size_t *length, size_t *capacity) const {
     return m_op.getArgAddress(m_arg, length, capacity);
   }
   inline void setArgSize(size_t length) const {
     return m_op.setArgSize(m_arg, length);
   }
 };

 class Worker;
 class RCCUserWorker;
 class RCCUserTask {
 public:
   RCCUserTask();
   virtual void run() = 0;
   void spawn();
 private:
   Worker & m_worker;
 };

 class RCCUserWorker {
   friend class Worker;
   friend class RCCUserTask;
   friend class RCCUserSlave;
   Worker &m_worker;
   RCCUserPort *m_ports; // array of C++ port objects
 public:
   size_t getMember() const { return m_rcc.member; }
   size_t getCrewSize() const { return m_rcc.crewSize; };
   void addTask(RCCTask task, RCCTaskArgs *args);
   void addTask(RCCUserTask *task);
   void waitTasks();
   bool checkTasks();
   // Simple distribution calculation of now many items a given member will be responsible
   // for given a total, and a limit on message sizes.  Return is total for member,
   // optional output arg is max per message for this member.
   size_t memberItemTotal(uint64_t totalItems, size_t maxPerMessage = 0,
                          size_t *perMessage = NULL);
 protected:
   bool m_first;
   RCCWorker &m_rcc;
   RCCUserWorker();
   virtual ~RCCUserWorker();

   // These are called by the worker.
   virtual void propertyWritten(unsigned /*ordinal*/) {}
   virtual void propertyRead(unsigned /*ordinal*/) {}
   RCCUserPort &getPort(unsigned n) const { return m_ports[n]; }
   inline bool firstRun() const { return m_first; };
   bool isInitialized() const;
   bool isSuspended() const;
   bool isFinished() const;
   bool isUnusable() const;
   bool isOperating() const;
   // access the current run condition
   const RunCondition *getRunCondition() const;
   // Change the current run condition - if NULL, revert to the default run condition
   void setRunCondition(const RunCondition *rc);
   virtual uint8_t *rawProperties(size_t &size) const;
   RCCResult setError(const char *fmt, ...);
   bool willLog(unsigned level) const;
   void log(unsigned level, const char *fmt, ...) throw() __attribute__((format(printf, 3, 4)));
   OCPI::API::Application &getApplication();
   // These below are called by the container, and NOT by the worker.

   // The worker author implements any of these that have non-default behavior.
   // There is a default implementation for all of them.
   virtual RCCResult
     initialize(), start(), stop(), release(), test(), beforeQuery(), afterConfigure();
   // The worker author must implement this one.
   virtual RCCResult run(bool timedOut) = 0;
   virtual RCCTime getTime();
 };

 // This class emulates the API worker, and is customized for the specific implementation
 // It forwards all the API methods
 class RCCUserSlave {
 protected:
   OCPI::API::Worker &m_worker;
   RCCUserSlave(unsigned n = 0);
 public:
   inline bool isOperating() const { return m_worker.isOperating(); }
   inline void start() { m_worker.start(); }
   inline void stop() { m_worker.stop(); }
   inline void beforeQuery() { m_worker.beforeQuery(); }
   inline void afterConfigure() { m_worker.afterConfigure(); }
   // Untyped property setting - slowest but convenient
   inline void setProperty(const char *name, const char *value,
			   OCPI::API::AccessList &list = OCPI::API::emptyList) {
     m_worker.setProperty(name, value, list);
   }
   inline const char *getProperty(unsigned ordinal, std::string &value,
				  OCPI::API::AccessList &list,
				  OCPI::API::PropertyOptionList &options,
				  OCPI::API::PropertyAttributes *attributes) const {
     return m_worker.getProperty(ordinal, value, list, options, attributes);
   }
   // Untyped property list setting - slow but convenient
   inline void setProperties(const char *props[][2]) {
     m_worker.setProperties(props);
   }
   // Typed property list setting - slightly safer, still slow
   inline void setProperties(const OCPI::API::PValue *props) { m_worker.setProperties(props); }
   inline bool getProperty(unsigned ordinal, std::string &name, std::string &value,
                           bool *unreadablep = NULL, bool hex = false) {
     return m_worker.getProperty(ordinal, name, value, unreadablep, hex);
   }
   inline void getRawPropertyBytes(size_t offset, uint8_t *buf, size_t count) {
     m_worker.getRawPropertyBytes(offset, buf, count);
   }
   inline void setRawPropertyBytes(size_t offset, const uint8_t *buf, size_t count) {
     m_worker.setRawPropertyBytes(offset, buf, count);
   }
 };
}} // end of namespace OCPI::RCC
#endif
#endif
