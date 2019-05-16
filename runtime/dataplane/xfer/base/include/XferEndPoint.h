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

// The EndPoint base class, and things it owns

#ifndef DT_ENDPOINT_H_
#define DT_ENDPOINT_H_

#include <assert.h>
#include <string>
#include <map>
#include "OcpiPValue.h"
#include "OcpiUuid.h"
#include "OcpiRes.h"

namespace DataTransfer {

// Address offset within endpoint
#ifndef OCPI_EP_SIZE_BITS
#define OCPI_EP_SIZE_BITS 32
#endif
#if OCPI_EP_SIZE_BITS == 32
  typedef uint32_t Offset;
  #define DTOSDATATYPES_OFFSET_PRIu PRIu32
  #define DTOSDATATYPES_OFFSET_PRIx PRIx32
#else
  typedef uint64_t Offset;
  #define DTOSDATATYPES_OFFSET_PRIu PRIu64
  #define DTOSDATATYPES_OFFSET_PRIx PRIx64
#endif
typedef uint16_t MailBox;
// Maximum number of SMB's and mailboxes allowed in the system unless overriden by env
// FIXME:  make this part of config
const MailBox MAX_SYSTEM_SMBS = 10;
#ifndef OCPI_EP_FLAG_BITS
#define OCPI_EP_FLAG_BITS 32
#endif
#if OCPI_EP_FLAG_BITS == 32
  typedef uint32_t Flag;
  #define DTOSDATATYPES_FLAG_PRIx PRIx32
#else
  typedef uint64_t Flag;
  #define DTOSDATATYPES_FLAG_PRIx PRIx64
#endif
}
// legacy
namespace DtOsDataTypes {
  typedef DataTransfer::Offset Offset;
  typedef DataTransfer::MailBox MailBox;
  typedef DataTransfer::Flag Flag;
  const MailBox MAX_SYSTEM_SMBS = DataTransfer::MAX_SYSTEM_SMBS;
}
namespace DataTransfer {

// This will be absorbed into endpoint someday..
class SmemServices;
class ResourceServices;
struct ContainerComms;
class XferFactory;
class XferServices;
class EndPoint {
  friend class XferFactory;
  friend class XferServices;
public:
  struct Receiver {
    virtual ~Receiver() {}
    virtual void receive(DtOsDataTypes::Offset offset, uint8_t *data, size_t count) = 0;
  };
private:
  std::string          m_name;       // the full text name
  uint16_t             m_mailBox;    // endpoint mailbox
  uint16_t             m_maxCount;   // Number of mailboxes in communication domain
  size_t               m_size;       // Size of endpoint area in bytes
  bool                 m_local;      // local endpoint - in this process
  XferFactory          &m_factory;   // parent driver
  unsigned             m_refCount;
  OCPI::Util::Uuid     m_uuid;
  Receiver            *m_receiver;      // receiver when data isn't really written to memory
  SmemServices        *m_sMemServices;  // mapping services
  ResourceServices    *m_resourceMgr;   // allocation services
  ContainerComms      *m_comms;         // in-band/mailbox communications
  void                *m_context;       // ownership context, no need for more than void* for now
protected:
  std::string          m_protoInfo;  // protocol-specific string set by derived classes
  uint64_t             m_address;    // Address of endpoint in its address space (usually 0)
  EndPoint(XferFactory &factory, const char *eps, const char *other, bool local, size_t size,
	   const OCPI::Util::PValue *params);
 public:
  virtual ~EndPoint();
 private:
  EndPoint(const EndPoint&);
  EndPoint& operator=(EndPoint&);
  EndPoint& operator=(EndPoint*);
  // Hook to allow a driver to decide this - like with different L2 ether interfaces etc.
  virtual bool isCompatibleLocal(const char *) const { return true; }
  static void parseEndPointString(const char* ep, uint16_t* mailBox, uint16_t* maxMb,
				  size_t* size);
 protected:
  void setName();
  Receiver *receiver() const { return m_receiver; };
 public:
  void setReceiver(Receiver &a_receiver) {
    m_receiver = &a_receiver;
  }
  static void getProtocolFromString(const char* ep, std::string &);
  // get uuid from an endpoint string
  static void getUuid(const char *ep, OCPI::Util::Uuid &uuid);
  uint64_t address() const { return m_address; }
  uint16_t mailBox() const { return m_mailBox; }
  uint16_t maxCount() const { return m_maxCount; }
  void *context() const { return m_context; }
  bool local() const { return m_local; }
  XferFactory &factory() const { return m_factory; }
  const std::string &name() const { return m_name; }
  size_t size() const { return m_size; }
  const OCPI::Util::Uuid &uuid() const { return m_uuid; }
  virtual SmemServices &createSmemServices() = 0;
  SmemServices &sMemServices();
  ResourceServices &resourceMgr() { assert(m_resourceMgr); return *m_resourceMgr; }
  ContainerComms &containerComms() { assert("containercomms disabled"==0); assert(m_comms); return *m_comms; }
  // Check compatibility
  bool canSupport(const char *remote_endpoint);
  // Given a string form of "other" is this one an acceptable counterparty?
  bool isCompatibleLocal(const XferFactory &factory, const char *other);
  void addRef();
  void release();
  // Commit resources.  Caller says whether remote access will be required.
  void finalize(); //bool remoteAccess = false);
};
typedef std::map<OCPI::Util::Uuid, EndPoint *, OCPI::Util::UuidComp> EndPoints;
typedef EndPoints::iterator EndPointsIter;

// Shared memory MAPPING services.  Virtual/base class, With default NOP methods.
// Not part of endpoint so that this object can be shared
class SmemServices {
 protected:
  EndPoint &m_endpoint;

 public:
  SmemServices (EndPoint &ep);
  virtual ~SmemServices();
  // Return zero on success for all of these
  virtual int32_t attach(EndPoint* loc);
  virtual int32_t detach();
  virtual void* map(DtOsDataTypes::Offset offset, size_t size ) = 0;
  virtual void* mapTx(DtOsDataTypes::Offset offset, size_t size ){return map(offset,size);}
  virtual void* mapRx(DtOsDataTypes::Offset offset, size_t size ){return map(offset,size);}
  virtual int32_t unMap();
  inline EndPoint &endPoint() {return m_endpoint;}
};
// There is no good place for this at the moment, but FIXME: move it somewhere better?
//SmemServices &createHostSmemServices(EndPoint& loc);
// Memory pool ALLOCATION services. FIXME: this should be defined in util/res etc.
class ResourceServices {
public:
  virtual int alloc(size_t nbytes, unsigned alignment, OCPI::Util::ResAddrType* addr_p) = 0;
  virtual int free(OCPI::Util::ResAddrType addr, size_t nbytes) = 0;
  virtual ~ResourceServices() {};
};
}

#endif
