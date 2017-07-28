#ifndef DTOSDATATYPES_H
#define DTOSDATATYPES_H
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



/*
 * Abstract:
 *   Data transfer data type definitions
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#include "stdint.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiRes.h"

namespace DtOsDataTypes {

  // relative priority
  typedef int16_t   Priority;

  // port rank
  typedef uint32_t  Rank;

  // Address offset within endpoint
#ifndef OCPI_EP_SIZE_BITS
#define OCPI_EP_SIZE_BITS 32
#endif
#if OCPI_EP_SIZE_BITS == 32
  typedef uint32_t Offset;
  #define DTOSDATATYPES_OFFSET_PRIu PRIu32
  #define DTOSDATATYPES_OFFSET_PRIx PRIx32
#else
  typedef uint64_t Offset;
  #define DTOSDATATYPES_OFFSET_PRIu PRIu64
  #define DTOSDATATYPES_OFFSET_PRIx PRIx64
#endif

  // The (max) size of flag values for flow control purposes
  // Some hardware might insist on forcing it larger.
#ifndef OCPI_EP_FLAG_BITS
#define OCPI_EP_FLAG_BITS 32
#endif
#if OCPI_EP_FLAG_BITS == 32
  typedef uint32_t Flag;
  #define DTOSDATATYPES_FLAG_PRIx PRIx32
#else
  typedef uint64_t Flag;
  #define DTOSDATATYPES_FLAG_PRIx PRIx64
#endif

  typedef uint16_t MailBox;
  // Maximum number of SMB's and mailboxes allowed in the system unless overriden by env
  // FIXME:  make this part of config
  const MailBox MAX_SYSTEM_SMBS = 10;
}

#endif
