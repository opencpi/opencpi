
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
 *   This file contains the implementation for the programed I/O transfer class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef DataTransfer_PCI_PioTransfer_H_
#define DataTransfer_PCI_PioTransfer_H_

#include <map>
#include <vector>
#include <string>
#include <OcpiOsDataTypes.h>
#include <OcpiOsMutex.h>
#include <DtDriver.h>
#include <DtTransferInterface.h>
#include <xfer_if.h>
#include "DtSharedMemoryInterface.h"
#include "DtExceptions.h"
#include "DtPioXfer.h"

namespace OCPI {
  namespace OS {
    class Mutex;
  }
}

namespace DataTransfer {

  /**********************************
   * This is our PCI shared memory location implementation.  The format of the 
   * address is as follows:
   *     "ocpi-pci-pio:bid.base.size:300000.1.2"
   *  Where:
   *      bid:  is the PCI bus id
   *      name: is the name of the endpoint
   *********************************/
  class  PCIEndPoint : public EndPoint 
  {
  public:

      virtual ~PCIEndPoint();
      PCIEndPoint( std::string& ep, bool local)
        :EndPoint(ep, 0, local){parse(ep);};

        // Sets smem location data based upon the specified endpoint
        OCPI::OS::int32_t parse( std::string& ep );

        // Get the address from the endpoint
        virtual const char* getAddress(){return p_virt_addr;}
        char*              p_virt_addr;

        int bus_id;
  };


  class PCISmemServices : public DataTransfer::SmemServices
  {
    // Public methods available to clients
  public:

    // Create the service
    void create (PCIEndPoint* loc);

    // Close shared memory object.
    void close ();

    // Attach to an existing shared memory object by name.
    OCPI::OS::int32_t attach (EndPoint*);

    // Detach from shared memory object
    OCPI::OS::int32_t detach ();

    // Map a view of the shared memory area at some offset/size and return the virtual address.
    void* map (uint32_t offset, uint32_t size );

    // Unmap the current mapped view.
    OCPI::OS::int32_t unMap ();

    // Enable mapping
    void* enable ();

    // Disable mapping
    OCPI::OS::int32_t disable ();

    //        GetName - the name of the shared memory object
    const char* getName ()
      {
        return m_location->getAddress();
      }

    //        GetHandle - platform dependent opaque handle for current mapping
    void* getHandle ();

    // Ctor/dtor
    PCISmemServices ( XferFactory * p, EndPoint* loc ) 
      : DataTransfer::SmemServices( p, loc ), m_init(false),m_fd(-1)
      {
        m_location = dynamic_cast<PCIEndPoint*>(loc);
        create(m_location);

      }
      PCISmemServices ();
      virtual ~PCISmemServices ();

  protected:

      //        Our thread safe mutex
      OCPI::OS::Mutex m_threadSafeMutex;

      // Our location
      PCIEndPoint      *m_location;
      bool              m_init;
      OCPI::OS::uint64_t m_last_offset;
      unsigned int      m_size;
      int               m_fd;

      struct Map {
        OCPI::OS::uint64_t offset;
        unsigned int size;
        void*        vaddr;
        Map( OCPI::OS::uint64_t o,unsigned int s, void* v ):offset(o),size(s),vaddr(v){};
      };
      std::vector<Map> m_map;

      // Our virtual address
      void* m_vaddr;

  };

  /**********************************
   * Each transfer implementation must implement a factory class.  This factory
   * implementation creates a named resource compatible SMB and a programmed I/O
   * based transfer driver.
   *********************************/
  class PCIPIOXferFactory;
  class PCIPIODevice : public OCPI::Driver::DeviceBase<PCIPIOXferFactory,PCIPIODevice> {
  };
  extern const char *pci;
  class PCIPIOXferFactory : public DriverBase<PCIPIOXferFactory, PCIPIODevice,PIOXferServices,pci> {

  public:

    // Default constructor
    PCIPIOXferFactory()
      throw();

    // Destructor
    virtual ~PCIPIOXferFactory()
      throw ();

    // Get the transfer description
    const char* getDescription();

    // Get our protocol string
    const char* getProtocol();


    /***************************************
     * This method is used to allocate a transfer compatible SMB
     ***************************************/
    SmemServices* getSmemServices( EndPoint* ep );


    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    XferServices* getXferServices(SmemServices* source, SmemServices* target);


    /***************************************
     *  Get the location via the endpoint
     ***************************************/
    // EndPoint* getEndPoint( std::string& end_point, bool local  );
    EndPoint* createEndPoint(std::string& endpoint, bool local);
    // void releaseEndPoint( EndPoint* loc );


    /***************************************
     *  This method is used to dynamically allocate
     *  an endpoint for an application running on "this"
     *  node.
     ***************************************/
    std::string allocateEndpoint(const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes);

    /***************************************
     *  This method is used to flush any cached items in the factoy
     ***************************************/
    //    void clearCache();

    //  protected:

    // OCPI::OS::Mutex  m_mutex;


  };


  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

  // inline methods for PIOXferFactory
  inline const char* PCIPIOXferFactory::getProtocol(){return "ocpi-pci-pio";}
  inline const char* PCIPIOXferFactory::getDescription(){return "PCI Based pogrammed I/O transport";}

}




#endif
