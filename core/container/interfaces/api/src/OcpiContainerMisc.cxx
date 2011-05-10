
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

#include <climits>
#include <errno.h>
#include <cstdarg>
#include "OcpiContainerInterface.h"
#include "OcpiPValue.h"
#include "OcpiContainerPort.h"
#include "OcpiContainerMisc.h"
#include <OcpiContainerErrorCodes.h>

using namespace OCPI::Util;

namespace OCPI {
  namespace Container {
    // Convenience for single line, multi-string, API exceptions (API called badly)
    // Its easy to scan all callers for the terminating null
    ApiError::ApiError(const char *err, ...)
      : OCPI::Util::EmbeddedException(OCPI::Container::APPLICATION_EXCEPTION, "", OCPI::Container::ApplicationFatal)
    {
      va_list ap;
      va_start(ap, err);
      m_auxInfo = err;
      const char *s;
      while ((s = va_arg(ap, const char*)))
        m_auxInfo += s;
      m_error = m_auxInfo; // FIXME later
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
  API::Error::Error(){}
}
