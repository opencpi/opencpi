
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

#define __STDC_LIMIT_MACROS // wierd standards goof up

#include <climits>
#include <string>

#include "OcpiOsAssert.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilException.h"
#include "OcpiUtilDataTypes.h"
#include "OcpiUtilValue.h"

namespace OCPI {
  namespace Util {
    namespace OA = OCPI::API;
    namespace OE = OCPI::Util::EzXml;

    ValueType::ValueType(OA::BaseType bt)
      : m_baseType(bt), m_arrayRank(0), m_nMembers(0), m_nBits(0), m_align(1),
	m_isSequence(false), m_nBytes(0), m_arrayDimensions(NULL), m_stringLength(0),
	m_sequenceLength(0), m_members(NULL), m_type(NULL), m_enums(NULL), m_nEnums(0),
	m_nItems(1)
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
    void Reader::endSequence(Member &){}
    void Reader::endString(Member &){}
    void Reader::beginStruct(Member &){}
    void Reader::beginArray(Member &, uint32_t){}
    void Reader::endArray(Member &){}
    void Reader::endStruct(Member &){}
    void Reader::beginType(Member &){}
    void Reader::endType(Member &){}
    void Reader::end(){}
    Writer::Writer(){}
    Writer::~Writer(){}
    void Writer::endSequence(Member &){}
    void Writer::writeOpcode(const char *, uint8_t) {}
    void Writer::beginStruct(Member &){}
    void Writer::beginArray(Member &, uint32_t){}
    void Writer::endArray(Member &){}
    void Writer::endStruct(Member &){}
    void Writer::beginType(Member &){}
    void Writer::endType(Member &){}
    void Writer::end(){}

    Member::Member()
      :  m_offset(0), m_isIn(false), m_isOut(false), m_isKey(false), m_defaultValue(NULL)
    {
    }
    Member::~Member() {
      if (m_defaultValue)
	delete m_defaultValue;
    }
    const char *
    Member::parse(ezxml_t xm, bool isFixed, bool hasName, const char *hasDefault, unsigned ordinal) {
      bool found;
      const char *err;
      const char *name = ezxml_cattr(xm, "Name");
      m_ordinal = ordinal;
      if (name)
	m_name = name;
      else if (hasName)
	return "Missing Name attribute in Property/Argument/Member element";
      const char *typeName = ezxml_cattr(xm, "Type");
      if (!typeName)
	typeName = "ULong";
      if ((err = OE::getBoolean(xm, "Key", &m_isKey)))
	return err;
      if (!strcasecmp(typeName, "struct")) {
	m_baseType = OA::OCPI_Struct;
	if ((err = parseMembers(xm, m_nMembers, m_members, isFixed, "member", hasDefault)))
	  return err;
	if (m_nMembers == 0)
	  return "No struct members under type == \"struct\"";
      } else if (!strcasecmp(typeName, "type")) {
	m_baseType = OA::OCPI_Type;
	m_type = new Member();
	ezxml_t xt = ezxml_cchild(xm, "type");
	if (!xt)
	  return "missing \"type\" child element under data type with type=\"type\"";
	if ((err = OE::checkAttrs(xt, OCPI_UTIL_MEMBER_ATTRS, NULL)) ||
	    (err = m_type->parse(xt, isFixed, false, NULL, 0)))
	  return err;
	if (!m_type->m_isSequence)
	  return "recursive \"type\" element must be a sequence";
      } else {
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
	  m_enums = new const char*[m_nEnums];
	  const char **ep = m_enums;
	  for (unsigned n = 0; n < v.m_nElements; n++, ep++) {
	    *ep = new char[strlen(v.m_pString[n]) + 1];
	    strcpy((char *)*ep, v.m_pString[n]);
	  }
	  // FIXME:  check for duplicate enum values
	  // enums have a baseTypeSize of 32 per IDL
	}
	if (m_baseType == OA::OCPI_String) {
	  if ((err = OE::getNumber(xm, "StringLength", &m_stringLength, &found, 0, false)) ||
	      (!found &&
	       (err = OE::getNumber(xm, "size", &m_stringLength, &found, 0, false))))
	    return err;
	  if (isFixed) {
	    if (!found)
	      return "Missing StringLength attribute for string type that must be bounded";
	    if (m_stringLength == 0)
	      return "StringLength cannot be zero";
	  }
	}
      }
      if (ezxml_cattr(xm, "StringLength") && m_baseType != OA::OCPI_String)
	return "StringLength attribute only valid for string types";
      
      // Deal with arrays now that we have the "basic" type dealt with

      bool isArray = false;
      uint32_t arrayLength;
      const char *arrayDimensions;
      if ((err = OE::getNumber(xm, "ArrayLength", &arrayLength, &isArray, 0, false)))
	return err;
      if (isArray) {
	if (arrayLength == 0)
	  return "ArrayLength cannot be zero";
	// Single dimension array
	m_arrayRank = 1;
	m_arrayDimensions = new uint32_t[1];
	*m_arrayDimensions = arrayLength;
	m_nItems = arrayLength;
      } else if ((arrayDimensions = ezxml_cattr(xm, "ArrayDimensions"))) {
	ValueType vt;
	vt.m_baseType = OA::OCPI_ULong;
	vt.m_isSequence = true;
	vt.m_sequenceLength = 10;
	Value v(vt);
	if ((err = v.parse(arrayDimensions)))
	  return esprintf("Error parsing array dimensions: %s", err);
	m_arrayRank = v.m_nElements;
	m_arrayDimensions = new uint32_t[v.m_nElements];
	uint32_t *p = v.m_pULong;
	for (unsigned n = 0; n < v.m_nElements; n++, p++) {
	  if (*p == 0)
	    return "ArrayDimensions cannot have zero values";
	  m_nItems *= *p;
	  m_arrayDimensions[n] = *p;
	}
      }

      // Deal with sequences after arrays (because arrays belong to declarators)
      if ((err = OE::getNumber(xm, "SequenceLength", &m_sequenceLength, &m_isSequence, 0, false)) ||
	  (!m_isSequence &&
	    ((err = OE::getNumber(xm, "SequenceSize", &m_sequenceLength, &m_isSequence, 0, false)))))
	return err;
      if (m_isSequence && isFixed && m_sequenceLength == 0)
	return "Sequence must have a bounded size";
      // Process default values
      if (hasDefault) {
	const char *defValue = ezxml_cattr(xm, hasDefault);
	if (defValue) {
	  m_defaultValue = new Value(*this);
	  if ((err = m_defaultValue->parse(defValue)))
	    return esprintf("for member %s: %s", m_name.c_str(), err);
	}
      }
      return 0;
    }

    void Member::
    printAttrs(FILE *f, const char *tag, unsigned indent) {
      fprintf(f, "%*s<%s", indent * 2, "", tag);
      if (!m_name.empty())
	fprintf(f, " name=\"%s\"", m_name.c_str());
      if (m_baseType != OA::OCPI_ULong)
	fprintf(f, " type=\"%s\"", baseTypeNames[m_baseType]);
      if (m_baseType == OA::OCPI_String)
	fprintf(f, " stringLength=\"%u\"", m_stringLength);
      if (m_isSequence)
	fprintf(f, " sequenceLength=\"%u\"", m_sequenceLength);
      if (m_arrayRank == 1)
	fprintf(f, " arrayLength=\"%u\"", m_arrayDimensions[0]);
      else if (m_arrayRank > 1) {
	fprintf(f, " arrayDimensions=\"");
	for (unsigned n = 0; n < m_arrayRank; n++)
	  fprintf(f, "%s%u", n ? "," : "", m_arrayDimensions[n]);
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
      if (m_defaultValue) {
	std::string val;
	m_defaultValue->unparse(val);
	fprintf(f, " default=\"%s\"", val.c_str()); // FIXME: string value properties may have extra quotes
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
    inline void advance(const uint8_t *&p, unsigned nBytes, uint32_t &length) {
      if (nBytes > length)
	throw Error("Aligning data exceeds buffer when writing");
      length -= nBytes;
      p += nBytes;
    }
    inline void radvance(uint8_t *&p, unsigned nBytes, uint32_t &length) {
      advance(*(const uint8_t **)&p, nBytes, length);
    }
    inline void align(const uint8_t *&p, unsigned n, uint32_t &length) {
      uint8_t *tmp = (uint8_t *)(((uintptr_t)p + (n - 1)) & ~((uintptr_t)(n)-1));
      advance(p, tmp - p, length);
    }
    // We clear bytes we skip
    inline void ralign(uint8_t *&p, unsigned n, uint32_t &length) {
      align(*(const uint8_t **)&p, n, length);
    }
    // Push the data in the linear buffer into a writer object
    void Member::write(Writer &writer, const uint8_t *&data, uint32_t &length, bool topSeq) {
      unsigned nElements = 1;
      if (m_isSequence) {
	if (topSeq) {
	  ocpiAssert(((intptr_t)data & ~(m_align - 1)) == 0);
	  ocpiAssert(length % m_nBytes == 0);
	  nElements = length / m_nBytes;
	} else {
	  align(data, m_align, length);
	  nElements = *(uint32_t *)data;
	}
	if (m_sequenceLength != 0 && nElements > m_sequenceLength)
	  throw Error("Sequence in buffer exceeds max length (%u)", m_sequenceLength);
	writer.beginSequence(*this, nElements);
	advance(data, m_align, length); // skip over count, and align for data
	if (!nElements)
	  return;
      }
      nElements *= m_nItems;

      if ( m_arrayRank ) {
	writer.beginArray(*this, m_nItems);			  
      }

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
	  unsigned nBytes = strlen((const char *)data) + 1;
	  advance(data, nBytes, length);
	  writer.writeString(*this, p, nBytes - 1, n == 0, topSeq);
	}
	break;
      default:
	{ // Scalar - write them all at once
	  align(data, m_align, length);
	  WriteDataPtr p = {data};
	  unsigned nBytes = nElements * (m_nBits / 8);
	  advance(data, nBytes, length);
	  writer.writeData(*this, p, nBytes, nElements);
	  break;
	}
      case OA::OCPI_none:
      case OA::OCPI_scalar_type_limit:
	ocpiAssert(0);
      }
      if (m_isSequence) {
	writer.endSequence(*this);
      }
      if (m_arrayRank ) {
	writer.endArray(*this);
      }
    }

    // Fill the linear buffer from a reader object
    void Member::read(Reader &reader, uint8_t *&data, uint32_t &length) {
      unsigned nElements = 1;
      if (m_isSequence) {
	ralign(data, m_align, length);
	uint32_t *start = (uint32_t *)data;
	radvance(data, m_align, length); // skip over count, check for space
	if (!(*start = nElements = reader.beginSequence(*this)))
	  return;
	if (m_sequenceLength != 0 && nElements > m_sequenceLength)
	  throw Error("Sequence in being read exceeds max length (%u)", m_sequenceLength);
      }

      if ( m_arrayRank ) {
	reader.beginArray(*this, m_nItems);			  
      }

      nElements *= m_nItems;
      ralign(data, m_dataAlign, length);
      switch (m_baseType) {
      case OA::OCPI_Struct:
	reader.beginStruct(*this);
	for (unsigned n = 0; n < nElements; n++)
	  for (unsigned nn = 0; nn < m_nMembers; nn++)
	    m_members[nn].read(reader, data, length);
	reader.endStruct(*this);
	break;
      case OA::OCPI_Type:
	reader.beginType(*this);
	for (unsigned n = 0; n < nElements; n++)
	  m_type->read(reader, data, length);
	reader.endType(*this);
	break;
      case OA::OCPI_String:
	for (unsigned n = 0; n < nElements; n++) {
	  ralign(data, 4, length);
	  const char *chars;
	  uint32_t strLength = reader.beginString(*this, chars, n == 0);
	  if (m_stringLength != 0 && strLength > m_stringLength)
	    throw Error("String being read is larger than max length");
	  uint8_t *start = data;
	  radvance(data, strLength + 1, length); // perform error check
	  memcpy(start, chars, strLength);
	  start[strLength] = 0;
	}
	break;
      default:
	{ // Scalar - write them all at once
	  ralign(data, m_align, length);
	  ReadDataPtr p = {data};
	  unsigned nBytes = nElements * (m_nBits / 8);
	  radvance(data, nBytes, length);
	  reader.readData(*this, p, nBytes, nElements);
	  break;
	}
      case OA::OCPI_none:
      case OA::OCPI_scalar_type_limit:
	ocpiAssert(0);
      }
      if (m_isSequence) {
	reader.endSequence(*this);
      }
      if (m_arrayRank ) {
	reader.endArray(*this);
      }
    }
    void Member::generate(const char *name, unsigned ordinal, unsigned depth) {
      m_name = name;
      m_ordinal = ordinal;
      m_baseType = (OA::BaseType)((random() >> 24) % (OA::OCPI_scalar_type_limit - 1) + 1);
      // printf(" %d", m_baseType);
      if (++depth == 4 && (m_baseType == OA::OCPI_Type || m_baseType == OA::OCPI_Struct))
	m_baseType = OA::OCPI_ULong;
      m_isSequence = random() % 3 == 0;
      if (m_isSequence)
	m_sequenceLength = random() & 1 ? 0 : random() % 10;
      if (random() & 1) {
	m_arrayRank = random() % 3 + 1;
	m_arrayDimensions = new uint32_t[m_arrayRank];
	for (unsigned n = 0; n < m_arrayRank; n++) {
	  m_arrayDimensions[n] = random() % 3 + 1;
	  m_nItems *= m_arrayDimensions[n];
	}
      }
      switch (m_baseType) {
      case OA::OCPI_String:
	m_stringLength = random() & 1 ? 0 : random() % testMaxStringLength;
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
	}
	break;
      default:
	break;
      }
    }
    // This static method is shared between parsing members of a structure and parsing arguments
    // to an operation.
    const char *
    Member::parseMembers(ezxml_t mems, unsigned &nMembers, Member *&members,
			 bool isFixed, const char *tag, const char *hasDefault) {
      for (ezxml_t m = ezxml_cchild(mems, tag); m ; m = ezxml_next(m))
	nMembers++;
      if (nMembers) {
	Member *m = new Member[nMembers];
	members = m;
	const char *err = NULL;
	for (ezxml_t mx = ezxml_cchild(mems, tag); mx ; mx = ezxml_next(mx), m++) {
	  if ((err = OE::checkAttrs(mx, OCPI_UTIL_MEMBER_ATTRS,
				    hasDefault ? hasDefault : NULL, NULL)) ||
	      (err = m->parse(mx, isFixed, true, hasDefault, m - members)))
	    return err;
	}
      }
      return NULL;
    }
    const char *Member::
    offset(unsigned &maxAlign, uint32_t &argOffset,
	   unsigned &minSizeBits, bool &diverseSizes, bool &sub32, bool &unBounded, bool isTop) {
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
	unsigned scalarBits;
	if (m_baseType == OA::OCPI_String) {
	  nBytes = m_stringLength + 1;
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
      // Calculate the number of bytes in each element of an array/sequence
      if (nBytes > UINT32_MAX)
	return "Total member size in bytes is too large (> 4G)";
      // Array?
      if (m_arrayRank) {
	uint32_t *p = m_arrayDimensions;
	nBytes = roundup(nBytes, m_align);
	for (unsigned n = 0; n < m_arrayRank; n++, p++) {
	  nBytes *= *p;
	  if (nBytes > UINT32_MAX)
	    return "Total array size in bytes is too large (> 4G)";
	}
      }
      m_dataAlign = m_align; // capture this before adjusting it in the sequence case.
      if (m_isSequence) {
	// Pad the size to be what is required for an array of same.
	nBytes = roundup(nBytes, m_align);
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
      argOffset = roundup(argOffset, m_align);
      m_offset = argOffset;
      argOffset += m_nBytes;
      return 0;
    }
    const char *
    Member::alignMembers(Member *m, unsigned nMembers, 
			 unsigned &maxAlign, uint32_t &myOffset,
			 unsigned &minSizeBits, bool &diverseSizes, bool &sub32, bool &unBounded,
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
