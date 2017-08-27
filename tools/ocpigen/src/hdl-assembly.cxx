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

// This file contains assembly support, which is almost entirely for HDL,
// but just a little also for RCC and OCL.  FIXME: factor non-HDL assy stuff

#include <cstdio>
#include <cstring>
#include <cassert>
#include "OcpiUtilMisc.h"
#include "data.h"
#include "assembly.h"
#include "hdl.h"
#include "hdl-platform.h"

namespace OX = OCPI::Util::EzXml;

const char *Instance::
initHDL(::Assembly &assy) {
  const char *err;
  if (!m_worker->m_noControl)
      assy.m_nWCIs += m_worker->m_ports[0]->m_count;
  if (assy.m_assyWorker.m_type == Worker::Container || 
      assy.m_assyWorker.m_type == Worker::Configuration) {
    if ((err = OE::getBoolean(m_xml, "emulated", &m_emulated)))
      return err;
    const char
      *ic = ezxml_cattr(m_xml, "interconnect"), // which interconnect
      *ad = ezxml_cattr(m_xml, "adapter"),      // adapter to which interconnect or io
      *io = ezxml_cattr(m_xml, "device");       // which device
    if ((err = OE::getNumber(m_xml, "configure", &m_config, &m_hasConfig, 0)))
      return OU::esprintf("Invalid configuration value for adapter: %s", err);
    if (ic) {
      if (io)
	return "Container workers cannot be both IO and Interconnect";
      m_attach = ic;
      m_iType = Instance::Interconnect;
    } else if (io) {
      m_attach = io;
      assert(m_iType == Instance::Device);
    } else if (ad) {
      assert(m_iType == Instance::Application);
      m_iType = Instance::Adapter;
      m_attach = ad; // an adapter is for an interconnect or I/O
    }
  }
  return NULL;
}

const char *Assembly::
insertAdapter(Connection &c, InstancePort &from, InstancePort &to) {
  DataPort
    &dpFrom = *static_cast<DataPort *>(from.m_port),
    &dpTo = *static_cast<DataPort *>(to.m_port);
  if (dpFrom.m_dataWidth == dpTo.m_dataWidth)
    return NULL;
  // 1. Create the adapter instance and its ports
  m_instances.resize(m_instances.size()+1);
  Instance &i = m_instances.back();
  i.m_inserted = true;
  // We create an OU::AssemblyInstance to feed the rest of the code.
  std::string name;
  OU::format(name, "%s_%s_2_%s_%s", from.m_instance->cname(), dpFrom.pname(),
	     to.m_instance->cname(), dpTo.pname());
  // Add the width parameters for the adapter
  OU::Assembly::Properties props;
  props.resize(2);
  std::string s;
  OU::format(s, "%zu", dpFrom.m_dataWidth);
  props[0].setValue("width_in", s.c_str());
  OU::format(s, "%zu", dpTo.m_dataWidth);
  props[1].setValue("width_out", s.c_str());
  const char *err;
  if ((err = i.init(*this, name.c_str(), "wsi_width_adapter", NULL, props)) ||
      (err = i.initHDL(*this)))
    return err;
  // 2. Create the a2c (adapter to consumer) connection and attach it to both
  // 2a.Detach the consumer port from the original connection
  assert(to.m_attachments.size() == 1);
  to.detach(c);
  // 2b.Create the new a2c connection
  Connection &a2c = *new Connection(NULL, NULL);
  a2c.m_count = 1;
  m_connections.push_back(&a2c);
  // 2c.Attach the adapter and the consumer to the new connection
  bool hasControl = i.m_ports[0].m_port->m_type == WCIPort;
  InstancePort
    &in = i.m_ports[hasControl ? 1 : 0],
    &out = i.m_ports[hasControl ? 2 : 1];
  assert(!strcmp("in", in.m_port->pname()) || !strcmp("out", out.m_port->pname()));
  if ((err = a2c.attachPort(out, 0)) || // output from adapter
      (err = a2c.attachPort(to, 0)))
    return err;
  // 3. Attach the adapter as consumer of original connection
  if ((err = c.attachPort(in, 0)))
    return err;
  return NULL;
}

const char *Worker::
parseHdlAssy() {
  const char *err;
  if ((err = addBuiltinProperties()))
    return err;
  if (strcasecmp(OX::ezxml_tag(m_xml), "HdlAssembly") &&
      (err =
       addProperty("<property name='sdp_width' type='uchar' parameter='true' default='1'/>",
		   true)))
    return err;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    m_ctl.summarizeAccess(**pi);
  ::Assembly *a = m_assembly = new ::Assembly(*this);
  
  static const char
    *topAttrs[] = {IMPL_ATTRS, HDL_TOP_ATTRS, HDL_IMPL_ATTRS, NULL},
    // FIXME: reduce to those that are hdl specific
    *instAttrs[] =  { INST_ATTRS },
    *contInstAttrs[] = { "Index", "interconnect", "io", "adapter", "configure", "emulated", NULL},
    *platInstAttrs[] = { "Index", "interconnect", "io", "adapter", "configure", NULL};
  // Do the generic assembly parsing, then to more specific to HDL
  if ((err = a->parseAssy(m_xml, topAttrs,
			  m_type == Container ? contInstAttrs :
			  (m_type == Configuration ? platInstAttrs : instAttrs),
			  true)))
    return err;
  // Do the OCP derivation for all workers
  for (WorkersIter wi = a->m_workers.begin(); wi != a->m_workers.end(); wi++)
    if ((err = (*wi)->deriveOCP()))
      return err;
  // Do the HDL-specific parsing and initializations for the instances in the assembly
  Instance *i = &a->m_instances[0];
  for (unsigned n = 0; n < a->m_instances.size(); n++, i++)
    if ((err = i->initHDL(*a)))
      return err;
  i = &a->m_instances[0];
  for (unsigned n = 0; n < a->m_instances.size(); n++, i++)
    if (i->m_worker->m_emulate) {
      Instance *ii = &a->m_instances[0];
      for (unsigned nn = 0; nn < a->m_instances.size(); nn++, ii++)
	if (!strcasecmp(ii->m_worker->m_implName, i->m_worker->m_emulate->m_implName))
	  ii->m_emulated = true;
    }
  // Look at connections for inserting adapters between data ports
  for (ConnectionsIter ci = a->m_connections.begin(); ci != a->m_connections.end(); ci++) {
    Connection &c = **ci;
    InstancePort *from = NULL, *to = NULL;
    for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
      Attachment &at = **ai;
      InstancePort &ip = at.m_instPort;
      if (!ip.m_external && ip.m_port->isData()) {
	if (ip.m_port->isDataProducer()) {
	  assert(from == NULL);
	  from = &ip;
	} else if (!ip.m_port->isDataBidirectional()) {
	  assert(to == NULL);
	  to = &ip;
	}
      }
    }
    if (from && to && (err = a->insertAdapter(c, *from, *to)))
      return err;
  }
  Port *wci = NULL;
  unsigned n;
  Clock *wciClk = NULL;
  // Establish the wciClk for all wci slaves
  if (m_type == Container) {
    // The default WCI clock comes from the (single) wci master
    for (n = 0, i = &a->m_instances[0]; !wciClk && n < a->m_instances.size(); n++, i++)
      if (i->m_worker) {
	unsigned nn = 0;
	for (InstancePort *ip = &i->m_ports[0]; nn < i->m_worker->m_ports.size(); nn++, ip++) 
	  if (ip->m_port->m_type == WCIPort && ip->m_port->m_master) {
	    // Found the instance that is mastering the control plane
	    wciClk = addClock();
	    // FIXME:  this should access the 0th clock more specifically for VHDL
	    wciClk->m_name = "wciClk";
	    OU::format(wciClk->m_signal, "%s_%s_out_i(0).Clk", i->cname(), ip->m_port->pname());
	    OU::format(wciClk->m_reset, "%s_%s_out_i(0).MReset_n", i->cname(), ip->m_port->pname());
	    wciClk->assembly = true;
	    if (i->m_clocks)
	      i->m_clocks[ip->m_port->clock->ordinal] = wciClk;
	    break;
	  }
      }
  } else if (a->m_nWCIs) {
    //    assert(m_ports.size() == 0);
    char *cp;
    ocpiCheck(asprintf(&cp, "<control name='wci' count='%zu'>", a->m_nWCIs) > 0);
    ezxml_t x = ezxml_parse_str(cp, strlen(cp));
    // Create the assy's wci slave port, at the beginning of the list
    wci = createPort<WciPort>(*this, x, NULL, -1, err);
    assert(wci);
    // Clocks: coalesce all WCI clock and clocks with same reqts, into one wci, all for the assy
    Clock *clk = addClock();
    // FIXME:  this should access the 0th clock more specifically for VHDL
    clk->m_signal =
      clk->m_name =
      a->m_nWCIs > 1 ? (m_language == VHDL ? "wci_in(0).Clk" : "wci0_Clk") : "wci_Clk";
    clk->m_reset =
      a->m_nWCIs > 1 ?
      (m_language == VHDL ? "wci_in(0).MReset_n" : "wci0_MReset_n") : "wci_MReset_n";
    clk->port = wci;
    wci->myClock = true;
    wci->clock = clk;
    wci->clock->port = wci;
    OU::Assembly::External *ext = new OU::Assembly::External;
    ext->m_name = "wci";
    ext->m_role.m_provider = false; // provisional
    ext->m_role.m_bidirectional = false;
    ext->m_role.m_knownRole = true;
    InstancePort &ip = *new InstancePort(NULL, wci, ext);
    unsigned nControl = 0;
    for (n = 0, i = &a->m_instances[0]; n < a->m_instances.size(); n++, i++)
      if (i->m_worker && i->m_worker->m_ports[0]->m_type == WCIPort && !i->m_worker->m_noControl) {
	std::string l_name;
	OU::format(l_name, "wci%u", nControl);
	Connection &c = *new Connection(NULL, l_name.c_str());
	c.m_count = 1;
	a->m_connections.push_back(&c);
	if ((err = c.attachPort(i->m_ports[0], 0)) ||
	    (err = c.attachPort(ip, nControl)))
	  return err;
	nControl++;
      }
    wciClk = wci->clock;
  }
  // Map all the wci slave clocks to the assy's wci clock
  for (n = 0, i = &a->m_instances[0]; n < a->m_instances.size(); n++, i++)
    // Map the instance's WCI clock to the assembly's WCI clock if it has a wci port
    if (i->m_worker && i->m_worker->m_wciClock && !i->m_worker->m_assembly)
      i->m_clocks[i->m_worker->m_wciClock->ordinal] = wciClk;
#if 0
    if (i->m_worker && i->worker->m_ports[0]->m_type == WCIPort &&
	!i->m_worker->m_ports[0]->m_master && !i->m_worker->m_assembly)
      i->m_clocks[i->m_worker->m_ports[0]->clock->ordinal] = wciClk;
#endif

  // Assign the wci clock to connections where we can
  if (wciClk)
    for (ConnectionsIter ci = m_assembly->m_connections.begin();
	 ci != m_assembly->m_connections.end(); ++ci) {
      Connection &c = **ci;
      Attachment *at = NULL;
      for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	InstancePort &ip = (**ai).m_instPort;
	if (!ip.m_external && 
	    (ip.m_port->m_type == WCIPort ||
	     // FIXME: how can we really know this is not an independent clock??
	     (ip.m_port->worker().m_noControl && ip.m_port->clock && ip.m_port->clock->port == 0) ||
	     (ip.m_instance->m_worker->m_ports[0]->m_type == WCIPort &&
	      !ip.m_instance->m_worker->m_ports[0]->m_master &&
	      // If this (data) port on the worker uses the worker's wci clock
	      ip.m_port->isData() && ip.m_port->clock &&
	      ip.m_port->clock == ip.m_instance->m_worker->m_ports[0]->clock))) {
	  at = *ai;
	  break;
	}
      }
      if (at) {
	// So this case is when some data port on the connection uses the wci clock
	// So we assign the wci clock as the clock for this whole connection
	c.m_clock = wciClk;
	if (c.m_external)
	  c.m_external->m_instPort.m_port->clock = c.m_clock;
	// And force the clock for each connected port to BE the wci clock
	// This will potentialy connect an independent clock on a worker to the wci clock
	for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	  if (!(**ai).m_instPort.m_external) {
	    InstancePort &ip = (**ai).m_instPort;
	    // FIXME: check for compatible clock constraints? insert synchronizer?
	    if (ip.m_instance->m_clocks)
	      ip.m_instance->m_clocks[ip.m_port->clock->ordinal] = wciClk;
	  }
	}
      }
    }

  // Deal with all the internal connection clocks that are not WCI clocks
  for (ConnectionsIter ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
      Attachment &at = **ai;
      InstancePort &ip = at.m_instPort;
      if (!ip.m_external && ip.m_port->isData() && ip.m_instance->m_clocks) {
        size_t nc = ip.m_port->clock->ordinal;
        if (!c.m_clock) {
          // This connection doesn't have a clock yet,
          // so its not using the WCI clock either
          if (ip.m_instance->m_clocks[nc])
            // The clock of the port is already mapped, so we just use it.
            c.m_clock = ip.m_instance->m_clocks[nc];
          else {
            // The connection has no clock, and the port's clock is not mapped.
            // We need a new top level clock.
	    Clock *clk = addClock();
            if (ip.m_port->clock->port) {
              // This clock is owned by a port, so it is a "port clock". So name it
              // after the connection (and external port).
	      OU::format(clk->m_name, "%s_Clk", c.m_name.c_str());
              clk->m_signal = clk->m_name;
              clk->port = c.m_external->m_instPort.m_port;
              c.m_external->m_instPort.m_port->myClock = true;
            } else {
              // This port's clock is a separately defined clock
              // We might as well keep the name since we have none better
              clk->m_name = ip.m_port->clock->m_name;
              clk->m_signal = ip.m_port->clock->m_signal;
            }
            clk->assembly = true;
            c.m_clock = clk;
            ip.m_instance->m_clocks[nc] = c.m_clock;
            // FIXME inherit ip->port->clock constraints
          }
        } else if (ip.m_instance->m_clocks[nc]) {
          // This port already has a mapped clock
          if (ip.m_instance->m_clocks[nc] != c.m_clock)
            return OU::esprintf("Connection %s at interface %s of instance %s has clock conflict",
				c.m_name.c_str(), ip.m_port->pname(), ip.m_instance->cname());
        } else {
          // FIXME CHECK COMPATIBILITY OF c->clock with ip->port->clock
          ip.m_instance->m_clocks[nc] = c.m_clock;
        }
      }
    }
  }
  bool cantDataResetWhileSuspended = false;
  for (n = 0, i = &a->m_instances[0]; n < a->m_instances.size(); n++, i++)
    if (i->m_worker && !i->m_worker->m_assembly) {
      unsigned nn = 0;
      for (InstancePort *ip = &i->m_ports[0]; nn < i->m_worker->m_ports.size(); nn++, ip++) 
	if (ip->m_port->isData()) {
	  size_t nc = ip->m_port->clock->ordinal;
	  if (!i->m_clocks[nc]) {
	    if (ip->m_port->m_type == WSIPort || ip->m_port->m_type == WMIPort)
	      return OU::esprintf("Unconnected data interface %s of instance %s has its own clock",
				  ip->m_port->pname(), i->cname());
	    Clock *clk = addClock();
	    i->m_clocks[nc] = clk;
	    OU::format(clk->m_name, "%s_%s", i->cname(), ip->m_port->clock->cname());
	    if (ip->m_port->clock->m_signal.size())
	      OU::format(clk->m_signal, "%s_%s", i->cname(), ip->m_port->clock->signal());
	    clk->assembly = true;
	  }
	}
    }

  // Now all clocks are done.  We process the non-data external ports.
  // Now all data ports that are connected have mapped clocks and
  // all ports with WCI clocks are connected.  All that's left is
  // WCI: WTI, WMemI, and the platform ports
  //  size_t nWti = 0, nWmemi = 0;
  for (n = 0, i = &a->m_instances[0]; n < a->m_instances.size(); n++, i++) {
    assert(i->m_worker);
    Worker *iw = i->m_worker;
    unsigned nn = 0;
    for (InstancePort *ip = &i->m_ports[0]; nn < iw->m_ports.size(); nn++, ip++)
      if ((err = ip->m_port->finalizeExternal(*this, *iw, *ip, cantDataResetWhileSuspended)))
	return err;
  }
  if (!cantDataResetWhileSuspended && wci)
    m_wci->setResetWhileSuspended(true);
  // Process the external data ports on the assembly worker
  for (ConnectionsIter ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (c.m_nExternals)
      for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	Attachment &at = **ai;
        if (at.m_instPort.m_external) {
          Port *p = at.m_instPort.m_port;
	  assert(p->clock == c.m_clock);
          //p->clock = c.m_clock;
          if (p->clock && p->clock->port && p->clock->port != p)
            p->clockPort = p->clock->port;
        }
      }
  }
  return 0;
}


Attachment::
Attachment(InstancePort &ip, Connection &c, size_t index)
  : m_instPort(ip), m_connection(c), m_index(index)
{
}
Connection::
Connection(OU::Assembly::Connection *connection, const char *name)
  : m_name(name ? name : (connection ? connection->m_name.c_str() : "")),
    m_nExternals(0), m_clock(NULL), m_external(NULL),
    m_count(connection ? connection->m_count : 0) {
}

void InstancePort::
emitConnectionSignal(FILE *f, bool output, Language lang) {
  std::string signal = m_instance->m_name;
  signal += '_';
  OU::formatAdd(signal,
		output ?
		(lang == VHDL ? m_port->typeNameOut.c_str() : m_port->fullNameOut.c_str()) :
		(lang == VHDL ? m_port->typeNameIn.c_str() : m_port->fullNameIn.c_str()), "");
  signal += "_i"; // Use this to avoid colliding with port signals
  m_port->emitConnectionSignal(f, output, lang, signal);
  (output ? m_signalOut : m_signalIn) = signal;
}

// An instance port that is internal needs to be bound to ONE input and ONE output signal bundle,
// even when it has multiple attachments, some of which might be external.
// Signals are established as the output of an instance port, except in a few cases where it is
// unneeded (directly connected to an external port, or to a bundled port).
// Signals are established for the input in some rare cases.
// This function defines the internal signals, but doesn't bind to any yet.
const char *InstancePort::
createConnectionSignals(FILE *f, Language lang, size_t &unused) {
  const char *err;
  // Find out the widest of all ports related to this instance port
  size_t maxCount = 0;
  for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
    Connection &c = (*ai)->m_connection;
    for (AttachmentsIter cai = c.m_attachments.begin(); cai != c.m_attachments.end(); cai++)
      if (&(*cai)->m_instPort != this && (*cai)->m_instPort.m_port->m_count > maxCount)
	maxCount = (*cai)->m_instPort.m_port->m_count;
  }
  // Output side: generate signal except when external or connected only to external
  // Or when there is a wider one
  if (m_attachments.size() &&
      !(m_attachments.size() == 1 &&
	m_attachments.front()->m_connection.m_attachments.size() == 2 &&
	m_attachments.front()->m_connection.m_external) &&
      maxCount <= m_port->m_count &&
      (m_port->m_type != TimePort || m_port->m_master))
    {
    emitConnectionSignal(f, true, lang);
    // All connections should use this as their signal
    for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
      Connection &c = (*ai)->m_connection;
      std::string &cName = m_port->m_master ? c.m_masterName : c.m_slaveName;
      assert(cName.empty());
      cName = m_signalOut;
    }
  }
    
  // Input side: rare - generate signal when it aggregates members from others,
  // Like a WSI slave port array
  if (m_port->m_count > 1 &&
      ((maxCount && maxCount < m_port->m_count) || m_attachments.size() > 1)) {
    emitConnectionSignal(f, false, lang);
    for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
      Connection &c = (*ai)->m_connection;
      std::string &cName = m_port->m_master ? c.m_slaveName : c.m_masterName;
      assert(cName.empty());
      cName = m_signalIn;
    }
  }
  if (m_port->isData())
    for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
      Connection &c = (*ai)->m_connection;
      for (AttachmentsIter cai = c.m_attachments.begin(); cai != c.m_attachments.end(); cai++) {
	InstancePort &other = (*cai)->m_instPort;
	if (&other != this) {
	  err = m_port->isDataProducer() ?
	    DataPort::
	    adjustConnection(c.m_masterName.c_str(), *m_port, m_ocp, m_hasExprs, *other.m_port,
			     other.m_ocp, other.m_hasExprs, lang, unused) :
	    DataPort::
	    adjustConnection(c.m_masterName.c_str(), *other.m_port, other.m_ocp,
			     other.m_hasExprs, *m_port, m_ocp, m_hasExprs, lang, unused);
	  if (err)
	    return OU::esprintf("For connection between %s/%s and %s/%s: %s",
				m_port->worker().m_implName, m_port->pname(),
				other.m_port->worker().m_implName, other.m_port->pname(), err);
	}
      }
    }
  if (m_hasExprs) {
    assert(m_signalIn.empty());
    emitConnectionSignal(f, false, lang);
  }
  return NULL;
}

void
doPrev(FILE *f, std::string &last, std::string &comment, const char *myComment) {
  if (last.size()) {
    fputs(last.c_str(), f);
    if (comment.size())
      fprintf(f, " %s %s", myComment, comment.c_str());
    fputs("\n", f);
  }
  last = ",";
  comment = "";
}

// single means that this is a signal that is not a vector at all.
// isSingle means that the signal is mapped an individual part of a vector.
static void
mapOneSignal(FILE *f, Signal &s, unsigned n, bool isSingle, const char *mapped,
	     const char *indent, const char *pattern, bool single) {
  std::string name, map;
  OU::format(name, pattern, s.cname());
  if (*mapped)
    OU::format(map, pattern, mapped);
  if (s.m_width && isSingle && !single)
    OU::formatAdd(name, "(%u)", n);
  fprintf(f, "%s%s => %s", indent, name.c_str(),
	  *mapped ? map.c_str() : (s.m_direction == Signal::IN ?
				   (isSingle || !s.m_width || single ? "'0'" : "(others => '0')") :
				   "open"));
}

void Assembly::
emitAssyInstance(FILE *f, Instance *i) { // , unsigned nControlInstances) {
  // Before we emit the instantiation, we first emit any tieoff assignments related to
  // unconnected parts (indices) of the intermiediate connection signal
  InstancePort *ip = &i->m_ports[0];
  for (unsigned n = 0; n < i->m_worker->m_ports.size(); n++, ip++)
    ip->emitTieoffAssignments(f);
  Language lang = m_assyWorker.m_language;
  std::string suff;
  i->m_worker->addParamConfigSuffix(suff);
  std::string pkg, tpkg;
  if (lang == Verilog)
    fprintf(f, "\n%s%s", i->m_worker->m_implName, suff.c_str());
  else {
    OU::format(pkg, "%s%s.%s_defs", i->m_worker->m_library, suff.c_str(), i->m_worker->m_implName);
    OU::format(tpkg, "%s%s.%s_constants", i->m_worker->m_library, suff.c_str(), i->m_worker->m_implName);
    fprintf(f, "\n  %s_i : component %s.%s_rv%s\n",
	    i->cname(), pkg.c_str(), i->m_worker->m_implName, suff.c_str());
  }
  bool any = false;
  if (i->m_properties.size()) {
    unsigned n = 0;
    // Emit the compile-time properties (a.k.a. parameter properties).
    for (InstanceProperty *pv = &i->m_properties[0]; n < i->m_properties.size(); n++, pv++) {
      const OU::Property *pr = pv->property;
      if (pr->m_isParameter) {
	std::string value;
	if (lang == VHDL) {
	  fprintf(f, any ? ",\n              "  : "  generic map(");
	  fprintf(f, "%s => %s", pr->m_name.c_str(),
		  vhdlValue(!strcasecmp(pr->m_name.c_str(), "ocpi_endian") ?
			    "ocpi.types" : tpkg.c_str(),
			    pr->m_name, pv->value, value));
	} else {
	  fprintf(f, "%s", any ? ", " : " #(");
#if 0
	  int64_t i64 = 0;
	  switch (pr->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)	\
	    case OA::OCPI_##pretty:			\
	      i64 = (int64_t)pv->value.m_##pretty;	\
	      break;
	    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	  default:;
	  }
	  size_t bits =
	    pr->m_baseType == OA::OCPI_Bool ?
	    1 : (pr->m_baseType == OA::OCPI_Enum ? rawBitWidth(*pr) : pr->m_nBits);
	  fprintf(f, ".%s(%zu'd%lld)",
		  pr->m_name.c_str(), bits, (long long)i64);
#endif
	  fprintf(f, ".%s(%s)",
		  pr->m_name.c_str(), verilogValue(pv->value, value));
	}
	any = true;
      }
    }
    if (any)
      fprintf(f, ")\n"); // both languages
  }
  if (lang == Verilog)
    fprintf(f, " %s (\n",i->cname());
  else
    fprintf(f, "    port map(   ");
  any = false;
  const char *indent = "              ";
  // For the instance, define the clock signals that are defined separate from
  // any interface/port.
  for (ClocksIter ci = i->m_worker->m_clocks.begin(); ci != i->m_worker->m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!c->port) {
      if (lang == Verilog) {
	if (i->m_clocks[c->ordinal])
	  fprintf(f, "%s  .%s(%s)", any ? ",\n" : "", c->signal(),
		  i->m_clocks[c->ordinal]->signal());
	if (c->m_reset.size())
	  fprintf(f, ",\n  .%s(%s)", c->reset(),
		  i->m_clocks[c->ordinal]->reset());
      } else if (i->m_clocks[c->ordinal]) {
	fprintf(f, "%s%s%s => %s", any ? ",\n" : "", any ? indent : "",
		c->signal(), i->m_clocks[c->ordinal]->signal());
	if (c->m_reset.size())
	  fprintf(f, ",\n%s%s => %s", indent, c->reset(),
		  i->m_clocks[c->ordinal]->reset());
      } else {
	fprintf(f, "%s%s%s => '0'", any ? ",\n" : "", any ? indent : "",
		c->signal());
	if (c->m_reset.size())
	  fprintf(f, ",\n%s%s => '1'", indent, c->reset());
      }
      any = true;
    }
  }
  std::string last(any ? "," : "");
  std::string comment;
  std::string exprs;
  ip = &i->m_ports[0];
  for (unsigned n = 0; n < i->m_worker->m_ports.size(); n++, ip++) {
    // We can't do this since we need the opportunity of stubbing unconnected ports properly
    //    if (ip->m_attachments.empty())
    //      continue;
    ip->m_port->emitPortSignals(f, ip->m_attachments, lang, indent, any, comment, last,
				myComment(), ip->m_ocp,
				ip->m_hasExprs ? &ip->m_signalIn : NULL, exprs);
    any = true;
  } // end of port loop
  // First we need to figure out whether this is an emulator worker or a worker that
  // has a paired emulator worker.
  Instance *emulated = NULL, *ii = &m_instances[0];
  if (i->m_worker->m_emulate)
    for (unsigned n = 0; n < m_instances.size(); n++, ii++)
      if (!strcasecmp(ii->m_worker->m_implName, i->m_worker->m_emulate->m_implName)) {
	emulated = ii;
	break;
      }
  // Instance signals are connected to external ports unless they are connected to an emulator,
  // and sometimes they are mapped to slot names, and sometimes they are mapped to nothing when
  // the platform doesn't support the signal.  Also, if they are tristate, they may be
  // connected to an internal signal that is attached to the tristate buffer instanced in the
  // container.
  std::string prefix;
  if (i->m_worker->m_isDevice && i->m_worker->m_type != Worker::Platform) {
    if (emulated)
      prefix = emulated->m_name;
    else
      prefix = i->cname();
    prefix += "_";
  }
  for (SignalsIter si = i->m_worker->m_signals.begin(); si != i->m_worker->m_signals.end(); si++) {
    Signal &s = **si;
    bool anyMapped = false;
    std::string name;
    // Allow for signals in a vector to be mapped individually (e.g. to slot signal).
    for (unsigned n = 0; s.m_width ? n < s.m_width : n == 0; n++) {
      bool isSingle = false;
      const char *mappedExt = i->m_extmap.findSignal(s, n, isSingle);
      ocpiDebug("Instance %s worker %s signal %s(%u) mapped to %s single %d", i->cname(),
		i->m_worker->m_implName, s.cname(), n, mappedExt ? mappedExt : "<none>", isSingle);
      if (mappedExt) {
	// mappedExt might actually be an empty string: ""
	if (!anyMapped)
	  assert(n == 0);
	any = true;
	anyMapped = true;
	const char *front = any ? indent : "";
	if (s.m_differential) {
	  doPrev(f, last, comment, myComment());
	  mapOneSignal(f, s, n, isSingle, mappedExt, front, s.m_pos.c_str(), false);
	  doPrev(f, last, comment, myComment());
	  mapOneSignal(f, s, n, isSingle, mappedExt, front, s.m_neg.c_str(), false);
	} else if (s.m_direction == Signal::INOUT || s.m_direction == Signal::OUTIN) {
	  // For inout, we only want to map the signals if they are NOT connected,
	  if (!*mappedExt) {
	    doPrev(f, last, comment, myComment());
	    mapOneSignal(f, s, n, isSingle, mappedExt, front, s.m_in.c_str(), false);
	    doPrev(f, last, comment, myComment());
	    mapOneSignal(f, s, n, isSingle, mappedExt, front, s.m_out.c_str(), false);
	    doPrev(f, last, comment, myComment());
	    mapOneSignal(f, s, n, isSingle, mappedExt, front, s.m_oe.c_str(), true);
	  }
	} else {
	  doPrev(f, last, comment, myComment());
	  mapOneSignal(f, s, n, isSingle, mappedExt, front, "%s", false);
	}
	if (*mappedExt) {
	  Signal *es = m_assyWorker.m_sigmap[mappedExt];
	  assert(es);
	  m_assyWorker.recordSignalConnection(*es, (prefix + s.cname()).c_str());
	}
	if (!isSingle)
	  break;
      }	else
	assert(!anyMapped);
    }
    if (anyMapped && (s.m_direction != Signal::INOUT && s.m_direction != Signal::OUTIN))
      continue;
    doPrev(f, last, comment, myComment());
    if (s.m_differential) {
      OU::format(name, s.m_pos.c_str(), s.cname());
      if (lang == VHDL)
	fprintf(f, "%s%s => %s%s,\n", any ? indent : "",
		name.c_str(), prefix.c_str(), name.c_str());
      OU::format(name, s.m_neg.c_str(), s.cname());
      if (lang == VHDL)
	fprintf(f, "%s%s => %s%s", any ? indent : "",
		name.c_str(), prefix.c_str(), name.c_str());
    } else if (s.m_direction == Signal::INOUT || s.m_direction == Signal::OUTIN) {
      OU::format(name, s.m_in.c_str(), s.cname());
      fprintf(f, "%s%s => %s%s,\n", any ? indent : "",
	      name.c_str(), prefix.c_str(), name.c_str());
      OU::format(name, s.m_out.c_str(), s.cname());
      fprintf(f, "%s%s => %s%s,\n", any ? indent : "",
	      name.c_str(), prefix.c_str(), name.c_str());
      OU::format(name, s.m_oe.c_str(), s.cname());
      fprintf(f, "%s%s => %s%s", any ? indent : "",
	      name.c_str(), prefix.c_str(), name.c_str());
    } else if (lang == VHDL)
      fprintf(f, "%s%s => %s%s", any ? indent : "",
	      s.cname(), prefix.c_str(), s.cname());
    else
      fprintf(f, "%s.%s(%s%s)", any ? indent : "",
	      s.cname(), prefix.c_str(), s.cname());
    if (!i->m_emulated && !i->m_worker->m_emulate && !anyMapped) {
      Signal *es = m_assyWorker.m_sigmap[(prefix + s.cname()).c_str()];
      if (!es)
	ocpiInfo("Signal %s of worker %s not mapped to slot", s.cname(), i->m_worker->m_implName);
      else
	m_assyWorker.recordSignalConnection(*es, (prefix + s.cname()).c_str());
    }
  }
  fprintf(f, ");%s%s\n", comment.size() ? " // " : "", comment.c_str());
  // Emit any deferred temporary assigments for signals that have expressions.
  fputs(exprs.c_str(), f);
  // Now we must tie off any outputs that are generically expected, but are not
  // part of the worker's interface
  if (i->m_worker->m_wci) {
    if (!i->m_worker->m_wci->ocp.SData.value) {
      ip = &i->m_ports[i->m_worker->m_wci->m_ordinal];
      assert(ip->m_attachments.size() == 1);
      Connection &c = ip->m_attachments.front()->m_connection;
      assert(c.m_attachments.size() == 2);
      InstancePort *otherIp = NULL;
      Attachment *at;
      for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++)
	if (&(*ai)->m_instPort != ip) {
	  at = *ai;
	  otherIp = &at->m_instPort;
	  break;
	}
      assert(otherIp);
      std::string externalName, num;
      OU::format(num, "%zu", at->m_index);
      OU::format(externalName, ip->m_attachments.front()->m_connection.m_slaveName.c_str(), num.c_str());
      if (lang == Verilog)
	fprintf(f, "assign %sSData = 32'b0;\n", externalName.c_str());
      else
	fprintf(f, "%s(%zu).SData <= (others => '0');\n", externalName.c_str(), at->m_index);
    }
  }
}

 static void
assignExt(FILE *f, Connection &c, std::string &intName, bool in2ext) {
  assert(c.m_attachments.size() == 2);
  Port &p = *c.m_external->m_instPort.m_port;
  Attachment
    &extAt = *c.m_external,
    &intAt = *(c.m_attachments.front() == c.m_external ?
	       c.m_attachments.back() : c.m_attachments.front());
  std::string extName;
  OU::format(extName, in2ext ? p.typeNameOut.c_str() : p.typeNameIn.c_str(), "");

  p.emitExtAssignment(f, in2ext, extName, intName, extAt, intAt, c.m_count);
}

const char *Worker::
emitAssyHDL() {
  FILE *f;
  const char *err = openSkelHDL(ASSY, f);
  if (err)
    return err;
  fprintf(f,
	  "%s This confile contains the generated assembly implementation for worker: %s\n\n",
	  myComment(), m_implName);
  if (m_language == Verilog) {
    fprintf(f,
	    "// This confile contains the generated assembly implementation for worker: %s\n\n"
	    // "`default_nettype none\n" FIXME sure would be nice, but its a "wire" vs "reg" thing.
	    "`define NOT_EMPTY_%s // suppress the \"endmodule\" in %s%s%s\n"
	    "`include \"%s%s%s\"\n\n",
	    m_implName, m_implName, m_implName, DEFS, VERH, m_implName, DEFS, VERH);
  } else {
    fprintf(f,
	    "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;\n"
	    "library ocpi, util;\n");
    emitVhdlLibraries(f);
    fprintf(f,
	    "architecture rtl of %s_rv is\n",
	    m_implName);
  }
  fprintf(f, "%s Define signals for connections that are not externalized\n\n", myComment());
  if (m_language == Verilog)
    fprintf(f, "wire [255:0] nowhere; // for passing output ports\n");
  // Generate the intermediate signals for internal connections
  Instance *i = &m_assembly->m_instances[0];
  size_t unused = 0;
  for (unsigned n = 0; n < m_assembly->m_instances.size(); n++, i++) {
    for (unsigned nn = 0; nn < i->m_worker->m_ports.size(); nn++) {
      InstancePort &ip = i->m_ports[nn];
      assert(!ip.m_external);
      if (!ip.m_external && (err = ip.createConnectionSignals(f, m_language, unused)))
	return err;
    }
    // Generate internal signal for emulation implicit connections
    i->emitDeviceConnectionSignals(f, *this);
  }
  if (unused && m_language == VHDL)
    fprintf(f, "  signal unused : std_logic_vector(0 to %zu);\n", unused - 1);
  if (m_language == VHDL)
    fprintf(f, "begin\n");
  // Set assign external signals where necessary
  for (ConnectionsIter ci = m_assembly->m_connections.begin();
       ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (c.m_external) {
      Port &p = *c.m_external->m_instPort.m_port;
      std::string &nameExt = p.m_master ? c.m_slaveName : c.m_masterName;
      if (nameExt.size() && p.haveInputs())
	assignExt(f, c, nameExt, false);
      nameExt = p.m_master ? c.m_masterName : c.m_slaveName;
      if (nameExt.size() && p.haveOutputs())
	assignExt(f, c, nameExt, true);
    }
  }
  // Assign external port names to connections that don't have temp signals
  for (ConnectionsIter ci = m_assembly->m_connections.begin();
       ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (c.m_external) {
      Port &p = *c.m_external->m_instPort.m_port;
      if (c.m_masterName.empty()) {
	c.m_masterName = p.m_master ?
	  (m_language == VHDL ? p.typeNameOut.c_str() : p.fullNameOut.c_str()) :
	  (m_language == VHDL ? p.typeNameIn.c_str() : p.fullNameIn.c_str());
      }
      if (c.m_slaveName.empty()) {
	c.m_slaveName = p.m_master ?
	  (m_language == VHDL ? p.typeNameIn.c_str() : p.fullNameIn.c_str()) :
	  (m_language == VHDL ? p.typeNameOut.c_str() : p.fullNameOut.c_str());
      }
    }
  }
  // Create the instances
  i = &m_assembly->m_instances[0];
  for (unsigned n = 0; n < m_assembly->m_instances.size(); n++, i++)
    m_assembly->emitAssyInstance(f, i); //, nControlInstances);
  emitTieoffSignals(f);
  if (m_language == Verilog)
    fprintf(f, "\n\nendmodule //%s\n",  m_implName);
  else
    fprintf(f, "end rtl;\n");
  return 0;
}

void Worker::
emitWorkersAttribute()
{
  bool first = true;
  for (WorkersIter wi = m_assembly->m_workers.begin();
       wi != m_assembly->m_workers.end(); wi++, first = false)
    printf("%s%s", first ? "" : " ", (*wi)->m_implName);
  printf("\n");
}

 const char *Worker::
emitWorkersHDL(const char *outFile)
{
  FILE *f;
  const char *err;
  if ((err = openOutput(outFile, m_outDir, "", "", ".wks", NULL, f)))
    return err;
  printgen(f, "#", m_file.c_str(), false);
  Instance *i = &m_assembly->m_instances[0];
  fprintf(f, "# Workers in this %s: <implementation>:<instance>\n",
	  m_type == Container ? "container" : "assembly");
  for (unsigned n = 0; n < m_assembly->m_instances.size(); n++, i++)
    fprintf(f, "%s:%zu:%s\n", i->m_worker->m_implName,
	    i->m_worker->m_paramConfig ? i->m_worker->m_paramConfig->nConfig : 0, i->cname());
  fprintf(f, "# end of instances\n");
  return NULL;
}

void Instance::
emitHdl(FILE *f, const char *prefix, size_t &index)
{
  assert(m_iType == Instance::Application ||
	 m_iType == Instance::Interconnect ||
	 m_iType == Instance::Device ||
	 m_iType == Instance::Platform ||
	 m_iType == Instance::Adapter);
  assert(!m_worker->m_assembly);
  fprintf(f, "<%s name=\"%s%s%s\" worker=\"%s",
	  m_iType == Instance::Application ? "instance" :
	  m_iType == Instance::Interconnect ? "interconnect" :
	  (m_iType == Instance::Device ||
	   m_iType == Instance::Platform) ? "io" : "adapter",
	  prefix ? prefix : "", prefix ? "/" : "", cname(), m_worker->m_implName);
  // FIXME - share this param-named implname with emitWorker
  if (m_worker->m_paramConfig && m_worker->m_paramConfig->nConfig)
    fprintf(f, "-%zu", m_worker->m_paramConfig->nConfig);
  fprintf(f, "\"");
  if (!m_worker->m_noControl)
    fprintf(f, " occpIndex=\"%zu\"", index++);
  if (m_attach)
    fprintf(f, " attachment=\"%s\"", m_attach);
  if (m_iType == Instance::Interconnect) {
    if (m_hasConfig)
      fprintf(f, " ocdpOffset='0x%zx'", m_config * 32 * 1024);
  } else if (m_hasConfig)
    fprintf(f, " configure=\"%#lx\"", (unsigned long)m_config);
  if (m_inserted)
    fprintf(f, " inserted=\"1\"");
  fprintf(f, "/>\n");
}

// Device connection signals are needed to connect signals within the container.
// Most signals are used for connecting to external signal ports of the container,
// and thus do not need these internal signals.
// The two cases we need to handle are:
// 1. Signals between device workers and their emulators.
// 2. Signals to the tristate buffers generated in the container.
void Instance::
emitDeviceConnectionSignals(FILE *f, Worker &assy) {
  ocpiDebug("Emitting device connection signals.");
  for (SignalsIter si = m_worker->m_signals.begin(); si != m_worker->m_signals.end(); si++) {
    Signal &s = **si;
    if (s.m_differential && m_emulated) {
      s.emitConnectionSignal(f, cname(), s.m_pos.c_str(), false, assy.m_language);
      s.emitConnectionSignal(f, cname(), s.m_neg.c_str(), false, assy.m_language);
    } else if (s.m_direction == Signal::INOUT && 
	       (assy.m_type == Worker::Container || m_emulated)) {
      const char *prefix = m_worker->m_type == Worker::Configuration ? NULL : cname();
      s.emitConnectionSignal(f, prefix, s.m_in.c_str(), false, assy.m_language);
      s.emitConnectionSignal(f, prefix, s.m_out.c_str(), false, assy.m_language);
      s.emitConnectionSignal(f, prefix, s.m_oe.c_str(), true, assy.m_language);
    } else if (m_emulated)
      s.emitConnectionSignal(f, cname(), "%s", false, assy.m_language);
  }
}

void Worker::
emitInstances(FILE *f, const char *prefix, size_t &index) {
  Instance *i = &m_assembly->m_instances[0];
  for (unsigned n = 0; n < m_assembly->m_instances.size(); n++, i++)
    if (!i->m_worker->m_assembly)
      i->emitHdl(f, prefix, index);
}

void Worker::
emitInternalConnections(FILE *f, const char *prefix) {
  for (ConnectionsIter ci = m_assembly->m_connections.begin();
       ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (!c.m_external && c.m_attachments.front()->m_instPort.m_port->isData()) {
      InstancePort *from = 0, *to = 0, *bidi = 0;
      for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	InstancePort *ip = &(*ai)->m_instPort;
	Port &p = *ip->m_port;
	if (p.isDataProducer())
	  from = ip;
	else if (p.isDataBidirectional())
	  if (ip->m_role.m_knownRole && !ip->m_role.m_bidirectional)
	    if (ip->m_role.m_provider)
	      to = ip;
	    else
	      from = ip;
	  else
	    bidi = ip;
	else
	  to = ip;
      }
      if (bidi) {
	if (!from)
	  from = bidi;
	else if (!to)
	  to = bidi;
      }
      assert(from && to);
      if (!from->m_port->worker().m_assembly && !to->m_port->worker().m_assembly)
	fprintf(f, "<connection from=\"%s/%s\" out=\"%s\" to=\"%s/%s\" in=\"%s\"/>\n",
		prefix, from->m_instance->cname(), from->m_port->pname(),
		prefix, to->m_instance->cname(), to->m_port->pname());
    }
  }
}

const char *InstancePort::
attach(Attachment *a, size_t index) {
  size_t count = a->m_connection.m_count;
  assert(count);
  //  if (!count)
  //    count = m_port->count - index;
  for (size_t n = index; n < count; n++) {
    if (m_connected[n])
      if (!(m_port->m_type == TimePort && m_port->m_master))
	return OU::esprintf("Multiple connections not allowed for port '%s' "
			    "on instance '%s' of worker '%s'", m_port->pname(),
			    m_instance->cname(), m_port->worker().cname());
    m_connected[n] = true;
  }
  m_attachments.push_back(a);
  return NULL;
}

void InstancePort::
detach(Connection &c) {
  for (Attachments::iterator ai = m_attachments.begin(); ai != m_attachments.end(); ai++)
    if (&(*ai)->m_connection == &c) {
      m_connected[(*ai)->m_index] = false;
      m_attachments.erase(ai);
      c.m_attachments.remove(*ai);
      delete *ai;
      break;
    }
}

// Emit any tieoff assignments related to unconnected parts (indices) of the intermediate
// connection signal
void InstancePort::
emitTieoffAssignments(FILE *f) {
  // Tie off all indices with no connection
  if (m_port->haveInputs() && m_port->m_count > 1 && m_attachments.size())
    for (unsigned i = 0; i < m_port->m_count; i++) {
      bool connected = false;
      // For all connections to this port
      for (AttachmentsIter ai = m_attachments.begin();
	   !connected && ai != m_attachments.end(); ai++)
	if ((*ai)->m_index <= i && i < (*ai)->m_index + (*ai)->m_connection.m_count)
	  connected = true;
      if (!connected)
	fprintf(f, "  %s(%u) <= %s;\n", m_signalIn.c_str(), i,
		m_port->m_master ? m_port->slaveMissing() : m_port->masterMissing());
    }
}

// Attach an instance port to a connection
const char *Connection::
attachPort(InstancePort &ip, size_t index) {
  if (ip.m_external)
    m_nExternals++;
  Attachment *a = new Attachment(ip, *this, index);
  m_attachments.push_back(a);
  if (ip.m_external)
    m_external = a; // last one if there are more than one
  return ip.attach(a, index);
}

const char *Worker::
emitAssyImplHDL(FILE *f, bool wrap) {
  if (m_type == Configuration) {
    assert(!wrap);
    return emitConfigImplHDL(f);
  }
  if (m_type == Container) {  
    assert(!wrap);
    return emitContainerImplHDL(f);
  }
  if (wrap)
    if (m_language == VHDL)
      // The Verilog wrapper around a VHDL assembly doesn't do anything,
      // Since Verilog can already instantiate based on what is in the VHDL impl file.
      fprintf(f, "// The wrapper to enable instantion from Verilog is in the VHDL -impl.vhd file.\n");
    else
      // The assembly is in Verilog, so we implement the record-to-signal wrapper here.
      emitVhdlRecordWrapper(f);
  else
    if (m_language == VHDL)
      if (m_type == Assembly) {
	// This is an application assembly in VHDL.
	// This file simply contains the entity
	const char *comment = hdlComment(m_language);
	fprintf(f,
		"%s This file contains the entity declaration for the VHDL application "
		"assembly: %s\n",
		comment, m_implName);
	fprintf(f,
		"Library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
		"Library ocpi; use ocpi.all, ocpi.types.all;\n"
		"use work.%s_defs.all, work.%s_constants.all;\n",
		m_implName, m_implName);
	emitVhdlLibraries(f);
	fprintf(f,
		"\nentity %s_rv is\n", m_implName);
	emitParameters(f, m_language);
	emitSignals(f, VHDL, true, true, false);
	fprintf(f, "end entity %s_rv;\n", m_implName);
      } else {
	// We need to enable instantiation from Verilog, so we produce the
	// signal-to-record wrapper, which must be in VHDL.
	// The defs file already defined the signal level interface (as a VHDL component).
	// So all we need here is the entity and architecture
	emitVhdlSignalWrapper(f);
      }
    else
      // We need to enable instantiation from VHDL with record interfaces.
      // But that can't happen in Verilog, so the native IMPL file doesn't do anything.
      fprintf(f, "// The wrapper to enable instantion from VHDL is in the VHDL -impl.vhd file.\n");
  fclose(f);
  return NULL;
}

HdlAssembly *HdlAssembly::
create(ezxml_t xml, const char *xfile, Worker *parent, const char *&err) {
  HdlAssembly *ha = new HdlAssembly(xml, xfile, parent, err);
  if (err) {
    delete ha;
    ha = NULL;
  }
  return ha;
}

HdlAssembly::
HdlAssembly(ezxml_t xml, const char *xfile, Worker *parent, const char *&err)
  : Worker(xml, xfile, "", Worker::Assembly, parent, NULL, err) {
  if (!(err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS, (void*)0)) &&
      !(err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, ASSY_ELEMS, (void*)0)))
    err = parseHdl();
}

HdlAssembly::
~HdlAssembly() {
}
