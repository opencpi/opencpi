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

#include <stdint.h>
#include <string.h>
#include <strings.h>
#include "ocpi-config.h"
#include "OcpiUtilCppMacros.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilWorker.h"

namespace OCPI {
  namespace Util {

    namespace OE = OCPI::Util::EzXml;
    Worker::Worker()
      : m_attributes(NULL), m_ports(NULL), m_memories(NULL), m_nPorts(0), m_nMemories(0), m_version(0),
        m_totalPropertySize(0), m_isSource(false), m_isDebug(false), m_nProperties(0), m_properties(NULL),
	m_firstRaw(NULL), m_xml(NULL), m_ordinal(0) {
    }

    Worker::~Worker() {
      delete [] m_ports;
      delete [] m_properties;
      //      delete [] m_tests;
      delete [] m_memories;
    }
    Property *Worker::
    getProperty(const char *id) const {
      Property *p = m_properties;
      for (unsigned n=0; n < m_nProperties; n++, p++)
        if (!strcasecmp(p->m_name.c_str(), id))
          return p;
      return NULL;
    }
    unsigned Worker::
    whichProperty(const char *id) const {
      Property *p = getProperty(id);
      if (p)
	return p->m_ordinal;
      throw Error("Unknown property: \"%s\" for worker \"%s\"", id, m_specName.c_str());
    }
    Property &Worker::findProperty(const char *id) const {
      return *(m_properties + whichProperty(id));
    }
    Port *Worker::findMetaPort(const char *id, const Port *except) const {
      Port *p = m_ports;
      for (unsigned int n = m_nPorts; n; n--, p++)
        if (p != except && !strcasecmp(p->m_name.c_str(), id))
          return p;
      return NULL;
    }
    const char *Worker::finalizeProperties(size_t &offset, uint64_t &totalSize, const IdentResolver *resolver) {
      const char *err;
      assert(m_firstRaw == NULL);
      Property *p = m_properties;
      for (unsigned n = 0; n < m_nProperties; n++, p++) {
	if (p->m_isRaw) {
	  if (!m_firstRaw)
	    m_firstRaw = p;
	} else if ((err = p->offset(offset, totalSize, resolver)))
	  return err;
	if (!strcasecmp("ocpi_debug", p->cname()) && p->m_default && p->m_default->m_Bool)
	  m_isDebug = true;
      }
      if (m_firstRaw) {
	offset = roundUp(offset, 4);
	p = m_properties;
	for (unsigned n = 0; n < m_nProperties; n++, p++)
	  if (p->m_isRaw && (err = p->offset(offset, totalSize, resolver)))
	    return err;
      }
      return NULL;
    }
#if 0
    Test &Worker::findTest(unsigned int testId) const {
       (void)testId;
       ocpiAssert(0); return *m_tests;
    }
#endif
    const char *Worker::
    parse(ezxml_t xml, Attributes *attr) {
      m_xml = xml;
      m_attributes = attr;
      const char *err = OE::getRequiredString(xml, m_name, "name", "worker");
      if (err ||
	  (err = OE::getNumber8(xml, "version", &m_version)) ||
	  (err = OE::getBoolean(xml, "workerEOF", &m_workerEOF)) ||
	  (err = OE::getRequiredString(xml, m_model, "model", "worker")))
	return err;
      if (!OE::getOptionalString(xml, m_specName, "specName"))
	m_specName = m_name;
      const char *slave = ezxml_cattr(xml, "slave");
      if (slave) {
	if (ezxml_cchild(xml, "slave"))
	  return esprintf("cannot have slave elements when you have a slave attribute");
	m_slaves.push_back(slave);
      } else
	for (ezxml_t cx = ezxml_cchild(xml, "slave"); cx; cx = ezxml_cnext(cx)) {
	  const char *w = ezxml_cattr(cx, "worker");
	  if (!w)
	    return esprintf("Missing \"worker\" attribute for \"slave\" element");
	  m_slaves.push_back(w);
	}
      if ((m_nProperties = OE::countChildren(xml, "property")))
	m_properties = new Property[m_nProperties];
      if ((m_nPorts = OE::countChildren(xml, "port")))
	m_ports = new Port[m_nPorts];
      m_nMemories = OE::countChildren(xml, "localMemory");
      if ((m_nMemories += OE::countChildren(xml, "memory")))
        m_memories = new Memory[m_nMemories];
#if 0
      size_t firstRaw;
      bool haveRaw;
      if ((err = OE::getNumber(xml, "firstRaw", &firstRaw, &haveRaw)))
	return err;
      if (haveRaw) {
	m_firstRaw = m_properties + firstRaw;
	ocpiAssert(firstRaw < m_nProperties);
      }
#endif
      // Second pass - decode all information
      Property *prop = m_properties;
      ezxml_t x;
      for (x = ezxml_cchild(xml, "property"); x; x = ezxml_cnext(x), prop++)
        if ((err = prop->parse(x, (unsigned)(prop - m_properties))))
          return esprintf("Invalid xml property description: %s", err);
      prop = m_properties;
      size_t offset = 0;
      uint64_t totalSize = 0;
#if 1
      if ((err = finalizeProperties(offset, totalSize, NULL)))
	return err;
#else
      for (unsigned n = 0; n < m_nProperties; n++, prop++) {
	if (m_firstRaw && prop == m_firstRaw)
	  offset = roundUp(offset, 4);
	prop->offset(offset, totalSize);
      }
#endif
      ocpiAssert(totalSize < UINT32_MAX);
      m_totalPropertySize = OCPI_UTRUNCATE(size_t, totalSize);
      // Ports at this level are unidirectional? Or do we support the pairing at this point?
      // First pass to establish names and xml and ordinals
      Port *p = m_ports;
      unsigned n = 0;
      for (x = ezxml_cchild(xml, "port"); x; x = ezxml_cnext(x), p++, n++)
        if ((err = p->preParse(*this, x, n)))
          return esprintf("Invalid xml port description(1): %s", err);
      // Second pass to do most of the parsing
      p = m_ports;
      for (n = 0; n < m_nPorts; n++, p++)
        if ((err = p->parse()))
          return esprintf("Invalid xml port description(2): %s", err);
      // Third pass to propagate info from one port to another
      p = m_ports;
      bool hasInput = false, hasOutput = false;
      for (unsigned nn = 0; nn < m_nPorts; nn++, p++)
	if ((err = p->postParse()))
          return esprintf("Invalid xml port description(3): %s", err);
        else if (p->m_provider) {
	  if (!p->m_isOptional)
	    hasInput = true;
        } else
	  hasOutput = true;
      m_isSource = hasOutput && !hasInput;
      Memory* m = m_memories;
      for (x = ezxml_cchild(xml, "memory"); x; x = ezxml_cnext(x), m++ )
        if ((err = m->parse(x)))
          return esprintf("Invalid xml local memory description: %s", err);
      for (x = ezxml_cchild(xml, "scaling"); x; x = ezxml_cnext(x)) {
	std::string l_name;
	OE::getOptionalString(x, l_name, "name");
	Port::Scaling s;
	if ((err = s.parse(x, this)))
	  return err;
	if (l_name.empty())
	  m_scaling = s;
	else
	  m_scalingParameters[l_name] = s;
      }
      return NULL;
    }
    // Get a property value from the metadata
    const char *Worker::getValue(const char *sym, ExprValue &val) const {
      // Our builtin symbols take precendence, but can be overridden with @
      if (!strcasecmp(sym, "model")) {
	val.setString(m_model);
	return NULL;
      } else if (!strcasecmp(sym, "platform") && m_attributes) {
	val.setString(m_attributes->platform());
	return NULL;
      } else if (!strcasecmp(sym, "os") && m_attributes) {
	val.setString(m_attributes->os());
	return NULL;
      } else if (!strcasecmp(sym, "host_platform")) {
	val.setString(OCPI_CPP_STRINGIFY(OCPI_PLATFORM));
	return NULL;
      } else if (!strcasecmp(sym, "spec")) {
	val.setString(m_specName);
	return NULL;
      }
      Property *p = m_properties;
      if (sym[0] == '@')
	sym++;
      for (unsigned n = 0; n < m_nProperties; n++, p++)
	if (!strcasecmp(p->m_name.c_str(), sym))
	  return p->getValue(val);
      return esprintf("no property found for identifier \"%s\"", sym);
    }
    const char *Worker::
    getNumber(ezxml_t x, const char *attr, size_t *np, bool *found, size_t defaultValue,
	      bool setDefault) const {
      return OE::getNumber(x, attr, np, found, defaultValue, setDefault);
    }
    void parse3(char *s, std::string &s1, std::string &s2,
		std::string &s3) {
      char *orig = strdup(s), *temp = orig;

      if ((s = strsep(&temp, "-"))) {
	s1 = s;
	if ((s = strsep(&temp, "-"))) {
	  s2 = s;
	  if ((s = strsep(&temp, "-")))
	    s3 = s;
	}
      }
      free(orig);
    }

    void Attributes::parse(ezxml_t x) {
      const char *cp;
      if ((cp = ezxml_cattr(x, "os"))) m_os = cp;
      if ((cp = ezxml_cattr(x, "osVersion"))) m_osVersion = cp;
      if ((cp = ezxml_cattr(x, "arch"))) m_arch = cp;
      if ((cp = ezxml_cattr(x, "platform"))) m_platform = cp;
      if ((cp = ezxml_cattr(x, "runtime"))) m_runtime = cp;
      if ((cp = ezxml_cattr(x, "runtimeVersion"))) m_runtimeVersion = cp;
      if ((cp = ezxml_cattr(x, "tool"))) m_tool = cp;
      if ((cp = ezxml_cattr(x, "toolVersion"))) m_toolVersion = cp;
      // Before 1.3, the attribute was "av_version" but then "ocpi_version"
      if ((cp = ezxml_cattr(x, "av_version"))) m_opencpiVersion = cp;
      if ((cp = ezxml_cattr(x, "ocpi_version"))) m_opencpiVersion = cp;
      if ((cp = ezxml_cattr(x, "opencpiVersion"))) m_opencpiVersion = cp;
      if ((cp = ezxml_cattr(x, "uuid"))) m_uuid = cp;
      OE::getBoolean(x, "dynamic", &m_dynamic);
      validate();
    }

    void Attributes::parse(const char *pString) {
      std::string unused;
      char *p = strdup(pString), *temp = p, *val;

      if ((val = strsep(&temp, "="))) {
	parse3(val, m_os, m_osVersion, unused);
	if ((val = strsep(&temp, "="))) {
	  parse3(val, m_platform, unused, unused);
	  if ((val = strsep(&temp, "="))) {
	    parse3(val, m_tool, m_toolVersion, unused);
	    if ((val = strsep(&temp, "=")))
	      parse3(val, m_runtime, m_runtimeVersion, unused);
	  }
	}
      }
      free(p);
      validate();
    }
    void Attributes::validate() { }
    const char *Worker::s_controlOpNames[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3, s4)  #x,
          OCPI_CONTROL_OPS
#undef CONTROL_OP
	  NULL
    };
    const char *Worker::s_controlStateNames[] = {
#define CONTROL_STATE(s) #s,
      OCPI_CONTROL_STATES
#undef CONTROL_STATE
      NULL
    };
  }
}
