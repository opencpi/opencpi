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

#include <algorithm>
#include "data.h"
#include "hdl.h"

WsiPort::
WsiPort(Worker &w, ezxml_t x, DataPort *sp, int ordinal, const char *&err)
  : DataPort(w, x, sp, ordinal, WSIPort, err) {
  if (err)
    return;
  if ((err = OE::checkAttrs(x, "Name", "Clock", "DataWidth", "PreciseBurst", "myoutputclock",
			    "ImpreciseBurst", "Continuous", "Abortable",
			    "EarlyRequest", "MyClock", "RegRequest", "InsertEOM", "workerEOF", "Pattern",
			    "NumberOfOpcodes", "MaxMessageValues",
			    "datavaluewidth", "zerolengthmessages",
			    "datavaluegranularity", "implname", "producer", "optional", "buffersize",
			    DISTRIBUTION_ATTRS, PARTITION_ATTRS,
			    (void*)0)) ||
      (err = OE::getBoolean(x, "InsertEOM", &m_insertEOM)) ||
      (err = OE::getBoolean(x, "Abortable", &m_abortable)) ||
      (err = OE::getBoolean(x, "RegRequest", &m_regRequest)) ||
      (err = OE::getBoolean(x, "EarlyRequest", &m_earlyRequest)))
    return;
  m_master = m_isProducer;
  //  finalize();
}

// Our special copy constructor
WsiPort::
WsiPort(const WsiPort &other, Worker &w , std::string &a_name, size_t count,
	OU::Assembly::Role *role, const char *&err)
  : DataPort(other, w, a_name, count, role, err) {
  if (err)
    return;
  m_abortable = other.m_abortable;
  m_earlyRequest = other.m_earlyRequest;
  // The attributes that aren't interface-related
  m_insertEOM = false;
  m_regRequest = false;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &WsiPort::
clone(Worker &w, std::string &a_name, size_t count, OCPI::Util::Assembly::Role *role,
      const char *&err) const {
  err = NULL;
  return *new WsiPort(*this, w, a_name, count, role, err);
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

// This is just here to supply the default output initializations of optionally-driven worker signals
void WsiPort::
emitRecordSignal(FILE *f, std::string &last, const char *aprefix, bool inRecord, bool inPackage,
		 bool inWorker, const char *defaultIn, const char *defaultOut) {
  std::string d;
  if (inWorker && haveWorkerOutputs() && !masterIn()) {
    d = "(";
    if (ocp.MData.value)
      d += "data => (others => '0'), ";
    if (ocp.MByteEn.value)
      d += "byte_enable => (others => '1'), ";
    if (m_nOpcodes > 1) {
      d += "opcode => ";
      if (operations())
	OU::formatAdd(d, "%s_OpCode_t'val(0), ", OU::Protocol::cname());
      else
	d += "(others => '0'), ";
    }
    d += "others => bfalse)"; // for metadata signals
    defaultOut = d.c_str();
  }
  OcpPort::emitRecordSignal(f, last, aprefix, inRecord, inPackage, inWorker, defaultIn, defaultOut);
}

const char *WsiPort::
deriveOCP() {
  static uint8_t s[1]; // a non-zero string pointer
  OcpPort::deriveOCP();
  ocp.MCmd.width = 3;
  if (m_preciseBurst) {
    ocp.MBurstLength.width =
      OU::floorLog2((m_maxMessageValues * m_dataValueWidth +
		 m_dataWidth - 1)/
		m_dataWidth) + 1;
    //	ocpiInfo("Burst %u from mmv %u dvw %u dw %u",
    //		  ocp->MBurstLength.width, p->m_maxMessageValues,
    //		  p->m_dataValueWidth, p->dataWidth);
    if (ocp.MBurstLength.width < 2)
      ocp.MBurstLength.width = 2;
    // FIXME: this is not really supported, but was for compatibility
    if (m_impreciseBurst)
      ocp.MBurstPrecise.value = s;
  } else
    ocp.MBurstLength.width = 2;
  if (m_byteWidth && (m_byteWidth != m_dataWidth || m_zeroLengthMessages)) {
    ocp.MByteEn.width = m_dataWidth / m_byteWidth;
    ocp.MByteEn.value = s;
  }
  if (m_dataWidth != 0)
    ocp.MData.width =
      m_byteWidth != m_dataWidth && m_byteWidth != 8 ?
      8 * m_dataWidth / m_byteWidth : m_dataWidth;
  if (m_byteWidth && m_byteWidth != m_dataWidth && m_byteWidth != 8)
    ocp.MDataInfo.width = m_dataWidth - (8 * m_dataWidth / m_byteWidth);
  if (m_earlyRequest) {
    ocp.MDataLast.value = s;
    ocp.MDataValid.value = s;
  }
  ocp.MDataInfo.width++; // used for abort or eof so always present
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
emitVhdlShell(FILE *f, ::Port *wci) {
  bool slave = masterIn();
  std::string mNameTemp;
  if (!slave)
    mNameTemp = typeNameOut + "_temp";
  const char
    *mOption0 = slave ? "(others => '0')" : "open",
    *mOption1 = slave ? "(others => '1')" : "open",
    *mName = slave ? typeNameIn.c_str() : mNameTemp.c_str(),
    *sName = slave ? typeNameOut.c_str() : typeNameIn.c_str();

  size_t opcode_width = ocp.MReqInfo.value ? ocp.MReqInfo.width : 1;

  fprintf(f,
	  "  --\n"
	  "  -- The WSI interface helper component instance for port \"%s\"\n",
	  cname());
  if (ocp.MReqInfo.value) {
    if (nOperations()) {
      if (slave) {
#if 0
	fprintf(f,
		"  %s_opcode <= %s_opcode_t'val(to_integer(unsigned(%s_opcode_temp)));\n",
		name(), nOperations() ?
		OU::Protocol::cname() : pname(), pname());
#else
	fprintf(f,
		"  -- Xilinx/ISE 14.6 synthesis doesn't do the t'val(x) function properly\n"
		"  -- Hence this workaround\n");
	fprintf(f,
		"  %s_opcode <=\n", cname());
	OU::Operation *op = operations();
	unsigned nn;
	for (nn = 0; nn < nOperations(); nn++, op++)
	  fprintf(f, "%s    %s_%s_op_e when to_integer(unsigned(%s_opcode_temp)) = %u",
		  nn ? " else\n" : "", OU::Protocol::cname(), op->cname(), pname(), nn);
	// If the protocol opcodes do not fill the space, fill it
	if (nn < m_nOpcodes)
#if 1 // fix inferred latch warning
	  for (unsigned o = 0; nn < m_nOpcodes; nn++, o++) {
	    fprintf(f, " else\n    op%u_e", nn);
	    if (nn != m_nOpcodes-1)
	      fprintf(f, "  when to_integer(unsigned(%s_opcode_temp)) = %u", cname(), nn);
	  }
#else
	  for (unsigned o = 0; nn < m_nOpcodes; nn++, o++)
	    fprintf(f, " else\n    op%u _e when to_integer(unsigned(%s_opcode_temp)) = %u",
		    nn, cname(), nn);
#endif
	fprintf(f, ";\n");
#endif
      } else {
	fprintf(f,
		"  -- Xilinx/ISE 14.6 synthesis doesn't do the t'pos(x) function properly\n"
		"  -- Hence this workaround\n");
	fprintf(f,
		"  %s_opcode_pos <=\n", cname());
	OU::Operation *op = operations();
	unsigned nn;
	for (nn = 0; nn < nOperations(); nn++, op++)
	  fprintf(f, "    %u when %s_opcode = %s_%s_op_e else\n",
		  nn, cname(), OU::Protocol::cname(), op->cname());
	// If the protocol opcodes do not fill the space, fill it
	if (nn < m_nOpcodes)
	  for (unsigned o = 0; nn < m_nOpcodes; nn++, o++)
	    fprintf(f, "    %u when %s_opcode = %s_opcode_t'val(%u) else\n",
		    nn, cname(), OU::Protocol::cname(), nn);
	fprintf(f, "    0;\n");
	fprintf(f,
		"  %s_opcode_temp <= std_logic_vector(to_unsigned(%s_opcode_pos, %s_opcode_temp'length));\n",
		cname(), cname(), cname());
      }
    } else 
      fprintf(f, "  %s_opcode%s <= %s_opcode%s;\n",
	      cname(), slave ? "" : "_temp", cname(), slave ? "_temp" : "");
  }

  if (!slave)
    // xsim segfaults if there is no intermediate signal here
    fprintf(f, "  %s <= %s; -- temp needed to workaround Vivado/xsim bug v2016.4\n",
	    typeNameOut.c_str(), mName);

  // Compute name of first WSI input port and output port, if any
  const char *firstIn = NULL;
  for (unsigned i = 0; i < worker().m_ports.size(); i++) {
    ::Port &p = *worker().m_ports[i];
    if (p.m_type == WSIPort && !firstIn && !p.m_master) {
      firstIn = p.pname();
      break;
    }
  }
  std::string width;
  OU::format(width, "ocpi_port_%s_", cname());
  fprintf(f,
	  "  %s_port : component ocpi.wsi.%s%s\n"
	  "    generic map(precise          => %s,\n"
	  "                mdata_width      => %s%s,\n"
	  "                mdata_info_width => %s%s,\n"
	  "                burst_width      => %zu,\n"
	  "                n_bytes          => %s%s,\n"
	  "                byte_width       => %zu,\n"
	  "                opcode_width     => %zu,\n"
	  "                own_clock        => %s,\n",
	  cname(),
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
	  BOOL(m_myClock));
  if (!slave)
    fprintf(f,
	    "                insert_eom        => %s,\n"
	    "                max_bytes         => to_integer(ocpi_max_bytes_%s),\n"
	    "                max_latency       => to_integer(ocpi_max_latency_%s),\n"
	    "                worker_eof        => %s,\n"
	    "                fixed_buffer_size => %s,\n"
	    "                debug             => its(ocpi_debug),\n",
	    BOOL(m_insertEOM),
	    cname(), cname(),
	    BOOL(m_workerEOF),
	    "false");
  fprintf(f,
	  "                hdl_version      => to_integer(ocpi_version),\n"
	  "                early_request    => %s)\n",
	  BOOL(m_earlyRequest));
  std::string clockName(m_clock->signal());
  if (m_clock->m_port) {
    if (m_clock->m_output) {
      if (m_clock->m_port->isDataProducer())
	clockName = m_clock->m_port->typeNameOut + "_temp.";
      else
	clockName = m_clock->m_port->pname(), clockName += "_";
    } else
      clockName = m_clock->m_port->typeNameIn + ".";
    clockName += "Clk";
  }
  fprintf(f, "    port map   (Clk              => %s,\n", clockName.c_str());
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
	  (m_clock->m_port ? m_clock->m_port->typeNameIn.c_str() : m_clock->signal()),
	  wci || m_clock->m_port ? ".Clk" : "");
  fprintf(f, "                wci_reset        => %s,\n", "wci_reset");
  fprintf(f, "                wci_is_operating => %s,\n",	"wci_is_operating");
  if (slave)
    fprintf(f,
	    "                first_take       => %s_first_take, -- output the input port\n"
	    "                eof              => %s_eof,        -- output from the input port\n",
	    cname(), cname());
  else
    fprintf(f,
	    "                first_take       => %s%s,   -- from input port to output port\n"
	    "                input_eof        => %s%s,   -- from input port to output port\n"
	    "                eof              => %s%s,   -- from worker to output port\n"
	    "                latency          => %s%s,\n"
	    "                buffer_size      => %s%s,\n",
	    firstIn ? firstIn : "bfalse", firstIn ? "_first_take" : "",
	    firstIn ? firstIn : "bfalse", firstIn ? "_eof" : "",
	    worker().version() > 1 ? cname() : "bfalse", worker().version() > 1 ? "_eof" : "",
	    ::Port::m_worker->m_noControl ? "open" : "props_builtin_ocpi_latency_",
	    ::Port::m_worker->m_noControl ? "" : cname(),
#if 0
	    m_isUnbounded && !::Port::m_worker->m_noControl ?
	    "props_to_worker.ocpi_buffer_size_" : "(others => '0')",
	    m_isUnbounded && !::Port::m_worker->m_noControl ? cname() : ""
#else
	    !::Port::m_worker->m_noControl ?
	    "props_to_worker.ocpi_buffer_size_" : "(others => '0')",
	    !::Port::m_worker->m_noControl ? cname() : ""
#endif
	    );
  fprintf(f, "                reset            => %s_reset,\n", cname());
  fprintf(f, "                ready            => %s_ready,\n", cname());
  if (ocp.MData.value)
    fprintf(f,
	    "                som              => %s_som,\n"
	    "                eom              => %s_eom,\n"
	    "                valid            => %s_valid,\n"
	    "                data             => %s_data,\n", cname(), cname(), cname(), cname());
  else
    fprintf(f,
	    "                som              => %s,\n"
	    "                eom              => %s,\n"
	    "                valid            => %s,\n"
	    "                data             => %s,\n",
	    slave ? "open" : "btrue", slave ? "open" : "btrue",
	    slave ? "open" : "bfalse", slave ? "open" : "(others => '0')");
  if (m_abortable)
    fprintf(f, "                abort            => %s_abort,\n", cname());
  else
    fprintf(f, "                abort            => %s,\n", slave ? "open" : "'0'");
  if (ocp.MByteEn.value)
    fprintf(f, "                byte_enable      => %s_byte_enable,\n", cname());
  else
    fprintf(f, "                byte_enable      => open,\n");
  if (m_preciseBurst)
    fprintf(f, "                burst_length     => %s_burst_length,\n", cname());
  else if (slave)
    fprintf(f, "                burst_length     => open,\n");
  else
    fprintf(f, "                burst_length     => (%zu downto 0 => '0'),\n",
	    ocp.MBurstLength.width-1);
  if (ocp.MReqInfo.value)
    fprintf(f, "                opcode           => %s_opcode_temp,\n", cname());
  else if (slave)
    fprintf(f, "                opcode           => open,\n");
  else
    fprintf(f, "                opcode           => (%zu downto 0 => '0'),\n",
	    opcode_width-1);
  if (slave)
    fprintf(f, "                take             => %s_take", cname());
  else
    fprintf(f, "                give             => %s_give", cname());
  if (m_isPartitioned)
    fprintf(f,
	    ",\n"
	    "                part_size        => %s_part_size,\n"
	    "                part_offset      => %s_part_offset,\n"
	    "                part_start       => %s_part_start,\n"
	    "                part_ready       => %s_part_ready,\n"
	    "                part_%s        => %s_part_%s",
	    cname(), cname(), cname(), cname(),
	    slave ? "take" : "give", cname(), slave ? "take" : "give");
  fprintf(f, ");\n");
  if (!isDataProducer() && m_myClock && m_clock->m_output)
    fprintf(f, "  %s.Clk <= %s_Clk;\n", typeNameOut.c_str(), cname());
}

void WsiPort::
emitImplSignals(FILE *f) {
  //	  const char *tofrom = p->masterIn() ? "from" : "to";
  if (m_myClock && !isDataProducer() && m_clock->m_output)
    fprintf(f, "  signal %s_Clk : std_logic; -- this input port has its own output clock\n",
	    cname());
  fprintf(f,
	  "  signal %s_%s  : Bool_t;\n"
	  "  signal %s_ready : Bool_t;\n"
	  "  signal %s_reset : Bool_t; -- this port is being reset from the outside\n",
	  cname(), masterIn() ? "take" : "give", cname(), cname());
  if (masterIn())
    fprintf(f,
	    "  signal %s_first_take  : Bool_t;\n", cname());
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
  if (m_dataWidth)
    fprintf(f,
	    "  signal %s_som   : Bool_t;\n"
	    "  signal %s_eom   : Bool_t;\n"
	    "  signal %s_valid : Bool_t;\n", cname(), cname(), cname());
  fprintf(f, "  signal %s_eof   : Bool_t;\n", cname());
  if (m_isPartitioned)
    fprintf(f,
	    "  signal %s_part_size        : UShort_t;\n"
	    "  signal %s_part_offset      : UShort_t;\n"
	    "  signal %s_part_start       : Bool_t;\n"
	    "  signal %s_part_ready       : Bool_t;\n"
	    "  signal %s_part_%s        : Bool_t;\n",
	    cname(), cname(), cname(), cname(), cname(), masterIn() ? "take" : "give");
  if (!masterIn())
    // This temp/intermediate record signal is needed by xsim because some recent versions segfault
    // when putting output port signals that are in records, in the port map of internal instances
    fprintf(f,
            "  signal %s_temp : %s_t; -- temp needed to workaround Vivado/xsim bug v2016.4\n",
            typeNameOut.c_str(), typeNameOut.c_str());
}

void WsiPort::
emitVHDLShellPortClock(FILE *f, std::string &last) {
#if 0
    OcpPort::emitVHDLShellPortMap(f, last); // between ins and outs
#else
    // This is a bit clumsy, but its not clear how to do it better.
    // Basically the OCP logic deals with the clock signal, but for wsi, we have an intermediate
    // _temp signal to deal with...
    // ALERT:  this code is mostly a duplicate of the code in ocp.cxx
    if (m_myClock) {
      std::string
	&inout = m_clock->m_output ? typeNameOut : typeNameIn,
	rhs = inout;
      if (isDataProducer())
	rhs += m_clock->m_output ? "_temp." : ".";
      else if (m_clock->m_output)
	rhs = cname(), rhs += "_";
      else
	rhs += ".";
      fprintf(f, "%s    %s.Clk => %sClk", last.c_str(), inout.c_str(), rhs.c_str());
      last = ",\n";
    }
#endif
}

void WsiPort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "%s    %s_in.reset => %s_reset,\n"
	  "    %s_in.ready => %s_ready",
	  last.c_str(), cname(), cname(), cname(), cname());
  if (masterIn()) {
    if (m_dataWidth) {
      fprintf(f, ",\n    %s_in.data => %s_data", cname(), cname());
      if (ocp.MByteEn.value)
	fprintf(f, ",\n    %s_in.byte_enable => %s_byte_enable", cname(), cname());
    }
    if (m_nOpcodes > 1)
      fprintf(f, ",\n    %s_in.opcode => %s_opcode", cname(), cname());
    if (m_dataWidth)
      fprintf(f,
	      ",\n    %s_in.som => %s_som,\n"
	      "    %s_in.eom => %s_eom,\n"
	      "    %s_in.valid => %s_valid", 
	      cname(), cname(), cname(), cname(), cname(), cname());
    fprintf(f, ",\n    %s_in.eof => %s_eof", cname(), cname());
    if (m_isPartitioned)
      fprintf(f,
	      ",\n    %s_in.part_size   => %s_part_size,\n"
	      "    %s_in.part_offset => %s_part_offset,\n"
	      "    %s_in.part_start  => %s_part_start,\n"
	      "    %s_in.part_ready  => %s_part_ready",
	      cname(), cname(), cname(), cname(), cname(), cname(), cname(), cname());
    emitVHDLShellPortClock(f, last);
    fprintf(f, ",\n    %s_out.take => %s_take", cname(), cname());
    if (m_isPartitioned)
      fprintf(f, ",\n    %s_out.part_take  => %s_part_take", cname(), cname());
  } else {
    if (m_isPartitioned)
      fprintf(f, ",\n    %s_in.part_ready   => %s_part_ready", cname(), cname());
    emitVHDLShellPortClock(f, last);
    fprintf(f, ",\n    %s_out.give => %s_give", cname(), cname());
    if (m_dataWidth) {
      fprintf(f, ",\n    %s_out.data => %s_data", cname(), cname());
      if (ocp.MByteEn.value)
	fprintf(f, ",\n    %s_out.byte_enable => %s_byte_enable", cname(), cname());
    }
    if (ocp.MReqInfo.value)
      fprintf(f, ",\n    %s_out.opcode => %s_opcode", cname(), cname());
    if (m_dataWidth)
      fprintf(f,
	      ",\n    %s_out.som => %s_som,\n"
	      "    %s_out.eom => %s_eom,\n"
	      "    %s_out.valid => %s_valid",
	    cname(), cname(), cname(), cname(), cname(), cname());
    fprintf(f, ",\n    %s_out.eof => %s_eof", cname(), cname());
    if (m_isPartitioned)
      fprintf(f,
	      ",\n"
	      "    %s_out.part_size   => %s_part_size,\n"
	      "    %s_out.part_offset => %s_part_offset,\n"
	      "    %s_out.part_start  => %s_part_start,\n"
	      "    %s_out.part_give   => %s_part_give",
	      cname(), cname(), cname(), cname(), cname(), cname(), cname(), cname());
  }
  last = ",\n";
}

const char *WsiPort::
adjustConnection(::Port &consPort, const char *masterName, Language lang,
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
	ocpiCheck(asprintf((char **)&oa->expr,
			   lang == Verilog ?
			   "{%zu'b0,%%s}" : "std_logic_vector(to_unsigned(0,%zu)) & %%s",
			   cons.ocp.MBurstLength.width - 2) > 0);
	oa->comment = "Consumer only needs imprecise burstlength (2 bits)";
	oa->isExpr = true; // concatenation
      }
    }
  }
  if (m_preciseBurst && cons.m_preciseBurst &&
      ocp.MBurstLength.width < cons.ocp.MBurstLength.width) {
    oa = &consAdapt[OCP_MBurstLength];
    ocpiCheck(asprintf((char **)&oa->expr,
		       lang == Verilog ? "{%zu'b0,%%s}" : "to_unsigned(0,%zu) & %%s",
		       cons.ocp.MBurstLength.width - ocp.MBurstLength.width) > 0);
    oa->comment = "Consumer takes bigger bursts than producer creates";
    oa->other = OCP_MBurstLength;
  }
  // Abortable compatibility and adaptation
  if (cons.m_abortable) {
#if 0
    // The abort signal is now always present since it is eof too
    if (!m_abortable) {
      oa = &consAdapt[OCP_MDataInfo];
      oa->expr = lang == Verilog ? "{1'b0,%s}" : "\"0\" & %s";
      oa->comment = "Tell consumer no frames are ever aborted";
    }
#endif
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
  if (cons.m_nOpcodes != m_nOpcodes) {
    if (cons.ocp.MReqInfo.value) {
      if (ocp.MReqInfo.value) {
	if (cons.ocp.MReqInfo.width > ocp.MReqInfo.width) {
	  oa = &consAdapt[OCP_MReqInfo];
	  ocpiCheck(asprintf((char **)&oa->expr,
			     lang == Verilog ?
			     "{%zu'b0,%%s}" : "std_logic_vector(to_unsigned(0,%zu)) & %%s",
			     cons.ocp.MReqInfo.width - ocp.MReqInfo.width) > 0);
	  oa->isExpr = true;
	  oa->other = OCP_MReqInfo;
	} else {
	  // producer has more, we just connect the LSBs
	}
      } else {
	// producer has none, consumer has some
	oa = &consAdapt[OCP_MReqInfo];
	ocpiCheck(asprintf((char **)&oa->expr,
			   lang == Verilog ? "%zu'b0" : "std_logic_vector(to_unsigned(0,%zu))",
			   cons.ocp.MReqInfo.width) > 0);
      }
    } else {
      // consumer has none
      oa = &prodAdapt[OCP_MReqInfo];
      oa->expr = lang == Verilog ? "" : "open";
      oa->comment = "Consumer doesn't have opcodes (or has exactly one)";
      unused += ocp.MReqInfo.width;
    }
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
      ocpiCheck(asprintf((char **)&oa->expr, "{%zu{1'b1}}", cons.ocp.MByteEn.width) > 0);
  } else if (ocp.MByteEn.value) {
    // only producer has byte enables
    oa = &prodAdapt[OCP_MByteEn];
    oa->expr = lang == Verilog ? "" : "open";
    oa->comment = "consumer does not have byte enables";
    unused += ocp.MByteEn.width;
  }
  size_t
    cmdi = cons.ocp.MDataInfo.width - 1, // (cons.m_abortable ? 1 : 0),
    pmdi = ocp.MDataInfo.width - 1, // (m_abortable ? 1 : 0),
    pbytes = ocp.MByteEn.value ? ocp.MByteEn.width : 1,
    cbytes = cons.ocp.MByteEn.value ? cons.ocp.MByteEn.width : 1,
    pbs = (pmdi + ocp.MData.width) / pbytes,
    cbs = (cmdi + cons.ocp.MData.width) / cbytes;
  ocpiInfo("pbytes %zu, cbytes %zu, pbs %zu cbs %zu cmdi %zu pmdi %zu",
	   pbytes, cbytes, pbs, cbs, cmdi, pmdi);
  if (cons.ocp.MData.width + cmdi != ocp.MData.width + pmdi)
    return OU::esprintf("data widths do not match (output %zu/%zu input %zu/%zu)",
			ocp.MData.width, pmdi, cons.ocp.MData.width, cmdi);
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
	    comment, cname());
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
	  "  %s Aliases for interface \"%s\"\n", comment, cname());
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
  if (worker().m_language != VHDL && m_regRequest)
    fprintf(f,
	    "// GENERATED: OCP request phase signals for interface \"%s\" are registered\n",
	    cname());
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
	      cname());
      // This shouldn't be here when the width is 1, but some workers have been written this way
      // so we'll leave it, but it is redundant with valid when the width is 1
      if (ocp.MByteEn.value)
	fprintf(f,
		"    byte_enable      : std_logic_vector(ocpi_port_%s_MByteEn_width-1 downto 0);\n",
		cname());
    }
    if (m_nOpcodes > 1)
      fprintf(f,
	      "    opcode           : %s_OpCode_t;\n",
	      operations() ? OU::Protocol::cname() : pname());
    fprintf(f,
	    m_dataWidth ?
	    "    som, valid, eom, eof  : Bool_t;           -- valid means data and byte_enable are present\n" :
	    "    eof  : Bool_t;\n");
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
	      cname());
      // This should not really be generated when the width is one, but some workers set this
      // signal so we don't want to break them.  But it is ignored since "valid" is what is
      // documented and specified.
      if (ocp.MByteEn.value)
	fprintf(f,
		"    byte_enable      : std_logic_vector(ocpi_port_%s_MByteEn_width-1 downto 0);\n",
		cname());
    }
    if (m_nOpcodes > 1)
      fprintf(f,
	      "    opcode           : %s_OpCode_t;\n",
	      operations() ? OU::Protocol::cname() : pname());
    if (m_dataWidth)
      fprintf(f,
	      "    som, eom, valid  : Bool_t;       -- one or more must be true when 'give' is asserted\n");
    fprintf(f,
	    "    eof  : Bool_t;\n");
    if (m_isPartitioned)
      fprintf(f,
	      "    part_size        : UShort_t;\n"
	      "    part_offset      : UShort_t;\n"
	      "    part_start       : Bool_t;\n"
	      "    part_give        : Bool_t;\n");
  }
}

const char * WsiPort::
finalize() {
  const char *err;
  if ((err = DataPort::finalize()))
    return err;
  if (isDataProducer()) {
    // name         type,   debug  param  initl  volatl impl   builtin value enums
    AP(blocked,     ULong,  true,  false, false, true,  true,  true);    // cycles when output was blocked
    AP(max_latency, UShort, false, true,  false, false, true,  false,  256); // maximum input-to-output latency 
    AP(latency,     UShort, false, false, false, true,  true,  true);   // measured latency
  }
  return NULL;
}


#if 0
unsigned WsiPort::
extraDataInfo() const {
  return m_abortable ? 1 : 0;
}
#endif
