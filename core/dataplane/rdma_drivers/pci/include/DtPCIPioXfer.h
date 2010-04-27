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
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef DataTransfer_PCI_PioTransfer_H_
#define DataTransfer_PCI_PioTransfer_H_

#include <CpiOsDataTypes.h>
#include <DtTransferInterface.h>
#include <xfer_if.h>

namespace CPI {
  namespace OS {
    class Mutex;
  }
}

namespace DataTransfer {

  /**********************************
   * Each transfer implementation must implement a factory class.  This factory
   * implementation creates a named resource compatible SMB and a programmed I/O
   * based transfer driver.
   *********************************/
  class PCIPIOXferFactory : public XferFactory {

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
    SmemServices* createSmemServices( EndPoint* ep );


    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    XferServices* getXferServices(SmemServices* source, SmemServices* target);


    /***************************************
     *  Get the location via the endpoint
     ***************************************/
    EndPoint* getEndPoint( std::string& end_point  );
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

    CPI::OS::Mutex  m_mutex;

  };


  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

  // inline methods for PIOXferFactory
  inline const char* PCIPIOXferFactory::getProtocol(){return "cpi-pci-pio";}
  inline const char* PCIPIOXferFactory::getDescription(){return "PCI Based pogrammed I/O transport";}

}




#endif
