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

#include <string.h>
#include <assert.h>
#include <climits>
#include <set>

#include "OcpiOsAssert.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "OcpiUtilDataTypes.h"
#include "OcpiUtilValue.h"

namespace OCPI {
  namespace Util {
    namespace OA = OCPI::API;
    namespace OE = OCPI::Util::EzXml;

    ValueTypeInternal::ValueTypeInternal(OCPI::API::BaseType bt, bool isSequence)
      : m_baseType(bt), m_arrayRank(0), m_isSequence(isSequence), m_sequenceLength(0),
	m_nMembers(0), m_stringLength(0), m_nEnums(0), m_dataAlign(0), m_align(1), m_nBits(0),
	m_elementBytes(0), m_nBytes(0), m_nItems(1), m_fixedLayout(true), m_usesParameters(false)
    {}
    ValueType::ValueType(OA::BaseType bt, bool a_isSequence)
      : ValueTypeInternal(bt, a_isSequence),
	m_arrayDimensions(NULL), m_members(NULL), m_type(NULL), m_enums(NULL)
    {}
    // Using an inherited struct for default copy would be nice here
    ValueType::ValueType(const ValueType &other)
      : ValueTypeInternal(other),
	m_arrayDimensions(other.m_arrayDimensions ? new size_t[other.m_arrayRank] : NULL),
	m_members(other.m_nMembers ? new Member[other.m_nMembers] : NULL),
	m_type(other.m_type ? new Member(*other.m_type) : NULL), // recursion
	m_enums(other.m_enums ? new const char*[other.m_nEnums + 1] : NULL) {
      // Do the deep copies for array dimensions, struct members, enum strings
      std::copy(other.m_arrayDimensions, other.m_arrayDimensions + other.m_arrayRank,
		m_arrayDimensions);
      std::copy(other.m_members, other.m_members + other.m_nMembers, m_members);
      for (size_t i = 0; i < m_nEnums; i++) {
	char *p = new char[strlen(other.m_enums[i]) + 1];
	strcpy(p, other.m_enums[i]);
	m_enums[i] = p;
      }
      if (m_enums)
	m_enums[m_nEnums] = NULL;
    }
    // boiler plate copy-and-swap idiom delegating work to "swap"
    ValueType& ValueType::operator=(ValueType other) {
      swap(*this, other);
      return *this;
    }
    // Swap
    void swap(ValueType &f, ValueType &s) {
      using std::swap;
      swap<ValueTypeInternal>(f, s);
      swap(f.m_arrayDimensions, s.m_arrayDimensions);
      swap(f.m_members, s.m_members);
      swap(f.m_type, s.m_type);
      swap(f.m_enums, s.m_enums);
    }
    ValueType::~ValueType() {
      delete [] m_arrayDimensions;
      delete [] m_members;
      delete m_type;
      for (unsigned n = 0; n < m_nEnums; n++)
	delete [] m_enums[n];
      delete [] m_enums;
    }
    bool ValueType::isFixed(bool top) const {
      if (m_isSequence && !top)
	return false;
      switch (m_baseType) {
      case OA::OCPI_String:
      case OA::OCPI_Type:
	return false;
      case OA::OCPI_Struct:
	for (unsigned n = 0; n < m_nMembers; n++)
	  if (!m_members[n].isFixed(false))
	    return false;
      default:
	;
      }
      return true;
    }

    Member::
    Member() : m_offset(0), m_isIn(false), m_isOut(false), m_isKey(false), m_default(NULL)
    {
    }
    Member::
    Member(const Member &other)
      : ValueType(other), m_name(other.m_name), m_abbrev(other.m_abbrev), m_pretty(other.m_pretty),
	m_description(other.m_description), m_offset(0), m_isIn(false), m_isOut(false), m_isKey(false),
	m_default(NULL), m_defaultExpr(other.m_defaultExpr), m_ordinal(0) {
      if (other.m_default)
	m_default = new Value(*other.m_default);
    }
    // Constructor when you are not parsing, and doing static initialization
    Member::
    Member(const char *name, const char *abbrev, const char *description, OA::BaseType type,
	   bool a_isSequence, const char *defaultValue)
      : ValueType(type, a_isSequence), m_name(name), m_abbrev(abbrev ? abbrev : ""),
	m_description(description ? description : ""),
	m_offset(0), m_isIn(false), m_isOut(false), m_isKey(false), m_default(NULL) {
      if (defaultValue) {
	m_default = new Value(*this);
	ocpiCheck(m_default->parse(defaultValue) == 0);
      }
    }
    Member &Member::
    operator=(Member other){
      swap(*this, other);
      return *this;
    }
    void swap(Member& f, Member& s){
      using std::swap;
      swap<ValueType>(f, s);
      swap(f.m_offset, s.m_offset);
      swap(f.m_isIn, s.m_isIn);
      swap(f.m_isOut, s.m_isOut);
      swap(f.m_isKey, s.m_isKey);
      swap(f.m_default, s.m_default);
    }
    Member::~Member() {
      if (m_default)
	delete m_default;
    }
    // Return a type object that is a sequence of this type
    // This is not a member function of ValueType because hierarchical types
    // are based on members, which they should not be AFAICT
    Member &Member::
    sequenceType() const {
      Member &newType = *new Member(*this);
      if (m_isSequence) {
	Member &seqType = *new Member();
	seqType.m_baseType = OA::OCPI_Type;
	seqType.m_type = &newType;
	seqType.m_isSequence = true;
	return seqType;
      }
      // easy case - just a copy adding the "sequence" attribute.
      newType.m_isSequence = true;
      return newType;
    }

    // This is called during normal parsing of a member, but also used after initial parsing
    // of the member XML when a value is being overriden later.
    const char *Member::
    parseDefault(const char *defValue, const char *tag, const IdentResolver *resolv) {
      if (defValue) {
	delete m_default;
	m_default = new Value(*this);
	bool isVariable;
	const char *err = m_default->parse(defValue, NULL, false, resolv, &isVariable);
	if (err)
	  return esprintf("for %s %s: %s", tag, m_name.c_str(), err);
	if (isVariable)
	  m_defaultExpr = defValue;
      }
      // FIXME: if any children (struct or type) have defaults, build a sparse default here
      return NULL;
    }
    const char *Member::
    parse(ezxml_t xm, bool a_isFixed, bool hasName, const char *hasDefault, const char *tag,
	  unsigned ordinal, const IdentResolver *resolver) {
      bool found;
      const char *err;
      const char *name = ezxml_cattr(xm, "Name");
      m_ordinal = ordinal;
      if (name)
	m_name = name;
      else if (hasName)
	return "Missing Name attribute in Property/Argument/Member element";
      OE::getOptionalString(xm, m_abbrev, "Abbrev");
      OE::getOptionalString(xm, m_pretty, "Pretty");
      if (m_pretty.empty())
	m_pretty = m_name;
      OE::getOptionalString(xm, m_description, "Description");
      ezxml_t desc = ezxml_cchild(xm, "description");
      if (desc)
	m_description = ezxml_txt(desc);
      OE::unindent(m_description);
      OE::getOptionalString(xm, m_format, "Format");
      const char *typeName = ezxml_cattr(xm, "Type");
      if (!typeName)
	typeName = "ULong";
      if ((err = OE::getBoolean(xm, "Key", &m_isKey)))
	return err;
      if (!strcasecmp(typeName, "struct")) {
	m_baseType = OA::OCPI_Struct;
	if ((err = OE::checkElements(xm, OCPI_UTIL_MEMBER_ELEMENTS, "member", (void*)0)) ||
	    (err = parseMembers(xm, m_nMembers, m_members, a_isFixed, "member", hasDefault,
				resolver)))
	  return err;
	if (m_nMembers == 0)
	  return "No struct members under type == \"struct\"";
	for (unsigned n = 0; n < m_nMembers; n++)
	  if (!m_members[n].m_fixedLayout)
	    m_fixedLayout = false;
      } else if (!strcasecmp(typeName, "type")) {
	m_fixedLayout = false;
	m_baseType = OA::OCPI_Type;
	m_type = new Member();
	ezxml_t xt = ezxml_cchild(xm, "type");
	if (!xt)
	  return "missing \"type\" child element under data type with type=\"type\"";
	if ((err = OE::checkAttrs(xt, OCPI_UTIL_MEMBER_ATTRS, NULL)) ||
	    (err = OE::checkElements(xm, OCPI_UTIL_MEMBER_ELEMENTS, "type", (void*)0)) ||
	    (err = m_type->parse(xt, a_isFixed, false, NULL, NULL, 0)))
	  return err;
	if (!m_type->m_isSequence)
	  return "recursive \"type\" element must be a sequence";
      } else {
	if ((err = OE::checkElements(xm, OCPI_UTIL_MEMBER_ELEMENTS, (void*)0)))
	  return err;
	// A primitive/scalar type
	const char **tp;
	for (tp = baseTypeNames; *tp; tp++)
	  if (!strcasecmp(typeName, *tp))
	    break;
	if (!*tp)
	  return esprintf("Unknown property/argument type: \"%s\"", typeName);
	m_baseType = (OA::BaseType)(tp - baseTypeNames);
	if (m_baseType == OA::OCPI_Enum) {
	  const char *enums = ezxml_cattr(xm, "enums");
	  if (!enums)
	    return "Missing \"enums\" attribute when type attribute is \"enum\"";
	  ValueType vt;
	  vt.m_baseType = OA::OCPI_String;
	  vt.m_isSequence = true;
	  vt.m_sequenceLength = 0;
	  Value v(vt);
	  if ((err = v.parse(enums)))
	    return esprintf("Error parsing enums attribute: %s", err);
	  m_nEnums = v.m_nElements;
	  m_enums = new const char*[m_nEnums + 1];
	  const char **ep = m_enums;
	  for (unsigned n = 0; n < v.m_nElements; n++, ep++) {
	    *ep = new char[strlen(v.m_pString[n]) + 1];
	    strcpy((char *)*ep, v.m_pString[n]);
	  }
	  *ep = NULL;
	  // FIXME:  check for duplicate enum values, check for enum string chars being sane
	  // enums have a baseTypeSize of 32 per IDL
	}
	if (m_baseType == OA::OCPI_String) {
	  if ((err = OE::getExprNumber(xm, "StringLength", m_stringLength, &found,
				       m_stringLengthExpr, resolver)) ||
	      (!found &&
	       (err = OE::getExprNumber(xm, "size", m_stringLength, &found, m_stringLengthExpr,
					resolver))))
	    return err;
	  if (a_isFixed) {
	    if (!found)
	      return "Missing StringLength attribute for string type that must be bounded";
	    if (m_stringLength == 0)
	      return "StringLength cannot be zero";
	  } else
	    m_fixedLayout = false;
	  if (!m_stringLengthExpr.empty())
	    m_usesParameters = true;
	}
      }
      if (ezxml_cattr(xm, "StringLength") && m_baseType != OA::OCPI_String)
	return "StringLength attribute only valid for string types";

      // Deal with arrays now that we have the "basic" type dealt with

      bool isArray = false;
      size_t arrayLength;
      std::string expr;
      const char *arrayDimensions;
      if ((err = OE::getExprNumber(xm, "ArrayLength", arrayLength, &isArray, expr, resolver)))
	return err;
      if (isArray) {
	if (arrayLength == 0)
	  return "ArrayLength cannot be zero";
	// Single dimension array
	m_arrayRank = 1;
	m_arrayDimensions = new size_t[1];
	m_arrayDimensionsExprs.resize(1);
	m_arrayDimensions[0] = arrayLength;
	m_arrayDimensionsExprs[0] = expr;
	m_nItems = arrayLength;
      } else if ((arrayDimensions = ezxml_cattr(xm, "ArrayDimensions"))) {
	ValueType vt;
	vt.m_baseType = OA::OCPI_String;
	vt.m_isSequence = true;
	vt.m_sequenceLength = 10;
	Value v(vt);
	if ((err = v.parse(arrayDimensions)))
	  return esprintf("Error parsing array dimensions: %s", err);
	m_arrayRank = v.m_nElements;
	m_arrayDimensions = new size_t[v.m_nElements];
	m_arrayDimensionsExprs.resize(v.m_nElements);
	const char **p = v.m_pString;
	for (unsigned n = 0; n < v.m_nElements; n++, p++) {
	  if ((err = parseExprNumber(*p, m_arrayDimensions[n], &m_arrayDimensionsExprs[n],
				     resolver)))
	    return err;
	  if (m_arrayDimensions[n] == 0)
	    return "ArrayDimensions cannot have zero values";
	  m_nItems *= m_arrayDimensions[n];
	}
      }
      if (m_arrayRank && !m_arrayDimensionsExprs[0].empty())
	m_usesParameters = true;
      if ((err = OE::getExprNumber(xm, "SequenceLength", m_sequenceLength, &m_isSequence,
				   m_sequenceLengthExpr, resolver)) ||
	  (!m_isSequence &&
	   ((err = OE::getExprNumber(xm, "SequenceSize", m_sequenceLength, &m_isSequence,
				     m_sequenceLengthExpr, resolver)))))
	return err;
      if (m_isSequence) {
	if (a_isFixed) {
	  if (m_sequenceLength == 0)
	    return "Sequence must have a bounded size";
	} else {
	  m_fixedLayout = false;
	}
	if (!m_sequenceLengthExpr.empty())
	  m_usesParameters = true;
      }
      // Process default values
      if (hasDefault && (err = parseDefault(ezxml_cattr(xm, hasDefault), tag, resolver)))
	return err;
      if (m_format.size() && !strchr(m_format.c_str(), '%'))
	return esprintf("invalid format string '%s' for '%s'", m_format.c_str(), m_name.c_str());
      return 0;
    }

    // Finalize the data types by recomputing all the attributes that might be
    // based on parameters whose values were set later.
    const char *Member::
    finalize(const IdentResolver &resolver, const char *tag, bool a_isFixed) {
      const char *err;
      // First we finalize any members, recursively
      if (m_baseType == OA::OCPI_Struct)
	for (unsigned n = 0; n < m_nMembers; n++)
	  m_members[n].finalize(resolver, "member", a_isFixed);
      if (m_arrayRank) {
	m_nItems = 1;
	for (unsigned i = 0; i < m_arrayRank; i++) {
	  if (m_arrayDimensionsExprs[i].length() &&
	      (err = parseExprNumber(m_arrayDimensionsExprs[i].c_str(), m_arrayDimensions[i],
				     NULL,  &resolver)))
	    return err;
	  // FIXME: this is redundant with the code in parse() - share it
	  if (m_arrayDimensions[i] == 0)
	    return "ArrayDimensions cannot have zero values";
	  m_nItems *= m_arrayDimensions[i];
	}
      }
      if (m_isSequence) {
	if (m_sequenceLengthExpr.length() &&
	    (err = parseExprNumber(m_sequenceLengthExpr.c_str(), m_sequenceLength, NULL,
				   &resolver)))
	  return err;
	if (a_isFixed && m_sequenceLength == 0)
	  return "Sequence must have a bounded size";
      }
      if (m_baseType == OA::OCPI_String) {
	if (m_stringLengthExpr.length() &&
	    (err = parseExprNumber(m_stringLengthExpr.c_str(), m_stringLength, NULL, &resolver)))
	  return err;
	if (a_isFixed && m_stringLength == 0)
	  return "StringLength cannot be zero";
      }
      return m_defaultExpr.length() ? parseDefault(m_defaultExpr.c_str(), tag, &resolver) : NULL;
    }

    void Member::
    printAttrs(std::string &out, const char *tag, unsigned indent, bool suppressDefault) {
      formatAdd(out, "%*s<%s", indent * 2, "", tag);
      if (!m_name.empty())
	formatAdd(out, " name=\"%s\"", m_name.c_str());
      if (m_baseType != OA::OCPI_ULong)
	formatAdd(out, " type=\"%s\"", baseTypeNames[m_baseType]);
      if (m_baseType == OA::OCPI_String)
	formatAdd(out, " stringLength=\"%zu\"", m_stringLength);
      if (m_isSequence)
	formatAdd(out, " sequenceLength=\"%zu\"", m_sequenceLength);
      if (m_arrayRank == 1)
	formatAdd(out, " arrayLength=\"%zu\"", m_arrayDimensions[0]);
      else if (m_arrayRank > 1) {
	formatAdd(out, " arrayDimensions=\"");
	for (size_t n = 0; n < m_arrayRank; n++)
	  formatAdd(out, "%s%zu", n ? "," : "", m_arrayDimensions[n]);
	formatAdd(out, "\"");
      }
      if (m_nEnums) {
	formatAdd(out, " enums=\"");
	for (unsigned n = 0; n < m_nEnums; n++)
	  formatAdd(out, "%s%s", n ? "," : "", m_enums[n]);
	formatAdd(out, "\"");
      }
      if (m_isKey)
	formatAdd(out, " key=\"true\"");
      if (m_default && !suppressDefault) {
	std::string val;
	m_default->unparse(val);
	out += " default='";
	std::string xml;
	encodeXmlAttrSingle(val, xml);
	out += xml;
	out += "'";
      }
    }
    void Member::
    printChildren(std::string &out, const char *tag, unsigned indent) {
      if (m_baseType == OA::OCPI_Struct || m_baseType == OA::OCPI_Type) {
	formatAdd(out, ">\n");
	if (m_baseType == OA::OCPI_Struct) {
	  for (unsigned n = 0; n < m_nMembers; n++) {
	    m_members[n].printAttrs(out, "member", indent + 1);
	    m_members[n].printChildren(out, "member", indent + 1);
	  }
	} else {
	  m_type->printAttrs(out, "type", indent + 1);
	  m_type->printChildren(out, "type", indent + 1);
	}
	formatAdd(out, "%*s</%s>\n", indent * 2, "", tag);
      } else
	formatAdd(out, "/>\n");
    }
    void Member::
    printXML(std::string &out, const char *tag, unsigned indent) {
      printAttrs(out, tag, indent);
      printChildren(out, tag, indent);
    }
    inline void advance(const uint8_t *&p, size_t nBytes, size_t &length) {
      if (nBytes > length)
	throw Error("Aligning data exceeds buffer when writing: length %zu advance %zu",
		    length, nBytes);
      // Cannot enforce this because a "fake" mode is used to prepass and determine buffer size:
      // ocpiAssert(p != nullptr);
      length -= nBytes;
      p += nBytes;
    }
    inline void radvance(uint8_t *&p, size_t nBytes, size_t &length) {
      advance(*(const uint8_t **)&p, nBytes, length);
    }
    inline void align(const uint8_t *&p, size_t n, size_t &length) {
      uint8_t *tmp = (uint8_t *)(((uintptr_t)p + (n - 1)) & ~((uintptr_t)(n)-1));
      advance(p, tmp - p, length);
    }
    // We clear bytes we skip
    inline void ralign(uint8_t *&p, size_t n, size_t &length) {
      align(*(const uint8_t **)&p, n, length);
    }
    // Push the data in the linear buffer into a writer object
    void Member::
    write(Writer &writer, const uint8_t *&data, size_t &length, bool topSeq) {
      size_t nElements = 1;
      const uint8_t *startData = NULL; // quiet warning
      size_t startLength = 0; // quiet warning
      if (m_isSequence) {
	if (topSeq && !m_fixedLayout) {
	  ocpiAssert(((intptr_t)data & ~(m_align - 1)) == 0);
	  ocpiAssert(length % m_nBytes == 0);
	  nElements = length / m_nBytes;
	} else {
	  align(data, m_align, length);
	  nElements = *(uint32_t *)data;
	}
	startData = data;
	startLength = length;
	if (m_sequenceLength != 0 && nElements > m_sequenceLength)
	  throw Error("Sequence in buffer (%zu) exceeds max length (%zu)", nElements,
		      m_sequenceLength);
	writer.beginSequence(*this, nElements);
	if (!nElements) {
	  advance(data, m_fixedLayout && !topSeq ? m_nBytes : m_align, length);
	  return;
	}
	advance(data, m_align, length);
      }
      nElements *= m_nItems;
      if (m_arrayRank)
	writer.beginArray(*this, m_nItems);
      align(data, m_dataAlign, length);
      switch (m_baseType) {
      case OA::OCPI_Struct:
	writer.beginStruct(*this);
	for (unsigned n = 0; n < nElements; n++) {
	  align(data, m_dataAlign, length);
	  for (unsigned nn = 0; nn < m_nMembers; nn++)
	    m_members[nn].write(writer, data, length);
        }
	writer.endStruct(*this);
	break;
      case OA::OCPI_Type:
	writer.beginType(*this);
	for (unsigned n = 0; n < nElements; n++)
	  m_type->write(writer, data, length);
	writer.endType(*this);
	break;
      case OA::OCPI_String:
	for (unsigned n = 0; n < nElements; n++) {
	  align(data, 4, length);
	  WriteDataPtr p = {data};
	  size_t nBytes = strlen((const char *)data) + 1;
	  advance(data, m_fixedLayout ?  (m_stringLength + 4) & ~3 : nBytes, length);
	  writer.writeString(*this, p, nBytes - 1, n == 0, topSeq);
	}
	break;
      default:
	{ // Scalar - write them all at once
	  align(data, m_align, length);
	  WriteDataPtr p = {data};
	  size_t nBytes = nElements * m_elementBytes;
	  advance(data, nBytes, length);
	  writer.writeData(*this, p, nBytes, nElements);
	  break;
	}
      case OA::OCPI_none:
      case OA::OCPI_scalar_type_limit:
	ocpiAssert(0);
      }
      if (m_arrayRank )
	writer.endArray(*this);
      if (m_isSequence) {
	writer.endSequence(*this);
	if (m_fixedLayout && !topSeq) {
	  // If fixed layout override the incremental data/length advance and
	  // advance over the whole thing, including the length prefix
	  advance(startData, m_nBytes, startLength);
	  assert(startData >= data && startLength <= length);
	  data = startData;
	  length = startLength;
	}
      }
    }

    // Fill the linear buffer from a reader object
    void Member::
    read(Reader &reader, uint8_t *&data, size_t &length, bool fake, bool top) const {
      size_t nElements = 1;
      uint8_t *startData = NULL; // quiet warning
      size_t startLength = 0; // quiet warning
      if (m_isSequence) {
	ralign(data, m_align, length);
	startData = data;
	startLength = length;
	nElements = reader.beginSequence(*this);
	if (m_sequenceLength != 0 && nElements > m_sequenceLength)
	  throw Error("Sequence being read (%zu) exceeds max length (%zu)", nElements, m_sequenceLength);
	if (!fake)
	  *(uint32_t *)data = (uint32_t)nElements;
	if (!nElements) {
	  // Sequence is empty. skip over header or whole thing if fixedLayout
	  radvance(data, m_fixedLayout && !top ? m_nBytes : m_align, length);
	  return;
	}
	// Non empty - skip over header for now
	radvance(data, m_align, length);
      }
      if (m_arrayRank)
	reader.beginArray(*this, m_nItems);
      nElements *= m_nItems;
      ralign(data, m_dataAlign, length);
      switch (m_baseType) {
      case OA::OCPI_Struct:
	reader.beginStruct(*this);
	for (unsigned n = 0; n < nElements; n++) {
	  ralign(data, m_dataAlign, length);
	  for (unsigned nn = 0; nn < m_nMembers; nn++)
	    m_members[nn].read(reader, data, length, fake);
        }
	reader.endStruct(*this);
	break;
      case OA::OCPI_Type:
	reader.beginType(*this);
	for (unsigned n = 0; n < nElements; n++)
	  m_type->read(reader, data, length, fake);
	reader.endType(*this);
	break;
      case OA::OCPI_String:
	for (unsigned n = 0; n < nElements; n++) {
	  ralign(data, 4, length);
	  const char *chars;
	  size_t strLength = reader.beginString(*this, chars, n == 0);
	  if (m_stringLength != 0 && strLength > m_stringLength)
	    throw Error("String being read is larger than max length");
	  uint8_t *start = data;
	  // Error check before copy
	  radvance(data, m_fixedLayout ? (m_stringLength + 4) & ~3 : strLength + 1, length);
	  if (!fake) {
	    memcpy(start, chars, strLength);
	    start[strLength] = 0;
	  }
	}
	break;
      default:
	{ // Scalar - write them all at once
	  ralign(data, m_align, length);
	  ReadDataPtr p = {data};
	  size_t nBytes = nElements * m_elementBytes;
	  radvance(data, nBytes, length);
	  reader.readData(*this, p, nBytes, nElements, fake);
	  break;
	}
      case OA::OCPI_none:
      case OA::OCPI_scalar_type_limit:
	ocpiAssert(0);
      }
      if (m_arrayRank)
	reader.endArray(*this);
      if (m_isSequence) {
	reader.endSequence(*this);
	if (m_fixedLayout && !top) {
	  // If fixed layout override the incremental data/length advance and
	  // advance over the whole thing, including the length prefix
          // ocpiDebug("radvance(<ptr>, %zu, %zu)", m_nBytes, startLength);
	  radvance(startData, m_nBytes, startLength);
	  assert(startData >= data && startLength <= length);
	  data = startData;
	  length = startLength;
	}
      }
    }
    void Member::
    generate(const char *name, unsigned ordinal, unsigned depth) {
      m_name = name;
      m_ordinal = ordinal;
      m_baseType = (OA::BaseType)((random() >> 24) % (OA::OCPI_scalar_type_limit - 1) + 1);
      // printf(" %d", m_baseType);
      if (++depth == 4 && (m_baseType == OA::OCPI_Type || m_baseType == OA::OCPI_Struct))
	m_baseType = OA::OCPI_ULong;
      if (m_baseType == OA::OCPI_Type)
	m_fixedLayout = false;
      m_isSequence = random() % 3 == 0;
      if (m_isSequence) {
	m_sequenceLength = random() & 1 ? 0 : random() % 10;
	if (m_sequenceLength == 0 || random() & 1)
	  m_fixedLayout = false;
      }
      if (random() & 1) {
	m_arrayRank = random() % 3 + 1;
	m_arrayDimensions = new size_t[m_arrayRank];
	for (unsigned n = 0; n < m_arrayRank; n++) {
	  m_arrayDimensions[n] = random() % 3 + 1;
	  m_nItems *= m_arrayDimensions[n];
	}
      }
      switch (m_baseType) {
      case OA::OCPI_String:
	m_stringLength = random() & 1 ? 0 : random() % testMaxStringLength;
	if (m_stringLength == 0 || random() & 1)
	  m_fixedLayout = false;
	break;
      case OA::OCPI_Enum:
	m_nEnums = random() % 5 + 1;
	m_enums = new const char *[m_nEnums + 1];
	for (unsigned n = 0; n < m_nEnums; n++) {
	  char *e;
	  ocpiCheck(asprintf(&e, "enum%u", n) > 0);
	  m_enums[n] = new char[strlen(e) + 1];
	  strcpy((char *)m_enums[n], e);
	  free(e);
	}
	m_enums[m_nEnums] = NULL;
	break;
      case OA::OCPI_Type:
	if (m_isSequence) {
	  m_type = new Member();
	  m_type->generate("type", 0, depth);
	  if (m_type->m_isSequence)
	    break;
	  delete m_type;
	  m_type = 0;
	}
	m_baseType = OA::OCPI_Float;
	break;
      case OA::OCPI_Struct:
	m_nMembers = random() % 6 + 1;
	m_members = new Member[m_nMembers];
	for (unsigned n = 0; n < m_nMembers; n++) {
	  char *e;
	  ocpiCheck(asprintf(&e, "member%u", n) > 0);
	  m_members[n].generate(e, n, depth);
	  free(e);
	  if (!m_members[n].m_fixedLayout)
	    m_fixedLayout = false;
	}
	break;
      default:
	break;
      }
    }
    // This static method is shared between parsing members of a structure and parsing arguments
    // to an operation.
    const char *
    Member::
    parseMembers(ezxml_t mems, size_t &nMembers, Member *&members, bool a_isFixed,
		 const char *tag, const char *hasDefault, const IdentResolver *resolver) {
      for (ezxml_t m = ezxml_cchild(mems, tag); m ; m = ezxml_cnext(m))
	nMembers++;
      if (nMembers) {
	std::set<const char *, ConstCharComp> names, abbrevs;
	Member *m = new Member[nMembers];
	members = m;
	const char *err = NULL;
	for (ezxml_t mx = ezxml_cchild(mems, tag); mx ; mx = ezxml_cnext(mx), m++) {
	  if ((err = OE::checkAttrs(mx, OCPI_UTIL_MEMBER_ATTRS,
				    hasDefault ? hasDefault : NULL, NULL)) ||
	      (err = m->parse(mx, a_isFixed, true, hasDefault, "member", (unsigned)(m - members),
			      resolver)))
	    return err;
	  if (!names.insert(m->m_name.c_str()).second)
	    return esprintf("Duplicate member name: %s", m->m_name.c_str());
	  if (m->m_abbrev.size() && !abbrevs.insert(m->m_abbrev.c_str()).second)
	    return esprintf("Duplicate member abbreviation: %s", m->m_name.c_str());
	}
      }
      return NULL;
    }
    const char *Member::
    offset(size_t &maxAlign, size_t &argOffset, size_t &minSizeBits, bool &diverseSizes,
	   bool &sub32, bool &unBounded, bool &isVariable, bool isTop) {
      const char *err;
      uint64_t nBytes;
      m_offset = 0;
      switch (m_baseType) {
      case OA::OCPI_Struct:
	if ((err = alignMembers(m_members, m_nMembers, m_align, m_offset, minSizeBits,
				diverseSizes, sub32, unBounded, isVariable)))
	  return err;
	nBytes = m_offset;
	m_nBits = m_offset * CHAR_BIT;
	break;
      case OA::OCPI_Type:
	if ((err = m_type->offset(m_align, m_offset, minSizeBits, diverseSizes, sub32,
				  unBounded, isVariable)))
	  return err;
	nBytes = m_offset;
	m_nBits = m_offset * CHAR_BIT;
	break;
      default:
	// No special enum processing here
	m_nBits = baseTypeSizes[m_baseType];
	m_align = (m_nBits + CHAR_BIT - 1) / CHAR_BIT;
	size_t scalarBits;
	if (m_baseType == OA::OCPI_String) {
	  // Make strings whole 32 bit words
	  // Since this is not CDR anyway, this is best for hardware
	  // And meets the rule: nothing both spans and shares words.
	  nBytes = (m_stringLength + 4) & ~3;
	  scalarBits = CHAR_BIT;
	  if (!m_stringLength)
	    unBounded = true;
	  isVariable = true;
	} else {
	  nBytes = m_align;
	  scalarBits = m_align * CHAR_BIT;
	}
	if (minSizeBits) {
	  if (minSizeBits != scalarBits)
	    diverseSizes = true;
	  if (scalarBits < minSizeBits)
	    minSizeBits = scalarBits;
	} else
	  minSizeBits = scalarBits;
      }
      m_elementBytes = OCPI_UTRUNCATE(size_t, nBytes); // was m_nBits/CHAR_BIT;
      if (m_arrayRank || m_isSequence)
	m_elementBytes = roundUp((uint32_t)m_elementBytes, m_align);
      // Calculate the number of bytes in each element of an array/sequence
      if (nBytes > UINT32_MAX)
	return "Total member size in bytes is too large (> 4G)";
      // Array?
      if (m_arrayRank) {
	size_t *p = m_arrayDimensions;
	nBytes = roundUp((uint32_t)nBytes, m_align);
	for (unsigned n = 0; n < m_arrayRank; n++, p++) {
	  nBytes *= *p;
	  if (nBytes > UINT32_MAX)
	    return "Total array size in bytes is too large (> 4G)";
	}
      }
      m_dataAlign = m_align; // capture this before adjusting it in the sequence case.
      if (m_isSequence) {
	isVariable = true;
	// Pad the size to be what is required for an array of same.
	nBytes = roundUp((uint32_t)nBytes, m_align);
	if (m_sequenceLength != 0)
	  nBytes *= m_sequenceLength;
	else
	  unBounded = true;
	if (m_align < 4)
	  m_align = 4;
	// Add the bytes for the 32 bit sequence count, and if the alignment be larger
	// than 32 bits, add padding for that.  But not for a top level singular sequence
	if (!isTop || argOffset)
	  nBytes += m_align > 4 ? m_align : 4;
	if (nBytes > UINT32_MAX)
	  return "Total sequence size in bytes is too large (> 4G)";
      }
      if (m_align > maxAlign)
	maxAlign = m_align;
      if (m_align < 4)
	sub32 = true;
      m_nBytes = (uint32_t)nBytes;
      argOffset = roundUp(argOffset, m_align);
      m_offset = argOffset;
      argOffset += m_nBytes;
      return 0;
    }
    const char * Member::
    alignMembers(Member *m, size_t nMembers, size_t &maxAlign, size_t &myOffset,
		 size_t &minSizeBits, bool &diverseSizes, bool &sub32, bool &unBounded,
		 bool &isVariable, bool isTop) {
      const char *err;
      for (unsigned n = 0; n < nMembers; n++, m++)
	if ((err = m->offset(maxAlign, myOffset, minSizeBits, diverseSizes, sub32, unBounded,
			     isVariable, isTop)))
	  return err;
      return 0;
    }
    uint8_t *Member::
    getField(uint8_t */*data*/, size_t &/*length*/) const {
      return NULL;
    }

    const char *baseTypeNames[] = {
      "None",
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #pretty,
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      "Struct", "Enum", "Type",
      0
    };
    const char *idlTypeNames[] = {
      "None",
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #corba,
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      "Struct", "Enum", "Type",
      0
    };
    unsigned baseTypeSizes[] = {
      0,// for OCPI_NONE
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) bits,
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      0, 32, 0 // enum size is 32 bits
    };

    Reader::Reader() {}
    Reader::~Reader() {}
    void Reader::endSequence(const Member &) {}
    void Reader::endString(const Member &) {}
    void Reader::beginStruct(const Member &) {}
    void Reader::beginArray(const Member &, size_t) {}
    void Reader::endArray(const Member &) {}
    void Reader::endStruct(const Member &) {}
    void Reader::beginType(const Member &) {}
    void Reader::endType(const Member &) {}
    void Reader::end() {}
    Writer::Writer() {}
    Writer::~Writer() {}
    void Writer::endSequence(Member &) {}
    void Writer::writeOpcode(const char *, uint8_t) {}
    void Writer::beginStruct(Member &) {}
    void Writer::beginArray(Member &, size_t) {}
    void Writer::endArray(Member &) {}
    void Writer::endStruct(Member &) {}
    void Writer::beginType(Member &) {}
    void Writer::endType(Member &) {}
    void Writer::end() {}
  }
}
