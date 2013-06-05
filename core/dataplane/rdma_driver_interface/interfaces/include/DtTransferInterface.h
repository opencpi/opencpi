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

#include <set> 
#include <vector>
#include <string.h>
#include "ezxml.h"
#include <OcpiUtilSelfMutex.h>
#include <OcpiList.h>
#include <OcpiDriverManager.h>
#include "DtOsDataTypes.h"

// Forward declarations to avoid circular dependencies.
typedef struct XFTransfer* XF_transfer;
typedef struct XFTemplate* XF_template;
typedef struct pio_transfer_ * PIO_transfer;

namespace DataTransfer {

  // Forward references
  class SmemServices;
  class XferServices;
  class XferFactory;
  struct EndPoint;
  typedef std::set<EndPoint *> EndPoints;
  typedef EndPoints::iterator EndPointsIter;



  // A single request to perform a data transfer, this is effectively a transfer 
  // template that is used aOCPI::OS::int32_t with a transfer service object to 
  // describe a transfer.  This base class is specialized by each transfer driver
  class XferRequest
  {
  public:
   
    // Constructor
    XferRequest(XF_template temp = NULL);

    // Flags used when created
    typedef enum { 
      None = 0, 
      DataTransfer       = 0x01, 
      MetaDataTransfer      = 0x02, // First transfer in list
      FlagTransfer       = 0x04, // Last transfer in list
      SizeModifiable     = 0x06, // Size of this transfer is modifiable on start
      WakeupNotification = 0x10  // Notification transfer
    }  Flags;

    typedef enum {
      CompleteSuccess = 0,
      CompleteFailure = 1,
      Pending = 2
    } CompletionStatus;

    /*
     * Queue Data Transfer Request
     *    
     * Returns: void
     *        
     *        Throws:
     *                DataTransferEx for all exception conditions
     */
    virtual void post ();

    /*
     * Get Information about a Data Transfer Request
     *        Arguments:
     *        Returns: 
     *     0 if transfer complete 1 if transfer is pending
     *        
     *        Throws:
     *                DataTransferEx for all exception conditions
     */
    virtual CompletionStatus  getStatus ();


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
    virtual void modify( DtOsDataTypes::Offset new_offsets[],
			 DtOsDataTypes::Offset old_offsets[] );


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
    virtual XferRequest* copy (DtOsDataTypes::Offset srcoff, 
                               DtOsDataTypes::Offset dstoff, 
                               size_t nbytes, 
                               XferRequest::Flags flags
                               );

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
    virtual XferRequest & group( XferRequest* lhs );

    // Perform a PIO transfer.  Default null implementation when no using default "post" method
    virtual void action_transfer(PIO_transfer);
    virtual void start_pio(PIO_transfer);
    // Destructor - Note that invoking OcpiXferServices::Release is the preferred method.
    virtual ~XferRequest ();
  private:
    XF_transfer m_thandle;                // Transfer handle returned by xfer_xx
    XF_template m_xftemplate;             // parent's template
  };

         
  // Driver dependent data transfer services.  Instances of the driver classes that
  // inherit this base class manage a connection between a local and remote 
  // endpoint and create transfers between them (instances of the driver class that 
  // inherits from XferRequest above).
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
    XferServices ( SmemServices * /* source */ , SmemServices * /* target */ ){};

    /*
     * If this service requires a cookie for the remote connection, this method will return it.
     */
    virtual uint64_t getConnectionCookie(){return 0;}


    /*
     * Finalize the connection with the remote cookie
     */
    virtual void finalize( uint64_t /* cookie */ ){}

                 
    /*
     * Create tranfer request object
     */
    virtual XferRequest* createXferRequest() = 0;

                 
    /*
     * Destructor - implementations are required to track all OcpiXferRequests that they
     * produce (via Copy, Copy2D, and Group) and dispose of them when destructed.
     */
    virtual ~XferServices () {};

  };


  // This is the base class for a factory configuration sheet
  // that is common to the manager, drivers, and devices
  class FactoryConfig {
  public:
    // These are arguments to allow different defaults
    FactoryConfig(size_t smbSize = 0, size_t retryCount = 0);
    void parse(FactoryConfig *p, ezxml_t config );
    size_t m_SMBSize;
    size_t m_retryCount;
    ezxml_t  m_xml; // the element that these attributes were parsed from
  };
         
  // This class is the transfer driver base class.
  // All drivers indirectly inherit this by using the template class in DtDriver.h
  // Its parent relationship with actual devices is managed by that transfer driver
  // template and thus is not evident here.
  class XferFactoryManager;
  class XferFactory
    : public OCPI::Driver::DriverType<XferFactoryManager, XferFactory>,
    //      public OCPI::Util::Parent<SmemServices>,
      public FactoryConfig,
      virtual protected OCPI::Util::SelfMutex
  {
  public:

    XferFactory(const char *);
                 
    // Destructor
    virtual ~XferFactory();
                 
    // Configure from xml
    void configure(ezxml_t x);

    // Get our protocol string
    virtual const char* getProtocol()=0;

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
    virtual EndPoint* getEndPoint(const char *endpoint, bool local=false, bool cantExist = false );
    inline EndPoint* getEndPoint(const std::string &s, bool local=false) {
      return getEndPoint(s.c_str(), local);
    }
    EndPoint* addCompatibleLocalEndPoint(const char *remote, uint16_t mailBox, uint16_t maxMb);
    EndPoint* addEndPoint(const char *endpoint, bool local);
    // Avoid the mailbox, and match the mailbox count, if not -1
    virtual EndPoint* createEndPoint(std::string& endpoint, bool local=false) = 0;

    /***************************************
     *  This method is used to dynamically allocate
     *  a source endpoint for an application running on "this"
     *  node.  This endpoint does not need to be finalized until
     * it has been passed to finalizeEndpoint().
     ***************************************/
    virtual std::string allocateEndpoint(const OCPI::Util::PValue*,
					 uint16_t mailBox, uint16_t maxMailBoxes) = 0;
    virtual std::string allocateCompatibleEndpoint(const OCPI::Util::PValue*params,
						   const char *remote,
						   uint16_t mailBox, uint16_t maxMailBoxes);

    // The endpoint is telling its factory that it is being deleted.
    void removeEndPoint(EndPoint &ep);
    /***************************************
     *  This method is used to dynamically allocate
     *  source endpoints for all this driver's devices, for an application running on "this"
     *  node.  These endpoints do not need to be finalized until
     *  then have been passed to finalizeEndpoint().
     ***************************************/
    virtual void allocateEndpoints(std::vector<std::string> &l);

#if 0 // moved to be an endpoint method
    /***************************************
     *  This method is used to allocate a transfer compatible SMB
     ***************************************/
    virtual SmemServices* getSmemServices( EndPoint* ep )=0;
#endif

    /***************************************
     *  This method is used to get a transfer service object. The implementation
     * of this method should cache the service object if possible so that they
     * can be re-used.
     ***************************************/
    virtual XferServices* getXferServices(SmemServices* source, SmemServices* target)=0;


    /***************************************
     *  Gets the first node specified by name, otherwise null
     ***************************************/    
    static ezxml_t getNode( ezxml_t tn, const char* name );
    // The range of mailboxes is common across all transports.
    // This will allow us to share memory between protocol someday.
    uint16_t getMaxMailBox(), getNextMailBox();
  private:
    // This vector is indexed by the mailbox number of the endpoint
    // and only contains local endpoints
    std::vector<EndPoint *> m_locations;
    // This is just a set of all endpoints
    EndPoints m_endPoints;
  };
  // OCPI::Driver::Device is virtually inherited to give access
  // to the class that is not normally inherited here.
  // This also delays the calling of the destructor
  class Device : public FactoryConfig, virtual public OCPI::Driver::Device {
    virtual XferFactory &driverBase() = 0;
  protected:
    void configure(ezxml_t x);
  };
}


#endif
