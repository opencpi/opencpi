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

#include <stdarg.h>
#include <string>
#include <cassert>
#include "ezxml.h"
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilDataTypes.h"

namespace OCPI {
  namespace Util {
    class Value;
    typedef Value **StructValue;
    // A type value is a pointer to a value
    typedef Value *TypeValue;
    // An enum value is a uint32_t
    typedef uint32_t EnumValue;

    struct Unparser {
      virtual ~Unparser();
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      virtual bool unparse##pretty(std::string &s, run, bool hex) const;
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
#undef OCPI_DATA_TYPE
      virtual bool unparseEnum(std::string &s, EnumValue val, const char **enums, size_t nEnums,
			       bool) const;
      virtual bool unparseStruct(std::string &fs, StructValue val, Member *members,
				 size_t nMembers, bool hex, char comma) const;
      void doFormat(std::string &, const char *fmt, ...) const;
      virtual bool
      elementUnparse(const Value &v, std::string &s, unsigned nSeq, bool hex, char comma,
		     bool wrap, const Unparser &up) const;
      virtual bool
      valueUnparse(const Value &v, std::string &s, unsigned nSeq, size_t nArray, bool hex,
		   char comma, bool wrap, const Unparser &up) const;
      virtual bool
      dimensionUnparse(const Value &v, std::string &s, unsigned nseq, size_t dim, size_t offset,
			 size_t nItems, bool hex, char comma, const Unparser &up) const;
    };
    extern bool parseOneChar(const char *&cp, const char *end, char &vp);
    struct IdentResolver;
    // A typed value
    class Value : public Unparser {
      static const ValueType *s_vt;
    public:
      const ValueType *m_vt;
      static OS::Mutex s_mutex;
      static Value *s_parent;
      size_t
	m_nElements,        // How many in the sequence?
	m_nTotal;           // Now many total? (including arrays).
      // allocated space for array of strings, and running pointer during parsing
      char *m_stringSpace, *m_stringNext;
      size_t m_stringSpaceLength;
      // allocated space for an array of Value ptrs that might be zero for missing members
      Value **m_struct, **m_structNext;
      // allocated space for OCPI_Type - a contiguous array of value objects
      Value *m_types;
      mutable Value *m_typeNext;
      const Value *m_parent;       // for navigating upward
      mutable unsigned m_next;       // for navigating horizontally
      size_t m_length;     // for debugging - length of value buffer
      bool m_parsed;
      Value(const ValueType &vt, const Value* parent = Value::s_parent);
      Value();
      virtual ~Value();
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
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
      const char *parse##pretty(const char*cp, const char *end, run &vp);
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      static const char *parse##pretty(const char*cp, const char *end, run &vp);
	OCPI_PROPERTY_DATA_TYPES

        OCPI_DATA_TYPE_S(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE_S(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE_S(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE
	  const char *parse(const char *unparsed, const char *stop = NULL, bool add = false,
			    const IdentResolver *resolv = NULL, bool *isVariable = NULL);
      const char *allocate(bool add = false);
      char &nextStringChar() {
	assert(m_stringNext && (size_t)(m_stringNext - m_stringSpace) < m_stringSpaceLength);
	return *m_stringNext++;
      }
      char setNextStringChar(char c) {
	nextStringChar() = c;
	return c;
      }
      bool needsComma() const;
      bool needsCommaDimension() const;
      bool needsCommaElement() const;
      void
        reserveStringSpace(size_t len, bool add),
	generate(),
	generateElement(unsigned nSeq),
	generateDimension(unsigned nseq, size_t dim, size_t offset, size_t nItems),
	generateValue(unsigned nSeq, size_t nArray);
      const char *getValue(ExprValue &val) const;
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      run generate##pretty();
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      bool
	  unparse(std::string &s, const Unparser *up = NULL, bool append = false,
		  bool hex = false, char comma = ',') const,
	  unparseElement(std::string &s, unsigned nSeq, bool hex, char comma,
			 const Unparser &up) const;
      // return TRUE if value is an empty value
      bool unparseValue(std::string &s, unsigned nSeq, size_t nArray, bool hex, char comma,
			const Unparser &up) const;
#if 0
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      bool unparse##pretty(std::string &s, run, bool hex) const;
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      bool unparseStruct(std::string &fs, StructValue val, bool hex, char comma) const;
#endif
    private:
      const char
	*parseExpressionValue(const char *start, const char *end, size_t nSeq, size_t nArray),
	*parseValue(const char *unparsed, const char *stop, size_t nSeq, size_t nArray),
	*parseElement(const char *start, const char *end, size_t nSeq),
	*parseDimension(const char *unparsed, const char *stop,
			size_t nseq, size_t dim, size_t offset, size_t nItems);
      void clear();
      void clearStruct();
    };
  }
}
#endif
