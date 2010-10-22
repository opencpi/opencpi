
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
 *   This file contains the implementation for the base class for SMB transfers.
 *
 * Revision History:

   06/24/09 - John Miller
   Removed printf's from destructor.

   02/15/09 - John Miller
   Added auto mutex protection.
   Removed exception monitor.

   07/20/04 - John Miller
   Initial version.

 * 
 *
 */



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <OcpiOsAssert.h>
#include <OcpiOsMisc.h>
#include <OcpiUtilHash.h>
#include <OcpiUtilAutoMutex.h>
#include <DtTransferInternal.h>

using namespace DataTransfer;
using namespace OCPI::Util;
using namespace OCPI::OS;


XferFactoryManager& XferFactoryManager::getFactoryManager()
{
  static XferFactoryManager * fm = NULL;
  if ( !fm ) {
    fm = new XferFactoryManager;
  }
  return *fm;
}

// Register the Data transfer class
void XferFactoryManager::registerFactory( XferFactory* dt )
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  insert_to_list( &m_registeredTransfers, dt, 64,8);
}

// Register the Data transfer class
void XferFactoryManager::unregisterFactory( XferFactory* dt )
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  remove_from_list( &m_registeredTransfers, dt);
}


// This method is used to retreive all of the available endpoints that have been registered
// in the system.  Note that some of the endpoints may not be finalized. 
std::vector<std::string> XferFactoryManager::getListOfSupportedEndpoints()
{
  OCPI::OS::uint32_t default_ep_size = 10*1024*1024;


  XferFactory* factory;
  std::vector<std::string> l;
  for (int i=0; i < get_nentries(&m_registeredTransfers); i++) {
    factory = static_cast<XferFactory*>(get_entry(&m_registeredTransfers, i));
    l.push_back( factory->allocateEndpoint( &default_ep_size ) );
  }
  return l;
}



bool XferFactory::supportsEndPoints(
                                    std::string& end_point1, 
                                    std::string& end_point2 )
{
#ifndef NDEBUG
  printf("In  XferFactory::supportsEndPoints, (%s) (%s)\n",
         end_point1.c_str(), end_point2.c_str() );
#endif

  unsigned int len = strlen( getProtocol() );
  if ( end_point1.length() && end_point2.length() ) {

    if ( (strncmp( end_point1.c_str(), getProtocol(), strlen(getProtocol())) == 0 ) &&
         strncmp( end_point2.c_str(), getProtocol(), strlen(getProtocol())) == 0 ) {
      if ( (end_point1[len] != ':') ||  (end_point2[len] != ':') ) {
        return false;
      }
      return true;
    }
  }
  else if ( end_point1.length() ) {
    if ( (strncmp( end_point1.c_str(), getProtocol(), strlen(getProtocol())) == 0) ) {
      if ( (len<end_point1.length()) && (end_point1[len] != ':')  ) {
        return false;
      }
      return true;
    }
  }
  else if ( end_point2.length() ) {
    if ( (strncmp( end_point2.c_str(), getProtocol(), strlen(getProtocol())) == 0 ) ) {
      if ( (len<end_point2.length()) && (end_point2[len] != ':')  ) {
        return false;
      }
      return true;
    }
  }

  return false;
}



// Register the Data transfer class
XferFactory* XferFactoryManager::find( const char* ep1, const char* ep2 )
{
  std::string sep1, sep2;
  if ( ep1 && ep2 ) {
    sep1 = ep1;
    sep2 = ep2;
  }
  else if ( ep1 && !ep2 ) {
    sep1 = ep1;
  }
  else {
    sep2 = ep2;
  }
  return XferFactoryManager::find( sep1 , sep2);
}

// Register the Data transfer class
XferFactory* XferFactoryManager::find( std::string& ep1, std::string& ep2 )
{
  XferFactory* factory;
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  for (int i=0; i < get_nentries(&m_registeredTransfers); i++) {
    factory = static_cast<XferFactory*>(get_entry(&m_registeredTransfers, i));
    if ( factory->supportsEndPoints( ep1, ep2)  ) {
      return factory;
    }
  }

  return NULL;
}


XferFactoryManager::XferFactoryManager()
  :OCPI::Util::DriverManager("DataTransfer"),
   m_refCount(0),m_init(false),m_mutex(true),
  m_resources(0)
{
  startup();
}


XferFactoryManager::~XferFactoryManager()
{
  shutdown();
}


void XferFactoryManager::startup()
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  // We will find any drivers that were statically registered and add them
  // to our list
  std::vector<OCPI::Util::Driver*> & list = getDrivers();
  std::vector<OCPI::Util::Driver*>::iterator it;
  for (it=list.begin(); it!=list.end(); it++ ) {
    registerFactory( static_cast<XferFactory*>( (*it) ) );
  }

  m_refCount++;

#ifndef NDEBUG
  printf("++++++++ In void XferFactoryManager::startup(), ref count = %d\n", m_refCount );
#endif

}

// Shuts down the transer sub-system
void XferFactoryManager::shutdown()
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  m_refCount--;
  if ( m_refCount == 0 ) {
    clearCache();
    destroy_list(&m_registeredTransfers);
    m_resources.destroyList();
    m_init = false;
  }
}


// Default constructor
XferFactory::XferFactory(const char* name )
  throw()
  : OCPI::Util::Driver( "DataTransfer", name, true )
{
}

// Destructor
XferFactory::~XferFactory()
  throw()
{
  // Factory life cycles are managed by the factory Manager, nothing to do here.
}



/******
 *  TEMPLATE MANAGEMENT
 ******/

// Constant declarations 
#define TLIST_INITIAL_SIZE  64
#define TLIST_INCREMENT     8

// Transfer Template List data entity 
struct template_list_item_
{
  XferServices* xf_template;
  char* src;
  char* dst;
  int rcount;
  ~template_list_item_()
  {
    delete []src;
    delete []dst;
    delete xf_template;
  }
};

typedef struct template_list_item_ TList_Item;

void XferFactoryManager::clearCache()
{
  for (OCPI::OS::int32_t i=0; i<get_nentries(&m_templatelist); i++) {
    TList_Item *item = static_cast<TList_Item*>(get_entry(&m_templatelist, i));
    delete item;
  }
  destroy_list(&m_templatelist);

  for ( OCPI::OS::uint32_t n=0; n<m_resources.getElementCount(); n++ ) {
    SMBResources* res = static_cast<SMBResources*>(m_resources.getEntry(n));
    delete res;
  }
  m_resources.destroyList();
  
}

int
XferFactoryManager::get_template(const char *src, const char *dst, XferServices* &xfer_template)
{
  /* Check to see if the list has any items */
  if (get_nentries(&m_templatelist) == 0) {
    /* There are no entries in the list */
    return 1;
  }

  /* Check for a match in the list */
  for (int i=0; i < get_nentries(&m_templatelist); i++) {
    TList_Item *item = (TList_Item *)get_entry(&m_templatelist, i);
    /* Check for a complete match */
    if ((!(strcmp((const char *)item->src, src))) &&
        (!(strcmp((const char *)item->dst, dst)))) {
      /* We have a match, return the template */
      xfer_template = item->xf_template;
      /* Increment the ref count */
      item->rcount++;
      return 0;
    }
  }

  /* Return to caller */
  return 1;
}

int
XferFactoryManager::add_template(std::string& src, std::string& dst, XferServices* xf_template)
{

  /* Local Variables */
  TList_Item *item;

  /* allocate memory for the template item */
  item = new TList_Item;
    
  /* Set the reference count */
  item->rcount = 1;

  /* Initialize the TList_Item */
  if ( src.length() ) {
    item->src = new char[src.length()+1];
    strcpy(item->src, src.c_str() );
  }
  if ( dst.length() ) {
    item->dst = new char[dst.length()+1];
    strcpy(item->dst, dst.c_str() );
  }

  item->xf_template = xf_template;

  /* Add the template to the list */
  if (insert_to_list(&m_templatelist,
                     (void *)item,
                     TLIST_INITIAL_SIZE,
                     TLIST_INCREMENT)) {
    return 1;
  }

  /* Return to caller */
  return 0;
}

SMBResources* XferFactoryManager::findResource(const char* ep)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  for ( OCPI::OS::uint32_t n=0; n<m_resources.getElementCount(); n++ ) {
    SMBResources* res = static_cast<SMBResources*>(m_resources.getEntry(n));
    if ( res->sMemServices->getEndPoint()->end_point == ep ) {
      return res;
    }
  }
  return NULL;
}


void XferFactoryManager::deleteSMBResources(
                                            EndPoint* loc)
{

  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  SMBResources* sr;        
  sr = findResource( loc->end_point.c_str() );  
  ocpiAssert( sr );
  m_resources.remove(sr);  
  delete sr;
}

// create a transfer compatible SMB
SMBResources* XferFactoryManager::createSMBResources( 
                                                     EndPoint* loc)

{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  SMBResources* sr;        
  sr = findResource( loc->end_point.c_str() );
  if ( sr ) {
    return sr;
  }
  else {
    ocpiAssert( loc->size );
    sr = new SMBResources;
  }

  std::string nuls;
  XferFactory* factory = find( loc->end_point, nuls );
  if ( ! factory ) {
    throw OCPI::Util::EmbeddedException( UNSUPPORTED_ENDPOINT, loc->end_point.c_str());
  }

  sr->sMemServices = factory->createSmemServices( loc );
  if ( !sr->sMemServices ) {
    throw OCPI::Util::EmbeddedException( UNSUPPORTED_ENDPOINT, loc->end_point.c_str());
  }
     
  sr->sMemResourceMgr = CreateResourceServices();
  sr->sMemResourceMgr->createLocal( loc->size );
  OCPI::OS::uint64_t offset;
  if ( sr->sMemResourceMgr->alloc( sizeof(ContainerComms), 
                                   0, &offset) != 0 ) {
    throw OCPI::Util::EmbeddedException(  NO_MORE_SMB, loc->end_point.c_str() );
  }

  sr->m_comms = static_cast<DataTransfer::ContainerComms*>
    (sr->sMemServices->map(offset,sizeof(ContainerComms)));

  if ( ! sr->m_comms ) {
    throw OCPI::Util::EmbeddedException(  SMB_MAP_ERROR, loc->end_point.c_str());
  }

  memset( sr->m_comms, 0, sizeof(ContainerComms) );
  sr->m_comms->upAndRunning = UpAndRunningMarker;
  m_resources.insert(sr);
  return sr;
}



// create a transfer compatible SMB
SMBResources* XferFactoryManager::getSMBResources(        
                                                  EndPoint* ep )
{
  ocpiAssert( ep );
  if ( ep->resources ) {
    return ep->resources;
  }
  else {
    SMBResources* res = getSMBResources( ep->end_point );
    ep->resources = res;
    return res;
  }
}



// create a transfer compatible SMB
SMBResources* XferFactoryManager::getSMBResources(        
                                                  std::string& ep)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  if ( ep.length() == 0 ) {
    throw OCPI::Util::EmbeddedException( UNSUPPORTED_ENDPOINT, "Null Endpoint");
  }
  SMBResources* sr = findResource( ep.c_str() );
  if ( sr ) {
    return sr;
  }
  else {
    sr = new SMBResources;
  }

  // Find the factory that knows how to create the shared memory block for 
  // this address
  std::string nuls;
  XferFactory* factory = find( ep, nuls );
  if ( ! factory ) {
    throw OCPI::Util::EmbeddedException( UNSUPPORTED_ENDPOINT, ep.c_str());
  }
  EndPoint* loc = factory->getEndPoint( ep );
  sr->sMemServices = factory->createSmemServices(loc);
  sr->sMemResourceMgr = NULL;
  sr->sMemServices->attach( loc );
  m_resources.insert(sr);
  return sr;
}


// This method makes a request to the 
bool XferMailBox::makeRequest( SMBResources* source, SMBResources* target )
{

#ifndef NDEBUG
  printf("In makerequest from %s to %s\n", 
         source->sMemServices->getEndPoint()->end_point.c_str(), 
         target->sMemServices->getEndPoint()->end_point.c_str() );
#endif

#ifdef MULTI_THREADED
  // Lock the mailbox
  if ( ! lockMailBox() ) {
    return false;
  }
#endif

  /* Attempt to get or make a transfer template */
  XferServices* ptemplate = 
    XferFactoryManager::getFactoryManager().getService( 
                                   source->sMemServices->getEndPoint(), 
                                   target->sMemServices->getEndPoint() );
  if ( ! ptemplate ) {
    ocpiAssert(0);
  }

  OCPI::OS::uint32_t offset = sizeof(ContainerComms::MailBox) * m_slot + sizeof(OCPI::OS::uint32_t);

#ifndef NDEBUG
  printf("In make request with offset = %d\n", offset );
#endif

  // create the copy in the template
  XferRequest* ptransfer_a =
    ptemplate->copy (
                     offset + sizeof(ContainerComms::BasicReq),
                     offset + sizeof(ContainerComms::BasicReq),
                     sizeof(ContainerComms::MailBox) - sizeof(ContainerComms::BasicReq),
                     XferRequest::FirstTransfer,0 );

  XferRequest *ptransfer_b =
    ptemplate->copy (
                     offset,
                     offset,
                     sizeof(ContainerComms::BasicReq),
                     XferRequest::LastTransfer, ptransfer_a );

  // Create a list
  XferRequest *grps[3];

  grps[0] = ptransfer_a;
  grps[1] = ptransfer_b;
  grps[2] = 0;

  // Group the transfer
  XferRequest *ptransfer_c = ptemplate->group (grps);

  // Start the transfer
  ptransfer_c->start();

  while( ptransfer_c->getStatus() ) {
    OCPI::OS::sleep(0);
  }

  return true;
}

// Statically available allocation routine
std::string XferFactoryManager::allocateEndpoint(std::string& protocol, OCPI::OS::uint32_t *size)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
                
  XferFactory* factory;
  if ( protocol.length() == 0 ) {
                
    // We will simply ask the first factory for an address
    factory = static_cast<XferFactory*>(get_entry(&m_registeredTransfers, 0));
  }
  else {
    std::string nuls;
    factory = find(protocol,nuls);
  }
        
  if ( ! factory ) {
    return NULL;
  }

  return factory->allocateEndpoint( size);
}


XferServices* XferFactoryManager::getService(
                                             EndPoint *s_endpoint,                        
                                             EndPoint *t_endpoint)
{

  SMBResources* source_info;
  SMBResources* target_info;
  XferServices* pxfer;

  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  /* Check to see if we already have a transfer template */
  if (get_template(s_endpoint->end_point.c_str(), t_endpoint->end_point.c_str(), pxfer)) {

    // Find the factory that supports the endpoints
    XferFactory* factory = find( s_endpoint->end_point, 
                                                     t_endpoint->end_point);
    if ( factory == NULL ) {
      printf("Enpoint connection, %s to %s not supported\n", s_endpoint->end_point.c_str(),
             t_endpoint->end_point.c_str() );
      throw -1;
    }

    source_info = getSMBResources( s_endpoint );
    target_info = getSMBResources( t_endpoint );

    /* We couldn't find a template, create one */
    /* create the transfer template, source->target */
    pxfer = factory->getXferServices(source_info->sMemServices, target_info->sMemServices);

    /* Insert the template into the template list */
    if (add_template(s_endpoint->end_point, t_endpoint->end_point, pxfer)) {
      return NULL;
    }
  }

  return pxfer;

}

XferServices* XferFactoryManager::getService( std::string& source_sname, std::string& target_sname)
{
  SMBResources* source_info;
  SMBResources* target_info;
  XferServices* pxfer;

  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  /* Check to see if we already have a transfer template */
  if (get_template(source_sname.c_str(), target_sname.c_str(), pxfer)) {

#ifndef NDEBUG
    printf("Getting new service !!\n");
#endif

    // Find the factory that supports the endpoints
    XferFactory* factory = find( source_sname, target_sname);

    source_info = getSMBResources( source_sname );
    target_info = getSMBResources( target_sname );

    /* We couldn't find a template, create one */
    /* create the transfer template, source->target */
    pxfer = factory->getXferServices(source_info->sMemServices, target_info->sMemServices);

    /* Insert the template into the template list */
    if (add_template(source_sname, target_sname, pxfer)) {
      return NULL;
    }
  }

  return pxfer;
}

