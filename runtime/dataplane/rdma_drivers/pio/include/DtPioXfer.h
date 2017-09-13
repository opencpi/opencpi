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

/*
 * Abstract:
 *   This file contains the declarations for the programmed I/O transfer class
 *   This header exists outside the PioXfer driver because some aspects of it
 *   are inherited by other drivers (e.g. PCI DMA).
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

#include <DtDriver.h>

namespace OCPI {
  namespace PIO {

    /**********************************
     * This is our GPP shared memory location implementation.  This class
     * relies shared memory implementations that support named resources.
     * Although this class allows for a hostname in the address, it is
     * currently not being used.
     *********************************/
    class EndPoint : public DataTransfer::EndPoint 
    {
    public:
      EndPoint( std::string& ep, bool local = false);
      virtual ~EndPoint();

      // Sets smem location data based upon the specified endpoint
      int32_t parse(std::string& ep);

    protected:
      DataTransfer::SmemServices &createSmemServices();

    private:
      std::string m_smb_name;
    };


    /**********************************
     * Each transfer implementation must implement a factory class.  This factory
     * implementation creates a named resource compatible SMB and a programmed I/O
     * based transfer driver.
     *********************************/
    class XferFactory;
    class Device : public DataTransfer::DeviceBase<XferFactory,Device> {
      Device(const char *name);
    };

    class XferServices;
    extern const char *pio;
    class XferFactory : public DataTransfer::DriverBase<XferFactory, Device, XferServices,pio> {
      friend class XferServices;
    public:
      // Default constructor
      XferFactory()
	throw ();

      // Destructor
      virtual ~XferFactory()
	throw ();

      // Get our protocol string
      const char* getProtocol();

      /***************************************
       * This method is used to allocate a transfer compatible SMB
       ***************************************/
      DataTransfer::SmemServices* getSmemServices(DataTransfer::EndPoint* ep );

      /***************************************
       *  This method is used to create a transfer service object
       ***************************************/
      DataTransfer::XferServices* getXferServices(DataTransfer::SmemServices* source,
						  DataTransfer::SmemServices* target);

      /***************************************
       *  Get the location via the endpoint
       ***************************************/
      DataTransfer::EndPoint* createEndPoint(std::string& endpoint, bool local);

      /***************************************
       *  This method is used to dynamically allocate
       *  an endpoint for an application running on "this"
       *  node.
       ***************************************/
      std::string allocateEndpoint(const OCPI::Util::PValue*, uint16_t mailBox,
				   uint16_t maxMailBoxes, size_t size);
    };

    /**********************************
     * This is the Programmed I/O transfer request class
     *********************************/
    class XferRequest : public DataTransfer::TransferBase<XferServices, XferRequest>
      {
      public:
	// Constructor
	XferRequest(XferServices & parent, XF_template temp);

	// Destructor - implementation at end of file
	virtual ~XferRequest();
      };

    class XferServices
      : public DataTransfer::ConnectionBase<XferFactory,XferServices,XferRequest>
      {
	// So the destructor can invoke "remove"
	friend class XferRequest;
      public:
	XferServices(DataTransfer::SmemServices* source, DataTransfer::SmemServices* target);
	virtual ~XferServices();

	virtual DataTransfer::XferRequest* createXferRequest();

      protected:
	// Create tranfer services template
	void createTemplate (DataTransfer::SmemServices* p1, DataTransfer::SmemServices* p2);

      private:
	// The handle returned by xfer_create
	XF_template m_xftemplate;

	// Our transfer request
	DataTransfer::XferRequest* m_txRequest;

	// Source SMB services pointer
	DataTransfer::SmemServices* m_sourceSmb;

	// Target SMB services pointer
	DataTransfer::SmemServices* m_targetSmb;

      };
  }
}

#endif