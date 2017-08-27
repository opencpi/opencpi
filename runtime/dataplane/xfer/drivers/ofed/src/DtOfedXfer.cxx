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
 *  John Miller -  12/2010
 *  Initial version
 *
 */
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <map>
#include <list>
#include <vector>
#include <verbs.h>
#include <ezxml.h>

#include "OcpiOsDataTypes.h"
#include "OcpiOsMisc.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiPValue.h"
#include "XferException.h"
#include "XferDriver.h"
#include "XferServices.h"
#include "XferFactory.h"
#include "XferEndPoint.h"
#include "XferManager.h"

#define MASK_ADDR( x ) if(sizeof(char*)==4)((x) &= 0x00000000ffffffffL);

using namespace OCPI::Util;
using namespace OCPI::OS;
namespace OX = OCPI::Util::EzXml;
namespace OU = OCPI::Util;
namespace XF = DataTransfer;
// Create tranfer services template
const int MAX_TX_DEPTH = 8*1024;
const int MAX_Q_DEPTH = 4*1024;
namespace DataTransfer {
  namespace OFED {

    /**********************************
     * This is the Programmed I/O transfer request class
     *********************************/
    class XferServices;
    class XferRequest : public XF::TransferBase<XferServices,XferRequest>
    {
    public:
      friend class XferServices;

      // Constructor
      XferRequest( XferServices &s);

      XF::XferRequest & group( XF::XferRequest* lhs );
      
      XF::XferRequest* copy (DtOsDataTypes::Offset srcoff, 
			 DtOsDataTypes::Offset dstoff, 
			 size_t nbytes, 
			 XF::XferRequest::Flags flags
			 );

      // Queue data transfer request
      void post ();

      // Get Information about a Data Transfer Request
      XF::XferRequest::CompletionStatus getStatus();

      // Destructor - implementation at end of file
      virtual ~XferRequest ();
      
      // Modify the source buffer offfsets
      void modify(DtOsDataTypes::Offset new_offsets[], DtOsDataTypes::Offset old_offsets[] );

      // Data members accessible from this/derived class
    protected:
      ibv_send_wr * m_wr;
      ibv_send_wr ** m_nextWr;
      ibv_send_wr * m_firstWr;
      ibv_send_wr * m_lastWr;
      ibv_send_wr * m_badWr[3];
      int m_PCount, m_PComplete;
      XF::XferRequest::CompletionStatus m_status;
    };


    // XferServices specializes 
    class SmemServices;
    class XferFactory;
    class XferServices : public XF::ConnectionBase<XferFactory,XferServices,XferRequest>
    {
      friend class XferRequest;
    public:

      // Constructor
      XferServices(XF::EndPoint &source, XF::EndPoint &target);

      // Destructor
     virtual ~XferServices ();
     
     // Create tranfer request object
     XF::XferRequest* createXferRequest();

     // Get the connection cookie
     uint64_t getConnectionCookie();

     // Finalize this connection
     void finalize( uint64_t cookie);

    protected:

     // Interpret the status of any pending/complete transfer requests
     void status();

     // Source SMB services pointer
     SmemServices* m_sourceSmb;

     // Target SMB services pointer
     SmemServices* m_targetSmb;	

#ifndef NDEBUG
      // Add/Remove posted transfer
      void addPost( XferRequest *r ) {m_xfers.push_back(r);}
      void remPost( XferRequest *r ) {m_xfers.remove(r);}
      std::list<XferRequest*> m_xfers;
#else
      void addPost( XferRequest * ) {}
      void remPost( XferRequest * ) {}
#endif


      OCPI::OS::Mutex     m_mutex;     
      ibv_qp            * m_qp;
      uint32_t            m_tqpn;
      bool                m_finalized;
      int                 m_post_count;
      int                 m_cq_count;

#ifdef L1_DEBUG
      int cq_mod;
#endif

    };


    class Device;
    struct EndPoint : public XF::EndPoint 
    {
      Device *     m_device;
      std::string  m_addr;
      std::string  m_dev;

      // Each device can support N endpoints, we hold the device context for our endpoint
      ibv_gid               m_gid;
      uint16_t              m_lid;
      uint32_t              m_psn;
      uint32_t              m_rkey;
      uint32_t              m_lkey;
      uint64_t              m_vaddr;
      uint8_t               m_port;
      // Constructors
      EndPoint(XferFactory &factory, const char *protoInfo, const char *eps, const char *other,
	       bool local, size_t size, const OU::PValue *params);
      
      // Destructor
      virtual ~EndPoint();

      // Sets smem location data based upon the specified endpoint
      void parse(const char *protoInfo);

      // Finalize this endpoint
      void finalize();

      XF::SmemServices &createSmemServices();

    };


    // Shared memory services.  
    class SmemServices : public XF::SmemServices
    {
    public:
      SmemServices (/* XF::XferFactory * parent, */EndPoint& ep);

      OCPI::OS::int32_t attach ( XF::EndPoint* ){return 0;};
      OCPI::OS::int32_t detach (){return 0;}
      void* map (DtOsDataTypes::Offset offset, size_t/* size */)
      {
	// Local memory, just pass back the pointer
	return &m_mem[offset];
      }
      OCPI::OS::int32_t unMap (){return 0;}
      virtual ~SmemServices ();

      // Convenience functions
      inline EndPoint * getOfedEp(){return m_ofed_ep;}
      inline Device   * getOfedDevice(){return static_cast<Device *>(m_ofed_ep->m_device);}
      ibv_qp          * getNextQp();
      ibv_mr          *& getMr(){return m_mr;}
      ibv_cq          *& getCq(){return m_cq;}

    private:
      ibv_cq       * m_cq;
      ibv_mr       * m_mr;
      EndPoint * m_ofed_ep;
      char     * m_mem;
    };

    XF::SmemServices & EndPoint::
    createSmemServices() {
      return *new SmemServices(*this);
    }

#define OFED_DEVICE_ATTRS \
    "port", "hop_limit", "ibv_qp_timeout", "ibv_qp_retry_cnt", \
    "ibv_qp_rnr_retry", "ibv_qp_rnr_timer"


    // Configuration for both the driver (defaults) and the devices
    class FactoryConfig
    {
    public:
      // Constructor happens after the driver is configured so we have
      // any XML as well as the parent's configuration available
      // The xml passed in is for the driver or device
      FactoryConfig()
	: m_port(1),m_hopLimit(3),m_ibv_qp_timeout(14),m_ibv_qp_retry_cnt(12),
	  m_ibv_qp_rnr_retry(1),m_ibv_qp_rnr_timer(14)
      {}
      void parse(FactoryConfig *defs, ezxml_t x)
      {
	if (defs)
	  *this = *defs;
	if (!x)
	  return;
#ifndef NDEBUG
	printf("Processing device %s\n", ezxml_attr(x,"name") );
#endif
	const char *err;
	if ((err = OX::checkAttrs(x, OFED_DEVICE_ATTRS, NULL)) ||
	    (err = OX::getNumber(x, "port", &m_port, NULL, 0, false)) ||
	    (err = OX::getNumber8(x, "hop_limit", &m_hopLimit, NULL, 0, false)) ||
	    (err = OX::getNumber8(x, "ibv_qp_timeout", &m_ibv_qp_timeout,
				  NULL, 0, false)) ||
	    (err = OX::getNumber8(x, "ibv_qp_retry_cnt", &m_ibv_qp_retry_cnt,
				  NULL, 0, false)) ||
	    (err = OX::getNumber8(x, "ibv_qp_rnr_retry", &m_ibv_qp_rnr_retry,
				  NULL, 0, false)) ||
	    (err = OX::getNumber8(x, "ibv_qp_rnr_timer", &m_ibv_qp_rnr_timer,
				  NULL, 0, false)))
	  throw err; // FIXME API configuration error exception
      }
 
      size_t m_port;
      uint8_t  m_hopLimit;
      uint8_t  m_ibv_qp_timeout;
      uint8_t  m_ibv_qp_retry_cnt;
      uint8_t  m_ibv_qp_rnr_retry;
      uint8_t  m_ibv_qp_rnr_timer;
    };

    /**********************************
     * Each transfer implementation must implement a factory class.  This factory
     * implementation creates a named resource compatible SMB and a programmed I/O
     * based transfer driver.
     *********************************/
    const char *ofed = "ofed"; // name passed to inherited template class
    class XferFactory
      : public XF::DriverBase<XferFactory, Device, XferServices, ofed>,
	public FactoryConfig
    {

    public:

      // Default constructor
      XferFactory()
	throw ();

      // Destructor
      virtual ~XferFactory()
	throw ();

      // Configuration
      void configure(ezxml_t);

      // Get our protocol string
      inline const char* getProtocol(){return "ocpi-ofed-rdma";};

      // Factory for derived class
      //      XF::SmemServices *createSmemServices(EndPoint &ep);

      /***************************************
       *  This method is used to create a transfer service object
       ***************************************/
      XF::XferServices & 
      createXferServices(XF::EndPoint &source, XF::EndPoint &target);

      /***************************************
       *  Get the location via the endpoint
       ***************************************/
      XF::EndPoint &createEndPoint(const char *protoInfo, const char *eps, const char *other,
				   bool local, size_t size, const OCPI::Util::PValue *params);
      // From driver base class
      unsigned search(const OCPI::Util::PValue* props, const char **exclude, bool discoveryOnly)
	throw (OCPI::Util::EmbeddedException);

    protected:
      //OCPI::OS::Mutex                  m_mutex;
      //std::map<std::string, EndPoint * > m_map;
    };

    // Note that there is no base class for DT devices
    class Device
      : public XF::DeviceBase<XferFactory,Device>,
	public FactoryConfig
    {
    public:

      ibv_context      * m_context;
      ibv_pd           * m_pd;
      ibv_device       * m_dev;     
      ibv_device_attr    m_device_attribute;
      
      Device(const char* a_name)
	: XF::DeviceBase<XferFactory,Device>(a_name, *this)
      {
	m_dev = find(a_name);
	if (!m_dev) {
	  ocpiInfo("OFED::Device ERROR: OFED device not found (%s)", a_name);
	  throw XF::DataTransferEx( DEV_NOT_FOUND, a_name);
	}
	m_context = ibv_open_device(m_dev);
	if (!m_context) {
	  ocpiInfo("OFED::Device ERROR: Couldn't get context for %s",
		  ibv_get_device_name(m_dev));
	  throw XF::DataTransferEx( COULD_NOT_OPEN_DEVICE, a_name);
	}
	m_pd = ibv_alloc_pd(m_context);
	if (!m_pd) {
	  ocpiInfo("OFED::Device ERROR: Couldn't allocate verbs protection domain");
	  throw XF::DataTransferEx( RESOURCE_EXCEPTION, "protection domain" );	  
	}
      }
      void configure(ezxml_t x) {
	XF::Device::configure(x);
	// Parse for ofed properties
	OFED::FactoryConfig::parse(&parent(), x);
      }

      virtual ~Device()
	throw()
      {
	if (ibv_dealloc_pd(m_pd) < 0)
	  ocpiInfo("OFED::Device ERROR: Could not deallocate protection domain for device");
	if (ibv_close_device(m_context) < 0)
	  ocpiInfo("OFED::Device ERROR: Could not close device");
      }

      ibv_device* find(const char *ib_devname) {
	int num_of_device;
	ibv_device **dev_list;
	ibv_device *ib_dev = NULL;
	dev_list = ibv_get_device_list(&num_of_device);
	if (num_of_device <= 0) {
	  ocpiInfo("OFED::Device ERROR: Did not detect devices");
	  ocpiInfo("If device exists, check if driver is up");
	  return NULL;
	}
	if (!ib_devname) {
	  ib_dev = dev_list[0];
	  if (!ib_dev)
	    ocpiInfo("OFED::Device No IB devices found");
	} else {
	  for (; (ib_dev = *dev_list); ++dev_list)
	    if (!strcmp(ibv_get_device_name(ib_dev), ib_devname))
	      break;
	  if (!ib_dev)
	    ocpiInfo("OFED::Device IB device %s not found", ib_devname);
	}
	return ib_dev;
      }

      void destroyMemContext(SmemServices *ms) {
	if (ibv_destroy_cq( ms->getCq()))
	  ocpiInfo("OFED::Device ERROR: Couldn't destroy Completion Queue");
	if (ibv_dereg_mr(ms->getMr()))
	  ocpiInfo("OFED::Device ERROR: Couldn't de-register memory");
      }

      void createMemContext( SmemServices * ms ) {

	// We dont really want IBV_ACCESS_LOCAL_WRITE, but IB spec says:
	// The Consumer is not allowed to assign Remote Write or Remote Atomic to
	// a Memory Region that has not been assigned Local Write.
	ms->getMr() = 
	  ibv_reg_mr(m_pd, ms->map(0,ms->endPoint().size()), ms->endPoint().size(),
		     IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
	if (! ms->getMr()) {
	  ocpiInfo("OFED::Device ERROR: Couldn't register OFED memory");
	  ocpiInfo("Check memory limits with ""ulimit -l""");
	  throw XF::DataTransferEx( COULD_NOT_OPEN_DEVICE, "memory registration failed" );
	}
	
	ms->getCq() = ibv_create_cq( m_context, MAX_Q_DEPTH, NULL,NULL, 0 );
	if ( ms->getCq() == NULL ) {
	  ocpiInfo("OFED::Device ERROR: Could not create completion Queue");
	  throw XF::DataTransferEx( RESOURCE_EXCEPTION, "completion channel Q" );	  
	}

	int errno;
	ibv_port_attr pattr;
	EndPoint * ep = static_cast<XF::OFED::SmemServices*>(ms)->getOfedEp();
	if (ibv_query_port( ep->m_device->m_context,ep->m_port,&pattr)) {
	  ocpiInfo("OFED::Device ERROR: Could not query device with ibv_query_port(%d)",
		   ep->m_port);
	  throw XF::DataTransferEx( API_ERROR, "ibv_query_port()");
	}
	ep->m_lid  = pattr.lid;
	ep->m_vaddr = (uint64_t)ms->map(0,1);
	MASK_ADDR( ep->m_vaddr );
	ep->m_rkey = ms->getMr()->rkey;
	ep->m_lkey = ms->getMr()->lkey;
	if ((errno=ibv_query_gid( m_context, 
				  ep->m_port, 0,
				  &ep->m_gid))) {
	  ocpiInfo("OFED::Device ERROR: Could not query device with ibv_query_gid() %s",
		   strerror(errno));
	  throw XF::DataTransferEx( API_ERROR, "ibv_query_gid()");
	}
      }
    };
    EndPoint::
    EndPoint(XferFactory &a_factory, const char *protoInfo, const char *eps, const char *other,
	     bool a_local, size_t a_size, const OU::PValue *params)
      : XF::EndPoint(a_factory, eps, other, a_local, a_size, params) { 
      if (protoInfo) {
	m_protoInfo = protoInfo;
	parse(protoInfo);
      } else {
	Device *d;
	const char *deviceName = 0;
	if (OU::findString(params, "Device", deviceName))
	  d = a_factory.findDevice(deviceName);
	else
	  d = a_factory.firstDevice();
	if (!d)
	  throw XF::DataTransferEx(DEV_NOT_FOUND , "OFED");
	// First get the entry point from the properties
	size_t port = d->m_port;
	// This will be the non-finalized version of the ep

	OU::format(m_protoInfo, "%s:%zu:%lld.%lld:%d:%d:%d:%llu",
		   d->name().c_str(), port, (long long)0, (long long)0, 0, 0, 0,
		   (unsigned long long)0);
      }
    }


    // Factory methods
    XferFactory::
    XferFactory()
      throw()
    {
#ifndef NDEBUG
      printf("In OFED::XferFactory()\n");
#endif
    }

    XferFactory::
    ~XferFactory()
      throw()
    {
#ifndef NDEBUG
      printf(" ~OFED::XferFactory() Implement me~~\n");
#endif
    }


    // We assume we are configured before any devices exist.
    void 
    XferFactory::
    configure(ezxml_t x)
    {
      // First parse generic properties and default from parent's
      XF::FactoryConfig::parse(&parent(), x);
      // Next parse our own driver-specific configuration
      OFED::FactoryConfig::parse(NULL, x);
    }

    unsigned 
    XferFactory::
    search(const OCPI::Util::PValue* /*props*/, const char ** /*exclude*/, bool /* discoveryOnly */)
      throw (OCPI::Util::EmbeddedException) 
    {
      int num_of_device;
      ibv_device **dev_list;
      ibv_device *ib_dev = NULL;
      dev_list = ibv_get_device_list(&num_of_device);
      if ((num_of_device <= 0) || (dev_list[0] == NULL)) {
	ocpiInfo("OFED::XferFactory ERROR: Did not detect any OFED devices in the system");
	ocpiInfo(" If device exists, check if driver is up");
	return 0;
      }
      for (; (ib_dev = *dev_list); ++dev_list) {
	const char * dname = ibv_get_device_name(ib_dev);
	ezxml_t devXml = OX::findChildWithAttr(m_xml, "device", "name", dname);
	Device *d = new Device(dname);
	d->configure(devXml);
      }
      // FIXME:  report devices that don't match in xml?
      return num_of_device;
    }

#if 0
    // This method is used to allocate a transfer compatible SMB
    XF::SmemServices* 
    XferFactory::
    createSmemServices( XF::EndPoint& loc )
    {
      OCPI::Util::SelfAutoMutex guard (this);
#if 1
      return new SmemServices(loc);
      //      if (!loc->smem)
      //	loc->smem = new SmemServices(this, loc);
#else
      if ( loc->smem ) {
	return loc->smem;
      }
      m_map.erase(loc->name());
      if ( ! loc->smem ) {
	loc->smem = new SmemServices( this, loc );
      }
      m_map[ loc->end_point ] = static_cast<EndPoint*>(loc);
#endif
      return loc->smem;
    }
#endif

    XF::XferServices &XferFactory::
    createXferServices(XF::EndPoint &source, XF::EndPoint &target) {
      return *new XferServices(source, target);
    }

    XF::EndPoint &XferFactory::
    createEndPoint(const char *protoInfo, const char *eps, const char *other, bool local,
		   size_t size, const OCPI::Util::PValue *params) {
      EndPoint &ep = *new EndPoint(*this, protoInfo, eps, other, local, size, params);
      for (Device *d = firstDevice(); d; d = d->nextDevice())
	if ( d->name() == ep.m_dev ) {
	  ep.m_device = d;
	  break;
	}
      return ep;
    }

    void
    EndPoint::
    finalize()
    {
      m_psn = mailBox();
      OU::format(m_protoInfo,
		 "%s:%u:%lld.%lld:%u:%u:%u:%llu",
		 m_device->name().c_str(),
		 m_port, (long long)m_gid.global.subnet_prefix,
		 (long long)m_gid.global.interface_id, m_lid,
		 m_psn, m_rkey,(unsigned long long)m_vaddr);
      setName();
    }

    // Sets smem location data based upon the specified endpoint
    void EndPoint::
    parse(const char *protoInfo)
    {
      /* ocpi-ofed-rdma:<device id>:<port id>:<gidm.gidl>:<lid>:<psn>:<rkey>:<vaddr>:<smb size>.<mb id> */
      char buf[128];
      int tport;
      int c = sscanf(protoInfo,
		     "%[^:]:%d:%" SCNu64 " .%" SCNu64 ":%hu:%u:%u:%" SCNu64,
		     buf, &tport, 
		     &m_gid.global.subnet_prefix, 
		     &m_gid.global.interface_id, 
		     &m_lid, &m_psn, 
		     &m_rkey, &m_vaddr);
      if ( c != 8 ) {
	ocpiBad("OFED::EndPoint  ERROR: Bad OFED endpoint format (%s)", protoInfo);
	throw XF::DataTransferEx( UNSUPPORTED_ENDPOINT, protoInfo);
      }
      m_dev = buf;
      m_port = (uint8_t)tport;
    }

    EndPoint::
    ~EndPoint()
    {
      // Empty
    }

    void XferRequest::
    modify(DtOsDataTypes::Offset new_offsets[], DtOsDataTypes::Offset old_offsets [] )
    {
      int n=0;
      if ( ! m_wr ) return;
      ibv_send_wr * wr = m_wr;
      while ( wr && new_offsets[n] ) {
	ocpiAssert(wr->sg_list);
	XferServices &xferServices = parent();
	old_offsets[n] =
	  (uint32_t)(wr->sg_list->addr - (uint64_t)xferServices.m_sourceSmb->map(0,1));
	wr->sg_list->addr = (uint64_t)xferServices.m_sourceSmb->map(0,1) + new_offsets[n];
	MASK_ADDR( wr->sg_list->addr );
	n++;
      }
    }

    // XferRequest destructor implementation
    XferRequest::
    ~XferRequest ()
    {
      ibv_send_wr * wr  = m_wr;
      while ( wr ) {
	ibv_send_wr * next = wr->next;
	free ( wr->sg_list);
	free ( wr );
	wr = next;
      };
    }


    void
    XferServices::
    status()
    {
      OCPI::Util::AutoMutex guard ( m_mutex, true ); 
      if ( ! m_post_count ) {
	return;
      }
      const int WC_COUNT=1;
      ibv_wc wc[WC_COUNT];
      int c = ibv_poll_cq( m_sourceSmb->getCq(), WC_COUNT, wc );
      if ( ! c ) {
	OCPI::OS::sleep( 0 );
	c = ibv_poll_cq( m_sourceSmb->getCq(), WC_COUNT, wc );
      }
      
      m_cq_count += c;
      int index=0;
      while ( c ) {
#ifndef NDEBUG
	//	printf("*** Got a completion event\n");
#endif
	if ( c < 0 ) {
	  ocpiInfo("OFED::XferServices ERROR: Couldn't poll completion Q()");
	  throw XF::DataTransferEx( API_ERROR, "ibv_poll_cq()");
	}
	if ( wc[index].status == IBV_WC_SUCCESS ) {
	  reinterpret_cast<XferRequest*>(wc[index].wr_id)->m_PComplete++;
	  reinterpret_cast<XferRequest*>(wc[index].wr_id)->m_status = XF::XferRequest::CompleteSuccess;
	  if ( reinterpret_cast<XferRequest*>(wc[index].wr_id)->m_PComplete ==
	       reinterpret_cast<XferRequest*>(wc[index].wr_id)->m_PCount) {
	    remPost( reinterpret_cast<XferRequest*>(wc[index].wr_id) );
	  }
	}
	else {
	  reinterpret_cast<XferRequest*>(wc[index].wr_id)->m_status = XF::XferRequest::CompleteFailure;
	  ocpiAssert( 0 );
	}
	index++;
	c--;
      }
      
#ifdef L1_DEBUG
      cq_mod++;
      if ( (cq_mod%10000) == 0 ) 
	printf("got %d completions, posted %d\n", m_cq_count, m_post_count);
#endif


    }


    ibv_qp  * 
    SmemServices::
    getNextQp()
    {
      ibv_qp * qp;
      ibv_qp_init_attr iattr;
      memset(&iattr, 0, sizeof(ibv_qp_init_attr));
      iattr.send_cq = getCq();
      iattr.recv_cq = getCq();
      iattr.cap.max_send_wr  = MAX_TX_DEPTH;
      iattr.cap.max_recv_wr  = MAX_TX_DEPTH;
      iattr.cap.max_send_sge = 1;
      iattr.cap.max_recv_sge = 1;
      iattr.cap.max_inline_data = 0;
      iattr.qp_type = IBV_QPT_RC;
      qp = ibv_create_qp( getOfedEp()->m_device->m_pd, &iattr);
      if (! qp)  {
	ocpiInfo("OFED::XferServices ERROR: Could not create Queue Pair");
	throw XF::DataTransferEx( RESOURCE_EXCEPTION, "completion channel Q" );	  
      }
      ibv_qp_attr tattr;
      int flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
      memset(&tattr, 0, sizeof(ibv_qp_attr));
      tattr.qp_state        = IBV_QPS_INIT;
      tattr.pkey_index      = 0;
      tattr.port_num        = getOfedEp()->m_port;
      tattr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE;
      int errno;
      if ( (errno=ibv_modify_qp( qp, &tattr, flags ) ) ) {
	ocpiInfo("OFED::SmemServices ERROR: Failed to modify RC QP to RTR, %s", strerror(errno));
	throw XF::DataTransferEx( API_ERROR, "ibv_query_port()");
      }
      return qp;
    }

    uint64_t
    XferServices::
    getConnectionCookie( )
    {
      return (uint64_t)m_qp->qp_num;
    }


    void 
    XferServices::
    finalize( uint64_t cookie )
    {
      if ( m_finalized ) {
	return;
      }
      m_finalized = true;
      int errno;
      uint32_t qpn = (uint32_t)cookie;

      EndPoint * sep= m_sourceSmb->getOfedEp();
      EndPoint * tep= m_targetSmb->getOfedEp();

      ibv_port_attr pattr;
      if (ibv_query_port( sep->m_device->m_context,sep->m_port,&pattr)) {
	ocpiInfo("OFED::XferServices ERROR: Could not query device with ibv_query_port()");
	throw XF::DataTransferEx( API_ERROR, "ibv_query_port()");
      }
      ibv_qp_attr attr;
      memset(&attr, 0, sizeof attr);
      attr.qp_state 	= IBV_QPS_RTR;
      attr.path_mtu     = pattr.active_mtu;
      attr.dest_qp_num 	= qpn;
      attr.rq_psn       = tep->m_psn;
      attr.ah_attr.dlid   = tep->m_lid;
      attr.max_dest_rd_atomic     = 1;
      attr.min_rnr_timer          = 127;
      attr.ah_attr.is_global  = 1;
      attr.ah_attr.grh.dgid   = tep->m_gid;
#ifndef NDEBUG
      printf("Dest QPN = %d, PSN = %d, dlid = %d, dgid= %lld.%lld\n", attr.dest_qp_num, attr.rq_psn, attr.ah_attr.dlid,
	     (long long)attr.ah_attr.grh.dgid.global.subnet_prefix, (long long)attr.ah_attr.grh.dgid.global.interface_id );
#endif
      attr.ah_attr.grh.sgid_index = 0;
      attr.ah_attr.grh.hop_limit = sep->m_device->m_hopLimit;
      attr.ah_attr.sl         = 0;
      attr.ah_attr.src_path_bits = 0;
      ocpiAssert( tep->m_port == 1 );
      attr.ah_attr.port_num   = tep->m_port;

      if ( (errno=ibv_modify_qp( m_qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_AV                 |
			 IBV_QP_PATH_MTU           |
			 IBV_QP_DEST_QPN           |
			 IBV_QP_RQ_PSN             |
			 IBV_QP_MIN_RNR_TIMER      |
			 IBV_QP_MAX_DEST_RD_ATOMIC
				 ))) {
	ocpiInfo("OFED::XferServices ERROR: Failed to modify RC QP to RTR, %s", strerror(errno));
	throw XF::DataTransferEx( API_ERROR, "ibv_modify_qp()");
      }

      attr.qp_state 	      = IBV_QPS_RTS;
      attr.sq_psn 	      = sep->m_psn;
      attr.timeout            = sep->m_device->m_ibv_qp_timeout;
      attr.retry_cnt          = sep->m_device->m_ibv_qp_retry_cnt;
      attr.rnr_retry          = sep->m_device->m_ibv_qp_rnr_retry;
      attr.min_rnr_timer      = sep->m_device->m_ibv_qp_rnr_timer;
      attr.max_rd_atomic  = 1;
      if ( (errno=ibv_modify_qp( m_qp, &attr,
		      IBV_QP_STATE              |
		      IBV_QP_SQ_PSN             |
		      IBV_QP_TIMEOUT            |
		      IBV_QP_RETRY_CNT          |
		      IBV_QP_RNR_RETRY          |
		      IBV_QP_MAX_QP_RD_ATOMIC
				 ))) {
	ocpiInfo("OFED::XferServices ERROR: Failed to modify RC QP to RTS, %s", strerror(errno));
	throw XF::DataTransferEx( API_ERROR, "ibv_modify_qp()");
      }
    }

    // Create a transfer request
    XF::XferRequest* 
    XferRequest::
    copy (DtOsDataTypes::Offset srcoffs, 
	  DtOsDataTypes::Offset dstoffs, 
	  size_t nbytes, 
	  XF::XferRequest::Flags flags 
	  )
    {
      m_PCount++;
      ibv_send_wr * wr  = (ibv_send_wr*)malloc( sizeof(ibv_send_wr) );
      memset( wr,0,sizeof( ibv_send_wr));
       ibv_sge     * sge = (ibv_sge*)malloc( sizeof(ibv_sge ) );
      memset( sge,0,sizeof( ibv_sge));
      XferServices       & xferServices = parent();
      sge->addr = (uint64_t)xferServices.m_sourceSmb->map(0,nbytes) + srcoffs;
      MASK_ADDR( sge->addr );
#ifndef NDEBUG
      printf("local addr = %llu\n", (unsigned long long )sge->addr );
#endif
      sge->length = (uint32_t)nbytes;
      sge->lkey = xferServices.m_sourceSmb->getMr()->lkey;
      wr->wr_id = (uint64_t)this;
      wr->sg_list = sge;
      wr->num_sge = 1;
      wr->opcode = IBV_WR_RDMA_WRITE;
      wr->next = NULL;
      wr->wr.rdma.remote_addr =  static_cast<EndPoint*>(&xferServices.m_targetSmb->endPoint())->m_vaddr + dstoffs;
      ocpiDebug("***** Remote vaddr = %" PRIu64 ", offset = %" DTOSDATATYPES_OFFSET_PRIu ",nbytes = %zu",
		 wr->wr.rdma.remote_addr, dstoffs, nbytes );
      wr->wr.rdma.rkey = static_cast<EndPoint*>(&xferServices.m_targetSmb->endPoint())->m_rkey;
      wr->send_flags = IBV_SEND_SIGNALED;

      if (  (flags & XF::XferRequest::FlagTransfer) == XF::XferRequest::FlagTransfer ) {
	wr->send_flags |= IBV_SEND_FENCE;
	m_lastWr = wr;
      }
      else if (  (flags & XF::XferRequest::DataTransfer) == XF::XferRequest::DataTransfer ) {
	m_firstWr = wr;
      }
      else {
	if ( m_wr ) {
	  *m_nextWr = wr;
	}
	else {
	  m_wr = wr;
	}
	m_nextWr = &wr->next;
      }
      return this;
    }

    // Group data transfer requests
    XF::XferRequest & 
    XferRequest::
    group (XF::XferRequest* l )
    {
      XferRequest * lhs = static_cast<XferRequest*>(l);
      ibv_send_wr * wr;
      ibv_send_wr * twr = lhs->m_wr;
      while ( twr ) {
	wr = (ibv_send_wr*)malloc( sizeof(ibv_send_wr ));
	*wr = *twr;
	if ( m_wr ) {
	  *m_nextWr = wr;
	}
	else {
	  m_wr = wr;
	}
	m_nextWr = &wr->next;
	twr = wr->next;
      }
      if ( lhs->m_firstWr ) {
	wr = (ibv_send_wr*)malloc( sizeof(ibv_send_wr ));
	*wr = *lhs->m_firstWr;
	m_firstWr->next = wr;
      }
      if ( lhs->m_lastWr ) {
	wr = (ibv_send_wr*)malloc( sizeof(ibv_send_wr ));
	*wr = *lhs->m_lastWr;
	m_lastWr->next = wr;
      }
      return *this;
    }


    // Queue data transfer request
    void 
    XferRequest::
    post ()
    {
      m_PComplete = 0;
      m_status = XF::XferRequest::Pending;
      int errno;

      parent().addPost( this );      

      if ( m_firstWr ) {
	parent().m_post_count++;
	if ( (errno=ibv_post_send( parent().m_qp, m_firstWr, &m_badWr[0] )) ) {
	  OCPI::OS::sleep( 1 );	  
	  if ( (errno=ibv_post_send( parent().m_qp, m_firstWr, &m_badWr[0] )) ) {
	    ocpiInfo("OFED::XferRequest ERROR: Couldn't post send with ibv_post_send(), %s", strerror(errno));
	    throw XF::DataTransferEx( API_ERROR, "ibv_post_send()");
	  }
	}
      }

      if ( m_wr ) {
	parent().m_post_count++;
	if ( (errno=ibv_post_send( parent().m_qp, m_wr, &m_badWr[1] )) ) {
	  OCPI::OS::sleep( 1 );	  
	  if ( (errno=ibv_post_send( parent().m_qp, m_wr, &m_badWr[1] )) ) {
	    ocpiInfo("OFED::XferRequest ERROR: Couldn't post send with ibv_post_send(), %s", strerror(errno));
	    throw XF::DataTransferEx( API_ERROR, "ibv_post_send()");
	  }
	}
      }
      
      if ( m_lastWr ) {	
	parent().m_post_count++;
	if ( (errno=ibv_post_send(parent().m_qp, m_lastWr, &m_badWr[2] )) ) {
	  OCPI::OS::sleep( 1 );	  
	  if ( (errno=ibv_post_send( parent().m_qp, m_lastWr, &m_badWr[2] )) ) {
	    ocpiInfo("OFED::XferRequest ERROR: Couldn't post send with ibv_post_send(), %s", strerror(errno));
	    throw XF::DataTransferEx( API_ERROR, "ibv_post_send()");
	  }
	}
      }

    }


    XF::XferRequest::CompletionStatus 
    XferRequest::
    getStatus()
    {
      parent().status();

      




      if ( (m_status==XF::XferRequest::CompleteSuccess)
	   && (m_PComplete == m_PCount ) ) {
	return m_status;
      }
      return XF::XferRequest::Pending;
    }


    XferRequest::
    XferRequest(XferServices &s)
      : XF::TransferBase<XferServices,XferRequest>(s, *this),
	m_wr(NULL),m_nextWr(NULL),m_firstWr(NULL),m_lastWr(NULL), m_PCount(0), m_PComplete(0)
    {

    }

    XferServices::
    XferServices(XF::EndPoint &source, XF::EndPoint &target)
      : XF::ConnectionBase<XferFactory,XferServices,XferRequest>(*this, source, target),
	m_finalized(false), m_post_count(0), m_cq_count(0)
    {
      OCPI::Util::AutoMutex guard ( m_mutex, true ); 
      m_sourceSmb = static_cast<SmemServices*>(&source.sMemServices());
      m_targetSmb = static_cast<SmemServices*>(&target.sMemServices());
      if ( m_sourceSmb->endPoint().name() ==  m_targetSmb->endPoint().name() ) {
	ocpiAssert(0);
      }
      m_qp = m_sourceSmb->getNextQp();
    }

    XF::XferRequest* 
    XferServices::
    createXferRequest()
    {

#ifndef NDEBUG
      if ( ! m_finalized ) {
	printf("Attempt to create XferRequest object with non-finalized transfer service\n");
	printf(" s = (%s) t = (%s)\n", m_sourceSmb->endPoint().name().c_str(),
	       m_targetSmb->endPoint().name().c_str() );

      }
#endif

      ocpiAssert( m_finalized );
      return new XferRequest( *this );
    }


    XferServices::
    ~XferServices ()
    {
      if ( ibv_destroy_qp( m_qp ) ) {
	ocpiInfo("OFED :XferServies ERROR: failed to destroy queue pair");
      }
    }

    SmemServices::
    SmemServices (EndPoint& ep)
      :XF::SmemServices(ep), m_ofed_ep(&ep)
    {

      if (ep.local()) {
	if (posix_memalign((void**)&m_mem, sysconf(_SC_PAGESIZE), ep.size())) {
	  ocpiDebug("OFED::SmemServices Error: Couldn't allocate SMB.");
	  throw XF::DataTransferEx( NO_MORE_SMB, "memalign failed" );
	}
	memset(m_mem, 0, ep.size());
	m_ofed_ep->m_device->createMemContext( this );

	// Now that the memory has been registered, we can finalize our endpoint.
	//	EndPoint * ep = static_cast<EndPoint*>(endpoint());
	m_ofed_ep->m_vaddr = (uint64_t)m_mem;
	MASK_ADDR( m_ofed_ep->m_vaddr );
	ep.finalize();
      }
      else {
	m_mem = (char*)m_ofed_ep->m_vaddr;
      }
    };

    SmemServices::
    ~SmemServices ()
    {
      m_ofed_ep->m_device->destroyMemContext( this );
      delete [] m_mem;
    }
#ifndef OCPI_OS_macos
    RegisterTransferDriver<XferFactory> driver;
#endif
  }
}


