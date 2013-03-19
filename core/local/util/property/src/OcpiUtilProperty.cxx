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

#include <cstring>
#include <cstdio>
#include <cassert>
#include <climits>
#include "OcpiOsAssert.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilValue.h"

// Used both in spec and in impl
#define ACCESS_ATTRIBUTES \
"Readable", "Writable", "Volatile", "Initial"

#define PROPERTY_ATTRIBUTES \
  OCPI_UTIL_MEMBER_ATTRS, ACCESS_ATTRIBUTES, "IsTest", "Default"

// Readable/writable/volatile/initial are here redundantly to allow the implementation to
// have more access than the spec.  I.e. the spec might not specify readable,
// but for debugging the implementation might want it.  Similarly, the spec
// might say "initial", but the implementation supports dynamic writability

#define IMPL_ATTRIBUTES \
  ACCESS_ATTRIBUTES, \
  "ReadSync",   /* impl says you need before query */\
  "WriteSync",  /* impl says you need afterconfig */\
  "ReadError",  /* impl says reading this can return an error */\
  "WriteError", /* impl says writing this can return an error */\
  "Parameter",  /* impl is supplying a parameter property */	\
  "Indirect"    /* impl is supplying an indirect address */       


namespace OCPI {
  namespace API {
    PropertyInfo::PropertyInfo()
      : m_readSync(false), m_writeSync(false), m_isWritable(false),
	m_isReadable(false), m_readError(false), m_writeError(false),
	m_isVolatile(false), m_isInitial(false), m_isIndirect(false),
	m_indirectAddr(0)
    {}
  }
  namespace Util {
    namespace OE = EzXml;
    namespace OA = OCPI::API;
    Property::Property()
      : m_smallest(0), m_granularity(0), m_isParameter(false), m_isTest(false), m_dataOffset(0) {
    }
    Property::~Property() {
    }
    // parse a value for this property, which may be a struct
    // instance property values in an assembly.
    const char *
    Property::parseValue(const char *unparsed, Value &value) {
      return value.parse(unparsed);
    }

    const char *
    Property::parse(ezxml_t prop, unsigned ordinal) {
      bool readableConfigs = false, writableConfigs = false, sub32Configs = false;
      return parse(prop, readableConfigs, writableConfigs, sub32Configs, true, ordinal);
    }

    const char *
    Property::parseAccess(ezxml_t prop, bool &readableConfigs, bool &writableConfigs,
			  bool addAccess) {
      const char *err;
      if ((err = OE::getBoolean(prop, "Readable", &m_isReadable, addAccess)) ||
	  (err = OE::getBoolean(prop, "Writable", &m_isWritable, addAccess)) ||
	  (err = OE::getBoolean(prop, "Initial", &m_isInitial, addAccess)) ||
	  (err = OE::getBoolean(prop, "Volatile", &m_isVolatile, addAccess)))
	return err;
      if (m_isInitial)
	m_isWritable = true;
      if (m_isVolatile)
	m_isReadable = true;
      if (m_isReadable)
	readableConfigs = true;
      if (m_isWritable)
	writableConfigs = true;
      if (!m_isWritable && !m_isReadable && !m_isParameter)
	return "property is not readable or writable or a parameter";
      return NULL;
    }
    // This is parsing a newly create property that might be only defined in
    // an implementation (includeImpl == true)
    const char *
    Property::parse(ezxml_t prop, bool &readableConfigs, bool &writableConfigs,
		    bool &sub32Configs,  bool includeImpl, unsigned ordinal) {
      const char *err;

      if ((err = includeImpl ?
	   OE::checkAttrs(prop, "Name", PROPERTY_ATTRIBUTES, IMPL_ATTRIBUTES, NULL) :
	   OE::checkAttrs(prop, "Name", PROPERTY_ATTRIBUTES, NULL)) ||
	  (err = parseAccess(prop, readableConfigs, writableConfigs, false)) ||
	  (err = Member::parse(prop, true, true, "default", ordinal)) ||
	  includeImpl &&
	  (err = parseImplAlso(prop)))
	return err;
      // This call is solely used for sub32 determination.  The rest are ignored here.
      // FIXME: move this determination into the parse to avoid all this...
      unsigned maxAlign = 1; // dummy and not really used since property sheet is zero based anyway
      unsigned minSize = 0;
      bool diverseSizes = false;
      bool unBoundedDummy;   // we are precluding unbounded in any case
      uint32_t myOffset = 0;
      return Member::offset(maxAlign, myOffset, minSize, diverseSizes, sub32Configs,
			    unBoundedDummy);
    }
    // A higher up is creating offsets in a list of properties after we know it all
    void Property::
    offset(unsigned &cumOffset, uint64_t &sizeofConfigSpace) {
      if (m_isIndirect) {
	uint64_t top = m_indirectAddr;
	top += m_nBytes;
	if (top > sizeofConfigSpace)
	  sizeofConfigSpace = top;
	m_offset = m_indirectAddr;
      } else if (!m_isParameter || m_isReadable) {
	cumOffset = roundup(cumOffset, m_align);
	m_offset = cumOffset;
	cumOffset += m_nBytes;
	if (cumOffset > sizeofConfigSpace)
	  sizeofConfigSpace = cumOffset;
      }
    }

#if 0
    const char *Property::
    checkType(Scalar::Type ctype, unsigned n, bool write) {
      if (write && !isWritable)
	return "trying to write a non-writable property";
      if (!write && !isReadable)
	return "trying to read a non-readable property";
      if (isStruct)
	return "struct type used as scalar type";
      if (ctype != members->type.scalar)
	return "incorrect type for this property";
      if (write && n > members->type.length)
	return "sequence or array too long for this property";
      if (!write && n < members->type.length)
	return "sequence or array not large enough for this property";
      return 0;
    }
#endif

    // Shared routine to parse impl-only attributes. Parse access before this.
    const char *Property::
    parseImplAlso(ezxml_t prop) {
      const char *err;
      if ((err = OE::getBoolean(prop, "ReadSync", &m_readSync)) ||
	  (err = OE::getBoolean(prop, "WriteSync", &m_writeSync)) ||
	  (err = OE::getBoolean(prop, "ReadError", &m_readError)) ||
	  (err = OE::getBoolean(prop, "WriteError", &m_writeError)) ||
	  (err = OE::getBoolean(prop, "IsTest", &m_isTest)) ||
	  (err = OE::getBoolean(prop, "Parameter", &m_isParameter)) ||
	  // FIXME: consider allowing this only for HDL somehow.
	  (err = OE::getNumber(prop, "Indirect", &m_indirectAddr, &m_isIndirect, 0, true)))
	return err;
      if (m_isParameter && (m_isWritable | m_isIndirect))
	return esprintf("Property \"%s\" is a parameter and can't be writable or indirect",
			m_name.c_str());
      // FIXME: add overrides: writable clears initial
      return 0;
    }

    // This parses something that is adding impl attributes to an existing property
    const char *Property::
    parseImpl(ezxml_t x, bool &readableConfigs, bool &writableConfigs) {
      const char *err;
      if ((err = OE::checkAttrs(x, "Name", IMPL_ATTRIBUTES, NULL)) ||
	  (err = parseAccess(x, readableConfigs, writableConfigs, true)))
	return err;
      return parseImplAlso(x);
    }
    const char *Property::getValue(ExprValue &val) {
      if (!m_defaultValue)
	return esprintf("property \"%s\" has no value", m_name.c_str());
      if (m_arrayRank || m_isSequence || m_baseType == OA::OCPI_Struct ||
	  m_baseType == OA::OCPI_Type)
	return esprintf("property \"%s\" is an array/sequence/struct", m_name.c_str());
      return m_defaultValue->getValue(val);
    }
  }
}

