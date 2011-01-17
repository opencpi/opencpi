
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
#include <OcpiPCISMemServices.h>
#include <xfer_if.h>
#include <OcpiList.h>
#include <OcpiUtilHash.h>
#include <OcpiOsMutex.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiOsMisc.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <OcpiOsAssert.h>
#include <OcpiPValue.h>

using namespace DataTransfer;
using namespace OCPI::Util;
using namespace OCPI::OS;

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
EndPoint* PCIPIOXferFactory::getEndPoint( std::string& end_point, bool /* local */  )
{ 
  PCIEndPoint *loc;
  for ( OCPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
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
   ( void ) loc;

#ifndef NDEBUG
  printf("void PCIPIOXferFactory::releaseLocation( EndPoint* loc ), NOT YET IMPLEMENTED !!\n");
#endif

}


// This method is used to allocate a transfer compatible SMB
SmemServices* PCIPIOXferFactory::getSmemServices( EndPoint* loc )
{
  if ( loc->smem ) {
    return loc->smem;
  }
  return new DataTransfer::PCISmemServices( this, loc);
}



/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* PCIPIOXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  return new PIOXferServices( *this, source, target);
}




/***************************************
 *  This method is used to dynamically allocate
 *  an endpoint for an application running on "this"
 *  node.
 ***************************************/
static OCPI::OS::int32_t pid;
std::string PCIPIOXferFactory::allocateEndpoint( OCPI::Util::Device * , OCPI::Util::PValue * /* props */)
{
  std::string ep;
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  int mailbox = getNextMailBox();
  pid++;

  unsigned int size = m_config->m_SMBSize;

  char tep[128];
  pid = getpid();
  int bus_id = 0;
  snprintf(tep,128,"ocpi-pci-pio://%d.0:%d.%d.%d",bus_id, size, mailbox,getMaxMailBox());
  ep = tep;

  mailbox++;

  return ep;
}





