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

// Helper functions and classes for PIO data movement.

#ifndef XFER_IF_H
#define XFER_IF_H

#include "XferEndPoint.h"
#include "XferServices.h"

/* Constants */
#define XFER_FLAG  1
#define XFER_FIRST 2
#define XFER_LAST  4
#define XFER_MASK  6

/* Types */

struct  XFTemplate {
  DataTransfer::SmemServices* s_smem;
  DataTransfer::SmemServices* t_smem;
  uint32_t                    s_off;
  uint32_t                    t_off;
};
class Mapit {
 public:
  virtual void map( unsigned int, unsigned int){};
  virtual void unmap(){};
  virtual ~Mapit(){};
};

struct XFTransfer  {
XFTransfer():m(NULL){};
  Mapit *m;
};

typedef XFTemplate* XF_template;
typedef XFTransfer* XF_transfer;

typedef struct pio_template_ * PIO_template;
struct pio_transfer_ : public XFTransfer {
  struct pio_transfer_ *next;  /* pointer to next transfer */
  void                 *src_va;                /* virtual address of src buffer */
  void                 *dst_va;                /* virtual address of dst buffer */
  DtOsDataTypes::Offset     src_off;
  DtOsDataTypes::Offset     dst_off;
  uint32_t     src_stride;            /* number of bytes between each element */
  uint32_t     dst_stride;            /* number of bytes between each element */
  size_t     nbytes;                 /* size of transfer */
};
typedef struct pio_transfer_ * PIO_transfer;
/* External functions */
extern long xfer_create(DataTransfer::EndPoint &source, DataTransfer::EndPoint &target, int32_t , XF_template *);
extern long xfer_copy(XF_template, DtOsDataTypes::Offset, DtOsDataTypes::Offset, size_t, int32_t, XF_transfer *);
extern long xfer_group(XF_transfer *, int32_t, XF_transfer *);
extern long xfer_release(XF_transfer, int32_t);
extern long xfer_destroy(XF_template, int32_t);
extern long xfer_start(XF_transfer, int32_t);
extern long xfer_get_status(XF_transfer);
extern long xfer_modify( XF_transfer, DtOsDataTypes::Offset* noff, DtOsDataTypes::Offset* ooff );
extern void xfer_pio_action_transfer(PIO_transfer);

#endif /* !defined XFER_IF_H */
