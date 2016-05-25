#include "hdl.h"
#include "ocp.h"

WsiPort::
WsiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : DataPort(w, x, sp, ordinal, WSIPort, err) {
  if (err)
    return;
  if ((err = OE::checkAttrs(x, "Name", "Clock", "DataWidth", "PreciseBurst",
			    "ImpreciseBurst", "Continuous", "Abortable",
			    "EarlyRequest", "MyClock", "RegRequest", "Pattern",
			    "NumberOfOpcodes", "MaxMessageValues",
			    "datavaluewidth", "zerolengthmessages",
			    "datavaluegranularity", "implname", "producer", "optional",
			    DISTRIBUTION_ATTRS, PARTITION_ATTRS,
			    (void*)0)) ||
      (err = OE::getBoolean(x, "Abortable", &m_abortable)) ||
      (err = OE::getBoolean(x, "RegRequest", &m_regRequest)) ||
      (err = OE::getBoolean(x, "EarlyRequest", &m_earlyRequest)))
    return;
  master = m_isProducer;
  finalize();
}

// Our special copy constructor
WsiPort::
WsiPort(const WsiPort &other, Worker &w , std::string &name, size_t count,
	OCPI::Util::Assembly::Role *role, const char *&err)
  : DataPort(other, w, name, count, role, err) {
  if (err)
    return;
  m_abortable = other.m_abortable;
  m_earlyRequest = other.m_earlyRequest;
  // The only attribute that isn't interface-related
  m_regRequest = false;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &WsiPort::
clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
      const char *&err) const {
  return *new WsiPort(*this, w, name, count, role, err);
}

WsiPort::
~WsiPort() {
}

bool WsiPort::
masterIn() const {
  return !m_isProducer;
}

void WsiPort::
emitPortDescription(FILE *f, Language lang) const {
  DataPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, "  %s   Abortable: %s\n", comment, BOOL(m_abortable));
  fprintf(f, "  %s   EarlyRequest: %s\n", comment, BOOL(m_earlyRequest));
  fprintf(f, "  %s   RegRequest: %s\n", comment, BOOL(m_regRequest));
}

const char *WsiPort::
deriveOCP() {
  static uint8_t s[1]; // a non-zero string pointer
  OcpPort::deriveOCP();
  ocp.MCmd.width = 3;
  if (m_preciseBurst) {
    ocp.MBurstLength.width =
      OU::floorLog2((m_protocol->m_maxMessageValues * m_protocol->m_dataValueWidth +
		 m_dataWidth - 1)/
		m_dataWidth) + 1;
    //	ocpiInfo("Burst %u from mmv %u dvw %u dw %u",
    //		  ocp->MBurstLength.width, p->m_protocol->m_maxMessageValues,
    //		  p->m_protocol->m_dataValueWidth, p->dataWidth);
    if (ocp.MBurstLength.width < 2)
      ocp.MBurstLength.width = 2;
    // FIXME: this is not really supported, but was for compatibility
    if (m_impreciseBurst)
      ocp.MBurstPrecise.value = s;
  } else
    ocp.MBurstLength.width = 2;
  if (m_byteWidth != m_dataWidth || m_protocol->m_zeroLengthMessages) {
    ocp.MByteEn.width = m_dataWidth / m_byteWidth;
    ocp.MByteEn.value = s;
  }
  if (m_dataWidth != 0)
    ocp.MData.width =
      m_byteWidth != m_dataWidth && m_byteWidth != 8 ?
      8 * m_dataWidth / m_byteWidth : m_dataWidth;
  if (m_byteWidth != m_dataWidth && m_byteWidth != 8)
    ocp.MDataInfo.width = m_dataWidth - (8 * m_dataWidth / m_byteWidth);
  if (m_earlyRequest) {
    ocp.MDataLast.value = s;
    ocp.MDataValid.value = s;
  }
  if (m_abortable)
    ocp.MDataInfo.width++;
  if (m_nOpcodes > 1)
    ocp.MReqInfo.width = OU::ceilLog2(m_nOpcodes);
  ocp.MReqLast.value = s;
  ocp.MReset_n.value = s;
  ocp.SReset_n.value = s;
  ocp.SThreadBusy.value = s;
  fixOCP();
  return NULL;
}

void WsiPort::
emitVhdlShell(FILE *f, Port *wci) {
  bool slave = masterIn();
  const char
    *mOption0 = slave ? "(others => '0')" : "open",
    *mOption1 = slave ? "(others => '1')" : "open",
    *mName = slave ? typeNameIn.c_str() : typeNameOut.c_str(),
    *sName = slave ? typeNameOut.c_str() : typeNameIn.c_str();

  size_t opcode_width = ocp.MReqInfo.value ? ocp.MReqInfo.width : 1;
	  
  fprintf(f,
	  "  --\n"
	  "  -- The WSI interface helper component instance for port \"%s\"\n",
	  name());
  if (ocp.MReqInfo.value)
    if (m_protocol && m_protocol->nOperations())
      if (slave)
	fprintf(f,
		"  %s_opcode <= %s_opcode_t'val(to_integer(unsigned(%s_opcode_temp)));\n",
		name(), m_protocol && m_protocol->operations() ?
		m_protocol->m_name.c_str() : name(), name());
      else {
	fprintf(f,
		"  -- Xilinx/ISE 14.6 synthesis doesn't do the t'pos(x) function properly\n"
		"  -- Hence this workaround\n");
	fprintf(f,
		"  %s_opcode_pos <=\n", name());
	OU::Operation *op = m_protocol->operations();
	unsigned nn;
	for (nn = 0; nn < m_protocol->nOperations(); nn++, op++)
	  fprintf(f, "    %u when %s_opcode = %s_%s_op_e else\n",
		  nn, name(), m_protocol->m_name.c_str(), op->name().c_str());
	// If the protocol opcodes do not fill the space, fill it
	if (nn < m_nOpcodes)
	  for (unsigned o = 0; nn < m_nOpcodes; nn++, o++)
	    fprintf(f, "    %u when %s_opcode = %s_opcode_t'val(%u) else\n",
		    nn, name(), m_protocol->m_name.c_str(), nn);
	fprintf(f, "    0;\n");
	fprintf(f,
		"  %s_opcode_temp <= std_logic_vector(to_unsigned(%s_opcode_pos, %s_opcode_temp'length));\n",
		name(), name(), name());
      }
    else 
      fprintf(f, "  %s_opcode%s <= %s_opcode%s;\n",
	      name(), slave ? "" : "_temp", name(), slave ? "_temp" : "");
	
  std::string width;
  OU::format(width, "ocpi_port_%s_", name());
  fprintf(f,
	  "  %s_port : component ocpi.wsi.%s%s\n"
	  "    generic map(precise          => %s,\n"
	  "                mdata_width      => %s%s,\n"
	  "                mdata_info_width => %s%s,\n"
	  "                burst_width      => %zu,\n"
	  "                n_bytes          => %s%s,\n"
	  "                byte_width       => %zu,\n"
	  "                opcode_width     => %zu,\n"
	  "                own_clock        => %s,\n"
	  "                early_request    => %s)\n",
	  name(),
	  m_isPartitioned ? "part_" : "",
	  slave ? "slave" : "master",
	  BOOL(m_preciseBurst),
	  ocp.MData.value ? width.c_str() : "",
	  ocp.MData.value ? "MData_width" : "1",
	  ocp.MDataInfo.value ? width.c_str() : "",
	  ocp.MDataInfo.value ? "MDataInfo_width" : "1",
	  ocp.MBurstLength.width,
	  ocp.MByteEn.value ? width.c_str() : "",
	  ocp.MByteEn.value ? "MByteEn_width" : "1",
	  m_byteWidth,
	  opcode_width,
	  BOOL(myClock),
	  BOOL(m_earlyRequest));
  fprintf(f, "    port map   (Clk              => %s%s,\n",
	  clock->port ? clock->port->typeNameIn.c_str() : clock->signal(),
	  clock->port ? ".Clk" : "");
  fprintf(f, "                MBurstLength     => %s.MBurstLength,\n", mName);
  fprintf(f, "                MByteEn          => %s%s,\n",
	  ocp.MByteEn.value ? mName : mOption1,
	  ocp.MByteEn.value ? ".MByteEn" : "");
  fprintf(f, "                MCmd             => %s.MCmd,\n", mName);
  fprintf(f, "                MData            => %s%s,\n",
	  ocp.MData.value ? mName : mOption0,
	  ocp.MData.value ? ".MData" : "");
  fprintf(f, "                MDataInfo        => %s%s,\n",
	  ocp.MDataInfo.value ? mName : mOption0,
	  ocp.MDataInfo.value ? ".MDataInfo" : "");
  fprintf(f, "                MDataLast        => %s%s,\n",
	  ocp.MDataLast.value ? mName : "open",
	  ocp.MDataLast.value ? ".MDataLast" : "");
  fprintf(f, "                MDataValid       => %s%s,\n",
	  ocp.MDataLast.value ? mName : "open",
	  ocp.MDataLast.value ? ".MDataValid" : "");
  fprintf(f, "                MReqInfo         => %s%s,\n",
	  ocp.MReqInfo.value ? mName : mOption0,
	  ocp.MReqInfo.value ? ".MReqInfo" : "");
  fprintf(f, "                MReqLast         => %s.MReqLast,\n", mName);
  fprintf(f, "                MReset_n         => %s.MReset_n,\n", mName);
  fprintf(f, "                SReset_n         => %s.SReset_n,\n", sName);
  fprintf(f, "                SThreadBusy      => %s.SThreadBusy,\n", sName);
  fprintf(f, "                wci_clk          => %s%s,\n",
	  wci ? wci->typeNameIn.c_str() :
	  (clock->port ? clock->port->typeNameIn.c_str() : clock->signal()),
	  wci || clock->port ? ".Clk" : "");
  fprintf(f, "                wci_reset        => %s,\n", "wci_reset");
  fprintf(f, "                wci_is_operating => %s,\n",	"wci_is_operating");
  fprintf(f, "                reset            => %s_reset,\n", name());
  fprintf(f, "                ready            => %s_ready,\n", name());
  fprintf(f, "                som              => %s_som,\n", name());
  fprintf(f, "                eom              => %s_eom,\n", name());
  if (ocp.MData.value) {
    fprintf(f, "                valid            => %s_valid,\n", name());
    fprintf(f, "                data             => %s_data,\n", name());
  } else {
    fprintf(f, "                valid            => open,\n");
    fprintf(f, "                data             => open,\n");
  }
  if (m_abortable)
    fprintf(f, "                abort            => %s_abort,\n", name());
  else
    fprintf(f, "                abort            => %s,\n", slave ? "open" : "'0'");
  if (ocp.MByteEn.value)
    fprintf(f, "                byte_enable      => %s_byte_enable,\n", name());
  else
    fprintf(f, "                byte_enable      => open,\n");
  if (m_preciseBurst)
    fprintf(f, "                burst_length     => %s_burst_length,\n", name());
  else if (slave)
    fprintf(f, "                burst_length     => open,\n");
  else
    fprintf(f, "                burst_length     => (%zu downto 0 => '0'),\n",
	    ocp.MBurstLength.width-1);
  if (ocp.MReqInfo.value)
    fprintf(f, "                opcode           => %s_opcode_temp,\n", name());
  else if (slave)
    fprintf(f, "                opcode           => open,\n");
  else
    fprintf(f, "                opcode           => (%zu downto 0 => '0'),\n",
	    opcode_width-1);
  if (slave)
    fprintf(f, "                take             => %s_take", name());
  else
    fprintf(f, "                give             => %s_give", name());
  if (m_isPartitioned)
    fprintf(f,
	    ",\n"
	    "                part_size        => %s_part_size,\n"
	    "                part_offset      => %s_part_offset,\n"
	    "                part_start       => %s_part_start,\n"
	    "                part_ready       => %s_part_ready,\n"
	    "                part_%s        => %s_part_%s",
	    name(), name(), name(), name(),
	    slave ? "take" : "give", name(), slave ? "take" : "give");
  fprintf(f, ");\n");
}

const char *WsiPort::
adjustConnection(Port &consPort, const char *masterName, Language lang,
		 OcpAdapt *prodAdapt, OcpAdapt *consAdapt, size_t &unused) {
  WsiPort &cons = *static_cast<WsiPort *>(&consPort);
  OcpAdapt *oa;
  // Bursting compatibility and adaptation
  if (m_impreciseBurst && !cons.m_impreciseBurst)
    return "consumer needs precise, and producer may produce imprecise";
  if (cons.m_impreciseBurst) {
    if (!cons.m_preciseBurst) {
      // Consumer accepts only imprecise bursts
      if (m_preciseBurst) {
	// producer may produce a precise burst
	// Convert any precise bursts to imprecise
	oa = &consAdapt[OCP_MBurstLength];
	oa->expr =
	  lang == Verilog ? "%s ? 2'b01 : 2'b10" :
	  "std_logic_vector(to_unsigned(2,2) - unsigned(ocpi.types.bit2vec(%s,2)))";
	oa->other = OCP_MReqLast;
	oa->comment = "Convert precise to imprecise";
	oa->isExpr = true; // subtraction
	oa = &prodAdapt[OCP_MBurstLength];
	oa->expr = lang == Verilog ? "" : "open";
	unused += ocp.MBurstLength.width;
	oa->comment = "MBurstLength ignored for imprecise consumer";
	if (m_impreciseBurst) {
	  oa = &prodAdapt[OCP_MBurstPrecise];
	  oa->expr = lang == Verilog ? "" : "open";
	  oa->comment = "MBurstPrecise ignored for imprecise-only consumer";
	  unused++;
	}
      }
    } else { // consumer does both
      // Consumer accept both, has MPreciseBurst Signal
      oa = &consAdapt[OCP_MBurstPrecise];
      if (!m_impreciseBurst) {
	oa->expr = lang == Verilog ? "1'b1" : "to_unsigned(1,1)";
	oa->comment = "Tell consumer all bursts are precise";
      } else if (!m_preciseBurst) {
	oa = &consAdapt[OCP_MBurstPrecise];
	oa->expr = lang == Verilog ? "1'b0" : "'0'";
	oa->comment = "Tell consumer all bursts are imprecise";
	oa = &consAdapt[OCP_MBurstLength];
	oa->other = OCP_MBurstLength;
	asprintf((char **)&oa->expr,
		 lang == Verilog ? "{%zu'b0,%%s}" : "std_logic_vector(to_unsigned(0,%zu)) & %%s",
		 cons.ocp.MBurstLength.width - 2);
	oa->comment = "Consumer only needs imprecise burstlength (2 bits)";
	oa->isExpr = true; // concatenation
      }
    }
  }
  if (m_preciseBurst && cons.m_preciseBurst &&
      ocp.MBurstLength.width < cons.ocp.MBurstLength.width) {
    oa = &consAdapt[OCP_MBurstLength];
    asprintf((char **)&oa->expr,
	     lang == Verilog ? "{%zu'b0,%%s}" : "to_unsigned(0,%zu) & %%s",
	     cons.ocp.MBurstLength.width - ocp.MBurstLength.width);
    oa->comment = "Consumer takes bigger bursts than producer creates";
    oa->other = OCP_MBurstLength;
  }
  // Abortable compatibility and adaptation
  if (cons.m_abortable) {
    if (!m_abortable) {
      oa = &consAdapt[OCP_MDataInfo];
      oa->expr = lang == Verilog ? "{1'b0,%s}" : "\"0\" & %s";
      oa->comment = "Tell consumer no frames are ever aborted";
    }
  } else if (m_abortable)
    return "consumer cannot handle aborts from producer";
  // EarlyRequest compatibility and adaptation
  if (cons.m_earlyRequest) {
    if (!m_earlyRequest) {
      oa = &consAdapt[OCP_MDataLast];
      oa->other = OCP_MReqLast;
      oa->expr = "%s";
      oa->comment = "Tell consumer last data is same as last request";
      oa = &consAdapt[OCP_MDataValid];
      oa->other = OCP_MCmd;
      oa->expr = "%s == OCPI_OCP_MCMD_WRITE ? 1b'1 : 1b'0";
      oa->comment = "Tell consumer data is valid when its(request) is MCMD_WRITE";
      oa->isExpr = true;
    }
  } else if (m_earlyRequest)
    return "producer emits early requests, but consumer doesn't support them";
  // Opcode compatibility
  if (cons.m_nOpcodes != m_nOpcodes)
    if (cons.ocp.MReqInfo.value) {
      if (ocp.MReqInfo.value) {
	if (cons.ocp.MReqInfo.width > ocp.MReqInfo.width) {
	  oa = &consAdapt[OCP_MReqInfo];
	  asprintf((char **)&oa->expr,
		   lang == Verilog ?
		   "{%zu'b0,%%s}" : "std_logic_vector(to_unsigned(0,%zu)) & %%s",
		   cons.ocp.MReqInfo.width - ocp.MReqInfo.width);
	  oa->isExpr = true;
	  oa->other = OCP_MReqInfo;
	} else {
	  // producer has more, we just connect the LSBs
	}
      } else {
	// producer has none, consumer has some
	oa = &consAdapt[OCP_MReqInfo];
	asprintf((char **)&oa->expr,
		 lang == Verilog ? "%zu'b0" : "std_logic_vector(to_unsigned(0,%zu))",
		 cons.ocp.MReqInfo.width);
      }
    } else {
      // consumer has none
      oa = &prodAdapt[OCP_MReqInfo];
      oa->expr = lang == Verilog ? "" : "open";
      oa->comment = "Consumer doesn't have opcodes (or has exactly one)";
    }
  // Byte enable compatibility
  oa = &consAdapt[OCP_MByteEn];
  if (cons.ocp.MByteEn.value && ocp.MByteEn.value) {
    if (cons.ocp.MByteEn.width < ocp.MByteEn.width) {
      // consumer has less - "inclusive-or" the various bits
      if (ocp.MByteEn.width % cons.ocp.MByteEn.width)
	return "byte enable producer width not a multiple of consumer width";
      size_t nper = ocp.MByteEn.width / cons.ocp.MByteEn.width;
      std::string expr;
      size_t pw = ocp.MByteEn.width;
      if (lang == Verilog) {
	for (size_t n = 0; n < cons.ocp.MByteEn.width; n++) {
	  expr += n ? "," : "{";
	  for (size_t nn = 0; nn < nper; nn++)
	    OU::formatAdd(expr, "%s%sMByteEn[%zu]", nn ? "|" : "",
			  masterName, --pw);
	}
	expr += "}";
      } else {
	for (size_t n = 0; n < cons.ocp.MByteEn.width; n++) {
	  expr += n ? "&ocpi.util.slv(" : "ocpi.util.slv(";
	  for (size_t nn = 0; nn < nper; nn++)
	    OU::formatAdd(expr, "%s%s.MByteEn(%zu)", nn ? " or " : "",
			  masterName, --pw);
	  expr += ")";
	}
	oa->isExpr = true;
      }
      oa->expr = strdup(expr.c_str());
      oa->comment = "inclusive-or more numerous producer byte enables for consumer";
    } else if (cons.ocp.MByteEn.width > ocp.MByteEn.width) {
      // consumer has more - requiring replicating
      if (cons.ocp.MByteEn.width % ocp.MByteEn.width)
	return "byte enable consumer width not a multiple of producer width";
      size_t nper = cons.ocp.MByteEn.width / ocp.MByteEn.width;
      std::string expr;
      if (lang == Verilog) {
	expr = "{";
	for (size_t n = 0; n < ocp.MByteEn.width; n++)
	  for (size_t nn = 0; nn < nper; nn++)
	    OU::formatAdd(expr, "%s%sMByteEn[%zu]", n || nn ? "," : "",
			  masterName, ocp.MByteEn.width - n - 1);
	expr += "}";
      } else {
	oa->isExpr = true;
	for (size_t n = 0; n < ocp.MByteEn.width; n++)
	  for (size_t nn = 0; nn < nper; nn++)
	    OU::formatAdd(expr, "%s%s.MByteEn(%zu)", n || nn ? "&" : "",
			  masterName, ocp.MByteEn.width - n - 1);
      }
      oa->comment = "replicate producers fewer byte enables for consumer";
      oa->expr = strdup(expr.c_str());
    }
  } else if (cons.ocp.MByteEn.value) {
    // only consumer has byte enables - make them all 1
    if (lang == VHDL)
      oa->expr = strdup("(others => '1')");
    else
      asprintf((char **)&oa->expr, "{%zu{1'b1}}", cons.ocp.MByteEn.width);
  } else if (ocp.MByteEn.value) {
    // only producer has byte enables
    oa = &prodAdapt[OCP_MByteEn];
    oa->expr = lang == Verilog ? "" : "open";
    oa->comment = "consumer does not have byte enables";
  }
  size_t
    cmdi = cons.ocp.MDataInfo.width - (cons.m_abortable ? 1 : 0),
    pmdi = ocp.MDataInfo.width - (m_abortable ? 1 : 0),
    pbytes = ocp.MByteEn.value ? ocp.MByteEn.width : 1,
    cbytes = cons.ocp.MByteEn.value ? cons.ocp.MByteEn.width : 1,
    pbs = (pmdi + ocp.MData.width) / pbytes,
    cbs = (cmdi + cons.ocp.MData.width) / cbytes;
  ocpiInfo("pbytes %zu, cbytes %zu, pbs %zu cbs %zu cmdi %zu pmdi %zu",
	   pbytes, cbytes, pbs, cbs, cmdi, pmdi);
  if (cons.ocp.MData.width + cmdi != ocp.MData.width + pmdi)
    return "data widths do not match";
  // total data bits do match, but may be different bytes
  std::string expr;
  if (cmdi < pmdi) {
    // Consumer byte size is less than producer
    if (cmdi == 0) {
      // make consumer's mdata from a mix of producer's mdatainfo and mdata
      oa = &consAdapt[OCP_MData];
      oa->comment = "Consumer has no MDataInfo";
      if (lang == Verilog) {
	for (size_t n = pbytes; n > 0; n--)
	  OU::formatAdd(expr, "%s%sMDataInfo[%zu:%zu],%sMData[%zu:%zu]",
			n == pbytes ? "{" : ",",
			masterName, n*(pbs-8)-1, (n-1)*(pbs-8),
			masterName, n*8-1, (n-1)*8);
	expr += "}";
      } else {
	oa->isExpr = true;
	for (size_t n = pbytes; n > 0; n--)
	  OU::formatAdd(expr, "%s%s.MDataInfo(%zu downto %zu) & %s.MData(%zu downto %zu)",
			n == pbytes ? "" : "&",
			masterName, n*(pbs-8)-1, (n-1)*(pbs-8),
			masterName, n*8-1, (n-1)*8);
      }
      oa->expr = strdup(expr.c_str());
      ocpiInfo("expr: %s", oa->expr);
    } else {
      // remap producer's larger bytes into consumer's smaller bytes
    }
  } else if (pmdi < cmdi) {
    // Consumer byte size is greater than producer
    if (pmdi == 0) {
      // make consumer's mdata and mdatainfo from producer's mdata
      std::string dexpr, iexpr;
      OcpAdapt
	&oad = consAdapt[OCP_MData],
	&oai = consAdapt[OCP_MDataInfo];
      oad.comment = "Consumer gets 8 LSBs of each of the larger bytes in MData";
      oai.comment = "Consumer gets MSBs > 8 of each of the larger bytes in MDataInfo";
      if (lang == Verilog) {
	for (size_t n = cbytes; n > 0; n--) {
	  OU::formatAdd(dexpr, "%s%sMData[%zu:%zu]", n == cbytes ? "{" : ",",
			masterName, (n-1)*cbs+7, (n-1)*cbs);
	  OU::formatAdd(iexpr, "%s%sMData[%zu:%zu]", n == cbytes ? "{" : ",",
			masterName, n*cbs-1, (n-1)*cbs + 8);
	}
	dexpr += "}";
	iexpr += "}";
      } else {
	oa->isExpr = true;
	for (size_t n = cbytes; n > 0; n--) {
	  OU::formatAdd(dexpr, "%s%s.MData(%zu downto %zu)",
			n == cbytes ? "" : "&",
			masterName, (n-1)*cbs+7, (n-1)*cbs);
	  OU::formatAdd(iexpr, "%s%s.MData(%zu downto %zu)",
			n == cbytes ? "" : "&",
			masterName, n*cbs-1, (n-1)*cbs + 8);
	}
      }
      oad.expr = strdup(dexpr.c_str());
      oai.expr = strdup(iexpr.c_str());
      ocpiInfo("dexpr: %s", oad.expr);
      ocpiInfo("iexpr: %s", oai.expr);
    } else {
      // remap producer's smaller bytes into consumer's larger bytes
    }
  }
  return NULL;
}

void WsiPort::
emitImplAliases(FILE *f, unsigned n, Language lang) {
  const char *comment = hdlComment(lang);
  const char *pin = fullNameIn.c_str();
  const char *pout = fullNameOut.c_str();
  bool mIn = masterIn();

  if (m_regRequest) {
    fprintf(f,
	    "  %s Register declarations for request phase signals for interface \"%s\"\n",
	    comment, name());
    OcpSignalDesc *osd = ocpSignals;
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (osd->request && m_isProducer && m_regRequest && os->value &&
	  strcmp("MReqInfo", osd->name)) { // fixme add "aliases" attribute somewhere
	if (osd->vector)
	  fprintf(f, "  reg [%3zu:0] %s%s;\n", os->width - 1, pout, osd->name);
	else
	  fprintf(f, "  reg %s%s;\n", pout, osd->name);
      }
  }
  fprintf(f,
	  "  %s Aliases for interface \"%s\"\n", comment, name());
  if (ocp.MReqInfo.width) {
    if (n == 0) {
      if (lang != VHDL)
	fprintf(f,
		"  localparam %sOpCodeWidth = %zu;\n",
		mIn ? pin : pout, ocp.MReqInfo.width);
    }
    if (lang != VHDL) {
      if (mIn)
	fprintf(f,
		"  wire [%zu:0] %sOpcode = %sMReqInfo;\n",
		ocp.MReqInfo.width - 1, pin, pin);
      else
	fprintf(f,
		//"  wire [%u:0] %s_Opcode; always@(posedge %s) %s_MReqInfo = %s_Opcode;\n",
		// ocp.MReqInfo.width - 1, pout, clock->signal, pout, pout);
		"  %s [%zu:0] %sOpcode; assign %sMReqInfo = %sOpcode;\n",
		m_regRequest ? "reg" : "wire", ocp.MReqInfo.width - 1, pout, pout, pout);
    }
    emitOpcodes(f, mIn ? pin : pout, lang);
  }
  if (m_abortable) {
    if (lang == VHDL)
      fprintf(f,
	      "  alias %sAbort : std_logic is %s.MDataInfo(%s.MDataInfo'left);\n",
	      mIn ? pin : pout, mIn ? pin : pout, mIn ? pin : pout);
    else if (mIn)
      fprintf(f,
	      "  wire %sAbort = %sMDataInfo[%zu];\n",
	      pin, pin, ocp.MDataInfo.width-1);
    else
      fprintf(f,
	      "  wire %sAbort; assign %sMDataInfo[%zu] = %sAbort;\n",
	      pout, pout, ocp.MDataInfo.width-1, pout);
  }
}

void WsiPort::
emitSkelSignals(FILE *f) {
  if (m_worker->m_language != VHDL && m_regRequest)
    fprintf(f,
	    "// GENERATED: OCP request phase signals for interface \"%s\" are registered\n",
	    name());
}

void WsiPort::
emitRecordInputs(FILE *f) {
  DataPort::emitRecordInputs(f);
  if (masterIn()) {
    fprintf(f,
	    "                                         -- true means \"take\" is allowed\n"
	    "                                         -- one or more of: som, eom, valid are true\n");
    if (m_dataWidth) {
      fprintf(f,
	      "    data             : std_logic_vector(ocpi_port_%s_data_width-1 downto 0);\n",
	      name());	      
      // This shouldn't be here when the width is 1, but some workers have been written this way
      // so we'll leave it, but it is redundant with valid when the width is 1
      if (ocp.MByteEn.value)
	fprintf(f,
		"    byte_enable      : std_logic_vector(ocpi_port_%s_MByteEn_width-1 downto 0);\n",
		name());
    }
    if (m_nOpcodes > 1)
      fprintf(f,
	      "    opcode           : %s_OpCode_t;\n",
	      m_protocol && m_protocol->operations() ?
	      m_protocol->m_name.c_str() : name());
    fprintf(f,
	    m_dataWidth ?
	    "    som, eom, valid  : Bool_t;           -- valid means data and byte_enable are present\n" :
	    "    som, eom  : Bool_t;\n");
    if (m_isPartitioned)
      fprintf(f,
	      "    part_size        : UShort_t;\n"
	      "    part_offset      : UShort_t;\n"
	      "    part_start       : Bool_t;\n"
	      "    part_ready       : Bool_t;\n");
  } else if (m_isPartitioned)
    fprintf(f,
	      "    part_ready       : Bool_t;\n");
}
void WsiPort::
emitRecordOutputs(FILE *f) {
  DataPort::emitRecordOutputs(f);
  if (masterIn()) {
    fprintf(f,
	    "    take             : Bool_t;           -- take data now from this port\n"
	    "                                         -- can be asserted when ready is true\n");
    if (m_isPartitioned)
      fprintf(f,
	      "    part_take        : Bool_t;           -- take partition data\n");
  } else {
    fprintf(f,
	    "    give             : Bool_t;           -- give data now to this port\n"
	    "                                         -- can be asserted when ready is true\n");
    if (m_dataWidth) {
      fprintf(f,
	      "    data             : std_logic_vector(ocpi_port_%s_data_width-1 downto 0);\n",
	      name());
      // This should not really be generated when the width is one, but some workers set this
      // signal so we don't want to break them.  But it is ignored since "valid" is what is
      // documented and specified.
      if (ocp.MByteEn.value)
	fprintf(f,
		"    byte_enable      : std_logic_vector(ocpi_port_%s_MByteEn_width-1 downto 0);\n",
		name());
    }
    if (m_nOpcodes > 1)
      fprintf(f,
	      "    opcode           : %s_OpCode_t;\n",
	      m_protocol && m_protocol->operations() ?
	      m_protocol->m_name.c_str() : name());
    fprintf(f,
	    "    som, eom, valid  : Bool_t;            -- one or more must be true when 'give' is asserted\n");
    if (m_isPartitioned)
      fprintf(f,
	      "    part_size        : UShort_t;\n"
	      "    part_offset      : UShort_t;\n"
	      "    part_start       : Bool_t;\n"
	      "    part_give        : Bool_t;\n");
  }
}
unsigned WsiPort::
extraDataInfo() const {
  return m_abortable ? 1 : 0;
}
