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
 * DMA transfer driver
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
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h> // FIXME: use OS services?
#include "KernelDriver.h"
#include "OcpiOsDebug.h"
#include "OcpiUtilMisc.h"
#include "XferDriver.h"
#include "XferEndPoint.h"
#include "XferServices.h"
#include "XferPio.h"

namespace OU = OCPI::Util;
namespace XF = DataTransfer;
namespace OCPI {
  namespace DMA {

#define EPNAME "ocpi-dma-pio"

    class XferFactory;
    class Device : public XF::DeviceBase<XferFactory,Device> {
      Device(const char *a_name)
	: XF::DeviceBase<XferFactory,Device>(a_name, *this) {}
    };

    class SmemServices;
    class EndPoint : public XF::EndPoint {
      friend class SmemServices;
      friend class XferFactory;
    protected:
      uint32_t m_holeOffset, m_holeEnd;
      uint64_t m_busAddr;
    public:
      EndPoint(XF::XferFactory &a_factory, const char *protoInfo, const char *eps,
	       const char *other, bool a_local, size_t a_size, const OU::PValue *params)
	: XF::EndPoint(a_factory, eps, other, a_local, a_size, params), 
          m_holeOffset(0), m_holeEnd(0), m_busAddr(0) {
	if (protoInfo) {
	  m_protoInfo = protoInfo;
	  if (sscanf(protoInfo, "%" SCNx64 ".%" SCNx32 ".%" SCNx32,
		     &m_busAddr, &m_holeOffset, &m_holeEnd) != 3)
	    throw OU::Error("Invalid format for DMA endpoint: %s", protoInfo);
	  ocpiDebug("DMA ep %p %s: address = 0x%" PRIx64
		    " size = 0x%zx hole 0x%" PRIx32 " end 0x%" PRIx32,
		    this, protoInfo, m_busAddr, a_size, m_holeOffset, m_holeEnd);
	} else
	  m_protoInfo = "0.0.0";
      };
      virtual ~EndPoint() {}

      XF::SmemServices &createSmemServices();
    };

    class XferServices;
    const char *dma = "dma"; // name passed to inherited template class
    class XferFactory : public XF::DriverBase<XferFactory, Device, XferServices, dma> {
      friend class SmemServices;
      friend class XferServices;
      int       m_dmaFd; // if this is >= 0, then we have been there before
      bool      m_usingKernelDriver;
      uint64_t  m_dmaBase;
      unsigned  m_maxMBox, m_perMBox;
    public:
      XferFactory()
	: m_dmaFd(-1), m_usingKernelDriver(false), m_dmaBase(UINT64_MAX),
	  m_maxMBox(0)
      {}

      virtual
      ~XferFactory() {
	lock();
	if (m_dmaFd >= 0)
	  ::close(m_dmaFd); // FIXME need OS abstraction
      }
    private:
      void
      initDma(uint16_t maxCount) {
	if ((m_dmaFd = ::open(OCPI_DRIVER_MEM, O_RDWR | O_SYNC)) >= 0)
	  m_usingKernelDriver = true;
	else if ((m_dmaFd = ::open("/dev/mem", O_RDWR|O_SYNC )) < 0)
	  throw OU::Error("cannot open /dev/mem for DMA (Use sudo or load the driver)");
	else {
	  m_usingKernelDriver = false;
	  const char *l_dma = getenv("OCPI_DMA_MEMORY");
	  if (!l_dma)
	    throw OU::Error("OCPI_DMA_MEMORY environment variable not set");
	  unsigned sizeM, pagesize = (unsigned)getpagesize();
	  uint64_t top;
	  if (sscanf(l_dma, "%uM$0x%" SCNx64, &sizeM, &m_dmaBase) != 2)
	    throw OU::Error("Bad format for OCPI_DMA_MEMORY environment variable: '%s'",
			    l_dma);
	  ocpiDebug("DMA Memory:  %uM at 0x%" PRIx64, sizeM, m_dmaBase);
	  unsigned dmaSize = sizeM * 1024 * 1024;
	  top = m_dmaBase + dmaSize;
	  if (m_dmaBase & (pagesize-1)) {
	    m_dmaBase += pagesize - 1;
	    m_dmaBase &= ~(pagesize - 1);
	    top &= ~(pagesize - 1);
	    dmaSize = (uint32_t)(top - m_dmaBase);
	    ocpiBad("DMA Memory is NOT page aligned.  Now %u at 0x%" PRIx64 "\n",
		    dmaSize, m_dmaBase);
	  }
	  m_maxMBox = maxCount;
	  m_perMBox = (dmaSize / maxCount) & ~(pagesize - 1);
	}
      }
    protected:
      // Getting a memory region happens on demand, but releasing them
      // only happens on shutdown
      void
      getDmaRegion(EndPoint &ep) {
	OU::SelfAutoMutex guard(this);
	if (m_dmaFd < 0)
	  initDma(ep.maxCount());
	ocpi_request_t request;
	memset(&request, 0, sizeof(request));
	request.needed = (ocpi_size_t)ep.size();
	request.how_cached = ocpi_uncached;
	if (m_usingKernelDriver) {
	  if (ioctl(m_dmaFd, OCPI_CMD_REQUEST, &request))
	    throw OU::Error("Can't allocate memory size %zu for DMA memory", ep.size());
	} else {
	  ocpiAssert(ep.maxCount() == m_maxMBox);
	  // chop it up into equal parts assuming everyone has the same maxCount
	  if (ep.size() > m_perMBox)
	    throw OU::Error("not enough memory to accommodate all endpoints");
	  request.actual = m_perMBox;
	  request.address = m_dmaBase + m_perMBox * ep.mailBox();
	  request.bus_addr = ep.m_busAddr;
	}
	ep.m_address = request.address;
	ep.m_busAddr = request.bus_addr;
      }

      uint8_t *
      mapDmaRegion(EndPoint &ep, uint32_t offset, size_t size) {
	OU::SelfAutoMutex guard(this);
	if (m_dmaFd < 0)
	  initDma(ep.maxCount());

	void *vaddr =  mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED,
			    m_dmaFd, (off_t)(ep.m_address + offset));
	ocpiDebug("For ep %p, offset 0x%" PRIx32 " size %zu vaddr is %p to %p @ 0x%" PRIx64,
		  &ep, offset, size, vaddr, (uint8_t *)vaddr + size, ep.m_address);
	if (vaddr == MAP_FAILED)
	  throw OU::Error("mmap failed on DMA region %zu at 0x%" PRIx64,
			  size, ep.m_address + offset);
	return (uint8_t*)vaddr;
      }
    public:
      const char *
      getProtocol() { return EPNAME; }

      XF::XferServices &
      createXferServices(XF::EndPoint &source, XF::EndPoint &target);

      XF::EndPoint &
      createEndPoint(const char *protoInfo, const char *eps, const char *other, bool local,
		     size_t size, const OCPI::Util::PValue *params) {
	EndPoint &ep = *new EndPoint(*this, protoInfo, eps, other, local, size, params);
	if (!local) {
	  // This endpoint is remote: it means we have been told about it using the URL, which
	  // has a bus address. For us to map to it using mmap, we need to tell the kernel driver
	  // about this bus address region in case it could not discover it.
	  // It will tell us what local physical address to use for mmap offsets.
	  // (FIXME: security hole when not discovered properly in kernel mode)
	  // We'll use the ioctl request to the driver by setting the memory needed to zero
	  if (m_dmaFd < 0)
	    initDma(ep.maxCount());
	  if (m_usingKernelDriver) {
	    ocpi_request_t request;
	    memset(&request, 0, sizeof(request));
	    request.needed = 0; // signal to driver that we are doing bus2phys
	    request.bus_addr = ep.m_busAddr;
	    request.how_cached = ocpi_uncached;
	    request.actual =
	      OCPI_UTRUNCATE(ocpi_size_t, ep.m_holeOffset ? ep.m_holeOffset : ep.size());
	    // A request to enable mapping to this bus address/size and return the physaddr
	    if (ioctl(m_dmaFd, OCPI_CMD_REQUEST, &request))
	      throw OU::Error("Can't establish remote DMA memory size %" PRIu32 " at 0x%" PRIx64
			      "for DMA memory", request.actual, ep.m_busAddr);
	    ep.m_address = request.address;
	    if (ep.m_holeOffset) {
	      memset(&request, 0, sizeof(request));
	      request.bus_addr = ep.m_busAddr + ep.m_holeEnd;
	      request.actual = OCPI_UTRUNCATE(ocpi_size_t, ep.size() - ep.m_holeEnd);
	      if (ioctl(m_dmaFd, OCPI_CMD_REQUEST, &request))
		throw
		  OU::Error("Can't establish second remote DMA memory size %" PRIu32 " at 0x%"
			    PRIx64 "for DMA memory", request.actual, request.bus_addr);
	    }
	  } else
	    // If we are not using a driver we must assume the bus address is indeed the
	    // local phyisical address;
	    ep.m_address = ep.m_busAddr;
	}
	ocpiDebug("DMA create %s/%s ep %p %s: address = 0x%" PRIx64 " busaddr = 0x%" PRIx64
		  " size = 0x%zx hole 0x%" PRIx32 " end 0x%" PRIx32,
		  ep.local() ? "local" : "remote", local ? "local" : "remote",
		  &ep, ep.name().c_str(), ep.m_address, ep.m_busAddr, ep.size(), ep.m_holeOffset,
		  ep.m_holeEnd);
	return ep;
      }
    };

    class SmemServices : public XF::SmemServices {
      EndPoint &m_dmaEndPoint;
      uint8_t *m_vaddr, *m_vaddr1;
      XferFactory &m_driver;
      friend class EndPoint;
    protected:
      SmemServices(EndPoint& ep) 
	: XF::SmemServices(ep), m_dmaEndPoint(ep), m_vaddr(NULL), m_vaddr1(NULL),
	  m_driver(XferFactory::getSingleton()) {
	// For remote mappings all is deferred until mapping
	if (ep.local()) {
	  m_driver.getDmaRegion(ep);
	  m_vaddr = m_driver.mapDmaRegion(ep, 0, ep.size());
	  OU::format(ep.m_protoInfo, "%" PRIx64 ".%" PRIx32 ".%" PRIx32,
		     ep.m_busAddr, ep.m_holeOffset, ep.m_holeEnd);
	  ep.setName();
	  ocpiDebug("Finalized local DMA ep %p: %s", &ep, ep.name().c_str());
	}
      }
    public:
      virtual ~SmemServices () {
      }

      void *map(DtOsDataTypes::Offset offset, size_t size ) {
	EndPoint &ep = m_dmaEndPoint;
	OU::SelfAutoMutex guard (&m_driver);
	uint8_t *vaddr = 0;
	if (m_vaddr == NULL) {
	  if (ep.m_holeOffset) {
	    m_vaddr = m_driver.mapDmaRegion(ep, 0, ep.m_holeOffset);
	    m_vaddr1 = m_driver.mapDmaRegion(ep, ep.m_holeEnd, ep.size() - ep.m_holeEnd);
	    ocpiDebug("dma ep %p has vaddr %p to %p, vaddr1 %p to %p",
		      &ep, m_vaddr, m_vaddr + ep.m_holeOffset, m_vaddr1,
		      m_vaddr1 + (ep.size() - ep.m_holeEnd));
	  } else {
	    m_vaddr = m_driver.mapDmaRegion(ep, 0, ep.size());
	    ocpiDebug("dma ep %p has vaddr %p to %p, no vaddr1",
		      &ep, m_vaddr, m_vaddr + ep.size());
	  }
	}
	size_t top = offset + size;
	if (top <= ep.size()) {
	  if (ep.m_holeOffset == 0 || (offset < ep.m_holeOffset && top <= ep.m_holeOffset))
	    vaddr = m_vaddr + offset;
	  else if (ep.m_holeOffset && offset >= ep.m_holeEnd)
	    vaddr = m_vaddr1 + (offset - ep.m_holeEnd);
	}
	if (!vaddr)
	  throw OU::Error("Mapping offset %" DTOSDATATYPES_OFFSET_PRIx " size %zu out of range",
			  offset, size);
	ocpiDebug("DMA::SmemServices::map returning %s vaddr = %p base %p/%p offset 0x%"
		  DTOSDATATYPES_OFFSET_PRIx " size %zu, end 0x%p",
		  m_dmaEndPoint.local() ? "local" : "remote",
 		  vaddr, m_vaddr, m_vaddr1, offset, size, vaddr + size);
	return (void*)vaddr;
      }
    };
    
    XF::SmemServices & EndPoint::
    createSmemServices() {
      return *new SmemServices(*this);
    }

    class XferServices;
    class XferRequest : public XF::TransferBase<XferServices, XferRequest> {
    public:
      XferRequest(XferServices &a_parent, XFTemplate *temp)
	: XF::TransferBase<XferServices, XferRequest>(a_parent, *this, temp) {
      }
      virtual ~XferRequest() {
      }
    };
    class XferServices
      : public XF::ConnectionBase<XferFactory,XferServices,XferRequest> {
      // So the destructor can invoke "remove"
      friend class XferRequest;
      XFTemplate       *m_xftemplate; // The handle returned by xfer_create
      XF::XferRequest  *m_txRequest;  // Our transfer request
      XF::SmemServices *m_sourceSmb;  // Source SMB services pointer
      XF::SmemServices *m_targetSmb;  // Target SMB services pointerw
    public:
      XferServices(XF::EndPoint &source, XF::EndPoint &target)
	: XF::ConnectionBase<XferFactory, XferServices, XferRequest>(*this, source, target) {
	m_txRequest = NULL;
	m_sourceSmb = &source.sMemServices();
	m_targetSmb = &target.sMemServices();
	xfer_create(source, target, 0, &m_xftemplate);
      }
      virtual ~XferServices() {
	xfer_destroy(m_xftemplate, 0);
      }
      XF::XferRequest* createXferRequest() {
	OU::SelfAutoMutex guard (&parent()); 
	return new XferRequest(*this, m_xftemplate);
      }
    };

    XF::XferServices &XferFactory::
    createXferServices(XF::EndPoint &source, XF::EndPoint &target) {
      return *new XferServices(source, target);
    }

    XF::RegisterTransferDriver<XferFactory> driver;
  }
}
