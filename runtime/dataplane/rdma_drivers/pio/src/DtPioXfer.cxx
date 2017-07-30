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
#include <DtPioXfer.h>
#include <xfer_if.h>
#include <OcpiUtilMisc.h>

namespace OU = OCPI::Util;
namespace DT = DataTransfer;
namespace OCPI {
  namespace PIO {  

    Device::
    Device(const char *a_name)
      : DT::DeviceBase<XferFactory, Device>(a_name, *this) {
    }

    const char *pio = "pio"; // name passed to inherited template class
    XferFactory::XferFactory()
      throw ()
    {
      ocpiDebug("In PIO::XferFactory::PIO::XferFactory(): %zu", m_SMBSize);
    }

    // Destructor
    XferFactory::~XferFactory()
      throw ()
    {
    }

    DT::EndPoint* XferFactory::
    createEndPoint(std::string& endpoint, bool local) {
      ocpiDebug("In PIO::XferFactory::createEndPoint(): %zu", m_SMBSize);
      return new EndPoint(endpoint, local);
    }

    const char* XferFactory::
    getProtocol() {
      return "ocpi-smb-pio";
    }

    // Constructor
    EndPoint::
    EndPoint( std::string& ep, bool a_local)
      : DT::EndPoint(ep, 0, a_local) {
      parse(ep);
    }

    // Destructor
    EndPoint::
    ~EndPoint()
    {
    }

    // Sets smem location data based upon the specified endpoint
    int32_t EndPoint::
    parse(std::string& ep)
    {
      char sname[80];
      if (sscanf(ep.c_str(), "ocpi-smb-pio:%[^:]:", sname) != 1) {
	throw OU::Error("PIO Xfer:  ERROR: Bad PIO endpoint format (%s)\n", ep.c_str());
      }
      m_smb_name = sname;
      return 0;
    }

    // This method is used to allocate a transfer compatible SMB
    DT::SmemServices& EndPoint::
    createSmemServices()
    {
      return createHostSmemServices(*this);
    }

    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    DT::XferServices* XferFactory::
    getXferServices(DT::SmemServices* source, DT::SmemServices* target)
    {
      return new XferServices(source, target);
    }


    /***************************************
     *  This method is used to dynamically allocate
     *  an endpoint for an application running on "this"
     *  node.
     ***************************************/
    static int32_t smb_count = 0;
    std::string XferFactory::
    allocateEndpoint(const OU::PValue*, uint16_t mailBox, uint16_t maxMailBoxes, size_t size)
    {
      OU::SelfAutoMutex guard (this); 
      std::string ep;

      OU::formatString(ep, "ocpi-smb-pio:pioXfer%d%d;%zu.%" PRIu16 ".%" PRIu16,
		       getpid(), smb_count++, size ? size : m_SMBSize, mailBox, maxMailBoxes);
      return ep;
    }


    XferRequest::
    XferRequest(XferServices &a_parent, XF_template temp)
      : DT::TransferBase<XferServices, XferRequest>(a_parent, *this, temp) {
    }

    // PIOXferRequest destructor implementation
    XferRequest::~XferRequest ()
    {
    }


    XferServices::
    XferServices(DT::SmemServices *a_source, DT::SmemServices *a_target)
      : DT::ConnectionBase<XferFactory, XferServices, XferRequest>(*this, a_source, a_target) {
      createTemplate(a_source, a_target);
    }

    // Create tranfer services template
    void XferServices::
    createTemplate(DT::SmemServices* p1, DT::SmemServices* p2)
    {

      m_txRequest = NULL;
      m_sourceSmb = p1;
      m_targetSmb = p2;

      // Invoke original code, saving the returned template reference.
      xfer_create(p1, p2, 0, &m_xftemplate);
    }



    DT::XferRequest* XferServices::
    createXferRequest()
    {
      OU::SelfAutoMutex guard (&parent()); 
      return new XferRequest ( *this, m_xftemplate );
    }


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
    DT::XferRequest* PIOXferRequest::copy (uint32_t srcoffs, 
					   uint32_t dstoffs, 
					   uint32_t nbytes, 
					   DT::XferRequest::Flags flags
					   )
    {

      int32_t retVal = 0;
      int32_t newflags = 0;
      if (flags & DT::XferRequest::DataTransfer) newflags |= XFER_FIRST;
      if (flags & DT::XferRequest::FlagTransfer) newflags |= XFER_LAST;
      if ( getHandle() == NULL ) {
	retVal = xfer_copy ( parent().m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &getHandle());
	if (retVal){
	  return NULL;
	}
      }
      else {
	DT::XF_transfer handle;
	retVal = xfer_copy ( parent().m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &handle);
	if (retVal){
	  return NULL;
	}
	DT::XF_transfer handles[3];
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
    DT::XferRequest & XferRequest::group (XferRequest* lhs )
    {
      DT::XF_transfer handles[3];
      handles[0] = static_cast<PIOXferRequest*>(lhs)->getHandle();
      handles[1] = getHandle();
      handles[2] = 0;
      xfer_group ( handles, 0, &getHandle());
      return *this;
    }
#endif



    // Destructor
    XferServices::~XferServices ()
    {
      // Invoke destroy without flags.
      xfer_destroy (m_xftemplate, 0);
    }

    DT::RegisterTransferDriver<XferFactory> driver;

  }
}
