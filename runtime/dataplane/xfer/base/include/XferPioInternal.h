
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


#ifndef XFER_INTERNAL_H_
#define XFER_INTERNAL_H_

#include "XferPio.h"

#define PIO       1

struct pio_template_ : public XFTemplate
{
  void *src_va;        /* virtual address of source buffer */
  void *dst_va;        /* virtual address of destination buffer */
  void *rdst_va[100]; 
  int   rdst;
  pio_template_():rdst(0){};
};


struct xf_template_ : public XFTemplate
{
  void *src_va;              /* virtual address of src buffer (for PIO) */
  void *dst_va;              /* virtual address of dst buffer (for PIO) */
  PIO_template pio_template; /* pio template */
  int32_t type;                /* template type, PIO, PXB DMA, CE DMA */
};

struct xf_transfer_ : public XFTransfer
{
  struct xf_template_ *xf_template;  
  PIO_transfer first_pio_transfer;
  PIO_transfer pio_transfer; /* pio transfer (for HOST) */
  PIO_transfer last_pio_transfer;
};

extern int32_t xfer_pio_create(DataTransfer::EndPoint &, DataTransfer::EndPoint&, PIO_template *);
extern int32_t xfer_pio_copy(PIO_template, DtOsDataTypes::Offset, DtOsDataTypes::Offset, size_t, int32_t, 
			     PIO_transfer *);
extern int32_t xfer_pio_start(PIO_transfer, int32_t);
extern int32_t xfer_pio_group(PIO_transfer *, int32_t, PIO_transfer *);
extern int32_t xfer_pio_release(PIO_transfer);
extern int32_t xfer_pio_destroy(PIO_template);
extern void xfer_pio_modify( PIO_transfer, int,  DtOsDataTypes::Offset*,  DtOsDataTypes::Offset* );

#endif /* !defined XFER_INTERNAL_H */
