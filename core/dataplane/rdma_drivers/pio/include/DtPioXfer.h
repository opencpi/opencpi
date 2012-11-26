
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

#ifndef DataTransfer_PioTransfer_H_
#define DataTransfer_PioTransfer_H_

#include <OcpiOsDataTypes.h>
#include <OcpiOsMutex.h>
#include <DtDriver.h>
#include <DtTransferInterface.h>
#include <DtSharedMemoryInterface.h>
#include <xfer_if.h>


namespace DataTransfer {

  /**********************************
   * This is our GPP shared memory location implementation.  This class
   * relies shared memory implementations that support named resources.
   * Although this class allows for a hostname in the address, it is
   * currently not being used.
   *********************************/
  class  GppEndPoint : public EndPoint 
  {
  public:

      virtual ~GppEndPoint();
  GppEndPoint( std::string& ep, bool local = false)
    :EndPoint(ep, 0, local){parse(ep);};

        // Sets smem location data based upon the specified endpoint
        OCPI::OS::int32_t parse( std::string& ep );

        // Get the address from the endpoint
        virtual const char* getAddress(){return m_smb_name.c_str();}
  protected:
	SmemServices &createSmemServices();
  private:
        std::string m_smb_name;

  };



  /**********************************
   * Each transfer implementation must implement a factory class.  This factory
   * implementation creates a named resource compatible SMB and a programmed I/O
   * based transfer driver.
   *********************************/
  class PIOXferFactory;
  class PIODevice : public OCPI::Driver::DeviceBase<PIOXferFactory,PIODevice> {
  };
  class PIOXferServices;
  extern const char *pio;
  class PIOXferFactory : public DriverBase<PIOXferFactory, PIODevice,PIOXferServices,pio> {
    friend class PIOXferServices;
  public:

    // Default constructor
    PIOXferFactory()
      throw ();

    // Destructor
    virtual ~PIOXferFactory()
      throw ();

    // Get our protocol string
    const char* getProtocol();


    /***************************************
     * This method is used to allocate a transfer compatible SMB
     ***************************************/
    SmemServices* getSmemServices(EndPoint* ep );


    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    XferServices* getXferServices(SmemServices* source, SmemServices* target);


    /***************************************
     *  Get the location via the endpoint
     ***************************************/
    //    EndPoint* getEndPoint( std::string& end_point, bool local );
    DataTransfer::EndPoint* createEndPoint(std::string& endpoint, bool local);
    //    void releaseEndPoint( EndPoint* loc );


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

    //    OCPI::OS::Mutex m_mutex;
    //    OCPI::Util::VList g_locations;

    //  private:

  };


  /**********************************
   * This is the Programmed I/O transfer request class
   *********************************/
  class PIOXferRequest : public TransferBase<PIOXferServices,PIOXferRequest>
  {

  public:

    // Constructor
  PIOXferRequest( PIOXferServices & parent, XF_template temp)
    : TransferBase<PIOXferServices,PIOXferRequest>(parent, *this, temp)
      //      ,m_thandle(NULL)
{}

#if 0
    XferRequest & group( XferRequest* lhs );

    XferRequest* copy (OCPI::OS::uint32_t srcoff, 
		       OCPI::OS::uint32_t dstoff, 
		       OCPI::OS::uint32_t nbytes, 
		       XferRequest::Flags flags
		       );
#endif
    // Queue data transfer request
    //    void post();

    // Get Information about a Data Transfer Request
    //DataTransfer::XferRequest::CompletionStatus getStatus();

    // Destructor - implementation at end of file
    virtual ~PIOXferRequest ();

    //    void modify( OCPI::OS::uint32_t new_offsets[], OCPI::OS::uint32_t old_offsets[] );

    // Data members accessible from this/derived class
    //  protected:

    // Get the transfer handle
    //    XF_transfer & getHandle();
    //    XF_transfer               m_thandle;                // Transfer handle returned by xfer_xxx etal

  };



  // PIOXferServices specializes for MCOE-capable platforms
  class PIOXferServices : public ConnectionBase<PIOXferFactory,PIOXferServices,PIOXferRequest>
  {
    // So the destructor can invoke "remove"
    friend class PIOXferRequest;
    

  public:

    PIOXferServices(SmemServices* source, SmemServices* target)
      : ConnectionBase<PIOXferFactory,PIOXferServices,PIOXferRequest>(*this, source,target)
      {
	createTemplate( source, target);
      }

      virtual XferRequest* createXferRequest();

      // Destructor
      virtual ~PIOXferServices ();

  protected:


      // Create tranfer services template
      void createTemplate (SmemServices* p1, SmemServices* p2);

      // remove all transfer request instances from the list for "this"
      void releaseAll ();

  private:

      // The handle returned by xfer_create
      XF_template        m_xftemplate;

      // Our transfer request
      XferRequest* m_txRequest;

      // Source SMB services pointer
      SmemServices* m_sourceSmb;

      // Target SMB services pointer
      SmemServices* m_targetSmb;

  };



  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

  // inline methods for PIOXferFactory
  inline const char* PIOXferFactory::getProtocol(){return "ocpi-smb-pio";}
}


#endif
