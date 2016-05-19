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

    ValueType::ValueType(OA::BaseType bt, bool isSequence)
      : m_baseType(bt), m_arrayRank(0), m_nMembers(0), m_dataAlign(0), m_align(1), m_nBits(0),
	m_elementBytes(0), m_isSequence(isSequence), m_nBytes(0), m_arrayDimensions(NULL),
	m_stringLength(0), m_sequenceLength(0), m_members(NULL), m_type(NULL), m_enums(NULL),
	m_nEnums(0), m_nItems(1), m_fixedLayout(true)
    {}
    ValueType::~ValueType() {
      if (m_arrayDimensions)
	delete [] m_arrayDimensions;
      if (m_members)
	delete [] m_members;
      if (m_type)
	delete m_type;
      if (m_enums) {
	for (unsigned n = 0; n < m_nEnums; n++)
	  delete [] m_enums[n];
	delete [] m_enums;
      }
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

    Reader::Reader(){}
    Reader::~Reader(){}
    void Reader::endSequence(const Member &){}
    void Reader::endString(const Member &){}
    void Reader::beginStruct(const Member &){}
    void Reader::beginArray(const Member &, size_t){}
    void Reader::endArray(const Member &){}
    void Reader::endStruct(const Member &){}
    void Reader::beginType(const Member &){}
    void Reader::endType(const Member &){}
    void Reader::end(){}
    Writer::Writer(){}
    Writer::~Writer(){}
    void Writer::endSequence(Member &){}
    void Writer::writeOpcode(const char *, uint8_t) {}
    void Writer::beginStruct(Member &){}
    void Writer::beginArray(Member &, size_t){}
    void Writer::endArray(Member &){}
    void Writer::endStruct(Member &){}
    void Writer::beginType(Member &){}
    void Writer::endType(Member &){}
    void Writer::end(){}

    Member::Member()
      :  m_offset(0), m_isIn(false), m_isOut(false), m_isKey(false), m_default(NULL)
    {
    }
    // Constructor when you are not parsing, and doing static initialization
    Member::Member(const char *name, const char *abbrev, const char *description, OA::BaseType type,
		   bool isSequence, const char *defaultValue)
		   : ValueType(type, isSequence), m_name(name), m_abbrev(abbrev ? abbrev : ""),
		     m_description(description ? description : ""),
		     m_offset(0), m_isIn(false), m_isOut(false), m_isKey(false), m_default(NULL) {
      if (defaultValue) {
	  m_default = new Value(*this);
	  ocpiCheck(m_default->parse(defaultValue) == 0);
      }
    }
    Member::~Member() {
      if (m_default)
	delete m_default;
    }
    const char *
    Member::parseDefault(ezxml_t xm, const char *hasDefault) {
      const char *defValue = ezxml_cattr(xm, hasDefault);
      if (defValue) {
	delete m_default;
	m_default = new Value(*this);
	const char *err = m_default->parse(defValue);
	if (err)
	  return esprintf("for member %s: %s", m_name.c_str(), err);
      }
      return NULL;
    }

    const char *
    Member::parse(ezxml_t xm, bool isFixed, bool hasName, const char *hasDefault,
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
      OE::getOptionalString(xm, m_description, "Description");
      OE::getOptionalString(xm, m_format, "Format");
      const char *typeName = ezxml_cattr(xm, "Type");
      if (!typeName)
	typeName = "ULong";
      if ((err = OE::getBoolean(xm, "Key", &m_isKey)))
	return err;
      if (!strcasecmp(typeName, "struct")) {
	m_baseType = OA::OCPI_Struct;
	if ((err = OE::checkElements(xm, "member", (void*)0)) ||
	    (err = parseMembers(xm, m_nMembers, m_members, isFixed, "member", hasDefault)))
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
	    (err = OE::checkElements(xm, "type", (void*)0)) ||
	    (err = m_type->parse(xt, isFixed, false, NULL, 0)))
	  return err;
	if (!m_type->m_isSequence)
	  return "recursive \"type\" element must be a sequence";
      } else {
	if ((err = OE::checkElements(xm, (void*)0)))
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
	  if ((err = getExprNumber(xm, "StringLength", m_stringLength, &found,
				   &m_stringLengthExpr, resolver)) ||
	      (!found &&
	       (err = getExprNumber(xm, "size", m_stringLength, &found, &m_stringLengthExpr,
				    resolver))))
	    return err;
	  if (isFixed) {
	    if (!found)
	      return "Missing StringLength attribute for string type that must be bounded";
	    if (m_stringLength == 0)
	      return "StringLength cannot be zero";
	  } else
	    m_fixedLayout = false;
	}
      }
      if (ezxml_cattr(xm, "StringLength") && m_baseType != OA::OCPI_String)
	return "StringLength attribute only valid for string types";
      
      // Deal with arrays now that we have the "basic" type dealt with

      bool isArray = false;
      size_t arrayLength;
      std::string expr;
      const char *arrayDimensions;
      if ((err = getExprNumber(xm, "ArrayLength", arrayLength, &isArray, &expr, resolver)))
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

      // Deal with sequences after arrays (because arrays belong to declarators)
      if ((err = getExprNumber(xm, "SequenceLength", m_sequenceLength, &m_isSequence,
			       &m_sequenceLengthExpr, resolver)) ||
	  (!m_isSequence &&
	   ((err = getExprNumber(xm, "SequenceSize", m_sequenceLength, &m_isSequence,
				 &m_sequenceLengthExpr, resolver)))))
	return err;
      if (m_isSequence) {
	if (isFixed) {
	  if (m_sequenceLength == 0)
	    return "Sequence must have a bounded size";
	} else {
	  m_fixedLayout = false;
	}
      }
      // Process default values
      if (hasDefault && (err = parseDefault(xm, hasDefault)))
	return err;
      if (m_format.size() && !strchr(m_format.c_str(), '%'))
	return esprintf("invalid format string '%s' for '%s'", m_format.c_str(), m_name.c_str());
      return 0;
    }

    // Finalize the data types by recomputing all the attributes that might be
    // based on parameters whose values were set later.
    const char *Member::
    finalize(const IdentResolver &resolver, bool isFixed) {
      const char *err;
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
	if (isFixed && m_sequenceLength == 0)
	  return "Sequence must have a bounded size";
      }
      if (m_baseType == OA::OCPI_String) {
	if (m_stringLengthExpr.length() &&
	    (err = parseExprNumber(m_stringLengthExpr.c_str(), m_stringLength, NULL, &resolver)))
	  return err;
	if (isFixed && m_stringLength == 0)
	  return "StringLength cannot be zero";
      }
      return NULL;
    }

    void Member::
    printAttrs(FILE *f, const char *tag, unsigned indent, bool suppressDefault) {
      fprintf(f, "%*s<%s", indent * 2, "", tag);
      if (!m_name.empty())
	fprintf(f, " name=\"%s\"", m_name.c_str());
      if (m_baseType != OA::OCPI_ULong)
	fprintf(f, " type=\"%s\"", baseTypeNames[m_baseType]);
      if (m_baseType == OA::OCPI_String)
	fprintf(f, " stringLength=\"%zu\"", m_stringLength);
      if (m_isSequence)
	fprintf(f, " sequenceLength=\"%zu\"", m_sequenceLength);
      if (m_arrayRank == 1)
	fprintf(f, " arrayLength=\"%zu\"", m_arrayDimensions[0]);
      else if (m_arrayRank > 1) {
	fprintf(f, " arrayDimensions=\"");
	for (size_t n = 0; n < m_arrayRank; n++)
	  fprintf(f, "%s%zu", n ? "," : "", m_arrayDimensions[n]);
	fprintf(f, "\"");
      }
      if (m_nEnums) {
	fprintf(f, " enums=\"");
	for (unsigned n = 0; n < m_nEnums; n++)
	  fprintf(f, "%s%s", n ? "," : "", m_enums[n]);
	fprintf(f, "\"");
      }
      if (m_isKey)
	fprintf(f, " key=\"true\"");
      if (m_default && !suppressDefault) {
	std::string val;
	m_default->unparse(val);
	fprintf(f, " default='");
	std::string xml;
	encodeXmlAttrSingle(val, xml);
	fputs(xml.c_str(), f);
	fputc('\'', f);
      }
    }
    void Member::
    printChildren(FILE *f, const char *tag, unsigned indent) {
      if (m_baseType == OA::OCPI_Struct || m_baseType == OA::OCPI_Type) {
	fprintf(f, ">\n");
	if (m_baseType == OA::OCPI_Struct) {
	  for (unsigned n = 0; n < m_nMembers; n++) {
	    m_members[n].printAttrs(f, "member", indent + 1);
	    m_members[n].printChildren(f, "member", indent + 1);
	  }
	} else {
	  m_type->printAttrs(f, "type", indent + 1);
	  m_type->printChildren(f, "type", indent + 1);
	}
	fprintf(f, "%*s</%s>\n", indent * 2, "", tag);
      } else
	fprintf(f, "/>\n");
    }
    void Member::printXML(FILE *f, const char *tag, unsigned indent) {
      printAttrs(f, tag, indent);
      printChildren(f, tag, indent);
    }
    inline void advance(const uint8_t *&p, size_t nBytes, size_t &length) {
      if (nBytes > length) {
	throw Error("Aligning data exceeds buffer when writing: length %zu advance %zu",
		    length, nBytes);
      }
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
    void Member::write(Writer &writer, const uint8_t *&data, size_t &length, bool topSeq) {
      size_t nElements = 1;
      const uint8_t *startData;
      size_t startLength;
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
	for (unsigned n = 0; n < nElements; n++)
	  for (unsigned nn = 0; nn < m_nMembers; nn++)
	    m_members[nn].write(writer, data, length);
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
    void Member::read(Reader &reader, uint8_t *&data, size_t &length, bool fake, bool top) const {
      size_t nElements = 1;
      uint8_t *startData;
      size_t startLength;
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
	for (unsigned n = 0; n < nElements; n++)
	  for (unsigned nn = 0; nn < m_nMembers; nn++)
	    m_members[nn].read(reader, data, length, fake);
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
          ocpiDebug("radvance(<ptr>, %zu, %zu)", m_nBytes, startLength);
	  radvance(startData, m_nBytes, startLength);
	  assert(startData >= data && startLength <= length);
	  data = startData;
	  length = startLength;
	}
      }
    }
    void Member::generate(const char *name, unsigned ordinal, unsigned depth) {
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
	m_enums = new const char *[m_nEnums];
	for (unsigned n = 0; n < m_nEnums; n++) {
	  char *e;
	  asprintf(&e, "enum%u", n);
	  m_enums[n] = new char[strlen(e) + 1];
	  strcpy((char *)m_enums[n], e);
	  free(e);
	}
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
	  asprintf(&e, "member%u", n);
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
    Member::parseMembers(ezxml_t mems, size_t &nMembers, Member *&members,
			 bool isFixed, const char *tag, const char *hasDefault) {
      for (ezxml_t m = ezxml_cchild(mems, tag); m ; m = ezxml_next(m))
	nMembers++;
      if (nMembers) {
	std::set<const char *, ConstCharComp> names, abbrevs;
	Member *m = new Member[nMembers];
	members = m;
	const char *err = NULL;
	for (ezxml_t mx = ezxml_cchild(mems, tag); mx ; mx = ezxml_next(mx), m++) {
	  if ((err = OE::checkAttrs(mx, OCPI_UTIL_MEMBER_ATTRS,
				    hasDefault ? hasDefault : NULL, NULL)) ||
	      (err = m->parse(mx, isFixed, true, hasDefault, (unsigned)(m - members))))
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
    offset(size_t &maxAlign, size_t &argOffset,
	   size_t &minSizeBits, bool &diverseSizes, bool &sub32, bool &unBounded, bool isTop) {
      const char *err;
      uint64_t nBytes;
      m_offset = 0;
      switch (m_baseType) {
      case OA::OCPI_Struct:
	if ((err = alignMembers(m_members, m_nMembers, m_align, m_offset,
				minSizeBits, diverseSizes, sub32, unBounded)))
	  return err;
	nBytes = m_offset;
	m_nBits = m_offset * CHAR_BIT;
	break;
      case OA::OCPI_Type:
	if ((err = m_type->offset(m_align, m_offset, minSizeBits, diverseSizes, sub32, unBounded)))
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
      m_elementBytes = m_nBits/CHAR_BIT;
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
    const char *
    Member::alignMembers(Member *m, size_t nMembers, 
			 size_t &maxAlign, size_t &myOffset,
			 size_t &minSizeBits, bool &diverseSizes, bool &sub32, bool &unBounded,
			 bool isTop) {
      const char *err;
      for (unsigned n = 0; n < nMembers; n++, m++)
	if ((err = m->offset(maxAlign, myOffset, minSizeBits, diverseSizes, sub32, unBounded, isTop)))
	  return err;
      return 0;
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
  }
}
