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

/**
   @file

   @brief
   This file contains private support functions for the shared memory classes.

   Revision History:

   6/15/2004 - John Miller
   Initial version.

   2/18/2009 - John Miller
   Removed exception monitor class.

************************************************************************** */
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "OcpiOsAssert.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiOsMisc.h"
#include "OcpiUtilHash.h"
#include "OcpiUtilMisc.h"
#include "OcpiRes.h"
#include "DtHandshakeControl.h"
#include "XferManager.h"
#include "XferEndPoint.h"
#include "XferServices.h"

namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace XF = DataTransfer;
namespace OCPI {
  namespace DataTransport {

static int check = compileTimeSizeCheck<sizeof(XF::Offset),sizeof(OCPI::Util::ResAddr)>();

// Determine if the mail box is avialable
bool XferMailBox::
mailBoxAvailable(XF::EndPoint &/*ep*/) {
  assert("container comms disabled"==0);
  ContainerComms &comms = *(ContainerComms *)NULL; // ep.containerComms();
  return (comms.mailBox[m_slot].request.header.type == ContainerComms::NoRequest) ? true : false;
}
ContainerComms::MailBox* XferMailBox::
getMailBox(XF::EndPoint &/*ep*/) {
  assert("container comms disabled"==0);
  ContainerComms &comms = *(ContainerComms *)NULL; // ep.containerComms();
  return &comms.mailBox[m_slot];
}

// FIXME: move this to a new file along with the handshake stuff
bool XferMailBox::
makeRequest(XF::EndPoint &source, XF::EndPoint &target )
{

  ocpiDebug("In makerequest from %s to %s\n", source.name().c_str(), target.name().c_str());

#ifdef MULTI_THREADED
  // Lock the mailbox
  if ( ! lockMailBox() ) {
    return false;
  }
#endif

  /* Attempt to get or make a transfer template */
  XF::XferServices* ptemplate = XF::getManager().getService(&source, &target);
  ocpiAssert(ptemplate);

  XF::Offset offset =
    OCPI_SIZEOF(XF::Offset, ContainerComms::MailBox) * m_slot + OCPI_SIZEOF(XF::Offset, UpAndRunning);

  ocpiDebug("In make request with offset = %d\n", offset );

  XF::XferRequest * ptransfer = ptemplate->createXferRequest();

  // create the copy in the template
  ptransfer->copy (
		   offset + OCPI_SIZEOF(XF::Offset, ContainerComms::RequestHeader),
		   offset + OCPI_SIZEOF(XF::Offset, ContainerComms::RequestHeader),
		   sizeof(ContainerComms::MailBox) - sizeof(ContainerComms::RequestHeader),
		   XF::XferRequest::DataTransfer );

  ptransfer->copy (
		   offset,
		   offset,
		   sizeof(ContainerComms::RequestHeader),
		   XF::XferRequest::FlagTransfer );

  // Start the transfer
  ptransfer->post();

  while( ptransfer->getStatus() ) {
    OS::sleep(0);
  }

  delete ptransfer;

  return true;
}
}
}
