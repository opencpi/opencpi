
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
 * Non-API datatype declarations, for protocols and properties etc.
 */
#ifndef OCPI_UTIL_VALUE_H
#define OCPI_UTIL_VALUE_H

#include "ezxml.h"
#include <stdarg.h>
#include <string>
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilDataTypes.h"
#include "OcpiExprEvaluator.h"

namespace OCPI {
  namespace Util {
    class Value;
    typedef Value **StructValue;
    // A type value is a pointer to a value
    typedef Value *TypeValue;
    // An enum value is a uint32_t
    typedef uint32_t EnumValue;
    // A typed value
    class Value {
      static const ValueType *s_vt;
    public:
      const ValueType *m_vt;
      static OS::Mutex s_mutex;
      static Value *s_parent;
      unsigned
	m_nElements,        // How many in the sequence?
	m_nTotal;           // Now many total? (including arrays).
      // allocated space for array of strings, and running pointer during parsing
      char *m_stringSpace, *m_stringNext;
      unsigned m_stringSpaceLength;
      // allocated space for an array of Value ptrs that might be zero for missing members
      Value **m_struct, **m_structNext;
      // allocated space for OCPI_Type - a contiguous array of value objects
      Value *m_types, *m_typeNext;
      Value *m_parent;       // for navigating upward
      unsigned m_next;       // for navigating horizontally
      unsigned m_length;     // for debugging - length of value buffer
      Value(const ValueType &vt, Value* parent = Value::s_parent);
      Value();
      ~Value();
      void setType(const ValueType &vt);
    private:
      void init();
    public:
      // The value is either the thing itself (e.g. a float)
      // Or for a sequence of floats it is a pointer
      // Or for a sequence of arrays of floats it is a pointer-to-pointer
      union {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
        run m_##pretty, *m_p##pretty;
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      };
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      const char *parse##pretty(const char*cp, const char *end, run &vp);
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      const char *parse(const char *unparsed, const char *stop = NULL);
      const char *allocate();
      bool needsComma() const;
      bool needsCommaDimension() const;
      bool needsCommaElement() const;
      void
	generate(),
	generateElement(unsigned nSeq),
	generateDimension(unsigned nseq, unsigned dim, unsigned offset, unsigned nItems),
	generateValue(unsigned nSeq, unsigned nArray);
      const char *getValue(ExprValue &val);
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      run generate##pretty();
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      void
	unparse(std::string &s, bool append = false) const,
	unparseElement(std::string &s, unsigned nSeq) const;
      bool
	unparseDimension(std::string &s, unsigned nseq, unsigned dim, unsigned offset,
			 unsigned nItems) const;
      // return TRUE if value is an empty value
      bool unparseValue(std::string &s, unsigned nSeq, unsigned nArray) const;
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      bool unparse##pretty(std::string &s, run ) const;
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
    private:
      const char
	*parseValue(const char *unparsed, const char *stop,
		   unsigned nSeq, unsigned nArray),
	*parseElement(const char *start, const char *end, unsigned nSeq),
	*parseDimension(const char *unparsed, const char *stop,
			unsigned nseq, unsigned dim, unsigned offset, unsigned nItems);
      void clear();
      void clearStruct();
    };
  }
}
#endif
