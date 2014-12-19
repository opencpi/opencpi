#include "hdl.h"

WmiPort::
WmiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : DataPort(w, x, sp, ordinal, WMIPort, err) {
    if (err ||
	(err = OE::checkAttrs(x, "Name", "Clock", "MyClock", "DataWidth", "master",
                              "PreciseBurst", "MFlagWidth", "ImpreciseBurst",
                              "Continuous", "ByteWidth", "TalkBack",
                              "Bidirectional", "Pattern",
                              "NumberOfOpcodes", "MaxMessageValues",
			      "datavaluewidth", "zerolengthmessages",
                              (void*)0)) ||
	(err = OE::getBoolean(m_xml, "master", &master)) || // this is unique to WMI
        (err = OE::getBoolean(x, "TalkBack", &m_talkBack)) ||
        (err = OE::getBoolean(x, "Bidirectional", &m_isBidirectional)) ||
        (err = OE::getNumber(x, "MFlagWidth", &m_mflagWidth, 0, 0)))
    return;
  finalize();
}
// Our special copy constructor
WmiPort::
WmiPort(const WmiPort &other, Worker &w , std::string &name, size_t count,
	OCPI::Util::Assembly::Role *role, const char *&err)
  : DataPort(other, w, name, count, role, err) {
  if (err)
    return;
  m_talkBack = other.m_talkBack;
  m_mflagWidth = other.m_mflagWidth;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &WmiPort::
clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
      const char *&err) const {
  return *new WmiPort(*this, w, name, count, role, err);
}

void WmiPort::
emitPortDescription(FILE *f, Language lang) const {
  DataPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, "  %s   TalkBack: %s\n", comment, BOOL(m_talkBack));
}

const char *WmiPort::
deriveOCP() {
  static uint8_t s[1]; // a non-zero string pointer
  OcpPort::deriveOCP();
  ocp.MCmd.width = 3;
  {
    size_t n = (m_maxMessageValues * m_dataValueWidth +
		m_dataWidth - 1) / m_dataWidth;
    if (n > 1) {
      ocp.MAddr.width = ceilLog2(n);
      unsigned long nn = ceilLog2(m_dataWidth);
      if (nn > 3)
	ocp.MAddr.width += ceilLog2(m_dataWidth) - 3;
    }
    ocp.MAddrSpace.value = s;
    if (m_preciseBurst) {
      ocp.MBurstLength.width = n < 4 ? 2 : floorLog2(n-1) + 1;
      if (m_impreciseBurst)
	ocp.MBurstPrecise.value = s;
    } else
      ocp.MBurstLength.width = 2;
  }
  if (m_isProducer || m_talkBack || m_isBidirectional) {
    ocp.MData.width =
      m_byteWidth != m_dataWidth && m_byteWidth != 8 ?
      8 * m_dataWidth / m_byteWidth : m_dataWidth;
    if (m_byteWidth != m_dataWidth)
      ocp.MDataByteEn.width = m_dataWidth / m_byteWidth;
    if (m_byteWidth != m_dataWidth && m_byteWidth != 8)
      ocp.MDataInfo.width = m_dataWidth - (8 * m_dataWidth / m_byteWidth);
    ocp.MDataLast.value = s;
    ocp.MDataValid.value = s;
  }
  if ((m_isProducer || m_isBidirectional) &&
      (m_nOpcodes > 1 || m_variableMessageLength))
    ocp.MFlag.width = 8 + ceilLog2(m_maxMessageValues + 1);
  ocp.MReqInfo.width = 1;
  ocp.MReqLast.value = s;
  ocp.MReset_n.value = s;
  if (!m_isProducer || m_talkBack || m_isBidirectional)
    ocp.SData.width = m_dataWidth;
  if (m_isProducer || m_talkBack || m_isBidirectional)
    ocp.SDataThreadBusy.value = s;
  if ((!m_isProducer || m_isBidirectional) &&
      (m_nOpcodes > 1 || m_variableMessageLength))
    ocp.SFlag.width = 8 + ceilLog2(m_maxMessageValues + 1);
  ocp.SReset_n.value = s;
  if (!m_isProducer || m_talkBack || m_isBidirectional)
    ocp.SResp.value = s;
  if ((m_impreciseBurst || m_preciseBurst) &&
      (!m_isProducer || m_talkBack || m_isBidirectional))
    ocp.SRespLast.value = s;
  ocp.SThreadBusy.value = s;
  if (m_mflagWidth) {
    ocp.MFlag.width = m_mflagWidth; // FIXME remove when shep kludge unnecessary
    ocp.SFlag.width = m_mflagWidth; // FIXME remove when shep kludge unnecessary
  }
  fixOCP();
  return NULL;
}
const char *WmiPort::
adjustConnection(Port &/*cons*/, const char */*masterName*/, Language /*lang*/,
		 OcpAdapt */*prodAdapt*/, OcpAdapt */*consAdapt*/) {
  return NULL;
}

void WmiPort::
emitImplAliases(FILE *f, unsigned /*n*/, Language lang) {
  const char *comment = hdlComment(lang);
  const char *pin = fullNameIn.c_str();
  const char *pout = fullNameOut.c_str();
  bool mIn = masterIn();
  fprintf(f,
	  "  %s Aliases for interface \"%s\"\n", comment, name());
  if (lang != VHDL) {
    if (master) // if we are app
      fprintf(f,
	      "  wire %sNodata; assign %sMAddrSpace[0] = %sNodata;\n"
	      "  wire %sDone;   assign %sMReqInfo[0] = %sDone;\n",
	      pout, pout, pout, pout, pout, pout);
    else // we are infrastructure
      fprintf(f,
	      "  wire %sNodata = %sMAddrSpace[0];\n"
	      "  wire %sDone   = %sMReqInfo[0];\n",
	      pin, pin, pin, pin);
  }
  if (m_nOpcodes > 1) {
    if (lang != VHDL) {
      if (m_isProducer) // opcode is an output
	fprintf(f,
		"  wire [7:0] %sOpcode; assign %s%cFlag[7:0] = %sOpcode;\n",
		pout, pout, master ? 'M' : 'S', pout);
      else
	fprintf(f,
		"  wire [7:0] %sOpcode = %s%cFlag[7:0];\n",
		pin, pin, master ? 'S' : 'M');
      fprintf(f,
	      "  localparam %sOpCodeWidth = 7;\n",
	      mIn ? pin : pout);
    }
    emitOpcodes(f, mIn ? pin : pout, lang);
  }
  if (m_variableMessageLength) {
    if (lang != VHDL) {
      if (m_isProducer) { // length is an output
	size_t width =
	  (master ? ocp.MFlag.width : ocp.SFlag.width) - 8;
	fprintf(f,
		"  wire [%zu:0] %sLength; assign %s%cFlag[%zu:8] = %sLength;\n",
		width - 1, pout, pout, master ? 'M' : 'S', width + 7, pout);
      } else {
	size_t width =
	  (master ? ocp.SFlag.width : ocp.MFlag.width) - 8;
	fprintf(f,
		"  wire [%zu:0] %sLength = %s%cFlag[%zu:8];\n",
		width - 1, pin, pin, master ? 'S' : 'M', width + 7);
      }
    }
  }
}

// HACK: just a copy of the WSI interface
void WmiPort::
emitRecordInputs(FILE *f) {
  DataPort::emitRecordInputs(f);
  if (masterIn()) {
    fprintf(f,
	    "                                         -- true means \"take\" is allowed\n"
	    "                                         -- one or more of: som, eom, valid are true\n");
    if (m_dataWidth) {
      fprintf(f,
	      "    data             : std_logic_vector(%zu downto 0);\n",
	      m_dataWidth-1);	      
      if (ocp.MByteEn.value)
	fprintf(f,
		"    byte_enable      : std_logic_vector(%zu downto 0);\n",
		m_dataWidth/m_byteWidth-1);
    }
    if (m_nOpcodes > 1)
      fprintf(f,
	      "    opcode           : %s_OpCode_t;\n",
	      operations() ? OU::Protocol::m_name.c_str() : name());
    fprintf(f,
	    m_dataWidth ?
	    "    som, eom, valid  : Bool_t;           -- valid means data and byte_enable are present\n" :
	    "    som, eom  : Bool_t;\n");
  }
}
void WmiPort::
emitRecordOutputs(FILE *f) {
  DataPort::emitRecordOutputs(f);
  if (masterIn()) {
    fprintf(f,
	    "    take             : Bool_t;           -- take data now from this port\n"
	    "                                         -- can be asserted when ready is true\n");
  } else {
    fprintf(f,
	    "    give             : Bool_t;           -- give data now to this port\n"
	    "                                         -- can be asserted when ready is true\n");
    if (m_dataWidth) {
      fprintf(f,
	      "    data             : std_logic_vector(%zu downto 0);\n",
	      m_dataWidth-1);	      
      if (ocp.MByteEn.value)
	fprintf(f,
		"    byte_enable      : std_logic_vector(%zu downto 0);\n",
		m_dataWidth/m_byteWidth-1);
    }
    if (m_nOpcodes > 1)
      fprintf(f,
	      "    opcode           : %s_OpCode_t;\n",
	      operations() ? OU::Protocol::m_name.c_str() : name());
    fprintf(f,
	    "    som, eom, valid  : Bool_t;            -- one or more must be true when 'give' is asserted\n");
      }
}
