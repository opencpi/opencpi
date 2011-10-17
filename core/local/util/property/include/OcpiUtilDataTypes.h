
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
#ifndef OCPI_UTIL_DATA_TYPES_H
#define OCPI_UTIL_DATA_TYPES_H

#include "ezxml.h"
#include <stdarg.h>
#include <string>
#include "OcpiUtilDataTypesApi.h"

namespace OCPI {
  namespace Util {
    // Since arrays are part of the declarator, and not the type, a member with both
    // sequence and array characteristics is a sequence of arrays, not an array of sequences.
    // I.e. if it is an array of sequences, then the member must refer to a subsidiary type,
    // roughly like a typedef. (the BaseType is then OCPI_Type)
    class Member;
    // The class that defines the type of a data value.  Could have been called DataType.
    class ValueType {
    public:
      OCPI::API::BaseType m_baseType; // The basic type - scalar, string, struct or typedef
      unsigned
        m_arrayRank,                  // If > 0, we have an array
	m_nMembers,                   // For structs
	m_nBits,
	m_align;                      // The alignment requirement for this type
      bool m_isSequence;              // Are we a sequence?
      uint32_t
        m_nBytes,                     // Total bytes (if bounded), minimum bytes if unbounded
	*m_arrayDimensions,           // != NULL for an array, m_arrayRank long
        m_stringLength,               // maximum strlen (null not included, like strlen)
	m_sequenceLength;             // maximum length for sequences, zero is unbounded
      Member *m_members;              // if a struct, these are the members
      Member *m_type;                 // if a recursive type, that type
      const char **m_enums;
      uint32_t m_nEnums;
      // unions and enums
      ValueType(OCPI::API::BaseType bt = OCPI::API::OCPI_none);
      ~ValueType();
    };
    // A struct value is a pointer to an array of pointers to values (to be sparse)
    class Value;
    typedef Value **StructValue;
    // A type value is a pointer to a value
    typedef Value *TypeValue;
    // An enum value is a uint32_t
    typedef uint32_t EnumValue;
    // A typed value
    class Value { //: public ValueType {
      const ValueType &m_vt;
    public:
      unsigned m_nElements;        // How many sequence values?
      // space for array of strings, and running pointer during parsing
      char *m_stringSpace, *m_stringNext;
      Value(const ValueType &vt);
      ~Value();
      // The value is either the thing itself (e.g. a float)
      // Or for a sequence of floats it is a pointer
      // Or for a sequence of arrays of floats it is a pointer-to-pointer
      union {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
        run m_##pretty, *m_p##pretty, **m_pp##pretty;
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      };
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      bool parse##pretty(const char*cp, const char *end, run &vp);
	OCPI_PROPERTY_DATA_TYPES
        OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      const char *parse(const char *unparsed, const char *stop = NULL);
      void unparse(std::string &) const;
    private:
      const char
	*parseValue(const char *unparsed, const char *stop,
		   unsigned nSeq, unsigned nArray),
	*parseElement(const char *start, const char *end, unsigned nSeq),
	*parseDimension(const char *unparsed, const char *stop,
		       unsigned nseq, unsigned dim, unsigned offset);
      void clear();
    };
    // There are the data type attributes allowed for members
#define OCPI_UTIL_MEMBER_ATTRS						\
    "Name", "Type", "StringLength", "SequenceLength", "ArrayLength", "ArrayDimensions", "Key", "Enums"

    // A "member" is used for structure members, operation arguments, exception members, 
    // and properties.  Members have names, and offsets in their group, and possibly a
    // default value
    class Member : public ValueType {
    public:
      uint32_t m_offset;              // in group
      std::string m_name;
      bool m_isIn, m_isOut, m_isKey;  // for arguments (could use another class, but not worth it)
      Value *m_defaultValue;          // A default value, if one is appropriate and there is one
      Member();
      virtual ~Member();
      void printAttrs(FILE *f, const char *tag, unsigned indent = 0);
      void printChildren(FILE *f, const char *tag, unsigned indent = 0);
      void printXML(FILE *f, const char *tag, unsigned indent);
      const char *
      parse(ezxml_t xp, unsigned &maxAlign, uint32_t &argOffset,
	    unsigned &minSize, bool &diverseSizes, bool &sub32, bool &unBounded,
	    bool isFixed, bool hasName, bool hasDefault);
      static const char *
      parseMembers(ezxml_t prop, unsigned &nMembers, Member *&members,
		   unsigned &maxAlign, uint32_t &myOffset,
		   unsigned &minSize, bool &diverseSizes, bool &sub32, bool &unBounded,
		   const char *tag, bool isFixed, bool hasDefault);
    };
    // These two are indexed by the BaseType
    extern const char *baseTypeNames[];
    extern const char *idlTypeNames[];
    extern unsigned baseTypeSizes[];
    // FIXME: put this somewhere more sensible.
    inline unsigned long roundup(unsigned long n, unsigned long grain) {
      return (n + grain - 1) & ~(grain - 1);
    }
  }
}
#endif
