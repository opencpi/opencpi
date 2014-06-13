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
#include "HdlOCCP.h"
#include "assembly.h"
#include "hdl.h"
#include "hdl-platform.h"

const char *Worker::
parseHdlAssy() {
  const char *err;
  Assembly *a = m_assembly = new Assembly(*this);
  a->m_isContainer = strcasecmp(m_xml->name, "HdlContainerAssembly") == 0;
  a->m_isPlatform = strcasecmp(m_xml->name, "HdlPlatformAssembly") == 0;
  
  static const char
    *topAttrs[] = {IMPL_ATTRS, HDL_TOP_ATTRS, HDL_IMPL_ATTRS, NULL},
    // FIXME: reduce to those that are hdl specific
    *instAttrs[] =  { INST_ATTRS },
    *contInstAttrs[] = { "Index", "interconnect", "io", "adapter", "configure", NULL},
    *platInstAttrs[] = { "Index", "interconnect", "io", "adapter", "configure", NULL};
  // Do the generic assembly parsing, then to more specific to HDL
  if ((err = a->parseAssy(m_xml, topAttrs,
			  a->m_isContainer ? contInstAttrs :
			  (a->m_isPlatform ? platInstAttrs : instAttrs),
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
    if (a->m_isContainer || a->m_isPlatform) {
      ezxml_t x = i->instance->xml();
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
	i->iType = Instance::Interconnect;
      } else if (io) {
	i->attach = io;
	i->iType = Instance::IO;
      } else if (ad) {
	i->iType = Instance::Adapter;
	i->attach = ad; // an adapter is for an interconnect or I/O
      }
    }
    // Now we are doing HDL processing per instance
    // Allocate the instance-clock-to-assembly-clock map
    if (i->worker->m_clocks.size())
      i->m_clocks = myCalloc(Clock*, i->worker->m_clocks.size());
    if (!i->worker->m_noControl)
      nControls++;
  }
  Port *wci = NULL;
  unsigned n;
  Clock *clk, *wciClk = NULL;
  // Establish the wciClk for all wci slaves
  if (a->m_isContainer) {
    // The default WCI clock comes from the (single) wci master
    for (n = 0, i = a->m_instances; !wciClk && n < a->m_nInstances; n++, i++)
      if (i->worker) {
	unsigned nn = 0;
	for (InstancePort *ip = i->m_ports; nn < i->worker->m_ports.size(); nn++, ip++) 
	  if (ip->m_port->type == WCIPort && ip->m_port->master) {
	    wciClk = ip->m_port->clock;
	    if (i->m_clocks)
	      i->m_clocks[ip->m_port->clock->ordinal] = wciClk;
	    break;
	  }
      }
  } else {
    // Create the assy's wci slave port, at the beginning of the list
    Port *wci = new Port("wci", this, false, WCIPort, NULL, nControls);
    m_ports.insert(m_ports.begin(), wci);
    // Clocks: coalesce all WCI clock and clocks with same reqts, into one wci, all for the assy
    clk = addClock();
    // FIXME:  this should access the 0th clock more specifically for VHDL
    clk->signal = clk->name = nControls > 1 ? "wci0_Clk" : "wci_Clk";
    clk->reset = nControls > 1 ? "wci0_MReset_n" : "wci_MReset_n";
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
    if (i->worker && i->worker->m_ports[0]->type == WCIPort &&
	!i->worker->m_ports[0]->master && !i->worker->m_assembly)
      i->m_clocks[i->worker->m_ports[0]->clock->ordinal] = wciClk;

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
	     (ip.m_port->worker->m_noControl && ip.m_port->clock && ip.m_port->clock->port == 0) ||
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
      if (!ip.m_external && ip.m_port->isData && ip.m_instance->m_clocks) {
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
              asprintf((char **)&clk->name, "%s_Clk", c.m_name.c_str());
              clk->signal = clk->name;
              clk->port = c.m_external->m_instPort.m_port;
              c.m_external->m_instPort.m_port->myClock = true;
            } else {
              // This port's clock is a separately defined clock
              // We might as well keep the name since we have none better
              clk->name = strdup(ip.m_port->clock->name);
              clk->signal = strdup(ip.m_port->clock->signal);
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
				c.m_name.c_str(), ip.m_port->name, ip.m_instance->name);
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
	if (ip->m_port->isData) {
	  size_t nc = ip->m_port->clock->ordinal;
	  if (!i->m_clocks[nc]) {
	    if (ip->m_port->type == WSIPort || ip->m_port->type == WMIPort)
	      return OU::esprintf("Unconnected data interface %s of instance %s has its own clock",
				  ip->m_port->name, i->name);
	    clk = addClock();
	    i->m_clocks[nc] = clk;
	    asprintf((char **)&clk->name, "%s_%s", i->name, ip->m_port->clock->name);
	    if (ip->m_port->clock->signal)
	      asprintf((char **)&clk->signal, "%s_%s", i->name,
		       ip->m_port->clock->signal);
	    clk->assembly = true;
	  }
	}
    }
  // Now all clocks are done.  We process the non-data external ports.
  // Now all data ports that are connected have mapped clocks and
  // all ports with WCI clocks are connected.  All that's left is
  // WCI: WTI, WMemI, and the platform ports
  size_t nWti = 0, nWmemi = 0;
  for (n = 0, i = a->m_instances; n < a->m_nInstances; n++, i++)
    if (i->worker) {
      Worker *iw = i->worker;
      unsigned nn = 0;
      for (InstancePort *ip = i->m_ports; nn < iw->m_ports.size(); nn++, ip++) {
        Port *pp = ip->m_port;
        switch (pp->type) {
        case WCIPort:
	  // slave ports that are connected are ok as is.
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
	    // external data ports are only connected to ports of workers were this
	    // is true.  And the use-case is just that you can reset the
	    // infrastructure while maintaining worker state.  BUT resetting the
	    // CP could clearly reset anything anyway, so this is only relevant to
	    // just reset the dataplane infrastructure.
	    if (!pp->u.wci.resetWhileSuspended)
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
	case CPPort:
        case NOCPort:
        case MetadataPort:
        case TimePort:
          break;
        default:
          return "Bad port type";
        }
      }
      for (SignalsIter si = i->worker->m_signals.begin(); si != i->worker->m_signals.end(); si++) {
	Signal *s = new Signal(**si);
	// The assembly level signal name needs to be prefixed with the instance name
	if (i->worker->m_isDevice)
	  s->m_name = i->name + ("_" + s->m_name);
	// OU::format(s->m_name, "%s_%s", i->name, s->m_name.c_str());
	m_signals.push_back(s);
      }
    }
  if (!cantDataResetWhileSuspended && wci)
    wci->u.wci.resetWhileSuspended = true;
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
          if (p->type == WSIPort)
            p->u.wsi.regRequest = false;
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
  : /*m_connection(connection), */m_name(name ? name : (connection ? connection->m_name.c_str() : "")),
    m_nExternals(0), m_clock(NULL), m_external(NULL), m_count(0) {
}

// This is where OCP compatibility is checked, and if it can be
// adjusted via tie-offs or other trivial adaptations, those are done here too
static const char *
adjustConnection(Connection &c, InstancePort &consumer, InstancePort &producer, Language lang) {
  // Check WDI compatibility
  Port *prod = producer.m_port, *cons = consumer.m_port;
  // If both sides have protocol, check them for compatibility
  if (prod->protocol->nOperations() && cons->protocol->nOperations()) {
    if (prod->protocol->m_dataValueWidth != cons->protocol->m_dataValueWidth)
      return "dataValueWidth incompatibility for connection";
    if (prod->protocol->m_dataValueGranularity < cons->protocol->m_dataValueGranularity ||
	prod->protocol->m_dataValueGranularity % cons->protocol->m_dataValueGranularity)
      return "dataValueGranularity incompatibility for connection";
    if (prod->protocol->m_maxMessageValues > cons->protocol->m_maxMessageValues)
      return "maxMessageValues incompatibility for connection";
    if (prod->protocol->name().size() && cons->protocol->name().size() &&
	prod->protocol->name() != cons->protocol->name())
      return OU::esprintf("protocol incompatibility: producer: %s vs. consumer: %s",
			  prod->protocol->name().c_str(), cons->protocol->name().c_str());
    if (prod->protocol->nOperations() && cons->protocol->nOperations() && 
	prod->protocol->nOperations() != cons->protocol->nOperations())
      return "numberOfOpcodes incompatibility for connection";
    //  if (prod->u.wdi.nOpcodes > cons->u.wdi.nOpcodes)
    //    return "numberOfOpcodes incompatibility for connection";
    if (prod->protocol->m_variableMessageLength && !cons->protocol->m_variableMessageLength)
      return "variable length producer vs. fixed length consumer incompatibility";
    if (prod->protocol->m_zeroLengthMessages && !cons->protocol->m_zeroLengthMessages)
      return "zero length message incompatibility";
  }
  if (prod->type != cons->type)
    return "profile incompatibility";
  if (prod->dataWidth != cons->dataWidth)
    return OU::esprintf("dataWidth incompatibility. producer %zu consumer %zu",
			prod->dataWidth, cons->dataWidth);
  if (cons->u.wdi.continuous && !prod->u.wdi.continuous)
    return "producer is not continuous, but consumer requires it";
  // Profile-specific error checks and adaptations
  OcpAdapt *oa;
  switch (prod->type) {
  case WSIPort:
    // Bursting compatibility and adaptation
    if (prod->impreciseBurst && !cons->impreciseBurst)
      return "consumer needs precise, and producer may produce imprecise";
    if (cons->impreciseBurst) {
      if (!cons->preciseBurst) {
	// Consumer accepts only imprecise bursts
	if (prod->preciseBurst) {
	  // producer may produce a precise burst
	  // Convert any precise bursts to imprecise
	  oa = &consumer.m_ocp[OCP_MBurstLength];
	  oa->expr =
	    lang == Verilog ? "%s ? 2'b01 : 2'b10" :
	    "std_logic_vector(to_unsigned(2,2) - unsigned(ocpi.types.bit2vec(%s,2)))";
	  oa->other = OCP_MReqLast;
	  oa->comment = "Convert precise to imprecise";
	  oa = &producer.m_ocp[OCP_MBurstLength];
	  oa->expr = lang == Verilog ? "" : "open";
	  oa->comment = "MBurstLength ignored for imprecise consumer";
	  if (prod->impreciseBurst) {
	    oa = &producer.m_ocp[OCP_MBurstPrecise];
	    oa->expr = lang == Verilog ? "" : "open";
	    oa->comment = "MBurstPrecise ignored for imprecise-only consumer";
	  }
	}
      } else { // consumer does both
	// Consumer accept both, has MPreciseBurst Signal
	oa = &consumer.m_ocp[OCP_MBurstPrecise];
	if (!prod->impreciseBurst) {
	  oa->expr = lang == Verilog ? "1'b1" : "to_unsigned(1,1)";
	  oa->comment = "Tell consumer all bursts are precise";
	} else if (!prod->preciseBurst) {
	  oa = &consumer.m_ocp[OCP_MBurstPrecise];
	  oa->expr = lang == Verilog ? "1'b0" : "'0'";
	  oa->comment = "Tell consumer all bursts are imprecise";
	  oa = &consumer.m_ocp[OCP_MBurstLength];
	  oa->other = OCP_MBurstLength;
	  asprintf((char **)&oa->expr,
		   lang == Verilog ? "{%zu'b0,%%s}" : "std_logic_vector(to_unsigned(0,%zu)) & %%s",
		   cons->ocp.MBurstLength.width - 2);
	  oa->comment = "Consumer only needs imprecise burstlength (2 bits)";
	}
      }
    }
    if (prod->preciseBurst && cons->preciseBurst &&
	prod->ocp.MBurstLength.width < cons->ocp.MBurstLength.width) {
      oa = &consumer.m_ocp[OCP_MBurstLength];
      asprintf((char **)&oa->expr,
	       lang == Verilog ? "{%zu'b0,%%s}" : "to_unsigned(0,%zu) & %%s",
	       cons->ocp.MBurstLength.width - prod->ocp.MBurstLength.width);
      oa->comment = "Consumer takes bigger bursts than producer creates";
      oa->other = OCP_MBurstLength;
    }
    // Abortable compatibility and adaptation
    if (cons->u.wsi.abortable) {
      if (!prod->u.wsi.abortable) {
	oa = &consumer.m_ocp[OCP_MFlag];
	oa->expr = "1'b0";
	oa->comment = "Tell consumer no frames are ever aborted";
      }
    } else if (prod->u.wsi.abortable)
      return "consumer cannot handle aborts from producer";
    // EarlyRequest compatibility and adaptation
    if (cons->u.wsi.earlyRequest) {
      if (!prod->u.wsi.earlyRequest) {
	oa = &consumer.m_ocp[OCP_MDataLast];
	oa->other = OCP_MReqLast;
	oa->expr = "%s";
	oa->comment = "Tell consumer last data is same as last request";
	oa = &consumer.m_ocp[OCP_MDataValid];
	oa->other = OCP_MCmd;
	oa->expr = "%s == OCPI_OCP_MCMD_WRITE ? 1b'1 : 1b'0";
	oa->comment = "Tell consumer data is valid when its(request) is MCMD_WRITE";
      }
    } else if (prod->u.wsi.earlyRequest)
      return "producer emits early requests, but consumer doesn't support them";
    // Opcode compatibility
    if (cons->u.wdi.nOpcodes != prod->u.wdi.nOpcodes)
      if (cons->ocp.MReqInfo.value) {
	if (prod->ocp.MReqInfo.value) {
	  if (cons->ocp.MReqInfo.width > prod->ocp.MReqInfo.width) {
	    oa = &consumer.m_ocp[OCP_MReqInfo];
	    asprintf((char **)&oa->expr, "{%zu'b0,%%s}",
		     cons->ocp.MReqInfo.width - prod->ocp.MReqInfo.width);
	    oa->other = OCP_MReqInfo;
	  } else {
	    // producer has more, we just connect the LSBs
	  }
	} else {
	  // producer has none, consumer has some
	  oa = &consumer.m_ocp[OCP_MReqInfo];
	  asprintf((char **)&oa->expr,
		   lang == Verilog ? "%zu'b0" : "std_logic_vector(to_unsigned(0,%zu))",
		   cons->ocp.MReqInfo.width);
	}
      } else {
	// consumer has none
	oa = &producer.m_ocp[OCP_MReqInfo];
	oa->expr = lang == Verilog ? "" : "open";
	oa->comment = "Consumer doesn't have opcodes (or has exactly one)";
      }
    // Byte enable compatibility
    if (cons->ocp.MByteEn.value && prod->ocp.MByteEn.value) {
      if (cons->ocp.MByteEn.width < prod->ocp.MByteEn.width) {
	// consumer has less - "inclusive-or" the various bits
	if (prod->ocp.MByteEn.width % cons->ocp.MByteEn.width)
	  return "byte enable producer width not a multiple of consumer width";
	size_t nper = prod->ocp.MByteEn.width / cons->ocp.MByteEn.width;
	std::string expr = "{";
	size_t pw = prod->ocp.MByteEn.width;
	//oa = &consumer.m_ocp[OCP_MByteEn];
	for (size_t n = 0; n < cons->ocp.MByteEn.width; n++) {
	  if (n)
	    expr += ",";
	  for (size_t nn = 0; nn < nper; nn++)
	    OU::formatAdd(expr, "%s%sMByteEn[%zu]", nn ? "|" : "",
				c.m_masterName.c_str(), --pw);
	}
	expr += "}";
      } else if (cons->ocp.MByteEn.width > prod->ocp.MByteEn.width) {
	// consumer has more - requiring replicating
	if (cons->ocp.MByteEn.width % prod->ocp.MByteEn.width)
	  return "byte enable consumer width not a multiple of producer width";
	size_t nper = cons->ocp.MByteEn.width / prod->ocp.MByteEn.width;
	std::string expr = "{";
	size_t pw = cons->ocp.MByteEn.width;
	//oa = &consumer.m_ocp[OCP_MByteEn];
	for (size_t n = 0; n < prod->ocp.MByteEn.width; n++)
	  for (size_t nn = 0; nn < nper; nn++)
	    OU::formatAdd(expr, "%s%sMByteEn[%zu]", n || nn ? "," : "",
				c.m_masterName.c_str(), --pw);
	expr += "}";
      }
    } else if (cons->ocp.MByteEn.value) {
      // only consumer has byte enables - make them all 1
      oa = &consumer.m_ocp[OCP_MByteEn];
      if (lang == VHDL)
	oa->expr = strdup("(others => '1')");
      else
	asprintf((char **)&oa->expr, "{%zu{1'b1}}", cons->ocp.MByteEn.width);
    } else if (prod->ocp.MByteEn.value) {
      // only producer has byte enables
      oa = &producer.m_ocp[OCP_MByteEn];
      oa->expr = lang == Verilog ? "" : "open";
      oa->comment = "consumer does not have byte enables";
    }
    break;
  case WMIPort:
    break;
  default:
    return "unknown data port type";
  }
  return 0;
}

void InstancePort::
emitConnectionSignal(FILE *f, bool output, Language lang) {
  std::string signal = m_instance->name;
  signal += '_';
  OU::formatAdd(signal,
		output ?
		(lang == VHDL ? m_port->typeNameOut.c_str() : m_port->fullNameOut.c_str()) :
		(lang == VHDL ? m_port->typeNameIn.c_str() : m_port->fullNameIn.c_str()), "");
  (output ? m_signalOut : m_signalIn) = signal;
   switch (m_port->type) {
   case WCIPort:
   case WSIPort:
   case WMIPort:
   case WTIPort:
   case WMemIPort:
     if (lang == Verilog) {
       // Generate signals when both sides has the signal configured.
       OcpSignalDesc *osd;
       OcpSignal *os;
       bool wantMaster = m_port->master && output || !m_port->master && !output;
       for (osd = ocpSignals, os = m_port->ocp.signals; osd->name; os++, osd++)
	 if (os->master == wantMaster && os->value) {
	   fprintf(f, "wire ");
	   if (osd->vector)
	     fprintf(f, "[%3zu:0] ", os->width - 1);
	   else
	     fprintf(f, "        ");
	   fprintf(f, "%s%s;\n", signal.c_str(), osd->name);
	 }
     } else {
       std::string tname;
       OU::format(tname, output ? m_port->typeNameOut.c_str() : m_port->typeNameIn.c_str(), "");
       std::string type;
       Worker *w = m_instance->worker;
       // WCI ports on assemblies are always generic generic
       if (m_port->type == WCIPort && (m_port->master || m_port->worker->m_assembly))
	 OU::format(type, "platform.platform_pkg.wci_%s_%st", output ? "m2s" : "s2m",
		    m_port->count > 1 ? "array_" : "");
       else
	 OU::format(type, "%s.%s_defs.%s%s_t", w->m_library, w->m_implName, tname.c_str(),
		    m_port->count > 1 ? "_array" : "");
       if (m_port->count > 1)
	 OU::formatAdd(type, "(0 to %zu)", m_port->count - 1);
       // Make master the canonical type?
       fprintf(f,
	       "  signal %s : %s;\n", signal.c_str(), type.c_str());
     }	       
     break;
   case CPPort:
     fprintf(f, "  signal %s : platform.platform_pkg.occp_%s_t;\n",
	     signal.c_str(), m_port->master == output ? "in" : "out");
     break;
   case NOCPort:
     fprintf(f, "  signal %s : platform.platform_pkg.unoc_master_%s_t;\n",
	     signal.c_str(), m_port->master == output ? "out" : "in" );
     break;
   case MetadataPort:
     fprintf(f, "  signal %s : platform.platform_pkg.metadata_%s_t;\n",
	     signal.c_str(), output && m_port->master || !output && !m_port->master ? "out" : "in");
     break;
   case TimePort:
     fprintf(f, "  signal %s : platform.platform_pkg.time_service_t;\n", signal.c_str());
   default:;
   }
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
  if (m_port->count > 1) {
    if (maxCount < m_port->count) {
      emitConnectionSignal(f, false, lang);
      for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
	Connection &c = (*ai)->m_connection;
	std::string &cName = m_port->master ? c.m_slaveName : c.m_masterName;
	assert(cName.empty());
	cName = m_signalIn;
	}
    }
  }
  if (m_port->isData)
    for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
      Connection &c = (*ai)->m_connection;
      for (AttachmentsIter cai = c.m_attachments.begin(); cai != c.m_attachments.end(); cai++) {
	InstancePort &other = (*cai)->m_instPort;
	if (&other != this) {
	  err = m_port->u.wdi.isProducer ?
	    adjustConnection(c, other, *this, lang) : adjustConnection(c, *this, other, lang);
	  if (err)
	    return OU::esprintf("For connection between %s/%s and %s/%s: %s",
				m_port->worker->m_implName, m_port->name,
				other.m_port->worker->m_implName, other.m_port->name, err);
	}
      }
    }
  return NULL;
}

// Create the binding for this OCP signal, including any adjustments/adaptations
// This is a port of an instance in the assembly, never an external one
void InstancePort::
connectOcpSignal(OcpSignalDesc &osd, OcpSignal &os, OcpAdapt &oa,
		 std::string &signal, std::string &thisComment, Language lang) {
#if 0
  if (&os == &m_port->ocp.Clk)
    signal = m_instance->m_clocks[m_port->clock->ordinal]->signal;
  else
#endif
  if (m_attachments.empty()) {
    // A truly unconnected port.  All we want is a tieoff if it is an input
    // We can always use zero since that will assert reset
    if (os.master != m_port->master)
      if (lang == VHDL)
	signal = osd.vector ? "(others => '0')" : "'0'";
      else
	OU::format(signal, "%zu'b0", os.width);
    else if (lang == VHDL)
      signal = "open";
    return;
  }
  // Find the other end of the connection
  assert(m_attachments.size() == 1); // OCP connections are always point-to-point
  Connection &c = m_attachments.front()->m_connection;
  assert(c.m_attachments.size() == 2);
  InstancePort *otherIp = NULL;
  Attachment *at;
  for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++)
    if (&(*ai)->m_instPort != this) {
      at = *ai;
      otherIp = &at->m_instPort;
      break;
    }
  assert(otherIp);
  Port &p = *otherIp->m_port;
  // Decide on our indexing.  We need an index if our attachment is a subset of
  // what we are connecting to, which is either another internal port or an external one.
  // In either case
  std::string &cName = os.master ? c.m_masterName : c.m_slaveName;
  size_t index, top, count = 0; // count for indexing purpose
  if (otherIp->m_port->count > c.m_count) {
    // We're connecting to something bigger: indexing is needed in this port binding
    count = c.m_count;
    index = at->m_index;
    top = index + count - 1;
  }
  std::string temp;
  if (count) {
    std::string num, temp1;
    if (lang == Verilog)
      OU::format(num, "%zu", index);
    OU::format(temp1, cName.c_str(), num.c_str());
    if (count > 1) {
      assert(lang == VHDL);
      OU::format(temp, "%s(%zu to %zu)", temp1.c_str(), index, top);
    } else {
      if (lang == VHDL)
	OU::format(temp, "%s(%zu)", temp1.c_str(), index);
      else
	temp = temp1;
    }
  } else
    OU::format(temp, cName.c_str(), "");
  if (lang == VHDL)
    temp += '.';
  if (oa.expr) {
    std::string other;
    if (oa.other != N_OCP_SIGNALS)
      temp += ocpSignals[oa.other].name;
    OU::formatAdd(signal, oa.expr, temp.c_str());
  } else {
    signal = temp + osd.name;
    if (osd.vector && os.width != p.ocp.signals[osd.number].width) {
      OU::formatAdd(signal, lang == Verilog ? "[%zu:0]" : "(%zu downto 0)", os.width - 1);
      thisComment = "worker is narrower than external, which is OK";
    }
  }
}

static void
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

// Emit for one direction
void InstancePort::
emitPortSignals(FILE *f, bool output, Language lang, const char *indent,
		bool &any, std::string &comment, std::string &last) {
  Port &p = *m_port;
  std::string name;
  OU::format(name, output ? p.typeNameOut.c_str() : p.typeNameIn.c_str(), "");
  // only do WCI with individual signals if it is a slave that isn't an assembly
  OcpSignalDesc *osd;
  OcpSignal *os;
  OcpAdapt *oa;
  for (osd = ocpSignals, os = p.ocp.signals, oa = m_ocp; osd->name; os++, osd++, oa++)
    // If the signal is in the interface
    if (os->value && (output ? os->master == p.master : os->master != p.master)) {
      std::string signal, thisComment;
      connectOcpSignal(*osd, *os, *oa, signal, thisComment, lang);
      /* if (signal.length()) */ {
	// We have a new one, so can close the previous one
	doPrev(f, last, comment, hdlComment(lang));
	if (lang == VHDL) {
	  if (any)
	    fputs(indent, f);
	  fprintf(f, "%s.%s => %s", name.c_str(),
		  //		  p.master == os->master ? out.c_str() : in.c_str(),
		  osd->name, signal.c_str());
	} else {
	  fprintf(f, "  .%s(%s",
		  os->signal, signal.c_str());
	  fprintf(f, ")");
	}
	comment = oa->comment ? oa->comment : thisComment.c_str();
	any = true;
      }
    }
}

void Assembly::
emitAssyInstance(FILE *f, Instance *i, unsigned nControlInstances) {
  // emit before parameters
  Language lang = m_assyWorker.m_language;
  if (lang == Verilog) {
    std::string suff;
    if (i->worker->m_paramConfig && i->worker->m_paramConfig->nConfig)
      OU::format(suff, "_c%zu", i->worker->m_paramConfig->nConfig);
    fprintf(f, "%s%s", i->worker->m_implName, suff.c_str());
  } else
    fprintf(f, "  %s_i : component %s.%s_defs.%s_rv\n",
	    i->name, i->worker->m_library, i->worker->m_implName, i->worker->m_implName);
  bool any = false;
  if (i->properties.size()) {
    unsigned n = 0;
    // Emit the compile-time properties (a.k.a. parameter properties).
    for (InstanceProperty *pv = &i->properties[0]; n < i->properties.size(); n++, pv++) {
      OU::Property *pr = pv->property;
      if (pr->m_isParameter) {
	if (lang == VHDL) {
	  std::string value;
	  fprintf(f, any ? ",\n              "  : "  generic map(");
	  fprintf(f, "%s => %s", pr->m_name.c_str(), vhdlValue(pv->value, value));
	} else {
	  fprintf(f, "%s", any ? ", " : " #(");
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
	    1 : pr->m_nBits;
	  fprintf(f, ".%s(%zu'b%lld)",
		  pr->m_name.c_str(), bits, (long long)i64);
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
  if (i->worker->m_clocks.size())
    // For the instance, define the clock signals that are defined separate from
    // any interface/port.
    for (ClocksIter ci = i->worker->m_clocks.begin(); ci != i->worker->m_clocks.end(); ci++) {
      Clock *c = *ci;
      if (!c->port) {
	if (lang == Verilog) {
	  fprintf(f, "  .%s(%s),\n", c->signal,
		  i->m_clocks[c->ordinal]->signal);
	  if (c->reset.size())
	    fprintf(f, "  .%s(%s),\n", c->reset.c_str(),
		    i->m_clocks[c->ordinal]->reset.c_str());
	} else {
	  fprintf(f, "%s%s%s => %s", any ? ",\n" : "", any ? indent : "",
		  c->signal, i->m_clocks[c->ordinal]->signal);
	  if (c->reset.size())
	    fprintf(f, "%s%s => %s", indent, c->reset.c_str(),
		    i->m_clocks[c->ordinal]->reset.c_str());
	}
	any = true;
      }
    }
  std::string last;
  std::string comment;
  InstancePort *ip = i->m_ports;
  for (unsigned n = 0; n < i->worker->m_ports.size(); n++, ip++) {
    //    if (ip->m_attachments.empty())
    //      continue;
    Port &p = *ip->m_port;
    std::string in, out;
    OU::format(in, p.typeNameIn.c_str(), "");
    OU::format(out, p.typeNameOut.c_str(), "");
    // only do WCI with individual signals if it is a slave that isn't an assembly
    if (p.isOCP() && (p.type != WCIPort || (!p.master && !i->worker->m_assembly))) {
      ip->emitPortSignals(f, false, lang, indent, any, comment, last);
      ip->emitPortSignals(f, true, lang, indent, any, comment, last);
    } else {
      doPrev(f, last, comment, myComment());
      // Find the widest connection, since that is the one the signal is based on
      Attachment *at = ip->m_attachments.front();
      Connection *c = at ? &at->m_connection : NULL;
      if (p.type == TimePort) {
	// Only one direction - master outputs to slave
	fprintf(f, "%s%s => ",
		any ? indent : "",
		p.master ? out.c_str() : in.c_str());
	//	fputs(p.master ? c.m_masterName.c_str() : c.m_slaveName.c_str(), f);
	fputs(at ? c->m_masterName.c_str() : "open", f);
      } else {
	// We need to know the indexing of the other attachment
	Attachment *otherAt = NULL;
	for (AttachmentsIter ai = c->m_attachments.begin(); ai != c->m_attachments.end(); ai++)
	  if (*ai != at) {
	    otherAt = *ai;
	    break;
	  }
	assert(otherAt);
	std::string index;
	// Indexing is necessary when only when we are smaller than the other
	if (p.count < otherAt->m_instPort.m_port->count)
	  if (c->m_count > 1)
	    OU::format(index, "(%zu to %zu)", otherAt->m_index, otherAt->m_index + c->m_count - 1);
	  else
	    OU::format(index, "(%zu)", otherAt->m_index);
	// input, then output
	fprintf(f, "%s%s => %s%s,\n%s%s => %s%s",
		any ? indent : "",
		in.c_str(), p.master ? c->m_slaveName.c_str() : c->m_masterName.c_str(), index.c_str(),
		indent,
		out.c_str(), p.master ? c->m_masterName.c_str() : c->m_slaveName.c_str(), index.c_str());
      }
    }
    any = true;
  } // end of port loop
  // Signals are always mapped as external ports
  for (SignalsIter si = i->worker->m_signals.begin(); si != i->worker->m_signals.end(); si++) {
    Signal *s = *si;
    doPrev(f, last, comment, myComment());
    any = true;
    std::string prefix;
    if (i->worker->m_isDevice) {
      prefix = i->name;
      prefix += + "_";
    }
    if (s->m_differential) {
      std::string name;
      OU::format(name, s->m_pos.c_str(), s->m_name.c_str());
      if (lang == VHDL)
	fprintf(f, "%s%s => %s%s,\n", any ? indent : "",
		name.c_str(), prefix.c_str(), name.c_str());
      OU::format(name, s->m_neg.c_str(), s->m_name.c_str());
      if (lang == VHDL)
	fprintf(f, "%s%s => %s%s", any ? indent : "",
		name.c_str(), prefix.c_str(), name.c_str());
    } else if (lang == VHDL)
      fprintf(f, "%s%s => %s%s", any ? indent : "",
	      s->m_name.c_str(), prefix.c_str(), s->m_name.c_str());
  }

  fprintf(f, ");%s%s\n", comment.size() ? " // " : "", comment.c_str());
  // Now we must tie off any outputs that are generically expected, but are not
  // part of the worker's interface
  ip = i->m_ports;
  for (unsigned n = 0; n < i->worker->m_ports.size(); n++, ip++) {
    switch (ip->m_port->type) {
    case WCIPort:
      if (!ip->m_port->ocp.SData.value) {
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
      nControlInstances++;
      break;
    default:;
    }
  }
}

 static void
assignExt(FILE *f, Connection &c, std::string &intName, bool in2ext) {
  assert(c.m_attachments.size() == 2);
  Port &p = *c.m_external->m_instPort.m_port;
  Attachment
    &extAt = *c.m_external,
    &intAt = *(c.m_attachments.front() == c.m_external ? c.m_attachments.back() : c.m_attachments.front());
  std::string ours, theirs;
  OU::format(ours, in2ext ? p.typeNameOut.c_str() : p.typeNameIn.c_str(), "");
  theirs = intName.c_str();
  if (c.m_count < p.count) {
    if (c.m_count == 1)
      OU::formatAdd(ours, "(%zu)", extAt.m_index);
    else
      OU::formatAdd(ours, "(%zu to %zu)", extAt.m_index, extAt.m_index + c.m_count - 1);
  }
  if (c.m_count == 1)
    OU::formatAdd(theirs, "(%zu)", intAt.m_index);
  else
    OU::formatAdd(theirs, "(%zu to %zu)", intAt.m_index, intAt.m_index + c.m_count - 1);
  fprintf(f, "  %s <= %s;\n",
	  in2ext ? ours.c_str() : theirs.c_str(), in2ext ? theirs.c_str() : ours.c_str());
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
	    "library ocpi;\n");
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
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++)
    for (unsigned nn = 0; nn < i->worker->m_ports.size(); nn++) {
      InstancePort &ip = i->m_ports[nn];
      if (!ip.m_external && (err = ip.createConnectionSignals(f, m_language)))
	return err;
    }
  if (m_language == VHDL)
    fprintf(f, "begin\n");
  // Set assign external signals where necessary
  for (ConnectionsIter ci = m_assembly->m_connections.begin();
       ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (c.m_external) {
      bool master = c.m_external->m_instPort.m_port->master;
      std::string &nameExt2In = master ? c.m_slaveName : c.m_masterName;
      if (nameExt2In.size())
	assignExt(f, c, nameExt2In, false);
      std::string &nameIn2Ext = master ? c.m_masterName : c.m_slaveName;
      if (nameIn2Ext.size())
	assignExt(f, c, nameIn2Ext, true);
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
  unsigned nControlInstances = 0;
  i = m_assembly->m_instances;
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++)
    m_assembly->emitAssyInstance(f, i, nControlInstances);
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
	  m_assembly->m_isContainer ? "container" : "assembly");
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++) {
    //    if (!m_assembly->m_isContainer || i->iType != Instance::Application)
    std::string suff;
    if (i->worker->m_paramConfig && i->worker->m_paramConfig->nConfig)
      OU::format(suff, "_c%zu", i->worker->m_paramConfig->nConfig);
    fprintf(f, "%s%s:%s\n", i->worker->m_implName, suff.c_str(), i->name);
  }
  fprintf(f, "# end of instances\n");
  return NULL;
}

static void
emitInstance(Instance *i, FILE *f, const char *prefix, size_t &index)
{
  
  fprintf(f, "<%s name=\"%s%s%s\" worker=\"%s\"",
	  i->iType == Instance::Application ? "instance" :
	  i->iType == Instance::Interconnect ? "interconnect" :
	  i->iType == Instance::IO ? "io" : "adapter",
	  prefix ? prefix : "", prefix ? "/" : "", i->name, i->worker->m_implName);
  if (!i->worker->m_noControl)
    fprintf(f, " occpIndex=\"%zu\"", index++);
  if (i->attach)
    fprintf(f, " attachment=\"%s\"", i->attach);
  if (i->iType == Instance::Interconnect) {
    if (i->hasConfig)
      fprintf(f, " ocdpOffset='0x%zx'", i->config * 32 * 1024);
  } else if (i->hasConfig)
    fprintf(f, " configure=\"%#lx\"", (unsigned long)i->config);
  fprintf(f, "/>\n");
}

void Worker::
emitInstances(FILE *f, const char *prefix, size_t &index) {
  Instance *i = m_assembly->m_instances;
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++)
    if (!i->worker->m_assembly)
      emitInstance(i, f, prefix, index);
}

void Worker::
emitInternalConnections(FILE *f, const char *prefix) {
  for (ConnectionsIter ci = m_assembly->m_connections.begin();
       ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (!c.m_external && c.m_attachments.front()->m_instPort.m_port->isData) {
      InstancePort *from = 0, *to = 0, *bidi = 0;
      for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	InstancePort *ip = &(*ai)->m_instPort;
	Port &p = *ip->m_port;
	if (p.u.wdi.isProducer)
	  from = ip;
	else if (p.u.wdi.isBidirectional)
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
      if (!from->m_port->worker->m_assembly && !to->m_port->worker->m_assembly)
	fprintf(f, "<connection from=\"%s/%s\" out=\"%s\" to=\"%s/%s\" in=\"%s\"/>\n",
		prefix, from->m_instance->name, from->m_port->name,
		prefix, to->m_instance->name, to->m_port->name);
    }
  }
}

const char *Worker::
emitUuidHDL(const OU::Uuid &uuid) {
  const char *err;
  FILE *f;
  if ((err = openOutput(m_implName, m_outDir, "", "_UUID", VER, NULL, f)))
    return err;
  const char *comment = hdlComment(Verilog);
  printgen(f, comment, m_file.c_str(), true);
  OCPI::HDL::HdlUUID uuidRegs;
  memcpy(uuidRegs.uuid, uuid, sizeof(uuidRegs.uuid));
  uuidRegs.birthday = (uint32_t)time(0);
  strncpy(uuidRegs.platform, platform, sizeof(uuidRegs.platform));
  strncpy(uuidRegs.device, device, sizeof(uuidRegs.device));
  strncpy(uuidRegs.load, load ? load : "", sizeof(uuidRegs.load));
  assert(sizeof(uuidRegs) * 8 == 512);
  fprintf(f,
	  "module mkUUID(uuid);\n"
	  "output [511 : 0] uuid;\nwire [511 : 0] uuid = 512'h");
  for (unsigned n = 0; n < sizeof(uuidRegs); n++)
    fprintf(f, "%02x", ((char*)&uuidRegs)[n]&0xff);
  fprintf(f, ";\nendmodule // mkUUID\n");
  if (fclose(f))
    return "Could not close output file. No space?";
  return NULL;
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
			    m_port->name, m_port->worker->m_name.c_str());
    m_connected[n] = true;
  }
  m_attachments.push_back(a);
  return NULL;
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
  if (m_assembly->m_isPlatform) {
    assert(!wrap);
    return emitConfigImplHDL(f);
  }
  if (m_assembly->m_isContainer) {  
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
create(ezxml_t xml, const char *xfile, const char *&err) {
  HdlAssembly *ha = new HdlAssembly(xml, xfile, err);
  if (err) {
    delete ha;
    ha = NULL;
  }
  return ha;
}

HdlAssembly::
HdlAssembly(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, NULL, NULL, err) {
  if (!(err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS, (void*)0)) &&
      !(err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, ASSY_ELEMS, (void*)0)))
    err = parseHdl();
}

HdlAssembly::
~HdlAssembly() {
}
