
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
#include "OcpiUtilProperty.h"
#include "OcpiUtilValue.h"
#define PROPERTY_ATTRIBUTES OCPI_UTIL_MEMBER_ATTRS, "Readable", "Writable", "IsTest", "Default"

namespace OCPI {
  namespace API {
    PropertyInfo::PropertyInfo()
      : m_readSync(false), m_writeSync(false), m_isWritable(false),
	m_isReadable(false), m_readError(false), m_writeError(false)
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
    // parse a value from xml for this property, which may be a struct
    // This actually works for any XML that might have a value for the property, including
    // instance property values in an assembly.
    const char *
    Property::parseValue(ezxml_t vx, const char *unparsed, Value &value) {
      (void)vx;
      if (m_baseType == OA::OCPI_Struct)
	return "Struct property values unimplemented";
      return value.parse(unparsed);
    }

    const char *
    Property::parse(ezxml_t prop, unsigned ordinal) {
      unsigned argOffset = 0;
      bool readableConfigs = false, writableConfigs = false, sub32Configs = false;
      return parse(prop, argOffset, readableConfigs, writableConfigs, sub32Configs, true, ordinal);
    }
#define IMPL_ATTRIBUTES \
  "ReadSync", "WriteSync", "ReadError", "WriteError", "Parameter"
      

    const char *
    Property::parse(ezxml_t prop, unsigned &offset,
		    bool &readableConfigs, bool &writableConfigs,
		    bool &argSub32Configs,  bool includeImpl, unsigned ordinal) {
      const char *err;
      bool sub32Configs;
      unsigned maxAlign = 1; // dummy and not really used since property sheet is zero based anyway
      unsigned minSize = 0;
      bool diverseSizes = false;
      bool unBoundedDummy;   // we are precluding unbounded
      uint32_t myOffset = 0; // we can't let the member parser do this due to "parameter" properties

      if ((err = includeImpl ?
	   OE::checkAttrs(prop, "Name", PROPERTY_ATTRIBUTES, IMPL_ATTRIBUTES, NULL) :
	   OE::checkAttrs(prop, "Name", PROPERTY_ATTRIBUTES, NULL)) ||
	  (err = Member::parse(prop, true, true, "default", ordinal)) ||
	  (err = Member::offset(maxAlign, myOffset, minSize, diverseSizes, sub32Configs,
				unBoundedDummy)))
	return err;
      if (includeImpl &&
	  (err = parseImplAlso(prop)))
	return err;
      if (!m_isParameter) {
	// Member::parse would have done this for us, but it doesn't know about parameters.
	offset = roundup(offset, m_align);
	m_offset = offset;
	offset += m_nBytes;
	//printf("%s at %x(word) %x(byte)\n", p->name, p->offset/4, p->offset);
	if ((err = OE::getBoolean(prop, "Readable", &m_isReadable)) ||
	    (err = OE::getBoolean(prop, "Writable", &m_isWritable)))
	  return err;
	if (m_isReadable)
	  readableConfigs = true;
	if (m_isWritable)
	  writableConfigs = true;
	if (sub32Configs)
	  argSub32Configs = true;
      }
      return 0;
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

    const char *Property::
    parseImplAlso(ezxml_t prop) {
      const char *err;
      if ((err = OE::getBoolean(prop, "ReadSync", &m_readSync)) ||
	  (err = OE::getBoolean(prop, "WriteSync", &m_writeSync)) ||
	  (err = OE::getBoolean(prop, "ReadError", &m_readError)) ||
	  (err = OE::getBoolean(prop, "WriteError", &m_writeError)) ||
	  (err = OE::getBoolean(prop, "IsTest", &m_isTest)) ||
	  (err = OE::getBoolean(prop, "Parameter", &m_isParameter)))
	return err;
      if (m_isParameter && m_isWritable)
	return esprintf("Property \"%s\" is a parameter and can't be writable", m_name.c_str());
      return 0;
    }
    const char *Property::
    parseImpl(ezxml_t x) {
      const char *err;
      if ((err = OE::checkAttrs(x, "Name", IMPL_ATTRIBUTES, NULL)))
	return err;
      return parseImplAlso(x);
    }

  }
}

