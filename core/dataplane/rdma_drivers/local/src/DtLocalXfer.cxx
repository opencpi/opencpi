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
 * Heap-allocated transfer driver within a process
 * FIXME: make allocations individual heap allocs without limits
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#include <inttypes.h>
#include "OcpiOsDebug.h"
#include "OcpiUtilMisc.h"
// We build this transfer driver using the PIO::XferServices
#include "DtPioXfer.h"

namespace OU = OCPI::Util;
namespace DT = DataTransfer;
namespace OCPI {
  namespace Local {

#define EPNAME "ocpi-local-rdma"

    class XferFactory;
    class Device : public DT::DeviceBase<XferFactory,Device> {
      Device(const char *name)
	: DataTransfer::DeviceBase<XferFactory,Device>(name, *this) {}
    };

    class SmemServices : public DT::SmemServices {
      uint8_t *m_vaddr;
      friend class EndPoint;
    protected:
      SmemServices(DT::EndPoint& ep) 
        : DT::SmemServices(ep), m_vaddr(NULL) {
	assert(ep.local);
	ep.address = (uint64_t)(m_vaddr = new uint8_t[ep.size]);
	OU::formatString(ep.end_point, EPNAME ":0x%p;%zu.%" PRIu16 ".%" PRIu16,
			 m_vaddr, ep.size, ep.mailbox, ep.maxCount);
	ocpiDebug("Finalized Local PIO ep %p: %s", &ep, ep.end_point.c_str());
      }

      virtual ~SmemServices () {
	delete [] m_vaddr;
      }

      void *map(DtOsDataTypes::Offset offset, size_t size ) {
	assert(offset + size <= m_endpoint.size);
	return (void*)(m_vaddr + offset);
      }
    };
    
    class EndPoint : public DT::EndPoint {
    public:
      EndPoint(const std::string& ep, bool local)
        : DT::EndPoint(ep, 0, local) {
	if (sscanf(ep.c_str(), EPNAME ":%" SCNx64 ";", &address) != 1)
	  throw OU::Error("Invalid format for "EPNAME" endpoint: %s", ep.c_str());
  
	ocpiDebug("Local PIO ep %p %s: address = 0x%" PRIx64 " size = 0x%zx",
		  this, ep.c_str(), address, size);
      };
      virtual ~EndPoint() {}

      DT::SmemServices & createSmemServices() {
	return *new SmemServices(*this);
      }

      // Get the address from the endpoint
      // FIXME: make this get address thing NOT generic...
      virtual const char* getAddress() {
	return 0;
      }
    };
    const char *local = "local"; // name passed to inherited template class
    class XferFactory :
      public DT::DriverBase<XferFactory, Device, OCPI::PIO::XferServices, local> {
      friend class SmemServices;
    public:
      virtual ~XferFactory() {}
    public:
      const char *
      getProtocol() { return EPNAME; }

      DT::XferServices *
      getXferServices(DT::SmemServices* source, DT::SmemServices* target) {
	return new OCPI::PIO::XferServices(source, target);
      }
      
      DT::EndPoint *
      createEndPoint(std::string& endpoint, bool local) {
	return new EndPoint(endpoint, local);
      }
      // FIXME: provide ref to string as arg
      std::string 
      allocateEndpoint(const OU::PValue*, uint16_t mailBox, uint16_t maxMailBoxes) {
	std::string ep;
	
	OCPI::Util::formatString(ep, EPNAME ":0;%zu.%" PRIu16 ".%" PRIu16,
				 m_SMBSize, mailBox, maxMailBoxes);
	return ep;
      }
    };

    DT::RegisterTransferDriver<XferFactory> driver;
  }
}
