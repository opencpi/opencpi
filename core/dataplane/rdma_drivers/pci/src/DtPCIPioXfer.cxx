// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file contains the implementation for the base class for SMB transfers.
 *
 * Author: John F. Miller
 *
 * Date: 7/20/04
 *
 */

#include <DtSharedMemoryInternal.h>
#include <DtPCIPioXfer.h>
#include <DtPioXfer.h>
#include <CpiPCISMemServices.h>
#include <xfer_if.h>
#include <CpiList.h>
#include <CpiUtilHash.h>
#include <CpiOsMutex.h>
#include <CpiUtilAutoMutex.h>
#include <CpiOsMisc.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <CpiOsAssert.h>

using namespace DataTransfer;
using namespace CPI::Util;
using namespace CPI::OS;

// Used to register with the data transfer system;
PCIPIOXferFactory *g_pciFactory = new PCIPIOXferFactory;
static VList g_locations;


PCIPIOXferFactory::PCIPIOXferFactory()
  throw ()
  : XferFactory("PCI Programmed I/O transfer driver")
{
  printf("In PCIPIOXferFactory::PCIPIOXferFactory()\n");
}

// Destructor
PCIPIOXferFactory::~PCIPIOXferFactory()
  throw ()
{
  clearCache();
}


/***************************************
 *  This method is used to flush any cached items in the factoy
 ***************************************/
void PCIPIOXferFactory::clearCache()
{
  g_locations.destroyList();
}


// Get the location via the endpoint
EndPoint* PCIPIOXferFactory::getEndPoint( std::string& end_point  )
{ 
  PCIEndPoint *loc;
  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<PCIEndPoint*>(g_locations.getEntry(n));
    if ( end_point == loc->end_point ) {
      return loc;
    }
  }

  loc = new PCIEndPoint(end_point);
        
  // This is a test case to make sure that a factory can modify the endpoint
  // string and system can handle it !!

  g_locations.insert( loc );
  return loc;
}

void PCIPIOXferFactory::releaseEndPoint( EndPoint* loc )
{

#ifndef NDEBUG
  printf("void PCIPIOXferFactory::releaseLocation( EndPoint* loc ), NOT YET IMPLEMENTED !!\n");
#endif

}


// This method is used to allocate a transfer compatible SMB
SmemServices* PCIPIOXferFactory::createSmemServices(EndPoint* loc )
{
  return new DataTransfer::PCISmemServices((EndPoint*)loc);
}



/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* PCIPIOXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  return new PIOXferServices(source, target);
}




/***************************************
 *  This method is used to dynamically allocate
 *  an endpoint for an application running on "this"
 *  node.
 ***************************************/
static CPI::OS::int32_t mailbox=1;
static CPI::OS::int32_t pid;
static CPI::OS::int32_t smb_count=0;
std::string PCIPIOXferFactory::allocateEndpoint(CPI::OS::uint32_t *size )
{
  std::string ep;
  CPI::Util::AutoMutex guard ( m_mutex, true ); 

#ifdef USE_ENV_FOR_MAILBOX
  if ( mailbox == -1 ) {
    const char* env = getenv("CPI_TRANSFER_MAILBOX");
    if( !env || (env[0] == 0)) {
      CPI_THROWNULL( DataTransferEx(PROPERTY_NOT_SET, "CPI_TRANSFER_MAILBOX" ) ) ;
    }
    mailbox = atoi(env);
    pid++;
  }
#endif

  char tep[128];
  pid = getpid();
  int bus_id = 0;
  snprintf(tep,128,"cpi-pci-pio://%d.0:%d.%d.20",bus_id,*size, mailbox);
  ep = tep;

  mailbox++;

  return ep;
}





