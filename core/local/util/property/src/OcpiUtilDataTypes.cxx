
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
#include "OcpiUtilEzxml.h"
#include "OcpiUtilDataTypes.h"

namespace OCPI {
  namespace Util {
    namespace OA = OCPI::API;
    namespace OE = OCPI::Util::EzXml;

    ValueType::ValueType(OA::BaseType bt)
      : m_baseType(bt), m_arrayRank(0), m_nMembers(0), m_nBits(0), m_align(1),
	m_isSequence(false), m_nBytes(0), m_arrayDimensions(NULL), m_stringLength(0),
	m_sequenceLength(0), m_members(NULL), m_type(NULL), m_enums(NULL), m_nEnums(0)
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

    Member::Member()
      :  m_offset(0), m_isIn(false), m_isOut(false), m_isKey(false), m_defaultValue(NULL)
    {
    }
    Member::~Member() {
      if (m_defaultValue)
	delete m_defaultValue;
    }
    const char *
    Member::parse(ezxml_t xm,
		  unsigned &maxAlign,   // accumulating across the group we are a part of
		  uint32_t &argOffset,  // ditto, so it is our offset, prealignment, coming in
		  unsigned &minSizeBits,// min scalar size seen in bits
		  bool &diverseSizes,
		  bool &sub32,
		  bool &unBounded,
		  bool isFixed,         // should sequences and strings be bounded?
		  bool hasName,
		  bool hasDefault)
    {
      bool found;
      const char *err;
      uint64_t nBytes;
      const char *name = ezxml_cattr(xm, "Name");
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
	if ((err = parseMembers(xm, m_nMembers, m_members, m_align, m_offset,
				minSizeBits, diverseSizes, sub32, unBounded,
				"member", isFixed, hasDefault)))
	  return err;
	if (m_nMembers == 0)
	  return "No struct members under type == \"struct\"";
	nBytes = m_offset;
	m_nBits = m_offset * CHAR_BIT;
      } else if (!strcasecmp(typeName, "type")) {
	m_baseType = OA::OCPI_Type;
	m_type = new Member();
	ezxml_t xt = ezxml_cchild(xm, "type");
	if (!xt)
	  return "missing \"type\" child element under data type with type=\"type\"";
	if ((err = OE::checkAttrs(xt, OCPI_UTIL_MEMBER_ATTRS, NULL)) ||
	    (err = m_type->parse(xt, m_align, m_offset, minSizeBits, diverseSizes, sub32,
				 unBounded, isFixed, false, false)))
	  return err;
	nBytes = m_offset;
	m_nBits = m_offset * CHAR_BIT;
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
	  // enums have a baseTypeSize of 32 per IDL
	}
	m_nBits = baseTypeSizes[m_baseType];
	m_align = (m_nBits + CHAR_BIT - 1) / CHAR_BIT;
	unsigned scalarBits;
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
	  nBytes = m_stringLength + 1;
	  scalarBits = CHAR_BIT;
	} else {
	  nBytes = m_align;
	  scalarBits = m_align * CHAR_BIT;
	}
	// Now the scalar type is finished, and perhaps its a sequence (and perhaps its an array).
	if (minSizeBits) {
	  if (minSizeBits != scalarBits)
	    diverseSizes = true;
	  if (scalarBits < minSizeBits)
	    minSizeBits = scalarBits;
	} else
	  minSizeBits = scalarBits;
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
      } else if ((arrayDimensions = ezxml_cattr(xm, "ArrayDimensions"))) {
	ValueType vt;
	vt.m_baseType = OA::OCPI_ULong;
	vt.m_isSequence = true;
	vt.m_sequenceLength = 10;
	Value v(vt);
	if ((err = v.parse(arrayDimensions)))
	  return esprintf("Error parsing array dimensions: %s", err);
	m_arrayRank = v.m_nElements;
	m_arrayDimensions = v.m_pULong;
	uint32_t *p = v.m_pULong;
	v.m_pULong = NULL;
	for (unsigned n = 0; n < v.m_nElements; n++, p++)
	  if (*p == 0)
	    return "ArrayDimensions cannot have zero values";
      }
      // Calculate the number of bytes in each element of an array/sequence
      if (nBytes > UINT32_MAX)
	return "Total member size in bytes is too large (> 4G)";
      if (m_arrayRank) {
	uint32_t *p = m_arrayDimensions;
	nBytes = roundup(nBytes, m_align);
	for (unsigned n = 0; n < m_arrayRank; n++, p++) {
	  nBytes *= *p;
	  if (nBytes > UINT32_MAX)
	    return "Total array size in bytes is too large (> 4G)";
	}
      }

      // Deal with sequences after arrays (because arrays belong to declarators)

      if ((err = OE::getNumber(xm, "SequenceLength", &m_sequenceLength, &m_isSequence, 0, false)) ||
	  (!m_isSequence &&
	    ((err = OE::getNumber(xm, "SequenceSize", &m_sequenceLength, &m_isSequence, 0, false)))))
	return err;
      if (m_isSequence) {
	nBytes = roundup(nBytes, m_align);
	if (m_sequenceLength != 0)
	  nBytes *= m_sequenceLength;
	else if (isFixed)
	  return "Sequence must have a bounded size";
	if (m_align < 4)
	  m_align = 4;
	nBytes += m_align > 4 ? m_align : 4;
	if (nBytes > UINT32_MAX)
	  return "Total sequence size in bytes is too large (> 4G)";
      }

      // Process default values

      const char *defValue = ezxml_cattr(xm, "Default");
      if (defValue) {
	m_defaultValue = new Value(*this);
	if ((err = m_defaultValue->parse(defValue)))
	  return esprintf("for member %s:", m_name.c_str());
      }

      // Final calcutions for offset and alignment

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
    // This static method is shared between parsing members of a structure and parsing arguments
    // to an operation.
    const char *
    Member::parseMembers(ezxml_t mems, unsigned &nMembers, Member *&members,
			 unsigned &maxAlign, uint32_t &myOffset,
			 unsigned &minSizeBits, bool &diverseSizes, bool &sub32, bool &unBounded,
			 const char *tag, bool isFixed, bool hasDefault) {
      for (ezxml_t m = ezxml_cchild(mems, tag); m ; m = ezxml_next(m))
	nMembers++;
      if (nMembers) {
	Member *m = new Member[nMembers];
	members = m;
	const char *err = NULL;
	for (ezxml_t mx = ezxml_cchild(mems, tag); mx ; mx = ezxml_next(mx), m++) {
	  if ((err = OE::checkAttrs(mx, OCPI_UTIL_MEMBER_ATTRS,
				    hasDefault ? "Default" : NULL, NULL)) ||
	      (err = m->parse(mx, maxAlign, myOffset, minSizeBits, diverseSizes, sub32,
			      unBounded, isFixed, true, hasDefault)))
	    return err;
	}
      }
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
  }
}
