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

#include <CpiOsDataTypes.h>
#include <CpiOsMutex.h>
#include <DtTransferInterface.h>
#include <DtSharedMemoryInterface.h>
#include <xfer_if.h>


namespace DataTransfer {

  long  xfer_socket_start(XF_transfer xf_handle, CPI::OS::int32_t flags);

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

    // Constructors
    SocketEndPoint(CPI::OS::int32_t s=0)
      :EndPoint(s){};
      virtual ~SocketEndPoint();
      SocketEndPoint( std::string& ep, CPI::OS::uint32_t size=0)
        :EndPoint(ep,size){setEndpoint(ep);};

        // Sets smem location data based upon the specified endpoint
        virtual CPI::OS::int32_t setEndpoint( std::string& ep );

        // Get the address from the endpoint
        virtual const char* getAddress(){return ipAddress.c_str();}

        std::string ipAddress;
        int         portNum;
  };



  /**********************************
   * Each transfer implementation must implement a factory class.  This factory
   * implementation creates a named resource compatible SMB and a programmed I/O
   * based transfer driver.
   *********************************/
  class SocketServerT;
  class ClientSocketT;
  class SocketXferFactory : public XferFactory {

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
     * This method is used to allocate a transfer compatible SMB
     ***************************************/
    SmemServices* createSmemServices(EndPoint* ep );


    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    XferServices* getXferServices(SmemServices* source, SmemServices* target);


    /***************************************
     *  Get the location via the endpoint
     ***************************************/
    EndPoint* getEndPoint( std::string& end_point );
    void releaseEndPoint( EndPoint* loc );


    /***************************************
     *  This method is used to dynamically allocate
     *  an endpoint for an application running on "this"
     *  node.
     ***************************************/
    std::string allocateEndpoint(CPI::OS::uint32_t *size);

    /***************************************
     *  This method is used to flush any cached items in the factoy
     ***************************************/
    void clearCache();

  protected:

    CPI::OS::Mutex m_mutex;
    CPI::Util::VList g_locations;

  private:

  };


  /**********************************
   * This is the Programmed I/O transfer request class
   *********************************/
  class SocketXferServices;
  class SocketXferRequest : public XferRequest
  {

    // Public methods available to clients
  public:
    SocketXferRequest( SocketXferServices *s)
      :m_xferServices(s){}

    // General init
    void init (
               Creator cr, 
               Flags flags, 
               CPI::OS::uint32_t srcoffs, 
               Shape *psrcshape, 
               CPI::OS::uint32_t dstoffs, 
               Shape *pdstshape, 
               CPI::OS::uint32_t length );

    // Queue data transfer request
    void start (Shape* s_shape=NULL, Shape* t_shape=NULL);

    // Get Information about a Data Transfer Request
    CPI::OS::int32_t getStatus();

    // Get the transfer handle
    XF_transfer& getHandle();


    // Destructor - implementation at end of file
    virtual ~SocketXferRequest ();


    void modify( CPI::OS::uint32_t new_offsets[], CPI::OS::uint32_t old_offsets[] );

    SocketXferServices*                 m_xferServices;

    // Data members accessible from this/derived class
  protected:
    Creator                                                m_creator;                // What  method created this instance
    Flags                                                m_flags;                // Flags used during creation
    CPI::OS::uint32_t                        m_srcoffset;        // The source memory offset
    Shape                                                m_srcshape;                // The source shape
    CPI::OS::uint32_t                        m_dstoffset;        // The destination memory offset
    Shape                                                m_dstshape;                // The destination memory shape
    CPI::OS::uint32_t                        m_length;                // The length of the request in bytes
    XF_transfer                                        m_thandle;                // Transfer handle returned by xfer_xxx etal

  };




  // SocketXferServices specializes for MCOE-capable platforms
  class SocketSmemServices;
  class SocketXferServices : public XferServices
  {
    // So the destructor can invoke "remove"
    friend SocketXferRequest::~SocketXferRequest ();

  public:

    SocketXferServices(SmemServices* source, SmemServices* target)
      :XferServices(source,target){createTemplate( source, target);}


      // Create tranfer services template
      void createTemplate (SmemServices* p1, SmemServices* p2);

      // Create a transfer request
      XferRequest* copy (CPI::OS::uint32_t srcoffs, CPI::OS::uint32_t dstoffs, 
                         CPI::OS::uint32_t nbytes, XferRequest::Flags flags, XferRequest* add_to);

      // Create a 2-dimensional transfer request
      XferRequest* copy2D (CPI::OS::uint32_t srcoffs, Shape* psrc, 
                           CPI::OS::uint32_t dstoffs, Shape* pdst, XferRequest* add_to);

      // Group data transfer requests
      XferRequest* group (XferRequest* preqs[]);

      // Release a transfer request
      void release (XferRequest* preq);

      // Destructor
      virtual ~SocketXferServices ();

      // Source SMB services pointer
      SocketSmemServices* m_sourceSmb;

      // Target SMB services pointer
      SocketSmemServices* m_targetSmb;

      // Socket thread handler
      ClientSocketT * m_clientSocketT;

  protected:

      // add a new transfer request instance to the list
      void add (SocketXferRequest* pXferReq);

      // remove a specified transfer request instance from the list
      static void remove (SocketXferRequest* pXferReq);

      // remove all transfer request instances from the list for "this"
      void releaseAll ();



  private:

      // A map of data transfer requests to the instance that created it
      // to support rundow. Note that list access is not thread-safe.
      static CPI::Util::VList m_map;

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
  inline const char* SocketXferFactory::getProtocol(){return "cpi-socket-rdma";}
 
  // inline methods for SocketXferRequest
  inline XF_transfer& SocketXferRequest::getHandle(){return m_thandle;}
  inline CPI::OS::int32_t SocketXferRequest::getStatus(){return xfer_get_status (m_thandle);}

  // inline methods for SocketXferServices
  inline void SocketXferServices::add (SocketXferRequest* pXferReq){m_map.insert(pXferReq);}

}


#endif
