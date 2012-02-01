
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
#include <ezxml.h>
#include <OcpiOsAssert.h>
#include <OcpiOsMisc.h>
#include <OcpiUtilHash.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiUtilEzxml.h>
#include <OcpiPValue.h>
#include <DtTransferInternal.h>

namespace OX = OCPI::Util::EzXml;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OD = OCPI::Driver;

namespace DataTransfer {
uint32_t 
XferFactory::
getNextMailBox()
{
  static OS::int32_t mailbox=1;
  static bool mb_once=false;
  if ( ! mb_once ) {
    const char* env = getenv("OCPI_TRANSFER_MAILBOX");
    if( !env || (env[0] == 0)) {
      printf("Set ""OCPI_TRANSFER_MAILBOX"" environment varible to control mailbox\n");
    }
    else {
      mailbox = atoi(env);
    }
    mb_once = true;
  }
  return mailbox++;
}



uint32_t 
XferFactory::
getMaxMailBox()
{
  static bool mmb_once=false;
  static int max_mb=MAX_SYSTEM_SMBS;
  if ( ! mmb_once ) {
    const char* env = getenv("OCPI_MAX_MAILBOX");
    if( !env || (env[0] == 0)) {
      printf("Set ""OCPI_MAX_MAILBOX"" environment varible to control max mailbox\n");
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
	fprintf(stderr, "Could not allocate endpoint for %s, error = %s\n",
		factory->getProtocol(), ex.getAuxInfo() );
      }
      catch( ... ) {
	fprintf(stderr, "Caught an unknown exception while allocating an endpoint from %s\n",
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
    if ( node->name ) printf("node %s\n", node->name );
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

#ifndef NDEBUG
  printf("++++++++ In void XferFactoryManager::startup(), ref count = %d\n", m_refCount );
#endif

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

XferFactory::
XferFactory(const char *name)
  : OD::DriverType<XferFactoryManager,XferFactory>(name)
{
}

// Destructor
XferFactory::~XferFactory()
{
  this->lock();
  for (unsigned n = 0; n < m_locations.size(); n++)
    if (m_locations[n])
      delete m_locations[n];
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

// Get the location via the endpoint.  This is only called if the endpoint already
// matches the factory.
EndPoint* XferFactory::getEndPoint( std::string& end_point, bool local )
{ 
  OCPI::Util::SelfAutoMutex guard (this); 
  for (EndPoints::iterator i = m_locations.begin(); i != m_locations.end(); i++)
    if (*i && end_point == (*i)->end_point)
      return *i;
  EndPoint *loc = createEndPoint(end_point, local);
  m_locations.resize(loc->mailbox + 1, NULL);
  m_locations[loc->mailbox] = loc;
  return loc;
}

EndPoint* XferFactory::newCompatibleEndPoint(const char *remote_endpoint)
{ 
  OCPI::Util::SelfAutoMutex guard (this); 
  char *cs = strdup(remote_endpoint);
  uint32_t mailBox, maxMb, size;
  EndPoint::getResourceValuesFromString(remote_endpoint, cs, &mailBox, &maxMb, &size);
  free(cs);
  unsigned n;
  for (n = 1; n < MAX_SYSTEM_SMBS; n++)
    if (n != mailBox && (n >= m_locations.size() || !m_locations[n]))
      break;
  if (n == MAX_SYSTEM_SMBS)
    throw OCPI::Util::Error("Mailboxes for endpoints for protocol %s are exhausted (all %d are used)",
			    getProtocol(), MAX_SYSTEM_SMBS);
  std::string sep = allocateEndpoint(NULL, n, maxMb);
  EndPoint *loc = createEndPoint(sep, true);
  m_locations.resize(loc->mailbox + 1, NULL);
  m_locations[loc->mailbox] = loc;
  return loc;
}

// static

bool XferFactoryManager::
canSupport(EndPoint &local_ep, const char *remote_endpoint) {
  char *protocol = strdup(remote_endpoint);
  char *cs = strdup(remote_endpoint);
  uint32_t mailBox, maxMb, size;
  EndPoint::getProtocolFromString(remote_endpoint, protocol);
  EndPoint::getResourceValuesFromString(remote_endpoint, cs, &mailBox, &maxMb, &size);
  bool ret = 
    local_ep.protocol == protocol &&
    maxMb == local_ep.maxCount && mailBox != local_ep.mailbox;
  free(protocol);
  free(cs);
  return ret;
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
  uint64_t offset;
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
  EndPoint* loc = factory->getEndPoint( ep );
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

#ifndef NDEBUG
  printf("In makerequest from %s to %s\n",
         source->sMemServices->endpoint()->end_point.c_str(),
         target->sMemServices->endpoint()->end_point.c_str() );
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
						       source->sMemServices->endpoint(),
						       target->sMemServices->endpoint() );
  if ( ! ptemplate ) {
    ocpiAssert(0);
  }

  OS::uint32_t offset = sizeof(ContainerComms::MailBox) * m_slot + sizeof(OS::uint32_t);

#ifndef NDEBUG
  printf("In make request with offset = %d\n", offset );
#endif

  XferRequest * ptransfer = ptemplate->createXferRequest();

  // create the copy in the template
  ptransfer->copy (
		   offset + sizeof(ContainerComms::BasicReq),
		   offset + sizeof(ContainerComms::BasicReq),
		   sizeof(ContainerComms::MailBox) - sizeof(ContainerComms::BasicReq),
		   XferRequest::FirstTransfer );

  ptransfer->copy (
		   offset,
		   offset,
		   sizeof(ContainerComms::BasicReq),
		   XferRequest::LastTransfer );

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
    printf("new template from %s to %s\n", s_endpoint->end_point.c_str(), t_endpoint->end_point.c_str());
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

  OU::AutoMutex guard ( m_mutex, true );

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
    printf("new template from %s to %s\n", source_sname.c_str(), target_sname.c_str());
    pxfer = factory->getXferServices(source_info->sMemServices, target_info->sMemServices);

    /* Insert the template into the template list */
    if (add_template(source_sname, target_sname, pxfer)) {
      return NULL;
    }
  }

  return pxfer;
}

  void Device::
  configure(ezxml_t x)
  {
    OD::Device::configure(x); // give the base class a chance to do generic configuration
    parse(&driverBase(), x);
  }
}
