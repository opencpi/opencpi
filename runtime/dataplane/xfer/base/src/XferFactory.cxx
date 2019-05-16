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

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <ezxml.h>
#include <OcpiOsAssert.h>
#include <OcpiOsMisc.h>
#include <OcpiUtilHash.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilMisc.h>
#include <OcpiPValue.h>
#include "XferServices.h"
#include "XferFactory.h"
#include "XferManager.h"

namespace OX = OCPI::Util::EzXml;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OD = OCPI::Driver;
namespace DataTransfer {

// An XferFactory keeps track of which templates exist.
XferServices &XferFactory::
getTemplate(EndPoint &source, EndPoint &target) {
  TemplatePair pair(&source, &target);
  OU::SelfAutoMutex guard(this);
  TemplateMapIter ti = m_templates.find(pair);
  XferServices *temp;
  if (ti == m_templates.end()) {
    temp = m_templates[pair] = &createXferServices(source, target);
    temp->addRef(); // add one for us
  } else
    temp = ti->second;
  temp->addRef(); // add a ref for our caller
  return *temp;
}
#if 0
void XferFactory::
removeTemplate(XferServices &xfs) {
  TemplatePair pair(&xfs.m_from, &xfs.m_to);
  OU::SelfAutoMutex guard(this);
  TemplateMapIter ti = m_templates.find(pair);
  assert(ti != m_templates.end());
  m_templates.erase(ti);
}
#endif

MailBox 
XferFactory::
getNextMailBox()
{
  static MailBox mailbox = 1;
  static bool mb_once = false;
  if ( ! mb_once ) {
    const char* env = getenv("OCPI_TRANSFER_MAILBOX");
    if( !env || (env[0] == 0)) {
      ocpiDebug("Set ""OCPI_TRANSFER_MAILBOX"" environment variable to control mailbox");
    }
    else {
      mailbox = (MailBox)atoi(env);
    }
    mb_once = true;
  }
#if 0
  for (; mailbox < m_locations.size() && m_locations[mailbox]; mailbox++)
    if (mailbox == MAX_SYSTEM_SMBS || mailbox > getMaxMailBox())
      throw OU::Error("Mailboxes for endpoints for protocol %s are exhausted (all %u are used)",
		      getProtocol(), getMaxMailBox());
#endif
  ocpiDebug("Transfer factory %p returning mailbox %u", this, mailbox);
  return mailbox++;
}



MailBox 
XferFactory::
getMaxMailBox()
{
  static bool mmb_once = false;
  static MailBox max_mb = MAX_SYSTEM_SMBS;
  if ( ! mmb_once ) {
    const char* env = getenv("OCPI_MAX_MAILBOX");
    if( !env || (env[0] == 0)) {
      ocpiDebug("Set ""OCPI_MAX_MAILBOX"" environment variable to control max mailbox");
    }
    else {
      max_mb = (MailBox)atoi(env);
    }
    mmb_once = true;
  }
  return max_mb;
}

ezxml_t 
XferFactory::
getNode( ezxml_t tn, const char* name )
{
  ezxml_t node = tn;
  while ( node ) {
    if ( node->name ) ocpiDebug("node %s", node->name );
    if ( node->name && (strcmp( node->name, name) == 0 ) ) {
      return node;
    }
    node = ezxml_cnext( node );
  }
  if ( tn ) {
    if(tn->child)if((node=getNode(tn->child,name)))return node;
    if(tn->sibling)if((node=getNode(tn->sibling,name)))return node;	 
  }
  return NULL;
}

// These defaults are pre-configuration
FactoryConfig::
FactoryConfig(size_t smbSize, size_t retryCount)
  : m_SMBSize(3*1024*1024), m_retryCount(128)
{
  if (smbSize)
    m_SMBSize = smbSize;
  if (retryCount)
    m_retryCount = retryCount;
}

// Parse and default from parent
void FactoryConfig::
parse(FactoryConfig *parent, ezxml_t x) {
  if (parent)
    *this = *parent;
  m_xml = x;
  if (x) {
    const char *err;
    // Note we are not writing defaults here because they are set
    // in the constructor, and they need to be set even when there is no xml
    if ((err = OX::checkAttrs(x, "load", "SMBSize", "TxRetryCount", NULL)) ||
	(err = OX::getNumber(x, "SMBSize", &m_SMBSize, NULL, 0, false)) ||
	(err = OX::getNumber(x, "TxRetryCount", &m_retryCount, NULL, 0, false)))
      throw std::string(err); // FIXME configuration api error exception class
  }
}

bool XferFactory::
supportsEndPoint(const char *a_name) {
  const char *protocol = getProtocol();
  size_t len = strlen(protocol);
  return !strncmp(a_name, protocol, len) && (!a_name[len] || a_name[len] == ':');
}

XferFactory::
XferFactory(const char *a_name)
  : OD::DriverType<XferManager,XferFactory>(a_name, *this) {
}

// Destructor
XferFactory::
~XferFactory() {
  this->lock();
  while (!m_endPoints.empty()) {
    EndPointsIter epi = m_endPoints.begin();
    epi->second->release();
    m_endPoints.erase(epi);
  }
  // These are already children, so they get removec automatically
  //  for (DataTransfer::TemplateMapIter tmi = m_templates.begin(); tmi != m_templates.end(); tmi++)
  //    delete tmi->second;
}
// clean out our caches for connections that involve remote endpoints created in this context
// also, release remote endpoints from our endpoint cache
void XferFactory::
cleanForContext(void *context) {
  for (auto ti = m_templates.begin(); ti != m_templates.end();) {
    EndPoint
      &from = *ti->first.first,
      &to = *ti->first.second;
    auto i = ti++;
    if ((!from.local() && from.context() == context) ||
	(!to.local() && to.context() == context)) {
      i->second->release();
      m_templates.erase(i);
    }
  }
  for (auto ei = m_endPoints.begin(); ei != m_endPoints.end(); ) {
    auto it = ei++;
    EndPoint &ep = *it->second;
    if (ep.context() == context && !ep.local()) {
      ep.release();
      m_endPoints.erase(it);
    }
  }
}

// This default implementation is just to parse generic properties,
// defaulting from our parent
void XferFactory::
configure(ezxml_t x) {
  // parse generic attributes and default from parent
  parse(&XferManager::getFactoryManager(), x);
  // base class does device config if present
  OD::Driver::configure(x); 
}

// Internal method
EndPoint &XferFactory::
addEndPoint(const char *endPoint, const char *other, bool local, size_t size) {
  std::string info;
  if (endPoint) {
    const char *colon = strchr(endPoint, ':');
    if (colon && colon[1]) {
      colon++;
      const char *semi = strrchr(colon, ';');
      info.assign(colon, semi ? OCPI_SIZE_T_DIFF(semi, colon) : strlen(colon));
      if (!semi)
	endPoint = NULL;
    } else
      endPoint = NULL;
  }
  EndPoint &ep =
    createEndPoint(info.empty() ? NULL : info.c_str(), endPoint, other, local, size, NULL);
  ep.setName();
  ocpiInfo("Dataplane endpoint %p created: %s", &ep, ep.m_name.c_str());
  ocpiAssert(m_endPoints.find(ep.m_uuid) == m_endPoints.end());
  m_endPoints[ep.m_uuid] = &ep;
  ep.addRef();
  return ep;
}

EndPoint* XferFactory::
findEndPoint(const char *end_point) {
  OU::Uuid uuid;
  EndPoint::getUuid(end_point, uuid);
  OU::SelfAutoMutex guard (this); 
  EndPoints::iterator i = m_endPoints.find(uuid);
  if (i != m_endPoints.end())
    return i->second;
  return NULL;
}
// Get, and possible create, the endpoint.  The "local" argument is not involved in the lookup,
// only in the creation.
EndPoint &XferFactory::
getEndPoint(const char *endPoint, bool local, bool cantExist, size_t size) { 
  assert(endPoint); // find out if anyone uses NULL
  const char *semi = strrchr(endPoint, ';');
  if (!semi || !semi[0]) // not a complete endpoint, a true allocation
    return addEndPoint(endPoint, NULL, local, size);
  OU::Uuid uuid;
  EndPoint::getUuid(endPoint, uuid);
  OU::SelfAutoMutex guard (this); 
  EndPointsIter i = m_endPoints.find(uuid);
  if (i != m_endPoints.end()) {
    if (cantExist)
      throw OU::Error("Local explicit endpoint already exists: '%s'", endPoint);
    else
      return *i->second;
  }
  return addEndPoint(endPoint, NULL, local, size);
}

// Return a mailbox number for a new endpoint, given an "other" endpoint that we might
// want to avoid.
MailBox XferFactory::
setNewMailBox(const char *other) {
  uint16_t mailBox = 0, myMax = getMaxMailBox();
  if (other) {
    uint16_t maxMb, otherMailBox;
    size_t size;
    EndPoint::parseEndPointString(other, &otherMailBox, &maxMb, &size);
    assert(otherMailBox);
    if (maxMb && maxMb != myMax)
      throw OU::Error("Remote end point has different number of mailbox slots (%u vs. our %u)",
		      maxMb, myMax);
    OU::SelfAutoMutex guard (this); 
    // Find an unused slot that is different from the remote one
    // mailbox might be zero so this will find the first free slot in any case
    MailBox n = 0;
    if (!mailBox) {
      if (n == MAX_SYSTEM_SMBS || n > myMax)
	throw OU::Error("Mailboxes for protocol %s are exhausted (all %u are used)",
			getProtocol(), myMax);
      mailBox = getNextMailBox();
    }
  } else
    mailBox = getNextMailBox();
  //  m_locations.resize(myMax + 1, NULL); // 1 origin
  //  assert(!m_locations[mailBox]);
  //  m_locations[mailBox] = &ep;
  return mailBox;

}

// The caller (transport session) doesn't have one, and wants one of its own,
// even though this means there will be multiple "local" endpoints in the same
// process
EndPoint &XferFactory::
addCompatibleLocalEndPoint(const char *remote) {
  if (!strchr(remote, ':'))
    remote = NULL;
  OU::SelfAutoMutex guard (this); 
  return addEndPoint(NULL, remote, true, 0);
}

void Device::
configure(ezxml_t x) {
  OD::Device::configure(x); // give the base class a chance to do generic configuration
  parse(&driverBase(), x);
}

}
