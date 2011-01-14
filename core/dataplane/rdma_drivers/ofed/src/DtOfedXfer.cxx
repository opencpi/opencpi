
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

#ifdef OFED_RDMA_SUPPORT

/*
 *  John Miller -  12/2010
 *  Initial version
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <ezxml.h>

#include <infiniband/verbs.h>

#include <OcpiOsDataTypes.h>
#include <OcpiOsMutex.h>
#include <DtTransferInterface.h>
#include <DtSharedMemoryInterface.h>
#include <DtSharedMemoryInternal.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilAutoMutex.h>
#include <DtExceptions.h>
#include <OcpiPValue.h>

#define MASK_ADDR( x ) if(sizeof(char*)==4)((x) &= 0x00000000ffffffffL);

using namespace OCPI::Util;
using namespace OCPI::OS;

// Create tranfer services template
const int MAX_TX_DEPTH = 100;
const int MAX_Q_DEPTH = 100;

namespace DataTransfer {
  namespace OFED {

    /**********************************
     * This is the Programmed I/O transfer request class
     *********************************/
    class XferServices;
    class XferRequest : public DataTransfer::XferRequest
    {
    public:
      friend class XferServices;

      // Constructor
      XferRequest( XferServices *s);

      DataTransfer::XferRequest & group( DataTransfer::XferRequest* lhs );
      
      DataTransfer::XferRequest* copy (OCPI::OS::uint32_t srcoff, 
			 OCPI::OS::uint32_t dstoff, 
			 OCPI::OS::uint32_t nbytes, 
			 DataTransfer::XferRequest::Flags flags
			 );

      // Queue data transfer request
      void post ();

      // Get Information about a Data Transfer Request
      DataTransfer::XferRequest::CompletionStatus getStatus();

      // Destructor - implementation at end of file
      virtual ~XferRequest ();
      
      // Modify the source buffer offfsets
      void modify( OCPI::OS::uint32_t new_offsets[], OCPI::OS::uint32_t old_offsets[] );

      // Data members accessible from this/derived class
    protected:
      ibv_send_wr * m_wr;
      ibv_send_wr ** m_nextWr;
      ibv_send_wr * m_firstWr;
      ibv_send_wr * m_lastWr;
      ibv_send_wr * m_badWr;
      DataTransfer::XferRequest::CompletionStatus m_status;
    };


    // XferServices specializes 
    class SmemServices;
    class XferServices : public DataTransfer::XferServices
    {
      friend class XferRequest;
    public:

      // Constructor
      XferServices( XferFactory & parent, DataTransfer::SmemServices* source, DataTransfer::SmemServices* target);

      // Destructor
     virtual ~XferServices ();
     
     // Create tranfer request object
     DataTransfer::XferRequest* createXferRequest();

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

     // Create tranfer services template
     void createTemplate (DataTransfer::SmemServices* p1, DataTransfer::SmemServices* p2);

      OCPI::OS::Mutex     m_mutex;     
      ibv_qp            * m_qp;
      uint32_t            m_tqpn;
      bool                m_finalized;
    };


    class SmemServices;
    class  Device;
    struct  EndPoint : public DataTransfer::EndPoint 
    {
      // Constructors
      EndPoint( std::string & ep, bool local )
	: DataTransfer::EndPoint( ep, 0, local ),m_port(1)
      {
	parse(ep);
	m_psn = mailbox;
      }
      
      // Destructor
      virtual ~EndPoint();

      // Sets smem location data based upon the specified endpoint
      OCPI::OS::int32_t parse( std::string& ep );

      // Get the address from the endpoint
      virtual const char* getAddress(){return m_addr.c_str();}

      // Finalize this endpoint
      void finalize();

      Device *     m_device;
      std::string  m_addr;
      std::string  m_dev;

      // Each device can support N endpoints, we hold the device context for our endpoint
      ibv_gid               m_gid;
      uint32_t              m_lid;
      uint32_t              m_psn;
      uint32_t              m_rkey;
      uint32_t              m_lkey;
      uint64_t              m_vaddr;
      uint8_t               m_port;
    };


    // Shared memory services.  
    class SmemServices : public DataTransfer::SmemServices
    {
    public:
      SmemServices ( DataTransfer::XferFactory * parent, DataTransfer::EndPoint* ep);

      OCPI::OS::int32_t attach ( DataTransfer::EndPoint* ){return 0;};
      OCPI::OS::int32_t detach (){return 0;}
      void* map (OCPI::OS::uint64_t offset, OCPI::OS::uint32_t/* size */)
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


    /**********************************
     * Each transfer implementation must implement a factory class.  This factory
     * implementation creates a named resource compatible SMB and a programmed I/O
     * based transfer driver.
     *********************************/
    class FactoryConfig;
    class XferFactory : public DataTransfer::XferFactory {

    public:

      // Default constructor
      XferFactory()
	throw ();

      // Destructor
      virtual ~XferFactory()
	throw ();

      // Get our protocol string
      inline const char* getProtocol(){return "ocpi-ofed-rdma";};

      /***************************************
       * Configure the factory using the specified xml data
       ***************************************/
      virtual void configure(  DataTransfer::FactoryConfig & config );


      /***************************************
       * This method is used to allocate a transfer compatible SMB
       ***************************************/
      DataTransfer::SmemServices * getSmemServices( DataTransfer::EndPoint * ep );

      /***************************************
       *  This method is used to create a transfer service object
       ***************************************/
      DataTransfer::XferServices* 
      getXferServices(DataTransfer::SmemServices* source, DataTransfer::SmemServices* target);

      /***************************************
       *  Get the location via the endpoint
       ***************************************/
      DataTransfer::EndPoint* getEndPoint( std::string& end_point, bool local );
      void releaseEndPoint( DataTransfer::EndPoint* loc );

      /***************************************
       *  This method is used to dynamically allocate
       *  an endpoint for an application running on "this"
       *  node.
       ***************************************/
      std::string allocateEndpoint(OCPI::Util::Device * device, OCPI::Util::PValue * props );

      // From driver base class
      unsigned search(const OCPI::Util::PValue* props, const char **exclude)
	throw (OCPI::Util::EmbeddedException);

    protected:
      OCPI::OS::Mutex                  m_mutex;
      std::map<std::string, EndPoint * > m_map;
      std::map<std::string, FactoryConfig * > m_config;
    };


    class FactoryConfig;
    class Device : public OCPI::Util::Device {
    public:

      std::string        m_name;
      ibv_context      * m_context;
      ibv_pd           * m_pd;
      ibv_device       * m_dev;     
      ibv_device_attr    m_device_attribute;
      FactoryConfig    * m_config;
      
      Device( XferFactory * parent, const char* name, FactoryConfig * config ) 
	:OCPI::Util::Device( *parent, name ),m_name(name),m_config(config)
      {
	m_dev = find( name );
	if ( ! m_dev ) {
	  fprintf(stderr, "OFED::Device ERROR: OFED device not found (%s)\n", name );
	  throw DataTransfer::DataTransferEx( DEV_NOT_FOUND, name );
	}
	m_context = ibv_open_device(m_dev);
	if (!m_context) {
	  fprintf(stderr, "OFED::Device ERROR: Couldn't get context for %s\n",
		  ibv_get_device_name(m_dev));
	  throw DataTransfer::DataTransferEx( COULD_NOT_OPEN_DEVICE, name );
	}
	m_pd = ibv_alloc_pd(m_context);
	if (! m_pd ) {
	  fprintf(stderr, "OFED::Device ERROR: Couldn't allocate verbs protection domain\n");
	  throw DataTransfer::DataTransferEx( RESOURCE_EXCEPTION, "protection domain" );	  
	}
      }

      virtual ~Device()
	throw()
      {
	if ( ibv_dealloc_pd(m_pd) < 0 ) {
	  fprintf(stderr, "OFED::Device ERROR: Could not deallocate protection domain for device\n");
	}
	if ( ibv_close_device(m_context) < 0 ) {
	  fprintf(stderr, "OFED::Device ERROR: Could not close device\n");
	}
      }

      ibv_device* find(const char *ib_devname) {
	int num_of_device;
	ibv_device **dev_list;
	ibv_device *ib_dev = NULL;
	dev_list = ibv_get_device_list(&num_of_device);
	if (num_of_device <= 0) {
	  fprintf(stderr,"OFED::Device ERROR: Did not detect devices \n");
	  fprintf(stderr,"If device exists, check if driver is up\n");
	  return NULL;
	}
	if (!ib_devname) {
	  ib_dev = dev_list[0];
	  if (!ib_dev)
	    fprintf(stderr, "OFED::Device No IB devices found\n");
	} else {
	  for (; (ib_dev = *dev_list); ++dev_list)
	    if (!strcmp(ibv_get_device_name(ib_dev), ib_devname))
	      break;
	  if (!ib_dev)
	    fprintf(stderr, "OFED::Device IB device %s not found\n", ib_devname);
	}
	return ib_dev;
      }

      void destroyMemContext(  SmemServices * ms ) {
	if ( ibv_destroy_cq( ms->getCq() ) ) {
	  fprintf(stderr, "OFED::Device ERROR: Couldn't destroy Completion Queue\n");
	}
	if ( ibv_dereg_mr(  ms->getMr() ) ) {
	  fprintf(stderr, "OFED::Device ERROR: Couldn't de-register memory");
	}
      }

      void createMemContext( SmemServices * ms ) {

	// We dont really want IBV_ACCESS_LOCAL_WRITE, but IB spec says:
	// The Consumer is not allowed to assign Remote Write or Remote Atomic to
	// a Memory Region that has not been assigned Local Write.
	ms->getMr() = 
	  ibv_reg_mr(m_pd, ms->map(0,ms->endpoint()->size), ms->endpoint()->size,
		     IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
	if (! ms->getMr()) {
	  fprintf(stderr, "OFED::Device ERROR: Couldn't register OFED memory\n");
	  fprintf(stderr, "Check memory limits with ""ulimit -l""\n");
	  throw DataTransfer::DataTransferEx( COULD_NOT_OPEN_DEVICE, "memory registration failed" );
	}
	
	ms->getCq() = ibv_create_cq( m_context, MAX_Q_DEPTH, NULL,NULL, 0 );
	if ( ms->getCq() == NULL ) {
	  fprintf(stderr, "OFED::Device ERROR: Could not create completion Queue\n");
	  throw DataTransfer::DataTransferEx( RESOURCE_EXCEPTION, "completion channel Q" );	  
	}

	int errno;
	ibv_port_attr pattr;
	EndPoint * ep = static_cast<DataTransfer::OFED::SmemServices*>(ms)->getOfedEp();
	if (ibv_query_port( ep->m_device->m_context,ep->m_port,&pattr)) {
	  fprintf(stderr,"OFED::Device ERROR: Could not query device with ibv_query_port(%d)\n", ep->m_port);
	  throw DataTransfer::DataTransferEx( API_ERROR, "ibv_query_port()");
	}
	ep->m_lid  = pattr.lid;
	ep->m_vaddr = (uint64_t)ms->map(0,1);
	MASK_ADDR( ep->m_vaddr );
	ep->m_rkey = ms->getMr()->rkey;
	ep->m_lkey = ms->getMr()->lkey;
	if ((errno=ibv_query_gid( m_context, 
				  ep->m_port, 0,
				  &ep->m_gid))) {
	  fprintf(stderr,"OFED::Device ERROR: Could not query device with ibv_query_gid() %s\n", strerror(errno));
	  throw DataTransfer::DataTransferEx( API_ERROR, "ibv_query_gid()");
	}
      }
    };


    // Factory methods
    XferFactory::
    XferFactory()
      throw()
      :DataTransfer::XferFactory("OFED IB-Verbs enabled data transfer driver")
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


    class FactoryConfig {
    public:
      FactoryConfig( const DataTransfer::FactoryConfig & lhs, ezxml_t dnode )
	: m_port(1),m_timeout(10),m_hopLimit(1)
      {
	m_sys = lhs;
	if ( dnode ) {
	  m_sys.parse( dnode );
	  if ( ezxml_attr(dnode,"name") ) {
#ifndef NDEBUG
	    printf("Processing device %s\n", ezxml_attr(dnode,"name") );
#endif
	    DataTransfer::FactoryConfig::getLProp( dnode, "port", "value", m_port);
	    DataTransfer::FactoryConfig::getLProp( dnode, "timeout", "value", m_timeout);
	    DataTransfer::FactoryConfig::getLProp( dnode, "hopLimit", "value", m_hopLimit);
	  }
	}
      }
      DataTransfer::FactoryConfig m_sys;
 
      uint32_t m_port;
      uint32_t m_timeout;
      uint32_t m_hopLimit;
      ezxml_t  m_node;
    };

    void 
    XferFactory::
    configure(  DataTransfer::FactoryConfig & config )
    {
      m_config[ "default" ]  = new FactoryConfig( config, NULL );	  
      // Find our node
      ezxml_t m_node = XferFactory::getNode( config.m_xml, "ocpi-ofed-rdma");
      if ( m_node ) {
	// Under our node are the devices
	ezxml_t dnode = ezxml_child( m_node, "Device");
	while ( dnode ) {
	  const char * name = ezxml_attr(dnode, "name");
	  if ( name ) {
	    m_config[ name ]  = new FactoryConfig( config, dnode );	  
	  }
	  dnode = ezxml_next(dnode);
	}
      }
    }

    unsigned 
    XferFactory::
    search(const OCPI::Util::PValue* /*props*/, const char ** /*exclude*/)
      throw (OCPI::Util::EmbeddedException) 
    {
      int num_of_device;
      ibv_device **dev_list;
      ibv_device *ib_dev = NULL;
      dev_list = ibv_get_device_list(&num_of_device);
      if ((num_of_device <= 0) || (dev_list[0] == NULL)) {
	fprintf(stderr,"OFED::XferFactory ERROR: Did not detect any OFED devices in the system\n");
	fprintf(stderr," If device exists, check if driver is up\n");
	return 0;
      }
      for (; (ib_dev = *dev_list); ++dev_list) {
	const char * dname = ibv_get_device_name(ib_dev);
	FactoryConfig * config = m_config[ dname ] ? m_config[ dname ] : m_config[ "default" ];
	new Device( this, dname, config );
      }
      return num_of_device;
    }


    // Get the location via the endpoint
    DataTransfer::EndPoint* 
    XferFactory::
    getEndPoint( std::string& end_point, bool local )
    { 
      OCPI::Util::AutoMutex guard ( m_mutex, true ); 
      std::map<std::string, EndPoint*>::iterator it;      
      it = m_map.find( end_point );
      if (  it == m_map.end() ) {
	EndPoint * ep =  new EndPoint( end_point, local );
	m_map[ end_point ] = ep;
	if ( local ) {
	  Device * d = NULL;
	  do {
	    d = static_cast<Device*>(getNextDevice(d));
	    ocpiAssert(d);
	    if ( d->m_name == ep->m_dev ) {
	      ep->m_device = d;
	      break;
	    }
	  } while ( d );
	}
	return ep;
      }
      return (*it).second;
    }


    // This method is used to allocate a transfer compatible SMB
    DataTransfer::SmemServices* 
    XferFactory::
    getSmemServices( DataTransfer::EndPoint* loc )
    {
      OCPI::Util::AutoMutex guard ( m_mutex, true ); 
      if ( loc->smem ) {
	return loc->smem;
      }
      m_map.erase( loc->end_point );
      if ( ! loc->smem ) {
	loc->smem = new SmemServices( this, loc );
      }
      m_map[ loc->end_point ] = static_cast<EndPoint*>(loc);
      return loc->smem;
    }


    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    DataTransfer::XferServices* 
    XferFactory::
    getXferServices( DataTransfer::SmemServices * s, 
		     DataTransfer::SmemServices * t )
    {
      return new XferServices( *this, s, t );
    }


    std::string 
    XferFactory::
    allocateEndpoint(OCPI::Util::Device * d, OCPI::Util::PValue * )
    {
      OCPI::Util::AutoMutex guard ( m_mutex, true ); 
      if ( ! d ) {
	d = getNextDevice(NULL);
      }
      if ( ! d ) {
	throw DataTransfer::DataTransferEx( DEV_NOT_FOUND , "OFED" );
      }
      Device * device = static_cast<Device*>(d);

      // First get the entry point from the properties
      unsigned int port = device->m_config->m_port;
      unsigned int size = device->m_config->m_sys.m_SMBSize;

      // This will be the non-finalized version of the ep
      char buf[512];
      int mailbox = getNextMailBox();
      snprintf( buf, 512, "ocpi-ofed-rdma://%s:%d:%lld.%lld:%d:%d:%d:%llu:%d.%d.%d", device->m_name.c_str(),
		port, (long long)0,(long long)0,0,0,0,(unsigned long long)0,size,mailbox,getMaxMailBox());
      std::string ep = buf;
      return ep;
    }


    void
    EndPoint::
    finalize()
    {
      // This will be the non-finalized version of the ep
      char buf[512];
      snprintf( buf, 512, "ocpi-ofed-rdma://%s:%d:%lld.%lld:%d:%d:%d:%llu:%d.%d.%d", m_device->m_name.c_str(),
		m_port, (long long)m_gid.global.subnet_prefix,
		(long long)m_gid.global.interface_id, m_lid,
		m_psn, m_rkey,(unsigned long long)m_vaddr,size,mailbox,maxCount);
      end_point = buf;
    }

    // Sets smem location data based upon the specified endpoint
    OCPI::OS::int32_t 
    EndPoint::
    parse( std::string& ep )
    {
      /* ocpi-ofed-rdma://<device id>:<port id>:<gidm.gidl>:<lid>:<psn>:<rkey>:<vaddr>:<smb size>.<mb id> */
      char buf[128];
      int tport;
      int c = sscanf(ep.c_str(),"ocpi-ofed-rdma://%[^:]:%d:%lld.%lld:%d:%d:%d:%llu:%d.%d.%d", buf, &tport, 
		     (unsigned long long*)&m_gid.global.subnet_prefix, 
		     (unsigned long long*)&m_gid.global.interface_id, 
		     &m_lid, &m_psn, 
		     &m_rkey, (unsigned long long*)&m_vaddr, &size, &mailbox, &maxCount );
      if ( c != 11 ) {
	fprintf( stderr, "OFED::EndPoint  ERROR: Bad OFED endpoint format (%s)\n", ep.c_str() );
	throw DataTransfer::DataTransferEx( UNSUPPORTED_ENDPOINT, ep.c_str() );	  
      }
      m_dev = buf;
      m_port = (uint8_t)tport;
      return 0;
    }

    EndPoint::
    ~EndPoint()
    {
      // Empty
    }

    void XferRequest::
    modify( OCPI::OS::uint32_t new_offsets[], OCPI::OS::uint32_t old_offsets [] )
    {
      int n=0;
      if ( ! m_wr ) return;
      ibv_send_wr * wr = m_wr;
      while ( wr && new_offsets[n] ) {
	ocpiAssert(wr->sg_list);
	XferServices * xferServices = static_cast<XferServices*>(myParent);
	old_offsets[n] = wr->sg_list->addr - (uint64_t)xferServices->m_sourceSmb->map(0,1);
	wr->sg_list->addr = (uint64_t)xferServices->m_sourceSmb->map(0,1) + new_offsets[n];
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
      const int WC_COUNT=30;
      ibv_wc wc[WC_COUNT];
      int c = ibv_poll_cq( m_sourceSmb->getCq(), WC_COUNT, wc );
      int index=0;
      while ( c ) {
#ifndef NDEBUG
	printf("*** Got a completion event\n");
#endif
	if ( c < 0 ) {
	  fprintf(stderr,"OFED::XferServices ERROR: Couldn't poll completion Q()\n");
	  throw DataTransfer::DataTransferEx( API_ERROR, "ibv_poll_cq()");
	}
	if ( wc[index].status == IBV_WC_SUCCESS ) {
	  reinterpret_cast<XferRequest*>(wc[index].wr_id)->m_status = DataTransfer::XferRequest::CompleteSuccess;
	}
	else {
	  reinterpret_cast<XferRequest*>(wc[index].wr_id)->m_status = DataTransfer::XferRequest::CompleteFailure;
	}
	index++;
	c--;
      }
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
	fprintf(stderr, "OFED::XferServices ERROR: Could not create Queue Pair\n");
	throw DataTransfer::DataTransferEx( RESOURCE_EXCEPTION, "completion channel Q" );	  
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
	fprintf(stderr, "OFED::SmemServices ERROR: Failed to modify RC QP to RTR, %s\n", strerror(errno));
	throw DataTransfer::DataTransferEx( API_ERROR, "ibv_query_port()");
      }
      return qp;
    }


    void 
    XferServices::
    createTemplate ( DataTransfer::SmemServices* p1, DataTransfer::SmemServices* p2)
    {
      OCPI::Util::AutoMutex guard ( m_mutex, true ); 
      m_sourceSmb = static_cast<DataTransfer::OFED::SmemServices*>(p1);
      m_targetSmb = static_cast<DataTransfer::OFED::SmemServices*>(p2);
      if ( m_sourceSmb->endpoint()->end_point ==  m_targetSmb->endpoint()->end_point ) {
	ocpiAssert(0);
      }
      m_qp = m_sourceSmb->getNextQp();
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
	fprintf(stderr,"OFED::XferServices ERROR: Could not query device with ibv_query_port()\n");
	throw DataTransfer::DataTransferEx( API_ERROR, "ibv_query_port()");
      }
      ibv_qp_attr attr;
      memset(&attr, 0, sizeof attr);
      attr.qp_state 	= IBV_QPS_RTR;
      attr.path_mtu     = pattr.active_mtu;
      attr.dest_qp_num 	= qpn;
      attr.rq_psn       = tep->m_psn;
      attr.ah_attr.dlid   = tep->m_lid;
      attr.max_dest_rd_atomic     = 1;
      attr.min_rnr_timer          = 12;
      attr.ah_attr.is_global  = 1;
      attr.ah_attr.grh.dgid   = tep->m_gid;
#ifndef NDEBUG
      printf("Dest QPN = %d, PSN = %d, dlid = %d, dgid= %lld.%lld\n", attr.dest_qp_num, attr.rq_psn, attr.ah_attr.dlid,
	     (long long)attr.ah_attr.grh.dgid.global.subnet_prefix, (long long)attr.ah_attr.grh.dgid.global.interface_id );
#endif
      attr.ah_attr.grh.sgid_index = 0;
      attr.ah_attr.grh.hop_limit = sep->m_device->m_config->m_hopLimit;
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
	fprintf(stderr, "OFED::XferServices ERROR: Failed to modify RC QP to RTR, %s\n", strerror(errno));
	throw DataTransfer::DataTransferEx( API_ERROR, "ibv_modify_qp()");
      }

      attr.qp_state 	      = IBV_QPS_RTS;
      attr.sq_psn 	      = sep->m_psn;
      attr.timeout            = sep->m_device->m_config->m_timeout;
      attr.retry_cnt          = sep->m_device->m_config->m_sys.m_retryCount;
      attr.rnr_retry          = sep->m_device->m_config->m_sys.m_retryCount;
      attr.max_rd_atomic  = 1;
      if ( (errno=ibv_modify_qp( m_qp, &attr,
		      IBV_QP_STATE              |
		      IBV_QP_SQ_PSN             |
		      IBV_QP_TIMEOUT            |
		      IBV_QP_RETRY_CNT          |
		      IBV_QP_RNR_RETRY          |
		      IBV_QP_MAX_QP_RD_ATOMIC
				 ))) {
	fprintf(stderr, "OFED::XferServices ERROR: Failed to modify RC QP to RTS, %s\n", strerror(errno));
	throw DataTransfer::DataTransferEx( API_ERROR, "ibv_modify_qp()");
      }
    }

    // Create a transfer request
    DataTransfer::XferRequest* 
    XferRequest::
    copy (OCPI::OS::uint32_t srcoffs, 
	  OCPI::OS::uint32_t dstoffs, 
	  OCPI::OS::uint32_t nbytes, 
	  DataTransfer::XferRequest::Flags flags 
	  )
    {
      ibv_send_wr * wr  = (ibv_send_wr*)malloc( sizeof(ibv_send_wr) );
      memset( wr,0,sizeof( ibv_send_wr));
       ibv_sge     * sge = (ibv_sge*)malloc( sizeof(ibv_sge ) );
      memset( sge,0,sizeof( ibv_sge));
      XferServices       * xferServices = static_cast<XferServices*>(myParent);
      sge->addr = (uint64_t)xferServices->m_sourceSmb->map(0,nbytes) + srcoffs;
      MASK_ADDR( sge->addr );
#ifndef NDEBUG
      printf("local addr = %llu\n", (unsigned long long )sge->addr );
#endif
      sge->length = nbytes;
      sge->lkey = xferServices->m_sourceSmb->getMr()->lkey;
      wr->wr_id = (uint64_t)this;
      wr->sg_list = sge;
      wr->num_sge = 1;
      wr->opcode = IBV_WR_RDMA_WRITE;
      wr->next = NULL;
      wr->wr.rdma.remote_addr =  static_cast<EndPoint*>(xferServices->m_targetSmb->endpoint())->m_vaddr + dstoffs;
#ifndef NDEBUG
      printf("***** Remote vaddr = %llu, offset = %d, nbytes = %d\n",(long long) wr->wr.rdma.remote_addr, dstoffs, nbytes );
#endif
      wr->wr.rdma.rkey = static_cast<EndPoint*>(xferServices->m_targetSmb->endpoint())->m_rkey;
      wr->send_flags = IBV_SEND_SIGNALED;

      if (  (flags & DataTransfer::XferRequest::LastTransfer) == DataTransfer::XferRequest::LastTransfer ) {
#define RXE_BUG
#ifndef RXE_BUG
	wr->send_flags |= IBV_SEND_FENCE;
#endif
	m_lastWr = wr;
      }
      else if (  (flags & DataTransfer::XferRequest::LastTransfer) == DataTransfer::XferRequest::FirstTransfer ) {
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
    DataTransfer::XferRequest & 
    XferRequest::
    group (DataTransfer::XferRequest* l )
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
      m_status = DataTransfer::XferRequest::Pending;
      int errno;

      if ( m_firstWr ) {
	if ( (errno=ibv_post_send( static_cast<XferServices*>(myParent)->m_qp, m_firstWr, &m_badWr )) ) {
	  OCPI::OS::sleep( 1 );	  
	  if ( (errno=ibv_post_send( static_cast<XferServices*>(myParent)->m_qp, m_firstWr, &m_badWr )) ) {
	    fprintf(stderr,"OFED::XferRequest ERROR: Couldn't post send with ibv_post_send(), %s\n", strerror(errno));
	    throw DataTransfer::DataTransferEx( API_ERROR, "ibv_post_send()");
	  }
	}
      }

      if ( m_wr ) {
	if ( (errno=ibv_post_send( static_cast<XferServices*>(myParent)->m_qp, m_wr, &m_badWr )) ) {
	  OCPI::OS::sleep( 1 );	  
	  if ( (errno=ibv_post_send( static_cast<XferServices*>(myParent)->m_qp, m_wr, &m_badWr )) ) {
	    fprintf(stderr,"OFED::XferRequest ERROR: Couldn't post send with ibv_post_send(), %s\n", strerror(errno));
	    throw DataTransfer::DataTransferEx( API_ERROR, "ibv_post_send()");
	  }
	}
      }
      
      OCPI::OS::sleep( 0 );

      if ( m_lastWr ) {	
	if ( (errno=ibv_post_send( static_cast<XferServices*>(myParent)->m_qp, m_lastWr, &m_badWr )) ) {
	  OCPI::OS::sleep( 1 );	  
	  if ( (errno=ibv_post_send( static_cast<XferServices*>(myParent)->m_qp, m_lastWr, &m_badWr )) ) {
	    fprintf(stderr,"OFED::XferRequest ERROR: Couldn't post send with ibv_post_send(), %s\n", strerror(errno));
	    throw DataTransfer::DataTransferEx( API_ERROR, "ibv_post_send()");
	  }
	}
      }

    }


    DataTransfer::XferRequest::CompletionStatus 
    XferRequest::
    getStatus()
    {
      static_cast<XferServices*>(myParent)->status();
      return m_status;
    }


    XferRequest::
    XferRequest( XferServices *s)
      : DataTransfer::XferRequest(*s),m_wr(NULL),m_nextWr(NULL),m_firstWr(NULL),m_lastWr(NULL)
    {

    }

    XferServices::
    XferServices( DataTransfer::XferFactory & parent, DataTransfer::SmemServices* source, DataTransfer::SmemServices* target)
      : DataTransfer::XferServices(parent, source,target),m_finalized(false)
    {
      createTemplate( source, target);
    }

    DataTransfer::XferRequest* 
    XferServices::
    createXferRequest()
    {

#ifndef NDEBUG
      if ( ! m_finalized ) {
	printf("Attempt to create XferRequest object with non-finalized transfer service\n");
	printf(" s = (%s) t = (%s)\n", m_sourceSmb->endpoint()->end_point.c_str(),
	       m_targetSmb->endpoint()->end_point.c_str() );

      }
#endif

      ocpiAssert( m_finalized );
      return new XferRequest( this );
    }


    XferServices::
    ~XferServices ()
    {
      if ( ibv_destroy_qp( m_qp ) ) {
	fprintf(stderr, "OFED :XferServies ERROR: failed to destroy queue pair\n");
      }
    }

    SmemServices::
    SmemServices (DataTransfer::XferFactory *f, DataTransfer::EndPoint* ep)
      :DataTransfer::SmemServices(f, ep),m_ofed_ep(static_cast<EndPoint*>(ep))
    {

      if ( m_ofed_ep->local ) {
	m_mem = new char[m_ofed_ep->size];
	memset( m_mem, 0, m_ofed_ep->size );
	int page_size = sysconf(_SC_PAGESIZE);
	m_mem = (char*)memalign(page_size, m_ofed_ep->size );
	if (!m_mem) {
	  fprintf(stderr, "OFED::SmemServices Error: Couldn't allocate SMB.\n");
	  throw DataTransfer::DataTransferEx( NO_MORE_SMB, "memalign failed" );
	}
	memset(m_mem, 0, m_ofed_ep->size );
	m_ofed_ep->m_device->createMemContext( this );

	// Now that the memory has been registered, we can finalize our endpoint.
	EndPoint * ep = static_cast<EndPoint*>(endpoint());
	ep->m_vaddr = (uint64_t)m_mem;
	MASK_ADDR( ep->m_vaddr );
	ep->finalize();
      }
      else {
	m_mem = (char*)static_cast<EndPoint*>(ep)->m_vaddr;
      }
    };

    SmemServices::
    ~SmemServices ()
    {
      m_ofed_ep->m_device->destroyMemContext( this );
      delete [] m_mem;
    }
  }
}

// Used to register with the data transfer system;
DataTransfer::OFED::XferFactory *g_ofed_singlton_factory = new DataTransfer::OFED::XferFactory;
#endif


