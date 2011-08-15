
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


#include <memory>
#include <unistd.h>
#include <OcpiPValue.h>
#include <OcpiUtilPropertyType.h>

namespace OCPI {
  namespace API {
    PVULong PVEnd(0,0);
    unsigned PValue::length() const {
      unsigned n = 0;
      if (this)
	for (const PValue *p = this; p->name; p++, n++)
	  ;
      return n;
    }
  }
  namespace Util {
    PVULong PVEnd(0,0);
    static const PValue *
    find(const PValue* p, const char* name) {
      if (p)
	for (; p->name; p++)
	  if (!strcmp(p->name, name))
	    return p;
      return NULL;
    }
 
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca, corba, letter, bits, run, pretty, store)	\
    bool 							\
    find##pretty(const PValue* p, const char* name, run &value) {	\
      const PValue *fp = find(p, name);					\
      if (fp)								\
        if (fp->type == OCPI::API::OCPI_##pretty) {	\
          value = fp->v##pretty;					\
          return true;							\
	} else								\
	  throw ApiError("Property \"", name, "\" is not a ", #pretty, NULL); \
      return false;							\
    }
#define OCPI_DATA_TYPE_S(sca, corba, letter, bits, run, pretty, store)	\
    bool 							\
    find##pretty(const PValue* p, const char* name, const run &value) {	\
      const PValue *fp = find(p, name);					\
      if (fp)								\
        if (fp->type == OCPI::API::OCPI_##pretty) {	\
          value = fp->v##pretty;					\
          return true;							\
	} else								\
	  throw ApiError("Property \"", name, "\" is not a ", #pretty, NULL); \
      return false;							\
    }
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
  }
}
