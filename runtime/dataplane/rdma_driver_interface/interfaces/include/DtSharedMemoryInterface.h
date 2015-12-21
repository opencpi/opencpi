
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

#include <string>
#include "OcpiParentChild.h"
#include "OcpiOsDataTypes.h"
#include "DtOsDataTypes.h"

namespace DataTransfer {


  /*
   * This structure is used to manage the shared memory blocks.  It contains the 
   * shared memory service class, the resource manager for the SMB and a mapped
   * pointer to the local mailbox structure within the SMB.
   */
  class ResourceServices;
  struct ContainerComms;
  class SmemServices;
  struct EndPoint;
  struct SMBResources {
    SmemServices       *sMemServices;
    ResourceServices   *sMemResourceMgr;
    ContainerComms     *m_comms;
    void finalize(EndPoint &ep);
    SMBResources();
    ~SMBResources();
  };
  class SmemServices;
  class XferFactory;
  // Protocol specific location class
  struct EndPoint
  {
    struct Receiver {
      virtual ~Receiver() {}
      virtual void receive(DtOsDataTypes::Offset offset, uint8_t *data, size_t count) = 0;
    };
    std::string          end_point;    // deep copy of the endpoint string
    std::string          protocol;     // protocol string
    uint16_t             mailbox;      // endpoint mailbox
    uint16_t             maxCount;     // Number of mailboxes in communication domain
    size_t               size;         // Size of endpoint area in bytes
    uint64_t             address;      // Address of endpoint in its address space (usually 0)
    //    uint32_t             event_id;     
    bool                 local;        // local endpoint
    SMBResources         resources;    // SMB resources associated with this endpoint
    XferFactory*         factory;
    unsigned             refCount;
    EndPoint( std::string& ep, size_t size=0, bool local=false);
    virtual bool isCompatibleLocal(const char *) const { return true; }
    //    EndPoint& operator=(EndPoint&);
    //    EndPoint& operator=(EndPoint*);
    void    release();
    virtual ~EndPoint();

    // Commit resources
    void finalize();
    // Get resources, and finalize if needed
    SMBResources &getSMBResources() { finalize(); return resources; };
    // Check compatibility
    bool canSupport(const char *remote_endpoint);

    // Sets smem location data based upon the specified endpoint
    OCPI::OS::int32_t setEndpoint( std::string& ep );

    // See if a string matches this endpoint
    bool matchEndPointString(const char *ep);

    // Parse the endpoint string
    static void getProtocolFromString( const char* ep, std::string & );
    static void parseEndPointString(const char* ep, uint16_t* mailBox, uint16_t* maxMb,
				    size_t* size);

    virtual SmemServices &createSmemServices() = 0;
    SmemServices *getSmemServices(); // ptr for legacy
    virtual void setReceiver(Receiver &receiver);
  };

  // Shared memory services.  
  class SmemServices // : public OCPI::Util::Child<XferFactory,SmemServices>
  {
 protected:
    EndPoint &m_endpoint;

  public:
    SmemServices (/*XferFactory * parent,*/ EndPoint &ep);

    /*
     * Attach to an existing shared memory object by name.
     *        Arguments:
     *                name        - Name of shared memory to attach to
     *        Returns:
     *                Returns 0 if success, platform dependent error otherwise
     *
     */
    virtual OCPI::OS::int32_t attach(EndPoint* loc);

    /*
     * Detach from shared memory object
     *        Arguments:
     *        Returns:
     *                Returns 0 if success, platform dependent error otherwise
     *
     */
    virtual OCPI::OS::int32_t detach();

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
    virtual void* map(DtOsDataTypes::Offset offset, size_t size ) = 0;

    /*
     * Unmap the current mapped view.
     *        Arguments:
     *        Returns:
     *                Returns 0 if success, platform dependent error otherwise
     *
     */
    virtual int32_t unMap();

    /*
     *        GetEndPoint - Returns the endpoint of the shared memory area
     */
    inline EndPoint * endpoint() {return &m_endpoint;}



  public:

    // Destructor
    virtual ~SmemServices ();
  };

}


#endif
