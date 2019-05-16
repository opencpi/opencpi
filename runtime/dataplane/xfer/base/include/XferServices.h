/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef XferServices_H
#define XferServices_H

#include <set> 
#include <map>
#include <vector>
#include <string.h>
#include "ezxml.h"
#include "OcpiUtilSelfMutex.h"
#include "OcpiDriverManager.h"
#include "XferEndPoint.h"

// Forward declarations to avoid circular dependencies.
typedef struct XFTransfer* XF_transfer;
typedef struct XFTemplate* XF_template;
typedef struct pio_transfer_ * PIO_transfer;

namespace DataTransfer {

  // Forward references
  class XferServices;
  class XferFactory;
  class EndPoint;

  // A single request to perform a data transfer, this is effectively a transfer 
  // template that is used aOCPI::OS::int32_t with a transfer service object to 
  // describe a transfer.  This base class is specialized by each transfer driver
  class XferRequest
  {
  public:
   
    // Constructor
    XferRequest(XFTemplate *temp = NULL);

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
    virtual void modify(Offset new_offsets[],
			Offset old_offsets[] );


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
    virtual XferRequest* copy (Offset srcoff, 
                               Offset dstoff, 
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
    virtual void action_transfer(PIO_transfer, bool last=false);
    virtual void start_pio(PIO_transfer, bool last=false);
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
  class XferServices {
    friend class XferFactory;
    XferFactory &m_driver;
    unsigned m_users; // how many ports are using this template?
  protected:
    EndPoint &m_from, &m_to;
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
    XferServices(XferFactory &driver, EndPoint &source, EndPoint &target);
    virtual ~XferServices();
    EndPoint &from() { return m_from; }
    EndPoint &to() { return m_to; }
    void addRef() { m_users++; }
    void release() {
      ocpiLog(9, "Releasing xferservices/template/connection with refcount %u", m_users);
      assert(m_users);
      if (!--m_users)
	delete this;
    }

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

    // Send the given data to destination directly - like  write(2)
    virtual void send(DtOsDataTypes::Offset offset, uint8_t *data, size_t nbytes);
    /*
     * Destructor - implementations are required to track all OcpiXferRequests that they
     * produce (via Copy, Copy2D, and Group) and dispose of them when destructed.
     */
  };
}


#endif
