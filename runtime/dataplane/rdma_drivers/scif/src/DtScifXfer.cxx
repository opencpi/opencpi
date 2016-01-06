/*
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
 * SCIF DMA transfer driver, which is built on the PIO XferServices.
 *
 * Endpoint format is:
 * <EPNAME>:.<node>.<port>
 *    Where the node defines either the host (0) or 1 of N xeon Phi processors
 *
 * All values are decimal
 */

#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include <OcpiThread.h>
#include <OcpiRes.h>
#include "KernelDriver.h"
#include "OcpiOsDebug.h"
#include "OcpiUtilMisc.h"
// We build this transfer driver using the PIO::XferServices
#include "DtPioXfer.h"

#include <xfer_if.h>
#include <xfer_internal.h>

#include <scif.h>
#include <memory.h>
//#include <malloc.h>
#include <pthread.h>
#include <string.h>

#include <map>
#include <list>

#define BACKLOG 10
//#define EB_SIZE 128
//static char eb[EB_SIZE];

#define VA_GEN_MIN 0x4000000000000000
#define SCIF_FLAG_MEM_SIZE 2 * 0x1000
//#define VA_GEN_MIN 0x80000

namespace OU = OCPI::Util;
namespace DT = DataTransfer;
namespace OCPI {
  namespace SCIF {

    enum EPTYPE {
      ACCEPT,
      CONNECT
    };

#define EPNAME "ocpi-scif-dma"

    class XferFactory;
    class Device : public DT::DeviceBase<XferFactory,Device> {
      Device(const char *name)
	: DataTransfer::DeviceBase<XferFactory,Device>(name, *this) {}
    };

    class SmemServices;
    static int next_ep_port=0;
    class EndPoint : public DT::EndPoint {
      friend class SmemServices;
      friend class XferServices;
      friend class AcceptT;
    protected:
      struct scif_portID m_portID;
    public:
      EndPoint( std::string& ep, bool local, uint16_t/* node*/, uint16_t /*port*/ )
        : DT::EndPoint(ep, 0, local)
      {

	if (sscanf(ep.c_str(), EPNAME ":%" SCNx16 ".%" SCNx16 "", &m_portID.node,
		   &m_portID.port ) != 2)
	  throw OU::Error("Invalid format for SCIF DMA endpoint: %s", ep.c_str());

	//	printf("******************* PORT = %d\n", m_portID.port  );
  
	ocpiDebug("SCIF DMA ep %p %s: size = 0x%zx port = %d ",
		  this, ep.c_str(), size, m_portID.port);
      };
      virtual ~EndPoint() {}
      uint32_t port(){return m_portID.port;}
      uint32_t node(){return m_portID.node;}

      DT::SmemServices & createSmemServices();

      // Get the address from the endpoint
      // FIXME: make this get address thing NOT generic...
      virtual const char* getAddress() {
	return 0;
      }
    };


    /**********************************
     * This is the Programmed I/O transfer request class
     *********************************/
    class XferServices;
    class XferRequest : public DataTransfer::TransferBase<XferServices, XferRequest>
      {
	OCPI::Util::ResAddr m_addr;
	//	uint64_t * m_df;
      public:
	// Constructor
	XferRequest(XferServices & parent, XF_template temp);
	void action_transfer(PIO_transfer transfer, bool last );

	virtual CompletionStatus  getStatus ();

	// Destructor - implementation at end of file
	virtual ~XferRequest();

    };


    class AcceptT : public OCPI::Util::Thread {
      SmemServices & m_smb;
    public:
      AcceptT( SmemServices & smb )
	: m_smb(smb)
      {
	
      }
      void run();
    };

    class XferServices :
      public DT::ConnectionBase<XferFactory,XferServices,XferRequest>
      {
	// So the destructor can invoke "remove"
	friend class XferRequest;
	XferFactory &m_driver;
	bool m_init;

      public:
	XferServices(DT::SmemServices* source, DT::SmemServices* target);
	virtual ~XferServices(){};
	DataTransfer::XferRequest* createXferRequest();
	SmemServices & srcSmb(){return *m_sourceSmb;}
	SmemServices & tgtSmb(){return *m_targetSmb;}

      protected:
	// Create transfer services template
	void createTemplate (DataTransfer::SmemServices* p1, DataTransfer::SmemServices* p2);
	void createTemplateDeferred();

      private:
	// The handle returned by xfer_create
	XF_template m_xftemplate;

	// Our transfer request
	DataTransfer::XferRequest* m_txRequest;

	// Source SMB services pointer
	SmemServices* m_sourceSmb;

	// Target SMB services pointer
	SmemServices* m_targetSmb;

      };


    const char *scif = "scif"; // name passed to inherited template class
    

    class XferFactory :
      public DT::DriverBase<XferFactory, Device, XferServices, scif> {
      friend class SmemServices;
      friend class XferServices;
      uint16_t   m_node;
      uint16_t   m_port;
      //      unsigned  m_maxMBox, m_perMBox;
    public:
      XferFactory()
      //	: m_maxMBox(0)
      {
	const char* env = getenv("OCPI_SCIF_NODE");
	if ( ! env ) {
	  throw OU::Error("OCPI_SCIF_NODE environment variable must be set");
	}
	m_node = (uint16_t)atoi(env);	
	env = getenv("OCPI_SCIF_PORT");
	if ( ! env ) {
	  throw OU::Error("OCPI_SCIF_PORT environment variable must be set");
	}
	m_port = (uint16_t)atoi(env);	

	//	printf("******** ^^^^^^^^^^^^^^ IN scif FACTORY node = %d, port = %d!!\n", m_node, m_port );
      }

      virtual
      ~XferFactory() {
       
      }
      int getNode(){return m_node;}
      int getPort(){return m_port;}
            
    protected:
      static std::list<XferServices*> m_services;

    public:
      const char *
      getProtocol() { return EPNAME; }

      DT::XferServices *
      getXferServices(DT::SmemServices* source, DT::SmemServices* target);
      
      DT::EndPoint *
      createEndPoint(std::string& endpoint, bool local);

      std::string 
      allocateEndpoint(const OU::PValue*, uint16_t mailBox, uint16_t maxMailBoxes) {
	std::string ep;

	OCPI::Util::formatString(ep, EPNAME ":%d.%d;%zu.%" PRIu16 ".%" PRIu16, m_node, m_port + next_ep_port,
				 m_SMBSize, mailBox, maxMailBoxes);

	next_ep_port +=2;

	//	printf("&&&&& returning ep = %s\n", ep.c_str() );
	
	return ep;
      }
    };
    std::list<XferServices*> XferFactory::m_services;


    DT::EndPoint* XferFactory::
    createEndPoint(std::string& endpoint, bool local) {
      return new EndPoint(endpoint, local, m_node, m_port);
    }


    
    class SmemServices : public DT::SmemServices , public OCPI::Util::MemBlockMgr  {
      friend class EndPoint;
      friend class XferServices;
      friend class AcceptT;
      EndPoint    &m_ep;
      scif_epd_t   m_epn[2];
      uint8_t     *m_vaddr;
      int          m_conn_port;
      XferFactory &m_driver;
      scif_epd_t   m_connected_ep[10];
      bool         m_accept;
      AcceptT      m_accepter;


    protected:
      SmemServices(EndPoint& ep) 
	: DT::SmemServices(ep), 
	  MemBlockMgr(OCPI_UTRUNCATE(OU::ResAddr, ep.size), SCIF_FLAG_MEM_SIZE),
	  m_ep(ep), m_vaddr(NULL), m_conn_port(-1),
	  m_driver(XferFactory::getSingleton()),
	  m_accept(true),
	  m_accepter(*this)
	  
      {

	//	printf("&&&&&&&&& In SmemServices \n");

	/* We can open the endpoint for this node early, only one per factory */
	if ((m_epn[ACCEPT] = scif_open()) == SCIF_OPEN_FAILED)
	  throw OU::Error("scif_open failed with error %s\n",  strerror(errno));
	/* We can open the endpoint for this node early, only one per factory */
	if ((m_epn[CONNECT] = scif_open()) == SCIF_OPEN_FAILED)
	  throw OU::Error("scif_open failed with error %s\n",  strerror(errno));

	if ( ep.local ) 
	  printf("Local SCIF smemservices created \n");
	else
	  printf("Remote SCIF smemservices created \n");

      }

      

    public:
      bool accept(){return m_accept;}
      bool operator ==(const SmemServices &b) const
      {
	if ( (m_ep.node() == b.m_ep.node()) && (m_ep.port() == b.m_ep.port())) {
	  return true;
	}
	return false;
      }
      EndPoint & ep(){return m_ep;}
      scif_epd_t & epn(int n){return m_epn[n];}
      scif_epd_t & con_epn(){return m_connected_ep[0];}
      virtual ~SmemServices () {
	m_accept = false;
	scif_unregister( m_epn[CONNECT], VA_GEN_MIN, (( ep().size + 0x1000 -1) & ~0xfff) + SCIF_FLAG_MEM_SIZE );
	scif_close( m_epn[ACCEPT] );
	scif_close( m_epn[CONNECT] );
	::free( m_vaddr );
      }

      void* map(DtOsDataTypes::Offset offset , size_t /* size */ ) {
	OU::SelfAutoMutex guard (&m_driver);


	//	printf("&&&&&&& In map !! for %s, offset = %d\n", m_ep.local ? "true" : "false", offset );

	if (m_ep.local && (m_vaddr==NULL)) {

	  int err = posix_memalign((void**)&m_vaddr, 0x1000, m_ep.size + SCIF_FLAG_MEM_SIZE );
	  if (m_vaddr == NULL || err != 0 )
	    throw OU::Error("_aligned_malloc failed with error : %s\n",  strerror(errno));

	  //	  printf("TTTTTTTT About to bind using %d\n", m_ep.port() );

	  /* bind end point to available port, generated dynamically */
	  if ((m_conn_port = scif_bind(m_epn[ACCEPT], (uint16_t)m_ep.port())) < 0)
	    throw OU::Error("scif_bind failed with error %s\n",  strerror(errno));
	  //	  printf("bind success to port %d\n", m_conn_port);

	  /* bind end point to available port, generated dynamically */
	  if ((m_conn_port = scif_bind(m_epn[CONNECT], (uint16_t)m_ep.port()+20 )) < 0)
	    throw OU::Error("scif_bind failed with error %s\n",  strerror(errno));
	  //	  printf("bind success to port %d\n", m_conn_port);

	  m_accepter.start();

	}


	if ( !m_ep.local) {
	  //	  printf("SCIF SmemServices remote; WHY ARE WE MAPPING REMOTE ??????????\n");
	}

	return (void*)&m_vaddr[offset];
      }
    };


    static std::map<std::string,DT::SmemServices*> dict;
    DT::SmemServices & EndPoint::
    createSmemServices() {      
      SmemServices * s;
      s = static_cast<SmemServices*>(dict[end_point]);
      if ( s == NULL ) {
	s = new SmemServices(*this);
	dict[end_point] = s;
      }
      return *s;
    }


    void 
    XferServices::
    createTemplateDeferred()
    {

      if ( m_init ) 
	return;

      //      printf("In createTemplateDeferred() !!\n");
      m_init = true;

      // Our strategy here is simple, if we are the higher numbered port we are the connector 
      //      if (  m_sourceSmb->ep().port() <   m_targetSmb->ep().port() ) {
      //      if ( m_sourceSmb->m_connected_ep != -1 ) {

      if ( 1 )  {

	printf("**** About to connect\n");

	int retries=10;
	while( retries ) {
	  int err;
	  try {
	    printf("**** Trying to connect to %d\n", m_targetSmb->ep().m_portID.port );
	    if ((err = scif_connect(m_sourceSmb->m_epn[CONNECT], &m_targetSmb->ep().m_portID)) < 0) {
	      printf("* AFTER connected with error = %d, errno = %s\n", err, strerror(errno));
	      if ((errno == ECONNREFUSED) && (retries > 0)) {
		printf("connection to node %d failed : trial %d\n", m_targetSmb->ep().m_portID.node, retries);
	      }
	      retries--;
	      sleep(1);
	      continue;
	    }
	    else {
	      //	      printf("@@@@@@@@@@@@@  conect to node %d success\n", m_targetSmb->ep().m_portID.node);
	      m_sourceSmb->m_connected_ep[0] = m_sourceSmb->m_epn[CONNECT];
	      sleep(2);
	      break;

	    }
	  }
	  catch ( ... ) {
	    retries--;
	    sleep(1);
	  }
	}
	if ( retries <= 0 )
	  throw OU::Error("scif_connect failed with error %d\n", errno);
      }


      /* scif_register : marks a memory region for remote access */
      long offset;
      long size = ((m_sourceSmb->ep().size + 0x1000 -1) & ~0xfff) + SCIF_FLAG_MEM_SIZE;
      if ((offset = scif_register (m_sourceSmb->m_epn[CONNECT],
				   m_sourceSmb->m_vaddr,
				   size,
				   VA_GEN_MIN,
				   SCIF_PROT_READ | SCIF_PROT_WRITE,
				   /* SCIF_MAP_FIXED */0 )) < 0) {

	printf("scif_register FAILED !!  offset= 0x%lx, vaddr = %p, size = %ld\n",offset, m_sourceSmb->m_vaddr, size );

	throw OU::Error("scif_register failed with error : %s\n",  strerror(errno));
      }

      //      printf("$$$$$$$$$$$$$$$$$$$$$$  scif_register!!  offset= 0x%lx, vaddr = %p, size = %ld\n",offset, m_sourceSmb->m_vaddr, size );


      //      printf("* &&&&&&&& SUCCESS  connecting\n");
    }


    void 
    XferServices::
    createTemplate (DT::SmemServices* source, DT::SmemServices* target)
    {


      //      printf("@@@@@@@@@@@ In createTemplate\n");

      SmemServices * s = static_cast<SmemServices*>(source);
      SmemServices * t = static_cast<SmemServices*>(target);
      m_sourceSmb = s;
      m_targetSmb = t;
      m_txRequest = NULL;
      xfer_create (s, t, 0, &m_xftemplate);

    }


    DataTransfer::XferRequest* 
    XferServices::
    createXferRequest()
    {
      OCPI::Util::SelfAutoMutex guard (&XferFactory::getSingleton());
      //      printf("******* In createXferRequest()\n");

      pthread_t         tid;
      tid = pthread_self();
      //      printf("THREAD ID = %d, this = %p\n", tid, this );

      createTemplateDeferred();
      XferRequest * xf = new XferRequest( *this, m_xftemplate );
      return xf;

    }

    void
    AcceptT::
    run ()
    {
	
      /* scif_accept : accepts connection requests on listening end pt and creates a
       * new end pt which connects to the peer end pt that initiated connection,
       * when successful returns 0.
       * SCIF_ACCEPT_SYNC blocks the call untill a connection is present
       */
      if (scif_listen(m_smb.m_epn[ACCEPT], BACKLOG) != 0)
	throw OU::Error("scif_listen failed with error %s\n",  strerror(errno));

      int i = 0;
      while( m_smb.accept() ) {

	//	printf("****** &&&&&&&&&&&& ^^^^^^^^^^ About to scif accept on node %d, port %d!!\n", m_smb.ep().m_portID.node, m_smb.ep().m_portID.port  );

	//	assert( m_smb.m_connected_ep == -1 );
	if (scif_accept(m_smb.m_epn[ACCEPT], &m_smb.ep().m_portID, &m_smb.m_connected_ep[i], SCIF_ACCEPT_SYNC) != 0) {
	  //	  printf("scif_accept failed with error %s\n",  strerror(errno) );
	}
	else {
	  printf("@@@@@@@@@@@@@@@@@  accepted connection request from node:%d port:%d\n", m_smb.ep().m_portID.node, m_smb.ep().m_portID.port);

	  /* scif_register : marks a memory region for remote access */
	  long offset;
	  long size = (m_smb.ep().size + 0x1000 -1) & ~0xfff;
	  if ((offset = scif_register (m_smb.m_connected_ep[i],
				       m_smb.m_vaddr,
				       size,
				       VA_GEN_MIN,
				       SCIF_PROT_READ | SCIF_PROT_WRITE,
				       /* SCIF_MAP_FIXED */ 0)) < 0) {

	    printf("scif_register FAILED !!  offset= 0x%lx, vaddr = %p, size = %ld\n",offset, m_smb.m_vaddr, size );

	    throw OU::Error("scif_register failed with error : %s\n",  strerror(errno));
	  }

	  printf("$$$$$$$$$$$$$$$$$$$$$$  scif_register!!  offset= 0x%lx, vaddr = %p, size = %ld\n",offset, m_smb.m_vaddr, size );

	}
	i++;
      }

    }


    XferServices::
    XferServices(DT::SmemServices* source, DT::SmemServices* target)
      : DataTransfer::ConnectionBase<XferFactory,XferServices,XferRequest>(*this, source,target),
	m_driver(XferFactory::getSingleton()),m_init(false)
    {
      createTemplate (source, target);
    }


    DT::XferServices *
    XferFactory::
    getXferServices(DT::SmemServices* source, DT::SmemServices* target) {
      SmemServices * s = static_cast<SmemServices*>(source);
      SmemServices * t = static_cast<SmemServices*>(target);
      std::list<XferServices*>::iterator it;
      for ( it = m_services.begin(); it != m_services.end(); it++ ) {
	if ( ( (*it)->srcSmb() == *s) && ((*it)->tgtSmb() == *t)) {
	  printf("&&&&&&&& Already have a transfer Service !!\n");
	  return (*it);
	}
      }
      return new XferServices(source, target);
    }


    XferRequest::
    XferRequest(XferServices & parent, XF_template temp)
    : DT::TransferBase<XferServices,XferRequest>(parent, *this, temp )
    {
      //      printf("****** &&&&& ^^^^^^ In XferRequest()\n");
      if (  parent.m_sourceSmb->alloc( 4, 8, m_addr) < 0  ) {
	throw OU::Error("SCIF::XferRequest: out of flag memory !!");
      }
      //      printf("flag memory = %d\n", m_addr);
    }

    XferRequest::
    ~XferRequest() {
      parent().m_sourceSmb->free( m_addr);
    }


    DataTransfer::XferRequest::CompletionStatus  
    XferRequest::
    getStatus () {

      //      printf("&&&&&&&&& ******* Status =%d\n", *((int*)parent().m_sourceSmb->map((DtOsDataTypes::Offset)m_addr,4)) );
      if (  *((int*)parent().m_sourceSmb->map((DtOsDataTypes::Offset)m_addr,4)) ) {
	return DataTransfer::XferRequest::CompleteSuccess; 
      }
      return DataTransfer::XferRequest::Pending;
    }






    void
    XferRequest::
    action_transfer(PIO_transfer transfer, bool last ) {

      //      printf("****** &&&&& ^^^^^^ In action_transfer()\n");

#define TRACE_XFERS  
#ifdef TRACE_XFERS
      printf("scif: copying %zu bytes from 0x%llx to 0x%llx",
	     transfer->nbytes, (unsigned long long)transfer->src_off,
	     (unsigned long long)transfer->dst_off);
#endif

      int ret;
      if ( last ) {
	//	printf("***** LAST TX !!\n");	
	*((int*)parent().m_sourceSmb->map((DtOsDataTypes::Offset)m_addr,4)) = 0;
	uint64_t v =  *((int*)parent().m_sourceSmb->map((DtOsDataTypes::Offset)transfer->src_off,4)) |
	  *((int*)parent().m_sourceSmb->map((DtOsDataTypes::Offset)transfer->src_off,4)) << 31;
	if ( (ret=scif_fence_signal( parent().m_sourceSmb->epn(CONNECT), VA_GEN_MIN + m_addr, 1, VA_GEN_MIN + transfer->dst_off, 
				     v,
				     SCIF_FENCE_INIT_SELF | SCIF_SIGNAL_LOCAL | SCIF_SIGNAL_REMOTE ) ) )
	  throw OU::Error("scif_fence_signal failed with err = %d,error %s\n", ret,strerror(errno));
      }
      else{
	if ( (ret=scif_writeto ( parent().m_sourceSmb->epn(CONNECT), VA_GEN_MIN + transfer->src_off, transfer->nbytes,
				 VA_GEN_MIN + transfer->dst_off, 0 /* | SCIF_RMA_USECPU |  SCIF_RMA_SYNC */)) <0 )
	  throw OU::Error("scif_writeto failed with err = %d,error %s\n", ret,strerror(errno));
      }
    }

    DT::RegisterTransferDriver<XferFactory> driver;

  }
}
