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
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <strings.h>
#include <ctype.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilAssembly.h"
#include "wip.h"
#include "hdl.h"
#include "rcc.h"
#include "hdl-platform.h"
#include "hdl-container.h"
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

const char *platform = 0, *device = 0, *load = 0, *os = 0, *os_version = 0, *assembly = 0,
  *attribute, *platformDir;

Clock *Worker::
addClock() {
  Clock *c = new Clock;
  c->ordinal = m_clocks.size();
  m_clocks.push_back(c);
  return c;
}
// Check for implementation attributes common to data interfaces, several of which
// are able to override protocol-determined values.
// Take care of the case of implementation-specific ports (via implname);
const char *Worker::
checkDataPort(ezxml_t impl, Port *&sp) {
  const char
    *err,
    *name = ezxml_cattr(impl, "Name"),
    *portImplName = ezxml_cattr(impl, "implName");
  sp = NULL;
  if (name) {
    if ((err = getPort(name, sp)))
      return err;
    if (!sp->isData())
      return OU::esprintf("Name attribute of Stream/MessageInterface \"%s\" "
			  "matches a non-data spec port", name);
    if (sp->type != WDIPort)
      return OU::esprintf("Name attribute of Stream/MessageInterface \"%s\" "
			  "matches an implementation port, not a spec data port", name);
  } else if (!portImplName)
    return OU::esprintf("Missing \"Name\" or \"ImplName\" attribute of %s element",
			impl->name);
  return NULL;
}

// If the given element is xi:include, then parse it and return the parsed element.
// If not, *parsed is set to zero.
// If not optional then it MUST be the indicated element
// Also return the file name of the included file.
static const char *
tryInclude(ezxml_t x, const std::string &parent, const char *element, ezxml_t *parsed,
           std::string &child, bool optional) {
  *parsed = 0;
  const char *eName = ezxml_name(x);
  if (!eName || strcasecmp(eName, "xi:include"))
    return 0;
  const char *err;
  if ((err = OE::checkAttrs(x, "href", (void*)0)))
    return err;
  const char *incfile = ezxml_cattr(x, "href");
  if (!incfile)
    return OU::esprintf("xi:include missing an href attribute in file \"%s\"",
			parent.c_str());
  std::string ifile;
  if ((err = parseFile(incfile, parent, element, parsed, ifile, optional)))
    return OU::esprintf("Error in %s: %s", ifile.c_str(), err);
  child = ifile;
  return NULL;
}

// If this element is either the given element or an include of the given element...
// optional means the included file can be something else.
// If success, set *parsed, and maybe *childFile.
static const char *
tryChildInclude(ezxml_t x, const std::string &parent, const char *element,
                ezxml_t *parsed, std::string &childFile, bool optional = false) {
  const char *err = tryInclude(x, parent, element, parsed, childFile, optional);
  if (err || *parsed)
    return err;
  const char *eName = ezxml_name(x);
  if (!eName || strcasecmp(eName, element))
    return 0;
  *parsed = x;
  return 0;
}

// Find the single instance of a child, which might be xi:included
const char *
tryOneChildInclude(ezxml_t top, const std::string &parent, const char *element,
		   ezxml_t *parsed, std::string &childFile, bool optional) {
  *parsed = 0;
  const char *err = 0;
  for (ezxml_t x = OE::ezxml_firstChild(top); x; x = OE::ezxml_nextChild(x)) {
    const char *eName = ezxml_name(x);
    if (eName)
      if (!strcasecmp(eName, element))
	if (*parsed)
	  return OU::esprintf("found duplicate %s element where only one was expected",
			      element);
	else {
	  childFile = parent;
	  *parsed = x;
	}
      else {
	std::string file;
	ezxml_t found;
	if ((err = tryInclude(x, parent, element, &found, file, optional)))
	  return err;
	else if (found) {
	  if (*parsed)
	    return OU::esprintf("found duplicate %s element in file %s, "
				"included from file %s, where only one was expected",
				element, parent.c_str(), file.c_str());
	  else {
	    *parsed = found;
	    childFile = file;
	  }
	}
      }
  }
  if (!*parsed && !optional)
    return OU::esprintf("no %s element found under %s, whether included via xi:include or not",
			element, ezxml_name(top));
  return err;
}

const char *Worker::
addProperty(ezxml_t prop, bool includeImpl, bool anyIsBad)
{
  OU::Property *p = new OU::Property;
  
  const char *err =
    p->parse(prop, includeImpl, (unsigned)(m_ctl.ordinal++), this);
  // Now that have parsed the property "in a vacuum", do two context-sensitive things:
  // Override the default value of parameter properties
  // Skip debug properties if the debug parameter is not present.
  if (!err) {
    if (!p->m_isParameter && anyIsBad)
      return OU::esprintf("Property \"%s\" is not a parameter and so it invalid in this context",
			  p->m_name.c_str());
    // Now allow overrides of values.
    if (!strcasecmp(p->m_name.c_str(), "ocpi_debug"))
      m_debugProp = p;
    m_ctl.properties.push_back(p);
    p->m_isImpl = includeImpl;
    //    addAccess(*p);
    return NULL;
  }
  delete p;
  return err;
}

struct PropInfo {
  Worker *worker;
  bool isImpl; // Are we in an implementation context?
  bool anyIsBad;
  bool top;    // Are we in a top layer mixed with other elements?
  const char *parent;
  PropInfo(Worker *worker, bool isImpl, bool anyIsBad, const char *parent)
    : worker(worker), isImpl(isImpl), anyIsBad(anyIsBad), top(true), parent(parent) {}
};

// process something that might be a property, either at spec time or at impl time
// This tries to keep properties in order no matter where they occur
static const char *
doMaybeProp(ezxml_t maybe, void *vpinfo) {
  PropInfo &pinfo = *(PropInfo*)vpinfo;
  Worker *w = pinfo.worker;
  ezxml_t props = 0;
  std::string childFile;
  const char *err;
  if (pinfo.top) {
    if ((err = tryChildInclude(maybe, pinfo.parent, "ControlInterface", &props, childFile, true)))
      return err;
    if (props) {
      const char *parent = pinfo.parent;
      pinfo.parent = childFile.c_str();
      err = OE::ezxml_children(props, doMaybeProp, &pinfo);
      pinfo.parent = parent;
      return err;
    }
  }
  if (!props &&
      (err = tryChildInclude(maybe, pinfo.parent, "Properties", &props, childFile, pinfo.top)))
    return err;
  if (props) {
    //    if (pinfo.anyIsBad)
    // return "A Properties element is invalid in this context";
    bool save = pinfo.top;
    pinfo.top = false;
    const char *parent = pinfo.parent;
    pinfo.parent = childFile.c_str();
    err = OE::ezxml_children(props, doMaybeProp, &pinfo);
    pinfo.parent = parent;
    pinfo.top = save;
    return err;
  }
  const char *eName = ezxml_name(maybe);
  if (!eName)
    return 0;
  bool isSpec = !strcasecmp(eName, "SpecProperty");
  if (isSpec && !pinfo.isImpl)
    return "SpecProperty elements not allowed in component specification";
  if (!isSpec && strcasecmp(eName, "Property"))
    if (pinfo.top)
      return 0;
    else
      return OU::esprintf("Invalid child element '%s' of a 'Properties' element", eName);
  //  if (pinfo.anyIsBad)
  //    return "A Property or SpecProperty element is invalid in this context";
  const char *name = ezxml_cattr(maybe, "Name");
  if (!name)
    return "Property or SpecProperty has no \"Name\" attribute";
  OU::Property *p = NULL;
  for (PropertiesIter pi = w->m_ctl.properties.begin(); pi != w->m_ctl.properties.end(); pi++)
    if (!strcasecmp((*pi)->m_name.c_str(), name)) {
      p = *pi;
      break;
    }
  if (isSpec) {
    // FIXME mark a property as "impled" so we reject doing it more than once
    if (!p)
      return OU::esprintf("Existing property named \"%s\" not found", name);
    // So simply add impl info to the existing property.
    return p->parseImpl(maybe);
  } else if (p)
      return OU::esprintf("Property named \"%s\" conflicts with existing/previous property",
			  name);
  // All the spec attributes plus the impl attributes
  return w->addProperty(maybe, pinfo.isImpl, pinfo.anyIsBad);
}

const char *Worker::
doProperties(ezxml_t top, const char *parent, bool impl, bool anyIsBad) {
  PropInfo pi(this, impl, anyIsBad, parent);
  return OE::ezxml_children(top, doMaybeProp, &pi);
}

const char *parseControlOp(const char *op, void *arg) {
  Worker *w = (Worker *)arg;
  unsigned n = 0;
  const char **p;
  for (p = OU::Worker::s_controlOpNames; *p; p++, n++)
    if (!strcasecmp(*p, op)) {
      w->m_ctl.controlOps |= 1 << n;
      break;
    }
  return
    *p ? NULL : "Invalid control operation name in ControlOperations attribute";
}

static const char *
doScaling(ezxml_t x, void *worker) {
  return !strcasecmp(OE::ezxml_tag(x), "scaling") ? ((Worker*)worker)->doScaling(x) : NULL;
}
const char *Worker::
doScaling(ezxml_t x) {
  std::string name;
  const char *err = OE::getRequiredString(x, name, "name");
  if (err)
    return err;
  if (findProperty(name.c_str()))
    return OU::esprintf("Scaling parameter \"%s\" conflicts with property name", name.c_str());
  Scaling s;
  if ((err = s.parse(*this, x)))
    return err;
  if (m_scalingParameters.find(name) != m_scalingParameters.end())
    return OU::esprintf("Duplicate scaling parameter name: \"%s\"", name.c_str());
  m_scalingParameters[name] = s;
  return NULL;
}

const char *Worker::
addProperty(const char *xml, bool includeImpl) {
  // Add the built-in properties
  char *dprop = strdup(xml); // Make the contents persistent
  ezxml_t dpx = ezxml_parse_str(dprop, strlen(dprop));
  ocpiDebug("Adding ocpi_debug property xml %p", dpx);
  const char *err = addProperty(dpx, includeImpl, false);
  ezxml_free(dpx);
  return err;
}

const char *Worker::
addBuiltinProperties() {
  const char *err;
  if ((err = addProperty("<property name='ocpi_debug' type='bool' parameter='true' "
			 "          default='false'/>", true)) ||
      (err = addProperty("<property name='ocpi_endian' type='enum' parameter='true' "
			 "          default='little'"
      			 "          enums='little,big,dynamic'/>", true)))
    return err;
  return NULL;
}
// Parse the generic implementation control aspects (for rcc and hdl and other)
const char *Worker::
parseImplControl(ezxml_t &xctl, const char *firstRaw) {
  // Now we do the rest of the control interface
  const char *err;
  if ((xctl = ezxml_cchild(m_xml, "ControlInterface")) &&
      m_noControl)
    return "Worker has a ControlInterface element, but also has NoControl=true";
  // Allow overriding byte enables
  bool sub32;
  // either can set to true
  if ((err = OE::getBoolean(m_xml, "Sub32BitConfigProperties", &sub32)) ||
      !sub32 && xctl && (err = OE::getBoolean(xctl, "Sub32BitConfigProperties", &sub32)))
    return err;
  if (sub32)
    m_ctl.sub32Bits = true;
  // We take ops from either place as true
  if ((err = OU::parseList(ezxml_cattr(m_xml, "ControlOperations"), parseControlOp, this)) ||
      xctl &&
      (err = OU::parseList(ezxml_cattr(xctl, "ControlOperations"), parseControlOp, this)))
    return err;
  // Add the built-in properties
  if ((err = addBuiltinProperties()) ||
      (err = doProperties(m_xml, m_file.c_str(), true, false)))
    return err;
  // Now that we have all information about properties and we can actually
  // do the offset calculations and summarize the access type counts and flags
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
    OU::Property &p = **pi;
    // Raw properties must start on a 4 byte boundary
    if (firstRaw && !strcasecmp(p.m_name.c_str(), firstRaw))
      m_ctl.offset = OU::roundUp(m_ctl.offset, 4);
    if ((err = p.offset(m_ctl.offset, m_ctl.sizeOfConfigSpace, this)))
      return err;
    m_ctl.summarizeAccess(p);
  }
  // Allow overriding sizeof config space, giving priority to controlinterface
  uint64_t sizeOfConfigSpace;
  bool haveSize = false;
  if (xctl && (err = OE::getNumber64(xctl, "SizeOfConfigSpace", &sizeOfConfigSpace, &haveSize, 0)))
    return err;
  if (!haveSize &&
      (err = OE::getNumber64(m_xml, "SizeOfConfigSpace", &sizeOfConfigSpace, &haveSize, 0)))
    return err;
  if (haveSize) {
    if (sizeOfConfigSpace < m_ctl.sizeOfConfigSpace)
      return "SizeOfConfigSpace attribute of ControlInterface smaller than properties indicate";
    m_ctl.sizeOfConfigSpace = sizeOfConfigSpace;
  }
  // Scalability
  if ((err = OE::getBoolean(m_xml, "scalable", &m_scalable)) ||
      (err = OE::getBoolean(m_xml, "startBarrier", &m_ctl.startBarrier)))
    return err;
  // FIXME: have an expression validator
  OE::getOptionalString(m_xml, m_validScaling, "validScaling");
  if ((err = OE::ezxml_children(m_xml, ::doScaling, this)))
    return err;
  return 0;
}

// Parse the generic implementation local memories (for rcc and ocl and other)
  const char *Worker::
parseImplLocalMemory() {
  const char* err;
  for (ezxml_t x = ezxml_cchild(m_xml, "LocalMemory"); x; x = ezxml_next(x)) {
    LocalMemory* m = new LocalMemory();
    m_localMemories.push_back(m);
    if ((err = OE::checkAttrs(x, "Name", "SizeofLocalMemory", (void*)0)) )
      return err;
    m->name = ezxml_cattr(x, "Name");
    if (!m->name)
      return "Missing \"Name\" attribute on Local Memory element if OclWorker";
    if ((err = OE::getNumber(x, "SizeOfLocalMemory", &m->sizeOfLocalMemory, 0, 0)))
      return err;
  }
  return 0;
}

// Parse the control information about the component spec
  const char *Worker::
parseSpecControl(ezxml_t ps) {
  const char *err;
  if (ps &&
      ((err = OE::checkAttrs(ps, "SizeOfConfigSpace", "WritableConfigProperties",
			     "ReadableConfigProperties", "Sub32BitConfigProperties",
			     "Count", (void*)0)) ||
       (err = OE::getNumber64(ps, "SizeOfConfigSpace", &m_ctl.sizeOfConfigSpace, 0, 0)) ||
       (err = OE::getBoolean(ps, "WritableConfigProperties", &m_ctl.writables)) ||
       (err = OE::getBoolean(ps, "ReadableConfigProperties", &m_ctl.readables)) ||
       (err = OE::getBoolean(ps, "Sub32BitConfigProperties", &m_ctl.sub32Bits))))
    return err;
  return 0;
}

static const char *checkSuffix(const char *str, const char *suff, const char *last) {
  size_t nstr = last - str, nsuff = strlen(suff);
  const char *start = str + nstr - nsuff;
  return nstr > nsuff && !strncmp(suff, start, nsuff) ? start : str + nstr;
}

Protocol::
Protocol(Port &port)
  : m_port(port) {}

const char * Protocol::
parse(const char *file, ezxml_t prot)
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
    std::string ofile = m_port.m_worker->m_file;
    m_port.m_worker->m_file = file;
    const char *err = OU::Protocol::parse(prot);
    m_port.m_worker->m_file = ofile;
    return err;
  }
  return prot ? OU::Protocol::parse(prot) : NULL;
}

const char *Protocol::
parseOperation(ezxml_t op) {
  const char *err;
  std::string ifile;
  ezxml_t iprot = 0;
  if ((err = tryInclude(op, m_port.m_worker->m_file.c_str(), "Protocol", &iprot,
			ifile, false)))
    return err;
  // If it is an "include", basically recurse
  if (iprot) {
    std::string ofile = m_port.m_worker->m_file;
    m_port.m_worker->m_file = ifile;
    err = OU::Protocol::parse(iprot, false);
    m_port.m_worker->m_file = ofile;
    return err;
  }
  return OU::Protocol::parseOperation(op);
}

// The package serves two purposes: the spec and the impl.
// If the spec already has a package prefix, then it will only
// be used as the package of the impl.
const char *Worker::
findPackage(ezxml_t spec, const char *package) {
  if (!package)
    package = ezxml_cattr(spec, "package");
  if (package)
    m_package = package;
  else {
    std::string packageFileDir;
    // If the spec name already has a package, we don't use the package file name
    // to determine the package.
    const char *base =
      !strchr(m_specName, '.') && !m_specFile.empty() ? m_specFile.c_str() : m_file.c_str();
    const char *cp = strrchr(base, '/');
    const char *err;
    // If the specfile (first) or the implfile (second) has a dir,
    // look there for package name file.  If not, look in the CWD (the worker dir).
    if (cp)
      packageFileDir.assign(base, cp + 1 - base);

    // FIXME: Fix this using the include path maybe?
    std::string packageFileName = packageFileDir + "package-name";
    if ((err = OU::file2String(m_package, packageFileName.c_str()))) {
      // If that fails, try going up a level (e.g. the top level of a library)
      packageFileName = packageFileDir + "../package-name";
      if ((err = OU::file2String(m_package, packageFileName.c_str()))) {
	// If that fails, try going up a level and into "lib" where it my be generated
	packageFileName = packageFileDir + "../lib/package-name";
	if ((err = OU::file2String(m_package, packageFileName.c_str())))
	  return OU::esprintf("Missing package-name file: %s", err);
      }
    }
    for (cp = m_package.c_str(); *cp && isspace(*cp); cp++)
      ;
    m_package.erase(0, cp - m_package.c_str());
    for (cp = m_package.c_str(); *cp && !isspace(*cp); cp++)
      ;
    m_package.resize(cp - m_package.c_str());
  }
  return NULL;
}

const char *Worker::
parseSpec(const char *package) {
  const char *err;
  // xi:includes at this level are component specs, nothing else can be included
  ezxml_t spec = NULL;
  if ((err = tryOneChildInclude(m_xml, m_file, "ComponentSpec", &spec, m_specFile, true)))
    return err;
  const char *specAttr = ezxml_cattr(m_xml, "spec");
  if (specAttr) {
    if (spec)
      return "Can't have both ComponentSpec element (maybe xi:included) and a 'spec' attribute";
    if ((err = parseFile(specAttr, m_file, "ComponentSpec", &spec, m_specFile, false)))
      return err;
  } else if (!spec)
    return "missing componentspec element or spec attribute";
#if 0
  if (m_specFile == m_file) {
    // If not in its own file, then it must have a name attr
    if (!(m_specName = ezxml_cattr(spec, "Name")))
      return "Missing Name attribute for ComponentSpec";
  } else
#endif
   {
     // default the specname from the file of the current file,
     // which may in fact be the name of the worker file if the component spec is embedded
     std::string name, fileName;
     if ((err = getNames(spec, m_specFile.c_str(), "ComponentSpec", name, fileName)))
       return err;
     size_t len = strlen("-spec");
     if (name.length() > len) {
       const char *tail = name.c_str() + name.length() - len;
       if (!strcasecmp(tail, "-spec") || !strcasecmp(tail, "_spec"))
	 name.resize(name.size() - len);
     }
     m_specName = strdup(name.c_str());
   }
  // Find the package even though the spec package might be specified already
  if ((err = findPackage(spec, package)))
    return err;
  if (strchr(m_specName, '.'))
    m_specName = strdup(m_specName);
  else
    asprintf((char **)&m_specName, "%s.%s", m_package.c_str(), m_specName);
  if ((err = OE::checkAttrs(spec, "Name", "NoControl", "package", (void*)0)) ||
      (err = OE::getBoolean(spec, "NoControl", &m_noControl)))
    return err;
  // Parse control port info
  ezxml_t ps;
  std::string dummy;
  if ((err = tryOneChildInclude(spec, m_file, "PropertySummary", &ps, dummy, true)))
    return err;
  if ((err = doProperties(spec, m_file.c_str(), false, ps != NULL || m_noControl)))
    return err;
  if (m_noControl) {
    if (ps)
      return "NoControl specified, PropertySummary cannot be specified";
  } else if ((err = parseSpecControl(ps)))
    return err;
  // Now parse the data aspects, allocating (data) ports.
  if (ezxml_cchild(spec, "DataInterfaceSpec") && ezxml_cchild(spec, "Port"))
    return "Cannot use both 'datainterfacespec' and 'port' elements in the same spec";
  // First pass to establish ports with names
  for (ezxml_t x = ezxml_cchild(spec, "DataInterfaceSpec"); x; x = ezxml_next(x)) {
    new DataPort(*this, x, -1, err);
    if (err)
      return err;
  }
  for (ezxml_t x = ezxml_cchild(spec, "Port"); x; x = ezxml_next(x)) {
    new DataPort(*this, x, -1, err);
    if (err)
      return err;
  }
  // Second pass, really parse them, when they can refer to each other.
  for (PortsIter pi = m_ports.begin(); pi != m_ports.end(); pi++) {
    Port &p = **pi;
    if (p.isData() && (err = (**pi).parse()))
      return err;
  }
  return Signal::parseSignals(spec, m_file, m_signals, m_sigmap);
}

// Called for each non-data impl port type
const char *Worker::
initImplPorts(ezxml_t xml, const char *element, PortCreate &create) {
  const char *err;
  unsigned
    nTotal = OE::countChildren(xml, element),
    ordinal = 0;
  // Clocks depend on port names, so get those names in first pass(non-control ports)
  for (ezxml_t x = ezxml_cchild(xml, element); x; x = ezxml_next(x), ordinal++) {
#if 0
    
    if (!ezxml_cattr(x, "name")) {
      std::string name = prefix;
      if (nTotal != 1)
	OU::format(name, "%s%u", prefix, ordinal);
      ezxml_set_attr_d(xml, "name", name.c_str());
    }
#endif
    if (!create(*this, x, NULL, nTotal == 1 ? -1 : ordinal, err))
      return err;
  }
  return NULL;
}

// Parse a numeric value that might be overridden by assembly property values.
const char *Worker::
getNumber(ezxml_t x, const char *attr, size_t *np, bool *found,
	  size_t defaultValue, bool setDefault) {
  assert(np);
  const char *v = ezxml_cattr(x, attr);
  const char *err = getExprNumber(x, attr, *np, found, NULL, this);
  if (!err && !v && setDefault)
    *np = defaultValue;
  return err;
}

#if 0
const char *Worker::
getBoolean(ezxml_t x, const char *name, bool *b, bool trueOnly) {
  if (!m_instancePVs)
    return OE::getBoolean(x, name, b, trueOnly);
  return NULL;
}
#endif

const char*
extractExprValue(const OU::Property &p, const OU::Value &v, OU::ExprValue &val) {
  switch (p.m_baseType) {
  case OA::OCPI_Bool: val.number = v.m_Bool ? 1 : 0; break;
  case OA::OCPI_UChar: val.number = v.m_UChar; break;
  case OA::OCPI_UShort: val.number = v.m_UShort; break;
  case OA::OCPI_ULong: val.number = v.m_ULong; break;
  case OA::OCPI_Char: val.number = v.m_Char; break;
  case OA::OCPI_Short: val.number = v.m_Short; break;
  case OA::OCPI_Long: val.number = v.m_Long; break;
  case OA::OCPI_LongLong: val.number = v.m_LongLong; break;
  case OA::OCPI_ULongLong:
    if (v.m_ULongLong > INT64_MAX)
      return OU::esprintf("the \"%s\" parameter property value is too large.  Max is: %" PRIu64
			  ", value is %" PRIu64, p.m_name.c_str(), INT64_MAX, v.m_ULongLong);
    val.number = (int64_t)v.m_ULongLong;
    break;
  default:
    return OU::esprintf("the \"%s\" parameter property is of an unsupported type", 
			p.m_name.c_str());
  }
  val.isNumber = true;
  return NULL;
}

// This is a callback from the property parser used when some of the
// property attributes (like array dimensions, sequence or string length),
// are actually expressions in terms of other properties.
// We first look for instance property values applied in the assembly,
// and then look for parameter values directly
const char *Worker::
getValue(const char *sym, OU::ExprValue &val) const {
  if (m_instancePVs) {
    // FIXME: obviously a map would be good here..
    OU::Assembly::Property *ap = &(*m_instancePVs)[0];
    for (size_t n = m_instancePVs->size(); n; n--, ap++)
      if (ap->m_hasValue && !strcasecmp(sym, ap->m_name.c_str())) {
	// The value of the numeric attribute matches the name of a provided property
	// So we use that property value in place of this attribute's value
	// FIXME: why isn't this string value already parsed?
	// FIXME: the instance has parsed property values but it not accessible here
	size_t nval;
	if (OE::getUNum(ap->m_value.c_str(), &nval))
	  return OU::esprintf("Bad '%s' property value: '%s'",
			      ap->m_name.c_str(), ap->m_value.c_str());
	val.isNumber = true;
	val.number = nval;
	return NULL;
      }    
  }
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if (!strcasecmp((*pi)->m_name.c_str(), sym)) {
      OU::Property &p = **pi;
      if (!p.m_isParameter)
	return OU::esprintf("the '%s' property is invalid here since it is not a parameter",
			    sym);
      if (!p.m_default)
	return OU::esprintf("the '%s' parameter property has no value", sym);
      return extractExprValue(p, *p.m_default, val);
    }
  return OU::esprintf("There is no property named '%s'", sym);
}


// Get the filename and the name as required.
// Used when the name defaults from the filename
const char *
getNames(ezxml_t xml, const char *file, const char *tag, std::string &name,
	 std::string &fileName) {
  const char *xname = ezxml_name(xml);
  if (!xname)
    xname = "";
  if (tag && strcasecmp(tag, xname))
    return OU::esprintf("Found xml element: '%s' when expecting %s", xname, tag);
  if (file) {
    const char *cp = strrchr(file, '/');
    cp = cp ? cp + 1 : file;
    const char *lp = strrchr(cp, '.');
    fileName.assign(cp, lp ? lp - cp : strlen(cp));
  }
  if (fileName.empty())
    return OE::getRequiredString(xml, name, "name", ezxml_name(xml));
  OE::getOptionalString(xml, name, "name");
  if (name.empty())
    name = fileName;
  return NULL;
}

// The factory, which decides which class to instantiate
// This will evolve as more things are based on derived classes
Worker *Worker::
create(const char *file, const std::string &parentFile, const char *package, const char *outDir,
       Worker *parent, OU::Assembly::Properties *instancePVs, size_t paramConfig,
       const char *&err) {
  err = NULL;
  ezxml_t xml;
  std::string xf;
  if ((err = parseFile(file, parentFile, NULL, &xml, xf)))
    return NULL;
  const char *xfile = xf.c_str();
  const char *name = ezxml_name(xml);
  if (!name) {
    err = "Missing XML tag";
    return NULL;
  }
  Worker *w;
  if (!strcasecmp("HdlPlatform", name))
    w = HdlPlatform::create(xml, xfile, parent, err);
  else if (!strcasecmp("HdlConfig", name))
    w = HdlConfig::create(xml, NULL, xfile, parent, err);
  else if (!strcasecmp("HdlContainer", name))
    w = HdlContainer::create(xml, xfile, err);
  else if (!strcasecmp("HdlAssembly", name))
    w = HdlAssembly::create(xml, xfile, parent, err);
  else if (!strcasecmp("HdlDevice", name)) {
    //w = HdlDevice::get(file, parentFile.c_str(), parent, err);
    w = HdlDevice::create(xml, xfile, parent, instancePVs, err);
  } else if (!strcasecmp("RccAssembly", name))
    w = RccAssembly::create(xml, xfile, err);
  else if ((w = new Worker(xml, xfile, parentFile, Worker::Application, parent, instancePVs, err))) {
    if (!strcasecmp("RccImplementation", name) || !strcasecmp("RccWorker", name))
      err = w->parseRcc(package);
    else if (!strcasecmp("OclImplementation", name) || !strcasecmp("OclWorker", name))
      err = w->parseOcl();
    else if (!strcasecmp("HdlImplementation", name) || !strcasecmp("HdlWorker", name)) {
      if (!(err = OE::checkAttrs(xml, IMPL_ATTRS, GENERIC_IMPL_CONTROL_ATTRS, HDL_TOP_ATTRS,
				 HDL_IMPL_ATTRS, (void*)0)) &&
	  !(err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, (void*)0)))
	err = w->parseHdl(package);
    } else if (!strcasecmp("OclAssembly", name))
      err = w->parseOclAssy();
    else
      err = OU::esprintf("Unrecognized top level tag: \"%s\" in file \"%s\"", name, xfile);
    if (!err)
      err = w->setParamConfig(instancePVs, paramConfig);
  }
  if (err) {
    delete w;
    w = NULL;
  } else {
    w->m_outDir = outDir;
    if (w->m_library) {
      std::string lib(w->m_library);
      if (w->m_paramConfig && w->m_paramConfig->nConfig)
	OU::formatAdd(lib, "_c%zu", w->m_paramConfig->nConfig);
      addLibMap(lib.c_str());
    }
  }
  return w;
}

static unsigned nLibraries;
const char **libraries;
void
addLibrary(const char *lib) {
  for (const char **lp = libraries; lp && *lp; lp++) {
    if (!strcasecmp(lib, *lp))
      return;
  }
  libraries = (const char **)realloc(libraries, (nLibraries + 2) * sizeof(char *));
  libraries[nLibraries++] = lib;
  libraries[nLibraries] = 0;
}

// FIXME: use std::map
const char **mappedLibraries;
static const char **mappedDirs;
static unsigned nMaps;
// If just a library is added, it will be added to this list, but not have a file mapping
const char *
addLibMap(const char *map) {
  ocpiDebug("addLibMap: %s", map);
  const char *cp = strchr(map, ':');
  const char *newLib = cp ? strndup(map, cp - map) : strdup(map);
  if (cp)
    cp++;
  for (const char **mp = mappedLibraries; mp && *mp; mp++)
    if (!strcasecmp(newLib, *mp)) {
      const char **dir = &mappedDirs[mp - mappedLibraries];
      if (cp) // if a new dir is associated with this library
	if ((*dir)[0]) { // if there is an existing dir for this library
	  if (strcmp(cp, *dir))
	    return OU::esprintf("Inconsistent library mapping for %s: %s vs. %s",
				newLib, cp, *dir);
	} else
	  *dir = cp;
      return NULL;
    }
  mappedLibraries = (const char **)realloc(mappedLibraries, (nMaps + 2) * sizeof(char *));
  mappedDirs = (const char **)realloc(mappedDirs, (nMaps + 2) * sizeof(char *));
  mappedLibraries[nMaps] = newLib;
  mappedDirs[nMaps] = cp ? cp : "";
  ocpiDebug("addLibMap: lib is %s dir is %s", newLib, mappedDirs[nMaps]);
  mappedLibraries[++nMaps] = 0;
  mappedDirs[nMaps] = 0;
  return NULL;
}
const char *
findLibMap(const char *file) {
  const char *cp = strrchr(file, '/');
  for (unsigned n = 0; n < nMaps; n++) {
    size_t len = strlen(mappedDirs[n]);
    if (len && cp && len == (size_t)(cp - file) && !strncmp(mappedDirs[n], file, len))
      return mappedLibraries[n];
  }
  return NULL;
}

Control::
Control()
  : sizeOfConfigSpace(0), controlOps(0), offset(0), ordinal(0), firstRaw(NULL),
    startBarrier(false) {
  initAccess();
}
// For when we (re)scan/(re)count accesses
void Control::
initAccess() {
  writables = nonRawWritables = rawWritables = false;
  readables = nonRawReadables = rawReadables = false;
  sub32Bits = nonRawSub32Bits = volatiles = nonRawVolatiles = false;
  readbacks = nonRawReadbacks = rawReadbacks = rawProperties = false;
  nRunProperties = nNonRawRunProperties = nParameters = 0;
}
void Control::
summarizeAccess(OU::Property &p) {
  // All the raw stuff is done in the HDL parser.
  if (p.m_isParameter) {
    p.m_paramOrdinal = nParameters++;
    if (!p.m_isReadable)
      return;
  }
  if (p.m_isReadable)
    readables = true;
  if (p.m_isWritable)
    writables = true;
  if (p.m_isSub32)
    sub32Bits = true;
  if (p.m_isVolatile)
    volatiles = true;
  if (p.m_isVolatile || p.m_isReadable && !p.m_isWritable && !p.m_isParameter)
    readbacks = true;
  nRunProperties++;
}

// A minimum of zero means NO PARTITIONING
Scaling::Scaling()
  : m_min(0), m_max(1), m_modulo(1), m_default(1) {
}

const char *Scaling::
parse(Worker &w, ezxml_t x) {
  const char *err;
  if ((err = w.getNumber(x, "min", &m_min, NULL, 0, false)) ||
      (err = w.getNumber(x, "max", &m_max, NULL, 0, false)) ||
      (err = w.getNumber(x, "modulo", &m_modulo, NULL, 0, false)) ||
      (err = w.getNumber(x, "default", &m_default, NULL, 0, false)))
    return err;
  return NULL;
}

Worker::
Worker(ezxml_t xml, const char *xfile, const std::string &parentFile,
       WType type, Worker *parent, OU::Assembly::Properties *ipvs, const char *&err)
  : Parsed(xml, xfile, parentFile, NULL, err),
    m_model(NoModel), m_baseTypes(NULL), m_modelString(NULL), m_type(type), m_isDevice(false),
    m_wci(NULL), m_noControl(false), m_reusable(false), m_implName(m_name.c_str()),
    m_specName(NULL), m_isThreaded(false), m_maxPortTypeName(0), m_wciClock(NULL),
    m_endian(NoEndian), m_needsEndian(false), m_pattern(NULL), m_portPattern(NULL),
    m_staticPattern(NULL), m_defaultDataWidth(-1), m_language(NoLanguage), m_assembly(NULL),
    m_slave(NULL), m_emulate(NULL), m_library(NULL), m_outer(false), m_debugProp(NULL), 
    m_instancePVs(ipvs), m_mkFile(NULL), m_xmlFile(NULL), m_outDir(NULL), m_paramConfig(NULL),
  m_scalable(false), m_parent(parent), m_maxLevel(0), m_dynamic(false)
{
  const char *name = ezxml_name(xml);
  // FIXME: make HdlWorker and RccWorker classes  etc.
  if (!err && name && !strncasecmp("hdl", name, 3)) {
    // Parse things that the base class should parse.
    const char *lang = ezxml_cattr(m_xml, "Language");
    if (!lang)
      if (!strcasecmp("HdlContainer", name) || !strcasecmp("HdlConfig", name))
	m_language = VHDL;
      else if (!strcasecmp("HdlAssembly", name))
	m_language = Verilog;
      else
	err = "Missing Language attribute for HDL worker element";
    else if (!strcasecmp(lang, "Verilog"))
      m_language = Verilog;
    else if (!strcasecmp(lang, "VHDL"))
      m_language = VHDL;
    else
      err = OU::esprintf("Language attribute \"%s\" is not \"Verilog\" or \"VHDL\""
			 " in HdlWorker xml file: '%s'", lang, xfile ? xfile : "");
    m_model = HdlModel;
    m_modelString = "hdl";
    if ((m_library = ezxml_cattr(xml, "library")))
      ocpiDebug("m_library set from xml attr: %s", m_library);
    else if (xfile && (m_library = findLibMap(xfile)))
      ocpiDebug("m_library set from map from file %s: %s", xfile, m_library);
    else if (m_parent && (m_library = m_implName))
      ocpiDebug("m_library set from worker name: %s parent: %s", m_implName, parent->m_implName);
    else
      ocpiDebug("m_library not set");
    // Parse the optional endian attribute.
    // If not specified, it will be defaulted later based on protocols
    const char *myendian = ezxml_cattr(m_xml, "endian");
    if (myendian) {
      for (const char **ap = endians; *ap; ap++)
	if (!strcasecmp(myendian, *ap)) {
	  m_endian = (Endian)(ap - endians);
	  break;
	}
    }
  }
}

// Base class has no worker level expressions, but does all the ports
const char *Worker::
resolveExpressions(OU::IdentResolver &ir) {
  const char *err;
  for (PortsIter pi = m_ports.begin(); pi != m_ports.end(); pi++)
    if ((err = (**pi).resolveExpressions(ir)))
      return err;
  return NULL;
}

void Worker::
setParent(Worker *parent) {
  assert(!m_parent);
  m_parent = parent;
  if (parent && !m_library) {
    ocpiDebug("m_library set from worker name: %s", m_implName);
    m_library = m_implName;
  }
}

// FIXME: look for all the places this can be used..
Port *Worker::
findPort(const char *name, Port *except) const {
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *dp = m_ports[i];
    if (dp && dp->m_name.length() && !strcasecmp(dp->name(), name) && (!except || dp != except))
      return dp;
  }
  return NULL;
}
const char *Worker::
getPort(const char *name, Port *&p, Port *except) const {
  p = findPort(name, except);
  return p ? NULL :
    OU::esprintf("No port named \"%s\" was found in worker \"%s\"", name, m_implName);
}

Worker::~Worker() {
  deleteAssy();
}

const char *Worker::
emitAttribute(const char *attr) {
  if (!strcasecmp(attr, "language")) {
    printf(m_language == VHDL ? "VHDL" : "Verilog");
    return NULL;
  }
  if (!strcasecmp(attr, "workers")) {
    if (!m_assembly)
      return "Can't emit workers attribute if not an assembly";
    emitWorkersAttribute();
    return NULL;
  }
  return OU::esprintf("Unknown worker attribute: %s", attr);
}


Parsed::
Parsed(ezxml_t xml,        // The xml for this entity
       const char *file,   // The file with this as top level, possibly NULL
       const std::string &parent, // The file referencing this entity or file, possibly NULL
       const char *tag,    // The top level tag for this entity
       const char *&err)   // Errors detected during construction
  : m_file(file ? file : ""), m_parentFile(parent), m_xml(xml) {
  ocpiAssert(xml);
  err = getNames(xml, file, tag, m_name, m_fileName);
}

Clock::
Clock() 
  : port(NULL), assembly(false), ordinal(0) {
}

const char *Worker::
emitUuid(const OU::Uuid &) {
  return NULL;
}

// Emit the artifact XML.
const char *Worker::
emitArtXML(const char *wksFile) {
  const char *err;
  OU::Uuid uuid;
  OU::generateUuid(uuid);
  if ((err = emitUuid(uuid)))
    return err;
  FILE *f;
  if ((err = openOutput(m_implName, m_outDir, "", "-art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", m_file.c_str());
  fprintf(f,
	  " This file contains the artifact descriptor XML for the container\n"
	  " named \"%s\". It must be attached (appended) to the bitstream\n",
	  m_implName);
  fprintf(f, "  -->\n");
  OU::UuidString uuid_string;
  OU::uuid2string(uuid, uuid_string);
  fprintf(f,
	  "<artifact uuid=\"%s\"", uuid_string);
  if (os)         fprintf(f, " os=\"%s\"",        os);
  if (os_version) fprintf(f, " osVersion=\"%s\"", os_version);
  if (platform)   fprintf(f, " platform=\"%s\"",  platform);
  if (device)     fprintf(f, " device=\"%s\"", device);
  if (m_dynamic)  fprintf(f, " dynamic='1'");
  fprintf(f, ">\n");
  emitXmlWorkers(f);
  emitXmlInstances(f);
  emitXmlConnections(f);
  fprintf(f, "</artifact>\n");
  if (fclose(f))
    return "Could not close output file. No space?";
  if (wksFile)
    return emitWorkersHDL(wksFile);
  return 0;
}

const char *Worker::
deriveOCP() {
  //  printf("4095 %d 4096 %d\n", floorLog2(4095), floorLog2(4096));
  const char *err;
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    if (p->isOCP() &&
	(err = p->deriveOCP()))
      return err;
  }
  return NULL;
}

OU::Property *Worker::
findProperty(const char *name) const {
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if (!strcasecmp((*pi)->m_name.c_str(), name))
      return *pi;
  return NULL;
}
void Worker::
recordSignalConnection(Signal &/*s*/, const char */*from*/) {
}
void Worker::
emitTieoffSignals(FILE */*f*/) {
}
