
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
#include <OcpiOsAssert.h>
#include <OcpiUtilProperty.h>
#include <OcpiUtilEzxml.h>

namespace OCPI {
  namespace API {
    PropertyInfo::PropertyInfo()
      : m_readSync(false), m_writeSync(false), m_isWritable(false),
	m_isReadable(false), m_readError(false), m_writeError(false),
	m_offset(0), m_maxAlign(0), m_name(NULL), m_isStruct(false),
	m_isStructSequence(false)
      {}
  }
  namespace Util {
    namespace CE = EzXml;
    namespace Prop {

Member::Member()
  : name(NULL), offset(0), bits(0), align(0), nBytes(0), hasDefault(false)
{
  type.scalar = OCPI::API::OCPI_scalar_type_limit;
  type.isSequence = false;
  type.isArray = false;
  type.stringLength = 0;
  type.length = 0;
}
const char *
Member::parse(ezxml_t xp,
	      unsigned &maxAlign,
	      unsigned &argOffset,
	      bool &sub32)
{
  bool found;
  const char *err;
  name = ezxml_cattr(xp, "Name");
  if (!name)
    return "Missing Name attribute in Property or Argument element";
  const char *typeName = ezxml_cattr(xp, "Type");
  if (typeName) {
    const char **tp;
    for (tp = Scalar::names; *tp; tp++)
      if (!strcasecmp(typeName, *tp))
	break;
    if (!*tp)
      return esprintf("Unknown property/argument type: \"%s\"", typeName);
    type.scalar = (Scalar::Type)(tp - Scalar::names);
  } else
    type.scalar = Scalar::OCPI_ULong;
  bits = Scalar::sizes[type.scalar];
  align = (bits + CHAR_BIT - 1) / CHAR_BIT;
  if (align > maxAlign)
    maxAlign = align;
  if (align < 4)
    sub32 = true;
  if (type.scalar == Scalar::OCPI_String) {
    if ((err = CE::getNumber(xp, "StringLength", &type.stringLength, &found, 0, false)) ||
	( !found &&
	(err = CE::getNumber(xp, "size", &type.stringLength, &found, 0, false))))
      return err;
    if (!found)
      return "Missing StringLength attribute for string type";
    if (type.stringLength == 0)
      return "StringLength cannot be zero";
  } else if (ezxml_cattr(xp, "StringLength"))
    return "StringLength attribute only valid for string types";
  if ((err = CE::getNumber(xp, "SequenceLength", &type.length, &type.isSequence, 0, false)) ||
      ( !type.isSequence &&
      ((err = CE::getNumber(xp, "SequenceSize", &type.length, &type.isSequence, 0, false)) ||
      (err = CE::getNumber(xp, "ArrayLength", &type.length, &type.isArray, 0, false)))))
    return err;
  if (type.isSequence && type.isArray)
    return esprintf("Property/argument %s has both Array and Sequence length",
		    name);
  if ((type.isSequence || type.isArray) && type.length == 0)
    return esprintf("Property/argumnt %s: Array or Sequence length cannot be zero",
		    name);
  if (!type.length)
    type.length = 1;
  // Calculate the number of bytes in each element of an array/sequence
  nBytes =
    type.scalar == Scalar::OCPI_String ?
    roundup(type.stringLength + 1, 4) : (bits + CHAR_BIT - 1) / CHAR_BIT;
  if (type.length)
    nBytes *= type.length;
  if (type.isSequence)
    nBytes += align > 4 ? align : 4;
  argOffset = roundup(argOffset, align);
  offset = argOffset;
  argOffset += nBytes;
  // Process default values
  const char *defValue = ezxml_cattr(xp, "Default");
  if (defValue) {
    if ((err = parseValue(type, defValue, defaultValue)))
      return esprintf("for member %s:", name);
    hasDefault = true;
  }
  return 0;
}

Property::Property()
  : members(NULL), nBytes(0), nMembers(0), smallest(0), granularity(0),
    isParameter(false), isStruct(false), isStructSequence(false),
    nStructs(0), isTest(false), sequenceLength(0), dataOffset(0) {
}
Property::~Property() {
  if (members)
    delete [] members;
}
// parse a value from xml for this property, which may be a struct
const char *
Property::parseValue(ezxml_t x, Scalar::Value &value) {
  // For now, forget about structures
  const char *unparsed = ezxml_cattr(x, "Value");
  if (!unparsed)
    return esprintf("Missing \"value\" attribute for \"%s\" property value",
		    m_name);
  if (m_isStruct)
    return "Struct property values unimplemented";
  return OCPI::Util::Prop::parseValue(members->type, unparsed, value);
}

const char *
Property::parse(ezxml_t prop) {
  unsigned argOffset = 0;
  bool readableConfigs = false, writableConfigs = false, sub32Configs = false;
  return parse(prop, argOffset, readableConfigs, writableConfigs, sub32Configs, true);
}
#define SPEC_PROPERTIES "Type", "Readable", "Writable", "IsTest", "StringLength", "SequenceLength", "ArrayLength", "Default", "SequenceSize", "Size"
#define IMPL_PROPERTIES "ReadSync", "WriteSync", "ReadError", "WriteError", "Parameter", "IsTest", "Default"
      
// This static method is shared between parsing members of a structure and parsing arguments to an operation
const char *
Member::parseMembers(ezxml_t prop, unsigned &nMembers, Member *&members,
		     unsigned &maxAlign, unsigned &myOffset, bool &sub32, const char *tag) {
  for (ezxml_t m = ezxml_cchild(prop, tag); m ; m = ezxml_next(m))
    nMembers++;
  if (nMembers) {
    members = new Member[nMembers];
    Member *mem = members;
    const char *err = NULL;
    for (ezxml_t m = ezxml_cchild(prop, tag); m ; m = ezxml_next(m), mem++)
      if ((err = OCPI::Util::EzXml::checkAttrs(m, "Name", "Type", "StringLength",
					       "ArrayLength", "SequenceLength", "Default",
					       (void*)0)) ||
	  (err = mem->parse(m, maxAlign, myOffset, sub32)))
	return err;
  }
  return NULL;
}

const char *
Property::parse(ezxml_t prop, unsigned &argOffset,
		bool &readableConfigs, bool &writableConfigs,
		bool &argSub32Configs,  bool includeImpl) {
  bool sub32Configs = false;
  m_name = ezxml_cattr(prop, "Name");
  if (!m_name)
    return "Missing Name attribute for property";
  const char *err;
  if ((err = includeImpl ?
       CE::checkAttrs(prop, "Name", SPEC_PROPERTIES, IMPL_PROPERTIES, NULL) :
       CE::checkAttrs(prop, "Name", SPEC_PROPERTIES, NULL)))
    return err;
  const char *typeName = ezxml_cattr(prop, "Type");
  m_maxAlign = 1;
  unsigned myOffset = 0;
  if (typeName && !strcasecmp(typeName, "Struct")) {
    if ((err = Member::parseMembers(prop, nMembers, members, m_maxAlign, myOffset, sub32Configs, "member")))
      return err;
    if (nMembers == 0)
      return "No Property elements in Property with type == \"struct\"";
    isStruct = true;
    bool structArray = false;
    if ((err = CE::getNumber(prop, "SequenceLength", &nStructs, &isStructSequence,
			     1)) ||
	(err = CE::getNumber(prop, "ArrayLength", &nStructs, &structArray, 1)))
      return err;
    if (isStructSequence && structArray)
      return "Cannot have both SequenceLength and ArrayLength on struct properties";
  } else {
    nMembers = 1;
    isStruct = false;
    members = new Member[nMembers];
    if ((err = members->parse(prop, m_maxAlign, myOffset, sub32Configs)))
      return err;
  }
  nBytes = myOffset;
  if (includeImpl &&
      (err = parseImplAlso(prop)))
    return err;
  if (!isParameter) {
    argOffset = roundup(argOffset, m_maxAlign);
    m_offset = argOffset;
    argOffset += myOffset;
    //printf("%s at %x(word) %x(byte)\n", p->name, p->offset/4, p->offset);
    if ((err = CE::getBoolean(prop, "Readable", &m_isReadable)) ||
	(err = CE::getBoolean(prop, "Writable", &m_isWritable)))
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

void Member::
printXml(FILE *f) {
    if (type.scalar != Scalar::OCPI_ULong)
    fprintf(f, " type=\"%s\"", Scalar::names[type.scalar]);
  if (type.scalar == Scalar::OCPI_String)
    fprintf(f, " size=\"%u\"", type.stringLength);
  if (type.isSequence)
    fprintf(f, " sequenceSize=\"%u\"", type.length);
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
  if ((err = CE::getBoolean(prop, "ReadSync", &m_readSync)) ||
      (err = CE::getBoolean(prop, "WriteSync", &m_writeSync)) ||
      (err = CE::getBoolean(prop, "ReadError", &m_readError)) ||
      (err = CE::getBoolean(prop, "WriteError", &m_writeError)) ||
      (err = CE::getBoolean(prop, "IsTest", &isTest)) ||
      (err = CE::getBoolean(prop, "Parameter", &isParameter)))
    return err;
  if (isParameter && m_isWritable)
    return esprintf("Property \"%s\" is a parameter and can't be writable", m_name);
  return 0;
}
const char *Property::
parseImpl(ezxml_t x) {
  const char *err;
  if ((err = CE::checkAttrs(x, "Name", IMPL_PROPERTIES, NULL)))
    return err;
  return parseImplAlso(x);
}

    }}}

