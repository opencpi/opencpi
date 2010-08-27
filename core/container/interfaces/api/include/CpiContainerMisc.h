// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

// This file exposes the CPI user interface for workers and the ports they own.
#ifndef CPI_CONTAINER_WORKER_H
#define CPI_CONTAINER_WORKER_H
#include <stdint.h>
#include "ezxml.h"
#include "CpiWorker.h"

namespace CPI {
  namespace Container {
    class ApiError : public CPI::Util::EmbeddedException {
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



