
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


#include <DtSharedMemoryInternal.h>
#include <string.h>
#include <OcpiUtilHash.h>
#include <stdio.h>
#include <stdlib.h>
#include <OcpiRes.h>
#include <OcpiOsAssert.h>

using namespace DataTransfer;

EndPoint::EndPoint( std::string& end_point, OCPI::OS::uint32_t psize )
 :mailbox(0),maxCount(0),size(psize),local(false),resources(0)
{
  setEndpoint( end_point);
}

EndPoint::EndPoint(OCPI::OS::uint32_t psize)
   :mailbox(0),maxCount(0),size(psize),local(false),resources(0)
{

}

// Sets smem location data based upon the specified endpoint
OCPI::OS::int32_t EndPoint::setEndpoint( std::string& ep )
{
  char buf[30];
  end_point = ep;
  char* proto = (char*)getProtocolFromString(ep.c_str(),buf);
  protocol = proto;
  getResourceValuesFromString(ep.c_str() ,buf,&mailbox,&maxCount,&size);
  return 0;
}


EndPoint& EndPoint::operator=(EndPoint& rhs)
{
  return operator=( &rhs );
}

EndPoint& EndPoint::operator=(EndPoint* rhs)
{
  this->setEndpoint( rhs->end_point );
  return *this;
}


// Endpoint parsing
const char* EndPoint::getProtocolFromString( const char* ep, char *proto )
{
  int i=0;
  for ( unsigned int n=0; n<strlen(ep); n++ ) {
    if ( ep[n] == ':' ) {
      break;
    }
    proto[i++] = ep[n];
  }
  proto[i] = 0;
  return proto;
}

void EndPoint::getResourceValuesFromString( const char* ep, 
                                            char* cs, 
                                            OCPI::OS::uint32_t* mailBox, 
                                            OCPI::OS::uint32_t* maxMb, 
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
    else if ( ep[n] == ':' ) { 
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

class ResourceServicesImpl : public DataTransfer::ResourceServices
{
public:
  // Create a local resource pool
  OCPI::OS::int32_t createLocal (OCPI::OS::uint32_t size)
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
  OCPI::OS::int32_t alloc (OCPI::OS::uint32_t nbytes, OCPI::OS::uint32_t alignment, OCPI::OS::uint64_t* addr_p)
  {
    int  retval;
    retval = m_pool->alloc ( nbytes, alignment, *addr_p );
    return retval;
  }

  // Free back to pool
  OCPI::OS::int32_t free (OCPI::OS::uint32_t addr, OCPI::OS::uint32_t nbytes)
  {
    ( void ) nbytes;
    return m_pool->free ( addr );
  }

  // Destroy resource pool
  OCPI::OS::int32_t destroy ()
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


