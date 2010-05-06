/*
 * Abstact:
 *   This file contains the interface for the PPP based DMA transfer class.
 *
 * Revision History: 
 *
 *   06/23/09 - John Miller
 *   Integrated and tested modify method.
 *
 *   06/01/09 - John Miller
 *   Added modify method.
 *
 *   03/01/08 - John Miller
 *   Initial verions
 * 
 *
 */

#ifndef DataTransfer_PPP_DMATransfer_H_
#define DataTransfer_PPP_DMATransfer_H_

#include <CpiOsDataTypes.h>
#include <DtTransferInterface.h>
#include <CpiPPPSMemServices.h>
#include <rose/rose_defs.h>
#include <rose/rose_user_if.h>
#include <rose/rose_iovec.h>


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
  class PPPDMAXferFactory : public XferFactory {

  public:

    // Default constructor
    PPPDMAXferFactory()
      throw();

    // Destructor
    virtual ~PPPDMAXferFactory()
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

    // Process wide endpoint locations
    static CPI::Util::VList g_locations;

  };


  /**********************************
   * This is the Programmed I/O transfer request class
   *********************************/
  class PPPDMAXferRequest : public XferRequest
  {

    // Public methods available to clients
  public:

    // General init
    PPPDMAXferRequest ();

    int addTransfer(
                    Creator cr, 
                    Flags flags, 
                    CPI::OS::uint32_t srcoffs, 
                    Shape *psrcshape, 
                    CPI::OS::uint32_t dstoffs, 
                    Shape *pdstshape, 
                    CPI::OS::uint32_t length,
                    PPPEndPoint* sEp,
                    PPPEndPoint* dEp
                    );

    // Queue data transfer request
    void start (Shape* s_shape=NULL, Shape* t_shape=NULL);

    // Get Information about a Data Transfer Request
    CPI::OS::int32_t getStatus();

    // Destructor - implementation at end of file
    virtual ~PPPDMAXferRequest ();

    // Modify the source offsets
    virtual void modify( CPI::OS::uint32_t new_offsets[], CPI::OS::uint32_t old_offsets[] );

    // Data members accessible from this/derived class
  protected:
        
    void dump_transfer();

    Creator                   m_creator;                // What  method created this instance
    int                   m_index;        // index into the vector
    RoseIovec                    m_iovec;                    
    RoseUint32Type        m_iovec_flags;
    RoseIovecHandle       m_iovec_handle;
    bool                  m_init;

  };




  // PIOXferServices specializes for MCOE-capable platforms
  class PPPDMAXferServices : public XferServices
  {
    // So the destructor can invoke "remove"
    friend PPPDMAXferRequest::~PPPDMAXferRequest ();

  public:

  PPPDMAXferServices(SmemServices* source, SmemServices* target)
    :XferServices(source,target){createTemplate( source, target);}


    // Create tranfer services template
    void createTemplate (SmemServices* p1, SmemServices* p2);

    // Create a transfer request
    XferRequest* copy (CPI::OS::uint32_t srcoffs, CPI::OS::uint32_t dstoffs, 
                       CPI::OS::uint32_t nbytes, XferRequest::Flags flags, XferRequest* );

    // Create a 2-dimensional transfer request
    XferRequest* copy2D (CPI::OS::uint32_t srcoffs, Shape* psrc, 
                         CPI::OS::uint32_t dstoffs, Shape* pdst, XferRequest* );

    // Group data transfer requests
    XferRequest* group (XferRequest* preqs[]);

    // Release a transfer request
    void release (XferRequest* preq);

    // Destructor
    virtual ~PPPDMAXferServices ();

  protected:

    // add a new transfer request instance to the list
    void add (PPPDMAXferRequest* pXferReq);

    // remove a specified transfer request instance from the list
    static void remove (PPPDMAXferRequest* pXferReq);

    // remove all transfer request instances from the list for "this"
    void releaseAll ();

  private:

    // A map of data transfer requests.
    static CPI::Util::VList m_map;
     
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
  inline const char* PPPDMAXferFactory::getProtocol(){return "cpi-ppp-dma";}
  inline const char* PPPDMAXferFactory::getDescription(){return "PPP Based DMA transport";}
 
  // inline methods for PIOXferServices
  inline void PPPDMAXferServices::add (PPPDMAXferRequest* pXferReq){m_map.insert(pXferReq);}

}


#endif
