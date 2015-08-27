// Support for data ports

#include <assert.h>
#include "wip.h"
#include "data.h"
#include "hdl.h"

// Constructor when creating a derived impl port either from a spec port
// (based on an existing spec port (sp)), or just an impl-only data port
// (e.g. an internal connection of a crew)
DataPort::
DataPort(Worker &w, ezxml_t x, DataPort *sp, int ordinal, WIPType type, const char *&err)
  : OcpPort(w, x, sp, ordinal, type, NULL, err),
    OU::Port(sp, w, x, cname(), err) {
  if (err)
    return;
  // Now we do implementation-specific initialization that will precede the
  // initializations for specific port types (WSI, etc.)
  if ((err = OE::getNumber(x, "MaxMessageValues", &m_maxMessageValues, NULL, 0, false)) ||
      (err = OE::getNumber(x, "DataValueWidth", &m_dataValueWidth, NULL, 0, false)) ||
      (err = OE::getNumber(x, "DataValueGranularity", &m_dataValueGranularity, NULL, 0,
			   false)) ||
      (err = OE::getBoolean(x, "ZeroLengthMessages", &m_zeroLengthMessages, true)) ||
      (err = OU::Port::parse()))
    return;
  // Note buffer sizes are all determined in the OU::Util::Port.  FIXME allow parameterized?
  // Data width can be unspecified, specified explicitly, or specified with an expression
  if (!m_dataWidthFound) {
    if (w.m_defaultDataWidth >= 0)
      m_dataWidth = (unsigned)w.m_defaultDataWidth;
    else
      m_dataWidth = m_dataValueWidth;  // or granularity?
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
    OU::Port(NULL, w, x, cname(), err) {
  if (!err)
    err = OE::checkAttrs(x, SPEC_DATA_PORT_ATTRS, (void*)0);
}

// Our special clone copy constructor
DataPort::
DataPort(const DataPort &other, Worker &w , std::string &name, size_t count,
	 OCPI::Util::Assembly::Role *role, const char *&err)
  : OcpPort(other, w, name, count, err),
    OU::Port(other, w, cname(), err) {
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
  return OU::Protocol::parseChild(op);
}

// This is basically a callback from the low level OU::Port parser
// It needs the protocol to be parsed at this point.  From tools we allow file includes etc.
// Thus it is entirely replacing the protocol parsing in OU::port
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
  if (pSum) {
    if (protocolAttr || protocolElem)
      return "cannot have both Protocol and ProtocolSummary";
    if ((err = OE::checkAttrs(pSum, "DataValueWidth", "DataValueGranularity",
			      "DiverseDataSizes", "MaxMessageValues", "NumberOfOpcodes",
			      "VariableMessageLength", "ZeroLengthMessages",
			      "MinMessageValues",  (void*)0)) ||
	(err = OE::getNumber(pSum, "NumberOfOpcodes", &m_nOpcodes, NULL, 0, false)) ||
	(err = OU::Protocol::parseSummary(pSum)))
      return err;
    m_seenSummary = true;
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
      std::string name;
      const char *file = worker().m_file.c_str();
      if (protFile.length() && protFile != worker().m_file) {
	// If we are being parsed from a protocol file, default the name.
	const char *file = protFile.c_str();
	const char *start = strrchr(file, '/');
	if (start)
	  start++;
	else
	  start = file;
	const char *last = strrchr(file, '.');
	if (!last)
	  last = file + strlen(file);
	last = checkSuffix(start, "_protocol", last);
	last = checkSuffix(start, "_prot", last);
	last = checkSuffix(start, "-prot", last);
	name.assign(start, last - start);
	file = protFile.c_str();
      } else if (protocolElem)
	// If we are being parsed from an immediate element, default the name from port name.
	name = cname();
      if ((err = OU::Protocol::parse(protx, name.c_str(), file, doProtocolChild, this)))
	return err;
      m_nOpcodes = nOperations();
    } else if (m_nOperations == 0 && !m_seenSummary) { // if we have never parsed a protocol yet
      // When there is no protocol, we force it to variable, unbounded at 64k, diverse, zlm
      // I.e. assume it can do anything up to 64KB
      // But with no operations, we can scale back when connecting to something more specific
      // Note that these values can be overridden at the port level.
      // FIXME: why are these just not the basic defaults?
      m_diverseDataSizes = true;
      m_variableMessageLength = true;
      m_maxMessageValues = 64*1024;
      m_zeroLengthMessages = true;
      m_isUnbounded = true;
      m_nOpcodes = 256;
    }
    // Allow port level overrides for protocol
    if ((err = parseSummary(::Port::m_xml)))
      return err;
  }
  return NULL;
}

// After the specific port types have parsed everything
// and also *AGAIN* after all expressions are resolved when instanced in an assembly
const char *DataPort::
finalize() {
  if (!m_dataValueWidth && !nOperations())
    m_dataValueWidth = m_dataWidth;
  if (m_dataWidth >= m_dataValueWidth) {
    if (m_dataWidth % m_dataValueWidth)
      return OU::esprintf("DataWidth (%zu) on port '%s' not a multiple of DataValueWidth (%zu)",
			  m_dataWidth, cname(), m_dataValueWidth);
  } else if (m_dataValueWidth % m_dataWidth)
    return OU::esprintf("DataValueWidth (%zu) on port '%s' not a multiple of DataWidth (%zu)",
			m_dataValueWidth, cname(), m_dataWidth);
  // If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
  size_t granuleWidth =
    m_dataValueWidth * m_dataValueGranularity;
  // If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
  if (granuleWidth >= m_dataWidth && (m_dataWidth == 0 || (granuleWidth % m_dataWidth) == 0) &&
      !m_zeroLengthMessages)
    m_byteWidth = m_dataWidth;
  else
    m_byteWidth = m_dataValueWidth;
  if (m_byteWidth != 0 && m_dataWidth % m_byteWidth)
    return "Specified ByteWidth does not divide evenly into specified DataWidth";
  // Check if this port requires endianness
  // Either the granule is smaller than or not a multiple of data path width
  if (granuleWidth < m_dataWidth || m_dataWidth && granuleWidth % m_dataWidth)
    worker().m_needsEndian = true;
  return NULL;
}

void DataPort::
emitPortDescription(FILE *f, Language lang) const {
  OcpPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, " %s  This interface is a data interface acting as %s\n",
	  comment, m_isProducer ? "producer" : "consumer");
  fprintf(f, "  %s   Protocol: \"%s\"\n", comment, cname());
  fprintf(f, "  %s   DataValueWidth: %zu\n", comment, m_dataValueWidth);
  fprintf(f, "  %s   DataValueGranularity: %zu\n", comment, m_dataValueGranularity);
  fprintf(f, "  %s   DiverseDataSizes: %s\n", comment, BOOL(m_diverseDataSizes));
  fprintf(f, "  %s   MaxMessageValues: %zu\n", comment, m_maxMessageValues);
  fprintf(f, "  %s   NumberOfOpcodes: %zu\n", comment, m_nOpcodes);
  fprintf(f, "  %s   Producer: %s\n", comment, BOOL(m_isProducer));
  fprintf(f, "  %s   VariableMessageLength: %s\n", comment, BOOL(m_variableMessageLength));
  fprintf(f, "  %s   ZeroLengthMessages: %s\n", comment, BOOL(m_zeroLengthMessages));
  fprintf(f, "  %s   Continuous: %s\n", comment, BOOL(m_continuous));
  fprintf(f, "  %s   DataWidth: %zu\n", comment, m_dataWidth);
  fprintf(f, "  %s   ByteWidth: %zu\n", comment, m_byteWidth);
  fprintf(f, "  %s   ImpreciseBurst: %s\n", comment, BOOL(m_impreciseBurst));
  fprintf(f, "  %s   Preciseburst: %s\n", comment, BOOL(m_preciseBurst));
}

void DataPort::
emitRecordInterfaceConstants(FILE */*f*/) {
  // Before emitting the record, define the constants for the data path width.
#if 0
  fprintf(f,
	  "\n"
	  "  -- Constant declarations related to data width for port \"%s\"\n"
	  "  constant ocpi_port_%s_data_width : natural;\n"
	  "  constant ocpi_port_%s_MData_width : natural;\n"
	  "  constant ocpi_port_%s_MByteEn_width : natural;\n"
	  "  constant ocpi_port_%s_MDataInfo_width : natural;\n",
	  cname(), cname(), cname(), cname(), cname());
#endif
}

#if 1
void DataPort::
emitRecordInterface(FILE *f, const char *implName) {

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
  OcpPort::emitRecordInterface(f, implName);
}
#endif

void DataPort::
emitRecordDataTypes(FILE *f) {
  if (m_nOpcodes > 1) {
    if (operations()) {
      // See if this protocol has already been defined
      unsigned nn;
      for (nn = 0; nn < ::Port::m_ordinal; nn++)
	if (worker().m_ports[nn]->isData()) {
	  DataPort *dp = static_cast<DataPort*>(worker().m_ports[nn]);

	  if (dp->operations() &&
	      !strcasecmp(dp->OU::Protocol::m_name.c_str(), OU::Protocol::m_name.c_str()))
	    break;
	}
      if (nn >= ::Port::m_ordinal) {
	fprintf(f,
		"  -- This enumeration is for the opcodes for protocol %s (%s)\n"
		"  type %s_OpCode_t is (\n",
		OU::Protocol::m_name.c_str(), OU::Protocol::m_qualifiedName.c_str(),
		OU::Protocol::m_name.c_str());
	OU::Operation *op = operations();
	for (nn = 0; nn < nOperations(); nn++, op++)
	  fprintf(f, "%s    %s_%s_op_e", nn ? ",\n" : "",
		  OU::Protocol::m_name.c_str(), op->name().c_str());
	// If the protocol opcodes do not fill the space, fill it
	if (nn < m_nOpcodes) {
	  for (unsigned o = 0; nn < m_nOpcodes; nn++, o++)
	    fprintf(f, ",%sop%u_e", (o % 10) == 0 ? "\n    " : " ", nn);
	}
	fprintf(f, ");\n");
      }
    } else
      fprintf(f, "  subtype %s_OpCode_t is std_logic_vector(%zu downto 0); -- for %zu opcodes\n",
	      cname(), ceilLog2(m_nOpcodes) - 1, m_nOpcodes);
  }
}
void DataPort::
emitRecordInputs(FILE *f) {
  // All data ports have a ready input
  fprintf(f,
	  "    ready            : Bool_t;           -- this port is ready for data movement\n");
}
void DataPort::
emitRecordOutputs(FILE */*f*/) {

}
void DataPort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "%s    %s_in.reset => %s_reset,\n"
	  "    %s_in.ready => %s_ready,\n",
	  last.c_str(), cname(), cname(), cname(), cname());
  if (masterIn()) {
    if (m_dataWidth)
      fprintf(f,
	      "    %s_in.data => %s_data,\n",
	      cname(), cname());
    if (ocp.MByteEn.value)
      fprintf(f, "    %s_in.byte_enable => %s_byte_enable,\n", cname(), cname());
    if (m_nOpcodes > 1)
      fprintf(f, "    %s_in.opcode => %s_opcode,\n", cname(), cname());
    fprintf(f,
	    "    %s_in.som => %s_som,\n"
	    "    %s_in.eom => %s_eom,\n",
	    cname(), cname(), cname(), cname());
    if (m_dataWidth)
      fprintf(f,
	      "    %s_in.valid => %s_valid,\n",
	      cname(), cname());
    if (m_isPartitioned)
      fprintf(f,
	      "    %s_in.part_size   => %s_part_size,\n"
	      "    %s_in.part_offset => %s_part_offset,\n"
	      "    %s_in.part_start  => %s_part_start,\n"
	      "    %s_in.part_ready  => %s_part_ready,\n",
	      cname(), cname(), cname(), cname(), cname(),
	      cname(), cname(), cname());
    fprintf(f,
	    "    %s_out.take => %s_take",
	    cname(), cname());
    if (m_isPartitioned)
      fprintf(f,
	      ",\n"
	      "    %s_out.part_take  => %s_part_take",
	      cname(), cname());
    last = ",\n";
  } else {
    if (m_isPartitioned)
      fprintf(f,
	      "    %s_in.part_ready   => %s_part_ready,\n", cname(), cname());
    fprintf(f,
	    "    %s_out.give => %s_give,\n",
	    cname(), cname());
    if (m_dataWidth)
      fprintf(f,
	      "    %s_out.data => %s_data,\n", cname(), cname());
    if (ocp.MByteEn.value)
      fprintf(f, "    %s_out.byte_enable => %s_byte_enable,\n", cname(), cname());
    if (ocp.MReqInfo.value)
      fprintf(f, "    %s_out.opcode => %s_opcode,\n", cname(), cname());
    fprintf(f,
	    "    %s_out.som => %s_som,\n"
	    "    %s_out.eom => %s_eom,\n",
	    cname(), cname(), cname(), cname());
    if (m_dataWidth)
      fprintf(f,
	      "    %s_out.valid => %s_valid",
	      cname(), cname());
    if (m_isPartitioned)
      fprintf(f,
	      ",\n"
	      "    %s_out.part_size   => %s_part_size,\n"
	      "    %s_out.part_offset => %s_part_offset,\n"
	      "    %s_out.part_start  => %s_part_start,\n"
	      "    %s_out.part_give   => %s_part_give",
	      cname(), cname(), cname(),
	      cname(), cname(), cname(), cname(), cname());
    last = ",\n";
  }
}

void DataPort::
emitImplSignals(FILE *f) {
  //	  const char *tofrom = p->masterIn() ? "from" : "to";
  fprintf(f,
	  "  signal %s_%s  : Bool_t;\n"
	  "  signal %s_ready : Bool_t;\n"
	  "  signal %s_reset : Bool_t; -- this port is being reset from the outside\n",
	  cname(), masterIn() ? "take" : "give", cname(), cname());
  if (m_dataWidth)
    fprintf(f,
	    "  signal %s_data  : std_logic_vector(ocpi_port_%s_data_width-1 downto 0);\n",
	    cname(), cname());
  if (ocp.MByteEn.value)
    fprintf(f, "  signal %s_byte_enable: std_logic_vector(ocpi_port_%s_MByteEn_width-1 downto 0);\n",
	    cname(), cname());
  if (m_preciseBurst)
    fprintf(f, "  signal %s_burst_length: std_logic_vector(%zu downto 0);\n",
	    cname(), ocp.MBurstLength.width - 1);
  if (m_nOpcodes > 1) {
    fprintf(f,
	    "  -- The strongly typed enumeration signal for the port\n"
	    "  signal %s_opcode      : %s_OpCode_t;\n"
	    "  -- The weakly typed temporary signals\n"
	    "  signal %s_opcode_temp : std_logic_vector(%zu downto 0);\n"
	    "  signal %s_opcode_pos  : integer;\n",
	    cname(), operations() ?
	    OU::Protocol::m_name.c_str() : cname(), cname(), ocp.MReqInfo.width - 1, cname());
  }
  fprintf(f,
	  "  signal %s_som   : Bool_t;    -- valid eom\n"
	  "  signal %s_eom   : Bool_t;    -- valid som\n",
	  cname(), cname());
  if (m_dataWidth)
    fprintf(f,
	    "  signal %s_valid : Bool_t;   -- valid data\n", cname());
  if (m_isPartitioned)
    fprintf(f,
	    "  signal %s_part_size        : UShort_t;\n"
	    "  signal %s_part_offset      : UShort_t;\n"
	    "  signal %s_part_start       : Bool_t;\n"
	    "  signal %s_part_ready       : Bool_t;\n"
	    "  signal %s_part_%s        : Bool_t;\n",
	    cname(), cname(), cname(), cname(), cname(), masterIn() ? "take" : "give");
}

void DataPort::
emitXML(std::string &out) {
  OU::Port::emitXml(out);
}

// static method
const char *DataPort::
adjustConnection(const char *masterName,
		 ::Port &prodPort, OcpAdapt *prodAdapt,
		 ::Port &consPort, OcpAdapt *consAdapt,
		 Language lang) {
  assert(prodPort.isData() && consPort.isData());
  DataPort &prod = *static_cast<DataPort*>(&prodPort);
  DataPort &cons = *static_cast<DataPort*>(&consPort);
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
    if (prod.name().size() && cons.name().size() &&
	prod.name() != cons.name())
      return OU::esprintf("protocol incompatibility: producer: %s vs. consumer: %s",
			  prod.name().c_str(), cons.name().c_str());
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
  if (prod.type != cons.type)
    return "profile incompatibility";
  if (prod.m_dataWidth != cons.m_dataWidth)
    return OU::esprintf("dataWidth incompatibility. producer %zu consumer %zu",
			prod.m_dataWidth, cons.m_dataWidth);
  if (cons.m_continuous && !prod.m_continuous)
    return "producer is not continuous, but consumer requires it";
  // Profile-specific error checks and adaptations
  return prod.adjustConnection(cons, masterName, lang, prodAdapt, consAdapt);
}
const char *DataPort::
finalizeHdlDataPort() {
  const char *err = NULL;
  if (type == WDIPort) {
    // Do it via XML so we aren't duplicating other code
    char *wsi;
    asprintf(&wsi, "<streaminterface name='%s' dataWidth='%zu' impreciseburst='true'/>",
	     cname(), 
	     worker().m_defaultDataWidth >= 0 ?
	     worker().m_defaultDataWidth : m_dataValueWidth);
    ezxml_t wsix = ezxml_parse_str(wsi, strlen(wsi));
    DataPort *p = createDataPort<WsiPort>(worker(), wsix, this, -1, err);
    if (!err)
      err = p->checkClock();
  }
  return err;
}

const char *DataPort::
adjustConnection(::Port &, const char *, Language, OcpAdapt *, OcpAdapt *) {
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
		pName, pName, op->name().c_str(), n);
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

