
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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <DtTransferInternal.h>
#include <DtSharedMemoryInternal.h>
#include <OcpiUtilHash.h>
#include <OcpiRes.h>
#include <OcpiOsAssert.h>
#include <OcpiRDTInterface.h>

using namespace DataTransfer;

SmemServices::
SmemServices (/*XferFactory * parent, */EndPoint& ep)
  : /* OCPI::Util::Child<XferFactory,SmemServices>(*parent), */ m_endpoint(ep)
{

}

SmemServices::
~SmemServices()
{


}

EndPoint::EndPoint( std::string& end_point, OCPI::OS::uint32_t psize, bool l )
  :mailbox(0),maxCount(0),size(psize),address(0),local(l),factory(NULL),refCount(0)
{
  if ( ! size ) {
    size = XferFactoryManager::getFactoryManager().getSMBSize();
  }
  setEndpoint( end_point);
}

EndPoint::~EndPoint() {
  if (factory)
    factory->removeEndPoint(*this);
  delete resources.sMemServices;
}

void EndPoint::release() {
  ocpiAssert(refCount);
  ocpiInfo("Releasing ep %p refCount %u", this, refCount);
  if (--refCount == 0)
    delete this;
}

// Sets smem location data based upon the specified endpoint
OCPI::OS::int32_t EndPoint::setEndpoint( std::string& ep )
{
  char buf[OCPI::RDT::MAX_PROTOS_SIZE];
  end_point = ep;
  getProtocolFromString(ep.c_str(), protocol);
  getResourceValuesFromString(ep.c_str() ,buf,&mailbox,&maxCount,&size);
  return 0;
}


// Endpoint parsing
void EndPoint::getProtocolFromString( const char* ep, std::string &proto )
{
  const char *colon = strchr(ep, ':');
  proto.assign(ep, colon ? colon - ep : strlen(ep));
}

void EndPoint::getResourceValuesFromString( const char* ep, 
                                            char* cs, 
                                            uint16_t* mailBox, 
                                            uint16_t* maxMb, 
                                            OCPI::OS::uint32_t* size
                                            )
{
  *size = 0;
  int item_count = 0;
  int cs_index = 0;
  for ( ssize_t n=strlen(ep)-1; n>=0; n-- ) {
    if ( ep[n] == '.' ) { 
      if ( item_count == 1 ) { //  mailbox value
        if ( cs_index > 1 ) {
          char tmp = cs[0];
          for ( int y=0; y<cs_index-1; y++) {
            cs[y] = cs[cs_index-1-y];
          }
          cs[cs_index-1] = tmp;
        }
        cs[cs_index] = 0;
        *mailBox = atoi(cs);
        cs_index = 0;
        item_count++;
      }
      else if ( item_count == 0 ) { // max mailbox value
        if ( cs_index > 1 ) {
          char tmp = cs[0];
          for ( int y=0; y<cs_index-1; y++) {
            cs[y] = cs[cs_index-1-y];
          }
          cs[cs_index-1] = tmp;
        }
        cs[cs_index] = 0;
        *maxMb = atoi(cs);
        cs_index = 0;
        item_count++;
      }
    }
    else if ( ep[n] == ':'  || ep[n] == ';') { 
      if ( cs_index > 0  ) {  // buffer size
        if ( cs_index > 1 ) {
          char tmp = cs[0];
          for ( int y=0; y<cs_index-1; y++) {
            cs[y] = cs[cs_index-1-y];
          }
          cs[cs_index-1] = tmp;
        }
        cs[cs_index] = 0;
        *size = atoi(cs);
        cs_index = 0;
      }
      break;
    }
    else {
      cs[cs_index++] = ep[n];
    }
  }
}

void EndPoint::
finalize() {
  getSmemServices();
  if (!resources.sMemResourceMgr) {
    ocpiInfo("Finalizing endpoint %p %s %p %p %p", this, end_point.c_str(),
	     resources.sMemServices, resources.sMemResourceMgr, resources.m_comms);
    resources.sMemResourceMgr = CreateResourceServices();
    resources.sMemResourceMgr->createLocal(size);
    uint32_t offset;
    if (resources.sMemResourceMgr->alloc( sizeof(ContainerComms), 0, &offset) != 0)
      throw OCPI::Util::EmbeddedException(  NO_MORE_SMB, end_point.c_str() );
    resources.m_comms = static_cast<DataTransfer::ContainerComms*>
      (resources.sMemServices->map(offset, sizeof(ContainerComms)));
  }
}

bool DataTransfer::EndPoint::
canSupport(const char *remoteEndpoint) {
  std::string remoteProtocol;
  DataTransfer::EndPoint::getProtocolFromString(remoteEndpoint, remoteProtocol);
  char *cs = strdup(remoteEndpoint);
  uint16_t mailBox, maxMb;
  uint32_t size;
  getResourceValuesFromString(remoteEndpoint, cs, &mailBox, &maxMb, &size);
  bool ret = 
    protocol == remoteProtocol &&
    maxMb == maxCount && mailBox != mailbox;
  free(cs);
  return ret;
}

SmemServices *DataTransfer::EndPoint::
getSmemServices() {
  return resources.sMemServices ?
    resources.sMemServices : (resources.sMemServices = &createSmemServices());
}

class ResourceServicesImpl : public DataTransfer::ResourceServices
{
public:
  // Create a local resource pool
  int createLocal (uint32_t size)
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
  int alloc (uint32_t nbytes, unsigned alignment, OCPI::Util::ResAddrType* addr_p)
  {
    return m_pool->alloc ( nbytes, alignment, *addr_p );
  }

  // Free back to pool
  int free (uint32_t addr, uint32_t nbytes)
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
DataTransfer::ResourceServices* DataTransfer::CreateResourceServices ()
{
  return new ResourceServicesImpl ();
}


DataTransfer::SMBResources::SMBResources()
 : sMemServices(0), sMemResourceMgr(0), m_comms(0) {
}
DataTransfer::SMBResources::~SMBResources() {
  delete sMemResourceMgr;
}
