// Support for data ports

#include <assert.h>
#include "wip.h"
#include "hdl.h"

// Constructor when creating a derived impl port from a spec port
// Based on an existing spec port (sp), unless impl-only
DataPort::
DataPort(Worker &w, ezxml_t x, Port *sp, int ordinal, WIPType type, const char *&err)
  : OcpPort(w, x, sp, ordinal, type, NULL, err) {
  // assert(sp != NULL);
  //  DataPort *dp = static_cast<DataPort*>(sp);
  // Now we do data port initialization that will precede the initializations for specific port
  // types (WSI, etc.)
  // These are AFTER the protocol parsing is done
  bool dwFound;
  if ((err = parse()) ||
      (err = OE::getNumber(m_xml, "DataWidth", &m_dataWidth, &dwFound)))
    return;
  if (!dwFound) {
    if (w.m_defaultDataWidth >= 0)
      m_dataWidth = (unsigned)w.m_defaultDataWidth;
    else
      m_dataWidth = m_dataValueWidth;  // or granularity?
  } else if (!m_dataValueWidth && !nOperations())
    m_dataValueWidth = m_dataWidth;
  if (m_dataWidth >= m_dataValueWidth) {
    if (m_dataWidth % m_dataValueWidth) {
      err = OU::esprintf("DataWidth (%zu) on port '%s' not a multiple of DataValueWidth (%zu)",
			 m_dataWidth, name(), m_dataValueWidth);
      return;
    }
  } else if (m_dataValueWidth % m_dataWidth) {
    err =  OU::esprintf("DataValueWidth (%zu) on port '%s' not a multiple of DataWidth (%zu)",
			m_dataValueWidth, name(), m_dataWidth);
    return;
  }
  if (!m_impreciseBurst && !m_preciseBurst)
    m_impreciseBurst = true;
#if 0 // FIXME
  if (m_impreciseBurst && m_preciseBurst)
    return "Both ImpreciseBurst and PreciseBurst cannot be specified for WSI or WMI";
#endif
  //  pattern = ezxml_cattr(m_xml, "Pattern");
  // After all this, if there is no default buffer size specified, but there is
  // a maxMessageValues, set the default buffer size from it.
  if (!m_defaultBufferSize && m_maxMessageValues)
    m_defaultBufferSize =
      (m_maxMessageValues * m_dataValueWidth + 7) / 8;
  //  err = parseScaling();
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

static const char *checkSuffix(const char *str, const char *suff, const char *last) {
  size_t nstr = last - str, nsuff = strlen(suff);
  const char *start = str + nstr - nsuff;
  return nstr > nsuff && !strncmp(suff, start, nsuff) ? start : last;
}

// Constructor when this is the concrete derived class, when parsing spec ports
// Note the "parse" method is called after all spec ports are created - second pass
// Note that the underlying OU::Port class calls parseProtocol
DataPort::
DataPort(Worker &w, ezxml_t x, int ordinal, const char *&err)
  : OcpPort(w, x, NULL, ordinal, WDIPort, NULL, err) {
  if (!err && type == WDIPort)
    err = OE::checkAttrs(m_xml, SPEC_DATA_PORT_ATTRS, (void*)0);
}

// Our special clone copy constructor
DataPort::
DataPort(const DataPort &other, Worker &w , std::string &name, size_t count,
	OCPI::Util::Assembly::Role *role, const char *&err)
  : OcpPort(other, w, name, count, err) {
  if (err)
    return;
  //  m_isProducer = other.m_isProducer;
  //  m_isOptional = other.m_isOptional;
  //  m_isBidirectional = other.m_isBidirectional;
  //  m_nOpcodes = other.m_nOpcodes;
  //  m_minBufferCount = other.m_minBufferCount;
  //  m_bufferSize = other.m_bufferSize;
  //  m_bufferSizePort = other.m_bufferSizePort;
  //  m_isScalable = other.m_isScalable;
  //  m_defaultDistribution = other.m_defaultDistribution;
  //  m_opScaling = other.m_opScaling;
  //  m_isPartitioned = other.m_isPartitioned;
  if (role) {
    if (!role->m_provider) {
      if (!other.m_isProducer && !other.m_isBidirectional) {
	err = OU::esprintf("external producer role incompatible " "with port %s of worker %s",
			   other.name(), other.m_worker->m_implName);
	return;
      }
      m_isProducer = true;
      m_isBidirectional = false;
    } else if (role->m_bidirectional) {
      if (!other.m_isBidirectional) {
	  err = OU::esprintf("external bidirectional role incompatible "
			     "with port %s of worker %s",
			     other.name(), other.m_worker->m_implName);
      }
    } else if (role->m_provider) {
      if (other.m_isProducer) {
	  err = OU::esprintf("external consumer role incompatible "
			     "with port %s of worker %s",
			     other.name(), other.m_worker->m_implName);
      }
      m_isBidirectional = false;
    }
  }
}

Port &DataPort::
clone(Worker &, std::string &, size_t, OCPI::Util::Assembly::Role *, const char *&) const {
  assert("Can't clone generic data port" == 0);
}

// This is basically a callback from the low level OU::Port parser
// It needs the protocol to be parsed at this point.  From tools we allow file includes etc.
// Thus it is entirely replacing the protocol parsing in OU::port
const char *DataPort::
parseProtocol() {
  ezxml_t pSum;
  std::string protFile;
  const char *err;
  if ((err = tryOneChildInclude(m_xml, m_worker->m_file, "ProtocolSummary", &pSum, protFile,
				true)))
    return err;
  const char *protocolAttr = ezxml_cattr(m_xml, "protocol");
  if (pSum) {
    if (protocolAttr || ezxml_cchild(m_xml, "Protocol"))
      return "cannot have both Protocol and ProtocolSummary";
    if ((err = OE::checkAttrs(pSum, "DataValueWidth", "DataValueGranularity",
			      "DiverDataSizes", "MaxMessageValues", "NumberOfOpcodes",
			      "VariableMessageLength", "ZeroLengthMessages",
			      "MinMessageValues",  (void*)0)) ||
	(err = OE::getNumber(pSum, "NumberOfOpcodes", &m_nOpcodes, NULL, 0, false)) ||
	(err = OU::Protocol::parseSummary(pSum)))
      return err;
    m_seenSummary = true;
  } else {
    ezxml_t protx = NULL;
    if ((err = tryOneChildInclude(m_xml, m_worker->m_file, "Protocol", &protx, protFile, true)))
      return err;
    if (protocolAttr) {
      if (protx)
	return "can't have both 'protocol' element (maybe xi:included) and 'protocol' attribute";
      if ((err = parseFile(protocolAttr, m_worker->m_file.c_str(), "protocol", &protx, protFile,
			   false)))
	return err;
    }
    // The protx comes from an include, a child element, or the protocol attr file
    if (protx) {
      if (protFile.length() && protFile != m_worker->m_file) {
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
	std::string name(start, last - start);
	err = OU::Protocol::parse(protx, name.c_str(), protFile.c_str(), doProtocolChild, this);
      } else
	err = OU::Protocol::parse(protx, NULL, m_worker->m_file.c_str(), doProtocolChild, this);
      if (err)
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
    if ((err = parseSummary(m_xml)))
      return err;
  }
  return NULL;
}

// Second pass parsing of spec data ports when they might refer to each other.
// FIXME: consider whether this should be done for impls also...
const char *DataPort::
parse() {
  return OU::Port::parse();
}

// After the specific port types have parsed everything
const char *DataPort::
finalize() {
  // If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
  size_t granuleWidth =
    m_dataValueWidth * m_dataValueGranularity;
  // If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
  if (granuleWidth >= m_dataWidth &&
      (m_dataWidth == 0 || (granuleWidth % m_dataWidth) == 0) && !m_zeroLengthMessages)
    m_byteWidth = m_dataWidth;
  else
    m_byteWidth = m_dataValueWidth;
  if (m_byteWidth != 0 && m_dataWidth % m_byteWidth)
    return "Specified ByteWidth does not divide evenly into specified DataWidth";
  // Check if this port requires endianness
  // Either the granule is smaller than or not a multiple of data path width
  if (granuleWidth < m_dataWidth || m_dataWidth && granuleWidth % m_dataWidth)
    m_worker->m_needsEndian = true;
  return NULL;
}

void DataPort::
emitPortDescription(FILE *f, Language lang) const {
  OcpPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, " %s  This interface is a data interface acting as %s\n",
	  comment, m_isProducer ? "producer" : "consumer");
  fprintf(f, "  %s   Protocol: \"%s\"\n", comment, OU::Protocol::name().c_str());
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
emitRecordDataTypes(FILE *f) {
  if (m_nOpcodes > 1) {
    if (operations()) {
      // See if this protocol has already been defined
      unsigned nn;
      for (nn = 0; nn < m_ordinal; nn++)
	if (m_worker->m_ports[nn]->isData()) {
	  DataPort *dp = static_cast<DataPort*>(m_worker->m_ports[nn]);

	  if (dp->operations() &&
	      !strcasecmp(dp->OU::Protocol::m_name.c_str(), OU::Protocol::m_name.c_str()))
	    break;
	}
      if (nn >= m_ordinal) {
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
    } else {
      fprintf(f, "  subtype %s_OpCode_t is std_logic_vector(%zu downto 0); -- for %zu opcodes\n",
	      name(), ceilLog2(m_nOpcodes) - 1, m_nOpcodes);
    }
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
	  last.c_str(), name(), name(), name(), name());
  if (masterIn()) {
    if (m_dataWidth)
      fprintf(f,
	      "    %s_in.data => %s_data,\n",
	      name(), name());
    if (ocp.MByteEn.value)
      fprintf(f, "    %s_in.byte_enable => %s_byte_enable,\n", name(), name());
    if (m_nOpcodes > 1)
      fprintf(f, "    %s_in.opcode => %s_opcode,\n", name(), name());
    fprintf(f,
	    "    %s_in.som => %s_som,\n"
	    "    %s_in.eom => %s_eom,\n",
	    name(), name(), name(), name());
    if (m_dataWidth)
      fprintf(f,
	      "    %s_in.valid => %s_valid,\n",
	      name(), name());
    if (m_isPartitioned)
      fprintf(f,
	      "    %s_in.part_size   => %s_part_size,\n"
	      "    %s_in.part_offset => %s_part_offset,\n"
	      "    %s_in.part_start  => %s_part_start,\n"
	      "    %s_in.part_ready  => %s_part_ready,\n",
	      name(), name(), name(), name(), name(),
	      name(), name(), name());
    fprintf(f,
	    "    %s_out.take => %s_take",
	    name(), name());
    if (m_isPartitioned)
      fprintf(f,
	      ",\n"
	      "    %s_out.part_take  => %s_part_take",
	      name(), name());
    last = ",\n";
  } else {
    if (m_isPartitioned)
      fprintf(f,
	      "    %s_in.part_ready   => %s_part_ready,\n", name(), name());
    fprintf(f,
	    "    %s_out.give => %s_give,\n",
	    name(), name());
    if (m_dataWidth)
      fprintf(f,
	      "    %s_out.data => %s_data,\n", name(), name());
    if (ocp.MByteEn.value)
      fprintf(f, "    %s_out.byte_enable => %s_byte_enable,\n", name(), name());
    if (ocp.MReqInfo.value)
      fprintf(f, "    %s_out.opcode => %s_opcode,\n", name(), name());
    fprintf(f,
	    "    %s_out.som => %s_som,\n"
	    "    %s_out.eom => %s_eom,\n",
	    name(), name(), name(), name());
    if (m_dataWidth)
      fprintf(f,
	      "    %s_out.valid => %s_valid",
	      name(), name());
    if (m_isPartitioned)
      fprintf(f,
	      ",\n"
	      "    %s_out.part_size   => %s_part_size,\n"
	      "    %s_out.part_offset => %s_part_offset,\n"
	      "    %s_out.part_start  => %s_part_start,\n"
	      "    %s_out.part_give   => %s_part_give",
	      name(), name(), name(),
	      name(), name(), name(), name(), name());
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
	  name(), masterIn() ? "take" : "give", name(), name());
  if (m_dataWidth)
    fprintf(f,
	    "  signal %s_data  : std_logic_vector(%zu downto 0);\n",
	    name(),
	    m_dataWidth-1);
  if (ocp.MByteEn.value)
    fprintf(f, "  signal %s_byte_enable: std_logic_vector(%zu downto 0);\n",
	    name(), m_dataWidth / m_byteWidth - 1);		    
  if (m_preciseBurst)
    fprintf(f, "  signal %s_burst_length: std_logic_vector(%zu downto 0);\n",
	    name(), ocp.MBurstLength.width - 1);
  if (m_nOpcodes > 1) {
    fprintf(f,
	    "  -- The strongly typed enumeration signal for the port\n"
	    "  signal %s_opcode      : %s_OpCode_t;\n"
	    "  -- The weakly typed temporary signals\n"
	    "  signal %s_opcode_temp : std_logic_vector(%zu downto 0);\n"
	    "  signal %s_opcode_pos  : integer;\n",
	    name(), operations() ?
	    OU::Protocol::m_name.c_str() : name(), name(), ocp.MReqInfo.width - 1, name());
  }
  fprintf(f,
	  "  signal %s_som   : Bool_t;    -- valid eom\n"
	  "  signal %s_eom   : Bool_t;    -- valid som\n",
	  name(), name());
  if (m_dataWidth)
    fprintf(f,
	    "  signal %s_valid : Bool_t;   -- valid data\n", name());
  if (m_isPartitioned)
    fprintf(f,
	    "  signal %s_part_size        : UShort_t;\n"
	    "  signal %s_part_offset      : UShort_t;\n"
	    "  signal %s_part_start       : Bool_t;\n"
	    "  signal %s_part_ready       : Bool_t;\n"
	    "  signal %s_part_%s        : Bool_t;\n",
	    name(), name(), name(), name(), name(), masterIn() ? "take" : "give");
}

void DataPort::
emitXML(std::string &out) {
  OU::Port::emitXml(out);
}

// static method
const char *DataPort::
adjustConnection(const char *masterName,
		 Port &prodPort, OcpAdapt *prodAdapt,
		 Port &consPort, OcpAdapt *consAdapt,
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
    if (prod.OU::Protocol::name().size() && cons.OU::Protocol::name().size() &&
	prod.OU::Protocol::name() != cons.OU::Protocol::name())
      return OU::esprintf("protocol incompatibility: producer: %s vs. consumer: %s",
			  prod.OU::Protocol::name().c_str(), cons.OU::Protocol::name().c_str());
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
	     name(), 
	     m_worker->m_defaultDataWidth >= 0 ?
	     m_worker->m_defaultDataWidth : m_dataValueWidth);
    ezxml_t wsix = ezxml_parse_str(wsi, strlen(wsi));
    Port *p = createPort<WsiPort>(*m_worker, wsix, this, -1, err);
    if (!err)
      err = p->checkClock();
  }
  return err;
}
    

const char *DataPort::
adjustConnection(Port &, const char *, Language, OcpAdapt *, OcpAdapt *) {
  assert("Cannot adjust a generic data connection" == 0);
}

void DataPort::
emitOpcodes(FILE *f, const char *pName, Language lang) {
  if (nOperations()) {
    OU::Operation *op = operations();
    fprintf(f,
	    "  %s Opcode/operation value declarations for protocol \"%s\" on interface \"%s\"\n",
	    hdlComment(lang), OU::Protocol::m_name.c_str(), name());
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
			name(), m_worker->m_implName);
  } else {
    role.m_provider = !m_isProducer;
    role.m_bidirectional = m_isBidirectional;
    role.m_knownRole = true;
  }
  return NULL;
}

void DataPort::
initRole(OCPI::Util::Assembly::Role &role) {
  role.m_knownRole = true;
  if (m_isBidirectional)
    role.m_bidirectional = true;
  else
    role.m_provider = !m_isProducer;
}

#if 0
#endif
