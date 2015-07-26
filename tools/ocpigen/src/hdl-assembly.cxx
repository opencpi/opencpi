/*
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/for modify
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

// This file contains assembly support, which is almost entirely for HDL,
// but just a little also for RCC and OCL.  FIXME: factor non-HDL assy stuff

#include <cstdio>
#include <cstring>
#include <cassert>
#include "OcpiUtilMisc.h"
#include "assembly.h"
#include "hdl.h"
#include "hdl-platform.h"

const char *Worker::
parseHdlAssy() {
  const char *err;
  if ((err = addBuiltinProperties()))
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
			  true, m_outDir)))
    return err;
  // Do the OCP derivation for all workers
  for (WorkersIter wi = a->m_workers.begin(); wi != a->m_workers.end(); wi++)
    if ((err = (*wi)->deriveOCP()))
      return err;
  Instance *i = a->m_instances;
  size_t nControls = 0;
  for (unsigned n = 0; n < a->m_nInstances; n++, i++) {
    // Count nWCIs - this should work for workers and subassemblies
    if (!i->worker->m_noControl)
      a->m_nWCIs += i->worker->m_ports[0]->count;
    if (i->worker->m_assembly)
      continue;
    if (m_type == Container || m_type == Configuration) {
      ezxml_t x = i->instance->xml();
      if ((err = OE::getBoolean(x, "emulated", &i->m_emulated)))
	return err;
      const char
        *ic = ezxml_cattr(x, "interconnect"), // which interconnect
	*ad = ezxml_cattr(x, "adapter"),      // adapter to which interconnect or io
        *io = ezxml_cattr(x, "device");       // which device
      if ((err = OE::getNumber(x, "configure", &i->config, &i->hasConfig, 0)))
	return OU::esprintf("Invalid configuration value for adapter: %s", err);
      if (ic) {
	if (io)
	  return "Container workers cannot be both IO and Interconnect";
	i->attach = ic;
	//	assert(i->m_iType == Instance::Device);
	i->m_iType = Instance::Interconnect;
      } else if (io) {
	i->attach = io;
	assert(i->m_iType == Instance::Device);
      } else if (ad) {
	assert(i->m_iType == Instance::Application);
	i->m_iType = Instance::Adapter;
	i->attach = ad; // an adapter is for an interconnect or I/O
      }
    }
    // Now we are doing HDL processing per instance
    // Allocate the instance-clock-to-assembly-clock map
    if (i->worker->m_clocks.size()) {
      i->m_clocks = new Clock*[i->worker->m_clocks.size()];
      for (unsigned n = 0; n < i->worker->m_clocks.size(); n++)
	i->m_clocks[n] = NULL;
    }
    if (!i->worker->m_noControl)
      nControls++;
  }
  Port *wci = NULL;
  unsigned n;
  Clock *clk, *wciClk = NULL;
  // Establish the wciClk for all wci slaves
  if (m_type == Container) {
    // The default WCI clock comes from the (single) wci master
    for (n = 0, i = a->m_instances; !wciClk && n < a->m_nInstances; n++, i++)
      if (i->worker) {
	unsigned nn = 0;
	for (InstancePort *ip = i->m_ports; nn < i->worker->m_ports.size(); nn++, ip++) 
	  if (ip->m_port->type == WCIPort && ip->m_port->master) {
	    // Found the instance that is mastering the control plane
	    wciClk = addClock();
	    // FIXME:  this should access the 0th clock more specifically for VHDL
	    wciClk->m_name = "wciClk";
	    OU::format(wciClk->m_signal, "%s_%s_out_i(0).Clk", i->name, ip->m_port->name());
	    OU::format(wciClk->m_reset, "%s_%s_out_i(0).MReset_n", i->name, ip->m_port->name());
	    wciClk->assembly = true;
	    if (i->m_clocks)
	      i->m_clocks[ip->m_port->clock->ordinal] = wciClk;
	    break;
	  }
      }
  } else if (a->m_nWCIs) {
    //    assert(m_ports.size() == 0);
    char *cp;
    asprintf(&cp, "<control name='wci' count='%zu'>", nControls);
    ezxml_t x = ezxml_parse_str(cp, strlen(cp));
    // Create the assy's wci slave port, at the beginning of the list
    wci = createPort<WciPort>(*this, x, NULL, -1, err);
    assert(wci);
    // Clocks: coalesce all WCI clock and clocks with same reqts, into one wci, all for the assy
    clk = addClock();
    // FIXME:  this should access the 0th clock more specifically for VHDL
    clk->m_signal =
      clk->m_name =
      nControls > 1 ? (m_language == VHDL ? "wci_in(0).Clk" : "wci0_Clk") : "wci_Clk";
    clk->m_reset =
      nControls > 1 ?
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
    for (n = 0, i = a->m_instances; n < a->m_nInstances; n++, i++)
      if (i->worker && i->worker->m_ports[0]->type == WCIPort && !i->worker->m_noControl) {
	std::string name;
	OU::format(name, "wci%u", nControl);
	Connection &c = *new Connection(NULL, name.c_str());
	c.m_count = 1;
	a->m_connections.push_back(&c);
	if ((err = c.attachPort(*i->m_ports, 0)) ||
	    (err = c.attachPort(ip, nControl)))
	  return err;
	nControl++;
      }
    wciClk = wci->clock;
  }
  // Map all the wci slave clocks to the assy's wci clock
  for (n = 0, i = a->m_instances; n < a->m_nInstances; n++, i++)
    // Map the instance's WCI clock to the assembly's WCI clock if it has a wci port
    if (i->worker && i->worker->m_wciClock && !i->worker->m_assembly)
      i->m_clocks[i->worker->m_wciClock->ordinal] = wciClk;
#if 0
    if (i->worker && i->worker->m_ports[0]->type == WCIPort &&
	!i->worker->m_ports[0]->master && !i->worker->m_assembly)
      i->m_clocks[i->worker->m_ports[0]->clock->ordinal] = wciClk;
#endif

  // Assign the wci clock to connections where we can
  if (wciClk)
    for (ConnectionsIter ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end();
	 ci++) {
      Connection &c = **ci;
      Attachment *at = NULL;
      for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	Attachment &a = **ai;
	InstancePort &ip = a.m_instPort;
	if (!ip.m_external && 
	    (ip.m_port->type == WCIPort ||
	     // FIXME: how can we really know this is not an independent clock??
	     (ip.m_port->m_worker->m_noControl && ip.m_port->clock && ip.m_port->clock->port == 0) ||
	     ip.m_instance->worker->m_ports[0]->type == WCIPort &&
	     !ip.m_instance->worker->m_ports[0]->master &&
	     // If this (data) port on the worker uses the worker's wci clock
	     ip.m_port->clock == ip.m_instance->worker->m_ports[0]->clock)) {
	  at = &a;
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
	  Attachment &a = **ai;
	  if (!a.m_instPort.m_external) {
	    InstancePort &ip = a.m_instPort;
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
            if (ip.m_port->clock->port) {
              // This clock is owned by a port, so it is a "port clock". So name it
              // after the connection (and external port).
	      clk = addClock();
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
				c.m_name.c_str(), ip.m_port->name(), ip.m_instance->name);
        } else {
          // FIXME CHECK COMPATIBILITY OF c->clock with ip->port->clock
          ip.m_instance->m_clocks[nc] = c.m_clock;
        }
      }
    }
  }
  bool cantDataResetWhileSuspended = false;
  for (n = 0, i = a->m_instances; n < a->m_nInstances; n++, i++)
    if (i->worker && !i->worker->m_assembly) {
      unsigned nn = 0;
      for (InstancePort *ip = i->m_ports; nn < i->worker->m_ports.size(); nn++, ip++) 
	if (ip->m_port->isData()) {
	  size_t nc = ip->m_port->clock->ordinal;
	  if (!i->m_clocks[nc]) {
	    if (ip->m_port->type == WSIPort || ip->m_port->type == WMIPort)
	      return OU::esprintf("Unconnected data interface %s of instance %s has its own clock",
				  ip->m_port->name(), i->name);
	    clk = addClock();
	    i->m_clocks[nc] = clk;
	    OU::format(clk->m_name, "%s_%s", i->name, ip->m_port->clock->name());
	    if (ip->m_port->clock->m_signal.size())
	      OU::format(clk->m_signal, "%s_%s", i->name, ip->m_port->clock->signal());
	    clk->assembly = true;
	  }
	}
    }
  // Now all clocks are done.  We process the non-data external ports.
  // Now all data ports that are connected have mapped clocks and
  // all ports with WCI clocks are connected.  All that's left is
  // WCI: WTI, WMemI, and the platform ports
  size_t nWti = 0, nWmemi = 0;
  for (n = 0, i = a->m_instances; n < a->m_nInstances; n++, i++) {
    assert(i->worker);
    Worker *iw = i->worker;
    unsigned nn = 0;
    for (InstancePort *ip = i->m_ports; nn < iw->m_ports.size(); nn++, ip++) {
      Port *pp = ip->m_port;
      switch (pp->type) {
      case WCIPort:
	// slave ports that are connected are ok as is.
	assert(pp->master || pp == pp->m_worker->m_wci);
	if (!pp->master && !m_noControl) {

	  // Make assembly WCI the union of all inside, with a replication count
	  // We make it easier for CTOP, hoping that wires dissolve appropriately
	  // FIXME: when we generate containers, these might be customized, but not now
	  //if (iw->m_ctl.sizeOfConfigSpace > aw->m_ctl.sizeOfConfigSpace)
	  //            aw->m_ctl.sizeOfConfigSpace = iw->m_ctl.sizeOfConfigSpace;
	  m_ctl.sizeOfConfigSpace = (1ll<<32) - 1;
	  if (iw->m_ctl.writables)
	    m_ctl.writables = true;
#if 0
	  // FIXME: until container automation we must force this
	  if (iw->m_ctl.readables)
#endif
	    m_ctl.readables = true;
#if 0
	  // FIXME: Until we have container automation, we force the assembly level
	  // WCIs to have byte enables.  FIXME
	  if (iw->m_ctl.sub32Bits)
#endif
	    m_ctl.sub32Bits = true;
	  m_ctl.controlOps |= iw->m_ctl.controlOps; // needed?  useful?
	  // Reset while suspended: This is really only interesting if all
	  // external data ports are only connected to ports of workers where this
	  // is true.  And the use-case is just that you can reset the
	  // infrastructure while maintaining worker state.  BUT resetting the
	  // CP could clearly reset anything anyway, so this is only relevant to
	  // just reset the dataplane infrastructure.
	  if (!pp->m_worker->m_wci->resetWhileSuspended())
	    cantDataResetWhileSuspended = true;
	}
	break;
      case WTIPort:
	// We don't share ports since the whole point of WTi is to get
	// intra-chip accuracy via replication of the time clients.
	// We could have an option to use wires instead to make things smaller
	// and less accurate...
	if (!pp->master && ip->m_attachments.empty() &&
	    (err = m_assembly->externalizePort(*ip, "wti", nWti)))
	  return err;
	break;
      case WMemIPort:
	if (pp->master && ip->m_attachments.empty() &&
	    (err = m_assembly->externalizePort(*ip, "wmemi", nWmemi)))
	  return err;
	break;
      case WSIPort:
      case WMIPort:
	// Data ports must explicitly connected.
	break;
      case CPPort:
      case NOCPort:
      case MetadataPort:
      case TimePort:
      case TimeBase:
	break;
      case PropPort: // could do partials when multiple count?
      case DevSigPort: // same?
	break;
      default:
	return OU::esprintf("Bad port type: %u", pp->type);
      }
    }
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
  std::string signal = m_instance->name;
  signal += '_';
  OU::formatAdd(signal,
		output ?
		(lang == VHDL ? m_port->typeNameOut.c_str() : m_port->fullNameOut.c_str()) :
		(lang == VHDL ? m_port->typeNameIn.c_str() : m_port->fullNameIn.c_str()), "");
  signal += "_i"; // Use this to avoid colliding with port signals
  (output ? m_signalOut : m_signalIn) = signal;
  m_port->emitConnectionSignal(f, output, lang, signal);
}

// An instance port that is internal needs to be bound to ONE input and ONE output signal bundle,
// even when it has multiple attachments, some of which might be external.
// Signals are established as the output of an instance port, except in a few cases where it is
// unneeded (directly connected to an external port, or to a bundled port).
// Signals are established for the input in some rare cases.
// This function defines the internal signals, but doesn't bind to any yet.
const char *InstancePort::
createConnectionSignals(FILE *f, Language lang) {
  const char *err;
  // Find out the widest of all ports related to this instance port
  size_t maxCount = 0;
  for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
    Connection &c = (*ai)->m_connection;
    for (AttachmentsIter cai = c.m_attachments.begin(); cai != c.m_attachments.end(); cai++)
      if (&(*cai)->m_instPort != this && (*cai)->m_instPort.m_port->count > maxCount)
	maxCount = (*cai)->m_instPort.m_port->count;
  }
  // Output side: generate signal except when external or connected only to external
  // Or when there is a wider one
  if (!(m_attachments.size() == 1 &&
	m_attachments.front()->m_connection.m_attachments.size() == 2 &&
	m_attachments.front()->m_connection.m_external) &&
      maxCount <= m_port->count &&
      (m_port->type != TimePort || m_port->master))
    {
    emitConnectionSignal(f, true, lang);
    // All connections should use this as their signal
    for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
      Connection &c = (*ai)->m_connection;
      std::string &cName = m_port->master ? c.m_masterName : c.m_slaveName;
      assert(cName.empty());
      cName = m_signalOut;
    }
  }
    
  // Input side: rare - generate signal when it aggregates members from others,
  // Like a WSI slave port array
  if (m_port->count > 1 && (maxCount < m_port->count || m_attachments.size() > 1)) {
    emitConnectionSignal(f, false, lang);
    for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
      Connection &c = (*ai)->m_connection;
      std::string &cName = m_port->master ? c.m_slaveName : c.m_masterName;
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
	    DataPort::adjustConnection(c.m_masterName.c_str(), *m_port, m_ocp,
				       *other.m_port, other.m_ocp, lang) :
	    DataPort::adjustConnection(c.m_masterName.c_str(), *other.m_port, other.m_ocp,
				       *m_port, m_ocp, lang);
	  if (err)
	    return OU::esprintf("For connection between %s/%s and %s/%s: %s",
				m_port->m_worker->m_implName, m_port->name(),
				other.m_port->m_worker->m_implName, other.m_port->name(), err);
	}
      }
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

static void
mapOneSignal(FILE *f, Signal &s, unsigned n, bool isWhole, const char *mapped,
	     const char *indent, const char *pattern, bool single) {
  std::string name, map;
  OU::format(name, pattern, s.name());
  if (*mapped)
    OU::format(map, pattern, mapped);
  if (s.m_width && isWhole && !single)
    OU::formatAdd(name, "(%u)", n);
  fprintf(f, "%s%s => %s", indent, name.c_str(),
	  *mapped ? map.c_str() : (s.m_direction == Signal::IN ?
				   (isWhole || !s.m_width ? "'0'" : "(others => '0')") :
				   "open"));
}

void Assembly::
emitAssyInstance(FILE *f, Instance *i) { // , unsigned nControlInstances) {
  // Before we emit the instantiation, we first emit any tieoff assignments related to
  // unconnected parts (indices) of the intermiediate connection signal
  InstancePort *ip = i->m_ports;
  for (unsigned n = 0; n < i->worker->m_ports.size(); n++, ip++)
    ip->emitTieoffAssignments(f);
  Language lang = m_assyWorker.m_language;
  std::string suff;
  if (i->worker->m_paramConfig && i->worker->m_paramConfig->nConfig)
    OU::format(suff, "_c%zu", i->worker->m_paramConfig->nConfig);
  std::string pkg, tpkg;
  if (lang == Verilog)
    fprintf(f, "%s%s", i->worker->m_implName, suff.c_str());
  else {
    OU::format(pkg, "%s%s.%s_defs", i->worker->m_library, suff.c_str(), i->worker->m_implName);
    OU::format(tpkg, "%s%s.%s_constants", i->worker->m_library, suff.c_str(), i->worker->m_implName);
    fprintf(f, "  %s_i : component %s.%s_rv%s\n",
	    i->name, pkg.c_str(), i->worker->m_implName, suff.c_str());
  }
  bool any = false;
  if (i->properties.size()) {
    unsigned n = 0;
    // Emit the compile-time properties (a.k.a. parameter properties).
    for (InstanceProperty *pv = &i->properties[0]; n < i->properties.size(); n++, pv++) {
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
    fprintf(f, " %s (\n",i->name);
  else
    fprintf(f, "    port map(   ");
  any = false;
  const char *indent = "                ";
  // For the instance, define the clock signals that are defined separate from
  // any interface/port.
  for (ClocksIter ci = i->worker->m_clocks.begin(); ci != i->worker->m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!c->port) {
      if (lang == Verilog) {
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
  ip = i->m_ports;
  for (unsigned n = 0; n < i->worker->m_ports.size(); n++, ip++) {
    // We can't do this since we need the opportunity of stubbing unconnected ports properly
    //    if (ip->m_attachments.empty())
    //      continue;
    ip->m_port->emitPortSignals(f, ip->m_attachments, lang, indent, any, comment, last,
				myComment(), ip->m_ocp); 
    any = true;
  } // end of port loop
  // First we need to figure out whether this is an emulator worker or a worker that
  // has a paired emulator worker.
  Instance *emulated = NULL, *ii = m_instances;
  if (i->worker->m_emulate)
    for (unsigned n = 0; n < m_nInstances; n++, ii++)
      if (!strcasecmp(ii->worker->m_implName, i->worker->m_emulate->m_implName)) {
	emulated = ii;
	break;
      }
  // Instance signals are connected to external ports unless they are connected to an emulator,
  // and sometimes they are mapped to slot names, and sometimes they are mapped to nothing when
  // the platform doesn't support the signal.  Also, if they are tristate, they may be
  // connected to an internal signal that is attached to the tristate buffer instanced in the
  // container.
  std::string prefix;
  if (i->worker->m_isDevice && i->worker->m_type != Worker::Platform) {
    if (emulated)
      prefix = emulated->name;
    else
      prefix = i->name;
    prefix += "_";
  }
  for (SignalsIter si = i->worker->m_signals.begin(); si != i->worker->m_signals.end(); si++) {
    Signal &s = **si;
    bool anyMapped = false;
    std::string name;
    // Allow for signals in a vector to be mapped individually (e.g. to slot signal).
    for (unsigned n = 0; s.m_width ? n < s.m_width : n == 0; n++) {
      bool isSingle;
      const char *mappedExt = i->m_extmap.findSignal(s, n, isSingle);
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
	  m_assyWorker.recordSignalConnection(*es, (prefix + s.m_name).c_str());
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
      OU::format(name, s.m_pos.c_str(), s.name());
      if (lang == VHDL)
	fprintf(f, "%s%s => %s%s,\n", any ? indent : "",
		name.c_str(), prefix.c_str(), name.c_str());
      OU::format(name, s.m_neg.c_str(), s.name());
      if (lang == VHDL)
	fprintf(f, "%s%s => %s%s", any ? indent : "",
		name.c_str(), prefix.c_str(), name.c_str());
    } else if (s.m_direction == Signal::INOUT || s.m_direction == Signal::OUTIN) {
      OU::format(name, s.m_in.c_str(), s.name());
      fprintf(f, "%s%s => %s%s,\n", any ? indent : "",
	      name.c_str(), prefix.c_str(), name.c_str());
      OU::format(name, s.m_out.c_str(), s.name());
      fprintf(f, "%s%s => %s%s,\n", any ? indent : "",
	      name.c_str(), prefix.c_str(), name.c_str());
      OU::format(name, s.m_oe.c_str(), s.name());
      fprintf(f, "%s%s => %s%s", any ? indent : "",
	      name.c_str(), prefix.c_str(), name.c_str());
    } else if (lang == VHDL)
      fprintf(f, "%s%s => %s%s", any ? indent : "",
	      s.name(), prefix.c_str(), s.name());
    if (!i->m_emulated && !i->worker->m_emulate && !anyMapped) {
      Signal *es = m_assyWorker.m_sigmap[(prefix + s.m_name).c_str()];
      assert(es);
      m_assyWorker.recordSignalConnection(*es, (prefix + s.m_name).c_str());
    }
  }
  fprintf(f, ");%s%s\n", comment.size() ? " // " : "", comment.c_str());
  // Now we must tie off any outputs that are generically expected, but are not
  // part of the worker's interface
  if (i->worker->m_wci) {
    if (!i->worker->m_wci->ocp.SData.value) {
      ip = &i->m_ports[i->worker->m_wci->m_ordinal];
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
      fprintf(f, "assign %sSData = 32'b0;\n", externalName.c_str());
    }
    //    nControlInstances++;
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
  Instance *i = m_assembly->m_instances;
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++) {
    for (unsigned nn = 0; nn < i->worker->m_ports.size(); nn++) {
      InstancePort &ip = i->m_ports[nn];
      assert(!ip.m_external);
      if (!ip.m_external && (err = ip.createConnectionSignals(f, m_language)))
	return err;
    }
    // Generate internal signal for emulation implicit connections
    i->emitDeviceConnectionSignals(f, m_type == Container);
  }
  if (m_language == VHDL)
    fprintf(f, "begin\n");
  // Set assign external signals where necessary
  for (ConnectionsIter ci = m_assembly->m_connections.begin();
       ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (c.m_external) {
      Port &p = *c.m_external->m_instPort.m_port;
      std::string &nameExt = p.master ? c.m_slaveName : c.m_masterName;
      if (nameExt.size() && p.haveInputs())
	assignExt(f, c, nameExt, false);
      nameExt = p.master ? c.m_masterName : c.m_slaveName;
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
	c.m_masterName = p.master ?
	  (m_language == VHDL ? p.typeNameOut.c_str() : p.fullNameOut.c_str()) :
	  (m_language == VHDL ? p.typeNameIn.c_str() : p.fullNameIn.c_str());
      }
      if (c.m_slaveName.empty()) {
	c.m_slaveName = p.master ?
	  (m_language == VHDL ? p.typeNameIn.c_str() : p.fullNameIn.c_str()) :
	  (m_language == VHDL ? p.typeNameOut.c_str() : p.fullNameOut.c_str());
      }
    }
  }
  // Create the instances
  i = m_assembly->m_instances;
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++)
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
  Instance *i = m_assembly->m_instances;
  fprintf(f, "# Workers in this %s: <implementation>:<instance>\n",
	  m_type == Container ? "container" : "assembly");
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++) {
#if 0
    std::string suff;
    if (i->worker->m_paramConfig && i->worker->m_paramConfig->nConfig)
      OU::format(suff, "_c%zu", i->worker->m_paramConfig->nConfig);
#endif
    fprintf(f, "%s:%zu:%s\n", i->worker->m_implName,
	    i->worker->m_paramConfig ? i->worker->m_paramConfig->nConfig : 0, i->name);
  }
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
  assert(!worker->m_assembly);
  fprintf(f, "<%s name=\"%s%s%s\" worker=\"%s",
	  m_iType == Instance::Application ? "instance" :
	  m_iType == Instance::Interconnect ? "interconnect" :
	  (m_iType == Instance::Device ||
	   m_iType == Instance::Platform) ? "io" : "adapter",
	  prefix ? prefix : "", prefix ? "/" : "", name, worker->m_implName);
  // FIXME - share this param-named implname with emitWorker
  if (worker->m_paramConfig && worker->m_paramConfig->nConfig)
    fprintf(f, "-%zu", worker->m_paramConfig->nConfig);
  fprintf(f, "\"");
  if (!worker->m_noControl)
    fprintf(f, " occpIndex=\"%zu\"", index++);
  if (attach)
    fprintf(f, " attachment=\"%s\"", attach);
  if (m_iType == Instance::Interconnect) {
    if (hasConfig)
      fprintf(f, " ocdpOffset='0x%zx'", config * 32 * 1024);
  } else if (hasConfig)
    fprintf(f, " configure=\"%#lx\"", (unsigned long)config);
  fprintf(f, "/>\n");
}

// Device connection signals are needed to connect signals within the container.
// Most signals are used for connecting to external signal ports of the container,
// and thus do not need these internal signals.
// The two cases we need to handle are:
// 1. Signals between device workers and their emulators.
// 2. Signals to the tristate buffers generated in the container.
void Instance::
emitDeviceConnectionSignals(FILE *f, bool container) {
  for (SignalsIter si = worker->m_signals.begin(); si != worker->m_signals.end(); si++) {
    Signal &s = **si;
    if (s.m_differential && m_emulated) {
      s.emitConnectionSignal(f, name, s.m_pos.c_str(), false);
      s.emitConnectionSignal(f, name, s.m_neg.c_str(), false);
    } else if (s.m_direction == Signal::INOUT && container) {
      const char *prefix = worker->m_type == Worker::Configuration ? NULL : name;
      s.emitConnectionSignal(f, prefix, s.m_in.c_str(), false);
      s.emitConnectionSignal(f, prefix, s.m_out.c_str(), false);
      s.emitConnectionSignal(f, prefix, s.m_oe.c_str(), true);
    } else if (m_emulated)
      s.emitConnectionSignal(f, name, "%s", false);
  }
}

void Worker::
emitInstances(FILE *f, const char *prefix, size_t &index) {
  Instance *i = m_assembly->m_instances;
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++)
    if (!i->worker->m_assembly)
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
      if (!from->m_port->m_worker->m_assembly && !to->m_port->m_worker->m_assembly)
	fprintf(f, "<connection from=\"%s/%s\" out=\"%s\" to=\"%s/%s\" in=\"%s\"/>\n",
		prefix, from->m_instance->name, from->m_port->name(),
		prefix, to->m_instance->name, to->m_port->name());
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
      if (!(m_port->type == TimePort && m_port->master))
	return OU::esprintf("Multiple connections not allowed for port '%s' on worker '%s'",
			    m_port->name(), m_port->m_worker->m_name.c_str());
    m_connected[n] = true;
  }
  m_attachments.push_back(a);
  return NULL;
}

// Emit any tieoff assignments related to unconnected parts (indices) of the intermediate
// connection signal
void InstancePort::
emitTieoffAssignments(FILE *f) {
  // Tie off all indices with no connection
  if (m_port->haveInputs() && m_port->count > 1)
    for (unsigned i = 0; i < m_port->count; i++) {
      bool connected = false;
      // For all connections to this port
      for (AttachmentsIter ai = m_attachments.begin();
	   !connected && ai != m_attachments.end(); ai++)
	if ((*ai)->m_index <= i && i < (*ai)->m_index + (*ai)->m_connection.m_count)
	  connected = true;
      if (!connected)
	fprintf(f, "  %s(%u) <= %s;\n", m_signalIn.c_str(), i,
		m_port->master ? m_port->slaveMissing() : m_port->masterMissing());
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
      // We need to enable instantiation from Verilog, so we produce the
      // signal-to-record wrapper, which must be in VHDL.
      // The defs file already defined the signal level interface (as a VHDL component).
      // So all we need here is the entity and architecture
      emitVhdlSignalWrapper(f);
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
