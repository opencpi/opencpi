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

// Support for data ports

#include <cassert>
#include "assembly.h"
#include "data.h"

// Constructor when creating a derived impl port either from a spec port
// (based on an existing spec port (sp)), or just an impl-only data port
// (e.g. an internal connection of a crew)
DataPort::
DataPort(Worker &w, ezxml_t x, DataPort *sp, int ordinal, WIPType type, const char *&err)
  : OcpPort(w, x, sp, ordinal, type, NULL, err),
    OU::Port(sp, w, x, pname(), err) {
  if (err)
    return;
  // Now we do implementation-specific initialization that will precede the
  // initializations for specific port types (WSI, etc.)
  if (x &&
      ((err = OU::Port::parse()) || // parse protocol etc. first, then override here
       // Adding optionality in the impl xml is only relevant to devices.
       (err = OE::getBoolean(x, "Optional", &m_isOptional, true))))
    return;
  // Note buffer sizes are all determined in the OU::Util::Port.  FIXME allow parameterized?
  // Data width can be unspecified, specified explicitly, or specified with an expression
  if (!m_dataWidthFound) {
    if (w.m_defaultDataWidth != SIZE_MAX)
      m_dataWidth = w.m_defaultDataWidth;
    else
      m_dataWidth = m_dataValueWidth;  // or granularity?
    if (!m_bwFound)
      m_byteWidth = m_dataWidth;
  } else if (!m_dataValueWidth && !nOperations())
    m_dataValueWidth = m_dataWidth;
  if (m_dataWidth >= m_dataValueWidth) {
    if ((!m_dataValueWidth && m_dataWidth) || (m_dataValueWidth && m_dataWidth % m_dataValueWidth)) {
      err = OU::esprintf("DataWidth (%zu) on port '%s' not a multiple of DataValueWidth (%zu)",
			 m_dataWidth, OU::Port::cname(), m_dataValueWidth);
      return;
    }
  } else if (m_dataWidth && m_dataValueWidth % m_dataWidth) {
    err =  OU::esprintf("DataValueWidth (%zu) on port '%s' not a multiple of DataWidth (%zu)",
			m_dataValueWidth, OU::Port::cname(), m_dataWidth);
    return;
  }
  if (!m_impreciseBurst && !m_preciseBurst)
    m_impreciseBurst = true;
#if 0 // FIXME
  if (m_impreciseBurst && m_preciseBurst)
    return "Both ImpreciseBurst and PreciseBurst cannot be specified for WSI or WMI";
#endif
}

// Constructor when this is the concrete derived class, when parsing spec ports
// Note the "parse" method is called after all spec ports are created - second pass
DataPort::
DataPort(Worker &w, ezxml_t x, int ordinal, const char *&err)
  : OcpPort(w, x, NULL, ordinal, WDIPort, NULL, err),
    OU::Port(NULL, w, x, pname(), err) {
  if (!err)
    err = OE::checkAttrs(x, SPEC_DATA_PORT_ATTRS, (void*)0);
}

// Our special clone copy constructor
DataPort::
DataPort(const DataPort &other, Worker &w , std::string &a_name, size_t count,
	 OCPI::Util::Assembly::Role *role, const char *&err)
  : OcpPort(other, w, a_name, count, err),
    OU::Port(other, w, pname(), err) {
  if (err)
    return;
  if (role) {
    if (!role->m_provider) {
      if (!other.m_isProducer && !other.m_isBidirectional) {
	err = OU::esprintf("external producer role incompatible " "with port %s of worker %s",
			   other.cname(), other.worker().m_implName);
	return;
      }
      m_isProducer = true;
      m_isBidirectional = false;
    } else if (role->m_bidirectional) {
      if (!other.m_isBidirectional) {
	  err = OU::esprintf("external bidirectional role incompatible "
			     "with port %s of worker %s",
			     other.cname(), other.worker().m_implName);
      }
    } else if (role->m_provider) {
      if (other.m_isProducer) {
	  err = OU::esprintf("external consumer role incompatible "
			     "with port %s of worker %s",
			     other.cname(), other.worker().m_implName);
      }
      m_isBidirectional = false;
    }
  }
}

::Port &DataPort::
clone(Worker &, std::string &, size_t, OCPI::Util::Assembly::Role *, const char *&) const {
  assert("Can't clone generic data port" == 0);
}

const char *DataPort::
parse() {
  return OU::Port::parse();
}
// Very poor man's virtual callback
static const char *doProtocolChild(ezxml_t op, void *arg) {
  return ((DataPort *)arg)->parseProtocolChild(op);
}

// Called on each child element of the protocol
const char *DataPort::
parseProtocolChild(ezxml_t op) {
  ezxml_t subProto = 0;
  std::string subFile;
  const char *err;
  if ((err = tryInclude(op, m_file, "protocol", &subProto, subFile, true)))
    return err;
  if (subProto) {
    std::string ofile = OU::Protocol::m_file;
    OU::Protocol::m_file = subFile;
    err = OE::ezxml_children(subProto, doProtocolChild, this);
    OU::Protocol::m_file = ofile;
    return err;
  }
  return OU::Protocol::parseOperation(op);
}

// This is basically a (virtual) callback from the low level OU::Port parser
// It needs the protocol to be parsed at this point.  From tools we allow file includes etc.
// Thus it is entirely replacing the protocol parsing in OU::port
// NOTE:  this is called on a generic data port from the spec, and then CALLED AGAIN
// when that port is morphed into a implementation-specific data port
const char *DataPort::
parseProtocol() {
  ezxml_t pSum;
  std::string protFile;
  const char *err;
  if ((err = tryOneChildInclude(::Port::m_xml, worker().m_file, "ProtocolSummary", &pSum,
				protFile, true)))
    return err;
  const char *protocolAttr = ezxml_cattr(::Port::m_xml, "protocol");
  ezxml_t protocolElem = ezxml_cchild(::Port::m_xml, "Protocol");
  if (m_type != WDIPort && (pSum || protocolAttr || protocolElem))
    return "cannot have protocol or protocol summary in an OWD";
  if (pSum) {
    if (protocolAttr || protocolElem)
      return "cannot have both Protocol and ProtocolSummary";
    initNoProtocol();
    if ((err = OE::checkAttrs(pSum, OCPI_PROTOCOL_SUMMARY_ATTRS, NULL)) ||
	(err = OU::Protocol::parseSummary(pSum)))
      return err;
  } else {
    ezxml_t protx = NULL;
    if ((err = tryOneChildInclude(::Port::m_xml, worker().m_file, "Protocol", &protx, protFile,
				  true)))
      return err;
    if (protocolAttr) {
      if (protx)
	return "can't have both 'protocol' element (maybe xi:included) and 'protocol' attribute";
      if ((err = parseFile(protocolAttr, worker().m_file.c_str(), "protocol", &protx, protFile,
			   false)))
	return err;
    }
    // The protx comes from an include, a child element, or the protocol attr file
    if (protx) {
      if (m_type != WDIPort)
	return "cannot have protocol included in an OWD";
      std::string l_name;
      const char *file = worker().m_file.c_str();
      if (protFile.length() && protFile != worker().m_file) {
	// If we are being parsed from a protocol file, default the name.
	const char *path = protFile.c_str();
	const char *start = strrchr(path, '/');
	if (start)
	  start++;
	else
	  start = path;
	const char *last = strrchr(path, '.');
	if (!last)
	  last = path + strlen(path);
	last = checkSuffix(start, "_protocol", last);
	last = checkSuffix(start, "_prot", last);
	last = checkSuffix(start, "-prot", last);
	l_name.assign(start, (size_t)(last - start));
	file = protFile.c_str();
      } else if (protocolElem)
	// If we are being parsed from an immediate element, default the name from port name.
	l_name = cname();
      return OU::Protocol::parse(protx, l_name.c_str(), file, doProtocolChild, this);
    }
    // No protocolsummary nor protocol elements/attributes, but perhaps top-level protocol summary attrs
    if (!m_morphed)
      initNoProtocol();
    // Allow port level overrides for protocol
    if ((err = parseSummary(::Port::m_xml)))
      return err;
  }
  // For protocolsummary or possible protocol summary attributes on on the port itself, finish the parsing
  return finishParse();
}

const char *DataPort::
addProperty(const char *a_name, OA::BaseType type, bool isDebug, bool isParameter, bool isInitial,
	    bool isVolatile, bool isImpl, bool isBuiltin, size_t value, const char *enums) {
  if (::Port::m_worker->m_noControl && !isParameter)
    return NULL;
  std::string property;
  OU::format(property, "<property name='ocpi_%s_%s' hidden='1' parameter='%u' debug='%u' type='%s'"
	     "%s initial='%u' volatile='%u'%s%s%s",
	     a_name, cname(), isParameter ? 1 : 0, isDebug ? 1 : 0, OU::baseTypeNames[type],
	     isImpl ? " raw='0'" : "", isInitial ? 1 : 0, isVolatile ? 1 : 0, enums ? " enums='" : "",
	     enums ? enums : "", enums ? "'" : "");
  if (isInitial || isParameter)
    OU::formatAdd(property, " default='%zu'", value);
  property += "/>";
  return worker().addProperty(property.c_str(), isImpl, isBuiltin);
}

// After the specific port types have parsed everything
// and also *AGAIN* after all expressions are resolved when instanced in an assembly
const char *DataPort::
finalize() {
  if (!m_dataValueWidth && !nOperations())
    m_dataValueWidth = m_dataWidth;
  if (m_dataWidth >= m_dataValueWidth) {
    if ((!m_dataValueWidth && m_dataWidth) || (m_dataValueWidth && m_dataWidth % m_dataValueWidth))
      return OU::esprintf("DataWidth (%zu) on port '%s' not a multiple of DataValueWidth (%zu)",
			  m_dataWidth, cname(), m_dataValueWidth);
  } else if (m_dataWidth && m_dataValueWidth % m_dataWidth)
    return OU::esprintf("DataValueWidth (%zu) on port '%s' not a multiple of DataWidth (%zu)",
			m_dataValueWidth, cname(), m_dataWidth);
  // If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
  size_t granuleWidth =
    m_dataValueWidth * m_dataValueGranularity;
  // If messages are always a multiple of datawidth, bytes are datawidth
  if (granuleWidth >= m_dataWidth &&
      (m_dataWidth == 0 || (granuleWidth % m_dataWidth) == 0))
    m_byteWidth = m_dataWidth;
  else
    m_byteWidth = m_dataValueWidth;
  if (m_byteWidth != 0 && m_dataWidth % m_byteWidth)
    return "Specified ByteWidth does not divide evenly into specified DataWidth";
  // Check if this port requires endianness
  // Either the granule is smaller than or not a multiple of data path width
  if (granuleWidth < m_dataWidth || (m_dataWidth && granuleWidth % m_dataWidth))
    worker().m_needsEndian = true;
  size_t max_bytes = 16*1024; // jumbo
  if (!m_isUnbounded && m_maxMessageValues != SIZE_MAX)
    max_bytes = (m_maxMessageValues * m_dataValueWidth + 7) / 8;
  // Now that we know everything about the port, we add properties specific to the port
  // We make things "debug" that might add gates/resources
  // Parameters generally do not so they are not debug

  // 1. Protocol-dependent values that are spec-determined (impl == false)
  // name         type,  debug  param initl  volatl impl   builtin value enums
  AP(max_opcode,  UChar, false, true, false, false, false, false,  m_nOpcodes - 1);
  AP(max_bytes,   ULong, false, true, false, false, false, false,  max_bytes);
  // 2. Runtime values provided to the worker
  if (isDataProducer())
    // Add a runtime output size if the protocol does not bound it
    //    if (m_isUnbounded)
    AP(buffer_size, UShort,false, false, true, false, true);  // settable buffer size
    //    else
    //      AP(buffer_size, UShort,  false, true, false, false,  true);  // constant buffer size
  // 3. Statistics counters for all models, debug and volatile
  // name         type,  debug  param  initl  volatl impl   value
  //  AP(messages,    ULong,  true, false, false, true,  false); // messages crossing this port
  //  AP(opcode,      ULong,  true, false, false, true,  false);  // opcode of current message
  //  AP(length,      ULong,  true, false, false, true,  false);  // length of current message
  // 4. HDL-specific statistics (FIXME: in nonexistent HDL-data-port class)
  // name         type,  debug  param  initl  volatl impl   value
  //  AP(state,       Enum,  true,  false, false, true,  true, 0, "between,idle,data,blocked");
  //  AP(between,     ULong,  true,  false, false, true,  true); // cycles between messages
  //  AP(idle,        ULong,  true,  false, false, true,  true); // idle cycles within messages
  //  AP(data,        ULong,  true,  false, false, true,  true); // cycles used to move data
  return NULL;
}

const char *DataPort::
finalizeExternal(Worker &aw, Worker &/*iw*/, InstancePort &ip,
		 bool &/*cantDataResetWhileSuspended*/) {
  return ip.m_externalize ? aw.m_assembly->externalizePort(ip, cname(), NULL) : NULL;
}

static const char *maybeSizeMax(size_t n) {
  static std::string s;
  return n == SIZE_MAX ? "<unspecified>" : OU::format(s, "%zu", n);
}

void DataPort::
emitPortDescription(FILE *f, Language lang) const {
  OcpPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, " %s  This interface is a data interface acting as %s\n",
	  comment, m_isProducer ? "producer" : "consumer");
  const char *protName = OU::Protocol::cname();
  fprintf(f, "  %s   Protocol: \"%s\"\n", comment, protName && protName[0] ? protName : "<none>");
  fprintf(f, "  %s   DataValueWidth: %zu\n", comment, m_dataValueWidth);
  fprintf(f, "  %s   DataValueGranularity: %zu\n", comment, m_dataValueGranularity);
  fprintf(f, "  %s   DiverseDataSizes: %s\n", comment, BOOL(m_diverseDataSizes));
  fprintf(f, "  %s   MaxMessageValues: %s\n", comment, maybeSizeMax(m_maxMessageValues));
  fprintf(f, "  %s   VariableMessageLength: %s\n", comment, BOOL(m_variableMessageLength));
  fprintf(f, "  %s   ZeroLengthMessages: %s\n", comment, BOOL(m_zeroLengthMessages));
  fprintf(f, "  %s   MinMessageValues: %zu\n", comment, m_minMessageValues);
  fprintf(f, "  %s   Unbounded: %s\n", comment, BOOL(m_isUnbounded));
  fprintf(f, "  %s   NumberOfOpcodes: %zu\n", comment, m_nOpcodes);
  fprintf(f, "  %s   DefaultBufferSize: %s\n", comment, maybeSizeMax(m_defaultBufferSize));
  fprintf(f, "  %s   Producer: %s\n", comment, BOOL(m_isProducer));
  fprintf(f, "  %s   Continuous: %s\n", comment, BOOL(m_continuous));
  fprintf(f, "  %s   DataWidth: %zu\n", comment, m_dataWidth);
  fprintf(f, "  %s   ByteWidth: %zu\n", comment, m_byteWidth);
  fprintf(f, "  %s   ImpreciseBurst: %s\n", comment, BOOL(m_impreciseBurst));
  fprintf(f, "  %s   Preciseburst: %s\n", comment, BOOL(m_preciseBurst));
}

void DataPort::
emitRecordInterfaceConstants(FILE *f) {
  OcpPort::emitRecordInterfaceConstants(f);
  // This signal is available to worker code.
  fprintf(f, "  constant ocpi_port_%s_data_width : natural;\n", cname());
}
#if 1
void DataPort::
emitInterfaceConstants(FILE *f, Language lang) {
  OcpPort::emitInterfaceConstants(f, lang);
  emitConstant(f, "ocpi_port_%s_data_width", lang, m_dataWidth);
}
#endif
#if 1
void DataPort::
emitRecordInterface(FILE *f, const char *implName) {
#if 0

  std::string width = m_dataWidthExpr;
  if (m_dataWidthExpr.empty())
    OU::format(width, "%zu", m_dataWidth);
  else
    OU::format(width, "to_integer(%s)", m_dataWidthExpr.c_str());
  fprintf(f, "  constant ocpi_port_%s_data_width : natural := %s;\n", cname(), width.c_str());
  vectorWidth(&ocpSignals[OCP_MData], width, VHDL, false, true);
  fprintf(f, "  constant ocpi_port_%s_MData_width : natural := %s;\n", cname(), width.c_str());
  vectorWidth(&ocpSignals[OCP_MByteEn], width, VHDL, false, true);
  fprintf(f, "  constant ocpi_port_%s_MByteEn_width : natural := %s;\n", cname(), width.c_str());
  vectorWidth(&ocpSignals[OCP_MDataInfo], width, VHDL, false, true);
  std::string extra;
  size_t n = extraDataInfo();
  if (n)
    OU::format(extra, "(%s)+%zu", width.c_str(), n);
  else
    extra = width;
  fprintf(f, "  constant ocpi_port_%s_MDataInfo_width : natural := %s;\n",
	  cname(), extra.c_str());
#endif
  OcpPort::emitRecordInterface(f, implName);
}
#endif

void DataPort::
emitRecordDataTypes(FILE *f) {
  if (m_nOpcodes > 1) {
    if (operations()) {
      // See if this protocol has already been defined
      size_t maxOpcodes = 0;
      unsigned first = 0;
      for (unsigned nn = 0; nn < worker().m_ports.size(); nn++)
	if (worker().m_ports[nn]->isData()) {
	  DataPort *dp = static_cast<DataPort*>(worker().m_ports[nn]);
	  if (dp->operations() &&
	      !strcasecmp(dp->OU::Protocol::cname(), OU::Protocol::cname())) {
	    maxOpcodes = std::max(dp->m_nOpcodes, maxOpcodes);
	    if (!first)
	      first = nn;
	  }
	}
      if (first >= ::Port::m_ordinal) {
	fprintf(f,
		"  -- This enumeration is for the opcodes for protocol %s (%s)\n"
		"  type %s_OpCode_t is (\n",
		OU::Protocol::cname(), OU::Protocol::m_qualifiedName.c_str(),
		OU::Protocol::cname());
	OU::Operation *op = operations();
	unsigned nn;
	for (nn = 0; nn < nOperations(); nn++, op++)
	  fprintf(f, "%s    %s_%s_op_e", nn ? ",\n" : "",
		  OU::Protocol::cname(), op->cname());
	// If the protocol opcodes do not fill the space, fill it
	if (nn < maxOpcodes)
	  for (unsigned o = 0; nn < m_nOpcodes; nn++, o++)
	    fprintf(f, ",%sop%u_e", (o % 10) == 0 ? "\n    " : " ", nn);
	fprintf(f, ");\n");
      }
    } else
      fprintf(f, "  subtype %s_OpCode_t is std_logic_vector(%zu downto 0); -- for %zu opcodes\n",
	      cname(), OU::ceilLog2(m_nOpcodes) - 1, m_nOpcodes);
  }
}
void DataPort::
emitRecordInputs(FILE *f) {
  OcpPort::emitRecordInputs(f);
  // All data ports have a ready input
  fprintf(f,
	  "    ready            : Bool_t;           -- this port is ready for data movement\n");
}
void DataPort::
emitRecordOutputs(FILE *f) {
  OcpPort::emitRecordOutputs(f);
}

void DataPort::
emitVHDLShellPortMap(FILE *, std::string &) {
  assert("unexpected call to emitVHDLShellPortMap" == 0);
}

void DataPort::
emitImplSignals(FILE *) {
  assert("unexpected call to emitImplSignals" == 0);
}

void DataPort::
emitXML(std::string &out) {
  OU::Port::emitXml(out);
}

const char *DataPort::
adjustConnection(Connection &c, bool isProducer, OcpAdapt *myAdapt, bool &myHasExpr,
		 ::Port &otherPort, OcpAdapt *otherAdapt, bool &otherHasExpr, Language lang,
		 size_t &unused) {
  assert(otherPort.isData());
  if (!isProducer) // use role passed to us, which might resolve a bidirectional port
    return otherPort.adjustConnection(c, true, otherAdapt, otherHasExpr, *this, myAdapt, myHasExpr,
				      lang, unused);
  const char *err;
  if ((err = OcpPort::adjustConnection(c, true, myAdapt, myHasExpr, otherPort, otherAdapt, otherHasExpr,
				       lang, unused)))
    return err;
  DataPort
    &prod = *this,
    &cons = *static_cast<DataPort*>(&otherPort);
  OcpAdapt
    *prodAdapt = myAdapt,
    *consAdapt = otherAdapt;
  bool
    &prodHasExpr = myHasExpr,
    &consHasExpr = otherHasExpr;
  // Check WDI compatibility
  // If both sides have protocol, check them for compatibility
  if (prod.nOperations() && cons.nOperations()) {
    if (prod.m_dataValueWidth != cons.m_dataValueWidth)
      return "dataValueWidth incompatibility for connection";
    if (prod.m_dataValueGranularity < cons.m_dataValueGranularity ||
	prod.m_dataValueGranularity % cons.m_dataValueGranularity)
      return "dataValueGranularity incompatibility for connection";
    if (prod.m_maxMessageValues > cons.m_maxMessageValues)
      return "maxMessageValues incompatibility for connection";
    if (prod.OU::Protocol::cname()[0] && cons.OU::Protocol::cname()[0] &&
	strcasecmp(prod.OU::Protocol::cname(), cons.OU::Protocol::cname()))
      return OU::esprintf("protocol incompatibility: producer: %s vs. consumer: %s",
			  prod.OU::Protocol::cname(), cons.OU::Protocol::cname());
    if (prod.nOperations() && cons.nOperations() && 
	prod.nOperations() != cons.nOperations())
      return "numberOfOpcodes incompatibility for connection";
    //  if (prod.u.wdi.nOpcodes > cons.u.wdi.nOpcodes)
    //    return "numberOfOpcodes incompatibility for connection";
    if (prod.m_variableMessageLength && !cons.m_variableMessageLength)
      return "variable length producer vs. fixed length consumer incompatibility";
    if (prod.m_zeroLengthMessages && !cons.m_zeroLengthMessages)
      return "zero length message incompatibility";
  }
  if (prod.m_type != cons.m_type)
    return "profile incompatibility";
  if (prod.m_dataWidth != cons.m_dataWidth)
    return OU::esprintf("dataWidth incompatibility. producer %zu consumer %zu",
			prod.m_dataWidth, cons.m_dataWidth);
  if (cons.m_continuous && !prod.m_continuous)
    return "producer is not continuous, but consumer requires it";
  // Profile-specific error checks and adaptations
  if ((err = prod.adjustConnection(cons, c.m_masterName.c_str(), lang, prodAdapt, consAdapt, unused)))
    return err;
  // Figure out if this instance port has signal adaptations that will require a temp
  // signal bundle for the port.
  if (lang == VHDL)
    for (unsigned n = 0; n < N_OCP_SIGNALS; n++) {
      if (prodAdapt[n].isExpr)
	prodHasExpr = true;
      if (consAdapt[n].isExpr)
	consHasExpr = true;
    }
  return NULL;
}
const char *DataPort::
finalizeHdlDataPort() {
  const char *err = NULL;
  if (m_type == WDIPort) {
    // Do it via XML so we aren't duplicating other code
    char *wsi;
    ocpiCheck(asprintf(&wsi,
		       "<streaminterface name='%s' dataWidth='%zu' impreciseburst='true'/>",
		       cname(), 
		       worker().m_defaultDataWidth != SIZE_MAX ?
		       worker().m_defaultDataWidth : m_dataValueWidth) > 0);
    ezxml_t wsix = ezxml_parse_str(wsi, strlen(wsi));
    /*DataPort *p = */(void)createDataPort<WsiPort>(worker(), wsix, this, -1, err);
    //if (!err)
    //      err = p->checkClock();
  }
  return err;
}

const char *DataPort::
adjustConnection(::Port &, const char *, Language, OcpAdapt *, OcpAdapt *, size_t &) {
  assert("Cannot adjust a generic data connection" == 0);
}

void DataPort::
emitOpcodes(FILE *f, const char *pName, Language lang) {
  if (nOperations()) {
    OU::Operation *op = operations();
    fprintf(f,
	    "  %s Opcode/operation value declarations for protocol \"%s\" on port \"%s\"\n",
	    hdlComment(lang), OU::Protocol::m_name.c_str(), cname());
    for (unsigned n = 0; n < nOperations(); n++, op++)
      if (lang != VHDL)
	fprintf(f, "  localparam [%sOpCodeWidth - 1 : 0] %s%s_Op = %u;\n",
		pName, pName, op->cname(), n);
  }
}

const char *DataPort::
fixDataConnectionRole(OU::Assembly::Role &role) {
  if (role.m_knownRole) {
    if (!m_isBidirectional &&
	(role.m_bidirectional || m_isProducer == role.m_provider))
      return OU::esprintf("Role of port %s of worker %s in connection incompatible with port",
			  cname(), worker().m_implName);
  } else {
    role.m_provider = !m_isProducer;
    role.m_bidirectional = m_isBidirectional;
    role.m_knownRole = true;
  }
  return NULL;
}

unsigned DataPort::
extraDataInfo() const {
  return 0;
}

void DataPort::
initRole(OCPI::Util::Assembly::Role &role) {
  role.m_knownRole = true;
  if (m_isBidirectional)
    role.m_bidirectional = true;
  else
    role.m_provider = !m_isProducer;
}

const char *DataPort::
resolveExpressions(OCPI::Util::IdentResolver &ir) {
  const char *err;
  return (err = OcpPort::resolveExpressions(ir)) ? err : finalize();
}

#if 1 // why was this commented out?
void DataPort::
emitVerilogPortParameters(FILE *f) {
  std::string width = m_dataWidthExpr;
  if (m_dataWidthExpr.empty()) {
    if (m_dataWidth == 0)
      return;
    OU::format(width, "%zu", m_dataWidth);
  } else
    OU::format(width, "%s", m_dataWidthExpr.c_str());
  // FIXME: Can we use some sort of global procedure or macro?
  fprintf(f,
	  "  localparam ocpi_port_%s_data_width = %s;\n"
	  "  localparam ocpi_port_%s_MData_width = \n"
	  "    ocpi_port_%s_data_width == 0 ? 0 :\n"
	  "    ocpi_port_%s_data_width != %zu && %zu != 8 ?\n"
	  "    (8 * ocpi_port_%s_data_width) / %zu :\n"
	  "    ocpi_port_%s_data_width;\n",
	  cname(), width.c_str(), cname(), cname(), cname(), m_byteWidth, m_byteWidth, cname(),
	  m_byteWidth, cname());
  if (ocp.MByteEn.value)
    fprintf(f,
	    "  localparam ocpi_port_%s_MByteEn_width = ocpi_port_%s_data_width / %zu;\n",
	    cname(), cname(), m_byteWidth);
  if (ocp.MDataInfo.value)
    fprintf(f,
	    "  localparam ocpi_port_%s_MDataInfo_width = \n"
	    "    ocpi_port_%s_data_width != %zu && %zu != 8 ?\n"
	    "    ocpi_port_%s_data_width - ((8 * ocpi_port_%s_data_width) / %zu) :\n"
	    "    ocpi_port_%s_data_width;\n",
	    cname(), cname(), m_byteWidth, m_byteWidth, cname(), cname(), m_byteWidth, cname());
}
#endif
