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

// The EndPoint base class
// The format of endpoints as strings are:
// <protocol>:<protocol-specific-info>;<uuid>.<size>.<mbox>.<nmboxes>
// if there is no colon, it is just a protocol
// if there is a semicolon, then there must be a trailing semicolon after the protocol info
// i.e. protocol info can have semicolons
// So a string containing a protocol with protocol-specific info, must have the trailing
// semicolon even if it has nothing after that

#include "OcpiUtilMisc.h"
#include "XferException.h"
#include "XferEndPoint.h"
#include "XferFactory.h"
#include "XferManager.h"

namespace OU = OCPI::Util;

namespace DataTransfer {

EndPoint::
EndPoint(XferFactory &a_factory, const char *eps, const char *other, bool a_local, size_t a_size,
	 const OCPI::Util::PValue */*params*/)
  :  m_mailBox(0), m_maxCount(0), m_size(0), m_local(a_local), m_factory(a_factory),
     m_refCount(0), m_receiver(NULL), m_sMemServices(NULL), m_resourceMgr(NULL), m_comms(NULL),
     m_context(XferManager::getFactoryManager().getEndPointContext()), m_address(0) {
  if (eps) {
    getUuid(eps, m_uuid);
    size_t psize;
    parseEndPointString(eps, &m_mailBox, &m_maxCount, &psize);
    assert(psize && (!a_size || a_size == psize));
    m_size = psize;
  } else {
    OU::generateUuid(m_uuid);
    m_mailBox = a_factory.setNewMailBox(other);
    m_maxCount = a_factory.getMaxMailBox();
    m_size = a_size ? a_size : a_factory.getSMBSize();
  }
}

EndPoint::
~EndPoint() {
  //  m_factory.removeEndPoint(*this);
  delete m_sMemServices;
  delete m_resourceMgr;
}

// This is a helper method to the derived class's constructor to tell the baseclass
// about the protocol info.
void EndPoint::
setName() {
  OU::UuidString s;
  OU::uuid2string(m_uuid, s);
  OU::format(m_name, "%s:%s;%s.%zu.%" PRIu16 ".%" PRIu16, m_factory.getProtocol(),
	     m_protoInfo.c_str(), s.uuid, m_size, m_mailBox, m_factory.getMaxMailBox());
}

void EndPoint::
addRef()
{
  m_refCount++;
  ocpiLog(9, "Incrementing refcount on ep %p to %u", this, m_refCount);
}
void EndPoint::
release() {
  ocpiLog(9, "Releasing ep %p %s refCount %u", this, m_name.c_str(), m_refCount);
  ocpiAssert(m_refCount);
  if (--m_refCount == 0) {
    ocpiInfo("Dataplane endpoint %p destroyed: %s", this, m_name.c_str());
    delete this;
  }
}

// Endpoint parsing
void EndPoint::
getProtocolFromString( const char* ep, std::string &proto )
{
  const char *colon = strchr(ep, ':');
  proto.assign(ep, colon ? OCPI_SIZE_T_DIFF(colon, ep) : strlen(ep));
}

// static
void EndPoint::
getUuid(const char *ep, OCPI::Util::Uuid &uuid) {
  const char *semi = strrchr(ep, ';');
  if (semi) {
    OU::UuidString s;
    strncpy(s.uuid, semi+1, sizeof(s.uuid));
    s.uuid[sizeof(s.uuid)-1] = '\0';
    if (!OU::string2uuid(s, uuid))
      return;
  }
  throw OU::Error("Invalid endpoint uuid in: %s", ep);
}

// static
void EndPoint::
parseEndPointString(const char* ep, uint16_t* mailBox, uint16_t* maxMb, size_t* size) {
  const char *semi = strrchr(ep, ';');
  if (!semi || !(semi = strchr(semi, '.')) ||
      sscanf(semi+1, "%zu.%" SCNu16 ".%" SCNu16, size, mailBox, maxMb) != 3 ||
      size == 0 || mailBox == 0 || maxMb == 0)
    throw OU::Error("Invalid endpoint: %s", ep);
}

bool EndPoint::
isCompatibleLocal(const XferFactory &argFactory, const char *other) {
  uint16_t mBox = 0, maxMb = 0;
  const char *after = strchr(other, ':');
  if (after) {
    after++; // for isCompatible below
    size_t l_size;
    parseEndPointString(other, &mBox, &maxMb, &l_size);
  }
  return
    &m_factory == &argFactory &&
#if 0
    (mBox == 0 || (m_maxCount == maxMb && mBox != m_mailBox)) &&
#endif
    isCompatibleLocal(after);
}

class XferPool : public OU::MemBlockMgr, public ResourceServices {
public:
  XferPool(size_t size)
    : OU::MemBlockMgr(0, size) {
    OCPI::Util::ResAddrType dummy;    
    alloc(16, 16, &dummy);
    assert(!dummy);
  }
  int alloc(size_t nbytes, unsigned alignment, OCPI::Util::ResAddrType *addr_p) {
    return OU::MemBlockMgr::alloc(nbytes, alignment, *addr_p);
  }
  int free(OCPI::Util::ResAddrType addr, size_t /*nbytes*/) {
    return OU::MemBlockMgr::free(addr);
  }
};

void EndPoint::
finalize() { // bool remoteAccess) {
  // Create the mapping service if needed
  bool remoteAccess = false;
  if ((m_local || remoteAccess) && !m_sMemServices)
    m_sMemServices = &createSmemServices();
  if (m_local && !m_resourceMgr)
    m_resourceMgr = new XferPool(m_size);
  ocpiDebug("Finalize endpoint %p %s %p %p %p %u %u", this, name().c_str(),
	    m_sMemServices, m_resourceMgr, m_comms, m_local, remoteAccess);
}

bool EndPoint::
canSupport(const char *remoteEndpoint) {
  std::string remoteProtocol;
  EndPoint::getProtocolFromString(remoteEndpoint, remoteProtocol);
  uint16_t mBox, maxMb;
  size_t l_size;
  parseEndPointString(remoteEndpoint, &mBox, &maxMb, &l_size);
  bool ret = 
    m_factory.getProtocol() == remoteProtocol &&
    maxMb == m_maxCount && mBox != m_mailBox;
  return ret;
}

SmemServices &EndPoint::
sMemServices() {
  return *(m_sMemServices ?
	   m_sMemServices : (m_sMemServices = &createSmemServices()));
}

SmemServices::
SmemServices (/*XferFactory * parent, */EndPoint& ep)
  : /* OCPI::Util::Child<XferFactory,SmemServices>(*parent), */ m_endpoint(ep)
{
}

SmemServices::
~SmemServices()
{
}

int32_t SmemServices::
attach(EndPoint*) { return 0; }

int32_t SmemServices::
detach() { return 0; }

int32_t SmemServices::
unMap() { return 0; }

}
