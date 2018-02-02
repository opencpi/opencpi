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

// The classes that represent data types and serializers/deserializers

#ifndef OCPI_UTIL_DATA_TYPES_H
#define OCPI_UTIL_DATA_TYPES_H

#include "ezxml.h"
#include <stdarg.h>
#include <string>
#include <vector>
#include "OcpiExprEvaluator.h"
#include "OcpiUtilDataTypesApi.h"

namespace OCPI {
  namespace Util {
    // Since arrays are part of the declarator, and not the type, a member with both
    // sequence and array characteristics is a sequence of arrays, not an array of sequences.
    // I.e. if it is an array of sequences, then the member must refer to a subsidiary type,
    // roughly like a typedef. (the BaseType is then OCPI_Type)
    class Member;

    // Simple internal class to simplify copy constructor and assignment operator.
    struct ValueTypeInternal {
      // FIXME: We could make more members private or protected.
      OCPI::API::BaseType m_baseType; // The basic type - scalar, string, struct or typedef
      // Array info valid when by m_arrayRank != 0
      size_t m_arrayRank;
      std::vector<std::string> m_arrayDimensionsExprs;
      // Sequence info when m_isSequence is true
      bool m_isSequence;         // Are we a sequence?
      size_t m_sequenceLength;   // maximum length for sequences, zero is unbounded
      std::string m_sequenceLengthExpr;
      // Structure info valid when m_baseType is OCPI_Struct
      size_t m_nMembers;         // Length of m_members.
      // String info valid when m_baseType is OCPI_String
      // mutable since it may be changed from zero to the longest actual string length
      // of any string in a value (including arrays/sequences) see Value::maxStringLength()
      mutable size_t m_stringLength;     // maximum strlen (null not included, like strlen)
      std::string m_stringLengthExpr;
      // Enum info valid when m_baseType is OCPI_Enum
      size_t m_nEnums;
      // Alignment/sizing info
      size_t m_dataAlign;        // alignment of data for this type, independent of (may be less
                                 // than) the type's overall alignment, e.g. when a sequence
                                 // of shorts with initial count must be uint32_t aligned,
                                 // the m_dataAlign would be 2 while the m_align would be 4
      size_t m_align;            // The alignment requirement for this complete type
      size_t m_nBits;            // bits per element FIXME: is this redundant with mBytes
      size_t m_elementBytes;     // nytes per element (in arrays or if not array, in sequences
      size_t m_nBytes;           // total bytes (if bounded), minimum bytes if unbounded      
      size_t m_nItems;           // total number of fixed items ?for arrays?
      bool m_fixedLayout;        // is this type fixed length in its parent?
      bool m_usesParameters;     // is this type dependent on parameters in its exprs?

      std::string m_typeDef;     // FIXME: only for OCPIDDS Support, should be moved back there.
      std::string m_format;      // placeholder for formatting attribute for this type
      ValueTypeInternal(OCPI::API::BaseType bt, bool isSequence);
    };
    // The ValueType class (which could be DataType) represents the data type of something, and
    // not its value.  It is not named.  It has the base scalar type (from the list in
    // *DataTypesApi.h) or also extended types:  enum, struct, and recursively "type".  This
    // class has other generic attributes like array dimensions and sequence bounds.  The concept
    // is roughly based on the type declarators of the CORBA IDL type system, which is C-ish
    // also.  In particular, if it has sequence aspects and array aspects, it is a sequence of
    // arrays, not an array of sequences.  The "type" type, is a recursive type that opens the
    // typing system up to more complex types like arrays of sequences.  This data type system is
    // used for two things at a high level:  property structures of multiple properties and
    // protocol messages. The two variable length aspects are strings and sequences. If you are
    // using this class for properties, both variable aspects must be bounded, and the
    // alignment/offset/layout leaves room for the bounded size.  When used for messages
    // (serialization/deserialization/CDR etc.), the bounding is optional, and the layout is
    // variable even when bounded.
    class ValueType : public ValueTypeInternal {
    public:
      size_t *m_arrayDimensions; // for an array, m_arrayRank long
      Member *m_members;         // for m_baseType==OCPI_Struct, m_nMembers long
      Member *m_type;            // for m_baseType==OCPI_Type, points to type it is based on
      const char **m_enums;      // for m_baseType==OCPI_Enum, m_nEnums+1 long
      // Rule of 3 with copy swap idiom (makes a temp when assigning)
      ValueType(OCPI::API::BaseType bt = OCPI::API::OCPI_none, bool isSequence = false);
      ValueType(const ValueType &other);
      ValueType &operator=(ValueType other);
      friend void swap(ValueType& first, ValueType& second);
      virtual ~ValueType(); // not expected to be virtually destructed, but is sometimes concrete
      bool isSequence() const { return m_isSequence; }
      // This isFixed method is used in two ways.  Inelegantly overloaded.
      // If top is true, it is asking whether this type is EITHER fixed or a sequence of
      // fixed size things.  I.e. suitable for direct use in message buffers w/o seq length
      // If top is false, it is just asking whether the type is fixed in size.
      bool isFixed(bool top = true) const;
      bool needsComma() const {
	return m_isSequence || m_arrayRank != 0 || m_baseType == OCPI::API::OCPI_Struct;
      }
      bool needsCommaDimension() const {
	return m_baseType == OCPI::API::OCPI_Struct;
      }
      bool needsCommaElement() const {
	return m_arrayRank != 0 || m_baseType == OCPI::API::OCPI_Struct;
      }
      // This value is an element of a sequence.  Does it need to be wrapped in braces?
      bool needsNewLineBraces() const {
	return (m_isSequence && m_arrayRank) ||
	  (!m_isSequence && m_arrayRank > 1) ||
	  (m_baseType == OCPI::API::OCPI_Struct && (m_isSequence || m_arrayRank));
      }
    };
    const unsigned testMaxStringLength = 10;
    const unsigned maxDataTypeAlignment = sizeof(double); // max of all types we support

    union WriteDataPtr {
      const uint8_t *data;
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,u,bits,run,pretty,store) const run *p##pretty;
#define OCPI_DATA_TYPE_S(sca,corba,u,bits,run,pretty,store) const run p##pretty;
      OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE
    }; 

    union ReadDataPtr {
      uint8_t *data;
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,u,buts,run,pretty,store) run *p##pretty;
#define OCPI_DATA_TYPE_S(sca,corba,u,buts,run,pretty,store) run p##pretty;
      OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE
    };

    class Writer {
    protected:
      Writer();
      virtual ~Writer();
    public:
      virtual void
	writeOpcode(const char *name, uint8_t opcode),
	beginSequence(Member &m, size_t nElements) = 0,
	beginArray(Member &m, size_t nItems),
	endArray(Member &m),
	endSequence(Member &m),
	beginStruct(Member &m),
	endStruct(Member &m),
	beginType(Member &m),
	endType(Member &m),
	writeString(Member &m, WriteDataPtr p, size_t strLen, bool start, bool top) = 0,
	writeData(Member &m, WriteDataPtr p, size_t nBytes, size_t nElements) = 0,
	end();
    };

    class Reader {
    protected:
      Reader();
      virtual ~Reader();
    public:
      virtual size_t
	beginSequence(const Member &m) = 0,
	beginString(const Member &m, const char *&chars, bool first) = 0;
      virtual void
	beginArray(const Member &m, size_t nItems),
	endArray(const Member &m),
	endSequence(const Member &m),
	endString(const Member &m),
	beginStruct(const Member &m),
	endStruct(const Member &m),
	beginType(const Member &m),
	endType(const Member &m),
	readData(const Member &m, ReadDataPtr p, size_t nBytes, size_t nElements,
		 bool fake = false) = 0,
	end();
    };
    // There are the data type attributes allowed for members
#define OCPI_UTIL_MEMBER_ATTRS						\
    "Name", "Type", "StringLength", "SequenceLength", "ArrayLength", "ArrayDimensions", "Key", "Enums", "Description"
#define OCPI_UTIL_MEMBER_ELEMENTS "description"

    // A "member" is used for structure members, operation arguments, exception members, 
    // and properties.  Members have names, and offsets in their group, and possibly a
    // default value
    class Value;
    // Member is a named data type, which may also have a default value, it inherits ValueType.
    // The "named" aspect is used for members of structures as well as other things
    // (hence the recursion - if the data type is a struct, then it has children that are
    // Members.)  Members also have "descriptions", an abbreviation, an ordinal in a group, 
    // and an offset within a group (calculated with OMG CDR rules as extended according to 
    // section 7.3 of the CDK doc.)
    class Member : public ValueType {
    public:
      std::string m_name, m_abbrev, m_pretty, m_description;
      size_t m_offset;               // within group
      bool m_isIn, m_isOut, m_isKey; // for args (could use another class, but not worth it)
      Value *m_default;              // A default value, if one is appropriate and there is one
      std::string m_defaultExpr;
      unsigned m_ordinal;            // ordinal within group
      Member();
      Member(const char *name, const char *abbrev, const char *description,
	     OCPI::API::BaseType type, bool isSequence, const char *defaultValue);
      Member(const Member& other);
      Member& operator=(Member other);
      friend void swap(Member& first, Member& second);
      virtual ~Member(); // is concrete sometimes, but we don't expect virtual deletion
      Member &sequenceType() const;
      void printAttrs(std::string &out, const char *tag, unsigned indent = 0, bool suppressDefault = false);
      void printChildren(std::string &out, const char *tag, unsigned indent = 0);
      void printXML(std::string &out, const char *tag, unsigned indent);
      void write(Writer &writer, const uint8_t *&data, size_t &length, bool topSeq = false);
      // Fake means don't actually touch the message.
      void read(Reader &reader, uint8_t *&data, size_t &length, bool fake = false,
		bool top = false) const;
      void generate(const char *name, unsigned ordinal = 0, unsigned depth = 0);
      //      const std::string &name() const { return m_name; }
      const char *cname() const { return m_name.c_str(); }
      const char *pretty() const { return m_pretty.c_str(); }
      const char
        *finalize(const IdentResolver &resolv, const char *tag, bool isFixed),
	*parseDefault(const char *value, const char *tag, const IdentResolver *resolv = NULL),
	*parse(ezxml_t x, bool isFixed, bool hasName, const char *hasDefault, const char *tag,
	       unsigned ordinal, const IdentResolver *resolv = NULL),
	*offset(size_t &maxAlign, size_t &argOffset, size_t &minSize, bool &diverseSizes,
		bool &sub32, bool &unBounded, bool &isVariable, bool isTop = false);
      uint8_t *getField(uint8_t *data, size_t &length) const;
      static const char
	*parseMembers(ezxml_t prop, size_t &nMembers, Member *&members, bool isFixed,
		     const char *tag, const char *vtag, const IdentResolver *resolv = NULL),
	*alignMembers(Member *m, size_t nMembers, size_t &maxAlign, size_t &myOffset,
		     size_t &minSize, bool &diverseSizes, bool &sub32, bool &unBounded,
		     bool &isVariable, bool isTop = false);
    };
    // These are indexed by the BaseType
    extern const char *baseTypeNames[];
    extern const char *idlTypeNames[];
    extern unsigned baseTypeSizes[];
  }
}
#endif
