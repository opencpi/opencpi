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

      // Get the address from the endpoint
      virtual const char* getAddress();

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
				   uint16_t maxMailBoxes);
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
