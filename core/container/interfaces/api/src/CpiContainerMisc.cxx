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

#include <errno.h>
#include <cstdarg>
#include "CpiContainerInterface.h"
#include "CpiPValue.h"
#include "CpiContainerPort.h"
#include "CpiContainerMisc.h"
#include "CpiDriver.h"
#include <CpiContainerErrorCodes.h>

using namespace CPI::Util;

namespace CPI {
  namespace Container {

// Convenience for single line, multi-string, API exceptions (API called badly)
// Its easy to scan all callers for the terminating null
    ApiError::ApiError(const char *err, ...) :
      CPI::Util::EmbeddedException(APPLICATION_EXCEPTION, 0, ApplicationFatal) {
      va_list ap;
      va_start(ap, err);
      m_auxInfo = err;
      const char *s;
      while ((s = va_arg(ap, const char*)))
        m_auxInfo += s;
    }
    unsigned long getNum(const char *s) {
      char *endptr;
      errno = 0;
      unsigned long val =  strtoul(s, &endptr, 0);
      if (errno == 0) {
        if (*endptr == 'K' || *endptr == 'k')
          val *= 1024;
        return val;
      }
      return ULONG_MAX;
    }
    // Used for application and infrastructure WCI things.
    unsigned long getAttrNum(ezxml_t x, const char *attr, bool missingOK, bool *found) {
      const char *a = ezxml_cattr(x, attr);
      if (found)
        *found = false;
      if (a) {
        unsigned long val = getNum(a);
        if (val != ULONG_MAX) {
          if (found)
            *found = true;
          return val;
        }
      } else if (missingOK)
        return 0;
      throw ApiError("Attribute value \"", attr, a ? "\" invalid: \"" : "\" missing", a, "\"", 0);
    }
    
    
  }
}
