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
 *   This file contains the implementation for the base class for SMB transfers.
 *
 *  John Miller - 6/15/09
 *  Fixed Coverity issues
 *
 *  John Miller -  7/20/04
 *  Initial version
 *
 */

#include <DtSharedMemoryInternal.h>
#include <DtPioXfer.h>
#include <xfer_if.h>
#include <CpiList.h>
#include <CpiUtilHash.h>
#include <CpiOsMisc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <CpiOsAssert.h>
#include <CpiUtilAutoMutex.h>
#include <DtExceptions.h>

using namespace DataTransfer;
using namespace CPI::Util;
using namespace CPI::OS;

CPI::Util::VList PIOXferServices::m_map(0);

// Used to register with the data transfer system;
PIOXferFactory *g_pioFactory = new PIOXferFactory;


PIOXferFactory::PIOXferFactory()
  throw ()
  : XferFactory("Host SMB Programmed I/O transfer driver")
{
  printf("In PIOXferFactory::PIOXferFactory()\n");

  // Empty
}

// Destructor
PIOXferFactory::~PIOXferFactory()
  throw ()
{
  clearCache();
}


/***************************************
 *  This method is used to flush any cached items in the factoy
 ***************************************/
void PIOXferFactory::clearCache()
{
  GppEndPoint *loc;
  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<GppEndPoint*>(g_locations.getEntry(n));
    delete loc;
  }
  g_locations.destroyList();
}


// Get the location via the endpoint
EndPoint* PIOXferFactory::getEndPoint( std::string& end_point  )
{ 
  CPI::Util::AutoMutex guard ( m_mutex, true ); 

  GppEndPoint *loc;
  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<GppEndPoint*>(g_locations.getEntry(n));
    if ( end_point == loc->end_point ) {
      return loc;
    }
  }

  loc = new GppEndPoint(end_point);
        
  // This is a test case to make sure that a factory can modify the endpoint
  // string and system can handle it !!

  g_locations.insert( loc );
  return loc;
}

void PIOXferFactory::releaseEndPoint( EndPoint* )
{}


// This method is used to allocate a transfer compatible SMB
SmemServices* PIOXferFactory::createSmemServices(EndPoint* loc )
{
  return CreateSmemServices(loc);
}


/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* PIOXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  return new PIOXferServices(source, target);
}


/***************************************
 *  This method is used to dynamically allocate
 *  an endpoint for an application running on "this"
 *  node.
 ***************************************/
static CPI::OS::int32_t mailbox=1;
static CPI::OS::int32_t pid;
static CPI::OS::int32_t smb_count=0;
std::string PIOXferFactory::allocateEndpoint(CPI::OS::uint32_t *size )
{
  CPI::Util::AutoMutex guard ( m_mutex, true ); 
  std::string ep;

#ifdef USE_ENV_FOR_MAILBOX
  if ( mailbox == -1 ) {
    const char* env = getenv("CPI_TRANSFER_MAILBOX");
    if( !env || (env[0] == 0)) {
      CPI_THROWNULL( DataTransferEx(PROPERTY_NOT_SET, "CPI_TRANSFER_MAILBOX" ) ) ;
    }
    mailbox = atoi(env);
    pid++;
  }
#endif


  pid = getpid();
  char tep[128];
  snprintf(tep,128,"cpi-smb-pio://pioXfer%d%d:%d.%d.20",pid,smb_count++,*size, mailbox);
  ep = tep;
  mailbox++;

  return ep;
}




// Sets smem location data based upon the specified endpoint
CPI::OS::int32_t GppEndPoint::setEndpoint( std::string& ep )
{
  EndPoint::setEndpoint(ep);

  CPI::OS::uint32_t n,i=0;
  CPI::OS::int32_t start=0;
  char sname[80];
  for ( n=0; n<ep.length(); n++ ) {
    if ( (start<2) && (ep[n] == '/') ) {
      start++;
    }
    else if ( (start == 2) && (ep[n] == ':') ) {
      break;
    }
    else if ( start == 2 ) {
      sname[i++] = ep[n];
    }
  }

  sname[i] = 0;
  m_smb_name = sname;

  return 0;
}

GppEndPoint::~GppEndPoint()
{
}



void PIOXferRequest::init (Creator cr, 
                           Flags flags, 
                           CPI::OS::uint32_t srcoffs, 
                           Shape *psrcshape, 
                           CPI::OS::uint32_t dstoffs, 
                           Shape *pdstshape, 
                           CPI::OS::uint32_t length)
{
  m_creator = cr;
  m_flags = flags;
  m_srcoffset = srcoffs;
  m_dstoffset = dstoffs;
  m_length = length;
  m_thandle = 0;
  memset (&m_srcshape, 0, sizeof (m_srcshape));
  if (psrcshape)
    {
      memcpy (&m_srcshape, psrcshape, sizeof (m_srcshape));
    }
  memset (&m_dstshape, 0, sizeof (m_dstshape));
  if (pdstshape)
    {
      memcpy (&m_dstshape, pdstshape, sizeof (m_dstshape));
    }
}


void PIOXferRequest::modify( CPI::OS::uint32_t new_offsets[], CPI::OS::uint32_t old_offsets[] )
{
  int n=0;
  while ( new_offsets[n] ) {
    xfer_modify( m_thandle, &new_offsets[n], &old_offsets[n] );
    n++;
  }
}


// PIOXferRequest destructor implementation
PIOXferRequest::~PIOXferRequest ()
{
  // remove self from the map and release xfer handle.
  PIOXferServices::remove (this);
  if (m_thandle)
    {
      (void)xfer_release (m_thandle, 0);
    }
}


// Create tranfer services template
void PIOXferServices::createTemplate (SmemServices* p1, SmemServices* p2)
{

  m_txRequest = NULL;
  m_sourceSmb = p1;
  m_targetSmb = p2;

  // Invoke original code, saving the returned template reference.
  xfer_create (p1, p2, 0, &m_xftemplate);
}





// Create a transfer request
XferRequest* PIOXferServices::copy (CPI::OS::uint32_t srcoffs, 
                                    CPI::OS::uint32_t dstoffs, 
                                    CPI::OS::uint32_t nbytes, 
                                    XferRequest::Flags flags,
                                    XferRequest*
                                    )
{
  // Create a transfer request instance and save in list
  PIOXferRequest* pXferReq = new PIOXferRequest ();
  pXferReq->init (XferRequest::Copy, flags, srcoffs, 0, dstoffs, 0, nbytes);
  add (pXferReq);

  // Begin exception block
  CPI::OS::int32_t retVal = 0;
  CPI_TRY
    {
      // map flags
      CPI::OS::int32_t newflags = 0;
      if (flags & XferRequest::FirstTransfer) newflags |= XFER_FIRST;
      if (flags & XferRequest::LastTransfer) newflags |= XFER_LAST;

      // Invoke original code.
      retVal = xfer_copy (m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &pXferReq->getHandle());
      if (retVal)
        {
          CPI_RETHROW_TO_NEXT_LEVEL(LEVEL1);
        }
    }
  CPI_CATCH_LEVEL( m_exceptionMonitor, LEVEL1 )
    {
      remove (pXferReq);
      delete pXferReq;
      pXferReq = 0;
    }
  return pXferReq;

}


// Create a 2-dimensional transfer request
XferRequest* PIOXferServices::copy2D (CPI::OS::uint32_t srcoffs, Shape* psrc, 
                                      CPI::OS::uint32_t dstoffs, Shape* pdst, XferRequest*)
{
  // Create a transfer request instance and save in list
  PIOXferRequest* pXferReq = new PIOXferRequest ();
  pXferReq->init (XferRequest::Copy2D, (XferRequest::Flags)0, srcoffs, psrc, dstoffs, pdst, 0);
  add (pXferReq);

  // Begin exception block
  CPI::OS::int32_t retVal = 0;
  CPI_TRY
    {
      // Invoke original code.
      // We simple cast "XferServices::Shape" to "EP_shape" since they must have the
      // exact same definitions. We don't specify any flags (they weren't used in the original).
      //                        retVal = xfer_copy_2d (m_xftemplate, srcoffs, (Shape*)psrc, dstoffs, (Shape*)pdst, 0, &pXferReq->m_thandle);
      if (retVal)
        {
          CPI_RETHROW_TO_NEXT_LEVEL(LEVEL1);
        }
    }
  CPI_CATCH_LEVEL( m_exceptionMonitor, LEVEL1 )
    {
      remove (pXferReq);
      delete pXferReq;
      pXferReq = 0;
    }
  return pXferReq;
}


// Group data transfer requests
XferRequest* PIOXferServices::group (XferRequest* preqs[])
{
  // Create a transfer request instance and save in list
  PIOXferRequest* pXferReq = new PIOXferRequest ();
  pXferReq->init (XferRequest::Group, (XferRequest::Flags)0, 0, 0, 0, 0, 0);
  add (pXferReq);

  // Begin exception handler
  CPI::OS::int32_t retVal = 0;
  XF_transfer* handles = 0;
  CPI_TRY
    {
      // Make a list of existing XF_transfer from the XferRequest* [] argument.
      int numHandles = 0;
      while (preqs[numHandles]) { numHandles++;}
      handles = new XF_transfer [numHandles + 1] ;
      for (int i = 0; i < numHandles; i++)
        {
          handles[i] = ((PIOXferRequest*)preqs[i])->getHandle();
        }
      handles[numHandles] = 0;

      // Invoke original code.
      retVal = xfer_group (handles, 0, &pXferReq->getHandle());
      if (retVal)
        {
          CPI_RETHROW_TO_NEXT_LEVEL(LEVEL1);
        }
    }
  CPI_CATCH_LEVEL( m_exceptionMonitor, LEVEL1 )
    {
      remove (pXferReq);
      delete pXferReq;
      pXferReq = 0;
      delete handles;
    }
  delete[] handles;
  return pXferReq;
}

// Release a transfer request
void PIOXferServices::release (XferRequest* preq)
{
  // Delete of request insures list removal.
  delete preq;
}


// remove all transfer request instances from the list for "this"
void PIOXferServices::releaseAll ()
{
  for ( CPI::OS::uint32_t n=0; n<m_map.size(); n++ ) {
    PIOXferRequest* req = static_cast<PIOXferRequest*>(m_map[n]);
    delete req;
  }
}


// remove a specified transfer request instance from the list
void PIOXferServices::remove (PIOXferRequest* pXferReq )
{
  m_map.remove( pXferReq );
}


// Destructor
PIOXferServices::~PIOXferServices ()
{
  // Release all transfer requests
  releaseAll ();

  // Invoke destroy without flags.
  xfer_destroy (m_xftemplate, 0);

}






