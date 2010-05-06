/*
 * Abstact:
 *   This file contains the implementation for the base class for SMB transfers.
 *
 * Author: John F. Miller
 *
 * Date: 7/20/04
 *
 */

#include <DtPioXfer.h>
#include <DtPPP_PioXfer.h>
#include <CpiPPPSMemServices.h>
#include <CpiOsMutex.h>
#include <CpiOsMisc.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <CpiOsAssert.h>

using namespace DataTransfer;
using namespace CPI::Util;
using namespace CPI::OS;
static VList g_locations;

static PPPPIOXferFactory *g_pppPioFactory = new PPPPIOXferFactory;

PPPPIOXferFactory::PPPPIOXferFactory()
  throw ()
  : XferFactory("PPP PIO transfer driver")
{
  printf("In PPPPIOXferFactory::PPPPIOXferFactory()\n");

  m_mutex = new Mutex();
}

// Destructor
PPPPIOXferFactory::~PPPPIOXferFactory()
  throw()
{
  clearCache();
}


/***************************************
 *  This method is used to flush any cached items in the factoy
 ***************************************/
void PPPPIOXferFactory::clearCache()
{
  g_locations.destroyList();
}


// Get the location via the endpoint
EndPoint* PPPPIOXferFactory::getEndPoint( std::string& end_point  )
{ 
  PPPEndPoint *loc;
  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<PPPEndPoint*>(g_locations.getEntry(n));
    if ( end_point == loc->end_point ) {
      return loc;
    }
  }
#ifndef NDEBUG
  printf("Creating new location for %s\n", end_point.c_str() );
#endif

  loc = new PPPEndPoint(end_point);
        
  // This is a test case to make sure that a factory can modify the endpoint
  // string and system can handle it !!

  g_locations.insert( loc );
  return loc;
}

void PPPPIOXferFactory::releaseEndPoint( EndPoint* loc )
{
#ifndef NDEBUG
  printf("void PPPPIOXferFactory::releaseLocation( EndPoint* loc ), NOT YET IMPLEMENTED !!\n");
#endif

}


// This method is used to allocate a transfer compatible SMB
SmemServices* PPPPIOXferFactory::createSmemServices(EndPoint* loc )
{
#ifndef NDEBUG
  printf("^^^^  creating endpoint %s of size %d\n", loc->end_point.c_str(), loc->size );
#endif

  return new DataTransfer::PPPSmemServices((EndPoint*)loc);
}



/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* PPPPIOXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  return new PIOXferServices(source, target);
}




/***************************************
 *  This method is used to dynamically allocate
 *  an endpoint for an application running on "this"
 *  node.
 ***************************************/
static CPI::OS::int32_t mailbox=12;
std::string PPPPIOXferFactory::allocateEndpoint(CPI::OS::uint32_t *size )
{

  printf(" MAILBOX id = %d\n", mailbox );


  std::string ep;
  m_mutex->lock();
  RoseUint32Type target_id;

  rose_get_rio_base_device_id(&target_id);

  printf("rose_get_rio_base_device_id returned %d\n", target_id );


  rose_get_device_id( ROSE_BUS_RAPIDIO, &target_id);

  printf("rose_get_device_id returned %d\n", target_id );


  //  cpiAssert( getenv("TARGET_ID") );
  //  target_id = atoi(getenv("TARGET_ID"));

  if ( mailbox == -1 ) {
    const char* env = getenv("CPI_TRANSFER_MAILBOX");
    if( env && (env[0] != 0)) {

    }
    mailbox = atoi(env);
  }
  char tep[128];
  snprintf(tep,128,"cpi-ppp-pio://%d.%d:%d.%d.20",target_id,0,*size, target_id);
  ep = tep;
  mailbox++;
  m_mutex->unlock();
  return ep;
}


