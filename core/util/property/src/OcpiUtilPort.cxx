
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

#include <string>
#include <ctype.h>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilPort.h"
#include "OcpiUtilWorker.h"
#include "OcpiUtilMisc.h"

namespace OCPI {
  namespace Util {
  namespace OE = OCPI::Util::EzXml;

    void Port::
    init() {
      m_ordinal = 0;
      m_provider = false;
      m_optional = false;
      m_bidirectional = false;
      m_minBufferCount = 1;
      m_bufferSize = 0;
      m_xml = NULL;
      m_worker = NULL;
      m_bufferSizePort = NULL;
      m_isScalable = false;
      m_isPartitioned = false;
      m_defaultDistribution = All;
    }
    Port::
    Port(bool prov) {
      init();
      m_provider = prov;
    }

    // Clone constructor - meaning we can trash the source.
    Port::
    Port(Port *p) : Protocol(p) {
      if (p) {
	m_name = p->m_name;
	m_ordinal = p->m_ordinal;
	m_provider = p->m_provider;
	m_optional = p->m_optional;
	m_bidirectional = p->m_bidirectional;
	m_minBufferCount = p->m_minBufferCount;
	m_bufferSize = p->m_bufferSize;
	m_xml = p->m_xml;
	m_worker = p->m_worker;
	Port       *m_bufferSizePort;  // The port we should copy our buffer size from
	m_isScalable = p->m_isScalable;
	m_scaleExpr = p->m_scaleExpr;
	m_isPartitioned = p->m_isPartitioned;
	m_opScaling = p->m_opScaling;
	p->m_opScaling.clear(); // pointers moved to clone
	m_defaultDistribution = p->m_defaultDistribution;
	m_defaultPartitioning = p->m_defaultPartitioning;
	m_defaultHashField = p->m_defaultHashField;
      } else
	init();
    }
    // First pass captures name
    const char *Port::
    preParse(Worker &w, ezxml_t x, PortOrdinal ordinal) {
      m_worker = &w;
      m_ordinal = ordinal;
      m_xml = x;
      return OE::getRequiredString(x, m_name, "name", "port");
    }

    // second pass can use names of other ports.
    const char *Port::
    parse(ezxml_t x) {
      const char *err;
      // Initialize everything from the protocol, then other attributes can override the protocol
      ezxml_t protocol = ezxml_cchild(x, "protocol");
      if (protocol &&
	  (err = Protocol::parse(protocol, NULL, NULL, NULL, NULL)))
	return err;
      if ((err = OE::getBoolean(x, "twoWay", &m_isTwoWay)) ||           // protocol override
	  (err = OE::getBoolean(x, "bidirectional", &m_bidirectional)) ||
	  (err = OE::getBoolean(x, "provider", &m_provider)) ||
	  (err = OE::getBoolean(x, "optional", &m_optional)) ||
	  (err = OE::getNumber(x, "minBufferCount", &m_minBufferCount, 0, 1)))
	return err;
      const char *bs = ezxml_cattr(x, "bufferSize");
      if (bs && !isdigit(bs[0])) {
	if (!(m_bufferSizePort = m_worker->findMetaPort(bs)))
	  return esprintf("Buffersize set to '%s', which is not a port name or a number",
			      bs);
      } else if ((err = OE::getNumber(x, "bufferSize", &m_bufferSize, 0, 0)))
	return err;
      if (m_bufferSize == 0)
	m_bufferSize = m_defaultBufferSize; // from protocol
	  
      // FIXME: do we need the separately overridable nOpcodes here?
      return NULL;
    }
    const char *Port::
    postParse() {
      if (m_bufferSizePort)
	m_bufferSize = m_bufferSizePort->m_bufferSize;
      return NULL;
    }

    Port::Scaling::Scaling()
      : m_min(0), m_max(1), m_modulo(1), m_default(1) {
    }

    // A scaling 
    const char *Port::Scaling::
    parse(ezxml_t x, Worker &w) {
      const char *err;
      if ((err = w.getNumber(x, "min", &m_min, NULL, 0, false)) ||
	  (err = w.getNumber(x, "max", &m_max, NULL, 0, false)) ||
	  (err = w.getNumber(x, "modulo", &m_modulo, NULL, 0, false)) ||
	  (err = w.getNumber(x, "default", &m_default, NULL, 0, false)))
	return err;
      return NULL;
    }
    Port::Partitioning::Partitioning()
      : m_sourceDimension(0) {
    }

    const char *Port::Partitioning::
    parse(ezxml_t x, Worker &w) {
      const char *err = m_scaling.parse(x, w);
      if (!err && !(err = w.getNumber(x, "source", &m_sourceDimension)))
	err = m_overlap.parse(x);
      return err;
    }
    Port::Overlap::
    Overlap()
      : m_left(0), m_right(0), m_padding(None) {
    }

    const char *Port::Overlap::
    parse(ezxml_t x) {
      static const char *pNames[] = {
#define OCPI_PADDING(x) #x,
	OCPI_PADDINGS
#undef OCPI_PADDING
	NULL
      };
      const char *err;
      size_t n;
      if ((err = OE::getNumber(x, "left", &m_left)) ||
	  (err = OE::getNumber(x, "right", &m_right)) ||
	  (err = OE::getEnum(x, "padding", pNames, "overlap padding", n)))
	return err;
      m_padding = (Padding)n;
      return NULL;
    }

    const char *Port::
    parseDistribution(ezxml_t x, Distribution &d, std::string &hash) {
      const char *err;
      static const char *dNames[] = {
#define OCPI_DISTRIBUTION(d) #d,	  
	OCPI_DISTRIBUTIONS
#undef OCPI_DISTRIBUTION
	NULL
      };
      size_t n;
      if ((err = OE::getEnum(x, "distribution", dNames, "distribution type", n, d)))
	return err;
      d = (Distribution)n;
      if (OE::getOptionalString(m_xml, hash, "hashField")) {
	if (d != Hashed)
	  return esprintf("The \"hashfield\" attribute is only allowed with hashed distribution");
	if (!m_operations)
	  return esprintf("The \"hashfield\" attribute cannot be used with there is no protocol");
	Operation *o = m_operations;
	bool found = false;
	for (unsigned n = 0; n < m_nOperations; n++)
	  if (o->findArg(hash.c_str()))
	    found = true;
	if (!found)
	  return esprintf("The \"hashfield\" attribute \"%s\" doesn't match any field "
			      "in the any operation", hash.c_str());
      }
      return err;
    }

    const char *Port::
    parseOperations(ezxml_t x) {
      // Now we parse the child elements for operations.
      for (ezxml_t ox = ezxml_cchild(x, "operation"); ox; ox = ezxml_next(ox)) {
	const char *err;
	std::string oName;
	if ((err = OE::checkAttrs(ox, "name", DISTRIBUTION_ATTRS, PARTITION_ATTRS, (void*)0)) ||
	    (err = OE::checkElements(ox, "argument", (void*)0)) ||
	    (err = OE::getRequiredString(ox, oName, "name")))
	  return err;
	Operation *op = findOperation(oName.c_str());
	if (!op)
	  return esprintf("Here is no operation named \"%s\" in the protocol", oName.c_str());
	size_t ord = op - m_operations;
	if (m_opScaling[ord])
	  return esprintf("Duplicate operation element with name \"%s\" for port \"%s\"",
			  oName.c_str(), m_name.c_str());
	OpScaling *os = new OpScaling(op->m_nArgs);
	if ((err = os->parse(*this, *op, ox)))
	  return err;
	m_opScaling[ord] = os;
      }
      return NULL;
    }

    Port::OpScaling::
    OpScaling(size_t nArgs)
      : m_distribution(All), m_hashField(NULL), m_multiple(false), m_allSeeOne(false),
	m_allSeeEnd(false), m_isPartitioned(false) {
      m_partitioning.resize(nArgs);
      for (size_t n = 0; n < nArgs; n++)
	m_partitioning[n] = NULL;
    }
    const char *Port::OpScaling::
    parse(Port &dp, Operation &op, ezxml_t x) {
      Worker &w = *dp.m_worker;
      const char *err;
      m_defaultPartitioning = dp.m_defaultPartitioning;
      m_distribution = dp.m_defaultDistribution;
      std::string hash;
      if ((err = dp.parseDistribution(x, m_distribution, hash)) ||
	  (err = m_defaultPartitioning.parse(x, w)) ||
	  (err = OE::getBoolean(x, "multiple", &m_multiple)) ||
	  (err = OE::getBoolean(x, "allSeeOne", &m_allSeeOne)) ||
	  (err = OE::getBoolean(x, "allSeeEnd", &m_allSeeEnd)))
	return err;
      if (hash.empty() && m_distribution == Hashed)
	hash = dp.m_defaultHashField;
      if (hash.length() && !(m_hashField = op.findArg(hash.c_str())))
	return esprintf("hashfield attribute value \"%s\" not an argument to \"%s\"",
			hash.c_str(), op.m_name.c_str());
      m_partitioning.resize(op.m_nArgs, 0);
      for (ezxml_t ax = ezxml_cchild(x, "argument"); ax; ax = ezxml_next(ax)) {
	std::string aName;
	if ((err = OE::checkAttrs(ax, "name", PARTITION_ATTRS, (void*)0)) ||
	    (err = OE::checkElements(ax, "dimension", (void*)0)) ||
	    (err = OE::getRequiredString(ax, aName, "name")))
	  return err;
	Member *a = op.findArg(aName.c_str());
	if (!a)
	  return esprintf("name attribute of argument element is not an argument to \"%s\"",
			  op.m_name.c_str());
	size_t nDims = a->m_isSequence ? 1 : a->m_arrayRank;
	Partitioning *p = new Partitioning[nDims];
	m_partitioning[a - op.m_args] = p;
	// We have an array of partitionings for all the dimensions (sequences are 1D).
	// We start with a default based on what is in the argument element.
	Partitioning def = m_defaultPartitioning;
	if ((err = def.parse(ax, w)))
	  return err;
	unsigned n;
	for (n = 0; n < nDims; n++)
	  p[n] = def;
	n = 0;
	for (ezxml_t dx = ezxml_cchild(ax, "dimension"); dx; dx = ezxml_next(dx), n++, p++) {
	  if (n >= nDims)
	    return esprintf("Too many dimensions for argument \"%s\" in operation \"%s\"",
				a->m_name.c_str(), op.m_name.c_str());
	  if ((err = OE::checkAttrs(dx, PARTITION_ATTRS, (void*)0)) ||
	      (err = p->parse(dx, w)))
	    return err;
	}
      }
      return NULL;
    }
  }
}

