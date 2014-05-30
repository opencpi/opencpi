
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

 Definitions for worker metadata encoding,decoding.
 Offline, in tools, this information is encoded into a string format 
 suitable for program level arguments (argv).  All properties are encoded into
 a single string.  This relieves runtime of any parsing overhead (time or space)
 or dependencies on external parsing libraries.

 The "scope" of this property support is configuration properties for CP289
 components.  Thus it is not (yet) intended to support SCA GPP components.

 This file defines the binary (non-string) format of SCA component properties,
 as well as the functions to encode (binary to string) and decode 
 (string to binary).

 
*/
#ifndef OCPI_METADATA_PROPERTY_H
#define OCPI_METADATA_PROPERTY_H

#include "ezxml.h"
#include "OcpiUtilDataTypes.h"
#include "OcpiUtilPropertyApi.h"
#include "OcpiExprEvaluator.h"

namespace OCPI {
  namespace API {
    // This class is only exposed as a pointer in the API, hence it can be defined with OCPI::Util
    class PropertyInfo : public OCPI::Util::Member {
    public:
      PropertyInfo();
      bool m_readSync, m_writeSync, m_isWritable, m_isReadable, m_readError, m_writeError,
	   m_isVolatile, m_isInitial, m_isIndirect;
      size_t m_indirectAddr; // when isIndirect. zero means impl sets address
    };
  }

  namespace Util {
    class Property : public OCPI::API::PropertyInfo {
      const char
	*parseImplAlso(ezxml_t x),
	*parseAccess(ezxml_t prop, bool &readableConfigs, bool &writableConfigs, bool addAccess);
    public:
      Property();
      ~Property();
      // Describe structure member that might be the whole property
      unsigned 
	m_smallest,    // Smallest unit of storage
	m_granularity; // Granularity of smallest unit
      // Caller needs these to decide to do beforeQuery/afterConfigure
      bool
	m_isDebug,         // Should only be included when debug parameter is true
	m_isParameter,     // For compile-time parameter
	m_isSub32,
	m_isTest;
      unsigned long m_dataOffset;
      size_t m_paramOrdinal; // Among parameters, which position?
      // Sizes in bits of the various types
      const char *parse(ezxml_t x,
			bool &readableConfigs,
			bool &writableConfigs,
			bool &sub32Configs,
			bool includeImpl,
			unsigned ordinal);
      const char *parseImpl(ezxml_t x, bool &readableConfigs, bool &writableConfigs);
      const char *parse(ezxml_t x, unsigned ordinal);
      const char *parseValue(const char *unparsed, Value &value, const char *end = NULL) const;
      void offset(size_t &cumOffset, uint64_t &sizeofConfigSpace);
      // Check when accessing with scalar type and sequence length
      const char *checkType(OCPI::API::BaseType ctype, unsigned n, bool write);
      const char *getValue(ExprValue &val);
    private:
    };
  }
}
#endif
