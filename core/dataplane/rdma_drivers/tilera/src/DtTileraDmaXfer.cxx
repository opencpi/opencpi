#ifdef TILERA_RDMA_SUPPORT
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
 * DMA transfer driver, which is built on the PIO XferServices.
 * The "hole" in the address space is a cheap way of having a
 * segmented address space with 2 regions...
 * FIXME: support endpoints with regions
 *
 * Endpoint format is:
 * <EPNAME>:<address>.<holeOffset>.<holeEnd>
 *
 * All values are hex.
 * If holeOffset is zero there is no hole
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <list>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include "KernelDriver.h"
#include "OcpiOsDebug.h"
#include "OcpiUtilMisc.h"
// We build this transfer driver using the PIO::XferServices
#include "DtPioXfer.h"





#ifdef __tile__
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <gxio/trio.h>
#include <gxpci/gxpci.h>

#include <tmc/alloc.h>
#include <tmc/cpus.h>
#include <tmc/perf.h>
#include <tmc/sync.h>
#include <tmc/task.h>

/*
#include <gxio/trio.h>
//#include <gxpci/gxpci.h>
#include <tmc/cpus.h>
#include <tmc/alloc.h>
#include <tmc/task.h>
#include <tmc/sync.h>
// #include <arch/cycle.h>
#include <tmc/perf.h>
*/



#include "OcpiThread.h"
#else
#include <asm/tilegxpci.h>
#endif

#define MAP_LENGTH 	(16 * 1024 * 1024)
#define EPNAME          "ocpi-tilera-rdma"
#define EBSIZE 256
#define ERRBUF char errbuf[EBSIZE];

namespace OU = OCPI::Util;
namespace DT = DataTransfer;
namespace OCPI {
  namespace TILERA {

    class EndPoint;
    class SmemServices;
    static const int G_EP_CNT=1;
    static int g_ep_cnt = 0;
    static std::vector<EndPoint*> g_ep;


    struct Req {
      DtOsDataTypes::Offset srcoff;
      DtOsDataTypes::Offset dstoff;
      size_t                nbytes;
      uint32_t              flags;
    };

    class XferFactory;
    class Device : public DT::DeviceBase<XferFactory,Device> {
      Device(const char *name)
	: DataTransfer::DeviceBase<XferFactory,Device>(name, *this) {}
    };

    class EndPoint : public DT::EndPoint {
      friend class SmemServices;
      SmemServices * m_smem;
    protected:
      uint32_t m_holeOffset, m_holeEnd;
    public:
      EndPoint( std::string& ep, bool local)
        : DT::EndPoint(ep, 0, local),m_smem(NULL) {
	if (sscanf(ep.c_str(), EPNAME ":%" SCNx64 ".%" SCNx32 ".%" SCNx32 ";",
		   &address, &m_holeOffset, &m_holeEnd) != 3)
	  throw OU::Error("Invalid format for DMA endpoint: %s", ep.c_str());
  
	ocpiDebug("DMA ep %p %s: address = 0x%" PRIx64
		  " size = 0x%zx hole 0x%" PRIx32 " end 0x%" PRIx32,
		  this, ep.c_str(), address, size, m_holeOffset, m_holeEnd);
	if ( local ) { 
	    createSmemServices();
	}
      };
      virtual ~EndPoint() {}

      DT::SmemServices & createSmemServices();

      // Get the address from the endpoint
      // FIXME: make this get address thing NOT generic...
      virtual const char* getAddress() {
	return 0;
      }
    };

    const char *dma = "tilera-dma"; // name passed to inherited template class




#ifdef __tile__

    class XferServices;
    class XferRequest : public DT::XferRequest {
      std::list<Req*> m_XferReqs;
      XferServices * m_XferServices;
      friend class SmemServices;
    public:
      XferRequest( XferServices * xserv )
	: m_XferServices(xserv){}
      virtual void post ();

      virtual CompletionStatus  getStatus (){return CompleteSuccess;}
      virtual void modify( DtOsDataTypes::Offset [],
			   DtOsDataTypes::Offset [] )
      {
	
	throw std::string("Not implemeted");
      }
      virtual XferRequest* copy (DtOsDataTypes::Offset srcoff, 
				 DtOsDataTypes::Offset dstoff, 
				 size_t nbytes, 
				 XferRequest::Flags flags
				 )
      {
	Req * r = new Req;
	r->srcoff = srcoff;
	r->dstoff = dstoff;
	r->nbytes = nbytes;
	r->flags = flags;
	m_XferReqs.push_back( r );
	return this;
      }

      virtual XferRequest & group( XferRequest* rhs ) {
	std::list< Req* >::const_iterator it;
	for ( it = rhs->m_XferReqs.begin(); it != rhs->m_XferReqs.end(); it++ ) {
	  m_XferReqs.push_back( (*it) );
	}
	return *this;
      }
    };



    class PullReqHandler;
    class SmemServices : public DT::SmemServices {

      struct TR {
	int m_trio_index;
	int m_trio_asid;
	gxio_trio_context_t m_trio_context;
	gxpci_raw_dma_state_t m_rd_state;
	gxpci_context_t m_gxpci_send_context;
	gxpci_context_t m_gxpci_recv_context;
	int m_queue_index;
	uint8_t *m_vaddr;       // memory pool.
	PullReqHandler * m_prh;
	uint32_t m_host_buf_size;
	volatile bool m_init;
	int m_qindex;
	TR(){m_init=false;}
      } m_tr;

      class ResInit : public OCPI::Util::Thread {      
	TR & m_tr;
      public:	
	ResInit( TR & tr )
	  : m_tr(tr) {}
	void run();
      };	

      ResInit * m_init;
      EndPoint &m_dmaEndPoint;
      XferFactory &m_driver;

      friend class EndPoint;
    public:
      void join() {
	if ( m_init ) {
	  while ( ! m_tr.m_init ) {
	    sleep(1);
	  }
	}
      }
      void post(XferRequest & r );
      SmemServices(EndPoint& ep);
      virtual ~SmemServices ();
      int32_t attach(DT::EndPoint*) { return 0; }
      int32_t detach() { return 0; }
      int32_t unMap() { return 0; }
      void* map(DtOsDataTypes::Offset offset, size_t size );

      void* mapTx(DtOsDataTypes::Offset offset, size_t size )
      {
	return map(offset,size);
      }
      void* mapRx(DtOsDataTypes::Offset offset, size_t size )
      {
	return map(offset,size);
      }

    };


    class PullReqHandler : public OCPI::Util::Thread {      
      static const int SEND = 0;
      static const int RECV = 1;
      int m_fd[2];
      uint8_t * m_buffer[2];
      volatile bool m_terminate;
      gxpci_context_t m_gxpci_recv_context;
      uint8_t * m_vaddr;
    public:
      virtual ~PullReqHandler() {
	close( m_fd[RECV] );
	free( m_buffer[RECV] );
	close( m_fd[SEND] );
	free( m_buffer[SEND] );
      }
      PullReqHandler(int qindex, gxpci_context_t context, uint8_t* vaddr) 
	: m_terminate(false), m_gxpci_recv_context(context),m_vaddr(vaddr)
      {

	// Open the tile side of a host-to-tile zero-copy channel.
	const char* path = "/dev/trio0-mac0/h2t/%d";
	char name[128];
	sprintf(name, path, qindex); 
	m_fd[RECV] = open(name, O_RDWR);
	if (m_fd[RECV] < 0)
	  {
	    ERRBUF;
	    snprintf(errbuf, EBSIZE, "Failed to open '%s' \n", name ); fprintf(stderr,errbuf);
	    throw std::string(errbuf);
	  }
	
        // Open the tile side of a tile-to-host zero-copy channel.					 
	path = "/dev/trio0-mac0/t2h/%d";								 
	sprintf(name, path, qindex); 								 
	m_fd[SEND] = open(name, O_RDWR);							      
	if (m_fd[SEND] < 0)									
	  {											
	    ERRBUF;										
	    snprintf(errbuf, EBSIZE, "Failed to open '%s' \n", name ); fprintf(stderr,errbuf);	
	    throw std::string(errbuf);								
	  }											
        

	// On the tile side, packet buffers can come from any memory region,
	// but they must not cross a page boundary.
	m_buffer[RECV] = (uint8_t*)memalign(getpagesize(), 4096);
	assert(m_buffer[RECV] != NULL);
	m_buffer[SEND] = (uint8_t*)memalign(getpagesize(), 4096);
	assert(m_buffer[SEND] != NULL);

	// Register this buffer to TRIO.
	tilegxpci_buf_info_t buf_info;
	buf_info.va = reinterpret_cast<uintptr_t>(m_buffer[RECV]);
	buf_info.size = getpagesize();
	int result = ioctl(m_fd[RECV], TILEPCI_IOC_REG_BUF, &buf_info);
	assert(result == 0);
	
        buf_info.va = reinterpret_cast<uintptr_t>(m_buffer[SEND]);
	buf_info.size = getpagesize();				    
	result = ioctl(m_fd[SEND], TILEPCI_IOC_REG_BUF, &buf_info);
	assert(result == 0);					
        

	// Flow control:  We will get things started by providing a flow control token to the host
	tilepci_xfer_req_t send_cmd;
	m_buffer[SEND][0] = 1;
	send_cmd.addr = (uintptr_t) m_buffer[SEND];
	send_cmd.len = 4;
	send_cmd.cookie = 0xfeedbacc;


	//	sleep(2);
	while (write(m_fd[SEND], &send_cmd, sizeof(send_cmd)) != sizeof(send_cmd))
	  fprintf(stderr, "write() failed: %s\n", strerror(errno));

      }
      void stop(){m_terminate=true;}
      void run() {

	// Given that we're the tile side of a host-to-tile channel, we can
	// construct a receive command and post it to the PCI subsystem.
	tilepci_xfer_req_t recv_cmd;
	recv_cmd.addr = reinterpret_cast<uintptr_t>(m_buffer[RECV]);
	recv_cmd.len = 1024;
	recv_cmd.cookie = 0;
	
	while(!m_terminate) {


	  // Initiate the command.
	  while (write(m_fd[RECV], &recv_cmd, sizeof(recv_cmd)) != sizeof(recv_cmd))
	    fprintf(stderr, "write() failed: %s\n", strerror(errno));


	  tilepci_xfer_comp_t comp;
	  printf("Reading back completions !!\n");
	  // Read back the completion.
	  while (read(m_fd[RECV], &comp, sizeof(comp)) != sizeof(comp))
	    fprintf(stderr, "read() failed: %s\n", strerror(errno));


	  // Print the received data, including the metadata 'cookie' that
	  // came along with the data.
	  printf("Received %d bytes: cookie = %#lx, words[0] = %#x\n",
		 comp.len, (unsigned long)comp.cookie, 
		 ((uint32_t*)(uintptr_t)comp.addr)[0]);

	  
	  Req * r = (Req*)comp.addr;
	  size_t count = r->nbytes;
	  assert( count  * sizeof(Req) < 4096 );
	  gxpci_dma_cmd_t cmd;
	  r++;
	  for ( unsigned i=0; i<count; i++  ){

	    // This loop does not wait until the transfers are complete, but will block until all transfers are posted
	    unsigned credits = gxpci_raw_dma_recv_get_credits(&m_gxpci_recv_context);
	    if ( credits == 0 ) {
	      sleep(0);
	      i--;
	      continue;
	    }

	    cmd.buffer = m_vaddr + r->dstoff;
	    cmd.remote_buf_offset = r->srcoff;
	    cmd.size = r->nbytes;

	    //////////////////////////////////////////////////////////////////////
            // uint32_t * debp = (uint32_t*)cmd.buffer;			        //
	    // *debp = 0;						        //
	    // printf("dest pointer before dma request = %d\n", *debp );        //
            //////////////////////////////////////////////////////////////////////
	      
	    printf("Requesting %d bytes, remote off = %d, local off = %d\n",  cmd.size, r->srcoff,  r->dstoff );
	    r++;

	      
	    // Make data visible to TRIO.
	    __insn_mf();

	    //	    usleep( 40 );

	    int result = gxpci_raw_dma_recv_cmd(&m_gxpci_recv_context, &cmd);
	    if (result == GXPCI_ERESET)
	      {
		// FIXME!! - what do we do here if the other end hangs up
		gxpci_raw_dma_recv_destroy( &m_gxpci_recv_context );
		assert( 0 );
		
	      }
	    if ( result != 0 ) {
	      throw std::string("call to gxpci_raw_dma_send_cmd failed !!");
	    }

	    // FIXME !! - may not need this
	    //	    gxpci_comp_t comp;
	    //	    int comp_count = gxpci_raw_dma_recv_get_comps(&m_gxpci_recv_context, &comp, 0, 1);
	    //	    int hcount = gxpci_raw_dma_get_host_counter (&m_gxpci_recv_context);
	    
	  }


	  //	  printf("About to send Host a completion credit\n");
	  
	  // Flow control
	  tilepci_xfer_req_t send_cmd;
	  m_buffer[SEND][0] = 1;
	  send_cmd.addr = (uintptr_t) m_buffer[SEND];
	  send_cmd.len = 4;
	  send_cmd.cookie = 0xfeedbacc;
	  while (write(m_fd[SEND], &send_cmd, sizeof(send_cmd)) != sizeof(send_cmd))
	    fprintf(stderr, "write() failed: %s\n", strerror(errno));

	  // Read back the completion.
	  //	  printf("About to read from FB\n");
	  while (read(m_fd[SEND], &comp, sizeof(comp)) != sizeof(comp))
	    fprintf(stderr, "read() failed: %s\n", strerror(errno));

	  printf("Done with FB\n");

	}

	printf(" ****** TERMINATING PULL THREAD !!!! *********\n");



      }
    };


    class XferServices : public DT::XferServices {
    public:
      SmemServices * m_source;
      SmemServices * m_target;
      //      PullReqHandler * m_prh;
      XferServices ( DT::SmemServices * s , DT::SmemServices * t )
	: DT::XferServices(s,t), m_source(reinterpret_cast<SmemServices*>(s)),
	  m_target(reinterpret_cast<SmemServices*>(t))
      {

      }
      void post( XferRequest & req );
      virtual XferRequest* createXferRequest() {return new XferRequest(this);}
      virtual ~XferServices () 
      {

	/*
	try {
	  m_prh->stop();
	  //	  m_prh->cancel();
	  m_prh->join();
	  delete m_prh;
	}
	catch( ... ) {
	  // ignore
	}
	*/

      }
    };


    class XferFactory :
      public DT::DriverBase<XferFactory, Device, XferServices, dma> {
      friend class SmemServices;
      int       m_init; // if this is >= 0, then we have been there before
      SmemServices * m_smem;
    public:
      XferFactory()
	: m_init(false)
      {
	printf("In tilegx xfer factory !!\n");
      }


      
      void configure(ezxml_t x) {

	printf("****  In XFER configure , pre-creating %d endpoints\n", G_EP_CNT);

	for ( int n=0; n<G_EP_CNT; n++ ) {
	  std::string ep = allocateEndpoint(NULL, n, G_EP_CNT);
	  EndPoint * endpoint  = new EndPoint(ep, true );
	  g_ep.push_back( endpoint );
	}

      }



      virtual
      ~XferFactory() {
	lock();
	if (m_init == true ){

	}
      }
    private:

    protected:
      // Getting a memory region happens on demand, but releasing them
      // only happens on shutdown
      void
      getDmaRegion(EndPoint &ep) {
	OU::SelfAutoMutex guard(this);
	if (m_init == false ){
	}
      }

      uint8_t *
      mapDmaRegion(EndPoint &ep, uint32_t offset, size_t /* size */ ) {
	OU::SelfAutoMutex guard(this);
	return (uint8_t*)m_smem->map(offset, ep.size);
      }
    public:
      const char *
      getProtocol() { return EPNAME; }

      DT::XferServices *
      getXferServices(DT::SmemServices* source, DT::SmemServices* target) {
	SmemServices * ss = static_cast<SmemServices*>(source);
	ss->join();
	ss = static_cast<SmemServices*>(target);
	ss->join();

	printf("Creating xfer service \n");
	return new XferServices(source, target);
	printf("Creating xfer service AFTER \n");

      }
      
      DT::EndPoint *
      createEndPoint(std::string& endpoint, bool local);

      // FIXME: provide ref to string as arg
      std::string 
      allocateEndpoint(const OU::PValue*, uint16_t mailBox, uint16_t maxMailBoxes) {
	std::string ep;
	
	OCPI::Util::formatString(ep, EPNAME ":0.0.0;%zu.%" PRIu16 ".%" PRIu16,
				 m_SMBSize, mailBox, maxMailBoxes);

	printf("******* IN allocateEndpoint, ep = %s\n", ep.c_str() );

	return ep;
      }
    };

    SmemServices::
    ~SmemServices() {
      gxpci_raw_dma_recv_destroy( &m_tr.m_gxpci_recv_context );
      gxpci_raw_dma_destroy(&m_tr.m_trio_context, &m_tr.m_rd_state);
      m_tr.m_prh->join();
      delete m_tr.m_prh;
    }


    void 
    SmemServices::
    post(XferRequest & req ) {
      int result;
      gxpci_dma_cmd_t cmd;


      // This loop does not wait until the transfers are complete, but will block until all transfers are posted
      std::list<Req*>::const_iterator it = req.m_XferReqs.begin();
      while ( it !=  req.m_XferReqs.end() ) {
	unsigned credits = gxpci_raw_dma_send_get_credits(&m_tr.m_gxpci_send_context);
	if ( credits == 0 ) {
	  sleep(0);
	}
	while ( credits && (it!=req.m_XferReqs.end()) ) {
	  cmd.buffer = m_tr.m_vaddr + (*it)->srcoff;
	  cmd.remote_buf_offset = (*it)->dstoff;
	  cmd.size = static_cast<uint32_t>((*it)->nbytes);
	  it++;

	  printf("**** src = %p, dst = %d, nbytes = %d\n", cmd.buffer, cmd.remote_buf_offset, cmd.size );

	  // Make data visible to TRIO.
	  __insn_mf();

	  result = gxpci_raw_dma_send_cmd(&m_tr.m_gxpci_send_context, &cmd);
	  if (result == GXPCI_ERESET)
	    {
	      // FIXME!! - what do we do here if the other end hangs up
	      assert( 0 );

	    }
	  if ( result != 0 ) {
	    throw std::string("call to gxpci_raw_dma_send_cmd failed !!");
	  }
	}
      }
    }



    static int g_qindex = 0;
    SmemServices::SmemServices(EndPoint& ep) 
      : DT::SmemServices(ep), 
	m_dmaEndPoint(ep),
	m_driver(XferFactory::getSingleton())
      {
	m_tr.m_qindex = g_qindex++;
	m_init = NULL;
	if ( ep.local ) {
	  printf("Source side !!\n");
	  m_init = new ResInit( m_tr );
	  m_init->start();

	}
	else {
	  printf("Remote side !!\n");
	  return;
	}
      }


    void
    SmemServices::ResInit::
    run() {

	  m_tr.m_trio_index = 0;
	  m_tr.m_trio_asid = 0;
	  m_tr.m_vaddr = NULL;

	  // The local MAC index.
	  int loc_mac = 0;
	  m_tr.m_trio_index = m_tr.m_qindex;

	  //
	  // Initialize TRIO context and get a TRIO asid.
	  //
	  int result = gxio_trio_init( &m_tr.m_trio_context, m_tr.m_trio_index);
	  if (result != 0) {
	    throw OU::Error("Unable to initialize tilera gxio_trio_init()");
	  }
	  m_tr.m_trio_asid = gxio_trio_alloc_asids(&m_tr.m_trio_context, 1, 0, 0);
	  if (m_tr.m_trio_asid < 0) {
	    throw OU::Error("Failure in gxio_trio_alloc_asids(), asid = %d", m_tr.m_trio_asid);
	  }

	  //
	  // Initialize the Raw DMA state structure.
	  //
	  result = gxpci_raw_dma_init(&m_tr.m_trio_context, &m_tr.m_rd_state, m_tr.m_trio_index,
				      loc_mac, m_tr.m_trio_asid);
	  if (result != 0) {
	    throw OU::Error("Unable to initialize tilera gxio_raw_dma_init()");
	  }
	  //num_receivers = m_rd_state.num_rd_h2t_queues;

	  //
	  // It's good practice to reset the ASID value because gxpci_raw_dma_init()
	  // allocates a new ASID if it is passed a GXIO_ASID_NULL.
	  m_tr.m_trio_asid = m_tr.m_rd_state.asid;

	
	  //
	  // Allocate and register data buffers.
	  //
	  tmc_alloc_t alloc = TMC_ALLOC_INIT;
	  tmc_alloc_set_huge(&alloc);
	  void * buf_mem = tmc_alloc_map(&alloc, MAP_LENGTH);
	  assert(buf_mem);
	  result = gxio_trio_register_page(&m_tr.m_trio_context, m_tr.m_trio_asid, buf_mem, 
					   MAP_LENGTH, 0);
	  if (result != 0) {
	    throw OU::Error("Unable to allocate huge dma page");
	  }
	  m_tr.m_vaddr = (uint8_t*)buf_mem;
	  memset( buf_mem, MAP_LENGTH, 0 );



	  int rank = 0;
	  cpu_set_t desired_cpus;

	  if (tmc_cpus_get_my_affinity(&desired_cpus) != 0)
	    tmc_task_die("tmc_cpus_get_my_affinity() failed.");


	  // Bind to the rank'th tile in the cpu set.
	  if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(&desired_cpus, rank)) < 0)
	    tmc_task_die("tmc_cpus_set_my_cpu() failed.");


	  // FIXME: comes from EP
	  m_tr.m_queue_index = 0;
	  printf("My rank = %d\n", rank );
	  result = gxpci_open_raw_dma_send_queue(&m_tr.m_trio_context, &m_tr.m_rd_state,
						 &m_tr.m_gxpci_send_context, rank);
	  if (result != 0) {
	    throw OU::Error("Unable to open dma recv Q");
	  }
	  m_tr.m_host_buf_size = gxpci_raw_dma_get_host_buf_size(&m_tr.m_gxpci_send_context);
	  m_tr.m_gxpci_recv_context = m_tr.m_gxpci_send_context;

	  result = gxpci_open_raw_dma_recv_queue(&m_tr.m_trio_context, &m_tr.m_rd_state,  &m_tr.m_gxpci_recv_context, rank);

	
	  if (result != 0) {
	    throw OU::Error("Unable to open dma recv Q");
	  }

	  // FIXME !!- comes from ep
	  m_tr.m_prh = new PullReqHandler( m_tr.m_qindex, m_tr.m_gxpci_recv_context, m_tr.m_vaddr );
	  m_tr.m_prh->start();
	  //	  m_tr.m_prh->detach();
	  //	  sleep(1);
	  m_tr.m_init = true;
	}



    void 
    XferRequest::
    post () {
      m_XferServices->m_source->post(*this);
    }

    void* 
    SmemServices::
    map(DtOsDataTypes::Offset offset, size_t size ) {

      printf("In map %s, offset = %d, vaddr = %p\n", m_dmaEndPoint.local ? "loal" : "remote",
	     offset, m_tr.m_vaddr);     

      if ( ! m_dmaEndPoint.local ) {
	return NULL;
      }

      while ( ! m_tr.m_init ) {
	sleep(1);
      }



      OU::SelfAutoMutex guard (&m_driver);	
      assert( offset + size < MAP_LENGTH );
      return &m_tr.m_vaddr[offset];
    }


#else  // Host side


    class XferServices;
    class XferRequest : public DT::XferRequest {
      std::list<Req*> m_XferReqs;
      XferServices * m_XferServices;
      friend class SmemServices;
    public:
      XferRequest( XferServices * xserv )
	: m_XferServices(xserv){}
      virtual void post ();

      virtual CompletionStatus  getStatus (){return CompleteSuccess;}
      virtual void modify( DtOsDataTypes::Offset new_offsets[],
			   DtOsDataTypes::Offset old_offsets[] )
      {
	throw std::string("Not implemeted");
      }
      virtual XferRequest* copy (DtOsDataTypes::Offset srcoff, 
				 DtOsDataTypes::Offset dstoff, 
				 size_t nbytes, 
				 XferRequest::Flags flags
				 )
      {
	Req * r = new Req;
	r->srcoff = srcoff;
	r->dstoff = dstoff;
	r->nbytes = nbytes;
	r->flags = flags;
	m_XferReqs.push_back( r );
	return this;
      }

      virtual XferRequest & group( XferRequest* rhs ) {
	std::list< Req* >::const_iterator it;
	for ( it = rhs->m_XferReqs.begin(); it != rhs->m_XferReqs.end(); it++ ) {
	  m_XferReqs.push_back( (*it) );
	}
	return *this;
      }
    };

    class SmemServices : public DT::SmemServices,
			 virtual protected OCPI::Util::SelfMutex
    {

      char dev_name[40];
      int card_index;
      int queue_index;
      int errno;
      unsigned long host_buf_addr;
      unsigned int  host_buf_size;


      EndPoint &m_dmaEndPoint;
      XferFactory &m_driver;

     // Raw DMA resource handles
      struct tlr_rd_status *          m_rd_status[2];
      struct gxpci_host_rd_regs_app * m_rd_regs[2];
      int                             m_rd_fd[2];
      tilepci_raw_dma_get_buf_t       m_rd_buf_info[2];
      uint8_t *                       m_rd_buffer[2];
      int                             m_seq;

      // Request Q handles
      enum {
	SEND=0,
	RECV=1
      };
      int m_rq_fd[2];
      uint8_t * m_rq_buffer[2];

      friend class EndPoint;

      void createReqQ();
      void finalizeBuffer( int idx );
      void createRawDmaBuffer();


    public:
      void post( XferRequest & req );
      SmemServices(EndPoint& ep);
      virtual ~SmemServices ();
      int32_t attach(DT::EndPoint*) { return 0; }
      int32_t detach() { return 0; }
      int32_t unMap() { return 0; }
      void* map(DtOsDataTypes::Offset offset, size_t size );
      void* mapTx(DtOsDataTypes::Offset offset, size_t size );
      void* mapRx(DtOsDataTypes::Offset offset, size_t size );

    };


    class XferServices : public DT::XferServices {
    public:
      SmemServices * m_source;
      SmemServices * m_target;
      XferServices ( DT::SmemServices * s , DT::SmemServices * t )
	: DT::XferServices(s,t), m_source(reinterpret_cast<SmemServices*>(s)),
	  m_target(reinterpret_cast<SmemServices*>(t))
      {
	printf("XferServices::XferServices  Implement me !!\n");
      }
      virtual XferRequest* createXferRequest() {return new XferRequest(this);}
      virtual ~XferServices () {};
    };


    class XferFactory :
      public DT::DriverBase<XferFactory, Device, XferServices, dma> {
      friend class SmemServices;
      int       m_dmaFd; // if this is >= 0, then we have been there before
    public:
      XferFactory()
	: m_dmaFd(-1)
      {}


      void configure(ezxml_t x) {

      }

      virtual
      ~XferFactory() {
	lock();
	if (m_dmaFd >= 0)
	  ::close(m_dmaFd); // FIXME need OS abstraction
      }
    private:

    protected:
      // Getting a memory region happens on demand, but releasing them
      // only happens on shutdown
      void
      getDmaRegion(EndPoint &ep) {
	OU::SelfAutoMutex guard(this);
	ep.address = NULL;
      }

      uint8_t *
      mapDmaRegion(EndPoint &ep, uint32_t offset, size_t size) {
	return (uint8_t*)NULL;
      }
    public:
      const char *
      getProtocol() { return EPNAME; }

      DT::XferServices *
      getXferServices(DT::SmemServices* source, DT::SmemServices* target) {
	return new XferServices(source, target);
      }
      
      DT::EndPoint *
      createEndPoint(std::string& endpoint, bool local);

      // FIXME: provide ref to string as arg
      std::string 
      allocateEndpoint(const OU::PValue*, uint16_t mailBox, uint16_t maxMailBoxes) {
	std::string ep;
	
	OCPI::Util::formatString(ep, EPNAME ":0.0.0;%zu.%" PRIu16 ".%" PRIu16,
				 m_SMBSize, mailBox, maxMailBoxes);

	return ep;
      }
    };

    SmemServices::
    ~SmemServices() {
      munmap( m_rd_buffer[SEND], m_rd_buf_info[SEND].rd_buf_size );
      munmap( m_rd_buffer[RECV], m_rd_buf_info[RECV].rd_buf_size );
      close( m_rd_fd[SEND] );
      close( m_rd_fd[RECV] );    
    }

    void* 
    SmemServices::
    map(DtOsDataTypes::Offset offset, size_t size ) {

      assert(0);

      // FIXME - needs a new signature
      int idx = SEND;

      printf("Offset = %d, size = %d, vaddr = %p\n", offset, size, &m_rd_buffer[idx][offset] );
      assert( offset+size < m_rd_buf_info[idx].rd_buf_size);
      return &m_rd_buffer[idx][offset];
    }


    void* 
    SmemServices::
    mapTx(DtOsDataTypes::Offset offset, size_t size ) {

      if ( ! m_dmaEndPoint.local ) {
	return NULL;
      }


      // FIXME - needs a new signature
      int idx = SEND;

      printf("Offset = %d, size = %d, pool size =%d\n", offset, size, m_rd_buf_info[idx].rd_buf_size );
      assert( offset+size < m_rd_buf_info[idx].rd_buf_size);
      return &m_rd_buffer[idx][offset];
    }


    void* 
    SmemServices::
    mapRx(DtOsDataTypes::Offset offset, size_t size ) {

      if ( ! m_dmaEndPoint.local ) {
	return NULL;
      }


      // FIXME - needs a new signature
      int idx = RECV;

      printf("Offset = %d, size = %d\n", offset, size );

      assert( offset+size < m_rd_buf_info[idx].rd_buf_size);
      return &m_rd_buffer[idx][offset];
    }


    void 
    SmemServices::
    finalizeBuffer( int idx )
    {

	// Obtain the reserved DMA buffer's size and address.
	int err = ioctl(m_rd_fd[idx], TILEPCI_IOC_GET_RAW_DMA_BUF, &m_rd_buf_info[idx]);
	if (err < 0)
	  {
	    fprintf(stderr, "Host: Failed TILEPCI_IOC_GET_RAW_DMA_BUF: %s\n",
		    strerror(errno));
	    abort();
	  }

	host_buf_size = m_rd_buf_info[idx].rd_buf_size;

	printf("Host buffer size = %d\n", host_buf_size );

#ifdef RAW_DMA_USE_RESERVED_MEMORY

	host_buf_addr = m_rd_buf_info[idx].rd_buf_bus_addr;

	// On the host side, mmap the transmit DMA buffer *reserved* by the kernel.
	int mem_fd = open("/dev/mem", O_RDWR);
	m_rd_buffer = (uint8_t*)mmap(0, host_buf_size, PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, mem_fd, host_buf_addr);
	assert(buffer != MAP_FAILED);

	printf("Host transmit buffer %#x@%p, mapped at %p\n", host_buf_size,
	       (void *)host_buf_addr, buffer);

	// Zero out the first 64 bytes, which may contain
	// application-specific flow-control information.
	memset(buffer, 0, 64);

#else

	// On the host side, mmap the transmit DMA buffer *allocated* by the driver.
	m_rd_buffer[idx] = (uint8_t*)mmap(0, host_buf_size, PROT_READ | PROT_WRITE,
			    MAP_SHARED, m_rd_fd[idx], TILEPCI_RAW_DMA_BUF_MMAP_OFFSET);
	assert(m_rd_buffer[idx] != MAP_FAILED);
	memset( m_rd_buffer[idx], host_buf_size, 0 );

#endif

	// On the host side, mmap the queue status.
	m_rd_status[idx] = (struct tlr_rd_status *)
	  mmap(0, sizeof(struct tlr_rd_status), PROT_READ | PROT_WRITE,
	       MAP_SHARED, m_rd_fd[idx], TILEPCI_RAW_DMA_STS_MMAP_OFFSET);
	assert(m_rd_status != MAP_FAILED);
	

	// mmap the FC register, e.g. the producer index for H2T queue.
	m_rd_regs[idx] = (struct gxpci_host_rd_regs_app *)
	  mmap(0, sizeof(struct gxpci_host_rd_regs_app), PROT_READ | PROT_WRITE,
	       MAP_SHARED, m_rd_fd[idx], TILEPCI_RAW_DMA_REG_MMAP_OFFSET);
	assert(m_rd_regs[idx] != MAP_FAILED);

	// Activate this transmit queue.
	err = ioctl(m_rd_fd[idx], TILEPCI_IOC_ACTIVATE_RAW_DMA, NULL);
	if (err < 0)
	  {
	    fprintf(stderr, "Host: Failed TILEPCI_IOC_ACTIVATE_RAW_DMA: %s\n",
		    strerror(errno));
	    abort();
	  }

    }

    void
    SmemServices::
    createRawDmaBuffer() {

	// FIXME!! These come from EP
	card_index = 0;
	queue_index = 0;



	// Open the Raw DMA file.
	snprintf(dev_name, sizeof(dev_name), "/dev/tilegxpci%d/raw_dma/t2h/%d",
		 card_index, queue_index);
	m_rd_fd[RECV] = open(dev_name, O_RDWR);
	if (m_rd_fd[RECV] < 0)
	  {
	    fprintf(stderr, "Host: Failed to open '%s': %s\n", dev_name,
		    strerror(errno));
	    abort();
	  }
	finalizeBuffer( RECV );


	//	sleep( 4 );



	printf("About to OPEN the h2t raw dma device \n");

	// Open the Raw DMA file.
	snprintf(dev_name, sizeof(dev_name), "/dev/tilegxpci%d/raw_dma/h2t/%d",
		 card_index, queue_index);
	m_rd_fd[SEND] = open(dev_name, O_RDWR);
	if (m_rd_fd[SEND] < 0)
	  {
	    fprintf(stderr, "Host: Failed to open '%s': %s\n", dev_name,
		    strerror(errno));
	    abort();
	  }
	finalizeBuffer( SEND );

	printf("DOME !! the h2t raw dma device \n");



    }



    void 
    SmemServices::
    post( XferRequest & req ) {
      OU::SelfAutoMutex guard(this);

      printf("In SmemServices::post \n");

      // FIXME !! - We block until the last request has been processed.  We either need a work Q managed in the 
      // base classes or we will need to add a thread to process for this EP
      tilepci_xfer_req_t recv_cmd;
      recv_cmd.addr = reinterpret_cast<uintptr_t>(m_rq_buffer[RECV]);
      recv_cmd.len = 4;
      recv_cmd.cookie = 0xbaccfeed;

      while (write(m_rq_fd[RECV], &recv_cmd, sizeof(recv_cmd)) != sizeof(recv_cmd))
	fprintf(stderr, "write() failed: %s\n", strerror(errno));

      // Read back the completion.
      tilepci_xfer_comp_t comp;
      printf("waiting for credits \n");


      while (read(m_rq_fd[RECV], &comp, sizeof(comp)) != sizeof(comp))
	fprintf(stderr, "read() failed: %s\n", strerror(errno));


      printf("got a post credit\n");
      while ( m_rq_buffer[RECV][0] == 0 ) {
	printf("Waiting for Q to clear\n");
	sleep(0); // yield
      }

      assert( (req.m_XferReqs.size()+1) * sizeof(Req) < 4096 );
      Req * r = (Req*)m_rq_buffer[SEND];
      std::list<Req*>::iterator it = req.m_XferReqs.begin();
      r->nbytes = req.m_XferReqs.size();
      r++;
      for ( int i=0; i<req.m_XferReqs.size(); i++, it++ ){
	printf(" pull Req length = %d, offset = %d\n", (*it)->nbytes, (*it)->srcoff );
	uint32_t * debp = (uint32_t*)&m_rd_buffer[SEND][(*it)->srcoff];
	memcpy(r, (*it), sizeof(Req));
	r++;
      }

      
      // Given that we're the host side of a host-to-tile channel, we can
      // construct a send command and post it to the PCI subsystem.
      tilepci_xfer_req_t send_cmd;
      send_cmd.addr = (uintptr_t) m_rq_buffer[SEND];
      send_cmd.len = (req.m_XferReqs.size()+1) * sizeof(Req);
      send_cmd.cookie = m_seq++;
      //send_cmd.flags = TILEPCI_RCV_MUST_EOP;


      // Initiate the command.
      m_rd_regs[RECV]->host_counter += 1;


      printf("About to write\n");
      while (write(m_rq_fd[SEND], &send_cmd, sizeof(send_cmd)) != sizeof(send_cmd))
	fprintf(stderr, "write() failed: %s\n", strerror(errno));


      // FIXME:  For now we are single threaded

      // Read back the completion.
      printf("About to read\n");
      while (read(m_rq_fd[SEND], &comp, sizeof(comp)) != sizeof(comp))
	fprintf(stderr, "read() failed: %s\n", strerror(errno));

      // Print the send data, including the metadata 'cookie' that
      // came along with the data.
      printf("Sent %d bytes: cookie = %#lx, words[0] = %#x\n",
	     comp.len, (unsigned long)comp.cookie, 
	     ((uint32_t*)(uintptr_t)comp.addr)[0]);

    }


    void
    SmemServices::
    createReqQ() {

      // FIXME!! These come from EP
      card_index = 0;
      queue_index = 0;

      printf("Creating the h2t req Q\n");


      // Open the host side of a host-to-tile zero-copy channel.
      const char* path = "/dev/tilegxpci%d/h2t/%d";
      char name[128];
      sprintf(name, path, card_index, queue_index);      
      m_rq_fd[SEND] = open(name, O_RDWR);
      if (m_rq_fd[SEND] < 0)
	{
	  fprintf(stderr, "*** Failed to open '%s': %s\n", path, strerror(errno));
	  abort();
	}


      printf("Creating the t2h req Q\n");


      // Open the host side of a host-to-tile zero-copy channel.
      const char* tpath = "/dev/tilegxpci%d/t2h/%d";
      sprintf(name, tpath, card_index, queue_index);      
      m_rq_fd[RECV] = open(name, O_RDWR);
      if (m_rq_fd[RECV] < 0)
	{
	  fprintf(stderr, "*** Failed to open '%s': %s\n", path, strerror(errno));
	  abort();
	}


      printf("Mapping Q's\n");

      // On the host side, packet buffers must come from the per-channel
      // 'fast memory' region.
      m_rq_buffer[SEND] = (uint8_t*)mmap(0, 4096, PROT_READ | PROT_WRITE,
			 MAP_SHARED, m_rq_fd[SEND], 0);
      assert(m_rq_buffer[SEND] != MAP_FAILED);

      m_rq_buffer[RECV] = (uint8_t*)mmap(0, 4096, PROT_READ | PROT_WRITE,
			 MAP_SHARED, m_rq_fd[RECV], 0);
      assert(m_rq_buffer[RECV] != MAP_FAILED);
      m_rq_buffer[RECV][0] = 1;

    }

    SmemServices::
    SmemServices(EndPoint& ep) 
	: DT::SmemServices(ep), 
	  m_dmaEndPoint(ep),
	  m_driver(XferFactory::getSingleton()),
	  m_seq(0)
      {
	printf("In SmemServices\n");
	if ( ep.local ) {
	  printf("Source side !!\n");
	}
	else {
	  return;
	}
	      

	createRawDmaBuffer();
	createReqQ();
	sleep( 2 );

      }
    
    void XferRequest::post () {
      printf("About to post a transfer\n");
      m_XferServices->m_source->post( *this );
    }



      //    gmem = new SmemServices(ep);
  

#endif

    DT::EndPoint* XferFactory::
    createEndPoint(std::string& endpoint, bool local) {
#ifdef __tile__
      if ( ! local ) {
	printf("***** Creating new remote EP \n");

	return new EndPoint(endpoint, local);


      }
      else {

	printf("***** Returning pre-configured EP \n");

	EndPoint * e = g_ep[g_ep_cnt];

	printf("***** pre-configured EP = %p\n", e);

	return e;


      }
#else 

      return new EndPoint(endpoint, local);


#endif
    }
    
    DT::SmemServices & EndPoint::
    createSmemServices() {
      if ( m_smem ) {
	return *m_smem;
      }
      m_smem = new SmemServices(*this); 
      return *m_smem;
    }
    DT::RegisterTransferDriver<XferFactory> driver;


  }
}
#endif
