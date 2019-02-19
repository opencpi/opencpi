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

/*
 * Abstract:
 *   This file defines the PValue class for types property value lists
 *
 * Revision History:
 *
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_PVALUE_H
#define OCPI_PVALUE_H

#include <cstring>
#include <cstdarg>
#include "ezxml.h"
#include "OcpiUtilException.h"
#include "OcpiPValueApi.h"


namespace OCPI {
  namespace Util {
    typedef OCPI::API::PValue PValue;
    extern PValue allPVParams[];
#define OCPI_DATA_TYPE(sca, corba, letter, bits, run, pretty, store) \
    void add##pretty(const PValue *&p, const char *name, run value); \
    bool find##pretty(const PValue *p, const char *name, run &value);
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
    bool
      findAssign(const PValue *p, const char *name, const char *var, const char *&value),
      findAssign(const PValue *p, const char *name, const char *var, std::string &value),
      findAssignNext(const PValue *p, const char *name, const char *var, const char *&val,
		     unsigned &next);
      const PValue *find(const PValue *p, const char *name);
      PValue *find(PValue *p, const char *name);
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
    typedef OCPI::API::PV##pretty PV##pretty;
  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
    extern PVULong PVEnd;
    // A storage-managed list that originate from xml and can be used anywhere
    // that a PValue * can be used.  The values come from attributes.
    class PValueList {
      PValue *m_list;
      const char *vParse(const PValue *p, ezxml_t x, std::va_list ap);
    public:
      PValueList(const PValue *params, const PValue *override = NULL);
      PValueList();
      PValueList(const PValueList&);
      PValueList(const PValueList*);
      ~PValueList();
      PValueList &operator=(const PValueList & p);
      inline const PValue *list() const { return m_list; }
      inline operator const PValue*() const { return m_list; }
      const char
	*addXml(ezxml_t x),
	*parse(ezxml_t x, ...),
	*parse(const PValue *p, ezxml_t x, ...),
	*add(const char *name, const char *value, bool override = false);
      void
	add(const PValue *params, const PValue *override = NULL),
	add(const PValue &param);
#define OCPI_DATA_TYPE(sca, corba, letter, bits, run, pretty, store) \
      void add##pretty(const char *name, run value);
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
    };
  }
} // OCPI
#endif

