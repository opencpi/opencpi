
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


#ifndef XFER_IF_H
#define XFER_IF_H


#include <DtTransferInterface.h>

/* Constants */
#define XFER_FLAG  1
#define XFER_FIRST 2
#define XFER_LAST  4
#define XFER_MASK  6

/* Types */
typedef unsigned long ulong;

struct  XFTemplate {
  DataTransfer::SmemServices* s_smem;
  DataTransfer::SmemServices* t_smem;
  int                         s_off;
  int                         t_off;
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

/* External functions */
extern long xfer_create(DataTransfer::SmemServices*, DataTransfer::SmemServices*, OCPI::OS::int32_t , XF_template *);
extern long xfer_copy(XF_template, OCPI::OS::uint32_t, OCPI::OS::uint32_t, OCPI::OS::uint32_t, OCPI::OS::int32_t, XF_transfer *);
extern long xfer_group(XF_transfer *, OCPI::OS::int32_t, XF_transfer *);
extern long xfer_release(XF_transfer, OCPI::OS::int32_t);
extern long xfer_destroy(XF_template, OCPI::OS::int32_t);
extern long xfer_start(XF_transfer, OCPI::OS::int32_t);
extern long xfer_get_status(XF_transfer);
extern long xfer_modify( XF_transfer, OCPI::OS::uint32_t* noff, OCPI::OS::uint32_t* ooff );

#endif /* !defined XFER_IF_H */
