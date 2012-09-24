/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
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

#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#define __STDC_LIMIT_MACROS // wierd standards goof up
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "OcpiUtilMisc.h"
#include "wip.h"

namespace OU = OCPI::Util;

/*
 * Todo:
 *  property values in assembly instances?
 */

const char *propertyTypes[] = {
  "none", // for OCPI_none
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #pretty,
OCPI_PROPERTY_DATA_TYPES
0};
#undef OCPI_DATA_TYPE

const char *controlOperations[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3)  #x,
OCPI_CONTROL_OPS
#undef CONTROL_OP
0};

const char *container = 0, *platform = 0, *device = 0, *load = 0;

// MyClock boolean simply says whether the clock is "homed" and "named" here.
// The clock attribute says that the clock is defined elsewhere
// The "elsewhere" is either a port that has its own clock or a global definition.
static const char *
checkClock(Worker *w, ezxml_t impl, Port *p) {
  const char *err;
  const char *clock = 0;
  if (impl) {
    clock = ezxml_cattr(impl, "Clock");
    if ((err = OE::getBoolean(impl, "MyClock", &p->myClock)))
      return err;
  }
  if (!clock) {
    if (p->myClock || p->type == WCIPort) {
      // If port has its own clock, or is a WCI, establish a clock named and homed here
      p->myClock = true;
      p->clock = &w->clocks[w->nClocks++];
      asprintf((char **)&p->clock->name, "%s_Clk", p->name); // fixme
      p->clock->port = p;
    } else if (w->ports[0]->type == WCIPort) {
      // If no clock, and we have a WCI then assume the WCI's clock.
      p->clockPort = w->ports[0];
    } else
      // If no clock, and no wci port, we're hosed.
      return "Interface has no clock declared, and there is no control interface";
  } else {
    // Port refers to another clock by name
    for (unsigned i = 0; i < w->ports.size(); i++) {
      Port *op = w->ports[i];
      if (p != op && !strcmp(clock, op->name)) {
        if (p->myClock)
          // Can't refer to another port and also own the clock
          return OU::esprintf("Clock for interface \"%s\" refers to interface \"%s\","
                          " and also has MyClock=true?",
                          p->name, clock);
        p->clockPort = op;
        return 0;
      }
    }
    // We are not referring to another port.  It muts be a defined clock
    Clock *c = w->clocks;
    for (unsigned i = 0; i < w->nClocks; i++, c++)
      if (!strcmp(clock, c->name)) {
        p->clock = c;
        if (p->myClock)
          if (c->port)
            return OU::esprintf("Clock for interface \"%s\", \"%s\" is already owned by interface \"%s\"",
                            p->name, clock, c->port->name);
          else
            c->port = p;
        return 0;
      }
    return OU::esprintf("Clock attribute of \"%s\" matches no interface or clock", p->name);
  }
  return 0;
}

// Check for implementation attributes common to data interfaces, several of which
// are able to override protocol-determined values.
static const char *
checkDataPort(Worker *w, ezxml_t impl, Port **dpp) {
  const char *err;
  const char *name = ezxml_cattr(impl, "Name");
  if (!name)
    return OU::esprintf("Missing \"Name\" attribute of %s element", impl->name);
  unsigned i;
  Port *dp = 0;
  for (i = 0; i < w->ports.size(); i++) {
    dp = w->ports[i];
    if (dp && dp->name && !strcmp(dp->name, name))
      break;
  }
  if (i >= w->ports.size() || dp && !dp->isData)
    return
      OU::esprintf("Name attribute of Stream/MessageInterface \"%s\" "
               "does not match a DataInterfaceSpec", name);
  if ((err = checkClock(w, impl, dp)) ||
      (err = OE::getNumber(impl, "DataWidth", &dp->dataWidth, 0, dp->protocol->m_dataValueWidth)) ||
      (err = OE::getNumber(impl, "NumberOfOpcodes", &dp->u.wdi.nOpcodes,
                           0, dp->u.wdi.nOpcodes)) ||
      (err = OE::getNumber(impl, "MaxMessageValues", &dp->protocol->m_maxMessageValues,
                           0, dp->protocol->m_maxMessageValues)) ||
      (err = OE::getBoolean(impl, "Continuous", &dp->u.wdi.continuous)) ||
      (err = OE::getBoolean(impl, "ImpreciseBurst", &dp->impreciseBurst)) ||
      (err = OE::getBoolean(impl, "PreciseBurst", &dp->preciseBurst)))
    return err;
  if (dp->dataWidth >= dp->protocol->m_dataValueWidth) {
    if (dp->dataWidth % dp->protocol->m_dataValueWidth)
      return OU::esprintf("DataWidth (%u) on port '%s' not a multiple of DataValueWidth (%u)",
			  dp->dataWidth, dp->name, dp->protocol->m_dataValueWidth);
  } else if (dp->protocol->m_dataValueWidth % dp->dataWidth)
      return OU::esprintf("DataValueWidth (%u) on port '%s' not a multiple of DataWidth (%u)",
			  dp->protocol->m_dataValueWidth, dp->name, dp->dataWidth);
#if 0
  if (dp->impreciseBurst && dp->preciseBurst)
    return "Both ImpreciseBurst and PreciseBurst cannot be specified for WSI or WMI";
#endif
  dp->pattern = ezxml_cattr(impl, "Pattern");
  *dpp = dp;
  return 0;
}

// If the given element is xi:include, then parse it and return the parsed element.
// If not, *parsed is set to zero.
// Also return the file name of the included file.
static const char *
tryInclude(ezxml_t top, const char *parent, const char *element, ezxml_t *parsed,
           const char **child, bool optional = false) {
  const char *err;
  ezxml_t x = ezxml_child(top, "xi:include");
  if (x) {
    if ((err = OE::checkAttrs(x, "href", (void*)0)))
      return err;
    const char *ifile = ezxml_cattr(x, "href");
    if (!ifile)
      return OU::esprintf("xi:include missing an href attribute in file \"%s\"", parent);
    ezxml_t i = 0;
    if ((err = parseFile(ifile, parent, element, &i, &ifile, optional)))
      return err;
    *parsed = i;
    *child = ifile;
  } else {
    if (child)
      *child = 0;
    *parsed = 0;
  }
  return 0;
}

// The top element should have either a child "element" or
// an xi:include of such an element.
// If success, set *parsed, and maybe *childFile.
static const char *
tryChildInclude(ezxml_t top, const char *parent, const char *element,
                ezxml_t *parsed, const char **childFile, bool optional = false) {
  const char *err = tryInclude(top, parent, element, parsed, childFile, optional);
  if (err || *parsed)
    return err;
  // No xi:include, of the right type, try to find it directly
  if ((*parsed = ezxml_cchild(top, element))) {
    if (childFile)
      *childFile = parent;
    return 0;
  }
  return
    optional ? 0 :
    OU::esprintf("Neither %s nor xi:include found under %s in file \"%s\"",
             element, top->name, parent);
}


static const char *
addProperty(Worker *w, ezxml_t prop, bool includeImpl)
{
  w->ctl.properties.push_back(new OU::Property);
  return w->ctl.properties.back()->
    parse(prop, w->ctl.offset, w->ctl.readableConfigProperties,
	  w->ctl.writableConfigProperties, w->ctl.sub32BitConfigProperties,
	  includeImpl, w->ctl.ordinal++);
  
}

// Generic implementation properties
// Called both for counting and for filling out
static const char *
doImplProp(ezxml_t prop, void *arg) {
  const char *err;
  Worker *w = (Worker *)arg;
  const char *eName = ezxml_name(prop);
  bool isSpec = eName && !strcasecmp(eName, "SpecProperty");
  if (!eName || (strcasecmp(eName, "Property") && !isSpec))
    return 0;
  const char *name = ezxml_cattr(prop, "Name");
  if (!name)
    return "Property or SpecProperty in ControlInterface has no \"Name\" attribute";
  // See if it matches
  OU::Property *p = NULL;
  for (PropertiesIter pi = w->ctl.properties.begin(); pi != w->ctl.properties.end(); pi++)
    if ((*pi)->m_name == name) {
      p = *pi;
      break;
    }
  if (p) {
    if (!isSpec)
      return OU::esprintf("Implementation property named \"%s\" conflict with spec property",
                      name);
    if (p->m_defaultValue && ezxml_cattr(prop, "Default"))
      return OU::esprintf("Implementation property named \"%s\" cannot override "
		      "previous default value", name);
    if ((err = p->parseImpl(prop)))
      return err;
  } else {
    if (isSpec)
      return OU::esprintf("Specification property named \"%s\" not found in spec", name);
    // All the spec attributes plus the impl attributes
    if ((err = addProperty(w, prop, true)))
      return err;
  }
  return 0;
}
// Do top level properties mixed with other children
static const char *
doTopProp(ezxml_t prop, void *arg) {
  // Now actually process a property element
  const char *name = ezxml_name(prop);
  if (!name || strcasecmp(name, "Property"))
    return NULL;
  Worker *w = (Worker *)arg;
  const char *err;
  if (!(err = OE::checkAttrs(prop, OCPI_UTIL_MEMBER_ATTRS, "Readable", "Writable", "IsTest",
			    "Default", NULL)))
    err = addProperty(w, prop, false);
  return err;
}


// Process one element of a properties element, checking for xi:includes
static const char *
doSpecProp(ezxml_t prop, void *arg) {
  const char *err, *ifile;
  Worker *w = (Worker *)arg;
  ezxml_t iprop = 0;
  if ((err = tryInclude(prop, w->file, "Properties", &iprop, &ifile)))
    return err;
  // If it is an "include", basically recurse
  if (iprop) {
    const char *ofile = w->file;
    w->file = ifile;
    err = OE::ezxml_children(iprop, doSpecProp, arg);
    w->file = ofile;
    return err;
  }
  const char *name = ezxml_name(prop);
  if (!name || strcasecmp(name, "Property"))
    return "Element under Properties is neither Property or xi:include";
  return doTopProp(prop, (void*)w);
}

// parse an attribute value as a list separated by comma, space or tab
// and call a function with the given arg for each token found
static const char *parseList(const char *list,
                             const char * (*doit)(const char *tok, void *arg),
                             void *arg) {
  const char *err = 0;
  if (list) {
    char
      *mylist = strdup(list),
      *base = mylist,
      *last = 0,
      *tok;
    for (base = mylist; (tok = strtok_r(base, ", \t", &last)); base = NULL)
      if ((err = doit(tok, arg)))
        break;
    free(mylist);
  }
  return err;
}

static const char *parseControlOp(const char *op, void *arg) {
  Worker *w = (Worker *)arg;
  unsigned n = 0;
  const char **p;
  for (p = controlOperations; *p; p++, n++)
    if (!strcasecmp(*p, op)) {
      w->ctl.controlOps |= 1 << n;
      break;
    }
  return
    *p ? NULL : "Invalid control operation name in ControlOperations attribute";
}

// Parse the generic implementation control aspects (for rcc and hdl and other)
#define GENERIC_IMPL_CONTROL_ATTRS \
  "SizeOfConfigSpace", "ControlOperations", "Sub32BitConfigProperties"
const char *
parseImplControl(ezxml_t impl, const char *file, Worker *w, ezxml_t &xctl) {
  // Now we do the rest of the control interface
  xctl = ezxml_cchild(impl, "ControlInterface");
  const char *err;
  if (xctl) {
    if (w->noControl)
      return "Worker has a ControlInterface element, but also has NoControl=true";
    // Allow overriding byte enables
    bool sub32;
    if ((err = OE::getBoolean(xctl, "Sub32BitConfigProperties", &sub32)))
      return err;
    if (sub32)
      w->ctl.sub32BitConfigProperties = true;
#if 1
    if ((err = parseList(ezxml_cattr(xctl, "ControlOperations"), parseControlOp, w)))
      return err;
#else
    const char *ops = ezxml_cattr(xctl, "ControlOperations");
    if (ops) {
      char *last = 0, *o;
      while ((o = strtok_r((char *)ops, ", \t", &last))) {
        ops = 0;
        unsigned op = 0;
        const char **p;
        for (p = controlOperations; *p; p++, op++)
          if (!strcasecmp(*p, o)) {
            w->ctl.controlOps |= 1 << op;
            break;
          }
        if (!*p)
          return "Invalid control operation name in ControlOperations attribute";
      }
    }
#endif
    ezxml_t props;
    if ((err = tryChildInclude(xctl, file, "Properties", &props, NULL, true)))
      return err;
    // Properties might be in a "properties" element, maybe via xi:include
    if (props) {
      if ((err = OE::ezxml_children(props, doImplProp, w)))
        return err;
    } else
      // Properties might also be directly under ControlInterface for simplicity
      if ((err = OE::ezxml_children(xctl, doImplProp, w)))
        return err;
  }
  // parseing the impl control interface means we have visited all the properties,
  // both spec and impl, so now we know the whole config space.
  if (w->ctl.offset > w->ctl.sizeOfConfigSpace)
    w->ctl.sizeOfConfigSpace = w->ctl.offset;
  // Allow overriding sizeof config space
  if (xctl) {
    uint64_t sizeOfConfigSpace;
    bool haveSize;
    if ((err = OE::getNumber64(xctl, "SizeOfConfigSpace", &sizeOfConfigSpace, &haveSize, 0)))
      return err;
    if (haveSize) {
      if (sizeOfConfigSpace < w->ctl.sizeOfConfigSpace)
        return "SizeOfConfigSpace attribute of ControlInterface smaller than properties";
      w->ctl.sizeOfConfigSpace = sizeOfConfigSpace;
    }
  }
  return 0;
}

// Parse the generic implementation local memories (for rcc and ocl and other)
const char*
parseImplLocalMemory(ezxml_t impl, Worker *w) {
  const char* err;
  for (ezxml_t x = ezxml_cchild(impl, "LocalMemory"); x; x = ezxml_next(x)) {
    LocalMemory* m = new LocalMemory();
    w->localMemories.push_back(m);
    if ((err = OE::checkAttrs(x, "Name", "SizeofLocalMemory", (void*)0)) )
      return err;
    m->name = ezxml_cattr(x, "Name");
    if (!m->name)
      return "Missing \"Name\" attribute on Local Memory element if OclImplementation";
    if ((err = OE::getNumber(x, "SizeOfLocalMemory", &m->sizeOfLocalMemory, 0, 0)))
      return err;
  }
  return 0;
}

// Parse the control information about the component spec
const char *
parseSpecControl(Worker *w, ezxml_t ps, ezxml_t props, ezxml_t spec) {
  const char *err;
  if (ps) {
    if ((err = OE::checkAttrs(ps, "SizeOfConfigSpace", "WritableConfigProperties",
                          "ReadableConfigProperties", "Sub32BitConfigProperties",
                          "Count", (void*)0)) ||
        (err = OE::getNumber64(ps, "SizeOfConfigSpace", &w->ctl.sizeOfConfigSpace, 0, 0)) ||
        (err = OE::getBoolean(ps, "WritableConfigProperties", &w->ctl.writableConfigProperties)) ||
        (err = OE::getBoolean(ps, "ReadableConfigProperties", &w->ctl.readableConfigProperties)) ||
        (err = OE::getBoolean(ps, "Sub32BitConfigProperties", &w->ctl.sub32BitConfigProperties)))
      return err;
  } else if (props) {
    // No property summary, must have something else.
    if ((err = OE::ezxml_children(props, doSpecProp, w)))
      return err;
  } else if ((err = OE::ezxml_children(spec, doTopProp, w)))
    return err;
  return 0;
}

static const char *checkSuffix(const char *str, const char *suff, const char *last) {
  unsigned nstr = last - str, nsuff = strlen(suff);
  const char *start = str + nstr - nsuff;
  return nstr > nsuff && !strncmp(suff, start, nsuff) ? start : str + nstr;
}

Protocol::Protocol(Port &port)
  : m_port(port) {}

const char *
Protocol::parse(const char *file, ezxml_t prot)
{
  if (file) {
    // If we are being parsed from a protocol file, default the name.
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
    m_name.assign(start, last - start);
  }
  return prot ? OU::Protocol::parse(prot) : NULL;
}

const char *Protocol::parseOperation(ezxml_t op) {
  const char *err, *ifile;
  ezxml_t iprot = 0;
  if ((err = tryInclude(op, m_port.worker->file, "Protocol", &iprot, &ifile)))
    return err;
  // If it is an "include", basically recurse
  if (iprot) {
    const char *ofile = m_port.worker->file;
    m_port.worker->file = ifile;
    err = OU::Protocol::parse(iprot);
    m_port.worker->file = ofile;
    return err;
  }
  return OU::Protocol::parseOperation(op);
}

const char *
parseSpec(ezxml_t xml, const char *file, Worker *w) {
  const char *err;
  // xi:includes at this level are component specs, nothing else can be included
  ezxml_t spec;
  if ((err = tryChildInclude(xml, w->file, "ComponentSpec", &spec, &w->specFile)))
    return err;
  w->specName = ezxml_cattr(spec, "Name");
  if (!w->specName)
    return "Missing Name attribute for ComponentSpec";
  if ((err = OE::checkAttrs(spec, "Name", "NoControl", (void*)0)) ||
      (err = OE::getBoolean(spec, "NoControl", &w->noControl)))
    return err;
  // Parse control port info
  ezxml_t ps, props;
  if ((err = tryChildInclude(spec, file, "PropertySummary", &ps, NULL, true)))
    return err;
  if (ps) {
    if (ezxml_cchild(spec, "Properties") || ezxml_cchild(spec, "Property"))
      return "cannot have both PropertySummary and Properties";
    props = 0;
  } else if ((err = tryChildInclude(spec, file, "Properties", &props, NULL, true)))
    return err;
  if (w->noControl) {
    if (ps || props || OE::countChildren(spec, "property"))
      return "NoControl specified, PropertySummary, Properties, Property cannot be specified";
  } else if ((err = parseSpecControl(w, ps, props, spec)))
    return err;
  // Now parse the data aspects, allocating (data) ports.
  for (ezxml_t x = ezxml_cchild(spec, "DataInterfaceSpec"); x; x = ezxml_next(x)) {
    Port *p = new Port();
    w->ports.push_back(p);
    if ((err = OE::checkAttrs(x, "Name", "Producer", "Count", "Optional", (void*)0)) ||
        (err = OE::getBoolean(x, "Producer", &p->u.wdi.isProducer)) ||
        (err = OE::getBoolean(x, "Optional", &p->u.wdi.isOptional)))
      return err;
    p->worker = w;
    p->isData = true;
    p->type = WDIPort;
    if (!(p->name = ezxml_cattr(x, "Name")))
      return "Missing \"Name\" attribute in DataInterfaceSpec";
    for (unsigned i = 0; i < w->ports.size(); i++) {
      Port *pp = w->ports[i];
      if (pp != p && !strcmp(pp->name, p->name))
        return "DataInterfaceSpec Name attribute duplicates another interface name";
    }
    ezxml_t pSum;
    const char *protFile = 0;
    if ((err = tryChildInclude(x, file, "ProtocolSummary", &pSum, &protFile, true)))
      return err;
    Protocol *prot = p->protocol = new Protocol(*p);
    if (pSum) {
      if (ezxml_cchild(spec, "Protocol"))
        return "cannot have both Protocol and ProtocolSummary";
      if ((err = OE::checkAttrs(pSum, "DataValueWidth", "DataValueGranularity",
				"DiverDataSizes", "MaxMessageValues", "NumberOfOpcodes",
				"VariableMessageLength", "ZeroLengthMessages",
				"MinMessageValues",  (void*)0)) ||
	  (err = OE::getNumber(pSum, "NumberOfOpcodes", &p->u.wdi.nOpcodes, 0, 1)) ||
	  (err = prot->parseSummary(pSum)))
	return err;
    } else {
      ezxml_t protx = NULL;
      // FIXME: default protocol name from file name
      if ((err = tryChildInclude(x, file, "Protocol", &protx, &protFile, true)))
        return err;
      if (protx) {
        if ((err = prot->parse(protFile, protx)))
          return err;
        // So if there is a protocol, nOpcodes is initialized from it.
        p->u.wdi.nOpcodes = p->protocol->nOperations();
      } else {
	// When there is no protocol at all, we force it to variable, bounded at 64k, diverse, zlm
	prot->m_diverseDataSizes = true;
	prot->m_variableMessageLength = true;
	prot->m_maxMessageValues = 64*1024;
	prot->m_zeroLengthMessages = true;
        p->u.wdi.nOpcodes = 1;
      }
    }
  }
  return 0;
}

const char *
parseHdlImpl(ezxml_t xml, const char *file, Worker *w) {
  const char *err;
  ezxml_t xctl;
  if ((err = parseSpec(xml, file, w)) ||
      (err = parseImplControl(xml, file, w, xctl)))
    return err;
  Port *wci;
  if (!w->noControl) {
    // Insert the control port at the beginning of the port list since we want
    // To always process the control port first if we have one
    wci = new Port();
    w->ports.insert(w->ports.begin(), wci);
    // Finish HDL-specific control parsing
    if (w->ctl.controlOps == 0)
      w->ctl.controlOps = 1 << ControlOpStart;
    if (xctl) {
      if ((err = OE::checkAttrs(xctl, GENERIC_IMPL_CONTROL_ATTRS, "ResetWhileSuspended",
                            "Clock", "MyClock", "Timeout", "Count", "Name", "Pattern",
                            (void *)0)) ||
          (err = OE::getNumber(xctl, "Timeout", &wci->u.wci.timeout, 0, 0)) ||
          (err = OE::getNumber(xctl, "Count", &wci->count, 0, 0)) ||
          (err = OE::getBoolean(xctl, "ResetWhileSuspended",
                            &wci->u.wci.resetWhileSuspended)))
        return err;
      wci->pattern = ezxml_cattr(xctl, "Pattern");
      wci->name = ezxml_cattr(xctl, "Name");
    }
    if (!wci->count)
      wci->count = 1;
    // clock processing depends on the name so it must be defaulted here
    if (!wci->name)
      wci->name = "ctl";
    wci->type = WCIPort;
  } else
    wci = 0;
#if 0
  // Count up and allocate the ports that are HDL-specific
  for (ezxml_t x = ezxml_cchild(xml, "MemoryInterface"); x; x = ezxml_next(x))
    nMem++;
  for (ezxml_t x = ezxml_cchild(xml, "TimeInterface"); x; x = ezxml_next(x))
    nTime++;
  extraPorts += nMem + nTime;
  unsigned nextPort = w->ports.size();
  w->ports.resize(w->ports.size() + extraPorts);
#endif
  unsigned oldSize = w->ports.size(); // remember the base of extra ports
  unsigned nMem = 0, nTime = 0, memOrd = 0, timeOrd = 0;
  // Clocks depend on port names, so get those names in first pass(non-control ports)
  for (ezxml_t x = ezxml_cchild(xml, "MemoryInterface"); x;
       x = ezxml_next(x), memOrd++) {
    Port *p = new Port();
    w->ports.push_back(p);
    if (!(p->name = ezxml_cattr(x, "Name")))
      if (nMem == 1)
        p->name = "mem";
      else
        asprintf((char **)&p->name, "mem%u", memOrd);
  }
  for (ezxml_t x = ezxml_cchild(xml, "TimeInterface"); x;
       x = ezxml_next(x), timeOrd++) {
    Port *p = new Port();
    w->ports.push_back(p);
    if (!(p->name = ezxml_cattr(x, "Name")))
      if (nTime == 1)
        p->name = "time";
      else
        asprintf((char **)&p->name, "time%u", memOrd);
  }
  // Now we do clocks before interfaces since they may refer to clocks
  for (ezxml_t xc = ezxml_cchild(xml, "Clock"); xc; xc = ezxml_next(xc))
    w->nClocks++;
  // add one to allow for adding the WCI clock later
  w->clocks = myCalloc(Clock, w->nClocks + 1 + w->ports.size());
  Clock *c = w->clocks;
  for (ezxml_t xc = ezxml_cchild(xml, "Clock"); xc; xc = ezxml_next(xc), c++) {
    if ((err = OE::checkAttrs(xc, "Name", "Signal", "Home", (void*)0)))
      return err;
    c->name = ezxml_cattr(xc, "Name");
    if (!c->name)
      return "Missing Name attribute in Clock subelement of ComponentImplementation";
    c->signal = ezxml_cattr(xc, "Signal");
  }
  // Now that we have clocks roughly set up, we process the wci clock
  if (wci && (err = checkClock(w, xctl, wci)))
    return err;
  // End of control interface/wci processing (except OCP signal config)

  // Prepare to process data plane port implementation info
  // Now lets look at the implementation-specific data interface info
  Port *dp;
  for (ezxml_t s = ezxml_cchild(xml, "StreamInterface"); s; s = ezxml_next(s)) {
    if ((err = OE::checkAttrs(s, "Name", "Clock", "DataWidth", "PreciseBurst",
                              "ImpreciseBurst", "Continuous", "Abortable",
                              "EarlyRequest", "MyClock", "RegRequest", "Pattern",
                              "NumberOfOpcodes", "MaxMessageValues",
                              (void*)0)) ||
        (err = checkDataPort(w, s, &dp)) ||
        (err = OE::getBoolean(s, "Abortable", &dp->u.wsi.abortable)) ||
        (err = OE::getBoolean(s, "RegRequest", &dp->u.wsi.regRequest)) ||
        (err = OE::getBoolean(s, "EarlyRequest", &dp->u.wsi.earlyRequest)))
      return err;
    dp->type = WSIPort;
  }
  for (ezxml_t m = ezxml_cchild(xml, "MessageInterface"); m; m = ezxml_next(m)) {
    if ((err = OE::checkAttrs(m, "Name", "Clock", "MyClock", "DataWidth",
                              "PreciseBurst", "MFlagWidth", "ImpreciseBurst",
                              "Continuous", "ByteWidth", "TalkBack",
                              "Bidirectional", "Pattern",
                              "NumberOfOpcodes", "MaxMessageValues",
                              (void*)0)) ||
        (err = checkDataPort(w, m, &dp)) ||
        (err = OE::getNumber(m, "ByteWidth", &dp->byteWidth, 0, dp->dataWidth)) ||
        (err = OE::getBoolean(m, "TalkBack", &dp->u.wmi.talkBack)) ||
        (err = OE::getBoolean(m, "Bidirectional", &dp->u.wdi.isBidirectional)) ||
        (err = OE::getNumber(m, "MFlagWidth", &dp->u.wmi.mflagWidth, 0, 0)))
      return err;
    dp->type = WMIPort;
  }
  // Final pass over all data ports for defaulting and checking
  for (unsigned i = 0; i < w->ports.size(); i++) {
    dp = w->ports[i];
    switch (dp->type) {
    case WDIPort:
      // For data ports that have not been specified as stream or message,
      // default to imprecise stream clocked by the WSI, with data width implied from protocol.
      dp->type = WSIPort;
      dp->dataWidth = dp->protocol->m_dataValueWidth;
      dp->impreciseBurst = true;
      if (w->ports[0]->type == WCIPort)
	dp->clockPort = w->ports[0];
      else
	return "A data port that defaults to WSI must be in a worker with a WCI";
      // fall into
    case WSIPort:
    case WMIPort:
      {
	// If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
	unsigned granuleWidth =
	  dp->protocol->m_dataValueWidth * dp->protocol->m_dataValueGranularity;
	// If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
	if (granuleWidth >= dp->dataWidth && (granuleWidth % dp->dataWidth) == 0 && 
	    !dp->protocol->m_zeroLengthMessages)
	  dp->byteWidth = dp->dataWidth;
	else
	  dp->byteWidth = dp->protocol->m_dataValueWidth;
      }
      if (dp->dataWidth % dp->byteWidth)
	return "Specified ByteWidth does not divide evenly into specified DataWidth";
      break;
    default:;
    }
  }
  unsigned nextPort = oldSize;
  for (ezxml_t m = ezxml_cchild(xml, "MemoryInterface"); m; m = ezxml_next(m), nextPort++) {
    Port *mp = w->ports[nextPort];
    mp->type = WMemIPort;
    bool memFound = false;
    if ((err = OE::checkAttrs(m, "Name", "Clock", "DataWidth", "PreciseBurst", "ImpreciseBurst",
                              "MemoryWords", "ByteWidth", "MaxBurstLength", "WriteDataFlowControl",
                              "ReadDataFlowControl", "Count", "Pattern", "Slave", (void*)0)) ||
        (err = checkClock(w, m, mp)) ||
        (err = OE::getNumber(m, "Count", &mp->count, 0, 0)) ||
        (err = OE::getNumber64(m, "MemoryWords", &mp->u.wmemi.memoryWords, &memFound, 0)) ||
        (err = OE::getNumber(m, "DataWidth", &mp->dataWidth, 0, 8)) ||
        (err = OE::getNumber(m, "ByteWidth", &mp->byteWidth, 0, 8)) ||
        (err = OE::getNumber(m, "MaxBurstLength", &mp->u.wmemi.maxBurstLength, 0, 0)) ||
        (err = OE::getBoolean(m, "Slave", &mp->u.wmemi.isSlave)) ||
        (err = OE::getBoolean(m, "ImpreciseBurst", &mp->impreciseBurst)) ||
        (err = OE::getBoolean(m, "PreciseBurst", &mp->preciseBurst)) ||
        (err = OE::getBoolean(m, "WriteDataFlowControl", &mp->u.wmemi.writeDataFlowControl)) ||
        (err = OE::getBoolean(m, "ReadDataFlowControl", &mp->u.wmemi.readDataFlowControl)))
      return err;
    if (!memFound || !mp->u.wmemi.memoryWords)
      return "Missing \"MemoryWords\" attribute in MemoryInterface";
    if (!mp->preciseBurst && !mp->impreciseBurst) {
      if (mp->u.wmemi.maxBurstLength > 0)
        return "MaxBurstLength specified when no bursts are enabled";
      if (mp->u.wmemi.writeDataFlowControl || mp->u.wmemi.readDataFlowControl)
        return "Read or WriteDataFlowControl enabled when no bursts are enabled";
    }
    if (mp->byteWidth < 8 || mp->dataWidth % mp->byteWidth)
      return "Bytewidth < 8 or doesn't evenly divide into DataWidth";
    mp->name = ezxml_cattr(m, "Name");
    if (!mp->name)
      mp->name = "mem";
    mp->pattern = ezxml_cattr(m, "Pattern");
  }
  bool foundWTI = false;
  for (ezxml_t m = ezxml_cchild(xml, "TimeInterface"); m; m = ezxml_next(m), nextPort++) {
    Port *mp = w->ports[nextPort];
    if (foundWTI)
      return "More than one WTI specified, which is not permitted";
    mp->name = ezxml_cattr(m, "Name");
    if (!mp->name)
      mp->name = "time";
    mp->type = WTIPort;
    if ((err = OE::checkAttrs(m, "Name", "Clock", "SecondsWidth", "FractionWidth", "AllowUnavailable", "Pattern",
                          (void*)0)) ||
        (err = checkClock(w, m, mp)) ||
        (err = OE::getNumber(m, "SecondsWidth", &mp->u.wti.secondsWidth, 0, 32)) ||
        (err = OE::getNumber(m, "FractionWidth", &mp->u.wti.fractionWidth, 0, 0)) ||
        (err = OE::getBoolean(m, "AllowUnavailable", &mp->u.wti.allowUnavailable)))
      return err;
    mp->dataWidth = mp->u.wti.secondsWidth + mp->u.wti.fractionWidth;
    foundWTI = true;
    mp->pattern = ezxml_cattr(m, "Pattern");
  }
  // Now check that all clocks have a home and all ports have a clock
  c = w->clocks;
  for (unsigned i = 0; i < w->nClocks; i++, c++)
    if (c->port) {
      if (c->signal)
        return OU::esprintf("Clock %s is owned by interface %s and has a signal name",
                        c->name, c->port->name);
      //asprintf((char **)&c->signal, "%s_Clk", c->port->fullNameIn);
    } else if (!c->signal)
      return OU::esprintf("Clock %s is owned by no port and has no signal name",
                      c->name);
  // now make sure clockPort references are sorted out
  for (unsigned i = 0; i < w->ports.size(); i++) {
    Port *p = w->ports[i];
    if (p->clockPort)
      p->clock = p->clockPort->clock;
    if (p->count == 0)
      p->count = 1;
  }
  // process ad hoc signals
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_next(xs)) {
    if ((err = OE::checkAttrs(xs, "Input", "Output", "Inout", "Width", (void*)0)))
      return err;
    w->nSignals++;
  }
  if (w->nSignals) {
    Signal *s = w->signals = myCalloc(Signal, w->nSignals);
    for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_next(xs), s++) {
      if ((s->name = ezxml_cattr(xs, "Input")))
        s->direction = Signal::IN;
      else if ((s->name = ezxml_cattr(xs, "Output")))
        s->direction = Signal::OUT;
      else if ((s->name = ezxml_cattr(xs, "Inout")))
        s->direction = Signal::INOUT;
      else
        s->direction = Signal::IN;
      if ((err = OE::getNumber(xs, "Width", &s->width, 0, 0)))
        return err;
    }
  }
  return 0;
}


const char *
getWorker(Assembly *a, ezxml_t x, const char *aName, Worker **wp) {
  const char *wName = ezxml_cattr(x, aName);
  if (!wName)
    return OU::esprintf("Missing \"%s\" attribute on connection", aName);
  for (WorkersIter wi = a->workers.begin(); wi != a->workers.end(); wi++)
    if (!strcmp(wName, (*wi)->implName)) {
      *wp = (*wi);
      return 0;
    }
  return OU::esprintf("Attribute \"%s\": Worker name \"%s\" not foundr",
                  aName, wName);
}

const char *
getPort(Worker *w, ezxml_t x, const char *aName, Port **pp) {
  const char *pName = ezxml_cattr(x, aName);
  if (!pName)
    return OU::esprintf("Missing \"%s\" attribute for worker \"%s\"",
                    aName, w->implName);
  for (unsigned i = 0; i < w->ports.size(); i++) {
    Port *p = w->ports[i];
    if (!strcmp(pName, p->name)) {
      *pp = p;
      return 0;
    }
  }
  return OU::esprintf("Port name \"%s\" matches no port on worker \"%s\"",
                  pName, w->implName);
}

const char *
getConnPort(ezxml_t x, Assembly *a, const char *wAttr, const char *pAttr,
            Port **pp) {
  const char *err;
  Worker *w;
  if ((err = getWorker(a, x, wAttr, &w)))
    return err;
  return getPort(w, x, pAttr, pp);
}

// Attach an instance port to a connection
static void
attachPort(Connection *c, InstancePort *ip, const char *name,
           const char *externalRole) {
  if (externalRole)
    c->nExternals++;
#if 0
  if (isProducer)
    c->nProducers++;
  else if (isBidirectional)
    c->nBidirectionals++;
  else
    c->nConsumers++;
#endif
  // Append to list for connection
  InstancePort **pp;
  for (pp = &c->ports; *pp; pp = &(*pp)->nextConn)
    ;
  *pp = ip;
  ip->connection = c;
  ip->externalRole = externalRole;
  ip->name = name;
  c->nPorts++;
  if (externalRole)
    c->external = ip; // last one if there are more than one
}

#if 0

// We have a port clock on one of the underlying workers.
// We need to decide whether to coalesce it or surface it on its own.
// If the port's clock is WCI, its easy, otherwise,
// If it not its own clock, then it follows the clock it shares
// Otherwise we have a "new" clock to deal with and we have to consider
// whether it should be coalesced or be its own clock.
const char *
doAssyClock(Worker *aw, Instance *i, Port *p) {
  unsigned nc = p->clock - i->worker->clocks;
  Clock *aClock;
  if (i->clocks[nc])
    // If the instance's clock for this worker clock is already mapped, use it
    aClock = i->clocks[nc];
  else if (p->clock->port->type == WCIPort) {
    // If the port uses its worker's wci clock, then use the assembly's wci
    i->clocks[nc] = aw->ports[0]->clock;
    aClock = i->clocks[nc];
  } else {
    // Some principles:
    // An assembly is already an implementation specific thing here
    // How do we have assembly re-use while optimizing based on infrastructure
    // AUTOMATION:  marry to CTOP, must describe ctop first.
    // Negotiation between offered stuff from CTOP to needs of OCAPP.
    // Does this mean we must postpone certain mappings/optimizations since we
    // don't know?
    // So what coalescing should we postpone?  Can we do any?
    // So the only thing we can do is to expose clocks
    // Use infrastructure's clocks when you can
    //   WMemI, I/O, etc.
    // Share clocks when you can
    // Meet clock constraints expressed at the worker
    // Meet clock constraints expressed at external connections.
    ///////////////
    // So we need to consolidate clocks that have data connections?
    // Error check incompatible clocks


    if (isCompatible(p->clock, aw->clock))
      aClock = aw->clock;
    else {

    }

    // A port with a clock that is not mapped and doesn't use a wci clock
    switch (p->type) {
    case WCIPort:
      return "Internal error: WCI port doesn't use WCI clock";
    case WTIPort:
      //
    case WMemIPort:
    case WSIPort:
    case WMIPort:
    default:
      return "Internal error: port has no known type";
    }
  }
}
#endif
// This is a parsed for the assembly of what does into a single worker binary
 const char *
parseRccAssy(ezxml_t xml, const char *file, Worker *aw) {
  const char *err;
  Assembly *a = &aw->assembly;
  aw->model = RccModel;
  aw->modelString = "rcc";
  aw->isAssembly = true;
  if ((err = OE::checkAttrs(xml, "Name", (void*)0)))
    return err;
#if 0
  aw->implName = ezxml_cattr(xml, "Name");
  if (!aw->implName)
    aw->implName = "RccAssembly";
#endif
  for (ezxml_t x = ezxml_cchild(xml, "Worker"); x; x = ezxml_next(x)) {
    const char *wXmlName = ezxml_cattr(x, "File");
    if (!wXmlName)
      return "Missing \"File\" attribute is \"Worker\" element";
    Worker *w = new Worker;
    a->workers.push_back(w);
    if ((err = parseWorker(wXmlName, file, w)))
      return err;
  }
  return 0;
}

// This is a parsed for the assembly of what does into a single worker binary
 const char *
parseOclAssy(ezxml_t xml, const char *file, Worker *aw) {
  const char *err;
  Assembly *a = &aw->assembly;
  aw->model = OclModel;
  aw->modelString = "ocl";
  aw->isAssembly = true;
  if ((err = OE::checkAttrs(xml, "Name", (void*)0)))
    return err;
#if 0
  aw->implName = ezxml_cattr(xml, "Name");
  if (!aw->implName)
    aw->implName = "OclAssembly";
#endif
  for (ezxml_t x = ezxml_cchild(xml, "Worker"); x; x = ezxml_next(x)) {
    const char *wXmlName = ezxml_cattr(x, "File");
    if (!wXmlName)
      return "Missing \"File\" attribute is \"Worker\" element";
    Worker *w = new Worker;
    a->workers.push_back(w);
    if ((err = parseWorker(wXmlName, file, w)))
      return err;
  }
  return 0;
}
#if 0
static const char *
doInOut(const char *tok, Instance *i, bool isProducer) {
  Worker *w = i->worker;
  Port *p = w->ports;
  for (unsigned n = 0; n < w->ports.size(); n++, p++)
    if (!strcmp(tok, p->name))
      if (p->u.wdi.isBidirectional)
        i->ports[n].isProducer = isProducer;
      else
        return OU::esprintf("Port \"%s\" is neither WMI nor bidirectional", p->name);
  return OU::esprintf("Unknown port \"%s\" in \"inputs\" attribute of instance", tok);
}
static const char *
doInputs(const char *tok, void *arg) {
  return doInOut(tok, (Instance *)arg, false);
}
static const char *
doOutputs(const char *tok, void *arg) {
  return doInOut(tok, (Instance *)arg, true);
}
#endif

// The generic assembly parser
 static const char *
parseAssy(ezxml_t xml, Worker *aw,
          const char **topAttrs, const char **instAttrs, bool noWorkerOk) {
  const char *err;
  Assembly *a = &aw->assembly;
  aw->isAssembly = true;
  if ((err = OE::checkAttrsV(xml, topAttrs)))
    return err;
#if 0
  aw->implName = ezxml_cattr(xml, "Name");
  if (!aw->implName)
    if (defName)
      aw->implName = defName;
    else
      return OU::esprintf("Missing \"Name\" attribute for \"%s\"", xml->name);
#endif
  // Count instances and workers
  for (ezxml_t x = ezxml_cchild(xml, "Instance"); x; x = ezxml_next(x))
    a->nInstances++;
  Instance *i = a->instances = myCalloc(Instance, a->nInstances);
  for (ezxml_t x = ezxml_cchild(xml, "Instance"); x; x = ezxml_next(x), i++) {
    if ((err = OE::checkAttrsV(x, instAttrs)))
      return err;
    i->name = ezxml_cattr(x, "Name");   // Name attribute is in fact optional
    i->wName = ezxml_cattr(x, "Worker"); // Worker attribute is pathname
    if (!i->wName) {
      if (noWorkerOk) // caller's business to deal with this
        continue;
      return OU::esprintf("Missing \"Worker\" attribute for instance \"%s\"",
                      i->name ? i->name : "<no Name>");
    }
    // So we have an instance with a real live worker
    for (Instance *ii = a->instances; ii < i; ii++)
      if (ii->wName && !strcmp(i->wName, ii->wName))
        i->worker = ii->worker;
    if (!i->worker) {
      i->worker = new Worker;
      a->workers.push_back(i->worker);
      if ((err = parseWorker(i->wName, aw->file, i->worker)))
        return OU::esprintf("in file %s: %s", i->wName, err);
    }
    i->ports = myCalloc(InstancePort, i->worker->ports.size());
    for (unsigned n = 0; n < i->worker->ports.size(); n++) {
      i->ports[n].port = i->worker->ports[n];
      i->ports[n].instance = i;
    }
#if 0
    // Override bidirectional ports in the instance
    if ((err = parseList(ezxml_cattr(x, "inputs"), doInputs, i)) ||
        (err = parseList(ezxml_cattr(x, "outputs"), doOutputs, i)))
      return err;
#endif
    // Parse instance property values
    for (ezxml_t pv = ezxml_cchild(x, "PropertyValue"); pv; pv = ezxml_next(pv))
      i->nValues++;
    InstanceProperty *ipv = i->properties =
      myCalloc(InstanceProperty, i->nValues);
    for (ezxml_t pv = ezxml_cchild(x, "PropertyValue"); pv;
         pv = ezxml_next(pv), ipv++) {
      const char *name = ezxml_cattr(pv, "Name");
      if (!name)
        return "PropertyValue has no \"Name\" attribute";
      for (PropertiesIter pi = i->worker->ctl.properties.begin();
	   pi != i->worker->ctl.properties.end(); pi++)
        if ((*pi)->m_name == name) {
          ipv->property = (*pi);
          break;
        }
      if (!ipv->property)
        return OU::esprintf("Unknown property \"%s\" for worker \"%s\"", name,
                        i->worker->implName);
      const char *unparsed = ezxml_cattr(pv, "Value");
      if (!unparsed)
	return OU::esprintf("Missing \"value\" attribute for \"%s\" property value", name);

      if ((err = ipv->property->parseValue(unparsed, ipv->value)))
        return err;
    }
  }
  // Resolve instance names (i.e. generate those that are not specified)
  unsigned n = 0;
  for (i = a->instances; n < a->nInstances; n++, i++) {
    if (!i->name) {
      // compute ordinal in assembly for these instances
      unsigned nSame = 0, total = 1, nn = 0;
      for (Instance *ii = a->instances; nn < a->nInstances; ii++, nn++)
        if (n != nn && i->worker == ii->worker) {
          if (ii < i)
            nSame++;
          total++;
        }
#if 0
      const char *wName = strrchr(i->wName, '/');
      wName = wName ? wName+1 : i->wName;
      char *dot = strchr(wName, '.');
      *dot = 0;
#endif
      if (total == 1)
        i->name = i->worker->implName;
      else
        asprintf((char **)&i->name, "%s%d", i->worker->implName, nSame);
    }
    for (Instance *ii = a->instances; ii < i; ii++)
      if (!strcmp(ii->name, i->name))
        return OU::esprintf("Duplicate instance named \"%s\" in assembly", i->name);
  }
  for (ezxml_t x = ezxml_cchild(xml, "Connection"); x; x = ezxml_next(x)) {
    if ((err = OE::checkAttrs(x, "Name", "External", (void*)0)))
      return err;
    a->nConnections++;
  }
  Connection *c = a->connections = myCalloc(Connection, a->nConnections);
  for (ezxml_t x = ezxml_cchild(xml, "Connection"); x; x = ezxml_next(x), c++) {
    c->name = ezxml_cattr(x, "Name");
    for (ezxml_t at = ezxml_cchild(x, "Attach"); at; at = ezxml_next(at)) {
      const char *instName = ezxml_cattr(at, "Instance");
      if (!instName)
        return
          OU::esprintf("Missing \"Instance\" attribute in Attach subelement of "
                   "connection \"%s\"", c->name);
      n = 0;
      InstancePort *ip = 0; // kill warning
      Port *p = 0; // kill warning
      for (i = a->instances; n < a->nInstances; n++, i++)
        if (!strcmp(i->name, instName)) {
          const char *iName = ezxml_cattr(at, "Interface");
          if (!iName)
            return
              OU::esprintf("Missing \"Interface\" attribute in \"attach\" subelement of"
                       "connection \"%s\"", c->name);
          if (!i->worker)
            return OU::esprintf("Instance \"%s\" of connection \"%s\" has no worker",
                            i->name, c->name);
          unsigned nn = 0;
          for (nn = 0; nn < i->worker->ports.size(); nn++) {
            p = i->worker->ports[nn];
            if (!strcmp(p->name, iName)) {
              if (p->type != WSIPort && p->type != WMIPort)
                return "Connections for non-data ports not allowed";
              ip = &i->ports[nn];
              if (ip->connection)
                return
                  OU::esprintf("Interface \"%s\" of worker instance \"%s\" "
                           "attached to both connections \"%s\" and \"%s\"",
                           iName, instName, ip->connection->name, c->name);
              break;
            }
          }
          if (nn >= i->worker->ports.size())
            return OU::esprintf("Interface \"%s\" not found on instance \"%s\" in "
                            "connection  \"%s\"", iName, instName, c->name);
          break;
        }
      if (n >= a->nInstances)
        return OU::esprintf("Instance \"%s\" not found for connection  \"%s\"",
                        instName, c->name);
      attachPort(c, ip, p->name, NULL);
    } // all (local) attachments to the connection
    if (!c->name)
      asprintf((char **)&c->name, "conn%d", (int)(c - a->connections));
    for (Connection *cc = a->connections; cc < c; cc++)
      if (!strcmp(cc->name, c->name))
        return OU::esprintf("Duplicate connection named \"%s\" in assembly",
                        c->name);
    const char *ext = ezxml_cattr(x, "External");
    if (ext) {
      if (strcasecmp(ext, "producer") &&
          strcasecmp(ext, "consumer") &&
          strcasecmp(ext, "bidirectional"))
        return OU::esprintf("Value of \"External\" attribute of connection \"%s\" is not "
                        "\"consumer\" or \"producer\"", c->name);
      InstancePort *ip = myCalloc(InstancePort, 1);
      attachPort(c, ip, c->name, ext);
    }
  } // all connections
  // All parsing is done.
  // Now we fill in the top-level worker stuff.
  aw->specName = aw->implName;
  // Properties:  we only set the canonical hasDebugLogic property, which is a parameter.
  if ((err = OE::ezxml_children(xml, doImplProp, aw)))
    return err;
  // Create the external data ports on the assembly worker
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    if (c->nExternals) {
      Port *extPort = 0, *intPort = 0;
      const char *role = 0; // kill warning. if c->nExternals, it will be set
      for (InstancePort *ip = c->ports; ip; ip = ip->nextConn)
        if (ip->externalRole) {
          role = ip->externalRole;
          assert(!extPort); // for now only one
          ip->port = new Port();
          aw->ports.push_back(ip->port);
          extPort = ip->port;
        } else {
          assert(!intPort); // for now only one
          intPort = ip->port;  // remember the last one
        }
      // Start by copying everything.
      *extPort = *intPort;
      extPort->clock = 0;
      extPort->clockPort = 0;
      extPort->pattern = 0;
      extPort->name = c->name;
      extPort->isExternal = true;
      extPort->count = 1;
      extPort->worker = aw;
      // See how we expose this externally
      if (!strcasecmp(role, "producer")) {
        if (!intPort->u.wdi.isProducer && !intPort->u.wdi.isBidirectional)
          return OU::esprintf("Connection %s has external producer role incompatible "
                          "with port %s of worker %s",
                          c->name, intPort->name, intPort->worker->implName);
        extPort->u.wdi.isProducer = true;
        extPort->u.wdi.isBidirectional = false;
      } else if (!strcasecmp(role, "bidirectional")) {
        if (!intPort->u.wdi.isBidirectional)
          return OU::esprintf("Connection %s has external bidirectional role incompatible "
                          "with port %s of worker %s",
                          c->name, intPort->name, intPort->worker->implName);
      } else if (!strcasecmp(role, "consumer")) {
        if (intPort->u.wdi.isProducer)
          return OU::esprintf("Connection %s has external consumer role incompatible "
                          "with port %s of worker %s",
                          c->name, intPort->name, intPort->worker->implName);
        extPort->u.wdi.isBidirectional = false;
      }
    }
  // Check for unconnected non-optional data ports
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker) {
      InstancePort *ip = i->ports;
      for (unsigned nn = 0; nn < i->worker->ports.size(); nn++, ip++) {
        Port *pp = ip->port;
        if (pp->isData &&
            !pp->u.wdi.isOptional &&
            !ip->connection)
          return
            OU::esprintf("Port %s of instance %s of worker %s"
               " is not connected and not optional",
               pp->name, i->name, i->worker->implName);
      }
    }
  return 0;
}

const char *
parseHdlAssy(ezxml_t xml, Worker *aw) {
  const char *err;
  Assembly *a = &aw->assembly;
  a->isContainer = !strcasecmp(xml->name, "HdlContainer");
  static const char
    *topAttrs[] = {"Name", "Pattern", "Language", 0},
    *instAttrs[] = {"Worker", "Name", 0},
    *contInstAttrs[] = {"Worker", "Name", "Index", "Interconnect", "IO", "Adapter", "Configure", 0};
  // Do the generic assembly parsing, then to more specific to HDL
  if ((err = parseAssy(xml, aw, topAttrs,
                       a->isContainer ? contInstAttrs : instAttrs, true)))
      return err;
  // Do the OCP derivation for all workers
  for (WorkersIter wi = a->workers.begin(); wi != a->workers.end(); wi++)
    if ((err = deriveOCP(*wi)))
      return err;
  ezxml_t x = ezxml_cchild(xml, "Instance");
  Instance *i;
  // FIXME: ensure that container and application instance names don't collide
  for (i = a->instances; x; i++, x = ezxml_next(x)) {
    if (a->isContainer) {
      bool idxFound;
      // FIXME: perhaps an instance with no control?
      if ((err = OE::getNumber(x, "Index", &i->index, &idxFound, 0)))
        return err;
      if (!idxFound)
        return "Missing o\"Index\" attribute in instance in container assembly";
      const char
        *ic = ezxml_cattr(x, "Interconnect"),
	*ad = ezxml_cattr(x, "Adapter"),
        *io = ezxml_cattr(x, "IO");
      if (!i->wName) {
        // No worker means application instance in container
        if (!i->name)
          return "Missing \"Name\" attribute for application instance in container";
        if (io || ic || ad)
          return "Application workers in container can't be interconnects or io or adapter";
	i->iType = Instance::Application;
        continue; // for app instances we just capture index.
      } else {
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
        // we do allow for containers to have workers that are not io or interconnect
      }
    } // end of container processing
    // Now we are doing HDL processing per instance
    // Allocate the instance-clock-to-assembly-clock map
    i->clocks = myCalloc(Clock *, i->worker->nClocks);
  }
  // Rejuggle the ports because the generic parser only deals with data ports
  // Make the first one wci, then data, then others
  Port *wci = new Port();
  aw->ports.insert(aw->ports.begin(), wci);
#if 0
  unsigned morePorts = 0;
  // add time and memory services (even though they might be coalesced)
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker) {
      Port *p = i->worker->ports;
      for (unsigned nn = 0; nn < i->worker->ports.size(); nn++) {
        Port *p = &i->worker->ports[nn];
        if (p->type == WTIPort || p->type == WMemIPort)
          morePorts++;
      }
    }
  Port *dataPorts = aw->ports;
  unsigned nDataPorts = aw->ports.size();
  aw->ports.size() += 1 + morePorts; // +1 for wci
  aw->ports = new Port[aw->ports.size()];
  memcpy(aw->ports + 1, dataPorts, nDataPorts * sizeof(Port));
  // Ugliness because we have to change pointers into the ports
  Connection *c;
  InstancePort *ip;
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    for (ip = c->ports; ip; ip = ip->nextConn)
      if (ip->externalRole)
        ip->port = aw->ports + (ip->port - dataPorts + 1);
#endif
  // Clocks: coalesce all WCI clock and clocks with same reqts, into one wci, all for the assy
  aw->nClocks = 1; // first clock for all instance WCIs.
  Clock *clk = aw->clocks = myCalloc(Clock, aw->ports.size()); // overallocate
  clk->signal = clk->name = "wci_Clk";
  clk->port = wci;
  wci->name = "wci";
  wci->type = WCIPort;
  wci->myClock = true;
  wci->clock = clk++;
  wci->clock->port = wci;
  unsigned n;
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker && i->worker->ports[0]->type == WCIPort)
      i->ports->ordinal = wci->count++;
  // Map all the wci clocks to the assy's wci clock
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    // Map the instance's WCI clock to the assembly's WCI clock if it has a wci port
    if (i->worker && i->worker->ports[0]->type == WCIPort)
      i->clocks[i->worker->ports[0]->clock - i->worker->clocks] = wci->clock;
  // Assign the wci clock to connections where we can
  Connection *c;
  InstancePort *ip;
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++) {
    for (ip = c->ports; ip; ip = ip->nextConn)
      if (!ip->externalRole &&
          // If the worker of this connected port has a WCI port
          ip->instance->worker->ports[0]->type == WCIPort &&
          // If this port on the worker uses the worker's wci clock
          ip->port->clock == ip->instance->worker->ports[0]->clock)
        break;
    if (ip) {
      // So this case is when some port on the connection uses the wci clock
      // So we assign the wci clock as the clock for this whole connection
      c->clock = wci->clock;
      if (c->external)
        c->external->port->clock = c->clock;
      // And force the clock for each connected port to BE the wci clock
      // This will potentialy connect an independent clock to the wci clock
      for (ip = c->ports; ip; ip = ip->nextConn)
        if (!ip->externalRole)
          // FIXME: check for compatible clock constraints? insert synchronizer?
          ip->instance->clocks[ip->port->clock - ip->instance->worker->clocks] =
            wci->clock;
    }
  }
  // Deal with all the internal connection clocks that are not WCI clocks
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    for (ip = c->ports; ip; ip = ip->nextConn)
      if (!ip->externalRole) {
        unsigned nc = ip->port->clock - ip->instance->worker->clocks;
        if (!c->clock) {
          // This connection doesn't have a clock yet,
          // so its not using the WCI clock either
          if (ip->instance->clocks[nc])
            // The clock of the port is already mapped, so we just use it.
            c->clock = ip->instance->clocks[nc];
          else {
            // The connection has no clock, and the port's clock is not mapped.
            // We need a new top level clock.
            if (ip->port->clock->port) {
              // This clock is owned by a port, so it is a "port clock". So name it
              // after the connection (and external port).
              asprintf((char **)&clk->name, "%s_Clk", c->name);
              clk->signal = clk->name;
              clk->port = c->external->port;
              c->external->port->myClock = true;
            } else {
              // This port's clock is a separately defined clock
              // We might as well keep the name since we have none better
              clk->name = strdup(ip->port->clock->name);
              clk->signal = strdup(ip->port->clock->signal);
            }
            clk->assembly = true;
            aw->nClocks++;
            c->clock = clk++;
            ip->instance->clocks[nc] = c->clock;
            // FIXME inherit ip->port->clock constraints
          }
        } else if (ip->instance->clocks[nc]) {
          // This port already has a mapped clock
          if (ip->instance->clocks[nc] != c->clock)
            return OU::esprintf("Connection %s at interface %s of instance %s has clock conflict",
                            c->name, ip->port->name, ip->instance->name);
        } else {
          // FIXME CHECK COMPATIBILITY OF c->clock with ip->port->clock
          ip->instance->clocks[nc] = c->clock;
        }
      }
  bool cantDataResetWhileSuspended = false;
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker) {
      unsigned nn;
      for (nn = 0, ip = i->ports; nn < i->worker->ports.size(); nn++, ip++) {
        unsigned nc = ip->port->clock - ip->instance->worker->clocks;
        if (!i->clocks[nc]) {
          if (ip->port->type == WSIPort || ip->port->type == WMIPort)
            return OU::esprintf("Unconnected data interface %s of instance %s has its own clock",
                            ip->port->name, i->name);
          i->clocks[nc] = clk;
          asprintf((char **)&clk->name, "%s_%s", i->name, ip->port->clock->name);
          if (ip->port->clock->signal)
            asprintf((char **)&clk->signal, "%s_%s", i->name,
                     ip->port->clock->signal);
          clk->assembly = true;
          aw->nClocks++;
          c->clock = clk++;
          i->clocks[nc] = c->clock;
        }
      }
    }
  // Now all clocks are done.  We process the non-data external ports.
  // Now all data ports that are connected have mapped clocks and
  // all ports with WCI clocks are connected.  All that's left is
  // WCI: WTI, WMemI
  unsigned nWti = 0, nWmemi = 0;
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker) {
      unsigned nn;
      Worker *iw = i->worker;
      for (nn = 0, ip = i->ports; nn < iw->ports.size(); nn++, ip++) {
        Port *pp = ip->port;
        switch (pp->type) {
        case WCIPort:
          // Make assembly WCI the union of all inside, with a replication count
          // We make it easier for CTOP, hoping that wires dissolve appropriately
          if (iw->ctl.sizeOfConfigSpace > aw->ctl.sizeOfConfigSpace)
            aw->ctl.sizeOfConfigSpace = iw->ctl.sizeOfConfigSpace;
          if (iw->ctl.writableConfigProperties)
            aw->ctl.writableConfigProperties = true;
          if (iw->ctl.readableConfigProperties)
            aw->ctl.readableConfigProperties = true;
#if 0
	  // Until we have container automation, we force the assembly level
	  // WCIs to have byte enables.  FIXME
          if (iw->ctl.sub32BitConfigProperties)
#endif
            aw->ctl.sub32BitConfigProperties = true;
          aw->ctl.controlOps |= iw->ctl.controlOps; // needed?  useful?
          // Reset while suspended: This is really only interesting if all
          // external data ports are only connected to ports of workers were this
          // is true.  And the use-case is just that you can reset the
          // infrastructure while maintaining worker state.  BUT resetting the
          // CP could clearly reset anything anyway, so this is only relevant to
          // just reset the dataplane infrastructure.
          if (!pp->u.wci.resetWhileSuspended)
            cantDataResetWhileSuspended = true;
          ip->externalPort = wci;
          break;
        case WTIPort:
          // We don't share ports since the whole point of WTi is to get
          // intra-chip accuracy via replication of the time clients.
          // We could have an option to use wires instead to make things smaller
          // and less accurate...
          {
            Port *wti = new Port();
            aw->ports.push_back(wti);
            *wti = *pp;
            asprintf((char **)&wti->name, "wti%u", nWti++);
            ip->externalPort = wti;
            wti->clock = i->clocks[pp->clock - i->worker->clocks];
          }
          break;
        case WMemIPort:
          {
            Port *wmemi = new Port();
            aw->ports.push_back(wmemi);
            *wmemi = *pp;
            asprintf((char **)&wmemi->name, "wmemi%u", nWmemi++);
            ip->externalPort = wmemi;
            wmemi->clock = i->clocks[pp->clock - i->worker->clocks];
          }
          break;
        case WSIPort:
        case WMIPort:
          break;
        default:
          return "Bad port type";
        }
      }
    }
  if (!cantDataResetWhileSuspended)
    wci->u.wci.resetWhileSuspended = true;
  // Process the external data ports on the assembly worker
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    if (c->nExternals)
      for (ip = c->ports; ip; ip = ip->nextConn)
        if (ip->externalRole) {
          Port *p = ip->port;
          p->clock = c->clock;
          if (p->clock->port && p->clock->port != p)
            p->clockPort = p->clock->port;
          if (p->type == WSIPort)
            p->u.wsi.regRequest = false;
        }
  return 0;
}

// This is an HDL file, and perhaps an assembly
const char *
parseHdl(ezxml_t xml, const char *file, Worker *w) {
   const char *err;
  if (strcmp(w->implName, w->fileName))
    return OU::esprintf("File name (%s) and implementation name in XML (%s) don't match",
		    w->fileName, w->implName);
  if ((err = OE::checkAttrs(xml, "Name", "Pattern", "Language", (void*)0)))
    return err;
  const char *lang = ezxml_cattr(xml, "Language");
  if (!lang)
    return "Missing Language attribute for ComponentImplementation element";
  if (!strcasecmp(lang, "Verilog"))
    w->language = Verilog;
  else if (!strcasecmp(lang, "VHDL"))
    w->language = VHDL;
  else
    return "Language attribute not \"Verilog\" or \"VHDL\" in ComponentImplementation";
  w->pattern = ezxml_cattr(xml, "Pattern");
  if (!w->pattern)
    w->pattern = "%s_";
  // Here is where there is a difference between a implementation and as assembly
  if (!strcasecmp(xml->name, "HdlImplementation")) {
    if ((err = parseHdlImpl(xml, file, w)))
      return OU::esprintf("in %s for %s: %s", xml->name, w->implName, err);
  } else if (!strcasecmp(xml->name, "HdlAssembly")) {
    if ((err = parseHdlAssy(xml, w)))
      return OU::esprintf("in %s for %s: %s", xml->name, w->implName, err);
  } else
    return "file contains neither an HdlImplementation nor an HdlAssembly";
  // Whether a worker or an assembly, we derive the external OCP signals, etc.
  if ((err = deriveOCP(w)))
    return OU::esprintf("in %s for %s: %s", xml->name, w->implName, err);
  unsigned wipN[NWIPTypes][2] = {{0}};
  for (unsigned i = 0; i < w->ports.size(); i++) {
    Port *p = w->ports[i];
    // Derive full names
    bool mIn = masterIn(p);
    // ordinal == -1 means insert "%d" into the name for using latter
    if ((err = pattern(w, p, -1, wipN[p->type][mIn], true, !mIn, (char **)&p->fullNameIn)) ||
        (err = pattern(w, p, -1, wipN[p->type][mIn], false, !mIn, (char **)&p->fullNameOut)))
      return err;
    if (p->clock->port == p) {
      char *sin;
      // ordinal == -2 means suppress ordinal
      if ((err = pattern(w, p, -2, wipN[p->type][mIn], true, !mIn, &sin)))
        return err;
      asprintf((char **)&p->ocp.Clk.signal, "%s%s", sin, "Clk");
      p->clock->signal = p->ocp.Clk.signal;
    }
    OcpSignalDesc *osd = ocpSignals;
    for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
      if (osd->master == mIn && strcasecmp(osd->name, "Clk") && os->value)
        asprintf((char **)&os->signal, "%s%s", p->fullNameIn, osd->name);
    osd = ocpSignals;
    for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
      if (osd->master != mIn && strcasecmp(osd->name, "Clk") && os->value)
        asprintf((char **)&os->signal, "%s%s", p->fullNameOut, osd->name);
    wipN[p->type][mIn]++;
  }
  if (w->ports.size() > 32)
    return "worker has more than 32 ports";
  w->model = HdlModel;
  w->modelString = "hdl";
  return 0;
}

/*
 * What implementation-specific attributes does an RCC worker have?
 * And which are not available at runtime?
 * And if they are indeed available at runtime, do we really retreive them from the
 * container or just let the container use what it knows?
 */
const char *
parseRcc(ezxml_t xml, const char *file, Worker *w) {
  const char *err;
  if ((err = OE::checkAttrs(xml, "Name", "ExternMethods", "StaticMethods", "Threaded",
			    "ControlOperations", (void*)0)))
    return err;
  // We use the pattern value as the method naming for RCC
  // and its presence indicates "extern" methods.
  w->pattern = ezxml_cattr(xml, "ExternMethods");
  w->staticPattern = ezxml_cattr(xml, "StaticMethods");
  ezxml_t xctl;
  if ((err = parseSpec(xml, file, w)) ||
      (err = parseImplControl(xml, file, w, xctl)) ||
      (xctl && (err = OE::checkAttrs(xctl, GENERIC_IMPL_CONTROL_ATTRS, "Threaded", (void *)0))) ||
      (err = OE::getBoolean(xml, "Threaded", &w->isThreaded)))
    return err;
  if ((err = parseList(ezxml_cattr(xml, "ControlOperations"), parseControlOp, w)))
    return err;
  // Parse data port implementation metadata: maxlength, minbuffers.
  for (ezxml_t x = ezxml_cchild(xml, "Port"); x; x = ezxml_next(x)) {
    if ((err = OE::checkAttrs(x, "Name", "MinBuffers", "MinBufferCount", "BufferSize", (void*)0)))
      return err;
    const char *name = ezxml_cattr(x, "Name");
    if (!name)
      return "Missing \"Name\" attribute on Port element if RccImplementation";
    Port *p = 0; // kill warning
    unsigned n;
    for (n = 0; n < w->ports.size(); n++) {
      p = w->ports[n];
      if (!strcasecmp(p->name, name))
        break;
    }
    if (n >= w->ports.size())
      return OU::esprintf("No DataInterface named \"%s\" from Port element", name);
    if ((err = OE::getNumber(x, "MinBuffers", &p->u.wdi.minBufferCount, 0, 0)) || // backward compat
        (err = OE::getNumber(x, "MinBufferCount", &p->u.wdi.minBufferCount, 0, p->u.wdi.minBufferCount)) ||
        (err = OE::getNumber(x, "Buffersize", &p->u.wdi.bufferSize, 0,
			     p->protocol ? p->protocol->m_defaultBufferSize : 0)))
      return err;
  }
  w->model = RccModel;
  w->modelString = "rcc";
  return 0;
}
/*
 * What implementation-specific attributes does an OCL worker have?
 * And which are not available at runtime?
 * And if they are indeed available at runtime, do we really retreive them from the
 * container or just let the container use what it knows?
 */
const char *
parseOcl(ezxml_t xml, const char *file, Worker *w) {
  const char *err;
  if ((err = OE::checkAttrs(xml, "Name", "ExternMethods", "StaticMethods", (void*)0)))
    return err;
  // We use the pattern value as the method naming for OCL
  // and its presence indicates "extern" methods.
  w->pattern = ezxml_cattr(xml, "ExternMethods");
  w->staticPattern = ezxml_cattr(xml, "StaticMethods");
  ezxml_t xctl;
  if ((err = parseSpec(xml, file, w)) ||
      (err = parseImplControl(xml, file, w, xctl)) ||
      (err = parseImplLocalMemory(xml, w )))
    return err;
  // Parse data port implementation metadata: maxlength, minbuffers.
  for (ezxml_t x = ezxml_cchild(xml, "Port"); x; x = ezxml_next(x)) {
    if ((err = OE::checkAttrs(x, "Name", "MinBuffers", "MinBufferCount", (void*)0)))
      return err;
    const char *name = ezxml_cattr(x, "Name");
    if (!name)
      return "Missing \"Name\" attribute on Port element if OclImplementation";
    Port *p = 0; // kill warning
    unsigned n;
    for (n = 0; n < w->ports.size(); n++) {
      p = w->ports[n];
      if (!strcasecmp(p->name, name))
        break;
    }
    if (n >= w->ports.size())
      return OU::esprintf("No DataInterface named \"%s\" from Port element", name);
    if ((err = OE::getNumber(x, "MinBuffers", &p->u.wdi.minBufferCount, 0, 0)) || // backward compat
        (err = OE::getNumber(x, "MinBufferCount", &p->u.wdi.minBufferCount, 0, p->u.wdi.minBufferCount)))
      return err;
  }
  w->model = OclModel;
  w->modelString = "ocl";
  return 0;
}
// The most general case.  Could be any worker, or any assembly.
const char *
parseWorker(const char *file, const char *parent, Worker *w) {
  const char *err;
  ezxml_t xml;
  if ((err = parseFile(file, parent, NULL, &xml, &w->file)))
    return err;
  const char *cp = strrchr(file, '/');
  char *fp = strdup(cp ? cp + 1 : file);
  char *lp = strrchr(fp, '.');
  if (lp)
    *lp = 0;
  w->fileName = fp;
  w->implName = ezxml_cattr(xml, "Name");
  if (!w->implName)
    w->implName = w->fileName;
  const char *name = ezxml_name(xml);
  if (name) {
    if (!strcasecmp("RccImplementation", name))
      return parseRcc(xml, file, w);
    if (!strcasecmp("OclImplementation", name))
      return parseOcl(xml, file, w);
    if ((!strcasecmp("HdlImplementation", name) ||
         !strcasecmp("HdlAssembly", name)))
      return parseHdl(xml, file, w);
    if (!strcasecmp("RccAssembly", name))
      return parseRccAssy(xml, file, w);
    if (!strcasecmp("OclAssembly", name))
      return parseOclAssy(xml, file, w);
  }
#if 0
  if (name && !strcasecmp(xml->name, "ComponentAssembly"))
    return parseAssy(xml, w->file, w);
#endif
  return OU::esprintf("\"%s\" is not a valid implemention type (RccImplementation, HdlImplementation, OclImplementation, HdlAssembly, OclAssembly, ComponentAssembly)", xml->name);
}

void cleanWIP(Worker *w){ (void)w;}

Assembly::Assembly()
  : isContainer(false), outside(NULL), nInstances(0), instances(NULL), nConnections(0),
    connections(NULL)
{
}
Control::Control()
  : sizeOfConfigSpace(0), writableConfigProperties(false), readableConfigProperties(false),
    sub32BitConfigProperties(false), controlOps(0), offset(0), ordinal(0)
{
}
Worker::Worker()
  : model(NoModel), modelString(NULL), isDevice(false), noControl(false), file(0), specFile(0),
    implName(0), specName(0), fileName(0), isThreaded(false), nClocks(0),
    clocks(0), endian(NoEndian), pattern(0), staticPattern(0), isAssembly(false),
    nInstances(0), language(NoLanguage), nSignals(0), signals(0)
{
}

Port::Port()
  : name(0), fullNameIn(0), fullNameOut(0), worker(0), count(0),
    isExternal(false), isData(false), pattern(0), type(NoPort),
    dataWidth(0), byteWidth(0), impreciseBurst(false), preciseBurst(false),
    clock(0), clockPort(0), myClock(false),values(0), master(false), protocol(0)
{
  memset(&ocp, 0, sizeof(ocp));
  memset(&u, 0, sizeof(u));
  u.wdi.nOpcodes = 1;
}
