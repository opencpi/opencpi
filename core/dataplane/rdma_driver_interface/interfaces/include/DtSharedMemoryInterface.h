
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
   This file contains the common classes used in the SMB package

   Revision History:

   1/20/2005 - John Miller
   Initial version.

   2/18/2009 - John Miller
   Removed exception monitor class.

************************************************************************** */

#ifndef OCPI_DataTransfer_SharedMemoryInterface_H_
#define OCPI_DataTransfer_SharedMemoryInterface_H_

#include <OcpiOsDataTypes.h>
#include <string>

namespace DataTransfer {

  // Maximum number of SMB's allowed in the system
  const OCPI::OS::uint32_t MAX_SYSTEM_SMBS = 5;

  // Protocol specific location class
  struct SMBResources;
  struct EndPoint
  {

    std::string  end_point;                // deep copy of the endpoint string
    std::string  protocol;                // protocol string
    OCPI::OS::uint32_t     mailbox;        // endpoint mailbox
    OCPI::OS::uint32_t          maxCount;        // Number of mailboxes in communication domain
    OCPI::OS::uint32_t     size;                // Size of endpoint area in bytes
    OCPI::OS::uint32_t     event_id;     
    bool                  local;        // local endpoint
    SMBResources*          resources;        // SMB resources associated with this endpoint

    // Constructors
    EndPoint(OCPI::OS::uint32_t size=0);
    EndPoint( std::string& ep, OCPI::OS::uint32_t size=0);
    EndPoint& operator=(EndPoint&);
    EndPoint& operator=(EndPoint*);
    virtual ~EndPoint(){};

    // Sets smem location data based upon the specified endpoint
    virtual OCPI::OS::int32_t setEndpoint( std::string& ep );

    // Get the address from the endpoint
    virtual const char* getAddress()=0;

    // Parse the endpoint string
    static const char* getProtocolFromString( const char* ep, char* );

    static void getResourceValuesFromString( 
                            const char*  ep,                // Endpoint value
                            char*  cs,                        // User provided buffer (at least sizeof ep )
                            OCPI::OS::uint32_t* mailBox,        // Mailbox value returned
                            OCPI::OS::uint32_t* maxMb,        // Maximum mailbox value in circuit returned
                            OCPI::OS::uint32_t* bufsize        // Buffer size returned
                            );

  };


  // Shared memory services.  
  class SmemServices
  {
  public:
    SmemServices (const EndPoint* ){};

    /*
     * Attach to an existing shared memory object by name.
     *        Arguments:
     *                name        - Name of shared memory to attach to
     *        Returns:
     *                Returns 0 if success, platform dependent error otherwise
     *
     */
    virtual OCPI::OS::int32_t attach (EndPoint* loc) = 0;

    /*
     * Detach from shared memory object
     *        Arguments:
     *        Returns:
     *                Returns 0 if success, platform dependent error otherwise
     *
     */
    virtual OCPI::OS::int32_t detach () = 0;

    /*
     * Map a view of the shared memory area at some offset/size and return the virtual address.
     *        Arguments:
     *                offset        - offset into the shared memory area to map in bytes
     *                size        - size of the area to map where 0 means entire shared memory area
     *                pva                - receives the virtual address of the mapping
     *        Returns:
     *                Returns address if success, NULL on error and sets platform dependent exception
     *
     */
    virtual void* map (OCPI::OS::uint64_t offset, OCPI::OS::uint32_t size ) = 0;

    /*
     * Unmap the current mapped view.
     *        Arguments:
     *        Returns:
     *                Returns 0 if success, platform dependent error otherwise
     *
     */
    virtual OCPI::OS::int32_t unMap () = 0;

    /*
     *        GetEndPoint - Returns the endpoint of the shared memory area
     */
    virtual EndPoint* getEndPoint () = 0;



  public:

    // Destructor
    virtual ~SmemServices () {};
  };

}


#endif
