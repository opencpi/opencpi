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
   This file contains private support functions for the shared memory classes.

   Revision History:

   6/15/2004 - John Miller
   Initial version.

   2/18/2009 - John Miller
   Removed exception monitor class.

************************************************************************** */
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "OcpiOsAssert.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiOsMisc.h"
#include "OcpiUtilHash.h"
#include "OcpiUtilMisc.h"
#include "OcpiRes.h"
#include "DtTransferInternal.h"
#include "DtSharedMemoryInternal.h"
#include "DtHandshakeControl.h"

namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace DDT = DtOsDataTypes;
namespace DataTransfer {

static int check = compileTimeSizeCheck<sizeof(DDT::Offset),sizeof(OCPI::Util::ResAddr)>();

SmemServices::
SmemServices (/*XferFactory * parent, */EndPoint& ep)
  : /* OCPI::Util::Child<XferFactory,SmemServices>(*parent), */ m_endpoint(ep)
{
}

SmemServices::
~SmemServices()
{
}

EndPoint::
EndPoint( std::string& end_point, size_t psize, bool l )
  :mailbox(0),maxCount(0),size(psize),address(0),local(l),factory(NULL),refCount(0)
{
  if ( ! size ) {
    size = XferFactoryManager::getFactoryManager().getSMBSize();
  }
  setEndpoint( end_point);
}

EndPoint::
~EndPoint() {
  if (factory)
    factory->removeEndPoint(*this);
  delete resources.sMemServices;
}

void EndPoint::
release() {
  ocpiAssert(refCount);
  ocpiDebug("Releasing ep %p refCount %u", this, refCount);
  if (--refCount == 0)
    delete this;
}

// Sets smem location data based upon the specified endpoint
OCPI::OS::int32_t
EndPoint::setEndpoint(std::string& ep)
{
  end_point = ep;
  getProtocolFromString(ep.c_str(), protocol);
  parseEndPointString(ep.c_str(), &mailbox, &maxCount, &size);
  return 0;
}


// Endpoint parsing
void EndPoint::
getProtocolFromString( const char* ep, std::string &proto )
{
  const char *colon = strchr(ep, ':');
  proto.assign(ep, colon ? colon - ep : strlen(ep));
}

void EndPoint::
parseEndPointString(const char* ep, uint16_t* mailBox, uint16_t* maxMb, size_t* size) {
  const char *semi = strrchr(ep, ';');
  if (!semi || sscanf(semi+1, "%zu.%" SCNu16 ".%" SCNu16, size, mailBox, maxMb) != 3)
    throw OU::Error("Invalid endpoint: %s", ep);
}

bool EndPoint::
matchEndPointString(const char *ep) {
  uint16_t argMailBox = 0, argMaxMb = 0;
  size_t argSize;
  parseEndPointString(ep, &argMailBox, &argMaxMb, &argSize);
  if (mailbox == argMailBox) {
    if (size != argSize || maxCount != argMaxMb)
      throw OU::Error("Endpoint mismatch for %s: existing is %s", ep, end_point.c_str());
    return true;
  }
  return false;
}

void EndPoint::
finalize() {
  getSmemServices();
  resources.finalize(*this);
}

bool EndPoint::
canSupport(const char *remoteEndpoint) {
  std::string remoteProtocol;
  EndPoint::getProtocolFromString(remoteEndpoint, remoteProtocol);
  uint16_t mailBox, maxMb;
  size_t size;
  parseEndPointString(remoteEndpoint, &mailBox, &maxMb, &size);
  bool ret = 
    protocol == remoteProtocol &&
    maxMb == maxCount && mailBox != mailbox;
  return ret;
}

SmemServices *EndPoint::
getSmemServices() {
  return resources.sMemServices ?
    resources.sMemServices : (resources.sMemServices = &createSmemServices());
}

void EndPoint::
setReceiver(Receiver &/*receiver*/) {
  throw OU::Error("Endpoint \"%s\" can't support a receiver function", end_point.c_str());
}

class ResourceServicesImpl : public ResourceServices
{
public:
  // Create a local resource pool
  int createLocal (size_t size)
  {
    (void)terminate ();
    try {
      m_pool = new OCPI::Util::MemBlockMgr( 0, size );
    }
    catch( ... ) {
      return -1;
    }
    return 0;
  }

  // Allocate from pool
  int alloc (size_t nbytes, unsigned alignment, OCPI::Util::ResAddrType* addr_p)
  {
    return m_pool->alloc ( nbytes, alignment, *addr_p );
  }

  // Free back to pool
  int free (OCPI::Util::ResAddrType addr, size_t nbytes)
  {
    ( void ) nbytes;
    return m_pool->free ( addr );
  }

  // Destroy resource pool
  int destroy ()
  {
    terminate ();
    return 0;
  }

  // ctor/dtor
  ResourceServicesImpl ()
    :m_pool(NULL)
  {
  }
  ~ResourceServicesImpl ()
  {
    terminate ();
  }
private:
  OCPI::Util::MemBlockMgr* m_pool;

private:
  void terminate ()
  {
    if (m_pool) {
      delete m_pool;
      m_pool = NULL;
    }
  }
};

// Platform dependent global that creates an instance
ResourceServices* CreateResourceServices ()
{
  return new ResourceServicesImpl ();
}


SMBResources::
SMBResources()
  : sMemServices(0), sMemResourceMgr(0), m_comms(0) {
}

SMBResources::
~SMBResources() {
  delete sMemResourceMgr;
}

void SMBResources::
finalize(EndPoint &ep) {
  if (!sMemResourceMgr) {
    ocpiDebug("Finalizing endpoint %p %s %p %p %p", &ep, ep.end_point.c_str(),
	     sMemServices, sMemResourceMgr, m_comms);
    sMemResourceMgr = CreateResourceServices();
    sMemResourceMgr->createLocal(ep.size);
    OCPI::Util::ResAddr offset;
    if (sMemResourceMgr->alloc( sizeof(ContainerComms), 0, &offset) != 0)
      throw OCPI::Util::EmbeddedException(  NO_MORE_SMB, ep.end_point.c_str() );
    m_comms = static_cast<ContainerComms*>
      (sMemServices->map(offset, sizeof(ContainerComms)));
  }
}

// FIXME: move this to a new file along with the handshake stuff
bool XferMailBox::
makeRequest( SMBResources* source, SMBResources* target )
{

  ocpiDebug("In makerequest from %s to %s\n",
	    source->sMemServices->endpoint()->end_point.c_str(),
	    target->sMemServices->endpoint()->end_point.c_str() );


#ifdef MULTI_THREADED
  // Lock the mailbox
  if ( ! lockMailBox() ) {
    return false;
  }
#endif

  /* Attempt to get or make a transfer template */
  XferServices* ptemplate =
    XferFactoryManager::getFactoryManager().getService(
						       source->sMemServices->endpoint(),
						       target->sMemServices->endpoint() );
  ocpiAssert(ptemplate);

  DDT::Offset offset =
    OCPI_SIZEOF(DDT::Offset, ContainerComms::MailBox) * m_slot + OCPI_SIZEOF(DDT::Offset, UpAndRunning);

  ocpiDebug("In make request with offset = %d\n", offset );

  XferRequest * ptransfer = ptemplate->createXferRequest();

  // create the copy in the template
  ptransfer->copy (
		   offset + OCPI_SIZEOF(DDT::Offset, ContainerComms::RequestHeader),
		   offset + OCPI_SIZEOF(DDT::Offset, ContainerComms::RequestHeader),
		   sizeof(ContainerComms::MailBox) - sizeof(ContainerComms::RequestHeader),
		   XferRequest::DataTransfer );

  ptransfer->copy (
		   offset,
		   offset,
		   sizeof(ContainerComms::RequestHeader),
		   XferRequest::FlagTransfer );

  // Start the transfer
  ptransfer->post();

  while( ptransfer->getStatus() ) {
    OS::sleep(0);
  }

  delete ptransfer;

  return true;
}
}
