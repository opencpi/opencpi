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



#define __STDC_FORMAT_MACROS
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <ezxml.h>
#include <OcpiOsAssert.h>
#include <OcpiOsMisc.h>
#include <OcpiUtilHash.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiUtilEzxml.h>
#include <OcpiUtilMisc.h>
#include <OcpiPValue.h>
#include <xfer_internal.h>
#include <DtTransferInternal.h>

namespace OX = OCPI::Util::EzXml;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OD = OCPI::Driver;

namespace DataTransfer {

uint16_t 
XferFactory::
getNextMailBox()
{
  static uint16_t mailbox=1;
  static bool mb_once=false;
  if ( ! mb_once ) {
    const char* env = getenv("OCPI_TRANSFER_MAILBOX");
    if( !env || (env[0] == 0)) {
      ocpiInfo("Set ""OCPI_TRANSFER_MAILBOX"" environment varible to control mailbox");
    }
    else {
      mailbox = atoi(env);
    }
    mb_once = true;
  }
  ocpiDebug("Transfer factory %p returning mailbox %u", this, mailbox);
  return mailbox++;
}



uint16_t 
XferFactory::
getMaxMailBox()
{
  static bool mmb_once=false;
  static uint16_t max_mb=MAX_SYSTEM_SMBS;
  if ( ! mmb_once ) {
    const char* env = getenv("OCPI_MAX_MAILBOX");
    if( !env || (env[0] == 0)) {
      ocpiInfo("Set ""OCPI_MAX_MAILBOX"" environment varible to control max mailbox");
    }
    else {
      max_mb = atoi(env);
    }
    mmb_once = true;
  }
  return max_mb;
}

// Default implementation for drivers that have no devices.
void
XferFactory::
allocateEndpoints(std::vector<std::string> &l) {
  l.push_back(allocateEndpoint(NULL, getNextMailBox(), getMaxMailBox()));
}

#if 0
// Allocate a set of endpoints, one per driver, for the caller
void XferFactoryManager::
allocateSupportedEndpoints(EndPoints &endpoints) {
  parent().configure();
  for (XferFactory *f = firstDriver(); f; f = f->nextDriver()) {
    std::string eps = f->allocateEndpoint(NULL, f->getNextMailBox(), f->getMaxMailBox());
    ocpiDebug("Allocating supported local endpoint: %s", eps.c_str());
    endpoints.insert(f->addEndPoint(eps.c_str(), true));
  }
}
#endif

// This method is used to retreive all of the available endpoints that have been registered
// in the system.  Note that some of the endpoints may not be finalized.
std::vector<std::string> 
XferFactoryManager::
getListOfSupportedEndpoints()
{
  parent().configure();
  std::vector<std::string> l;

  // Loop over transfer drivers
    for (XferFactory *factory = firstDriver(); factory;
	 factory = factory->nextDriver()) {
      try {
	factory->allocateEndpoints(l);
      }
      catch( OU::EmbeddedException & ex ) {
	ocpiBad("Could not allocate endpoint for %s, error = %s",
		factory->getProtocol(), ex.getAuxInfo() );
      }
      catch( ... ) {
	ocpiBad("Caught an unknown exception while allocating an endpoint from %s",
		factory->getProtocol() );
      }
    }
  return l;
}



ezxml_t 
XferFactory::
getNode( ezxml_t tn, const char* name )
{
  ezxml_t node = tn;
  while ( node ) {
    if ( node->name ) ocpiDebug("node %s", node->name );
    if ( node->name && (strcmp( node->name, name) == 0 ) ) {
      return node;
    }
    node = ezxml_next( node );
  }
  if ( tn ) {
    if(tn->child)if((node=getNode(tn->child,name)))return node;
    if(tn->sibling)if((node=getNode(tn->sibling,name)))return node;	 
  }
  return NULL;
}

// These defaults are pre-configuration
FactoryConfig::
FactoryConfig(uint32_t smbSize, uint32_t retryCount)
  : m_SMBSize(3*1024*1024), m_retryCount(128)
{
  if (smbSize)
    m_SMBSize = smbSize;
  if (retryCount)
    m_retryCount = retryCount;
}

// Parse and default from parent
void FactoryConfig::
parse(FactoryConfig *parent, ezxml_t x) {
  if (parent)
    *this = *parent;
  m_xml = x;
  if (x) {
    const char *err;
    // Note we are not writing defaults here because they are set
    // in the constructor, and they need to be set even when there is no xml
    if ((err = OX::checkAttrs(x, "SMBSize", "TxRetryCount", NULL)) ||
	(err = OX::getNumber(x, "SMBSize", &m_SMBSize, NULL, 0, false)) ||
	(err = OX::getNumber(x, "TxRetryCount", &m_retryCount, NULL, 0, false)))
      throw err; // FIXME configuration api error exception class
  }
}


// Configure this manager.  The drivers will be configured by the base class
void 
XferFactoryManager::
configure( ezxml_t x)
{
  if (!m_configured) {
    m_configured = true;
    parse(NULL, x);
    // Allow the environment to override config files here
    const char* env = getenv("OCPI_SMB_SIZE");
    if ( env && OX::getUNum(env, &m_SMBSize))
      throw "Invalid OCPI_SMB_SIZE value";

    // Now configure the drivers
    OD::Manager::configure(x);
    startup();
  }
}



// This method is used to retreive all of the available endpoints that have
// been registered in the system.  Note that some of the endpoints may not
// be finalized.
std::vector<std::string> 
XferFactoryManager::
getListOfSupportedProtocols()
{
  parent().configure();
  std::vector<std::string> l;
  for (XferFactory* d = firstDriver(); d; d = d->nextDriver())
    l.push_back(d->getProtocol());
  return l;
}



bool 
XferFactory::
supportsEndPoints(
		  std::string& end_point1,
		  std::string& end_point2 )
{

  ocpiDebug("In  XferFactory::supportsEndPoints, (%s) (%s)",
         end_point1.c_str(), end_point2.c_str() );

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

std::string XferFactoryManager::null;

// Find the Data transfer class
XferFactory* 
XferFactoryManager::
find( const char* ep1, const char* ep2 )
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


XferFactory* 
XferFactoryManager::
find( std::string& ep1, std::string& ep2 )
{
  parent().configure();
  OU::AutoMutex guard ( m_mutex, true );
  for (XferFactory* d = firstDriver(); d; d = d->nextDriver())
    if (d->supportsEndPoints(ep1, ep2))
      return d;
  return NULL;
}


const char *transfer = "transfer";
XferFactoryManager::
XferFactoryManager()
  : m_refCount(0),m_init(false),m_mutex(true),
    m_resources(0),m_configured(false)
{

}

XferFactoryManager::
~XferFactoryManager()
{
  shutdown();
}


void 
XferFactoryManager::
startup()
{
  OU::AutoMutex guard ( m_mutex, true );

  ocpiDebug("++++++++ In void XferFactoryManager::startup(), ref count = %d", m_refCount);

  m_refCount++;
}

// Shuts down the transfer sub-system
void XferFactoryManager::shutdown()
{
  OU::AutoMutex guard ( m_mutex, true );
  m_refCount--;
  if ( m_refCount == 0 ) {
    clearCache();
    m_resources.destroyList();
    m_init = false;
  }
}

// We need to create an endpoint for some other (hardware) container.
// The string should not include the last two fields.
  EndPoint& XferFactoryManager::
  allocateProxyEndPoint(const char *loc, uint32_t size) {
    XferFactory* tfactory = find(loc);
    if (!tfactory)
      throw OU::Error("No driver/factory for endpoint string: '%s'", loc);
    std::string complete;
    OU::formatString(complete, "%s;%" PRIu32 ".%u.%u",
		     loc, size, tfactory->getNextMailBox(), tfactory->getMaxMailBox());
    return *tfactory->getEndPoint(complete.c_str(), false, true);
  }

XferFactory::
XferFactory(const char *name)
  : OD::DriverType<XferFactoryManager,XferFactory>(name)
{
}

// Destructor
XferFactory::~XferFactory()
{
  this->lock();
  
  while (!m_endPoints.empty())
    delete *m_endPoints.begin(); // this will call back and remove even though we don't need it.
}

// This default implementation is just to parse generic properties,
// defaulting from our parent
void XferFactory::
configure(ezxml_t x) {
  // parse generic attributes and default from parent
  parse(&XferFactoryManager::getFactoryManager(), x);
  // base class does device config if present
  OD::Driver::configure(x); 
}

EndPoint* XferFactory::
addEndPoint(const char *end_point, bool local) {
  std::string eps(end_point);
  EndPoint *loc = createEndPoint(eps, local);
  loc->factory = this;
  ocpiAssert(m_endPoints.find(loc) == m_endPoints.end());
  m_endPoints.insert(loc);
  if (local) {
    if (m_locations.size() <= loc->mailbox)
      m_locations.resize(loc->mailbox + 1, NULL);
    m_locations[loc->mailbox] = loc;
  }
  ocpiInfo("Creating ep %p %s %u", loc, loc->end_point.c_str(), local);
  loc->refCount = 1;
  return loc;
}
void XferFactory::
removeEndPoint(EndPoint &ep) {
  if (ep.local) {
    m_locations[ep.mailbox] = NULL; // note this might already be done.
  }
  m_endPoints.erase(&ep);
}

// Get the location via the endpoint.  This is only called if the endpoint already
// matches the factory.
EndPoint* XferFactory::
getEndPoint(const char *end_point, bool local, bool cantExist)
{ 
  OCPI::Util::SelfAutoMutex guard (this); 
  for (EndPoints::iterator i = m_endPoints.begin(); i != m_endPoints.end(); i++)
    if (*i && (*i)->end_point == end_point)
      if (cantExist)
	throw OU::Error("Local explicit endpoint already exists: '%s'", end_point);
      else {
	(*i)->refCount++; // FIXME:: this likely happens too often and thus will leak.
	ocpiInfo("Incrementing refcount on ep %p to %u", *i, (*i)->refCount);
#if 0
	OCPI::OS::dumpStack(std::cerr);
#endif
	return *i;
      }
  return addEndPoint(end_point, local);
}

// The default is that the remote one doesn't matter to this allocation
std::string XferFactory::
allocateCompatibleEndpoint(const OCPI::Util::PValue*params,
			   const char *,
			   uint16_t mailBox, uint16_t maxMailBoxes) {
  return allocateEndpoint(params, mailBox, maxMailBoxes);
}

EndPoint* XferFactory::
addCompatibleLocalEndPoint(const char *remote, uint16_t mailBox, uint16_t maxMb)
{ 
  OCPI::Util::SelfAutoMutex guard (this); 
  // Find an unused slot that is different from the remote one
  // mailbox might be zero so this will find the first free slot in any case
  unsigned myMax = getMaxMailBox();
  if (maxMb && maxMb != myMax)
    throw OU::Error("Remote end point has different number of mailbox slots (%u vs. our %u)",  maxMb, myMax);
  unsigned n;
  if (!mailBox)
    n = getNextMailBox();
  else {
    for (n = 1; n < m_locations.size(); n++)
      if (n != mailBox && m_locations[n])
	break;
    if (n == MAX_SYSTEM_SMBS || n > myMax)
      throw OCPI::Util::Error("Mailboxes for endpoints for protocol %s are exhausted (all %u are used)",
			      getProtocol(), myMax);
    if (n >= m_locations.size())
      n = getNextMailBox();
  }
  std::string stringEndPoint = 
    remote ? allocateCompatibleEndpoint(NULL, remote, n, myMax) :
    allocateEndpoint(NULL, n, myMax);
  return addEndPoint(stringEndPoint.c_str(), true);
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
  EndPoint *src;
  EndPoint *dst;
  //  char* src;
  // char* dst;
  int rcount;
  ~template_list_item_()
  {
    //delete []src;
    //delete []dst;
    delete xf_template;
  }
};

typedef struct template_list_item_ TList_Item;

void XferFactoryManager::clearCache()
{


    // These are children of the factory and the factory will delete them !!
#if 0
  for (OS::int32_t i=0; i<get_nentries(&m_templatelist); i++) {
    TList_Item *item = static_cast<TList_Item*>(get_entry(&m_templatelist, i));
    delete item;
  }
  destroy_list(&m_templatelist);
  for ( OS::uint32_t n=0; n<m_resources.getElementCount(); n++ ) {
    SMBResources* res = static_cast<SMBResources*>(m_resources.getEntry(n));
    delete res;
  }
  m_resources.destroyList();
#endif


}

int
XferFactoryManager::get_template(EndPoint *src, EndPoint *dst, XferServices* &xfer_template)
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
    if (item->src == src && item->dst == dst) {
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
XferFactoryManager::add_template(EndPoint *src, EndPoint *dst, XferServices* xf_template)
{

  /* Local Variables */
  TList_Item *item;

  /* allocate memory for the template item */
  item = new TList_Item;

  /* Set the reference count */
  item->rcount = 1;

#if 0
  /* Initialize the TList_Item */
  if ( src.length() ) {
    item->src = new char[src.length()+1];
    strcpy(item->src, src.c_str() );
  }
  if ( dst.length() ) {
    item->dst = new char[dst.length()+1];
    strcpy(item->dst, dst.c_str() );
  }
#else
  item->src = src;
  item->dst = dst;
#endif
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

SMBResources* 
XferFactoryManager::
findResource(const char* ep)
{
  parent().configure();
  OU::AutoMutex guard ( m_mutex, true );
  for ( OS::uint32_t n=0; n<m_resources.getElementCount(); n++ ) {
    SMBResources* res = static_cast<SMBResources*>(m_resources.getEntry(n));
    if ( res->sMemServices->endpoint()->end_point == ep ) {
      return res;
    }
  }
  return NULL;
}


void 
XferFactoryManager::
deleteSMBResources( EndPoint* loc)
{
  OU::AutoMutex guard ( m_mutex, true );
  SMBResources* sr;
  sr = findResource( loc->end_point.c_str() );
  ocpiAssert( sr );
  m_resources.remove(sr);
  delete sr;
}


// create a transfer compatible SMB
SMBResources* 
XferFactoryManager::
createSMBResources(
		   EndPoint* loc)

{
  parent().configure();
  OU::AutoMutex guard ( m_mutex, true );

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
    throw OU::EmbeddedException( UNSUPPORTED_ENDPOINT, loc->end_point.c_str());
  }

  sr->sMemServices = loc->smem ? loc->smem : factory->getSmemServices( loc );
  if ( !sr->sMemServices ) {
    throw OU::EmbeddedException( UNSUPPORTED_ENDPOINT, loc->end_point.c_str());
  }

  sr->sMemResourceMgr = CreateResourceServices();
  sr->sMemResourceMgr->createLocal( loc->size );
  uint32_t offset;
  uint32_t size = sizeof(ContainerComms); // variable some day?
  if ( sr->sMemResourceMgr->alloc( size, 0, &offset) != 0 ) {
    throw OU::EmbeddedException(  NO_MORE_SMB, loc->end_point.c_str() );
  }

  sr->m_comms = static_cast<DataTransfer::ContainerComms*>
    (sr->sMemServices->map(offset,size));

  if ( ! sr->m_comms ) {
    throw OU::EmbeddedException(  SMB_MAP_ERROR, loc->end_point.c_str());
  }

  memset( sr->m_comms, 0, size);
  sr->m_comms->upAndRunning = UpAndRunningMarker;
  m_resources.insert(sr);
  loc->resources = sr;
  return sr;
}



// create a transfer compatible SMB
SMBResources* 
XferFactoryManager::
getSMBResources(
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
SMBResources* 
XferFactoryManager::
getSMBResources(
		std::string& ep)
{
  parent().configure();
  OU::AutoMutex guard ( m_mutex, true );

  if ( ep.length() == 0 ) {
    throw OU::EmbeddedException( UNSUPPORTED_ENDPOINT, "Null Endpoint");
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
    throw OU::EmbeddedException( UNSUPPORTED_ENDPOINT, ep.c_str());
  }
  EndPoint* loc = factory->getEndPoint( ep.c_str());
  sr->sMemServices = factory->getSmemServices(loc);
  sr->sMemResourceMgr = NULL;
  sr->sMemServices->attach( loc );
  loc->resources = sr;
  m_resources.insert(sr);
  return sr;
}


// This method makes a request to the
bool XferMailBox::makeRequest( SMBResources* source, SMBResources* target )
{

  ocpiDebug("In makerequest from %s to %s\n",
	    source->sMemServices->endpoint()->end_point.c_str(),
	    target->sMemServices->endpoint()->end_point.c_str() );


#ifdef MULTI_THREADED
  // Lock the mailbox
  if ( ! lockMailBox() ) {
    return false;
  }
#endif

  /* Attempt to get or make a transfer template */
  XferServices* ptemplate =
    XferFactoryManager::getFactoryManager().getService(
						       source->sMemServices->endpoint(),
						       target->sMemServices->endpoint() );
  if ( ! ptemplate ) {
    ocpiAssert(0);
  }

  OS::uint32_t offset = sizeof(ContainerComms::MailBox) * m_slot + sizeof(OS::uint32_t);

  ocpiDebug("In make request with offset = %d\n", offset );

  XferRequest * ptransfer = ptemplate->createXferRequest();

  // create the copy in the template
  ptransfer->copy (
		   offset + sizeof(ContainerComms::BasicReq),
		   offset + sizeof(ContainerComms::BasicReq),
		   sizeof(ContainerComms::MailBox) - sizeof(ContainerComms::BasicReq),
		   XferRequest::DataTransfer );

  ptransfer->copy (
		   offset,
		   offset,
		   sizeof(ContainerComms::BasicReq),
		   XferRequest::FlagTransfer );

  // Start the transfer
  ptransfer->post();

  while( ptransfer->getStatus() ) {
    OS::sleep(0);
  }

  delete ptransfer;

  return true;
}

// Statically available allocation routine
std::string 
XferFactoryManager::
allocateEndpoint(std::string& protocol, const OCPI::Util::PValue * props )
{
  parent().configure();
  OU::AutoMutex guard ( m_mutex, true );

  XferFactory* factory;
  if ( protocol.length() == 0 ) {

    // We will simply ask the first factory for an address
    factory = firstDriver();
  }
  else {
    std::string nuls;
    factory = find(protocol,nuls);
  }
  if ( ! factory ) {
    return NULL;
  }
  return factory->allocateEndpoint( props, factory-> getNextMailBox(), factory->getMaxMailBox());
}

XferServices* XferFactoryManager::getService(
                                             EndPoint *s_endpoint,
                                             EndPoint *t_endpoint)
{

  SMBResources* source_info;
  SMBResources* target_info;
  XferServices* pxfer;

  OU::AutoMutex guard ( m_mutex, true );

  /* Check to see if we already have a transfer template */
  if (get_template(s_endpoint, t_endpoint, pxfer)) {

    // Find the factory that supports the endpoints
    XferFactory* factory = find( s_endpoint->end_point,
                                                     t_endpoint->end_point);
    if ( factory == NULL ) {
      ocpiBad("Enpoint connection, %s to %s not supported", s_endpoint->end_point.c_str(),
             t_endpoint->end_point.c_str() );
      throw -1;
    }

    source_info = getSMBResources( s_endpoint );
    target_info = getSMBResources( t_endpoint );

    /* We couldn't find a template, create one */
    /* create the transfer template, source->target */
    ocpiDebug("new template from %s to %s", s_endpoint->end_point.c_str(), t_endpoint->end_point.c_str());
    pxfer = factory->getXferServices(source_info->sMemServices, target_info->sMemServices);

    /* Insert the template into the template list */
    if (add_template(s_endpoint, t_endpoint, pxfer)) {
      return NULL;
    }
  }

  return pxfer;

}

#if 0
XferServices* XferFactoryManager::getService( std::string& source_sname, std::string& target_sname)
{
  SMBResources* source_info;
  SMBResources* target_info;
  XferServices* pxfer;

  OU::AutoMutex guard ( m_mutex, true );

  /* Check to see if we already have a transfer template */
  if (get_template(source_sname.c_str(), target_sname.c_str(), pxfer)) {
    ocpiDebug("Getting new service !!");

    // Find the factory that supports the endpoints
    XferFactory* factory = find( source_sname, target_sname);

    source_info = getSMBResources( source_sname );
    target_info = getSMBResources( target_sname );

    /* We couldn't find a template, create one */
    /* create the transfer template, source->target */
    ocpiDebug("new template from %s to %s", source_sname.c_str(), target_sname.c_str());
    pxfer = factory->getXferServices(source_info->sMemServices, target_info->sMemServices);

    /* Insert the template into the template list */
    if (add_template(source_sname, target_sname, pxfer)) {
      return NULL;
    }
  }

  return pxfer;
}
#endif

  void Device::
  configure(ezxml_t x)
  {
    OD::Device::configure(x); // give the base class a chance to do generic configuration
    parse(&driverBase(), x);
  }

// Create a transfer request
XferRequest* XferRequest::copy (OCPI::OS::uint32_t srcoffs, 
				OCPI::OS::uint32_t dstoffs, 
				OCPI::OS::uint32_t nbytes, 
				XferRequest::Flags flags
				)
{
  OCPI::OS::int32_t retVal = 0;
  OCPI::OS::int32_t newflags = 0;
  if (flags & XferRequest::DataTransfer) newflags |= XFER_FIRST;
  if (flags & XferRequest::FlagTransfer) newflags |= XFER_LAST;
  if ( m_thandle == NULL ) {
    retVal = xfer_copy (m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &m_thandle);
    if (retVal){
      return NULL;
    }
  }
  else {
    XF_transfer handle;
    retVal = xfer_copy (m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &handle);
    if (retVal){
      return NULL;
    }
    XF_transfer handles[3];
    handles[0] = handle;
    handles[1] = m_thandle;
    handles[2] = 0;
    retVal = xfer_group ( handles, 0, &m_thandle);
    if (retVal) {
      return NULL;
    }
    xfer_release(handles[0], 0);
    xfer_release(handles[1], 0);
  }
  return this;
}


// Group data transfer requests
XferRequest & XferRequest::group (XferRequest* lhs )
{
  XF_transfer handles[3];
  handles[0] = lhs->m_thandle;
  handles[1] = m_thandle;
  handles[2] = 0;
  xfer_group ( handles, 0, &m_thandle);
  return *this;
}

void XferRequest::modify( OCPI::OS::uint32_t new_offsets[], OCPI::OS::uint32_t old_offsets[] )
{
  int n=0;
  while ( new_offsets[n] ) {
    xfer_modify( m_thandle, &new_offsets[n], &old_offsets[n] );
    n++;
  }
}
void XferRequest::action_transfer(PIO_transfer pio_transfer) {
  xfer_pio_action_transfer(pio_transfer);
}
void XferRequest::start_pio(PIO_transfer pio_transfer) {
  for (PIO_transfer transfer = pio_transfer; transfer; transfer = transfer->next)
    action_transfer(transfer);
}
void XferRequest::post() {
  
  struct xf_transfer_ *xf_transfer = (struct xf_transfer_ *)m_thandle;
  
  /* Process the first transfers */

  if (xf_transfer->first_pio_transfer) {
    start_pio(xf_transfer->first_pio_transfer);
  }

  /* Get the type of transfer */
  if (xf_transfer->pio_transfer) {
    /* Start the pio transfer */
    start_pio(xf_transfer->pio_transfer);
  }

  /* Process the last transfers */
  if (xf_transfer->last_pio_transfer) {

    /* Start the last pio transfer */
    start_pio(xf_transfer->last_pio_transfer);
  }
}

XferRequest::CompletionStatus XferRequest::
getStatus() {
  return xfer_get_status (m_thandle) == 0 ? CompleteSuccess : Pending;
}

XferRequest::XferRequest(XF_template temp) : m_thandle(NULL), m_xftemplate(temp) {
}
XferRequest::~XferRequest() {
  if (m_thandle)
    {
      (void)xfer_release (m_thandle, 0);
    }
}
}
