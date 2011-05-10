
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


// This file exposes the OCPI user interface for workers and the ports they own.
#ifndef OCPI_CONTAINER_WORKER_H
#define OCPI_CONTAINER_WORKER_H
#include <stdint.h>
#include "ezxml.h"
#include "OcpiWorker.h"
#include "OcpiContainerApi.h"

namespace OCPI {
  namespace Container {
    class ApiError : public OCPI::Util::EmbeddedException, public OCPI::API::Error {
    public:
      ApiError(const char *err, ...);
    };

    inline unsigned long roundup(unsigned long n, unsigned long grain) {
      return (n + grain - 1) & ~(grain - 1);
    }
    unsigned long getNum(const char *s);
    // Used for application and infrastructure WCI things.
    unsigned long getAttrNum(ezxml_t x, const char *attr, bool missingOK = false, bool *found = 0);
    const unsigned
      DEFAULT_NBUFFERS = 2,
      DEFAULT_BUFFER_SIZE = 2*1024;
  }
}
#endif



