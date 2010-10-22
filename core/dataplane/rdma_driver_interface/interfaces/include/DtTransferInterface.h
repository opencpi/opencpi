
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
   This file contains the Interface for all SMB transfer types.

   Revision History:

   3/1/2005 - John Miller
   Initial version.

   2/18/2009 - John Miller
   Removed exception monitor class.

************************************************************************** */

#ifndef DataTransfer_TransferInterface_H_
#define DataTransfer_TransferInterface_H_

#include <string.h>
#include <OcpiOsDataTypes.h>
#include <OcpiList.h>
#include <OcpiDriver.h>

namespace DataTransfer {

  // Forward references
  class SmemServices;
  struct EndPoint;

  // EndPoint shape (basically equivalent to EP_shape but not dependent on MCOS).
  // The layout and field names are identical to EP_shape to simplify MCOE-capable platforms.
  typedef struct {
    OCPI::OS::uint32_t element_size;   /* Element size in bytes */
    struct {
      OCPI::OS::uint32_t offset;       
      OCPI::OS::uint32_t length;       /* Number of elements in X dimension */
      OCPI::OS::uint32_t stride;       /* X element stride */
    } x;                              /* X dimension (i.e. rows) */
    struct {
      OCPI::OS::uint32_t offset;       
      OCPI::OS::uint32_t length;       /* Number of elements in Y dimension */
      OCPI::OS::uint32_t stride;       /* Y element stride */
    } y;                              /* Y dimension (i.e. columns) */
    struct {
      OCPI::OS::uint32_t offset;       
      OCPI::OS::uint32_t length;       /* Number of elements in Z dimension */
      OCPI::OS::uint32_t stride;       /* Z element stride */
    } z;                              /* Z dimension (i.e. columns) */
  } Shape;
         

  // A single request to perform a data transfer, this is effectively a transfer template
  // that is used aOCPI::OS::int32_t with a transfer service object to describe a transfer.
  class XferRequest
  {
    // Types
  public:

    // XferServices method used to create this request
    typedef enum { Copy, Copy2D, Group } Creator;
                 
    // Flags used when created
    typedef enum { 
      None = 0, 
      FlagTransfer       = 0x01,         // 
      FirstTransfer      = 0x02, // First transfer in list
      LastTransfer       = 0x04, // Last transfer in list
      SizeModifiable     = 0x06, // Size of this transfer is modifiable on start
      DataOffset         = 0x08, // Source offset
      WakeupNotification = 0x10  // Notification transfer
    }  Flags;

    /*
     * Queue Data Transfer Request
     *        Arguments:
     *    s_shape - Used to modify the source shape for all requests created using
     *                                the SizeModifiable flag.
     *    t_shape - Used to modify the target shape for all requests created using
     *                                the SizeModifiable flag.
     *    
     * Returns: void
     *        
     *        Throws:
     *                DataTransferEx for all exception conditions
     */
    virtual void start ( Shape* s_shape=NULL, Shape* t_shape=NULL) = 0;

    /*
     * Get Information about a Data Transfer Request
     *        Arguments:
     *        Returns: 
     *     0 if transfer complete 1 if transfer is pending
     *        
     *        Throws:
     *                DataTransferEx for all exception conditions
     */
    virtual OCPI::OS::int32_t getStatus () = 0;


    /*
     * Modify the source offset pointers
     *        Arguments:
     *     new_offsets[] - List of new source offsets
     *
     *     old_offsets[] - Returned list of old offsets
     *
     *        Returns: 
     *    void
     *        
     *        Throws:
     *                DataTransferEx for all exception conditions
     */
    virtual void modify( OCPI::OS::uint32_t new_offsets[], OCPI::OS::uint32_t old_offsets[] )=0;
                 

    // Destructor - Note that invoking OcpiXferServices::Release is the preferred method.
    virtual ~XferRequest () {};

  };

         
  // Platform dependent data transfer services.  This class is responsible for 
  // creating the appropriate transfer service that is capable of transfering data
  // to and from the specifed shared memory objects.
  class XferServices
  {
                
  public:
                 
    /*
     * Create tranfer services template
     *        Arguments:
     *                p1        - Source memory instance
     *                p2        - Destination memory instance
     *        Returns: void
     *
     *        Errors:
     *                DataTransferEx for all exception conditions
     */
    XferServices (SmemServices* source, SmemServices* target)
    {
      ( void ) source;
      ( void ) target;
    }
                 
    /*
     * Create a transfer request.
     *        Arguments:
     *                srcoffs        - Source memory offset
     *                dstoffs        - Destination memory offset
     *                nbytes        - number of bytes in request
     *                flags        - Copy "flags"
     *                xfer        - receives transfer request instance
     *        Returns: void
     *
     *        Errors:
     *                DataTransferEx for all exception conditions
     */
    virtual XferRequest* copy (OCPI::OS::uint32_t srcoff, 
                               OCPI::OS::uint32_t dstoff, 
                               OCPI::OS::uint32_t nbytes, 
                               XferRequest::Flags flags,
                               XferRequest* add_to
                               ) = 0;
                 
    /*
     * Create a 2-dimensional transfer request.
     *        Arguments:
     *                srcoffs        - Source memory offset
     *                psrc        - Source shape
     *                dstoffs        - Destination memory offset
     *                pdst        - Destination shape
     *                nbytes        - number of bytes in request
     *                flags        - Copy "flags"
     *                xfer        - receives transfer request instance
     *        Returns: void
     *
     *        Errors:
     *                DataTransferEx for all exception conditions
     */
    virtual XferRequest* copy2D (OCPI::OS::uint32_t srcoffs, 
                                 Shape* psrc, 
                                 OCPI::OS::uint32_t dstoffs,
                                 Shape* pdst,
                                 XferRequest* add_to
                                 ) = 0;
                 
                 
    /*
     * Group data transfer requests.
     *        Arguments:
     *                preqs        - List of transfer request instances to group
     *                preq        - receives transfer request instance for the group
     *        Returns: void
     *
     *        Errors:
     *                DataTransferEx for all exception conditions
     */
    virtual XferRequest* group (XferRequest* preqs[]) = 0;
                 
                 
    /*
     * Release a transfer request that was created using Copy, Copy2D, or Group.
     *        Arguments:
     *                preq        - transfer request instance to release.
     *        Returns: void
     *
     *        Errors:
     *                DataTransferEx for all exception conditions
     */
    virtual void release (XferRequest* preq) = 0;
                 
                 
    // Destructor - implementations are required to track all OcpiXferRequests that they
    // produce (via Copy, Copy2D, and Group) and dispose of them when destructed.
    virtual ~XferServices () {};

  };
         
         
         
  // Each transfer implementation must implement a factory class
  class XferFactory : public OCPI::Util::Driver {
                 
  public:

    // Default constructor
    XferFactory(const char*)
      throw ();
                 
    // Destructor
    virtual ~XferFactory()
      throw ();
                 
    // Get our protocol string
    virtual const char* getProtocol()=0;

    // Transfer factories may not need these, so we will provide defaults
    virtual OCPI::Util::Device *probe(const OCPI::Util::PValue* props, const char *which )
      throw ( OCPI::Util::EmbeddedException ){ ( void ) props; ( void ) which; return NULL;}

    virtual unsigned search(const OCPI::Util::PValue* props, const char **exclude)
      throw (OCPI::Util::EmbeddedException) { ( void ) props; ( void ) exclude; return 1;}

    /***************************************
     * This method is called on this factory to dertermine if it supports
     * the specified endpoints.  This method may be called with either one
     * of the endpoints equal to NULL.
     ***************************************/
    virtual bool supportsEndPoints(
                                   std::string& end_point1, 
                                   std::string& end_point2 );
                 
                
    /***************************************
     * This method creates a specialized SmeLocation object.  This call should
     * cache locations and return the same location object for identical strings.
     ***************************************/
    virtual EndPoint* getEndPoint( std::string& endpoint )=0;
    virtual void releaseEndPoint( EndPoint* loc ) = 0;
                 
    /***************************************
     *  This method is used to dynamically allocate
     *  a source endpoint for an application running on "this"
     *  node.  This endpoint does not need to be finalized until
     * it has beed passed to the finalizeEndpoint().
     ***************************************/
    virtual std::string allocateEndpoint(OCPI::OS::uint32_t *size) = 0;

    /***************************************
     *  This method is used to allocate a transfer compatible SMB
     ***************************************/
    virtual SmemServices* createSmemServices( EndPoint* ep )=0;

    /***************************************
     *  This method is used to get a transfer service object. The implementation
     * of this method should cache the service object if possible so that they
     * can be re-used.
     ***************************************/
    virtual XferServices* getXferServices(SmemServices* source, SmemServices* target)=0;

  };
        
}


#endif
