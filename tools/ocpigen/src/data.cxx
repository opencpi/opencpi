// Support for data ports

#include <assert.h>
#include "OcpiUtilMisc.h"
#include "wip.h"
#include "hdl.h"

const char *DataPort::
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
      return OU::esprintf("The \"hashfield\" attribute is only allowed with hashed distribution");
    if (!m_protocol)
      return OU::esprintf("The \"hashfield\" attribute cannot be used with there is no protocol");
    OU::Operation *o = m_protocol->m_operations;
    bool found = false;
    for (unsigned n = 0; n < m_protocol->m_nOperations; n++)
      if (o->findArg(hash.c_str()))
	found = true;
    if (!found)
      return OU::esprintf("The \"hashfield\" attribute \"%s\" doesn't match any field "
			  "in the any operation", hash.c_str());
  }
  return err;
}

// Constructor when creating a derived impl port from a spec port
// Based on an existing spec port (sp).
DataPort::
DataPort(Worker &w, ezxml_t x, Port *sp, int ordinal, WIPType type, const char *&err)
  : OcpPort(w, x, sp, ordinal, type, NULL, err),
    m_protocol(NULL), m_isProducer(false), m_isOptional(false), m_isBidirectional(false),
    m_nOpcodes(0), m_minBufferCount(0), m_bufferSize(0), m_bufferSizePort(NULL),
    m_isScalable(false), m_defaultDistribution(All), m_isPartitioned(false) {
  if (err)
    return;
  if (sp) {
    DataPort *dp = static_cast<DataPort*>(sp);
    m_protocol = dp->m_protocol;
    dp->m_protocol = NULL;
    m_isProducer      = dp->m_isProducer;
    m_isOptional      = dp->m_isOptional;
    m_nOpcodes        = dp->m_nOpcodes;
    // The rest are really impl-only
    m_isBidirectional = dp->m_isBidirectional;
    m_minBufferCount  = dp->m_minBufferCount; 
    m_bufferSize      = dp->m_bufferSize;
    m_bufferSizePort  = dp->m_bufferSizePort;
  } else {
    // FIXME: not really, this stuff is just better in spcm2 branch so don't merge this
    assert(!ezxml_cattr(x, "protocol") && !ezxml_child(x, "protocol") &&
	   !ezxml_child(x, "protocolsummary"));
    if ((err = OE::getBoolean(m_xml, "Producer", &m_isProducer)))
      return;
    m_protocol = new Protocol(*this);
    m_protocol->m_diverseDataSizes = true;
    m_protocol->m_variableMessageLength = true;
    m_protocol->m_maxMessageValues = 64*1024;
    m_protocol->m_zeroLengthMessages = true;
    m_protocol->m_isUnbounded = true;
    m_nOpcodes = 256;
  }
  // Scalability all defaults since it is not a spec issue.  Initialize from protocol
  m_opScaling.resize(m_protocol->m_nOperations, NULL);
  // Now we do implementation-specific initialization that will precede the
  // initializations for specific port types (WSI, etc.)
  if (// Adding optionality in the impl xml is only relevant to devices.
      (err = OE::getBoolean(m_xml, "Optional", &m_isOptional, true)) ||
      // Be careful not to clobber protocol-determined values (i.e. don't set default values)
      (err = OE::getNumber(m_xml, "NumberOfOpcodes", &m_nOpcodes, NULL, 0, false)) ||
      (err = OE::getNumber(m_xml, "MaxMessageValues", &m_protocol->m_maxMessageValues, NULL, 0,
			   false)) ||
      (err = OE::getNumber(m_xml, "DataValueWidth", &m_protocol->m_dataValueWidth, NULL, 0,
			   false)) ||
      (err = OE::getNumber(m_xml, "DataValueGranularity", &m_protocol->m_dataValueGranularity, NULL, 0,
			   false)) ||
      (err = OE::getBoolean(m_xml, "ZeroLengthMessages", &m_protocol->m_zeroLengthMessages,
			    true)) ||
      (err = OE::getNumber(m_xml, "MinBuffers", &m_minBufferCount, 0, 0)) || // backward compat
      (err = OE::getNumber(m_xml, "MinBufferCount", &m_minBufferCount, 0, m_minBufferCount)) ||
      (err = OE::getNumber(m_xml, "Buffersize", &m_bufferSize, 0,
			   m_protocol ? m_protocol->m_defaultBufferSize : 0)))
    return;
  // Data width can be unspecified, specified explicitly, or specified with an expression
  if (!m_dataWidthFound) {
    if (w.m_defaultDataWidth >= 0)
      m_dataWidth = (unsigned)w.m_defaultDataWidth;
    else
      m_dataWidth = m_protocol->m_dataValueWidth;  // or granularity?
    if (!m_bwFound)
      m_byteWidth = m_dataWidth;
  } else if (!m_protocol->m_dataValueWidth && !m_protocol->nOperations())
    m_protocol->m_dataValueWidth = m_dataWidth;
  if (m_dataWidth >= m_protocol->m_dataValueWidth) {
    if (m_dataWidth % m_protocol->m_dataValueWidth) {
      err = OU::esprintf("DataWidth (%zu) on port '%s' not a multiple of DataValueWidth (%zu)",
			 m_dataWidth, name(), m_protocol->m_dataValueWidth);
      return;
    }
  } else if (m_protocol->m_dataValueWidth % m_dataWidth) {
    err =  OU::esprintf("DataValueWidth (%zu) on port '%s' not a multiple of DataWidth (%zu)",
			m_protocol->m_dataValueWidth, name(), m_dataWidth);
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
  if (!m_protocol->m_defaultBufferSize && m_protocol->m_maxMessageValues)
    m_protocol->m_defaultBufferSize =
      (m_protocol->m_maxMessageValues * m_protocol->m_dataValueWidth + 7) / 8;
  // Scalability
  if (OE::getOptionalString(m_xml, m_scaleExpr, "scale")) {
    // only for assembly scaling
  }
  // Here we parse defaults for operations and arguments.
  if ((err = parseDistribution(m_xml, m_defaultDistribution, m_defaultHashField)) ||
      (err = m_defaultPartitioning.parse(w, m_xml)))
    return;
  // Now we parse the child elements for operations.
  for (ezxml_t ox = ezxml_cchild(x, "operation"); ox; ox = ezxml_next(ox)) {
    std::string oName;
    if ((err = OE::checkAttrs(ox, "name", DISTRIBUTION_ATTRS, PARTITION_ATTRS, (void*)0)) ||
	(err = OE::checkElements(ox, "argument", (void*)0)) ||
	(err = OE::getRequiredString(ox, oName, "name")))
      return;
    OU::Operation *op = m_protocol ? m_protocol->findOperation(oName.c_str()) : NULL;
    if (!op) {
      err = OU::esprintf("Here is no operation named \"%s\" in the protocol", oName.c_str());
      return;
    }
    size_t ord = op - m_protocol->m_operations;
    if (m_opScaling[ord]) {
      err = OU::esprintf("Duplicate operation element with name \"%s\" for port \"%s\"",
			 oName.c_str(), sp->m_name.c_str());
      return;
    }
    OpScaling *os = new OpScaling(op->m_nArgs);
    if ((err = os->parse(*this, *op, ox)))
      return;
    m_opScaling[ord] = os;
  }
  OU::Operation *op = m_protocol->m_operations;
  for (unsigned o = 0; o < m_protocol->m_nOperations; o++, op++) {
    OpScaling *os = m_opScaling[o];
    OU::Member *arg = op->m_args;
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
}

// Constructor when this is the concrete derived class, when parsing spec ports
// Note the "parse" method is called after all spec ports are created - second pass
DataPort::
DataPort(Worker &w, ezxml_t x, int ordinal, const char *&err)
  : OcpPort(w, x, NULL, ordinal, WDIPort, NULL, err),
    m_protocol(NULL), m_isProducer(false), m_isOptional(false), m_isBidirectional(false),
    m_nOpcodes(0), m_minBufferCount(0), m_bufferSize(0), m_bufferSizePort(NULL),
    m_isScalable(false), m_defaultDistribution(All), m_isPartitioned(false) {
  if (err)
    return;
  // Spec initialization
  if (type == WDIPort &&
      ((err = OE::checkAttrs(m_xml, SPEC_DATA_PORT_ATTRS, (void*)0)) ||
       (err = OE::getBoolean(m_xml, "Producer", &m_isProducer)) ||
       (err = OE::getBoolean(m_xml, "Optional", &m_isOptional))))
    return;
  const char *protocolAttr = ezxml_cattr(m_xml, "protocol");
  ezxml_t pSum;
  std::string protFile;
  if ((err = tryOneChildInclude(m_xml, m_worker->m_file, "ProtocolSummary", &pSum, protFile,
				true)))
    return;
  m_protocol = new Protocol(*this);
  if (pSum) {
    if (protocolAttr || ezxml_cchild(m_xml, "Protocol")) {
      err = "cannot have both Protocol and ProtocolSummary";
      return;
    }
    if ((err = OE::checkAttrs(pSum, OCPI_PROTOCOL_SUMMARY_ATTRS, "NumberOfOpcodes",
			      (void*)0)) ||
	(err = OE::getNumber(pSum, "NumberOfOpcodes", &m_nOpcodes, 0, 1)) ||
	(err = m_protocol->parseSummary(pSum)))
      return;
  } else {
    ezxml_t protx = NULL;
    // FIXME: default protocol name from file name
    if ((err = tryOneChildInclude(m_xml, m_worker->m_file, "Protocol", &protx, protFile, true)))
      return;
    if (protocolAttr) {
      if (protx) {
	err = "can't have both 'protocol' element (maybe xi:included) and 'protocol' attribute";
	return;
      }
      if ((err = parseFile(protocolAttr, m_worker->m_file.c_str(), "protocol", &protx,
			   protFile, false)))
	return;
    }
    if (protx) {
      if ((err = m_protocol->parse(protFile.c_str(), protx)))
	return;
      // So if there is a protocol, nOpcodes is initialized from it.
      m_nOpcodes = m_protocol->nOperations();
    } else {
      // When there is no protocol, we force it to variable, unbounded at 64k, diverse, zlm
      // I.e. assume it can do anything up to 64KB
      // But with no operations, we can scale back when connecting to something more specific
      m_protocol->m_diverseDataSizes = true;
      m_protocol->m_variableMessageLength = true;
      m_protocol->m_maxMessageValues = 64*1024;
      m_protocol->m_zeroLengthMessages = true;
      m_protocol->m_isUnbounded = true;
      m_nOpcodes = 256;
    }
  }
}

// Our special clone copy constructor
DataPort::
DataPort(const DataPort &other, Worker &w , std::string &name, size_t count,
	OCPI::Util::Assembly::Role *role, const char *&err)
  : OcpPort(other, w, name, count, err) {
  if (err)
    return;
  m_protocol = other.m_protocol; // check on destruction
  m_isProducer = other.m_isProducer;
  m_isOptional = other.m_isOptional;
  m_isBidirectional = other.m_isBidirectional;
  m_nOpcodes = other.m_nOpcodes;
  m_minBufferCount = other.m_minBufferCount;
  m_bufferSize = other.m_bufferSize;
  m_bufferSizePort = other.m_bufferSizePort;
  m_isScalable = other.m_isScalable;
  m_defaultDistribution = other.m_defaultDistribution;
  m_opScaling = other.m_opScaling;
  m_isPartitioned = other.m_isPartitioned;
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

// Second pass parsing of spec data ports when they might refer to each other.
// FIXME: consider whether this should be done for impls also...
const char *DataPort::
parse() {
  const char *err;
  const char *bs = ezxml_cattr(m_xml, "bufferSize");
  if (bs && !isdigit(bs[0])) {
    Port *other;
    if ((err = m_worker->getPort(bs, other, this)))
      return err;
    m_bufferSizePort = other;
  } else if ((err = OE::getNumber(m_xml, "bufferSize", &m_bufferSize, 0, 0)))
    return err;
  // FIXME: outlaw buffer size port when you have protocol?
  if (m_bufferSize == 0)
    m_bufferSize = m_protocol->m_defaultBufferSize; // from protocol
  return NULL;
}

// After the specific port types have parsed everything
// and also *AGAIN* after all expressions are resolved when instanced in an assembly
const char *DataPort::
finalize() {
  if (!m_protocol->m_dataValueWidth && !m_protocol->nOperations())
    m_protocol->m_dataValueWidth = m_dataWidth;
  if (m_dataWidth >= m_protocol->m_dataValueWidth) {
    if (m_dataWidth % m_protocol->m_dataValueWidth)
      return OU::esprintf("DataWidth (%zu) on port '%s' not a multiple of DataValueWidth (%zu)",
			  m_dataWidth, name(), m_protocol->m_dataValueWidth);
  } else if (m_protocol->m_dataValueWidth % m_dataWidth)
    return OU::esprintf("DataValueWidth (%zu) on port '%s' not a multiple of DataWidth (%zu)",
			m_protocol->m_dataValueWidth, name(), m_dataWidth);
  // If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
  size_t granuleWidth =
    m_protocol->m_dataValueWidth * m_protocol->m_dataValueGranularity;
  // If messages are always a multiple of datawidth, bytes are datawidth
  if (granuleWidth >= m_dataWidth &&
      (m_dataWidth == 0 || (granuleWidth % m_dataWidth) == 0))
    m_byteWidth = m_dataWidth;
  else
    m_byteWidth = m_protocol->m_dataValueWidth;
  if (m_byteWidth != 0 && m_dataWidth % m_byteWidth)
    return "Specified ByteWidth does not divide evenly into specified DataWidth";
  // Check if this port requires endianness
  // Either the granule is smaller than or not a multiple of data path width
  if (granuleWidth < m_dataWidth || (m_dataWidth && granuleWidth % m_dataWidth))
    m_worker->m_needsEndian = true;
  return NULL;
}

void DataPort::
emitPortDescription(FILE *f, Language lang) const {
  OcpPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, " %s  This interface is a data interface acting as %s\n",
	  comment, m_isProducer ? "producer" : "consumer");
  fprintf(f, "  %s   Protocol: \"%s\"\n", comment, m_protocol->name().c_str());
  fprintf(f, "  %s   DataValueWidth: %zu\n", comment, m_protocol->m_dataValueWidth);
  fprintf(f, "  %s   DataValueGranularity: %zu\n", comment, m_protocol->m_dataValueGranularity);
  fprintf(f, "  %s   DiverseDataSizes: %s\n", comment, BOOL(m_protocol->m_diverseDataSizes));
  fprintf(f, "  %s   MaxMessageValues: %zu\n", comment, m_protocol->m_maxMessageValues);
  fprintf(f, "  %s   NumberOfOpcodes: %zu\n", comment, m_nOpcodes);
  fprintf(f, "  %s   Producer: %s\n", comment, BOOL(m_isProducer));
  fprintf(f, "  %s   VariableMessageLength: %s\n", comment, BOOL(m_protocol->m_variableMessageLength));
  fprintf(f, "  %s   ZeroLengthMessages: %s\n", comment, BOOL(m_protocol->m_zeroLengthMessages));
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
	  name(), name(), name(), name(), name());
#endif
}
void DataPort::
emitInterfaceConstants(FILE *f, Language lang) {
  if (lang == Verilog)
    emitVerilogPortParameters(f);
}
#if 1
void DataPort::
emitRecordInterface(FILE *f, const char *implName) {

  std::string width = m_dataWidthExpr;
  if (m_dataWidthExpr.empty())
    OU::format(width, "%zu", m_dataWidth);
  else
    OU::format(width, "to_integer(%s)", m_dataWidthExpr.c_str());
  fprintf(f, "  constant ocpi_port_%s_data_width : natural := %s;\n", name(), width.c_str());
  vectorWidth(&ocpSignals[OCP_MData], width, VHDL, false, true);
  fprintf(f, "  constant ocpi_port_%s_MData_width : natural := %s;\n", name(), width.c_str());
  vectorWidth(&ocpSignals[OCP_MByteEn], width, VHDL, false, true);
  fprintf(f, "  constant ocpi_port_%s_MByteEn_width : natural := %s;\n", name(), width.c_str());
  vectorWidth(&ocpSignals[OCP_MDataInfo], width, VHDL, false, true);
  std::string extra;
  size_t n = extraDataInfo();
  if (n)
    OU::format(extra, "(%s)+%zu", width.c_str(), n);
  else
    extra = width;
  fprintf(f, "  constant ocpi_port_%s_MDataInfo_width : natural := %s;\n",
	  name(), extra.c_str());
  OcpPort::emitRecordInterface(f, implName);
}
#endif

void DataPort::
emitRecordDataTypes(FILE *f) {
  if (m_nOpcodes > 1) {
    Protocol *prot = m_protocol;
    if (prot && prot->operations()) {
      // See if this protocol has already been defined
      size_t maxOpcodes = 0;
      unsigned first = 0;
      for (unsigned nn = 0; nn < m_worker->m_ports.size(); nn++)
	if (m_worker->m_ports[nn]->isData()) {
	  DataPort *dp = static_cast<DataPort*>(m_worker->m_ports[nn]);
	  if (dp->m_protocol && dp->m_protocol->operations() &&
	      !strcasecmp(dp->m_protocol->m_name.c_str(), m_protocol->m_name.c_str())) {
	    maxOpcodes = std::max(dp->m_nOpcodes, maxOpcodes);
	    if (!first)
	      first = nn;
	  }
	}
      if (first >= m_ordinal) {
	fprintf(f,
		"  -- This enumeration is for the opcodes for protocol %s (%s)\n"
		"  type %s_OpCode_t is (\n",
		prot->m_name.c_str(), prot->m_qualifiedName.c_str(),
		prot->m_name.c_str());
	OU::Operation *op = prot->operations();
	unsigned nn;
	for (nn = 0; nn < prot->nOperations(); nn++, op++)
	  fprintf(f, "%s    %s_%s_op_e", nn ? ",\n" : "",
		  prot->m_name.c_str(), op->name().c_str());
	// If the protocol opcodes do not fill the space, fill it
	if (nn < maxOpcodes)
	  for (unsigned o = 0; nn < m_nOpcodes; nn++, o++)
	    fprintf(f, ",%sop%u_e", (o % 10) == 0 ? "\n    " : " ", nn);
	fprintf(f, ");\n");
      }
    } else {
      fprintf(f, "  subtype %s_OpCode_t is std_logic_vector(%zu downto 0); -- for %zu opcodes\n",
	      name(), OU::ceilLog2(m_nOpcodes) - 1, m_nOpcodes);
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
	    "  signal %s_data  : std_logic_vector(ocpi_port_%s_data_width-1 downto 0);\n",
	    name(), name());
  if (ocp.MByteEn.value)
    fprintf(f, "  signal %s_byte_enable: std_logic_vector(ocpi_port_%s_MByteEn_width-1 downto 0);\n",
	    name(), name());
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
	    name(), m_protocol && m_protocol->operations() ?
	    m_protocol->m_name.c_str() : name(), name(), ocp.MReqInfo.width - 1, name());
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
emitXML(FILE *f) {
  fprintf(f, "  <port name=\"%s\" numberOfOpcodes=\"%zu\"", name(), m_nOpcodes);
  if (m_isBidirectional)
    fprintf(f, " bidirectional='1'");
  else if (!m_isProducer)
    fprintf(f, " provider='1'");
  if (m_minBufferCount)
    fprintf(f, " minBufferCount=\"%zu\"", m_minBufferCount);
  if (m_bufferSizePort)
    fprintf(f, " buffersize='%s'", m_bufferSizePort->name());
  else if (m_bufferSize)
    fprintf(f, " bufferSize='%zu'", m_bufferSize);
  if (m_isOptional)
    fprintf(f, " optional=\"%u\"", m_isOptional);
  fprintf(f, ">\n");
  m_protocol->printXML(f, 2);
  fprintf(f, "  </port>\n");
}

// static method
const char *DataPort::
adjustConnection(const char *masterName,
		 Port &prodPort, OcpAdapt *prodAdapt, bool &prodHasExpr,
		 Port &consPort, OcpAdapt *consAdapt, bool &consHasExpr,
		 Language lang, size_t &unused) {
  assert(prodPort.isData() && consPort.isData());
  DataPort &prod = *static_cast<DataPort*>(&prodPort);
  DataPort &cons = *static_cast<DataPort*>(&consPort);
  // Check WDI compatibility
  // If both sides have protocol, check them for compatibility
  if (prod.m_protocol->nOperations() && cons.m_protocol->nOperations()) {
    if (prod.m_protocol->m_dataValueWidth != cons.m_protocol->m_dataValueWidth)
      return "dataValueWidth incompatibility for connection";
    if (prod.m_protocol->m_dataValueGranularity < cons.m_protocol->m_dataValueGranularity ||
	prod.m_protocol->m_dataValueGranularity % cons.m_protocol->m_dataValueGranularity)
      return "dataValueGranularity incompatibility for connection";
    if (prod.m_protocol->m_maxMessageValues > cons.m_protocol->m_maxMessageValues)
      return "maxMessageValues incompatibility for connection";
    if (prod.m_protocol->name().size() && cons.m_protocol->name().size() &&
	prod.m_protocol->name() != cons.m_protocol->name())
      return OU::esprintf("protocol incompatibility: producer: %s vs. consumer: %s",
			  prod.m_protocol->name().c_str(), cons.m_protocol->name().c_str());
    if (prod.m_protocol->nOperations() && cons.m_protocol->nOperations() && 
	prod.m_protocol->nOperations() != cons.m_protocol->nOperations())
      return "numberOfOpcodes incompatibility for connection";
    //  if (prod.u.wdi.nOpcodes > cons.u.wdi.nOpcodes)
    //    return "numberOfOpcodes incompatibility for connection";
    if (prod.m_protocol->m_variableMessageLength && !cons.m_protocol->m_variableMessageLength)
      return "variable length producer vs. fixed length consumer incompatibility";
    if (prod.m_protocol->m_zeroLengthMessages && !cons.m_protocol->m_zeroLengthMessages)
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
  const char *err = prod.adjustConnection(cons, masterName, lang, prodAdapt, consAdapt, unused);
  if (err)
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
  if (type == WDIPort) {
    // Do it via XML so we aren't duplicating other code
    char *wsi;
    ocpiCheck(asprintf(&wsi,
		       "<streaminterface name='%s' dataWidth='%zu' impreciseburst='true'/>",
		       name(), 
		       m_worker->m_defaultDataWidth >= 0 ?
		       m_worker->m_defaultDataWidth : m_protocol->m_dataValueWidth) > 0);
    ezxml_t wsix = ezxml_parse_str(wsi, strlen(wsi));
    Port *p = createPort<WsiPort>(*m_worker, wsix, this, -1, err);
    if (!err)
      err = p->checkClock();
  }
  return err;
}

const char *DataPort::
adjustConnection(Port &, const char *, Language, OcpAdapt *, OcpAdapt *, size_t &) {
  assert("Cannot adjust a generic data connection" == 0);
}

void DataPort::
emitOpcodes(FILE *f, const char *pName, Language lang) {
  Protocol *prot = m_protocol;
  if (prot && prot->nOperations()) {
    OU::Operation *op = prot->operations();
    fprintf(f,
	    "  %s Opcode/operation value declarations for protocol \"%s\" on interface \"%s\"\n",
	    hdlComment(lang), prot->m_name.c_str(), name());
    for (unsigned n = 0; n < prot->nOperations(); n++, op++)
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
	  name(), width.c_str(), name(), name(), name(), m_byteWidth, m_byteWidth, name(),
	  m_byteWidth, name());
  if (ocp.MByteEn.value)
    fprintf(f,
	    "  localparam ocpi_port_%s_MByteEn_width = ocpi_port_%s_data_width / %zu;\n",
	    name(), name(), m_byteWidth);
  if (ocp.MDataInfo.value)
    fprintf(f,
	    "  localparam ocpi_port_%s_MDataInfo_width = \n"
	    "    ocpi_port_%s_data_width != %zu && %zu != 8 ?\n"
	    "    ocpi_port_%s_data_width - ((8 * ocpi_port_%s_data_width) / %zu) :\n"
	    "    ocpi_port_%s_data_width;\n",
	    name(), name(), m_byteWidth, m_byteWidth, name(), name(), m_byteWidth, name());
}

Overlap::
Overlap()
  : m_left(0), m_right(0), m_padding(None) {
}

const char *Overlap::
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

Partitioning::Partitioning()
  : m_sourceDimension(0) {
}

const char *Partitioning::
parse(Worker &w, ezxml_t x) {
  const char *err = m_scaling.parse(w, x);
  if (!err && !(err = OE::getNumber(x, "source", &m_sourceDimension)))
    err = m_overlap.parse(x);
  return err;
}

DataPort::OpScaling::
OpScaling(size_t nArgs)
  : m_distribution(All), m_hashField(NULL), m_multiple(false), m_allSeeOne(false),
    m_allSeeEnd(false), m_isPartitioned(false) {
  m_partitioning.resize(nArgs);
  for (size_t n = 0; n < nArgs; n++)
    m_partitioning[n] = NULL;
}
const char *DataPort::OpScaling::
parse(DataPort &dp, OU::Operation &op, ezxml_t x) {
  Worker &w = *dp.m_worker;
  const char *err;
  m_defaultPartitioning = dp.m_defaultPartitioning;
  m_distribution = dp.m_defaultDistribution;
  std::string hash;
  if ((err = dp.parseDistribution(x, m_distribution, hash)) ||
      (err = m_defaultPartitioning.parse(w, x)) ||
      (err = OE::getBoolean(x, "multiple", &m_multiple)) ||
      (err = OE::getBoolean(x, "allSeeOne", &m_allSeeOne)) ||
      (err = OE::getBoolean(x, "allSeeEnd", &m_allSeeEnd)))
    return err;
  if (hash.empty() && m_distribution == Hashed)
    hash = dp.m_defaultHashField;
  if (hash.length() && !(m_hashField = op.findArg(hash.c_str())))
    return OU::esprintf("hashfield attribute value \"%s\" not an argument to \"%s\"",
			hash.c_str(), op.m_name.c_str());
  m_partitioning.resize(op.m_nArgs, 0);
  for (ezxml_t ax = ezxml_cchild(x, "argument"); ax; ax = ezxml_next(ax)) {
    std::string aName;
    if ((err = OE::checkAttrs(ax, "name", PARTITION_ATTRS, (void*)0)) ||
	(err = OE::checkElements(ax, "dimension", (void*)0)) ||
	(err = OE::getRequiredString(ax, aName, "name")))
      return err;
    OU::Member *a = op.findArg(aName.c_str());
    if (!a)
      return OU::esprintf("name attribute of argument element is not an argument to \"%s\"",
			  op.m_name.c_str());
    size_t nDims = a->m_isSequence ? 1 : a->m_arrayRank;
    Partitioning *p = new Partitioning[nDims];
    m_partitioning[a - op.m_args] = p;
    // We have an array of partitionings for all the dimensions (sequences are 1D).
    // We start with a default based on what is in the argument element.
    Partitioning def = m_defaultPartitioning;
    if ((err = def.parse(w, ax)))
      return err;
    unsigned n;
    for (n = 0; n < nDims; n++)
      p[n] = def;
    n = 0;
    for (ezxml_t dx = ezxml_cchild(ax, "dimension"); dx; dx = ezxml_next(dx), n++, p++) {
      if (n >= nDims)
	return OU::esprintf("Too many dimensions for argument \"%s\" in operation \"%s\"",
			    a->m_name.c_str(), op.m_name.c_str());
      if ((err = OE::checkAttrs(dx, PARTITION_ATTRS, (void*)0)) ||
	  (err = p->parse(w, dx)))
	return err;
    }
  }
  return NULL;
}
