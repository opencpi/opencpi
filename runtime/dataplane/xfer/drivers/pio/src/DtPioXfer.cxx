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
 * Abstract:
 *   This file contains the implementation for the base class for PIO transfers.
 *
 *  John Miller - 6/15/09
 *  Fixed Coverity issues
 *
 *  John Miller -  7/20/04
 *  Initial version
 *
 */
#include <inttypes.h>
#include <unistd.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilSelfMutex.h"
#include "XferEndPoint.h"
#include "XferDriver.h"
#include "XferPio.h"
#include "HostSmemServices.h"
// Programmed I/O via named shared memory buffers

namespace OU = OCPI::Util;
namespace XF = DataTransfer;
namespace OCPI {
  namespace PIO {  

class EndPoint : public XF::EndPoint {
  friend class XferFactory;
  std::string m_smb_name;
protected:
  EndPoint(XF::XferFactory &a_factory, const char *protoInfo, const char *eps, const char *other,
	   bool a_local, size_t a_size, const OU::PValue *params)
    : XF::EndPoint(a_factory, eps, other, a_local, a_size, params) { 
    static uint16_t smb_count = 0;
    if (protoInfo) {
      m_protoInfo = protoInfo;
      m_smb_name = protoInfo;
    } else
      OU::format(m_protoInfo, "pioXfer%d.%d", getpid(), smb_count++);
  }
  // This method is used to allocate a transfer compatible SMB
  XF::SmemServices&
  createSmemServices() {
    return createHostSmemServices(*this);
  }
};

class XferFactory;
class Device : public XF::DeviceBase<XferFactory,Device> {
  Device(const char *a_name)
    : XF::DeviceBase<XferFactory, Device>(a_name, *this) {
  }
};


class XferServices;
class XferRequest : public XF::TransferBase<XferServices, XferRequest> {
  friend class XferServices;
protected:
  XferRequest(XferServices &a_parent, XF_template temp)
    : XF::TransferBase<XferServices, XferRequest>(a_parent, *this, temp) {
  }
};

class XferServices : public XF::ConnectionBase<XferFactory,XferServices,XferRequest> {
  friend class XferRequest;
  friend class XferFactory;
  XF_template m_xftemplate;
protected:
  XferServices(XF::EndPoint &source, XF::EndPoint &target)
    : XF::ConnectionBase<XferFactory, XferServices, XferRequest>(*this, source, target) {
    xfer_create(source, target, 0, &m_xftemplate);
  }

  ~XferServices ()
  {
    // Invoke destroy without flags.
    xfer_destroy(m_xftemplate, 0);
  }

  XF::XferRequest* 
  createXferRequest() {
    return new XferRequest (*this, m_xftemplate);
  }
};

const char *pio = "pio"; // name passed to inherited template class
class XferFactory : public XF::DriverBase<XferFactory, Device, XferServices, pio> {
  friend class XferServices;
public:
  XferFactory() throw () {
    ocpiDebug("In PIO::XferFactory::PIO::XferFactory(): %zu", m_SMBSize);
  }
protected:
  virtual ~XferFactory() throw () {
  }

  // Get our protocol string
  const char* getProtocol() {
    return "ocpi-smb-pio";
  }

  XF::EndPoint &
  createEndPoint(const char *protoInfo, const char *eps, const char *other, bool local,
		 size_t size, const OCPI::Util::PValue *params) {
    ocpiDebug("In PIO::XferFactory::createEndPoint(): %zu", m_SMBSize);
    return *new EndPoint(*this, protoInfo, eps, other, local, size, params);
  }

  /***************************************
   *  This method is used to create a transfer service object
   ***************************************/
  XF::XferServices &
  createXferServices(XF::EndPoint &source, XF::EndPoint &target) {
    return *new XferServices(source, target);
  }
};

#if 0
    void XferRequest::modify( OS::uint32_t new_offsets[], OS::uint32_t old_offsets[] )
    {
      int n=0;
      while ( new_offsets[n] ) {
	xfer_modify( m_thandle, &new_offsets[n], &old_offsets[n] );
	n++;
      }
    }

    // Create a transfer request
    XF::XferRequest* PIOXferRequest::copy (uint32_t srcoffs, 
					   uint32_t dstoffs, 
					   uint32_t nbytes, 
					   XF::XferRequest::Flags flags
					   )
    {

      int32_t retVal = 0;
      int32_t newflags = 0;
      if (flags & XF::XferRequest::DataTransfer) newflags |= XFER_FIRST;
      if (flags & XF::XferRequest::FlagTransfer) newflags |= XFER_LAST;
      if ( getHandle() == NULL ) {
	retVal = xfer_copy ( parent().m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &getHandle());
	if (retVal){
	  return NULL;
	}
      }
      else {
	XF::XF_transfer handle;
	retVal = xfer_copy ( parent().m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &handle);
	if (retVal){
	  return NULL;
	}
	XF::XF_transfer handles[3];
	handles[0] = handle;
	handles[1] = getHandle();
	handles[2] = 0;
	retVal = xfer_group ( handles, 0, &getHandle());
	if (retVal) {
	  return NULL;
	}
      }
      return this;
    }


    // Group data transfer requests
    XF::XferRequest & XferRequest::group (XferRequest* lhs )
    {
      XF::XF_transfer handles[3];
      handles[0] = static_cast<PIOXferRequest*>(lhs)->getHandle();
      handles[1] = getHandle();
      handles[2] = 0;
      xfer_group ( handles, 0, &getHandle());
      return *this;
    }
#endif
    XF::RegisterTransferDriver<XferFactory> driver;
  }
}
