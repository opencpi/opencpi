
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



/* Facility Interface Includes */
#include <stdio.h>
#include <stdlib.h>
#include "OcpiOsAssert.h"
#include "DtOsDataTypes.h"
#include "xfer_internal.h"

#define LocEQ(loc,place) strcmp(loc->getAddress(),place)==0

long
xfer_create(DataTransfer::SmemServices* src_ep,
            DataTransfer::SmemServices* dst_ep,
            int32_t,
            XF_template *xf_templatep)
{
  /* Define the local template */
  struct xf_template_ *tp;

  /* Generic return code */
  int32_t rc=0;

  /* Calculate where the src and dst are */
  
  /* Allocate a template structure */
  tp = new struct xf_template_;

  /* Set the transfer type */
  tp->type = PIO;

  /* Create the template */
  rc = xfer_pio_create(src_ep, dst_ep, &tp->pio_template);

  /* Check the return code */
  if (rc) {
    delete tp;
    return 1;
  }

  /* Set the out parameter */
  *xf_templatep = tp;
        
  return 0;
}


long
xfer_copy(XF_template xf_template,
          DtOsDataTypes::Offset src_os,
          DtOsDataTypes::Offset dst_os,
          size_t nbytes,
          int32_t flags,
          XF_transfer *xf_transferp)
{
  struct xf_template_ *tp = (struct xf_template_ *)xf_template;
  struct xf_transfer_ *xf = new struct xf_transfer_;

  int32_t rc=0;

  /* Initialize the transfer's template member */
  xf->xf_template = tp;

  /* Initialize the transfer pointers */
  xf->pio_transfer = 0;
  xf->first_pio_transfer = 0;
  xf->last_pio_transfer = 0;

  if (tp->type == PIO) {
    if (flags & XFER_FIRST) {
      rc = xfer_pio_copy(tp->pio_template,
                         src_os, dst_os,
                         nbytes,
                         flags,
                         &xf->first_pio_transfer);
    }
    else if (flags & XFER_LAST) {
      rc = xfer_pio_copy(tp->pio_template,
                         src_os, dst_os,
                         nbytes,
                         flags,
                         &xf->last_pio_transfer);
    }
    else {
      rc = xfer_pio_copy(tp->pio_template,
                         src_os, dst_os,
                         nbytes,
                         flags,
                         &xf->pio_transfer);
    }

    if (rc) {
      delete xf;
      return 1;
    }
  }

  /* Set the out parameter */
  *xf_transferp = (XF_transfer)xf;

  /* Return success */
  return 0;
}


long
xfer_release(XF_transfer xf_handle, int32_t)
{
  struct xf_transfer_ *xf_transfer = (struct xf_transfer_ *)xf_handle;
  int32_t pio_rc=0;

  /* Get the type of transfer */
  if (xf_transfer->first_pio_transfer) {

    /* Release the transfer */
    pio_rc = xfer_pio_release(xf_transfer->first_pio_transfer);
  }
  if (xf_transfer->last_pio_transfer) {

    /* Release the transfer */
    pio_rc = xfer_pio_release(xf_transfer->last_pio_transfer);
  }
  if (xf_transfer->pio_transfer) {

    /* Release the transfer */
    pio_rc = xfer_pio_release(xf_transfer->pio_transfer);
  }

  /* Free the handle */
  delete xf_transfer;

  return pio_rc;
}

long
xfer_destroy(XF_template xf_handle, int32_t)
{
  struct xf_template_ *xf_template = (struct xf_template_ *)xf_handle;
  int32_t rc;

  /* Get the type of template */
  if (xf_template->type == PIO) {

    /* Destroy the template */
    rc = xfer_pio_destroy(xf_template->pio_template);

    /* Free the template handle */
    delete xf_template;

    /* Return the return code */
    return rc;
  }
  return 0;
}

long
xfer_group(XF_transfer *xf_members,
           int32_t flags,
           XF_transfer *xf_transferp)
{
  /* Local Variables */
  PIO_transfer *pio_members=0;

  struct xf_transfer_ *xf_transfer = (struct xf_transfer_ *)xf_members[0];
  struct xf_template_ *xf_template = xf_transfer->xf_template;

  struct xf_transfer_ *xf = new struct xf_transfer_;

  int32_t i;

  /* Initialize the transfers template */
  xf->xf_template = xf_template;

  /* Initialize the transfer pointers */
  xf->pio_transfer = 0;
  xf->first_pio_transfer = 0;
  xf->last_pio_transfer = 0;

  int32_t rc=0;
  int32_t nxf;
  int32_t pio=0;

  /* Count the number of members */
  for (nxf=0; xf_members[nxf]; nxf++)
    ;

  /* Calculate the maximum size of the list */
  size_t size = (sizeof(PIO_transfer) * (nxf + 1));

  /* Allocate the memory for the list */
  if (!(pio_members = (PIO_transfer *)malloc(size))) {
    delete xf;
    return 1;
  }

  /* Loop for each member */
  for (i=0; i < nxf; i++) {

    /* See if the member has a pio component */
    if (((struct xf_transfer_ *)xf_members[i])->first_pio_transfer)
      pio_members[pio++] =
        ((struct xf_transfer_ *)xf_members[i])->first_pio_transfer;
  }

  /* Terminate the list */
  pio_members[pio] = 0;

  /* If we have any members, group them */
  if (pio && !rc) {
    rc = xfer_pio_group(pio_members, flags, &xf->first_pio_transfer);

    if (rc) {
#ifndef NDEBUG
      printf("xfer_pio_group failed\n");
#endif
          ocpiAssert(0);
    }
  }

  /* Reinitialize the counters */
 // dx=0;
 // pxb=0;
  pio=0;

  /* Loop for each member */
  for (i=0; i < nxf; i++) {

    /* See if the member has a pio component */
    if (((struct xf_transfer_ *)xf_members[i])->last_pio_transfer)
      pio_members[pio++] =
        ((struct xf_transfer_ *)xf_members[i])->last_pio_transfer;
  }

  /* Terminate the list */
  pio_members[pio] = 0;

  /* If we have any members, group them */
  if (pio && !rc) {
    rc = xfer_pio_group(pio_members, flags, &xf->last_pio_transfer);
    if (rc) {
#ifndef NDEBUG
      printf("xfer_pio_group failed\n");
#endif
          ocpiAssert(0);
    }
  }

  /* Reinitialize the counters */
  pio=0;

  /* Loop for each member */
  for (i=0; i < nxf; i++) {

    /* See if the member has a pio component */
    if (((struct xf_transfer_ *)xf_members[i])->pio_transfer)
      pio_members[pio++] =
        ((struct xf_transfer_ *)xf_members[i])->pio_transfer;
  }

  /* Terminate the list */
  pio_members[pio] = 0;

  /* If we have any members, group them */
  if (pio ) {
    rc = xfer_pio_group(pio_members, flags, &xf->pio_transfer);
    if (rc) {
#ifndef NDEBUG
      printf("xfer_pio_group failed\n");
#endif
          ocpiAssert(0);
    }
  }

  /* Free the member list */
  free(pio_members);

  /* Check for return code */
  if (rc) {
    delete xf;
    return 1;
  }

  /* Set the output parameter */
  *xf_transferp = (XF_transfer)xf;

  /* Return success */
  return 0;
}

#if 0

long
xfer_start(XF_transfer xf_handle, int32_t flags)
{
  struct xf_transfer_ *xf_transfer = (struct xf_transfer_ *)xf_handle;
  
  int32_t pio_rc=0;


  /* Process the first transfers */

  if (xf_transfer->first_pio_transfer) {
    pio_rc = xfer_pio_start(xf_transfer->first_pio_transfer, flags);
  }

  /* Get the type of transfer */
  if (xf_transfer->pio_transfer) {
    /* Start the pio transfer */
    pio_rc = xfer_pio_start(xf_transfer->pio_transfer, flags);
  }

  /* Process the last transfers */
  if (xf_transfer->last_pio_transfer) {

    /* Start the last pio transfer */
    pio_rc = xfer_pio_start(xf_transfer->last_pio_transfer, flags);
  }

  return pio_rc;
}

#endif
long 
xfer_modify( XF_transfer xf_handle, DtOsDataTypes::Offset* noff, DtOsDataTypes::Offset* ooff )
{
  struct xf_transfer_ *xf_transfer = (struct xf_transfer_ *)xf_handle;
  if (xf_transfer->first_pio_transfer) {
    xfer_pio_modify(xf_transfer->first_pio_transfer, 0, noff, ooff );
  }
  return 0;
}



long
xfer_get_status(XF_transfer )
{
  // PIO is synchronous
  return 0;
}

