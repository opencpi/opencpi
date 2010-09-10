#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#define __STDC_LIMIT_MACROS // wierd standards goof up
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "wip.h"

/*
 * Todo:
 *  property values in assembly instances?
 */
const char **includes;
unsigned nIncludes;
void
addInclude(const char *inc) {
  includes = (const char **)realloc(includes, (nIncludes + 2) * sizeof(char *));
  includes[nIncludes++] = inc;
  includes[nIncludes] = 0;
}


const char *propertyTypes[] = {
  "none", // for CPI_none
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #pretty,
CPI_PROPERTY_DATA_TYPES
0};
#undef CPI_DATA_TYPE

const char *controlOperations[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3)  #x,
CPI_CONTROL_OPS
#undef CONTROL_OP
0};

const char **deps;
bool *depChild;
unsigned nDeps;
const char *depFile = 0;
void
addDep(const char *dep, bool child) {
  for (unsigned n = 0; n < nDeps; n++)
    if (!strcmp(dep, deps[n])) {
      if (child)
	depChild[n] = child;
      return;
    }
  deps = (const char **)realloc(deps, (nDeps + 2) * sizeof(char *));
  depChild = (bool *)realloc(depChild, (nDeps + 2) * sizeof(bool));
  depChild[nDeps] = child;
  deps[nDeps++] = dep;
  depChild[nDeps] = 0;
  deps[nDeps] = 0;
}

const char *
dumpDeps(const char *top) {
  FILE *out = fopen(depFile, "w");
  if (out == NULL)
    return esprintf("Cannot open dependency file \"%s\" for writing", top);
  fprintf(out, "%s:", top);
  for (unsigned n = 0; n < nDeps; n++)
    fprintf(out, " %s", deps[n]);
  fprintf(out, "\n");
  for (unsigned n; n < nDeps; n++)
    if (depChild[n])
      fprintf(out, "\n%s:\n", deps[n]);
  return 0;
}

 const char *
parseFile(const char *file, const char *parent, const char *element, ezxml_t *xp,
	  const char **xfile, bool optional) {
  const char *cp = parent ? strrchr(parent, '/') : 0;
  if (file[0] != '/' && cp)
    asprintf((char**)&cp, "%.*s%s", (int)(cp - parent + 1), parent, file);
  else
    cp = file;
  int fd = open(cp, O_RDONLY);
  if (fd < 0) {
    // file was not where parent file was, and not local.
    // Try the incude paths
    if (file[0] != '/' && includes) {
      for (const char **ap = includes; *ap; ap++) {
	if (!(*ap)[0] || !strcmp(*ap, "."))
	  cp = file;
	else
	  asprintf((char **)&cp, "%s/%s", *ap, file);
	if ((fd = open(cp, O_RDONLY)) >= 0)
	  break;
      }
    }
    if (fd < 0)
      return esprintf("File \"%s\" could not be opened for reading/parsing", cp);
  }
  if (xfile)
    *xfile = cp;
  ezxml_t x = ezxml_parse_fd(fd);
  if (!x || !x->name)
    return esprintf("File \"%s\" (when looking for \"%s\") could not be parsed as XML (", cp, file);
  if (element && strcmp(x->name, element)) {
    if (optional)
      *xp = 0;
    else
      return esprintf("File \"%s\" does not contain a %s element", cp, element);
  } else
    *xp = x;
  addDep(cp, parent != 0);
  return 0;
}

// MyClock boolean simply says whether the clock is "homed" and "named" here.
// The clock attribute says that the clock is defined elsewhere
// The "elsewhere" is either a port that has its own clock or a global definition.
static const char *
checkClock(Worker *w, ezxml_t impl, Port *p) {
  const char *err;
  const char *clock = 0;
  if (impl) {
    clock = ezxml_cattr(impl, "Clock");
    if ((err = CE::getBoolean(impl, "MyClock", &p->myClock)))
      return err;
  }
  if (!clock) {
    if (p->myClock || p->type == WCIPort) {
      // If port has its own clock, or is a WCI, establish a clock named and homed here
      p->myClock = true;
      p->clock = &w->clocks[w->nClocks++];
      asprintf((char **)&p->clock->name, "%s_Clk", p->name); // fixme
      p->clock->port = p;
    } else if (w->ports->type == WCIPort)
      // If no clock, and we have a WCI then assume the WCI's clock.
      p->clock = w->ports->clock;
    else
      // If no clock, and no wci port, we're hosed.
      return "Interface has no clock declared, and there is no control interface";
  } else {
    // Port refers to another clock by name
    Port *op = w->ports;
    for (unsigned i = 0; i < w->nPorts; i++, op++)
      if (p != op && !strcmp(clock, op->name)) {
	if (p->myClock)
	  // Can't refer to another port and also own the clock
	  return esprintf("Clock for interface \"%s\" refers to interface \"%s\","
			  " and also has MyClock=true?",
			  p->name, clock);
	p->clockPort = op;
	return 0;
      }
    // We are not referring to another port.  It muts be a defined clock
    Clock *c = w->clocks;
    for (unsigned i = 0; i < w->nClocks; i++, c++)
      if (!strcmp(clock, c->name)) {
	p->clock = c;
	if (p->myClock)
	  if (c->port)
	    return esprintf("Clock for interface \"%s\", \"%s\" is already owned by interface \"%s\"",
			    p->name, clock, c->port->name);
	  else
	    c->port = p;
	return 0;
      }
    return esprintf("Clock attribute of \"%s\" matches no interface or clock", p->name);
  }
  return 0;
}

// Check for implementation attributes common to data interfaces
static const char *
checkDataPort(Worker *w, ezxml_t impl, Port **dpp) {
  const char *err;
  const char *name = ezxml_cattr(impl, "Name");
  if (!name)
    return esprintf("Missing \"Name\" attribute of %s element", impl->name);
  Port *dp = w->ports;
  unsigned i;
  for (i = 0; i < w->nPorts && dp->name; i++, dp++)
    if (!strcmp(dp->name, name))
      break;
  if (i >= w->nPorts || !dp->isData)
    return
      esprintf("Name attribute of Stream/MessageInterface \"%s\" "
	       "does not match a DataInterfaceSpec", name);
  if ((err = checkClock(w, impl, dp)) ||
      (err = CE::getNumber(impl, "DataWidth", &dp->dataWidth, 0, dp->wdi.dataValueWidth)) ||
      (err = CE::getBoolean(impl, "Continuous", &dp->wdi.continuous)) ||
      (err = CE::getBoolean(impl, "ImpreciseBurst", &dp->impreciseBurst)) ||
      (err = CE::getBoolean(impl, "PreciseBurst", &dp->preciseBurst)))
    return err;
  if (dp->dataWidth % dp->wdi.dataValueWidth)
    return "DataWidth not a multiple of DataValueWidth";
#if 0
  if (dp->impreciseBurst && dp->preciseBurst)
    return "Both ImpreciseBurst and PreciseBurst cannot be specified for WSI or WMI";
#endif
  dp->pattern = ezxml_cattr(impl, "Pattern");
  *dpp = dp;
  return 0;
}


 static const char *
ezxml_children(ezxml_t xml, const char* (*func)(ezxml_t child, void *arg), void *arg) {
   const char *err;
  for (xml = xml ? xml->child : NULL; xml; xml = xml->ordered)
    if ((err = (*func)(xml, arg)))
      return err;
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
    if ((err = CE::checkAttrs(x, "href", (void*)0)))
      return err;
    const char *ifile = ezxml_cattr(x, "href");
    if (!ifile)
      return esprintf("xi:include missing an href attribute in file \"%s\"", parent);
    ezxml_t i = 0;
    if ((err = parseFile(ifile, parent, element, &i, &ifile, optional)))
      return err;
    *parsed = i;
    *child = ifile;
  } else
    *parsed = 0;
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
  if ((*parsed = ezxml_child(top, element))) {
    if (childFile)
      *childFile = parent;
    return 0;
  }
  return
    optional ? 0 :
    esprintf("Neither %s nor xi:include found under %s in file \"%s\"",
	     element, top->name, parent);
}

// Yes, using STL would be saner..
static const char *
addProperty(Worker *w, ezxml_t prop, bool includeImpl)
{
  const char *err;
  w->ctl.properties = myCrealloc(Property, w->ctl.properties, w->ctl.nProperties, 1);
  w->ctl.prop = w->ctl.properties + w->ctl.nProperties;
  w->ctl.nProperties++;
  Property *p = w->ctl.prop++;
  if ((err = p->parse(prop, w->ctl.offset, w->ctl.readableConfigProperties,
		      w->ctl.writableConfigProperties, w->ctl.sub32BitConfigProperties,
		      includeImpl)))
    return err;
  return 0;
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
  Property *p = w->ctl.properties;
  bool found = false;
  for (unsigned n = 0; n < w->ctl.nProperties; n++, p++)
    if (!strcmp(p->name, name)) {
      found = true;
      break;
    }
  if (found) {
    if (!isSpec)
      return esprintf("Implementation property named \"%s\" conflict with spec property",
		      name);
    if (p->members->hasDefault && ezxml_cattr(prop, "Default"))
      return esprintf("Implementation property named \"%s\" cannot override previous default value", name);
    if ((err = p->parseImpl(prop)))
      return err;
  } else {
    if (isSpec)
      return esprintf("Specification property named \"%s\" not found in spec", name);
    // All the spec attributes plus the impl attributes
    if ((err = addProperty(w, prop, true)))
      return err;
  }
  return 0;
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
    err = ezxml_children(iprop, doSpecProp, arg);
    w->file = ofile;
    return err;
  }
  const char *name = ezxml_name(prop);
  if (!name || strcasecmp(name, "Property"))
    return "Element under Properties is neither Property or xi:include";
  // Now actually process a property element
  if ((err = CE::checkAttrs(prop, "Name", "Type", "Readable", "Writable", "IsTest",
			"StringLength", "SequenceLength", "ArrayLength", "Default",
			NULL)))
    return err;
  if ((err = addProperty(w, prop, false)))
    return err;
  return 0;
}

// Parse the generic implementation control aspects (for rcc and hdl and other)
#define GENERIC_IMPL_CONTROL_ATTRS \
  "SizeOfConfigSpace", "ControlOperations", "Sub32BitConfigProperties"
const char *
parseImplControl(ezxml_t impl, const char *file, Worker *w, ezxml_t *xctlp) {
  // Now we do the rest of the control interface
  ezxml_t xctl = ezxml_child(impl, "ControlInterface");
  const char *err;
  if (xctl) {
    unsigned sizeOfConfigSpace;
    bool haveSize;
    if (w->noControl)
      return "Worker has a ControlInterface element, but also has NoControl=true";
    // Allow overriding sizeof config space
    if ((err = CE::getNumber(xctl, "SizeOfConfigSpace", &sizeOfConfigSpace, &haveSize, 0)))
      return err;
    if (haveSize) {
      if (sizeOfConfigSpace < w->ctl.sizeOfConfigSpace)
	return "SizeOfConfigSpace attribute of ControlInterface smaller than properties";
      w->ctl.sizeOfConfigSpace = sizeOfConfigSpace;
    }
    // Allow overriding byte enables
    bool sub32;
    if ((err = CE::getBoolean(xctl, "Sub32BitConfigProperties", &sub32)))
      return err;
    if (sub32)
      w->ctl.sub32BitConfigProperties = true;
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
    ezxml_t props;
    if ((err = tryChildInclude(xctl, file, "Properties", &props, NULL, true)))
      return err;
    // Properties might be in a "properties" element, maybe via xi:include
    if (props) {
      if ((err = ezxml_children(props, doImplProp, w)))
	return err;
    } else
      // Properties might also be directly under ControlInterface for simplicity
      if ((err = ezxml_children(xctl, doImplProp, w)))
	return err;
  }
  if (xctlp)
    *xctlp = xctl;
  // parseing the impl control interface means we have visited all the properties,
  // both spec and impl, so now we know the whole confid space.
  w->ctl.sizeOfConfigSpace = w->ctl.offset;
  return 0;
}

// Parse the control information about the component spec
const char *
parseSpecControl(Worker *w, ezxml_t ps, ezxml_t props) {
  const char *err;
  if (ps) {
    if ((err = CE::checkAttrs(ps, "SizeOfConfigSpace", "WritableConfigProperties",
			  "ReadableConfigProperties", "Sub32BitConfigProperties",
			  "Count", (void*)0)) ||
	(err = CE::getNumber(ps, "SizeOfConfigSpace", &w->ctl.sizeOfConfigSpace, 0, 0)) ||
	(err = CE::getBoolean(ps, "WritableConfigProperties", &w->ctl.writableConfigProperties)) ||
	(err = CE::getBoolean(ps, "ReadableConfigProperties", &w->ctl.readableConfigProperties)) ||
	(err = CE::getBoolean(ps, "Sub32BitConfigProperties", &w->ctl.sub32BitConfigProperties)))
      return err;
  } else if (props) {
    // No property summary, must have something else.
    if ((err = ezxml_children(props, doSpecProp, w)))
      return err;
  }
  return 0;
}
static const char *
doOperation(ezxml_t op, void *arg) {
  const char *err, *ifile;
  Port *p = (Port *)arg;
  ezxml_t iprot = 0;
  if ((err = tryInclude(op, p->worker->file, "Protocol", &iprot, &ifile)))
    return err;
  // If it is an "include", basically recurse
  if (iprot) {
    const char *ofile = p->worker->file;
    p->worker->file = ifile;
    err = ezxml_children(iprot, doOperation, arg);
    p->worker->file = ofile;
    return err;
  }
  const char *name = ezxml_name(op);
  if (!name || strcasecmp(name, "Operation"))
    return "Element under Protocol is neither Operation, Protocol or or xi:include";
  // Now actually process a property element
  if ((err = CE::checkAttrs(op, "Name", "Twoway", (void*)0)))
    return err;
  // If this is NULL we're just counting properties.
  if (!p->wdi.operations) {
    p->wdi.nOperations++;
    return 0;
  }
  Operation *o = p->wdi.op++;
  o->name = ezxml_cattr(op, "Name");
  if (!p->name)
    return "Missing \"Name\" attribute for operation";
  if ((err = CE::getBoolean(op, "TwoWay", &o->isTwoWay)))
    return err;
  for (ezxml_t m = ezxml_child(op, "Argument"); m ; m = ezxml_next(m))
    o->nArgs++;
  // No support for twoway yet, so no exceptions, return values, inout, out
  if (o->nArgs) {
    o->args = myCalloc(Simple, o->nArgs);
    Simple *mem = o->args;
    unsigned myOffset = 0, maxAlign = 1;
    bool sub32 = false;
    for (ezxml_t m = ezxml_child(op, "Argument"); m ; m = ezxml_next(m), mem++) {
      if ((err = CE::checkAttrs(m, "Name", "Type", "StringLength",
			    "ArrayLength", "SequenceLength", (void*)0)) ||
	  (err = mem->parse(m, maxAlign, myOffset, sub32)))
	return err;
      if (p->wdi.dataValueWidth &&
	  mem->bits != p->wdi.dataValueWidth)
	p->wdi.diverseDataSizes = true;
      if (!p->wdi.dataValueWidth ||
	  mem->bits < p->wdi.dataValueWidth)
	p->wdi.dataValueWidth = mem->bits;
    }
    if (p->wdi.maxMessageValues &&
	p->wdi.maxMessageValues != myOffset)
      p->wdi.variableMessageLength = true;
    if (myOffset > p->wdi.maxMessageValues)
      p->wdi.maxMessageValues = myOffset; // still in bytes until later
  } else
    p->wdi.zeroLengthMessages = true;
  return 0;
}
static const char *
parseProtocol(Port *p, ezxml_t prot) {
  const char *err;
  // First time we call this it will just be for counting.
  if ((err = ezxml_children(prot, doOperation, p)))
    return err;
  p->wdi.op = p->wdi.operations = myCalloc(Operation, p->wdi.nOperations);
  // Now we call a second time t make them.
  if ((err = ezxml_children(prot, doOperation, p)))
    return err;
  // Convert max size from bytes back to values
  if (p->wdi.dataValueWidth) {
    unsigned bytes = (p->wdi.dataValueWidth + CHAR_BIT - 1) / CHAR_BIT;
    p->wdi.maxMessageValues += bytes - 1;
    p->wdi.maxMessageValues /= bytes;
    p->wdi.dataValueGranularity = 1;  // FIXME - compute this for real
  }
  p->wdi.numberOfOpcodes = p->wdi.nOperations;
  return 0;
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
  if ((err = CE::checkAttrs(spec, "Name", "NoControl", (void*)0)) ||
      (err = CE::getBoolean(spec, "NoControl", &w->noControl)))
    return err;
  // Parse control port info
  ezxml_t ps, props;
  if ((err = tryChildInclude(spec, file, "PropertySummary", &ps, NULL, true)))
    return err;
  if (ps) {
    if (ezxml_child(spec, "Properties") || ezxml_child(spec, "Property"))
      return "cannot have both PropertySummary and Properties";
    props = 0;
  } else if ((err = tryChildInclude(spec, file, "Properties", &props, NULL, true)))
    return err;
  if (w->noControl) {
    if (ps || props)
      return "NoControl specified, PropertySummary or Properties cannot be specified";
  } else if ((err = parseSpecControl(w, ps, props)))
    return err;
  // Now parse the data aspects, allocating (data) ports.
  for (ezxml_t x = ezxml_child(spec, "DataInterfaceSpec"); x; x = ezxml_next(x))
    w->nPorts++;
  // Allocate all the data ports
  Port *p = w->ports = myCalloc(Port, w->nPorts);
  for (ezxml_t x = ezxml_child(spec, "DataInterfaceSpec"); x; x = ezxml_next(x), p++) {
    if ((err = CE::checkAttrs(x, "Name", "Producer", "Count", "Optional", (void*)0)) ||
	(err = CE::getBoolean(x, "Producer", &p->wdi.isProducer)) ||
	(err = CE::getBoolean(x, "Optional", &p->wdi.isOptional)))
      return err;
    p->worker = w;
    p->isData = true;
    p->type = WDIPort;
    if (!(p->name = ezxml_cattr(x, "Name")))
      return "Missing \"Name\" attribute in DataInterfaceSpec";
    for (Port *pp = w->ports; pp < p; pp++)
      if (!strcmp(pp->name, p->name))
	return "DataInterfaceSpec Name attribute duplicates another interface name";
    ezxml_t pSum, prot;
    const char *protFile;
    if ((err = tryChildInclude(x, file, "ProtocolSummary", &pSum, &protFile, true)))
      return err;
    if (pSum) {
      if (ezxml_child(spec, "Protocol"))
	return "cannot have both Protocol and ProtocolSummary";
      prot = 0;
      if ((err = CE::checkAttrs(pSum, "DataValueWidth", "DataValueGranularity",
			    "DiverDataSizes", "MaxMessageValues", "NumberOfOpcodes",
			    "VariableMessageLength", "ZeroLengthMessages", (void*)0)) ||
	  (err = CE::getNumber(pSum, "DataValueWidth", &p->wdi.dataValueWidth, 0, 8)) ||
	  (err = CE::getNumber(pSum, "DataValueGranularity", &p->wdi.dataValueGranularity, 0, 1)) ||
	  (err = CE::getBoolean(pSum, "DiverseDataSizes", &p->wdi.diverseDataSizes)) ||
	  (err = CE::getNumber(pSum, "MaxMessageValues", &p->wdi.maxMessageValues, 0, 1)) ||
	  (err = CE::getNumber(pSum, "NumberOfOpcodes", &p->wdi.numberOfOpcodes, 0, 1)) ||
	  (err = CE::getBoolean(pSum, "VariableMessageLength", &p->wdi.variableMessageLength)) ||
	  (err = CE::getBoolean(pSum, "ZeroLengthMessages", &p->wdi.zeroLengthMessages)))
	return err;
    } else {
      if ((err = tryChildInclude(x, file, "Protocol", &prot, &protFile, true)) ||
	  prot && (err = parseProtocol(p, prot)))
	return err;
    }
  }
  return 0;
}

const char *
parseHdlImpl(ezxml_t xml, const char *file, Worker *w) {
  const char *err;
  ezxml_t xctl;
  if ((err = parseSpec(xml, file, w)) ||
      (err = parseImplControl(xml, file, w, &xctl)))
    return err;
  Port *wci;
  unsigned extraPorts = 0;
  if (!w->noControl) {
    // Insert the control port at the beginning of the port list since we want
    // To always process the control port first if we have one
    Port *ports = myCalloc(Port, w->nPorts + 1);
    memcpy(ports + 1, w->ports, w->nPorts * sizeof(Port));
    free(w->ports);
    w->ports = ports;
    w->nPorts++;
    // Finish HDL-specific control parsing
    wci = w->ports;
    if (w->ctl.controlOps == 0)
      w->ctl.controlOps = 1 << ControlOpStart;
    if (xctl) {
      if ((err = CE::checkAttrs(xctl, GENERIC_IMPL_CONTROL_ATTRS, "ResetWhileSuspended",
			    "Clock", "MyClock", "Timeout", "Count", "Name", "Pattern",
			    (void *)0)) ||
	  (err = CE::getNumber(xctl, "Timeout", &wci->wci.timeout, 0, 0)) ||
	  (err = CE::getNumber(xctl, "Count", &wci->count, 0, 0)) ||
	  (err = CE::getBoolean(xctl, "ResetWhileSuspended",
			    &wci->wci.resetWhileSuspended)))
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
  }
  // Count up and allocate the ports that are HDL-specific
  for (ezxml_t x = ezxml_cchild(xml, "MemoryInterface"); x; x = ezxml_next(x))
    extraPorts++;
  for (ezxml_t x = ezxml_cchild(xml, "TimeInterface"); x; x = ezxml_next(x))
    extraPorts++;
  // Reallocate all the ports
  w->ports = myCrealloc(Port, w->ports, w->nPorts, extraPorts);
  wci = w->ports;
  Port *p = w->ports + w->nPorts ;
  w->nPorts += extraPorts;
  // Clocks depend on port names, so get those names in first pass(non-control ports)
  for (ezxml_t x = ezxml_cchild(xml, "MemoryInterface"); x; x = ezxml_next(x), p++)
    if (!(p->name = ezxml_cattr(x, "Name")))
      return "Missing \"Name\" attribute in MemoryInterface";
  for (ezxml_t x = ezxml_cchild(xml, "TimeInterface"); x; x = ezxml_next(x), p++)
    if (!(p->name = ezxml_cattr(x, "Name")))
      return "Missing \"Name\" attribute in TimeInterface";
  // Now we do clocks before interfaces since they may refer to clocks
  for (ezxml_t xc = ezxml_cchild(xml, "Clock"); xc; xc = ezxml_next(xc))
    w->nClocks++;
  // add one to allow for adding the WCI clock later
  w->clocks = myCalloc(Clock, w->nClocks + 1 + w->nPorts);
  Clock *c = w->clocks;
  for (ezxml_t xc = ezxml_cchild(xml, "Clock"); xc; xc = ezxml_next(xc), c++) {
    if ((err = CE::checkAttrs(xc, "Name", "Signal", "Home", (void*)0)))
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
  p = w->ports + (w->noControl ? 0 : 1);
  // Now lets look at the implementation-specific data interface info
  for (ezxml_t s = ezxml_cchild(xml, "StreamInterface"); s; s = ezxml_next(s)) {
    Port *dp;
    if ((err = CE::checkAttrs(s, "Name", "Clock", "DataWidth", "PreciseBurst",
			      "ImpreciseBurst", "Continuous", "Abortable",
			      "EarlyRequest", "MyClock", "RegRequest", "Pattern",
			      "NumberOfOpcodes",
			      (void*)0)) ||
	(err = checkDataPort(w, s, &dp)) ||
	(err = CE::getBoolean(s, "Abortable", &dp->wsi.abortable)) ||
	(err = CE::getBoolean(s, "RegRequest", &dp->wsi.regRequest)) ||
	(err = CE::getBoolean(s, "EarlyRequest", &dp->wsi.earlyRequest)) ||
	(err = CE::getNumber(s, "NumberOfOpcodes", &p->wdi.numberOfOpcodes, 0, p->wdi.numberOfOpcodes)))
      return err;
    dp->type = WSIPort;
    if ((dp->wdi.dataValueWidth * dp->wdi.dataValueGranularity) % dp->dataWidth &&
	!dp->wdi.zeroLengthMessages)
      dp->byteWidth = dp->dataWidth;
    else
      dp->byteWidth = dp->wdi.dataValueWidth;
  }
  for (ezxml_t m = ezxml_cchild(xml, "MessageInterface"); m; m = ezxml_next(m)) {
    Port *dp;
    if ((err = CE::checkAttrs(m, "Name", "Clock", "MyClock", "DataWidth", "PreciseBurst",
			  "MFlagWidth", "ImpreciseBurst", "Continuous", "ByteWidth",
			  "TalkBack", "Bidirectional", "Pattern", (void*)0)) ||
	(err = checkDataPort(w, m, &dp)) ||
	(err = CE::getNumber(m, "ByteWidth", &dp->byteWidth, 0, dp->dataWidth)) ||
	(err = CE::getBoolean(m, "TalkBack", &dp->wmi.talkBack)) ||
	(err = CE::getBoolean(m, "Bidirectional", &dp->wmi.bidirectional)) ||
	(err = CE::getNumber(m, "MFlagWidth", &dp->wmi.mflagWidth, 0, 0)))
      return err;
    dp->type = WMIPort;
    if (dp->dataWidth % dp->byteWidth)
      return "Specified ByteWidth does not divide evenly into specified DataWidth";
  }
  Port *mp = w->ports + w->nPorts - extraPorts;
  for (ezxml_t m = ezxml_cchild(xml, "MemoryInterface"); m; m = ezxml_next(m), mp++) {
    mp->type = WMemIPort;
    bool memFound = false;
    if ((err = CE::checkAttrs(m, "Name", "Clock", "DataWidth", "PreciseBurst", "ImpreciseBurst",
			  "MemoryWords", "ByteWidth", "MaxBurstLength", "WriteDataFlowControl",
			  "ReadDataFlowControl", "Count", "Pattern", (void*)0)) ||
	(err = checkClock(w, m, mp)) ||
	(err = CE::getNumber(m, "Count", &mp->count, 0, 0)) ||
	(err = CE::getNumber64(m, "MemoryWords", &mp->wmemi.memoryWords, &memFound, 0)) ||
	(err = CE::getNumber(m, "DataWidth", &mp->dataWidth, 0, 8)) ||
	(err = CE::getNumber(m, "ByteWidth", &mp->byteWidth, 0, 8)) ||
	(err = CE::getNumber(m, "MaxBurstLength", &mp->wmemi.maxBurstLength, 0, 0)) ||
	(err = CE::getBoolean(m, "ImpreciseBurst", &mp->impreciseBurst)) ||
	(err = CE::getBoolean(m, "PreciseBurst", &mp->preciseBurst)) ||
	(err = CE::getBoolean(m, "WriteDataFlowControl", &mp->wmemi.writeDataFlowControl)) ||
	(err = CE::getBoolean(m, "ReadDataFlowControl", &mp->wmemi.readDataFlowControl)))
      return err;
    if (!memFound || !mp->wmemi.memoryWords)
      return "Missing \"MemoryWords\" attribute in MemoryInterface";
    if (!mp->preciseBurst && !mp->impreciseBurst) {
      if (mp->wmemi.maxBurstLength > 0)
	return "MaxBurstLength specified when no bursts are enabled";
      if (mp->wmemi.writeDataFlowControl || mp->wmemi.readDataFlowControl)
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
  for (ezxml_t m = ezxml_cchild(xml, "TimeInterface"); m; m = ezxml_next(m), mp++) {
    if (foundWTI)
      return "More than one WTI specified, which is not permitted";
    mp->name = ezxml_cattr(m, "Name");
    if (!mp->name)
      mp->name = "time";
    mp->type = WTIPort;
    if ((err = CE::checkAttrs(m, "Name", "Clock", "SecondsWidth", "FractionWidth", "AllowUnavailable", "Pattern",
			  (void*)0)) ||
	(err = checkClock(w, m, mp)) ||
	(err = CE::getNumber(m, "SecondsWidth", &mp->wti.secondsWidth, 0, 32)) ||
	(err = CE::getNumber(m, "FractionWidth", &mp->wti.fractionWidth, 0, 0)) ||
	(err = CE::getBoolean(m, "AllowUnavailable", &mp->wti.allowUnavailable)))
      return err;
    mp->dataWidth = mp->wti.secondsWidth + mp->wti.fractionWidth;
    foundWTI = true;
    mp->pattern = ezxml_cattr(m, "Pattern");
  }
  // Now check that all clocks have a home and all ports have a clock
  c = w->clocks;
  for (unsigned i = 0; i < w->nClocks; i++, c++)
    if (c->port) {
      if (c->signal)
	return esprintf("Clock %s is owned by interface %s and has a signal name",
			c->name, c->port->name);
      //asprintf((char **)&c->signal, "%s_Clk", c->port->fullNameIn);
    } else if (!c->signal)
      return esprintf("Clock %s is owned by no port and has no signal name",
		      c->name);
  // now make sure clockPort references are sorted out
  p = w->ports;
  for (unsigned i = 0; i < w->nPorts; i++, p++) {
    if (p->clockPort)
      p->clock = p->clockPort->clock;
    if (p->count == 0)
      p->count = 1;
  }
  return 0;
}     


const char *
getWorker(Assembly *a, ezxml_t x, const char *aName, Worker **wp) {
  const char *wName = ezxml_cattr(x, aName);
  if (!wName)
    return esprintf("Missing \"%s\" attribute on connection", aName);
  Worker *w = a->workers;
  for (unsigned i; i < a->nWorkers; i++, w++)
    if (!strcmp(wName, w->implName)) {
      *wp = w;
      return 0;
    }
  return esprintf("Attribute \"%s\": Worker name \"%s\" not foundr",
		  aName, wName);
}

const char *
getPort(Worker *w, ezxml_t x, const char *aName, Port **pp) {
  const char *pName = ezxml_cattr(x, aName);
  if (!pName)
    return esprintf("Missing \"%s\" attribute for worker \"%s\"",
		    aName, w->implName);
  Port *p = w->ports;
  for (unsigned i; i < w->nPorts; i++, p++)
    if (!strcmp(pName, p->name)) {
      *pp = p;
      return 0;
    }
  return esprintf("Port name \"%s\" matches no port on worker \"%s\"",
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

// Attach a port to a connection
static void
attachPort(Connection *c, InstancePort *ip, const char *name, bool isProducer,
	   bool isExternal) {
  if (isProducer) {
    c->nProducers++;
    if (isExternal)
      c->nExtProducers++;
  } else {
    c->nConsumers++;
    if (isExternal)
      c->nExtConsumers++;
  }
  // Append to list for connection
  InstancePort **pp;
  for (pp = &c->ports; *pp; pp = &(*pp)->nextConn)
    ;
  *pp = ip;
  ip->connection = c;
  ip->isExternal = isExternal;
  ip->isProducer = isProducer;
  ip->name = name;
  c->nPorts++;
  if (isExternal)
    c->external = ip;
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
    i->clocks[nc] = aw->ports->clock;
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
const char *
parseRccAssy(ezxml_t xml, const char *file, Worker *aw) {
  const char *err;
  Assembly *a = &aw->assembly;
  aw->model = RccModel;
  aw->isAssembly = true;
  if ((err = CE::checkAttrs(xml, "Name", (void*)0)))
    return err;
  aw->implName = ezxml_cattr(xml, "Name");
  if (!aw->implName)
    aw->implName = "RccAssembly";
  for (ezxml_t x = ezxml_child(xml, "Worker"); x; x = ezxml_next(x))
    a->nWorkers++;
  Worker *w = a->workers = myCalloc(Worker, a->nWorkers);
  for (ezxml_t x = ezxml_child(xml, "Worker"); x; x = ezxml_next(x), w++) {
    const char *wXmlName = ezxml_cattr(x, "File");
    if (!wXmlName)
      return "Missing \"File\" attribute is \"Worker\" element";
    if ((err = parseWorker(wXmlName, file, w)))
      return err;
  }    
  return 0;
}
const char *
parseHdlAssy(ezxml_t xml, Worker *aw) {
  const char *err;
  Assembly *a = &aw->assembly;
  aw->isAssembly = true;
  if ((err = CE::checkAttrs(xml, "Name", "Pattern", "Language", (void*)0)))
     return err;
  a->isContainer = !strcasecmp(xml->name, "HdlContainer");
  // Count instances and workers
  for (ezxml_t x = ezxml_child(xml, "Instance"); x; x = ezxml_next(x))
    a->nInstances++;
  Instance *i = a->instances = myCalloc(Instance, a->nInstances);
  // Overallocate workers - they won't exceed nInstances.
  Worker *w = a->workers = myCalloc(Worker, a->nInstances); // may overallocate
  for (ezxml_t x = ezxml_child(xml, "Instance"); x; x = ezxml_next(x), i++) {
    err = 
      a->isContainer ?
      CE::checkAttrs(x, "Worker", "Name", "Index", "Interconnect", "IO",
		 (void*)0) :
      CE::checkAttrs(x, "Worker", "Name", (void*)0);
    if (err)
      return err;
    i->name = ezxml_cattr(x, "Name");   // Name attribute is in fact optional
    i->wName = ezxml_cattr(x, "Worker"); // Worker attribute is pathname
    if (a->isContainer) {
      bool idxFound;
      if ((err = CE::getNumber(x, "Index", &i->index, &idxFound, 0)))
	return err;
      if (!idxFound)
	return "Missing o\"Index\" attribute in instance in container assembly";
      const char
	*ic = ezxml_cattr(x, "Interconnect"),
	*io = ezxml_cattr(x, "IO");
      if (!i->wName) {
	// No worker means application instance
	if (!i->name)
	  return "Missing \"Name\" attribute for application instance in container";
	if (io || ic)
	  return "Application workers in container can't be interconnects or io";
	continue; // for app instances we just capture index.
      }
      if (ic) {
	if (io)
	  return "Container workers cannot be both IO and Interconnect";
	i->attach = ic;
	i->isInterconnect = true;
      } else if (io)
	i->attach = io;
      // we do allow for containers to have workers that are not io or interconnect
    } else {
      // Not a container, just the application
      if (!i->wName)
	esprintf("Missing \"Worker\" attribute for instance \"%s\"",
		 i->name ? i->name : "<no Name>");
    }
    // So we have an instance with a real live worker
    for (Instance *ii = a->instances; ii < i; ii++)
      if (!strcmp(i->wName, ii->wName))
	i->worker = ii->worker;
    if (!i->worker) {
      if ((err = parseWorker(i->wName, w->file, w)) ||
	  (err = deriveOCP(w)))
	return esprintf("in file %s: %s", i->wName, err);
      i->worker = w++;
    }
    // Allocate the instance-clock-to-assembly-clock map
    i->clocks = myCalloc(Clock *, i->worker->nClocks);
    i->ports = myCalloc(InstancePort, i->worker->nPorts);
    for (unsigned n = 0; n < i->worker->nPorts; n++) {
      i->ports[n].port = &i->worker->ports[n];
      i->ports[n].instance = i;
    }
  }
  a->nWorkers = w - a->workers;
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
	return esprintf("Duplicate instance named \"%s\" in assembly", i->name);
  }
  for (ezxml_t x = ezxml_child(xml, "Connection"); x; x = ezxml_next(x)) {
    if ((err = CE::checkAttrs(x, "Name", "External", (void*)0)))
      return err;
    a->nConnections++;
  }
  Connection *c = a->connections = myCalloc(Connection, a->nConnections);
  Port *p;
  for (ezxml_t x = ezxml_child(xml, "Connection"); x; x = ezxml_next(x), c++) {
    c->name = ezxml_cattr(x, "Name");
    for (ezxml_t at = ezxml_child(x, "Attach"); at; at = ezxml_next(at)) {
      const char *instName = ezxml_cattr(at, "Instance");
      if (!instName)
	return
	  esprintf("Missing \"Instance\" attribute in Attach subelement of "
		   "connection \"%s\"", c->name);
      n = 0;
      InstancePort *ip;
      for (i = a->instances; n < a->nInstances; n++, i++)
	if (!strcmp(i->name, instName)) {
	  const char *iName = ezxml_cattr(at, "Interface");
	  if (!iName)
	    return
	      esprintf("Missing \"Interface\" attribute in Attach subelement of"
		       "connection \"%s\"", c->name);
	  unsigned nn = 0;
	  for (p = i->worker->ports; nn < i->worker->nPorts; p++, nn++)
	    if (!strcmp(p->name, iName)) {
	      if (p->type != WSIPort && p->type != WMIPort)
		return "Connections for non-data ports not allowed";
	      ip = &i->ports[nn];
	      if (ip->connection)
		return
		  esprintf("Interface \"%s\" of worker instance \"%s\" "
			   "attached to both connections \"%s\" and \"%s\"",
			   iName, instName, ip->connection->name, c->name);
	      break;
	    }
	  if (nn >= i->worker->nPorts)
	    return esprintf("Interface \"%s\" not found on instance \"%s\" in "
			    "connection  \"%s\"", iName, instName, c->name);
	  break;
	}
      if (n >= a->nInstances)
	return esprintf("Instance \"%s\" not found for connection  \"%s\"",
			instName, c->name);
      attachPort(c, ip, p->name, p->wdi.isProducer, false);
    } // all (local) attachments to the connection
    if (!c->name)
      asprintf((char **)&c->name, "conn%d", (int)(c - a->connections));
    for (Connection *cc = a->connections; cc < c; cc++)
      if (!strcmp(cc->name, c->name))
	return esprintf("Duplicate connection named \"%s\" in assembly",
			c->name);
    const char *ext = ezxml_cattr(x, "External");
    if (ext) {
      bool isProducer;
      if (!strcasecmp(ext, "producer"))
	isProducer = true;
      else if (!strcasecmp(ext, "consumer"))
	isProducer = false;
      else
	return esprintf("Value of \"External\" attribute of connection \"%s\" is not "
			"\"consumer\" or \"producer\"", c->name);
      InstancePort *ip = myCalloc(InstancePort, 1);
      attachPort(c, ip, c->name, isProducer, true);
    }
  } // all connections
  // All parsing is done.
  // Now we fill in the top-level worker stuff.
  aw->specName = aw->implName;
  // Properties:  we only set the canonical hasDebugLogic property, which is a parameter.
  if ((err = ezxml_children(xml, doImplProp, aw)))
    return err;
  // Compute nPorts, at least 1 (wci) 
  aw->nPorts = 1;
  // add data plane external ports
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    aw->nPorts += c->nExtProducers + c->nExtConsumers;
  // add time and memory services (even though they might be coalesced)
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++) 
    if (i->worker) {
      unsigned nn;
      for (nn = 0, p = i->worker->ports; nn < i->worker->nPorts; nn++, p++)
	if (p->type == WTIPort || p->type == WMemIPort)
	  aw->nPorts++;
    }
  p = aw->ports = myCalloc(Port, aw->nPorts);
  // Clocks: coalesce all WCI clock and clocks with same reqts, into one wci, all for the assy
  aw->nClocks = 1; // first clock for all instance WCIs.
  Clock *clk = aw->clocks = myCalloc(Clock, aw->nPorts); // overallocate
  clk->signal = clk->name = "wci_Clk";
  Port *wci = p++;
  clk->port = wci;
  wci->name = "wci";
  wci->type = WCIPort;
  wci->myClock = true;
  wci->clock = clk++;
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker && i->worker->ports->type == WCIPort)
      i->ports->ordinal = wci->count++;
  InstancePort *ip;
  // Check for WCI and nonWCI clocks on a connection.  Set wci clocks where we can.
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++) {
    for (ip = c->ports; ip; ip = ip->nextConn)
      if (!ip->isExternal &&
	  ip->instance->worker->ports->type == WCIPort &&
	  ip->port->clock == ip->instance->worker->ports->clock)
	break;
    if (ip) {
      c->clock = wci->clock;
      for (ip = c->ports; ip; ip = ip->nextConn)
	if (!ip->isExternal)
	  ip->instance->clocks[ip->port->clock - ip->instance->worker->clocks] =
	    wci->clock;
    }
  }
  // Map all the wci clocks to the assy's wci clock
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    // Map the instance's WCI clock to the assembly's WCI clock
    if (i->worker && i->worker->ports->type == WCIPort)
      i->clocks[i->worker->ports->clock - i->worker->clocks] = wci->clock;
  // Combine all the connection clocks that are not WCI clocks
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    for (ip = c->ports; ip; ip = ip->nextConn)
      if (!ip->isExternal) {
	unsigned nc = ip->port->clock - ip->instance->worker->clocks;
	if (!c->clock)
	  // This connection doesn't have a clock yet, so its not on the WCI clock either
	  if (ip->instance->clocks[nc])
	    // The clock of the port is already mapped, so we just use it.
	    c->clock = ip->instance->clocks[nc];
	  else {
	    // The connection has no clock, and the port's clock is not mapped.
	    // We need a new top level clock
	    asprintf((char **)&clk->name, "%s_%s", i->name,
		     ip->port->clock->name);
	    if (ip->port->clock->signal)
	      asprintf((char **)&clk->signal, "%s_%s", i->name, ip->port->clock->signal);
	    else
	      clk->signal = clk->name;
	    clk->assembly = true;
	    aw->nClocks++;
	    c->clock = clk++;	
	    ip->instance->clocks[nc] = c->clock;
	    // FIXME inherit ip->port->clock constraints
	  }
	else if (ip->instance->clocks[nc]) {
	  // This port already has a mapped clock
	  if (ip->instance->clocks[nc] != c->clock)
	    return esprintf("Connection %s at interface %s of instance %s has clock conflict",
			    c->name, ip->port->name, ip->instance->name);
	} else {
	  // FIXME CHECK COMPATIBILITY OF c->clock with ip->port->clock
	  ip->instance->clocks[nc] = c->clock;
	}
      }
  // Now all data ports that are connected have mapped clocks and
  // all ports with WCI clocks are connected.  All that's left is
  // non-WCI: WTI, WMemI
  unsigned nWti = 0, nWmemi = 0;
  bool cantDataResetWhileSuspended = false;
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker) {
      unsigned nn;
      for (nn = 0, ip = i->ports; nn < i->worker->nPorts; nn++, ip++) {
	unsigned nc = ip->port->clock - ip->instance->worker->clocks;
	if (!i->clocks[nc]) {
	  if (ip->port->type == WSIPort || ip->port->type == WMIPort)
	    return esprintf("Unconnected data interface %s of instance %s has its own clock",
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
  // Now all clocks are done.  We create all the external ports.
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++)
    if (i->worker) {
      unsigned nn;
      Worker *iw = i->worker;
      for (nn = 0, ip = i->ports; nn < iw->nPorts; nn++, ip++) {
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
	  if (iw->ctl.sub32BitConfigProperties)
	    aw->ctl.sub32BitConfigProperties = true;
	  aw->ctl.controlOps |= iw->ctl.controlOps; // needed?  useful?
	  // Reset while suspended: This is really only interesting if all
	  // external data ports are only connected to ports of workers were this
	  // is true.  And the use-case is just that you can reset the
	  // infrastructure while maintaining worker state.  BUT resetting the
	  // CP could clearly reset anything anyway, so this is only relevant to
	  // just reset the dataplane infrastructure.
	  if (!pp->wci.resetWhileSuspended)
	    cantDataResetWhileSuspended = true;
	  ip->external = wci;
	  break;
	case WTIPort:
	  // We don't share ports since the whole point of WTi is to get
	  // intra-chip accuracy via replication of the time clients.
	  // We could have an option to use wires instead to make things smaller
	  // and less accurate...
	  {
	    Port *wti = p++;
	    *wti = *pp;
	    asprintf((char **)&wti->name, "wti%u", nWti++);
	    ip->external = wti;
	    wti->clock = i->clocks[pp->clock - i->worker->clocks];
	  }
	  break;
	case WMemIPort:
	  {
	    Port *wmemi = p++;
	    *wmemi = *pp;
	    asprintf((char **)&wmemi->name, "wmemi%u", nWmemi++);
	    ip->external = wmemi;
	    wmemi->clock = i->clocks[pp->clock - i->worker->clocks];
	  }
	  break;
	case WSIPort:
	case WMIPort:
	  // Data ports.  Check for unconnected ports that are not optional
	  if (!pp->wdi.isOptional &&
	      !ip->connection)
	    return
	      esprintf("Port %s of instance %s of worker %s"
		       " is not connected and not optional",
		       pp->name, i->name, i->worker->implName);
	  break;
	default:
	  return "Bad port type";
	}
      }
    }
  if (!cantDataResetWhileSuspended)
    wci->wci.resetWhileSuspended = true;
  // Create the external data ports on the assembly worker
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    if (c->nExtProducers || c->nExtConsumers) {
      Port *extPort = 0, *intPort = 0;
      for (ip = c->ports; ip; ip = ip->nextConn)
	if (ip->isExternal) {
	  assert(!extPort); // for now only one
	  ip->port = p++;
	  extPort = ip->port;
	} else {
	  assert(!intPort); // for now only one
	  intPort = ip->port;  // remember the last one
	}
      // Start by copying everything.
      *extPort = *intPort;
      extPort->pattern = 0;
      extPort->name = c->name;
      extPort->isExternal = true;
      extPort->clock = c->clock;
      if (!extPort->clock->port) {
	extPort->clock->port = extPort;
	extPort->myClock = true;
      }
      if (extPort->type == WSIPort)
	extPort->wsi.regRequest = false;
    }
  aw->nPorts = p - aw->ports;
  // Finish up initializing ports
  for (n = 0, p = aw->ports; n < aw->nPorts; n++, p++) {
    if (p->type != WCIPort)
      p->count = 1;
    p->worker = aw;
  }
  return 0;
}

// This is an HDL file, and perhaps an assembly
const char *
parseHdl(ezxml_t xml, const char *file, Worker *w) {
   const char *err;
  if ((err = CE::checkAttrs(xml, "Name", "Pattern", "Language", (void*)0)))
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
      return esprintf("in %s for %s: %s", xml->name, w->implName, err);
  } else if (!strcasecmp(xml->name, "HdlAssembly")) {
    if ((err = parseHdlAssy(xml, w)))
      return esprintf("in %s for %s: %s", xml->name, w->implName, err);
  } else
    return "file contains neither an HdlImplementation nor an HdlAssembly";
  // Whether a worker or an assembly, we derive the external OCP signals, etc.
  if ((err = deriveOCP(w)))
    return esprintf("in %s for %s: %s", xml->name, w->implName, err);
  Port *p = w->ports;
  unsigned wipN[NWIPTypes][2] = {{0}};
  for (unsigned i = 0; i < w->nPorts; i++, p++) {
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
  if (w->nPorts > 32)
    return "worker has more than 32 ports";
  w->model = HdlModel;
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
  if ((err = CE::checkAttrs(xml, "Name", "ExternMethods", "StaticMethods", "Threaded", (void*)0)))
    return err;
  // We use the pattern value as the method naming for RCC
  // and its presence indicates "extern" methods.
  w->pattern = ezxml_cattr(xml, "ExternMethods");
  w->staticPattern = ezxml_cattr(xml, "StaticMethods");
  ezxml_t xctl;
  if ((err = parseSpec(xml, file, w)) ||
      (err = parseImplControl(xml, file, w, &xctl)) ||
      (err = CE::getBoolean(xml, "Threaded", &w->rcc.isThreaded)))
    return err;
  // Parse data port implementation metadata: maxlength, minbuffers.
  for (ezxml_t x = ezxml_child(xml, "Port"); x; x = ezxml_next(x)) {
    if ((err = CE::checkAttrs(x, "Name", "MinBuffers", (void*)0)))
      return err;
    const char *name = ezxml_cattr(x, "Name");
    if (!name)
      return "Missing \"Name\" attribute on Port element if RccImplementation";
    Port *p;
    unsigned n;
    for (p = w->ports, n = 0; n < w->nPorts; n++, p++)
      if (!strcmp(p->name, name))
	break;
    if (n >= w->nPorts)
      return esprintf("No DataInterface named \"%s\" from Port element", name);
    if ((err = CE::getNumber(x, "MinBuffers", &p->wdi.minBuffers, 0, 0)))
      return err;
  }
  w->model = RccModel;
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
  if (!cp)
    cp = file;
  w->fileName = strdup(cp);
  char *lp = strrchr(w->fileName, '.');
  if (lp)
    *lp = 0;
  w->implName = ezxml_cattr(xml, "Name");
  if (!w->implName)
    w->implName = w->fileName;
  const char *name = ezxml_name(xml);
  if (name) {
    if (!strcasecmp("RccImplementation", name))
      return parseRcc(xml, file, w);
    if ((!strcasecmp("HdlImplementation", name) ||
	 !strcasecmp("HdlAssembly", name)))
      return parseHdl(xml, file, w);
    if (!strcasecmp("RccAssembly", name))
      return parseRccAssy(xml, file, w);
  }
#if 0
  if (name && !strcasecmp(xml->name, "ComponentAssembly"))
    return parseAssy(xml, w->file, w);
#endif
  return esprintf("\"%s\" is not a valid implemention type (RccImplementation, HdlImplementation, HdlAssembly, ComponentAssembly)", xml->name);
}

void cleanWIP(Worker *w){ (void)w;}

