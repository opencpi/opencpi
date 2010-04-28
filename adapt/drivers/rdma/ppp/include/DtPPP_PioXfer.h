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

#ifndef DataTransfer_PPP_PioTransfer_H__
#define DataTransfer_PPP_PioTransfer_H__

#include <CpiOsDataTypes.h>
#include <DtTransferInterface.h>

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
  class PPPPIOXferFactory : public XferFactory {

  public:

    // Default constructor
    PPPPIOXferFactory()
      throw();

    // Destructor
    virtual ~PPPPIOXferFactory()
      throw();

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

    CPI::OS::Mutex* m_mutex;

  private:

  };


  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

  // inline methods for PIOXferFactory
  inline const char* PPPPIOXferFactory::getProtocol(){return "cpi-ppp-pio";}
  inline const char* PPPPIOXferFactory::getDescription(){return "PPP Based pogrammed I/O transport";}

}

#endif
