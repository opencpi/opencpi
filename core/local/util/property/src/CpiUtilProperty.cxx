#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <CpiOsAssert.h>
#include <CpiUtilProperty.h>
#include <CpiUtilEzxml.h>

namespace CPI {
  namespace Util {
    namespace CE = EzXml;
    namespace Prop {

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
      return esprintf("Unknown property/argument type: \"%s\"", type);
    type.scalar = (Scalar::Type)(tp - Scalar::names);
  } else
    type.scalar = Scalar::CPI_ULong;
  bits = Scalar::sizes[type.scalar];
  align = (bits + CHAR_BIT - 1) / CHAR_BIT;
  if (align > maxAlign)
    maxAlign = align;
  if (align < 4)
    sub32 = true;
  if (type.scalar == Scalar::CPI_String) {
    if ((err = CE::getNumber(xp, "StringLength", &type.stringLength, &found, 0, false)) ||
	!found &&
	(err = CE::getNumber(xp, "size", &type.stringLength, &found, 0, false)))
      return err;
    if (!found)
      return "Missing StringLength attribute for string type";
    if (type.stringLength == 0)
      return "StringLength cannot be zero";
  } else if (ezxml_cattr(xp, "StringLength"))
    return "StringLength attribute only valid for string types";
  if ((err = CE::getNumber(xp, "SequenceLength", &type.length, &type.isSequence, 0, false)) ||
      !type.isSequence &&
      (err = CE::getNumber(xp, "SequenceSize", &type.length, &type.isSequence, 0, false)) ||
      (err = CE::getNumber(xp, "ArrayLength", &type.length, &type.isArray, 0, false)))
    return err;
  if (type.isSequence && type.isArray)
    return esprintf("Property/argument %s has both Array and Sequence length",
		    name);
  if ((type.isSequence || type.isArray) && type.length == 0)
    return esprintf("Property/argumnt %s: Array or Sequence length cannot be zero",
		    name);
  // Calculate the number of bytes in each element of an array/sequence
  nBytes =
    type.scalar == Scalar::CPI_String ?
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
    if ((err = defaultValue.parse(defValue, type.scalar, type.stringLength)))
      return esprintf("%s: \"%s\" of type %s", err, defValue,
		      Scalar::names[type.scalar]);
    hasDefault = true;
  }
  return 0;
}
const char *
Property::parse(ezxml_t prop) {
  unsigned argOffset = 0;
  bool readableConfigs = false, writableConfigs = false, sub32Configs = false;
  return parse(prop, argOffset, readableConfigs, writableConfigs, sub32Configs, true);
}
#define SPEC_PROPERTIES "Type", "Readable", "Writable", "IsTest", "StringLength", "SequenceLength", "ArrayLength", "Default", "SequenceSize", "Size"
#define IMPL_PROPERTIES "ReadSync", "WriteSync", "ReadError", "WriteError", "Parameter", "IsTest", "Default"

const char *
Property::parse(ezxml_t prop, unsigned &argOffset,
		bool &readableConfigs, bool &writableConfigs,
		bool &sub32Configs,  bool includeImpl) {
  name = ezxml_cattr(prop, "Name");
  if (!name)
    return "Missing Name attribute for property";
  const char *err;
  if ((err = includeImpl ?
       CE::checkAttrs(prop, "Name", SPEC_PROPERTIES, IMPL_PROPERTIES, NULL) :
       CE::checkAttrs(prop, "Name", SPEC_PROPERTIES, NULL)))
    return err;
  const char *typeName = ezxml_cattr(prop, "Type");
  if (typeName && !strcasecmp(typeName, "Struct")) {
    isStruct = true;
    for (ezxml_t m = ezxml_cchild(prop, "Member"); m ; m = ezxml_next(m))
      nMembers++;
    if (nMembers == 0)
      return "No Property elements in Property with type == \"struct\"";
  } else {
    isStruct = false;
    nMembers = 1;
  }
  members = myCalloc(Member, nMembers);
  maxAlign = 1;
  unsigned myOffset = 0;
  bool sub32 = false;
  if (isStruct) {
    bool structArray = false;
    if ((err = CE::getNumber(prop, "SequenceLength", &nStructs, &isStructSequence,
			     1)) ||
	(err = CE::getNumber(prop, "ArrayLength", &nStructs, &structArray, 1)))
      return err;
    if (isStructSequence && structArray)
      return "Cannot have both SequenceLength and ArrayLength on struct properties";
    Member *mem = members;
    for (ezxml_t m = ezxml_cchild(prop, "Property"); m ; m = ezxml_next(m), mem++) {
      if ((err = CPI::Util::EzXml::checkAttrs(m, "Name", "Type", "StringLength",
				   "ArrayLength", "SequenceLength", "Default",
				   (void*)0)) ||
	  (err = mem->parse(m, maxAlign, myOffset, sub32)))
	return err;
    }
  } else if ((err = members->parse(prop, maxAlign, myOffset, sub32)))
    return err;
  nBytes = myOffset;
  argOffset = roundup(argOffset, maxAlign);
  offset = argOffset;
  argOffset += myOffset;
  //printf("%s at %x(word) %x(byte)\n", p->name, p->offset/4, p->offset);
  if ((err = CE::getBoolean(prop, "Readable", &isReadable)) ||
      (err = CE::getBoolean(prop, "Writable", &isWritable)))
    return err;
  if (isReadable)
    readableConfigs = true;
  if (isWritable)
    writableConfigs = true;
  if (sub32)
    sub32Configs = true;
  if (includeImpl)
    return parseImplAlso(prop);
  return 0;
}
#ifndef NDEBUG
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
  if ((err = CE::getBoolean(prop, "ReadSync", &needReadSync)) ||
      (err = CE::getBoolean(prop, "WriteSync", &needWriteSync)) ||
      (err = CE::getBoolean(prop, "ReadError", &readError)) ||
      (err = CE::getBoolean(prop, "WriteError", &writeError)) ||
      (err = CE::getBoolean(prop, "IsTest", &isTest)) ||
      (err = CE::getBoolean(prop, "Parameter", &isParameter)))
    return err;
  if (isParameter && isWritable)
    return esprintf("Property \"%s\" is a parameter and can't be writable", name);
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

