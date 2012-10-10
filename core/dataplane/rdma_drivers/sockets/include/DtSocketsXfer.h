
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
 *    Date: 2/2010
 *    Revision Detail: Created
 *
 */

#ifndef DataTransfer_SocketsTransfer_H_
#define DataTransfer_SocketsTransfer_H_

#include <OcpiOsDataTypes.h>
#include <DtDriver.h>
#include <DtTransferInterface.h>
#include <DtSharedMemoryInterface.h>
#include <xfer_internal.h>
#include <xfer_if.h>


namespace DataTransfer {

  //  long  xfer_socket_start(XF_transfer xf_handle, OCPI::OS::int32_t flags);

  /**********************************
   * This is our GPP shared memory location implementation.  This class
   * relies shared memory implementations that support named resources.
   * Although this class allows for a hostname in the address, it is
   * currently not being used.
   *
   *   Address format:  IPAddr:Port:
   *********************************/
  class  SocketEndPoint : public EndPoint 
  {
  public:

    virtual ~SocketEndPoint();
    SocketEndPoint( std::string& ep, bool local, OCPI::OS::uint32_t size=0)
      : EndPoint(ep, size, local) { parse(ep);}

        // Sets smem location data based upon the specified endpoint
        OCPI::OS::int32_t parse( std::string& ep );

        // Get the address from the endpoint
        virtual const char* getAddress(){return ipAddress.c_str();}

	SmemServices &createSmemServices();
        std::string ipAddress;
        unsigned  portNum;
  };



  /**********************************
   * Each transfer implementation must implement a factory class.  This factory
   * implementation creates a named resource compatible SMB and a programmed I/O
   * based transfer driver.
   *********************************/
  //  class SocketServerT;
  class ClientSocketT;
  class SocketXferFactory;
  class SocketDevice : public OCPI::Driver::DeviceBase<SocketXferFactory,SocketDevice> {
  };
  class SocketXferServices;
  extern const char *socket;
  class SocketXferFactory : public DriverBase<SocketXferFactory, SocketDevice,SocketXferServices,socket> {

  public:

    // Default constructor
    SocketXferFactory()
      throw ();

    // Destructor
    virtual ~SocketXferFactory()
      throw ();

    // Get our protocol string
    const char* getProtocol();

    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    XferServices* getXferServices(SmemServices* source, SmemServices* target);


    /***************************************
     *  Get the location via the endpoint
     ***************************************/
    //    EndPoint* getEndPoint( std::string& end_point, bool );
    //void releaseEndPoint( EndPoint* loc );


    /***************************************
     *  Set (unparse, snprintf) the endpoint string
     ***************************************/
    static void setEndpointString(std::string &str, const char *ipAddr, unsigned port,
				  unsigned size, uint16_t mbox, uint16_t maxCount);
    /***************************************
     *  This method is used to dynamically allocate
     *  an endpoint for an application running on "this"
     *  node.
     ***************************************/
    std::string allocateEndpoint(const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes);

  protected:
    EndPoint* createEndPoint(std::string& endpoint, bool local = false);
    
    /***************************************
     *  This method is used to flush any cached items in the factoy
     ***************************************/
    //    void clearCache();

    //  protected:

    //    OCPI::OS::Mutex m_mutex;
    //    OCPI::Util::VList g_locations;

    //  private:

  };


  /**********************************
   * This is the Programmed I/O transfer request class
   *********************************/
  class SocketXferServices;
  class SocketXferRequest : public TransferBase<SocketXferServices,SocketXferRequest>
  {

    // Public methods available to clients
  public:
  SocketXferRequest( SocketXferServices & parent, XF_template temp )
    : TransferBase<SocketXferServices,SocketXferRequest>(parent, temp)
      //,m_thandle(0)
      {}

    // Queue data transfer request
    //    void post();

    // Get Information about a Data Transfer Request
    DataTransfer::XferRequest::CompletionStatus getStatus();

    // Get the transfer handle
    //    XF_transfer& getHandle();

    // Destructor - implementation at end of file
    virtual ~SocketXferRequest ();

#if 0

    void modify( OCPI::OS::uint32_t new_offsets[], OCPI::OS::uint32_t old_offsets[] );


    XferRequest & group( XferRequest* lhs );

    XferRequest* copy (OCPI::OS::uint32_t srcoff, 
		       OCPI::OS::uint32_t dstoff, 
		       OCPI::OS::uint32_t nbytes, 
		       XferRequest::Flags flags
		       );
#endif
    // Data members accessible from this/derived class
  private:
    //    OCPI::OS::int32_t xfer_socket_starti(PIO_transfer pio_transfer, OCPI::OS::int32_t);
    void action_transfer(PIO_transfer transfer);
  protected:
    Flags                                     m_flags;                // Flags used during creation
    OCPI::OS::uint32_t                        m_srcoffset;        // The source memory offset
    OCPI::OS::uint32_t                        m_dstoffset;        // The destination memory offset
    OCPI::OS::uint32_t                        m_length;                // The length of the request in bytes

    XF_transfer                               m_thandle;                // Transfer handle returned by xfer_xxx etal

  };




  // SocketXferServices specializes for MCOE-capable platforms
  class SocketSmemServices;
  class SocketXferServices : public ConnectionBase<SocketXferFactory,SocketXferServices,SocketXferRequest>
  {

    // So the destructor can invoke "remove"
    friend class SocketXferRequest;

  public:

    SocketXferServices(SmemServices* source, SmemServices* target)
      : ConnectionBase<SocketXferFactory,SocketXferServices,SocketXferRequest>(source,target)
    {
      createTemplate( source, target);
    }

    /*
     * Create tranfer request object
     */
     XferRequest* createXferRequest();

    // Destructor
    virtual ~SocketXferServices ();

    // Socket thread handler
    ClientSocketT * m_clientSocketT;


  protected:

      // Source SMB services pointer
      SocketSmemServices* m_sourceSmb;

      // Target SMB services pointer
      SocketSmemServices* m_targetSmb;

      // Create tranfer services template
      void createTemplate (SmemServices* p1, SmemServices* p2);


  private:

      // The handle returned by xfer_create
      XF_template        m_xftemplate;

      // Our transfer request
      XferRequest* m_txRequest;

  };




  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

  // inline methods for SocketXferFactory
  inline const char* SocketXferFactory::getProtocol(){return "ocpi-socket-rdma";}

  // inline methods for SocketXferRequest
  //  inline XF_transfer& SocketXferRequest::getHandle(){return m_thandle;}

  inline DataTransfer::XferRequest::CompletionStatus SocketXferRequest::getStatus()
    { return xfer_get_status (m_thandle) == 0 ? DataTransfer::XferRequest::CompleteSuccess : DataTransfer::XferRequest::Pending;}

}


#endif
