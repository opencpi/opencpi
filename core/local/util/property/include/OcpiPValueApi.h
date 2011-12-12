
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
 * Abstact:
 *   This file defines the PValue class for types property value lists
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_PVALUE_API_H
#define OCPI_PVALUE_API_H

#include "OcpiUtilDataTypesApi.h"

namespace OCPI {
  
  namespace API {
    // Convenience class for type safe property values for "simplest" api.
    // Only non-type-safe aspect is PVEnd()
    // Typical syntax would be like:
    // PValue props[] = { PVString("label", "foolabel"), PVULong("nbuffers", 10), PVEnd()};

    class PValue {
    public:
      inline PValue(const char *aName, BaseType aType)
	: name(aName), type(aType) {}
      inline PValue()
	: name(0), type(OCPI_none) {}
      unsigned length() const;
      const char *name;
      BaseType type;
      // Anonymous union here for convenience even though redundant with ValueType.
      union {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) run v##pretty;
	OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      };
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) friend class PV##pretty;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE  
    };
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)   \
    class PV##pretty : public PValue {				 \
    public:							 \
      inline PV##pretty(const char *aname, const run val = 0) :	 \
      PValue(aname,						 \
	     OCPI_##pretty/*,					 \
			    sizeof(v##pretty)*/) {		 \
	v##pretty = (run)val;					 \
      }								 \
    };
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
    // an experiment: working maybe too hard to avoid std::vector and/or std__auto_ptr
    class PVarray {
      PValue *_p;
    public:
      inline PVarray(unsigned n) : _p(new PValue[n]){}
      inline PValue &operator[](size_t n) { return *(_p + n); }
      ~PVarray() { delete [] _p; }
      operator PValue*() const { return _p; }
    };
    extern PVULong PVEnd;
  }
} // OCPI
#endif

