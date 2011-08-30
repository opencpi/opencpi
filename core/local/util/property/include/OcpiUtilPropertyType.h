
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


// This file defines runtime data types for properties, property value lists,
// parsing, printing etc.



#ifndef OCPI_UTIL_PROPERTY_TYPES_H
#define OCPI_UTIL_PROPERTY_TYPES_H
#include <stdarg.h>
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS // wierd standards goof up
#endif
#include <stdint.h>
#include "OcpiPropertyTypesApi.h"

namespace OCPI {
  namespace Util {
    namespace Prop {
      namespace Scalar {
	typedef OCPI::API::ScalarType Type;
	typedef OCPI::API::Value Value;
	const Type OCPI_none = OCPI::API::OCPI_none;
	const Type OCPI_scalar_type_limit = OCPI::API::OCPI_scalar_type_limit;
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) const Type OCPI_##pretty = OCPI::API::OCPI_##pretty;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	
	extern uint8_t sizes[];
	extern const char *names[];
      }
      typedef OCPI::API::ValueType ValueType;
#if 0
      const char *parseValue(ValueType &vt, const char *value0, OCPI::API::ScalarValue &value1);
      void destroyValue(ValueType &vt, OCPI::API::ScalarValue &value);
#endif
    }
    // These obviously do not belong here and needs storage management
#define myCalloc(t, n) ((t *)calloc(sizeof(t), (n)))
    inline unsigned long roundup(unsigned long n, unsigned long grain) {
      return (n + grain - 1) & ~(grain - 1);
    }
    extern bool parseBool(const char *a, unsigned length, bool *b);
  }
}
#if 0
const char *esprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *buf;
  vasprintf(&buf, fmt, ap);
  va_end(ap);
  return buf;
}
#endif
#endif
