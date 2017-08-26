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
//#include "xfer_internal.h"
#include "XferEndPoint.h"
#include "XferServices.h"
#include "XferPioInternal.h"
#include "XferManager.h"

namespace OX = OCPI::Util::EzXml;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OD = OCPI::Driver;
namespace DDT = DtOsDataTypes;
namespace DataTransfer {

XferServices::
XferServices(XferFactory &driver, EndPoint &source, EndPoint &target)
  : m_driver(driver), m_users(0), m_from(source), m_to(target) {
  m_from.addRef();
  m_to.addRef();
}

XferServices::
~XferServices() {
  //  m_driver.removeTemplate(*this); this is done during destruction of the XferFactory
  m_from.release();
  m_to.release();
}

void XferServices::
send(DtOsDataTypes::Offset /*offset*/, uint8_t */*data*/, size_t /*nbytes*/) {
  throw OU::Error("Direct send on endpoint that doesn't support it");
}

// Create a transfer request
XferRequest* XferRequest::
copy(Offset srcoffs, Offset dstoffs, size_t nbytes, XferRequest::Flags flags) {
  long retVal = 0;
  int32_t newflags = 0;
  if (flags & XferRequest::DataTransfer)
    newflags |= XFER_FIRST;
  if (flags & XferRequest::FlagTransfer)
    newflags |= XFER_LAST;
  if ( m_thandle == NULL ) {
    retVal = xfer_copy (m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &m_thandle);
    if (retVal)
      return NULL;
  } else {
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
    if (retVal)
      return NULL;
    xfer_release(handles[0], 0);
    xfer_release(handles[1], 0);
  }
  return this;
}

// Group data transfer requests
XferRequest & XferRequest::group (XferRequest* lhs) {
  XF_transfer handles[3];
  handles[0] = lhs->m_thandle;
  handles[1] = m_thandle;
  handles[2] = 0;
  xfer_group ( handles, 0, &m_thandle);
  return *this;
}

void XferRequest::
modify(Offset new_offsets[], Offset old_offsets[]) {
  int n=0;
  while ( new_offsets[n] ) {
    xfer_modify( m_thandle, &new_offsets[n], &old_offsets[n] );
    n++;
  }
}

void XferRequest::
action_transfer(PIO_transfer pio_transfer, bool) {
  xfer_pio_action_transfer(pio_transfer);
}
void XferRequest::
start_pio(PIO_transfer pio_transfer, bool last) {
  for (PIO_transfer transfer = pio_transfer; transfer; transfer = transfer->next)
    action_transfer(transfer, last);
}
void XferRequest::
post() {
  struct xf_transfer_ *xf_transfer = (struct xf_transfer_ *)m_thandle;
  
  /* Process the first transfers */
  if (xf_transfer->first_pio_transfer)
    start_pio(xf_transfer->first_pio_transfer);

  /* Get the type of transfer */
  if (xf_transfer->pio_transfer)
    /* Start the pio transfer */
    start_pio(xf_transfer->pio_transfer);

  /* Process the last transfers */
  if (xf_transfer->last_pio_transfer)
    /* Start the last pio transfer */
    start_pio(xf_transfer->last_pio_transfer,
	      xf_transfer->first_pio_transfer || xf_transfer->pio_transfer);
}

XferRequest::CompletionStatus XferRequest::
getStatus() {
  return xfer_get_status (m_thandle) == 0 ? CompleteSuccess : Pending;
}

XferRequest::
XferRequest(XF_template temp) : m_thandle(NULL), m_xftemplate(temp) {
}
XferRequest::
~XferRequest() {
  if (m_thandle)
    (void)xfer_release (m_thandle, 0);
}
}
