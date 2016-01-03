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

#include <cassert>
#include <string>
#include <ctype.h>
#include <stdint.h>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilPort.h"
#include "OcpiUtilWorker.h"
#include "OcpiUtilMisc.h"

namespace OCPI {
  namespace Util {
    namespace OE = OCPI::Util::EzXml;
    namespace OA = OCPI::API;

    void Port::
    init() {
      m_ordinal = 0;
      m_provider = true;
      m_isProducer = false; // this default is consistent with OWD usage.
      m_isOptional = false;
      m_isBidirectional = false;
      m_isInternal = false;
      m_minBufferCount = 1;
      m_defaultBufferCount = SIZE_MAX;
      m_bufferSize = SIZE_MAX;
      m_xml = NULL;
      m_worker = NULL;
      m_bufferSizePort = -1;
      m_nOpcodes = 0;
      m_clone = false;
      m_parsed = false;
      m_seenSummary = false;
      m_isScalable = false;
      m_isPartitioned = false;
      m_defaultDistribution = Cyclic;
    }
    // When supplied with initial xml, its a "poor man's preparse" - see below.
    // This is used (at least) for runtime standalone ports, not port of a worker (yet).
    Port::
    Port(ezxml_t x) {
      init();
      if (x) {
	m_xml = x;
	OE::getOptionalString(m_xml, m_name, "name");
      }
    }

    // A constructor called by tools
    // If p == NULL, normal construction
    // If p != NULL, we are cloning/morphing from spec to impl
    Port::
    Port(Port *p, Worker &w, ezxml_t x, const char *defaultName, const char *&err)
      : Protocol(p) {
      if (err) // we might be initialized in an initializer list
	return;
      init();
      if (p) {
	// We are morphing from a spec.  Copy only spec-related members
	m_ordinal = p->m_ordinal;
	m_worker = p->m_worker;
	m_name = p->m_name;
	m_provider = p->m_provider;
	m_isProducer = p->m_isProducer;
	m_isOptional = p->m_isOptional;
	m_nOpcodes = p->m_nOpcodes; // in case the spec does not mention protocol etc.
	m_seenSummary = p->m_seenSummary;
	// Set the new xml
	m_xml = x ? x : p->m_xml;
      } else
	err = preParse(w, x, w.m_nPorts++, defaultName);
    }

    // This is the clone constructor from internal to external - i.e. a different worker.
    // No xml here at all.  Used by tools.
    Port::
    Port(const Port &other, Worker &w, const char *name, const char *&err)
      : Protocol(other)
    {
      err = NULL;
      if (w.findMetaPort(name)) {
	err = esprintf("Can't create port named \"%s\" since it already exists", name);
	return;
      }
      init();
      m_worker = &w;
      m_ordinal = w.m_nPorts++;
      m_name = name;
      m_clone = true;
      // everything else copied.
      m_provider = other.m_provider;
      m_isProducer = other.m_isProducer;
      m_isOptional = other.m_isOptional;
      m_isBidirectional = other.m_isBidirectional;
      m_isInternal = other.m_isInternal;
      m_minBufferCount = other.m_minBufferCount;
      m_bufferSize = other.m_bufferSize;
      m_bufferSizePort = other.m_bufferSizePort;
      m_nOpcodes = other.m_nOpcodes;
      m_seenSummary = other.m_seenSummary;
      m_isScalable = other.m_isScalable;
      m_scaleExpr = other.m_scaleExpr;
      m_isPartitioned = other.m_isPartitioned;
      m_opScaling = other.m_opScaling; // we are sharing pointers
      m_defaultDistribution = other.m_defaultDistribution;
      m_defaultPartitioning = other.m_defaultPartitioning;
      m_defaultHashField = other.m_defaultHashField;
    }

    Port::~Port() {}

    // First pass captures worker, name, ordinal and xml
    const char *Port::
    preParse(Worker &w, ezxml_t x, size_t ord, const char *defaultName) {
      const char *name = ezxml_cattr(x, "name");
      if (!name)
	name = defaultName;
      if (!name)
	return "Missing \"name\" attribute for port";
      
      if (w.findMetaPort(name, this))
	return esprintf("Can't create port named \"%s\" since it already exists",
			    m_name.c_str());
      m_name = name;
      m_worker = &w;
      m_ordinal = OCPI_UTRUNCATE(PortOrdinal, ord);
      m_xml = x;
      return NULL;
    }

    // The default runtime protocol parser expects simple inline protocols
    // FIXME: intern the protocols...
    const char *Port::
    parseProtocol() {
      // Initialize everything from the protocol, then other attributes can override the protocol
      ezxml_t protocol = ezxml_cchild(m_xml, "protocol");
      if (protocol) {
	const char *err;
	if ((err = Protocol::parse(protocol, NULL, NULL, NULL, NULL)))
	  return err;
	m_nOpcodes = nOperations();
      } else
	m_nOpcodes = 256;
      return NULL;
    }

    // second pass parsing can use names of other ports.
    // Note this parsing will occur on the spec port, and then later when it morphs to
    // an impl port.
    const char *Port::
    parse() {
      m_parsed = true;
      const char *err;
      bool providerFound = false;
      // Initialize everything from the protocol, then other attributes can override the protocol
      if ((err = parseProtocol()) || // virtual call
	  (err = OE::getBoolean(m_xml, "twoWay", &m_isTwoWay)) ||           // protocol override
	  (err = OE::getBoolean(m_xml, "bidirectional", &m_isBidirectional)) ||
	  // Don't use absence to set value
	  (err = OE::getBoolean(m_xml, "producer", &m_isProducer, false, false)) ||
	  (err = OE::getBoolean(m_xml, "provider", &m_provider, false, false,
				&providerFound)) ||
	  // Be sure we don't clobber a spec that has set optional,
	  // but impls can have optional ports in devices...
	  (err = OE::getBoolean(m_xml, "optional", &m_isOptional, true)) ||
	  (err = OE::getNumber(m_xml, "minBufferCount", &m_minBufferCount, 0, 1)) ||
	  (err = OE::getNumber(m_xml, "bufferCount", &m_defaultBufferCount, NULL, 0, false)) ||
	  (err = OE::getNumber(m_xml, "minBuffers", &m_minBufferCount, 0, m_minBufferCount)) ||
	  // Be careful not to clobber protocol-determined values (i.e. don't set default values)
	  (err = OE::getNumber(m_xml, "NumberOfOpcodes", &m_nOpcodes, NULL, 0, false)) ||
	  (err = parseScaling()))
	return err;
      // Kludgerama: in user xml, internal is a port name, in artificat xml it is boolean
      m_isInternal = ezxml_cattr(m_xml, "internal") != NULL;
      if (providerFound)
	m_isProducer = !m_provider;
      else
	m_provider = !m_isProducer;
      const char *bs = ezxml_cattr(m_xml, "bufferSize");
      if (bs && !isdigit(bs[0])) {
	Port *bsp = m_worker->findMetaPort(bs);
	if (bsp)
	  m_bufferSizePort = bsp->m_ordinal;
	else
	  return esprintf("Buffersize set to '%s', which is not a port name or a number", bs);
      } else if ((err = OE::getNumber(m_xml, "bufferSize", &m_bufferSize, 0, 0, false)))
	return err;
      if (m_bufferSize == SIZE_MAX)
	if (m_defaultBufferSize)
	  m_bufferSize = m_defaultBufferSize; // from protocol
	else if (m_maxMessageValues)
	  m_defaultBufferSize = (m_maxMessageValues * m_dataValueWidth + 7) / 8;
      // FIXME: do we need the separately overridable nOpcodes here?
      return NULL;
    }
    const char *Port::
    postParse() {
      if (m_bufferSizePort != -1)
	m_bufferSize = m_worker->port(m_bufferSizePort).m_bufferSize;
      return parseScaling();
    }

    // Start with the size in the port metadata, which we assume has come from
    // or overrided the default from the protocol.  If it is SIZE_MAX, then there is
    // no protocol and no default at all.
    size_t Port::
    getBufferSize(const PValue *portParams, const PValue *connParams) {
      size_t size = m_bufferSize;
      const char *type = "default";
      do {
	if (size == SIZE_MAX) {
	  size = DEFAULT_BUFFER_SIZE;
	  if (size < m_minBufferSize)
	    break;
	}
	type = "port";
	OA::ULong ul;
	if (findULong(portParams, "bufferSize", ul) && (size = ul) < m_minBufferSize)
	  break;
	type = "connection";
	if (findULong(connParams, "bufferSize", ul) && (size = ul) < m_minBufferSize)
	  break;
	return size;
      } while (0);
      throw Error("%s bufferSize %zu is below minimum for worker %s port %s of: %zu",
		  type, (size_t)size, m_worker->name().c_str(), m_name.c_str(),
		  m_minBufferSize);
    }

    Port::Distribution Port::
    getDistribution(unsigned op) const {
      if (op < m_nOperations &&
	  op < m_opScaling.size() &&
	  m_opScaling[op])
	return m_opScaling[op]->m_distribution;
      return m_defaultDistribution;
    }

    // Static method to determine the buffer size for a connection.
    // The metaports may be NULL if they are external
    // Challenges:
    //    buffer sizes may legitimately be zero (for ZLM-only protocols)
    //    external ports have no inherent buffer size, but may override
    //    external-to-external MUST specify a buffer size somehow.
    size_t Port::
    determineBufferSize(Port *in, const PValue *paramsIn,
			Port *out, const PValue *paramsOut,
			const PValue *connParams) {
      size_t
	sizeIn = in ? in->getBufferSize(paramsIn, connParams) : SIZE_MAX,
	sizeOut = out ? out->getBufferSize(paramsOut, connParams) : SIZE_MAX;
      size_t size =
	sizeIn == SIZE_MAX ? sizeOut :
	sizeOut == SIZE_MAX ? sizeIn :
	std::max(sizeIn, sizeOut);
      if (size == SIZE_MAX)
	throw Error("Buffer size for connection must be specified");
      return size;
    }

    Port::Scaling::Scaling()
      : m_min(0), m_max(1), m_modulo(1), m_default(1) {
    }

    static inline const char *getWkrNumber(Worker *w, ezxml_t x, const char *attr, size_t *np) {
      return w ? w->getNumber(x, attr, np) : OE::getNumber(x, attr, np, NULL, 0, false);
    }

    // A scaling 
    const char *Port::Scaling::
    parse(ezxml_t x, Worker *w) {
      const char *err;
      if ((err = getWkrNumber(w, x, "min", &m_min)) ||
	  (err = getWkrNumber(w, x, "max", &m_max)) ||
	  (err = getWkrNumber(w, x, "modulo", &m_modulo)) ||
	  (err = getWkrNumber(w, x, "default", &m_default)))
	return err;
      return NULL;
    }
    void Port::Scaling::
    emit(std::string &out, const Scaling *def) const {
      Scaling s;
      if (!def)
	def = &s;
      if (m_min != def->m_min) formatAdd(out, " min='%zu'", m_min);
      if (m_max != def->m_max) formatAdd(out, " max='%zu'", m_max);
      if (m_modulo != def->m_modulo) formatAdd(out, " modulo='%zu'", m_modulo);
      if (m_default != def->m_default) formatAdd(out, " default='%zu'", m_default);
    }

    bool Port::Scaling::
    check(size_t scale, std::string &error) {
      if (scale &&
	  (scale < m_min || m_max && scale > m_max || m_modulo && scale % m_modulo)) {
	format(error, "Scaling value of %zu incompatible with min: %zu, max: %zu, mod: %zu",
		   scale, m_min, m_max, m_modulo);
	return true;
      }
      return false;
    }


    Port::Partitioning::Partitioning()
      : m_sourceDimension(0) {
    }

    const char *Port::Partitioning::
    parse(ezxml_t x, Worker *w) {
      const char *err = m_scaling.parse(x, w);
      if (!err && !(err = getWkrNumber(w, x, "source", &m_sourceDimension)))
	err = m_overlap.parse(x);
      return err;
    }
    void Port::Partitioning::
    emit(std::string &out, const Partitioning *def) const {
      m_scaling.emit(out, def ? &def->m_scaling : NULL);
      m_overlap.emit(out, def ? &def->m_overlap : NULL);
      if (m_sourceDimension && (!def || m_sourceDimension != def->m_sourceDimension))
	formatAdd(out, " source='%zu'", m_sourceDimension);
    }

    Port::Overlap::
    Overlap()
      : m_left(0), m_right(0), m_padding(None) {
    }

    const char *Port::Overlap::s_oNames[] = {
#define OCPI_PADDING(x) #x,
      OCPI_PADDINGS
#undef OCPI_PADDING
      NULL
    };

    const char *Port::Overlap::
    parse(ezxml_t x) {
      const char *err;
      size_t n;
      if ((err = OE::getNumber(x, "left", &m_left)) ||
	  (err = OE::getNumber(x, "right", &m_right)) ||
	  (err = OE::getEnum(x, "padding", s_oNames, "overlap padding", n)))
	return err;
      m_padding = (Padding)n;
      return NULL;
    }

    void Port::Overlap::
    emit(std::string &out, const Overlap *def) const {
      Overlap o;
      if (!def)
	def = &o;
      if (m_left != def->m_left) formatAdd(out, " left='%zu'", m_left);
      if (m_right != def->m_right) formatAdd(out, " right='%zu'", m_right);
      if (m_padding != def->m_padding) formatAdd(out, " padding='%s'", s_oNames[m_padding]);
    }

    const char *Port::s_dNames[] = {
#define OCPI_DISTRIBUTION(d) #d,	  
	OCPI_DISTRIBUTIONS
#undef OCPI_DISTRIBUTION
	NULL
    };
    const char *Port::
    parseDistribution(ezxml_t x, Distribution &d, std::string &hash) {
      const char *err;
      size_t n;
      if ((err = OE::getEnum(x, "distribution", s_dNames, "distribution type", n, d)))
	return err;
      d = (Distribution)n;
      if (m_isProducer && (d == Balanced || d == Hashed))
	return esprintf("For port \"%s\": output ports cannot declare \"balanced\" "
			"or hashed distribution", m_name.c_str());
      if (!m_isProducer && d == Directed)
	d = Random;
      //	return esprintf("For port \"%s\": input ports cannot declare \"directed\" "
      //			"distribution", m_name.c_str());
      if (OE::getOptionalString(m_xml, hash, "hashField")) {
	if (d != Hashed)
	  return
	    esprintf("The \"hashfield\" attribute is only allowed with hashed distribution");
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
    parseOperations() {
      // Now we parse the child elements for operations.
      assert(m_nOpcodes >= m_nOperations);
      m_opScaling.resize(m_nOpcodes, NULL);
      for (ezxml_t ox = ezxml_cchild(m_xml, "operation"); ox; ox = ezxml_next(ox)) {
	const char *err;
	std::string oName;
	if ((err = OE::checkAttrs(ox, "name", DISTRIBUTION_ATTRS, PARTITION_ATTRS, (void*)0)) ||
	    (err = OE::checkElements(ox, "argument", (void*)0)) ||
	    (err = OE::getRequiredString(ox, oName, "name")))
	  return err;
	Operation *op = findOperation(oName.c_str());
	if (!op)
	  return esprintf("There is no operation named \"%s\" in the protocol", oName.c_str());
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

    const char *Port::
    parseScaling() {
      const char *err;
      if (OE::getOptionalString(m_xml, m_scaleExpr, "scale")) {
	// only for assembly scaling
      }
      // Here we parse defaults for operations and arguments.
      if ((err = parseDistribution(m_xml, m_defaultDistribution, m_defaultHashField)) ||
	  (err = m_defaultPartitioning.parse(m_xml, m_worker)) ||
	  (err = parseOperations()))
	return err;
      Operation *op = m_operations;
      for (unsigned o = 0; o < m_nOperations; o++, op++) {
	OpScaling *os = m_opScaling[o];
	Member *arg = op->m_args;
	for (unsigned a = 0; a < op->m_nArgs; a++, arg++)
	  if (arg->m_arrayRank || arg->m_isSequence) {
	    Partitioning *ap = os ? os->m_partitioning[a] : NULL;
	    if (ap) {
	      if (ap->m_scaling.m_min)
		os->m_isPartitioned = true;
	    } else if (os) {
	      if (os->m_defaultPartitioning.m_scaling.m_min != 0) {
		os->m_partitioning[a] = &os->m_defaultPartitioning;
		os->m_isPartitioned = true;
	      }
	    } else if (m_defaultPartitioning.m_scaling.m_min != 0) {
	      os = m_opScaling[o] = new OpScaling(op->m_nArgs);
	      os->m_partitioning[a] = &m_defaultPartitioning;
	      os->m_isPartitioned = true;
	    }
	  }
	if (os && os->m_isPartitioned)
	  m_isPartitioned = true;
      }
      return NULL;
    }

    void Port::
    emitDistribution(std::string &out, const Distribution &d) const {
      formatAdd(out, " distribution='%s'", s_dNames[d - (Distribution)0]);
    }

    void Port::
    emitScalingAttrs(std::string &out) const {
      if (m_scaleExpr.length())
	formatAdd(out, " scale='%s'", m_scaleExpr.c_str());
      if (m_defaultDistribution != Cyclic)
	emitDistribution(out, m_defaultDistribution);
      if (m_defaultHashField.length())
	formatAdd(out, " hashField='%s'", m_defaultHashField.c_str());
      m_defaultPartitioning.emit(out, NULL);
    }
    void Port::
    emitScaling(std::string &out) const {
      Operation *op = m_operations;
      for (unsigned n = 0; n < m_nOperations; n++, op++)
	if (!m_opScaling.empty() && m_opScaling[n])
	  m_opScaling[n]->emit(out, *this, *op);
    }

    void Port::
    emitXml(std::string &out) const {
      formatAdd(out, "  <port name=\"%s\"", m_name.c_str());
      if (m_isBidirectional)
	formatAdd(out, " bidirectional='1'");
      else if (m_isProducer)
	formatAdd(out, " producer='1'");
      if (!m_operations || m_nOpcodes != m_nOperations)
	formatAdd(out, " numberOfOpcodes=\"%zu\"", m_nOpcodes);
      if (m_minBufferCount != 1)
	formatAdd(out, " minBufferCount=\"%zu\"", m_minBufferCount);
      if (m_defaultBufferCount != SIZE_MAX)
	formatAdd(out, " BufferCount=\"%zu\"", m_defaultBufferCount);
      if (m_bufferSizePort != -1)
	formatAdd(out, " buffersize='%s'", m_worker->port(m_bufferSizePort).m_name.c_str());
      else if (m_bufferSize != SIZE_MAX && m_bufferSize != m_defaultBufferSize)
	formatAdd(out, " bufferSize='%zu'", m_bufferSize);
      if (m_isOptional)
	formatAdd(out, " optional=\"%u\"", m_isOptional);
      if (m_isInternal)
	formatAdd(out, " internal='1'");
      emitScalingAttrs(out);
      formatAdd(out, ">\n");
      printXML(out, 2);
      emitScaling(out);
      formatAdd(out, "  </port>\n");
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
    parse(Port &dp, const Operation &op, ezxml_t x) {
      Worker *w = dp.m_worker;
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
    void Port::OpScaling::
    emit(std::string &out, const Port &port, const Operation &op) const {
      formatAdd(out, "<operation name='%s'", op.m_name.c_str());
      if (m_distribution != port.m_defaultDistribution)
	port.emitDistribution(out, m_distribution);
      if (m_hashField && m_hashField->m_name != port.m_defaultHashField)
	formatAdd(out, " hashField='%s'", m_hashField->m_name.c_str());
      m_defaultPartitioning.emit(out, &port.m_defaultPartitioning);
      if (m_multiple) formatAdd(out, " multiple='1'");
      if (m_allSeeOne) formatAdd(out, " allSeeOne='1'");
      if (m_allSeeEnd) formatAdd(out, " allSeeEnd='1'");
      Member *a = op.m_args;
      bool first = true;
      for (unsigned n = 0; n < op.m_nArgs; n++, a++)
	if (m_partitioning[n]) {
	  size_t nDims = a->m_isSequence ? 1 : a->m_arrayRank;
	  Partitioning *p = m_partitioning[n];
	  for (unsigned i = 0; i < nDims; i++, p++) {
	    if (first) {
	      out += ">\n";
	      first = false;
	    }
	    out += "<dimension ";
	    p->emit(out, &m_defaultPartitioning);
	    out += "/>\n";
	  }
	}
      out += first ? "/>\n" : "</operation>\n";
    }
  }
}

