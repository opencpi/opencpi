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

// See whether adapters need to be inserted for this connection
const char *Assembly::
insertAdapter(Connection &c, InstancePort &from, InstancePort &to) {
  DataPort
    &dpFrom = *static_cast<DataPort *>(from.m_port),
    &dpTo = *static_cast<DataPort *>(to.m_port);
  Clock
    &fromClock = *from.m_instance->m_clocks[from.m_port->m_clock->m_ordinal],
    &toClock = *to.m_instance->m_clocks[to.m_port->m_clock->m_ordinal];
  ocpiInfo("  From instance \"%s\" worker \"%s\" spec \"%s\" port \"%s\" clock \"%s\" width %zu",
	   from.m_instance->cname(), from.m_instance->m_worker->cname(),
	   from.m_instance->m_worker->m_specName, dpFrom.pname(), fromClock.cname(),
	   dpFrom.m_dataWidth);
  ocpiInfo("  To   instance \"%s\" worker \"%s\" spec \"%s\" port \"%s\" clock \"%s\" width %zu",
	   to.m_instance->cname(), to.m_instance->m_worker->cname(),
	   to.m_instance->m_worker->m_specName, dpTo.pname(), toClock.cname(),
	   dpFrom.m_dataWidth);
  if (dpFrom.m_dataWidth == dpTo.m_dataWidth && &fromClock == &toClock)
    return NULL;
  ocpiInfo("WSI Adapter required between:");
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
  std::string worker;
  std::string s;

  if (dpFrom.m_dataWidth && dpTo.m_dataWidth) {
    props.resize(2);
    OU::format(s, "%zu", dpFrom.m_dataWidth);
    props[0].setValue("width_in", s.c_str());
    OU::format(s, "%zu", dpTo.m_dataWidth);
    props[1].setValue("width_out", s.c_str());
    worker = "wsi_width";
  } else {
    props.resize(1);
    OU::format(s, "%zu", dpFrom.m_dataWidth ? dpFrom.m_dataWidth : dpTo.m_dataWidth);
    props[0].setValue("width", s.c_str());
    worker = dpTo.m_dataWidth ? "wsi_from_zero" : "wsi_to_zero"; // to_zero needed when both are zero
  }
  if (&fromClock != &toClock)
    worker += "_clock";
  worker += "_adapter";
  const char *err;
  if ((err = i.init(*this, name.c_str(), worker.c_str(), NULL, props)) ||
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
  // set the clock of the new connection
  size_t
    ncFrom = dpFrom.m_clock->m_ordinal,
    ncTo = dpTo.m_clock->m_ordinal,
    ncIn = i.m_ports[0].m_port->m_clock->m_ordinal,
    ncOut =i.m_ports[1].m_port->m_clock->m_ordinal;
  i.m_clocks[ncIn] = from.m_instance->m_clocks[ncFrom];
  i.m_clocks[ncOut] = to.m_instance->m_clocks[ncTo];
  i.m_clocks[i.m_worker->m_wciClock->m_ordinal] = m_assyWorker.m_wciClock;
  if (c.m_clock)
    assert(c.m_clock == i.m_clocks[ncIn]);
  else
    c.setClock(*i.m_clocks[ncIn]);
  a2c.setClock(*i.m_clocks[ncOut]);
  return NULL;
}

bool Connection::
setClock(Clock &c) {
  assert(!m_clock);
  // And force the clock for each connected port to BE the this clock
  // This will potentially connect an independent clock on a worker to this clock
  for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
    InstancePort &ip = (**ai).m_instPort;
    if (!ip.m_external && ip.m_port->isOCP()) {
      size_t nc = ip.m_port->m_clock->m_ordinal; // the clock ordinal within the worker of the port
      if (ip.m_instance->m_clocks[nc] && ip.m_instance->m_clocks[nc] != &c)
	return false;
    }
  }
  for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
    InstancePort &ip = (**ai).m_instPort;
    if (!ip.m_external && ip.m_port->isOCP()) {
      size_t nc = ip.m_port->m_clock->m_ordinal; // the clock ordinal within the worker of the port
      assert(ip.m_instance->m_clocks);
      assert(!ip.m_instance->m_clocks[nc] || ip.m_instance->m_clocks[nc] == &c);
      ip.m_instance->m_clocks[nc] = &c;
      // If the clock was originally internal, it is now being propagated and becoming external.
      // So we externalize it
      if (c.m_internal && m_external) {
	ocpiInfo("Externalizing the assembly worker clock \"%s\" to be the %s clock for "
		 "external port \"%s\"", c.cname(), c.m_output ? "output" : "input", 
		 m_external->m_instPort.m_port->pname());
	c.rename(m_external->m_instPort.m_port->pname(), m_external->m_instPort.m_port);
      }
    }
  }
  m_clock = &c; // set connection's clock
  if (m_external) {
    // The external/assembly port might alerad
    assert(!m_external->m_instPort.m_port->m_clock ||
	   m_external->m_instPort.m_port->m_clock == m_clock);
    m_external->m_instPort.m_port->m_clock = m_clock; // set external port's clock
    c.m_internal = false; // in case it started out as internal
  }
  return true;
}

// Look for unclocked connections where some instance port has a mapped clock (due to a different
// port on the same instance with the same clock having its connection become clocked).
// Each time we find one, it might enable another connection attached to a
// different port of the instance that shares that clock.  So we keep trying until we find nothing
// to do.
void Assembly::
propagateClocks() {
  bool workDone;
  do {
    workDone = false;
    for (auto ci = m_connections.begin(); ci != m_connections.end(); ++ci) {
      Connection &c = **ci;
      if (!c.m_clock) {
	for (auto ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	  InstancePort &ip = (**ai).m_instPort;
	  if (ip.m_port->isOCP() && !ip.m_external &&
	      ip.m_instance->m_clocks[ip.m_port->m_clock->m_ordinal]) {
	    if (c.setClock(*ip.m_instance->m_clocks[ip.m_port->m_clock->m_ordinal]))
	      workDone = true;
	    break;
	  }
	}
      }
    }
  } while (workDone);
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
			  true)) ||
      (err = parseClocks()))
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
  Port *wci = NULL;
  unsigned n;
  // Establish the m_wciClock for all wci slaves
  if (m_type == Container) {
    // The default WCI clock comes from the (single) wci master
    for (n = 0, i = &a->m_instances[0]; !m_wciClock && n < a->m_instances.size(); n++, i++)
      if (i->m_worker) {
	unsigned nn = 0;
	for (InstancePort *ip = &i->m_ports[0]; nn < i->m_worker->m_ports.size(); nn++, ip++) 
	  if (ip->m_port->m_type == WCIPort && ip->m_port->m_master) {
	    // Found the instance that is mastering the control plane
	    m_wciClock = &addClock("wci");
	    ocpiInfo("Adding the top level control clock, wci, for the container");
	    // FIXME:  this should access the 0th clock more specifically for VHDL
	    OU::format(m_wciClock->m_signal, "%s_%s_out_i(0).Clk", i->cname(), ip->m_port->pname());
	    OU::format(m_wciClock->m_reset, "%s_%s_out_i(0).MReset_n", i->cname(), ip->m_port->pname());
	    if (i->m_clocks)
	      i->m_clocks[ip->m_port->m_clock->m_ordinal] = m_wciClock;
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
    assert(wci->m_clock);
    Clock &clk = *wci->m_clock;
    clk.m_signal =
      clk.m_name =
      a->m_nWCIs > 1 ? (m_language == VHDL ? "wci_in(0).Clk" : "wci0_Clk") : "wci_Clk";
    clk.m_reset =
      a->m_nWCIs > 1 ?
      (m_language == VHDL ? "wci_in(0).MReset_n" : "wci0_MReset_n") : "wci_MReset_n";
    clk.m_port = wci;
    wci->m_myClock = true;
    wci->m_clock = &clk;
    wci->m_clock->m_port = wci;
    OU::Assembly::External *ext = new OU::Assembly::External;
    ext->m_name = "wci";
    ext->m_role.m_provider = false; // provisional
    ext->m_role.m_bidirectional = false;
    ext->m_role.m_knownRole = true;
    InstancePort &ip = *new InstancePort(NULL, wci, ext);
    unsigned nControl = 0;
    for (n = 0, i = &a->m_instances[0]; n < a->m_instances.size(); n++, i++)
      if (i->m_worker && i->m_worker->m_ports.size() && 
          i->m_worker->m_ports[0]->m_type == WCIPort && !i->m_worker->m_noControl) {
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
  }
  // Map all the wci slave clocks to the assy's wci clock
  for (n = 0, i = &a->m_instances[0]; n < a->m_instances.size(); n++, i++)
    // Map the instance's WCI clock to the assembly's WCI clock if it has a wci port
    if (i->m_worker && i->m_worker->m_wciClock && !i->m_worker->m_assembly)
      i->m_clocks[i->m_worker->m_wciClock->m_ordinal] = m_wciClock;
#if 0
  if (i->m_worker && i->worker->m_ports[0]->m_type == WCIPort &&
    i->m_worker->m_ports[0]->m_master && !i->m_worker->m_assembly)
    i->m_clocks[i->m_worker->m_ports[0]->m_clock->m_ordinal] = m_wciClock;
#endif
  // Externalize ports that needed it, before dealing with clocks
  bool cantDataResetWhileSuspended = false;
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

  // All the external ports (and connections to them) are now established, but their clocks are not.

  // Every worker has a set of clocks.
  // Every port of a worker is associated with one of its worker's clocks.
  // At the same time each of a worker's clocks are usually "owned" by one of its ports.

  // Every connection in the assembly must be associated with one of the *assembly* worker's clocks.
  // Every port of every instance has a mapping between the clocks for the worker of that instance
  // and the assembly worker's clocks.  The map per instance (m_clocks) is indexed by the ordinal
  // of the worker clock.
  // So if a worker has 3 clocks, the instance's m_clocks array is of size 3, and for each of the
  // worker's (3) clocks, the map points to the assembly worker's clock that will be bound to the
  // instance's worker's // clocks

  // Finally, some of the assembly worker's clocks will be *internal*, meaning that they do not
  // appear in the assembly worker's external interface, either on some external port or as a
  // standalone clock of the assembly worker.

  // We are assigning a (assembly-level) clock to each connection and thus to each instance's port
  // that is connected.

  // Assigning clocks to connections happens in 4 passes:
  // 1. Assign the wci/control clock to connections where it is required.
  // 2. Assign clocks associated with worker ports that the worker port is driving (output from workers
  //    like ADC)
  // 3. Specify the clock that is associated with an external port of the connection.
  // 4. For "clock islands" with none of the above, create an assembly level clock input.

  // Pass 1. Assign the wci clock to internal ports where we can, and set the clock of the connection
  // accordingly if we can.
  assert(m_wciClock);// testing this
  if (m_wciClock)
    for (auto ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end(); ++ci) {
      Connection &c = **ci;
      if (!c.m_clock) {
	InstancePort *wciClocked1 = NULL, *wciClocked2 = NULL, *other = NULL;
	for (auto ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ++ai) {
	  InstancePort &ip = (**ai).m_instPort;
	  if (ip.m_external)
	    continue;
	  if (ip.m_port->m_type == WCIPort || ip.m_port->m_clock == ip.m_port->worker().m_wciClock) {
	    size_t nc = ip.m_port->m_clock->m_ordinal; // clock ordinal within worker of the port
	    ip.m_instance->m_clocks[nc] = m_wciClock;
	    (wciClocked1 ? wciClocked2 : wciClocked1) = &ip;
	  } else if (ip.m_port->m_myClock && !ip.m_port->m_clock->m_output)
	    other = &ip;
	}
	if (wciClocked1 && (wciClocked2 || c.m_external || other))
	  ocpiCheck(c.setClock(*m_wciClock));
      }
    }
  // Pass 2. set the clock for any connection with an external port to a internal port
  // with an owned clock.  This pass sets external port clocks for these ports.
  // It also sets the clock for internal connections with a driven clock
  for (auto ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (!c.m_clock) {
      InstancePort *clockOut = NULL, *other = NULL;
      for (auto ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	InstancePort &ip = (**ai).m_instPort;
	if (!ip.m_port->isOCP() || ip.m_external)
	  continue;
	assert(ip.m_port->m_clock);
	if (ip.m_port->m_myClock) {
	  assert(!ip.m_instance->m_clocks[ip.m_port->m_clock->m_ordinal]);
	  if (c.m_external) { // look for external connection to owned clocks
	    ocpiInfo("Promoting the port %s clock of instance %s port %s to be the assembly's clock "
		     "for port %s", ip.m_port->m_clock->m_output ? "output" : "input",
		     ip.m_instance->cname(), ip.m_port->pname(),
		     c.m_external->m_instPort.m_port->pname());
	    ocpiCheck(c.setClock(c.m_external->m_instPort.m_port->addMyClock(ip.m_port->m_clock->m_output)));
	    break;
	  }
	  if (ip.m_port->m_clock->m_output) { // look for internal connections with a driven clock
	    Clock &clk = addClock(ip.m_instance->m_name + "_" + ip.m_port->m_clock->cname(), true);
	    clk.m_internal = true; // may be overridden later during propagation
	    ocpiInfo("Creating assembly worker %s's internal clock \"%s\" from port's %s clock of "
		     "instance %s port %s", cname(), clk.cname(),
		     clk.m_output ? "output" : "input", ip.m_instance->cname(), ip.m_port->pname());
	    ip.m_instance->m_clocks[ip.m_port->m_clock->m_ordinal] = &clk;
	    (clockOut ? other : clockOut) = &ip;
	  } else
	    other = &ip;
	} else
	  other = &ip;
      }
      // Take care of internal connections with a driven clock to a free clock
      if (!c.m_clock && !c.m_external && clockOut && other && other->m_port->m_myClock &&
	  !other->m_port->m_clock->m_output)
	ocpiCheck(c.setClock(*clockOut->m_instance->m_clocks[clockOut->m_port->m_clock->m_ordinal]));
    }
  }
  // Now external ports have clocks if they are connected to internal ports with owned clocks, and
  // internal connections with driven clocks are also done
  m_assembly->propagateClocks();
#if 0
  // 3. Create clocks for remaining unclocked connections with external ports,
  // but after each one propagate the clocks to other connections.
  // I.e. a single clock may end up used for multiple connections with external ports.
  for (auto ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (!c.m_clock && c.m_external && c.m_external->m_instPort.m_port->isOCP()) {
      assert("external connections still without clock" == 0);
      // Not based on any other clock and not inferred from any other port's clock
      Clock &clk = c.m_external->m_instPort.m_port->addMyClock(false);
      //      ocpiInfo("Promoting the port's %s clock of instance %s port %s to be the assembly's clock "
      //	       "for port %s", clk.m_output ? "output" : "input");
      c.setClock(clk);
      m_assembly->propagateClocks();
    }
  }
#endif
  // 4. Now we are left with "clock islands" - internal connections with no inferrable clocks.
  //    Create a worker-level clock not associated with any external port of the assembly.
  for (auto ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (!c.m_clock && !c.m_external && (*c.m_attachments.begin())->m_instPort.m_port->isOCP()) {
      Clock *clk = NULL, *clk2 = NULL;
      for (auto ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++) {
	InstancePort &ip = (**ai).m_instPort;
	size_t nc = ip.m_port->m_clock->m_ordinal; // the clock ordinal within the worker of the port
	if (ip.m_instance->m_clocks[nc]) {
	  if (clk) {
	    if (clk != ip.m_instance->m_clocks[nc])
	      clk2 = clk; // two different clocks
	  } else
	    clk = ip.m_instance->m_clocks[nc];
	}
      }
      if (!c.m_clock && !clk2) {
	if (!clk)
	  clk = &addClock(c.m_name + "_Clk");
	ocpiCheck(c.setClock(*clk));
	m_assembly->propagateClocks();
      }
    }
  }
  // Check for unconnected ports without clocks
  for (n = 0, i = &a->m_instances[0]; n < a->m_instances.size(); n++, i++)
    if (i->m_worker && !i->m_worker->m_assembly) {
      unsigned nn = 0;
      for (InstancePort *ip = &i->m_ports[0]; nn < i->m_worker->m_ports.size(); nn++, ip++) 
	if (ip->m_port->isOCP()) {
	  size_t nc = ip->m_port->m_clock->m_ordinal;
	  if (!i->m_clocks[nc]) {
	    assert(ip->m_attachments.empty());
	    return OU::esprintf("Unconnected interface %s of instance %s has its own clock",
				ip->m_port->pname(), i->cname());
	  }
	}
    }
  // Look at connections for inserting adapters between ports
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
emitConnectionSignal(FILE *f, bool output, Language lang, bool clock) {
  std::string signal = m_instance->m_name;
  signal += '_';
  OU::formatAdd(signal,
		output ?
		(lang == VHDL ? m_port->typeNameOut.c_str() : m_port->fullNameOut.c_str()) :
		(lang == VHDL ? m_port->typeNameIn.c_str() : m_port->fullNameIn.c_str()), "");
  signal += "_i"; // Use this to avoid colliding with port signals
  m_port->emitConnectionSignal(f, output, lang, clock, signal);
  (clock ? m_clockSignal : (output ? m_signalOut : m_signalIn)) = signal;
}

// An instance port that is internal needs to be bound to ONE input and ONE output signal bundle,
// even when it has multiple attachments, some of which might be external.
// Such signals are established as the output of an instance port, except in a few cases where it is
// unneeded (directly connected to an external port, or to a bundled port).
// Signals are established for the input side in some rare cases.
// This function defines the internal signals, but doesn't assign them yet.
void InstancePort::
createConnectionSignals(FILE *f, Language lang) {
  // Find out the widest of all ports related to this instance port
  size_t maxCount = 0;
  for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
    Connection &c = (*ai)->m_connection;
    for (AttachmentsIter cai = c.m_attachments.begin(); cai != c.m_attachments.end(); cai++)
      if (&(*cai)->m_instPort != this && (*cai)->m_instPort.m_port->m_count > maxCount)
	maxCount = (*cai)->m_instPort.m_port->m_count;
  }
  // Output side: generate signal except when external or connected only to external
  // Or when connected to a wider one
  if (m_attachments.size() &&
      !(m_attachments.size() == 1 &&
	m_attachments.front()->m_connection.m_attachments.size() == 2 &&
	m_attachments.front()->m_connection.m_external) &&
      maxCount <= m_port->m_count &&
      (m_port->m_type != TimePort || m_port->m_master)) {
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
  // Like a WCI slave port array
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
}

const char *InstancePort::
adjustConnections(FILE *f, Language lang, size_t &unused) {
  // Create a global temp clock signal for this port if necessary.
  // Note the direction of a port-related clock signal is independent of the other port signals
  // or the port's data direction.
  // If this port (A) has its own clock, then it will already have a clock signal in its bundle, but
  // if its clock needs to be used for a connection to ANOTHER port (B) of the same instance that USES
  // this port's (A) clock, and the other port (B) is connected to a port (C) that requires a clock
  // as INPUT, then we must have a separate global signal for this port's clock (port A's clock)
  // for use by port C.
  Worker &w = *m_instance->m_worker;
  if (!m_external && m_port->m_myClock) // port A has its own clock
    for (unsigned i = 0; i < w.m_ports.size(); ++i)
      if (w.m_ports[i] != m_port && w.m_ports[i]->m_clock == m_port->m_clock) {
	// Port B uses port A's clock.  See if there is a port C that needs to see port A's clock
	InstancePort &b = m_instance->m_ports[i];
	for (auto ai = b.m_attachments.begin(); ai != b.m_attachments.end(); ++ai) {
	  Connection &conn = (**ai).m_connection;
	  for (auto ci = conn.m_attachments.begin(); ci != conn.m_attachments.end(); ++ci) {
	    InstancePort &c = (**ci).m_instPort;
	    if (!c.m_external && &c != &b && c.m_port->m_myClock && !c.m_port->m_clock->m_output) {
	      // We found a port C, connected to B, that needs to use this Port A's clock.
	      // The signal will either be driven by this worker (A's  myclock output)
	      // or it will be a copy of the signal that is driven into A's clock input
	      m_port->getClockSignal(*this, lang, c.m_clockSignal);
	      assert(conn.m_clockName.empty());
	      conn.m_clockName = c.m_clockSignal; // remember this for OCP adjustConnection
	      goto break2;
	    }
	  }
	}
      break2:;
      }
  // Adjust the details of the internal OCP signal assignments (we don't emit actual asignments here).
  for (AttachmentsIter ai = m_attachments.begin(); ai != m_attachments.end(); ai++) {
    Connection &c = (*ai)->m_connection;
    if (!c.m_external) // external connections have no adjustments by definition
      for (AttachmentsIter cai = c.m_attachments.begin(); cai != c.m_attachments.end(); cai++) {
	InstancePort &other = (*cai)->m_instPort;
	const char *err;
	if (&other != this &&
	    (err = m_port->adjustConnection(c, m_role.isProducer(), m_ocp, m_hasExprs, *other.m_port,
					    other.m_ocp, other.m_hasExprs, lang, unused)))
	  return OU::esprintf("For connection \"%s\" between instance \"%s\" worker \"%s\" "
			      "port \"%s\" to instance \"%s\" worker \"%s\" port \"%s\": %s",
			      c.cname(), m_instance->cname(), m_instance->m_worker->cname(),
			      m_port->pname(), other.m_instance->cname(),
			      other.m_instance->m_worker->cname(), other.m_port->pname(), err);
      }
  }
  if (m_hasExprs) {
    assert(m_signalIn.empty()); // make sure we did not make an input-side temp signal earlier
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
			    pr->m_name, pv->value, value, false, true));
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
		  pr->m_name.c_str(), verilogValue(pv->value, value, true));
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
    fprintf(f, "    port map( ");
  any = false;
  const char *indent = "              ";
  // For the instance, define the clock signals that are defined separate from
  // any interface/port.
  for (auto ci = i->m_worker->m_clocks.begin(); ci != i->m_worker->m_clocks.end(); ++ci) {
    Clock &c = **ci;
    if (!c.m_port && !c.m_internal) {
      if (lang == Verilog) {
	if (i->m_clocks[c.m_ordinal])
	  fprintf(f, "%s  .%s(%s)", any ? ",\n" : "", c.signal(),
		  i->m_clocks[c.m_ordinal]->signal());
	if (c.m_reset.size())
	  fprintf(f, ",\n  .%s(%s)", c.reset(),
		  i->m_clocks[c.m_ordinal]->reset());
      } else if (i->m_clocks[c.m_ordinal]) {
	fprintf(f, "%s%s%s => %s", any ? ",\n" : "", any ? indent : "",
		c.signal(), i->m_clocks[c.m_ordinal]->signal());
	if (c.m_reset.size())
	  fprintf(f, ",\n%s%s => %s", indent, c.reset(),
		  i->m_clocks[c.m_ordinal]->reset());
      } else {
	fprintf(f, "%s%s%s => '0'", any ? ",\n" : "", any ? indent : "",
		c.signal());
	if (c.m_reset.size())
	  fprintf(f, ",\n%s%s => '1'", indent, c.reset());
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
#if 1
    ip->m_port->emitPortSignals(f, *ip, lang, indent, any, comment, last, myComment(), exprs);
#else
    ip->m_port->emitPortSignals(f, ip->m_attachments, lang, indent, any, comment, last,
				myComment(), ip->m_ocp,
				ip->m_hasExprs ? &ip->m_signalIn : NULL, ip->m_clockSignal, exprs);
#endif
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
  for (auto si = i->m_worker->m_signals.begin(); si != i->m_worker->m_signals.end(); si++) {
    Signal &s = **si;
    if (s.m_direction == Signal::UNUSED)
      continue;
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
	} else if (!s.m_pin &&
		   (s.m_direction == Signal::INOUT || s.m_direction == Signal::OUTIN)) {
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
    if (anyMapped &&
	(s.m_pin || (s.m_direction != Signal::INOUT && s.m_direction != Signal::OUTIN)))
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
    } else if (!s.m_pin && (s.m_direction == Signal::INOUT || s.m_direction == Signal::OUTIN)) {
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
      if (lang == Verilog)
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
  // If instances have clocks that are not port-associated and are output we need them to be
  // local/internal signals
  bool first = true;
  Instance *i = &m_assembly->m_instances[0];
  for (unsigned n = 0; n < m_assembly->m_instances.size(); n++, i++) {
    for (auto ci = i->m_worker->m_clocks.begin(); ci != i->m_worker->m_clocks.end(); ++ci) {
      Clock &c = **ci;
      if (!c.m_port && c.m_output) {
	if (first)
	  fprintf(f, "  %s Define signals for non-port output clocks from instances\n",
		  myComment());
	first = false;
	if (m_language == VHDL)
	  fprintf(f, "  signal %s_%s : std_logic;\n", i->cname(), c.signal());
	else
	  fprintf(f, "  wire %s_%s;\n", i->cname(), c.signal());
      }
    }
  }
  fprintf(f, "  %s Define signals for connections that are not externalized\n", myComment());
  if (m_language == Verilog)
    fprintf(f, "wire [255:0] nowhere; // for passing output ports\n");
  // Generate the intermediate signals for internal connections
  i = &m_assembly->m_instances[0];
  size_t unused = 0;
  for (unsigned n = 0; n < m_assembly->m_instances.size(); n++, i++) {
    for (unsigned nn = 0; nn < i->m_worker->m_ports.size(); nn++) {
      InstancePort &ip = i->m_ports[nn];
      assert(!ip.m_external);
      ip.createConnectionSignals(f, m_language);
    }
    // Generate internal signal for emulation implicit connections
    i->emitDeviceConnectionSignals(f, *this);
  }
  // For connections that connect externally, and do not already have internal temp signals
  // assigned to them, assign external port names as the internal signals of the connection
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
  // For worker clock outputs (when an internal connection has a driven clock that is used for an
  // external port) we need to drive the output signal from the internal signal
  for (auto ci = m_clocks.begin(); ci != m_clocks.end(); ++ci) {
    Clock &c = **ci;
    if (c.m_output && !c.m_internal && !c.m_port) {
      // while it is not externally associated with a port, it must be internally driven by one
      fprintf(f, "%s assign the global clock output from the port that is driving it\n",
	      myComment());
      if (m_language == VHDL)
	fprintf(f, "  %s <= %s.Clk;\n", c.signal(), c.cname());
      else
	fprintf(f, "  assign %s = %s__iClk;\n", c.signal(), c.cname());
    }
  }

  i = &m_assembly->m_instances[0];
  for (unsigned n = 0; n < m_assembly->m_instances.size(); n++, i++) {
    for (unsigned nn = 0; nn < i->m_worker->m_ports.size(); nn++) {
      InstancePort &ip = i->m_ports[nn];
      if (!ip.m_external) {
	if ((err = ip.adjustConnections(f, m_language, unused)))
	  return err;
	if (ip.m_port->isOCP() && ip.m_port->m_clock->m_port == NULL)
	  ip.m_clockSignal = i->m_clocks[ip.m_port->m_clock->m_ordinal]->m_signal;
      }
    }
  }
  // Adjust connection signals
  if (unused && m_language == VHDL)
    fprintf(f, "  signal unused : std_logic_vector(0 to %zu);\n", unused - 1);
  if (m_language == VHDL)
    fprintf(f, "begin\n");
  // Assign external signals where necessary
  for (ConnectionsIter ci = m_assembly->m_connections.begin();
       ci != m_assembly->m_connections.end(); ci++) {
    Connection &c = **ci;
    if (c.m_external) {
      InstancePort *internal = NULL;
      for (auto ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ++ai)
	if (*ai != c.m_external) {
	  internal = &(*ai)->m_instPort;;
	  break;
	}
      assert(internal);
      Port &p = *c.m_external->m_instPort.m_port;
      std::string &nameExt2In = p.m_master ? c.m_slaveName : c.m_masterName;
      if (internal->m_signalOut.size() && nameExt2In.size() && p.haveInputs())
	assignExt(f, c, nameExt2In, false);
      std::string &nameIn2Ext = p.m_master ? c.m_masterName : c.m_slaveName;
      if (internal->m_signalIn.size() && nameIn2Ext.size() && p.haveOutputs())
	assignExt(f, c, nameIn2Ext, true);
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
