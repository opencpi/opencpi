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
// Note "readable" and "padding" are here for backward compatibility only and will produce warnings
#define ACCESS_ATTRIBUTES \
  "Writable", "Volatile", "Initial", "Parameter", "Padding", "Readable", "Readback"

#define PROPERTY_ATTRIBUTES \
  OCPI_UTIL_MEMBER_ATTRS, ACCESS_ATTRIBUTES, "IsTest", "Default", "Hidden", "Debug", "Value", "raw"

#define IMPL_ATTRIBUTES \
  ACCESS_ATTRIBUTES, \
  "ReadSync",      /* impl says you need before query */\
  "WriteSync",     /* impl says you need afterconfig */\
  "ReadBarrier",   /* impl says you need before query */\
  "WriteBarrier",  /* impl says you need afterconfig */\
  "ReadError",     /* impl says reading this can return an error */\
  "WriteError",    /* impl says writing this can return an error */\
  "Indirect",      /* impl is supplying an indirect address */	\
  "PadBefore",     /* impl is supplying padding bytes */	\
  "ReadScalable",  /* property has scalable read behavior */	\
  "Raw",           /* property access is raw in the implementation */ \
  "isimpl"

namespace OCPI {
  namespace API {
    PropertyInfo::PropertyInfo()
      : m_isWritable(false), m_isInitial(false), m_isParameter(false), m_isVolatile(false),
	m_readSync(false), m_writeSync(false), m_readError(false), m_writeError(false),
	m_isHidden(false), m_isDebug(false), m_isImpl(false), m_isReadable(false)
    {}
  }
  namespace Util {
    namespace OE = EzXml;
    namespace OA = OCPI::API;
    Property::Property()
      : m_smallest(0), m_granularity(0), m_isSub32(false),
	m_isPadding(false), m_isRaw(false), m_rawSet(false), m_isTest(false),
	m_isUsed(false), m_isReadback(false), m_isIndirect(false), m_isBuiltin(false),
	m_specParameter(false), m_specWritable(false), m_specInitial(false),
	m_specReadable(false), m_padBefore(0), m_indirectAddr(0), m_dataOffset(0),
	m_paramOrdinal(0), m_hasValue(false), m_readBarrier(false), m_writeBarrier(false),
	m_reduction(None) {
    }
    Property::~Property() {
    }
    // parse a value for this property, which may be a struct
    // used for instance property values in an assembly, etc.
    const char *
    Property::parseValue(const char *unparsed, Value &value, const char *end,
			 const IdentResolver *resolv) const {
      if (!value.m_vt)
	value.setType(*this);
      const char *err = value.parse(unparsed, end, false, resolv);
      return err ? esprintf("for property %s: %s", cname(), err) : NULL;
    }
    // FIXME find the caller and nuke this one
    const char *
    Property::parse(ezxml_t prop, unsigned ordinal) {
      return parse(prop, true, ordinal);
    }
    // Called in three contexts: spec, worker(worker), specproperty (addAccess)
    // For spec: isSpec == true
    // For worker property: isSpec == false, addAccess == false
    // For worker specproperty: isSpec == false, addAccess == true
    const char *
    Property::parseAccess(ezxml_t prop, bool isSpec, bool addAccess) {
      bool
	valueAttr = ezxml_cattr(prop, "value") != NULL,
	defaultAttr = ezxml_cattr(prop, "default") != NULL;
      // spec attrs need to be initialized since they are true-only
      bool isReadable = false, isWritable = false, isInitial = false, isVolatile = false,
	isParameter = false;
      size_t indirect, padBefore;
      bool isHidden, isPadding, isRaw, isIndirect, isReadback, isPadBefore;
      const char *err;
      // All syntax errors are immediate, but semantic errors are all printed
      if ((err = OE::getBoolean(prop, "Readable", &isReadable, addAccess)) ||
	  (err = OE::getBoolean(prop, "Readback", &isReadback)) ||
	  (err = OE::getBoolean(prop, "Writable", &isWritable, addAccess)) ||
	  (err = OE::getBoolean(prop, "Initial", &isInitial, addAccess)) ||
	  (err = OE::getBoolean(prop, "Volatile", &isVolatile, addAccess)) ||
	  (err = OE::getBoolean(prop, "Parameter", &isParameter, addAccess)) ||
	  (err = OE::getBoolean(prop, "Hidden", &isHidden)) ||
	  (err = OE::getBoolean(prop, "raw", &isRaw)) ||
	  (err = OE::getNumber(prop, "Indirect", &indirect, &isIndirect, 0, true)) ||
	  (err = OE::getNumber(prop, "PadBefore", &padBefore, &isPadBefore)) ||
	  (err = OE::getBoolean(prop, "Padding", &isPadding)))
	return esprintf("for property \"%s\": %s", cname(), err);
      // Checks for bad combinations in any context
      if (isReadable && isReadback)
	return esprintf("Error: for property \"%s\", only of one of the \"readable\" or \"readback\" "
			"attributes are allowed, and \"readable\" is deprecated", cname());
      if (valueAttr && defaultAttr)
	  return esprintf("Error: for property \"%s\", only of one of the \"value\" or \"default\" "
			  "attributes are allowed", cname());
      if (isVolatile) {
	if (isReadable) {
	  ewprintf("for volatile property \"%s\", \"readable\" is invalid and ignored (and it is "
		   "deprecated)", cname());
	  isReadable = false;
	}
	if (isReadback)
	  return esprintf("for volatile property \"%s\", \"readback\" is invalid", cname());
	if (isParameter)
	  return esprintf("Error: for property \"%s\", \"parameter\" and \"volatile\" cannot "
			  "both be true", cname());
	if (!isWritable && !isInitial && !m_isWritable && !m_isInitial && defaultAttr)
	  return esprintf("Error: for volatile property \"%s\", the \"default\" attribute "
			  "should not be used without either \"writable\" or \"initial\" being true",
			  cname());
	if (valueAttr)
	  return esprintf("Error: for volatile property \"%s\", the \"value\" attribute is "
			  "invalid", cname());
      }
      int settables = !!isWritable + !!isInitial + !!isParameter;
      if (settables > 1) {
	if (isParameter) {
	  ewprintf("for property \"%s\", only one of \"writable\", \"initial\" "
		   "and \"parameter\" can be true: \"parameter\" takes precedence; writable and "
		   "initial are ignored here", cname());
	  isWritable = isInitial = false;
	} else {
	  ewprintf("for property \"%s\", only one of \"writable\", \"initial\" "
		   "and \"parameter\" can be true: \"initial\" takes precedence; writable "
		   "is ignored here", cname());
	  isWritable = false;
	}
      } else if (settables == 0) {
	if (defaultAttr && !m_isWritable && !m_isInitial && !m_isParameter)
	  return esprintf("for property \"%s\", the \"default\" attribute is only "
			  "valid for settable properties using \"parameter\", \"initial\" or "
			  "\"writable\"", cname());
	if (valueAttr && !m_isParameter)
	  return esprintf("for property \"%s\", the \"value\" attribute is only "
			  "valid for settable properties using \"parameter\"", cname());
      }
      if (isPadding && (isReadable || isReadback || isWritable || isInitial || isVolatile ||
			isHidden || valueAttr || defaultAttr))
	  return esprintf("for property \"%s\", with \"padding\" == true, "
			  "these attributes are not allowed:  readback, writable, initial, "
			  "volatile, hidden, value, default, readable (deprecated)", cname());
      if (isInitial || isWritable) {
	if (valueAttr)
	  return esprintf("for property \"%s\", with \"writable\" or \"initial\" being "
			  "true, the \"value\" attribute is not allowed:  use \"default\" "
			  "instead", cname());
#if 0 // the zero default is historical and convenient, should be more fully tested
	else if (!defaultAttr && !m_default)
	  ewprintf("for property \"%s\", with \"writable\" or \"initial\" being "
		   "true, the \"default\" attribute should be set, otherwise zero values are "
		   "implied", cname());
#endif
      }
      if (isSpec) {
	if (isReadback)
	  return esprintf("for property \"%s\", in a spec(OCS), the \"readback\" attribute is not valid; "
			  "it should be used in workers that require it", cname());
	if (isPadding)
	  ewprintf("Warning: for property \"%s\", in a spec(OCS), padding is deprecated.  "
		   "Use padBefore in <specproperty> in OWD", cname());
	if (isRaw || isIndirect || isPadBefore)
	  return esprintf("Warning: for property \"%s\", in a spec(OCS), none of these are "
			  "allowed:  padding, padBefore, raw, indirect", cname());
	if (isParameter && !(defaultAttr || valueAttr)) {
	  static bool once = false;
	  if (!once)
	    ewprintf("for property \"%s\", in a spec(OCS), \"parameter\" without \"value\" or "
		     "\"default\" should be changed to default access (readable constant value), "
		     "with the worker perhaps specifying it as a parameter in <specproperty>",
		     cname());
	  once = true;
	}
	// Remember attributes that might be morphed by specproperty
	m_specParameter = isParameter;
	m_specWritable = isWritable;
	m_specInitial = isInitial;
	m_specReadable = isReadable; // actually invalid, but for compatibility
      } else if (addAccess) { // is a spec property
	if (isPadding)
	  return esprintf("Error: for specproperty \"%s\", setting \"padding\" is not allowed",
			  cname());
	if ((defaultAttr || valueAttr) && m_default && m_hasValue)
	  return esprintf("Error: for specproperty \"%s\", setting \"value\" or \"default\" is not "
			  "allowed when there is already a value or default in the spec(OCS)",
			  cname());
	// spec property - what are we allowed to change?
	if (m_specWritable) {
	  if (isWritable)
	    ewprintf("for specproperty \"%s\", it is already specified as writable "
		     "in the spec(OCS)", cname());
	  if (isInitial || isParameter)
	    return esprintf("for specproperty \"%s\", it is \"writable\" is the "
			    "spec(OCS), so neither \"initial\" nor \"parameter\" can be true",
			    cname());
	  isWritable = true;
	} else if (m_specParameter) {
	  if (isParameter)
	    ewprintf("for specproperty \"%s\", it is already specified as "
		     "parameter in the spec(OCS)", cname());
	  if (isInitial || isWritable)
	    return esprintf("Warning: for specproperty \"%s\", it is \"parameter\" is the "
			    "spec(OCS), so neither \"initial\" nor \"writable\" can be true",
			    cname());
	  isParameter = true;
	} else if (m_specInitial) {
	  if (isInitial)
	    ewprintf("for specproperty \"%s\", it is already specified as initial "
		     "in the spec(OCS)", cname());
	  if (isParameter || isWritable) // if it is morphed from initial, start over
	    m_isInitial = m_isWritable = false;
	  else
	    isInitial = true;
	} else if (!m_isVolatile) {
	  // Nothing to do - it is still just readable at initialization time
	}
	if (m_isVolatile) {
	  if (isVolatile)
	    ewprintf("for specproperty \"%s\", it is already specified as volatile "
		     "in the spec(OCS)", cname());
	  if (isReadable || isReadback)
	    return esprintf("Error: for specproperty \"%s\", it is already specified as volatile "
			    "in the spec(OCS), so neither \"readable\"(deprecated) nor \"readback\" can be "
			    "true", cname());
	  isVolatile = true;
	}
	if (m_isReadback) {
	  if (isReadback)
	    ewprintf("for specproperty \"%s\", it is already specified as readback"
		     "in the spec(OCS), which is not recommended in any case", cname());
	  isReadback = true;
	}
	if ((m_specParameter || isParameter) && !m_default && !(defaultAttr || valueAttr))
	  return esprintf("for parameter property \"%s\", in a specproperty, having no \"value\" or "
			  "\"default\" is not valid", cname());
	// end of spec property
      } else {
#if 0 // nope - this is too aggressive for now
	if (isParameter && !(defaultAttr || valueAttr))
	  return esprintf("for worker parameter property \"%s\", having no \"value\" or "
			  "\"default\" is not valid", cname());
#endif
	// Defining a new property in the OWD.
	// No specific error checks that are not above.
      }
      if (isReadable) {
	static bool once = false;
	if (!once)
	  ewprintf("for property \"%s\", the \"readable\" attribute is deprecated: all properties "
		   "are considered readable; workers can use the readback attribute in the OWD when "
		   "required; see the CDG for details", cname());
	once = true;
	isReadback = true; // backward compatibility
      }
      // commit the values
      m_isWritable = isWritable;
      m_isInitial = isInitial;
      m_isVolatile = isVolatile;
      m_isParameter = isParameter;
      m_isReadback = isReadback;
      m_isHidden = isHidden;
      m_isPadding = isPadding;
      m_isRaw = isRaw;
      m_padBefore = padBefore;
      m_isIndirect = isIndirect;
      m_indirectAddr = indirect;
      if (m_isInitial)
	m_isWritable = true; // in this code initial implies writable
      // readable is shorthand for: can we read a value from the worker
      m_isReadable = m_isVolatile || !(m_isParameter || m_isWritable) || m_isReadback;
      return NULL;
    }
    // Common checking for initial parsing and add-impl-stuff parsing
    const char *
    Property::parseCheck() {
#if 0
      if (m_isParameter && ((m_isWritable && !m_isInitial) || m_isIndirect || m_isVolatile))
	return esprintf("Property \"%s\" is a parameter and can't be writable or indirect or volatile",
			m_name.c_str());
#if 0
      if (!m_isWritable && !m_isReadable && !m_isParameter && !m_isPadding)
	return esprintf("Property \"%s\" is not readable or writable or padding or a parameter",
			m_name.c_str());
#endif
      if (!m_isImpl) {
	if (m_isReadable && !m_isVolatile)
	  return esprintf( "Warning: for property \"%s\" in a spec, the \"readable\" attribute "
		  "should not be used", cname());
	if (m_isPadding)
	  return esprintf( "Warning: for property \"%s\" in a spec, the \"padding\" attribute "
		  "should not be used", cname());
      }
#endif
      return NULL;
    }
    // This is parsing a newly create property that might be only defined in
    // an implementation (includeImpl == true)
    const char *
    Property::parse(ezxml_t prop, bool includeImpl, unsigned ordinal,
		    const IdentResolver *resolv) {
      const char *err;

      if ((err = includeImpl ?
	   OE::checkAttrs(prop, "Name", PROPERTY_ATTRIBUTES, IMPL_ATTRIBUTES, NULL) :
	   OE::checkAttrs(prop, "Name", PROPERTY_ATTRIBUTES, NULL)) ||
	  (includeImpl && (err = parseImplAlso(prop))) ||
	  (err = parseAccess(prop, !includeImpl, false)))
	return err;
      // parseAccess has already ensured whether the "value" or "default" attribute is valid
      const char *valueAttr = ezxml_cattr(prop, "value") ? "value" : "default";
      if ((err = Member::parse(prop, !m_isParameter, true, valueAttr, "property", ordinal, resolv)))
	return err;
      // This call is solely used for sub32 determination.  The rest are ignored here.
      // FIXME: move this determination into the parse to avoid all this...
      size_t maxAlign = 1; // dummy and not really used since property sheet is zero based anyway
      size_t minSize = 0;
      bool diverseSizes = false;
      bool unBoundedDummy, variableDummy;   // we are precluding unbounded in any case
      size_t myOffset = 0;
      if ((err = Member::offset(maxAlign, myOffset, minSize, diverseSizes, m_isSub32,
				unBoundedDummy, variableDummy)))
	return err;
#if 0 // moved to parse.cxx of ocpigen
      if (m_isSub32)
	sub32Configs = true;
#endif
      return parseCheck();
    }
    // A higher up is creating offsets in a list of properties after we know it all
    // Here is where is need to ensure that all aspects of the underlying data type
    // are fully resolved.
    const char *Property::
    offset(size_t &cumOffset, uint64_t &sizeofConfigSpace, const IdentResolver *resolver) {
      const char *err = NULL;
      if (resolver && (err = finalize(*resolver, "property", !m_isParameter)))
	return err;
      // Now we evaluate offsets since some sizes may have changed
      size_t maxAlign = 1, minSize = 0, myOffset = 0;
      bool diverseSizes = false, unBounded, variable, isSub32;
      if ((err = Member::offset(maxAlign, myOffset, minSize, diverseSizes, isSub32, unBounded,
				variable)))
	return err;
      if (m_isIndirect) {
	uint64_t top = m_indirectAddr;
	top += m_nBytes;
	if (top > sizeofConfigSpace)
	  sizeofConfigSpace = top;
	m_offset = m_indirectAddr;
      } else if (!m_isParameter || m_isReadback) {
	cumOffset = roundUp(cumOffset, m_align);
	m_offset = cumOffset;
	cumOffset += m_nBytes;
	if (cumOffset > sizeofConfigSpace)
	  sizeofConfigSpace = cumOffset;
      }
      return err;
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
	  (err = OE::getBoolean(prop, "ReadBarrier", &m_readBarrier)) ||
	  (err = OE::getBoolean(prop, "WriteBarrier", &m_writeBarrier)) ||
	  (err = OE::getBoolean(prop, "ReadError", &m_readError)) ||
	  (err = OE::getBoolean(prop, "WriteError", &m_writeError)) ||
	  (err = OE::getBoolean(prop, "Test", &m_isTest)) ||
	  (err = OE::getBoolean(prop, "Raw", &m_isRaw, false, true, &m_rawSet)) ||
	  (err = OE::getBoolean(prop, "Debug", &m_isDebug)) ||
	  // FIXME: consider allowing this only for HDL somehow.
	  (err = OE::getNumber(prop, "Indirect", &m_indirectAddr, &m_isIndirect, 0, true)))
	return err;
      if (m_isParameter && !m_isReadback && m_isRaw)
	return esprintf("Property %s specified as both parameter (without readback) and raw, which "
			"is invalid", cname());
      static const char *reduceNames[] = {
#define OCPI_REDUCE(c) #c,
	OCPI_REDUCTIONS
#undef OCPI_REDUCE
	NULL
      };
      size_t n;
      if ((err = OE::getEnum(prop, "readScalable", reduceNames, "reduction operation", n,
			     m_reduction)))
	return err;
      m_reduction = (Reduction)n;
      // FIXME: add overrides: writable clears initial
      return 0;
    }

    // This parses something that is adding impl attributes to an existing property
    const char *Property::
    parseImpl(ezxml_t x, const IdentResolver *resolv) {
      const char *err;
      if ((err = OE::checkAttrs(x, "Name", IMPL_ATTRIBUTES, "hidden", "default", "value", NULL)) ||
	  (err = parseAccess(x, false, true)) ||
	  (err = parseImplAlso(x)))
	return err;
      const char
	*v = ezxml_cattr(x, "value"),
	*d = ezxml_cattr(x, "default");
      if (m_default && (v || d) && !m_isParameter)
	return esprintf("Implementation property named \"%s\" cannot override "
			"previous default value in spec", m_name.c_str());

      if (v) {
	if (m_hasValue)
	  return esprintf("Property \"%s\" already has a non-default value which cannot be "
			  "overridden", m_name.c_str());
	if ((err = parseDefault(v, "property", resolv)))
	  return err;
	m_hasValue = true;
      } else if (d && (err = parseDefault(d, "property", resolv)))
	return err;
      return parseCheck();
    }
    const char *Property::getValue(ExprValue &val) {
      if (!m_default)
	return esprintf("property \"%s\" has no value", m_name.c_str());
      if (m_arrayRank || m_isSequence || m_baseType == OA::OCPI_Struct ||
	  m_baseType == OA::OCPI_Type)
	return esprintf("property \"%s\" is an array/sequence/struct", m_name.c_str());
      return m_default->getValue(val);
    }
  }
}
