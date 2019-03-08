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

namespace OCPI {
  namespace API {
    // This class is only exposed as a pointer in the API, hence it can be defined with OCPI::Util
    class PropertyInfo : public OCPI::Util::Member {
    public:
      PropertyInfo();
      // Spec-level attributes - allowable in normal specs
      bool m_isWritable, m_isInitial, m_isParameter, m_isVolatile;
      // Worker-level attributes that are needed in API-level code
      bool
	m_readSync,   // tell worker when/before it is read
	m_writeSync,  // tell worker when/after it has been written
	m_readError,  // the worker mightr produce an error when it is read
	m_writeError, // the worker might produce an error when it is written
	m_isHidden,   // Should not be dumped by default
	m_isDebug,    // Should only be included when debug parameter is true
	m_isImpl,     // is an impl property, not a spec property, should be isWorker
	m_isReadable; // summary: can the worker itself provide a value?  Readable uncached.
	              // 1) volatile or 2) not writable or 3) writable and readback.
    };
  }

  namespace Util {
    class Property : public OCPI::API::PropertyInfo {
    public:
      // Describe structure member that might be the whole property
      unsigned
	m_smallest,    // Smallest unit of storage
	m_granularity; // Granularity of smallest unit
      // Caller needs these to decide to do beforeQuery/afterConfigure
      bool
	m_isSub32,
	m_isPadding,
	m_isRaw,           // Is handled specially by some models
	m_rawSet,          // Was raw attr explicitly set?
	m_isTest,
	m_isUsed,          // track whether this (parameter) was used in an expression
        m_isReadback,      // the (settable) property has a readback path for reading what was written
	m_isIndirect,      // the property's address is not based on accumulating offsets
	m_isBuiltin,       // the value is produced by infrastructure, not worker
	// These below represent what was in the spec before potentially being modified by the OWD
	// m_specReadable is for legacy/deprecatred only since it is not valid in specs
	m_specParameter,   // AV-5137 these 4 members should be moved to a class that extends this one
	m_specWritable,
	m_specInitial,
	m_specReadable;
      size_t m_padBefore;    // add padding before this property
      size_t m_indirectAddr; // when isIndirect. zero means impl sets address
      unsigned long m_dataOffset;
      size_t m_paramOrdinal; // Among parameters, which position?
      bool   m_hasValue;     // A value is set that is not a default, but an immutable value
      // Scalability:
      bool m_readBarrier, m_writeBarrier;
#define OCPI_REDUCTIONS \
  OCPI_REDUCE(None) \
  OCPI_REDUCE(Min) \
  OCPI_REDUCE(Max) \
  OCPI_REDUCE(Sum) \
  OCPI_REDUCE(Product) \
  OCPI_REDUCE(And) \
  OCPI_REDUCE(Or) \
  OCPI_REDUCE(Xor) \
  OCPI_REDUCE(Average)
#define OCPI_REDUCE(x) x,
      enum Reduction { OCPI_REDUCTIONS ReductionLimit } m_reduction;

#undef OCPI_REDUCE
    public:
      Property();
      ~Property();
    private:
      const char
	*parseCheck(),
	*parseImplAlso(ezxml_t x),
	*parseAccess(ezxml_t prop, bool worker, bool addAccess);
    public:
      const char
	*parse(ezxml_t x, bool includeImpl, unsigned ordinal,
	       const IdentResolver *resolv = NULL),
	*parseImpl(ezxml_t x, const IdentResolver *resolv = NULL),
	*parse(ezxml_t x, unsigned ordinal),
	*parseValue(const char *unparsed, Value &value, const char *end = NULL,
		    const IdentResolver *resolv = NULL) const,
        *checkType(OCPI::API::BaseType ctype, unsigned n, bool write),
	*getValue(ExprValue &val),
	*offset(size_t &cumOffset, uint64_t &sizeofConfigSpace,
		const IdentResolver *resolv = NULL);
    };
  }
}
#endif
