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

#include <set>
#include "OcpiUtilExceptionApi.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilAssembly.h"

namespace OCPI {
  namespace Util {
    namespace OA = OCPI::API;
    namespace OE = OCPI::Util::EzXml;

    Assembly::Assembly(const char *file, const char **extraTopAttrs,
		       const char **extraInstAttrs, const PValue *params)
      : m_copy(NULL), m_xmlOnly(false), m_isImpl(false) {
      const char *cp = file;
      while (isspace(*cp))
	cp++;
      const char *err;
      if (*cp == '<') {
	size_t len = strlen(cp);
	m_copy = new char[len + 1];
	strcpy(m_copy, cp);
	err = OE::ezxml_parse_str(m_copy, len, m_xml);
      } else
	err = OE::ezxml_parse_file(file, m_xml);
      if (err || (err = parse(NULL, extraTopAttrs, extraInstAttrs, params)))
	throw Error("%s", err);
    }
    Assembly::Assembly(const std::string &string, const char **extraTopAttrs,
		       const char **extraInstAttrs, const PValue *params)
      : m_xmlOnly(false), m_isImpl(false) {
      m_copy = new char[string.size() + 1];
      strcpy(m_copy, string.c_str());
      const char *err = OE::ezxml_parse_str(m_copy, string.size(), m_xml);
      if (err || (err = parse(NULL, extraTopAttrs, extraInstAttrs, params)))
	throw Error("%s", err);
    }
    // FIXME:  we infer that this is an impl assy from this constructor.  Make it explicit?
    Assembly::Assembly(const ezxml_t top, const char *defaultName, bool a_isImpl,
		       const char **extraTopAttrs, const char **extraInstAttrs,
		       const PValue *params)
      : m_xml(top), m_copy(NULL), m_xmlOnly(true), m_isImpl(a_isImpl) {
      const char *err = parse(defaultName, extraTopAttrs, extraInstAttrs, params);
      if (err)
	throw Error("Error parsing assembly xml string due to: %s", err);
    }
    Assembly::~Assembly() {
      if (m_xml && !m_xmlOnly)
	ezxml_free(m_xml);
      delete [] m_copy;
    }

    unsigned Assembly::s_count = 0;

    const char *Assembly::
    parse(const char *defaultName, const char **extraTopAttrs, const char **extraInstAttrs,
	  const PValue *params) {
      // This is where common initialization is done except m_xml and m_copy
      m_doneInstance = -1;
      m_cMapPolicy = RoundRobin;
      m_processors = 0;
      ezxml_t ax = m_xml;
      const char *err;
      static const char *baseAttrs[] = { "name", "package", NULL};
      bool maxProcs = false, minProcs = false, roundRobin = false;
      // FIXME: move app-specific parsing up into library assy
      if ((err = OE::checkAttrsVV(ax, baseAttrs, extraTopAttrs, NULL)) ||
	  (err = OE::checkElements(ax, "instance", "connection", "policy", "property",
				   "external", NULL)) ||
	  (err = OE::getNumber(ax, "maxprocessors", &m_processors, &maxProcs)) ||
	  (err = OE::getNumber(ax, "minprocessors", &m_processors, &minProcs)) ||
	  (err = OE::getBoolean(ax, "roundrobin", &roundRobin)))
	return err;
      if (maxProcs)
	m_cMapPolicy = MaxProcessors;
      else if (minProcs)
	m_cMapPolicy = MinProcessors;
      else if (roundRobin)
	m_cMapPolicy = RoundRobin;
      else
	m_cMapPolicy = RoundRobin;
      ezxml_t plx = ezxml_cchild(ax, "policy");
      if (plx) {
	const char * tmp = ezxml_attr(plx, "mapping" );
	if ( tmp ) {
	  if (!strcasecmp(tmp, "maxprocessors"))
	    m_cMapPolicy = MaxProcessors;
	  else if (!strcasecmp(tmp, "minprocessors"))
	    m_cMapPolicy = MinProcessors;
	  else if (!strcasecmp(tmp, "roundrobin"))
	    m_cMapPolicy = RoundRobin;
	  else
	    return esprintf("Invalid policy mapping option: %s", tmp);
	}
	tmp  = ezxml_attr(plx, "processors");	
	if (tmp) {
	  m_processors = atoi(tmp);
	}
      }
	
      OE::getNameWithDefault(ax, m_name, defaultName ? defaultName : "unnamed%u", s_count);
      OE::getOptionalString(ax, m_package, "package");
      if (m_package.empty())
	m_package = "local";
      m_instances.resize(OE::countChildren(ax, "Instance"));
      Instance *i = &m_instances[0];
      unsigned n = 0;
      for (ezxml_t ix = ezxml_cchild(ax, "Instance"); ix; ix = ezxml_next(ix), i++, n++)
	if ((err = i->parse(ix, *this, n, extraInstAttrs, params)))
	  return err;
      const char *done = ezxml_cattr(ax, "done");
      if (done) {
	if ((err = getInstance(done, n)))
	  return err;
	m_doneInstance = (int)n;
      }
      // Note the mapped properties are AFTER the instances so that a value
      // set on an instance is overriden by one on a mapped property.
      // Not likely, but the instance level stuff may be shared someday
      m_mappedProperties.resize(OE::countChildren(ax, "property"));
      MappedProperty *p = &m_mappedProperties[0];
      for (ezxml_t px = ezxml_cchild(ax, "property"); px; px = ezxml_next(px), p++)
	if ((err = p->parse(px, *this)))
	  return err;
      n = 0;
      for (ezxml_t cx = ezxml_cchild(ax, "Connection"); cx; cx = ezxml_next(cx)) {
	Connection tmp;
	m_connections.push_back(tmp);
	if ((err = m_connections.back().parse(cx, *this, n, params)))
	  return err;
      }
      // Add top level externals that simply define single port external connections
      // name defaults from port.
      for (ezxml_t ex = ezxml_cchild(ax, "External"); ex; ex = ezxml_next(ex))
	if ((err = addExternalConnection(ex, params)))
	  return err;
      i = &m_instances[0];
      for (ezxml_t ix = ezxml_cchild(ax, "Instance"); ix; ix = ezxml_next(ix), i++)
	if ((err = i->parseConnection(ix, *this, params)))
	  return err;
      // Check instance parameters that don't name instances properly
      // Set the last/fourth (singleAssignment) parameter to true for those that should
      // only be a single assignment per  workers should only make a  once.
      if ((err = checkInstanceParams("selection", params, false, true)) ||
	  (err = checkInstanceParams("transport", params)) ||
	  (err = checkInstanceParams("transferRole", params)) ||
	  (err = checkInstanceParams("bufferCount", params)) ||
	  (err = checkInstanceParams("worker", params, false, true)) ||
	  (err = checkInstanceParams("property", params, true)))
	return err;
      return NULL;
    }

    // Given a assignment value that is an instance assignment, find the instance and modify
    // the parameter value to point past the = sign.  Two output args.
    const char *Assembly::
    findInstanceForParam(const char *pName, const char *&assign, unsigned &instn) {
      const char *eq = strchr(assign, '=');
      if (!eq)
	return esprintf("Parameter assignment for \"%s\", \"%s\" is invalid. "
			"Format is: <instance>=<parameter-value>", pName, assign);
      size_t len = eq - assign;
      for (unsigned nn = 0; assign && nn < m_instances.size(); nn++)
	if (!strncasecmp(assign, m_instances[nn].m_name.c_str(), len) &&
	    m_instances[nn].m_name.length() == len) {
	  instn = nn;
	  assign = eq + 1;
	  return NULL;
	}
      return esprintf("No instance found for \"%s\" assignment for \"%s\" parameter",
		      assign, pName);
    }

    // Error check parameters for that have instance names
    const char *Assembly::
    checkInstanceParams(const char *pName, const PValue *params, bool checkMapped, 
			bool singleAssignment) {
      const char *assign;
      // Keep track of which instances we have seen for this parameter, using ordinals
      std::set<unsigned> instancesSeen;
      for (unsigned n = 0; findAssignNext(params, pName, NULL, assign, n); ) {
	const char *eq = strchr(assign, '=');
	if (!eq || !eq[1]) // empty values are not allowed for any instance params (so far)
	  return esprintf("Parameter assignment '%s' is invalid. "
			  "Format is: [<instance>]=<parameter-value>", assign);
	size_t len = eq - assign;
	if (len == 0) // an empty assignment is ok as default for later ones
	  continue;
	for (unsigned nn = 0; assign && nn < m_instances.size(); nn++)
	  if (m_instances[nn].m_name.length() == len &&
	      !strncasecmp(assign, m_instances[nn].m_name.c_str(), len)) {
	    if (singleAssignment && !instancesSeen.insert(nn).second)
	      return esprintf("%s assignment '%s' is a reassignment of that instance.",
			      pName, assign);
	    assign = NULL;
	  }
	if (assign && checkMapped) {
	  MappedProperty *mp = &m_mappedProperties[0];
	  for (size_t nn = m_mappedProperties.size(); assign && nn; nn--, mp++)
	    if (mp->m_name.length() == len && !strncasecmp(assign, mp->m_name.c_str(), len))
	      assign = NULL;
	}
	if (assign)
	  return esprintf("No instance for %s assignment '%s'", pName, assign);
      }
      return NULL;
    }

    const char * Assembly::
    getInstance(const char *name, unsigned &n) {
      for (n = 0; n < m_instances.size(); n++)
	if (m_instances[n].m_name == name)
	  return NULL;
      return esprintf("No instance named \"%s\" found", name);
    }

    const char *Assembly::
    addConnection(const char *name, Connection *&c) {
      for (ConnectionsIter ci = m_connections.begin(); ci != m_connections.end(); ci++)
	if (!strcasecmp((*ci).m_name.c_str(), name))
	  return esprintf("Duplicate connection named '%s' in assembly", name);
      Connection tmp;
      m_connections.push_back(tmp);
      c = &m_connections.back();
      c->m_name = name;
      return NULL;
    }
    const char *Assembly::
    addPortConnection(unsigned from, const char *fromPort, unsigned to, const char *toPort,
		      const char *transport, const PValue *params) {
      std::string name = m_instances[from].m_name + "." + (fromPort ? fromPort : "output");
      Connection *c;
      Port *toP, *fromP;
      const char *err;
      if ((err = addConnection(name.c_str(), c)) ||
	  (err = c->addPort(*this, to, toPort, true, false, true, 0, params, toP)) ||
	  (err = c->addPort(*this, from, fromPort, false, false, true, 0, params, fromP)))
	return err;
      toP->m_connectedPort = fromP;
      fromP->m_connectedPort = toP;
      if (transport)
	c->m_parameters.add("transport", transport);
      return NULL;
    }
    // This is called to create an external connection either from the very short shortcut
    // as an attribute of an instance (saying this port should be externalized with its
    // own name), or with the other short cut: a top level "external" that just describes
    // the instance, port and other options.
    const char *Assembly::
    addExternalConnection(unsigned instance, const char *port, const PValue *params,
			  bool isInput, bool bidi, bool known) {
      Connection *c;
      const char *err;
      if ((err = addConnection(port, c)))
	return err;
      External &e = c->addExternal();
      e.init(port);
      Port *p;
      return c->addPort(*this, instance, port, isInput, bidi, known, 0, params, p);
    }

    const char *Assembly::
    addExternalConnection(ezxml_t a_xml, const PValue *params) {
      const char *err;
      // What is the name for this connection?
      std::string name, port;
      // We preparse some attributes of the external to get the connection name
      OE::getOptionalString(a_xml, name, "name");
      if ((err = OE::getRequiredString(a_xml, port, "port", "external")))
	return err;
      if (name.empty())
	name = port;
      Connection *c;
      if ((err = addConnection(name.c_str(), c)))
	return err;
      External &e = c->addExternal();
      unsigned dummy = 0;
      // These names default from the port name, and
      if ((err = e.parse(a_xml, name.c_str(), dummy, c->m_parameters)))
	  return err;
      // Now attach an internal port to this connection
      std::string iName;
      unsigned instance;
      Port *p;
      if ((err = OE::getRequiredString(a_xml, iName, "instance", "external")) ||
	  (err = getInstance(iName.c_str(), instance)) ||
	  (err = c->addPort(*this, instance, port.c_str(), false, false, false, e.m_index,
			    params, p)))
	return err;
      // An external that is external-based can specify a count
      if (e.m_count)
	c->m_count = e.m_count;
      return NULL;
    }
    // Note that this may be called both from an application/mapped property as well as an
    // instance property - we essentially merge the info from both, checking for inconsistencies
    const char *Assembly::Property::
    setValue(ezxml_t px) {
      const char *cp, *err;
      if ((cp = ezxml_cattr(px, "value"))) {
	if (ezxml_cattr(px, "valueFile"))
	  return esprintf("For instance property \"%s\", having both \"value\" and \"valueFile\""
			  " attributes is invalid", m_name.c_str());
	if (m_hasValue)
	  return esprintf("For instance property \"%s\", already has application value \"%s\"",
			  m_name.c_str(), m_value.c_str());
	m_hasValue = true;
	m_value = cp;
      } else if ((cp = ezxml_cattr(px, "valueFile"))) {
	if (m_hasValue)
	  return esprintf("For instance property \"%s\", already has application value \"%s\"",
			  m_name.c_str(), m_value.c_str());
	if ((err = file2String(m_value, cp, ',')))
	  return err;
	m_hasValue = true;
      }
      if ((cp = ezxml_cattr(px, "dumpFile"))) {
	if (cp && m_dumpFile.length())
	  return esprintf("For instance property \"%s\", duplicate dumpFile attributes",
			  m_name.c_str());
	m_dumpFile = cp;
      }
      return NULL;
    }

    void Assembly::Property::
    setValue(const char *name, const char *value) {
      m_name = name;
      m_value = value;
      m_hasValue = true;
    }

    const char *Assembly::MappedProperty::
    parse(ezxml_t px, Assembly &a) {
      const char *err;
      std::string instance;

      if ((err = OE::checkAttrs(px, "name", "value", "valuefile", "dumpFile", "instance", "property", NULL)) ||
	  (err = OE::getRequiredString(px, m_name, "name", "property")) ||
	  (err = OE::getRequiredString(px, instance, "instance", "property")) ||
	  (err = a.getInstance(instance.c_str(), m_instance)))
	return err;
      MappedProperty *p = &a.m_mappedProperties[0];
      for (size_t n = a.m_mappedProperties.size(); n && p < this; n--, p++)
	if (p->m_name == m_name)
	  return esprintf("Duplicate mapped property: %s", m_name.c_str());
      const char *cp = ezxml_cattr(px, "property");
      m_instPropName = cp ? cp : m_name.c_str();
      //      if (ezxml_cattr(px, "value") || ezxml_cattr(px, "valueFile") || ezxml_cattr(px, "dumpFile"))
      return a.m_instances[m_instance].addProperty(m_instPropName.c_str(), px);
    }

    const char *Assembly::Property::
    parse(ezxml_t px, Assembly::Property *first) {
      const char *err;
      if ((err = OE::checkAttrs(px, "name", "value", "valuefile", "dumpFile", NULL)) ||
	  (err = OE::getRequiredString(px, m_name, "name", "property")))
	return err;
      for (Property *p = first; p && p < this; p++)
	if (!strcasecmp(p->m_name.c_str(), m_name.c_str()))
	  return esprintf("Duplicate property \"%s\" in instance", m_name.c_str());
      return setValue(px);
    }

    // connect, then optionally, which local port (from) and which dest port (to).
    // external=port, connect=instance, then to or from?
    const char *Assembly::Instance::
    parseConnection(ezxml_t ix, Assembly &a, const PValue *params) {
      const char *err, *c, *e, *s;
      unsigned n;
      if ((c = ezxml_cattr(ix, "connect"))) {
	const char *transport;
	if (!findAssign(params, "transport", m_name.c_str(), transport))
	  transport = ezxml_cattr(ix, "transport");
	if ((err = a.getInstance(c, n)) ||
	    (err = a.addPortConnection(m_ordinal, ezxml_cattr(ix, "from"), n,
				       ezxml_cattr(ix, "to"), transport, params)))
	  return err;
      } else if (ezxml_cattr(ix, "transport"))
	return esprintf("Instance %s has transport attribute without connect attribute",
			m_name.c_str());

      if ((e = ezxml_cattr(ix, "external")) &&
	  (err = a.addExternalConnection(m_ordinal, e, params)))
	return err;
      if ((s = ezxml_cattr(ix, "slave"))) {
	if ((err = a.getInstance(s, m_slave)))
	  return err;
	else {
	  Instance &slave = a.m_instances[m_slave];
	  if (slave.m_hasMaster)
	    return esprintf("Instance %s is slave to multiple proxies",
			    slave.m_name.c_str());
	  else {
	    m_hasSlave = true;
	    slave.m_hasMaster = true;
	    slave.m_master = m_ordinal;
	  }
	}
      }
      return NULL;
    }

    const char *Assembly::Instance::
    addProperty(const char *name, ezxml_t px) {
      Property *p = &m_properties[0];
      size_t n;
      for (n = m_properties.size(); n ; n--, p++)
	if (!strcasecmp(p->m_name.c_str(), name)) {
	  // Top level property matches a worker-specific setting
	  // It is an error IF the top level property supplies a value that is already supplied
	  if (p->m_hasValue && (ezxml_cattr(px, "value") || ezxml_cattr(px, "valueFile")))
	    return esprintf("duplicate property value \"%s\" for instance \"%s\"",
			    name, m_name.c_str());
	  break;
	}
      if (!n) {
	m_properties.resize(m_properties.size() + 1);
	p = &m_properties.back();
	p->m_name = name;
      }
      return p->setValue(px);
    }

#if 0
    static void
    baseName(const char *path, std::string &out) {
      const char
	*dot = strrchr(path, '.'),
	*slash = strrchr(path, '/');
      if (slash)
	slash++;
      else
	slash = path;
      if (dot && dot > slash)
	out.assign(slash, dot - slash);
      else
	out = slash;
    }
#endif
    const char *Assembly::Instance::
    setProperty(const char *propAssign) {
      const char *eq = strchr(propAssign, '=');
      if (!eq)
	return esprintf("Property assignment '%s=%s' is invalid. "
			"Format is: <instance>=<prop>=<value>",
			m_name.c_str(), propAssign);
      std::string pName(propAssign, eq - propAssign);
      Property *p = &m_properties[0];
      for (unsigned nn = 0; nn < m_properties.size(); nn++, p++)
	if (!strcasecmp(pName.c_str(), p->m_name.c_str())) {
	  p->m_value = eq + 1;
	  p->m_hasValue = true;
	  propAssign = NULL;
	  break;
	}
      if (propAssign) {
	Property prop;
	prop.m_name = pName;
	prop.m_hasValue = true;
	prop.m_value = eq + 1;
	m_properties.push_back(prop);
      }
      return NULL;
    }
    // There is no non-default constructor so initialize here...
    const char *Assembly::Instance::
    parse(ezxml_t ix, Assembly &a, unsigned ordinal, const char **extraInstAttrs,
	  const PValue *params) {
      m_ordinal = ordinal;
      m_hasSlave = false;
      m_hasMaster = false;
      const char *err;
      static const char *instAttrs[] =
	{ "component", "Worker", "Name", "connect", "to", "from", "external", "selection",
	  "index", "externals", "slave", "transport", NULL};
      if ((err = OE::checkAttrsVV(ix, instAttrs, extraInstAttrs, NULL)) ||
	  (err = OE::getBoolean(ix, "externals", &m_externals)))
	return err;
      m_xml = ix;
      std::string component, myBase;
      const char *compName = 0;
      if (a.isImpl()) {
	if (ezxml_cattr(ix, "component"))
	  return "'component' attributes are invalid in this implementation assembly";
	if ((err = OE::getRequiredString(ix, m_implName, "worker", "instance")))
	  return err;
	baseName(m_implName.c_str(), myBase);
      } else if ((err = OE::getRequiredString(ix, component, "component", "instance")))
	return err;
      else {
	if ((compName = strrchr(component.c_str(), '.')))
	  compName++;
	else {
	  m_specName = a.m_package;
	  m_specName += ".";
	  compName = component.c_str();
	}
	m_specName += component;
	myBase = compName;
      }
      // FIXME: somehow pass in valid elements or do this test somewhere else...
      if ((err = OE::checkElements(ix, "property", "signal", NULL)))
	return err;
      // Figure out the name of this instance.
      if (!OE::getOptionalString(ix, m_name, "name")) {
      // default is component%u unless there is only one, in which case it is "component".
	unsigned me = 0, n = 0;
	for (ezxml_t x = ezxml_cchild(a.xml(), "instance"); x; x = ezxml_next(x)) {
	  std::string base;
	  const char
	    *c = ezxml_cattr(x, "component"),
	    *w = ezxml_cattr(x, "worker");
	  if (a.isImpl() && w)
	    baseName(w, base);
	  else if (!a.isImpl() && c) {
	    const char *dot = strrchr(c, '.');
	    base = dot ? dot + 1 : c;
	  }
	  if (base.size() && !strcasecmp(base.c_str(), myBase.c_str())) {
	    if (x == ix)
	      me = n;
	    n++;
	  }
	}
	if (n > 1)
	  formatString(m_name, "%s%u", myBase.c_str(), me);
	else
	  m_name = myBase;
      }
      if (!a.isImpl()) {
	if (!findAssign(params, "worker", m_name.c_str(), m_implName))
	  OE::getOptionalString(ix, m_implName, "worker");
      }
      if (!findAssign(params, "selection", m_name.c_str(), m_selection))
	OE::getOptionalString(ix, m_selection, "selection");
      ocpiDebug("Component: %s name: %s impl: %s spec: %s selection: %s",
		component.c_str(), m_name.c_str(), m_implName.c_str(), m_specName.c_str(),
		m_selection.c_str());
      m_properties.resize(OE::countChildren(ix, "property"));
      Property *p = &m_properties[0];
      for (ezxml_t px = ezxml_cchild(ix, "property"); px; px = ezxml_next(px), p++)
	if ((err = p->parse(px, &m_properties[0])))
	  return err;
      const char *propAssign;
      // Now deal with instance-based property parameters that might override the XML ones
      // First, process the parameters for ALL instances, then the parameters for specific
      // instances
      for (unsigned n = 0; findAssignNext(params, "property", NULL, propAssign, n); )
	if (propAssign[0] == '=' && (err = setProperty(propAssign + 1)))
	  return err;
      for (unsigned n = 0; findAssignNext(params, "property", m_name.c_str(), propAssign, n); )
	if ((err = setProperty(propAssign)))
	  return err;
      // Now check for additional or override values from parameters
      return m_parameters.parse(ix, "name", "component", "worker", "selection", "connect",
				"external", "from", "to", "externals", "slave", "transport",
				NULL);
    }

    Assembly::Connection::
    Connection() : m_count(0) {}

    const char *Assembly::Connection::
    parse(ezxml_t cx, Assembly &a, unsigned &n, const PValue *params) {
      const char *err;
      if ((err = OE::checkElements(cx, "port", "external", NULL)) ||
	  //	  (err = OE::checkAttrs(cx, "name", "transport", "external", "count", NULL)) ||
	  (err = OE::getNumber(cx, "count", &m_count, NULL, 1)))
	return err;
      
      OE::getNameWithDefault(cx, m_name, "conn%u", n);
      if ((err = m_parameters.parse(cx, "name", "external", "count", NULL)))
	return err;

      // This creates an external port of a connection defaulting
      // the name from the connection, and the role from this attribute
      const char *ext = ezxml_cattr(cx, "external");
      if (ext) {
	External &e = addExternal();
	// default the external's name from the connection's name
	if ((err = e.init(m_name.c_str(), ext)))
	  return err;
      }
      unsigned nExt = 0; // for name ordinals when unnamed
      for (ezxml_t x = ezxml_cchild(cx, "external"); x; x = ezxml_next(x), nExt++) {
	External tmp;
	m_externals.push_back(tmp);
	if ((err = m_externals.back().parse(x, "ext%u", nExt, m_parameters)))
	  return err;
      }
      if (OE::countChildren(cx, "port") < 1)
	return "no ports found under connection";
      Port *other = NULL;
      for (ezxml_t x = ezxml_cchild(cx, "port"); x; x = ezxml_next(x)) {
	Port tmp;
	m_ports.push_back(tmp);
	Port &p = m_ports.back();
	if ((err = p.parse(x, a, m_parameters, params)))
	  return err;
	if (other) {
	  ocpiAssert(!p.m_connectedPort && !other->m_connectedPort);
	  p.m_connectedPort = other;
	  other->m_connectedPort = &p;
	} else
	  other = &p;
      }
      return NULL;
    }

    const char *Assembly::Connection::
    addPort(Assembly &a, unsigned instance, const char *portName, bool isInput, bool bidi,
	    bool known, size_t index, const PValue *params, Assembly::Port *&port) {
      const char *err;
      Port tmp;
      m_ports.push_back(tmp);
      Port &p = m_ports.back();
      if ((err = p.init(a, portName, instance, isInput, bidi, known, index, params)))
	return err;
      port = &p;
      return NULL;
    }

#if 0
    // Find the value of a parameter for this port.  Return error.
    // "value" is out arg - NULL if not found.
    // FIXME: should this be nuked in favor of the implementation in 
    // OcpiLibraryAssembly that operates when all port names are known?
    static const char *
    findPortValue(const char *instName, const char *portName, const char *paramName,
		  const PValue *params, PValueList &pvlist) {
      const char *val;
      if (findAssign(params, paramName, instName, val)) {
	const char *eq = strchr(val, '=');
	if (!eq)
	  return esprintf("Port parameter assignment '%s' is invalid. "
			  "Format is: <port>=<parameter-value>", val);
	if (!strncasecmp(portName, val, eq - val))
	  pvlist.add(paramName, eq + 1);
      }
      return NULL;
    }
#endif
    const char *Assembly::Port::
    init(Assembly &a, const char *name, unsigned instance, bool isInput, bool bidir,
	 bool isKnown, size_t index, const PValue */*params*/) {
      if (name)
	m_name = name;
#if 0 // this is done at the application level since we don't know port names herea
     {
	const char *err, *iname = a.m_instances[instance].m_name.c_str();
	if ((err = findPortValue(iname, name, "transferRole", params, m_parameters)) ||
	    (err = findPortValue(iname, name, "bufferCount", params, m_parameters)))
	  return err;
	m_name = name;
      }	
#endif
      m_role.m_provider = isInput;
      m_role.m_bidirectional = bidir;
      m_role.m_knownRole = isKnown;
      m_instance = instance;
      m_role.m_provider = isInput;
      m_connectedPort = NULL;
      m_index = index;
      a.m_instances[instance].m_ports.push_back(this);
      return NULL;
    }

    Assembly::External &Assembly::Connection::
    addExternal() {
      m_externals.resize(m_externals.size() + 1);
      return m_externals.back();
    }
    const char *Assembly::Port::
    parse(ezxml_t x, Assembly &a, const PValue *pvl, const PValue *params) {
      const char *err;
      std::string iName;
      unsigned instance;
      size_t index;
      if ((err = OE::checkElements(x, NULL)) ||
	  (err = OE::getRequiredString(x, iName, "instance")) ||
	  (err = OE::getNumber(x, "index", &index)) ||
	  (err = a.getInstance(iName.c_str(), instance)))
	return err;
      std::string name, from, to;
      OE::getOptionalString(x, name, "name");
      OE::getOptionalString(x, from, "from");
      OE::getOptionalString(x, to, "to");
      bool isInput = false, isKnown = false;
      const char *onlyOne = "Only one of 'name', 'from' and 'to' attributes allowed in 'port' element";
      if (name.size()) {
	if (from.size() || to.size())
	  return onlyOne;
      } else if (from.size()) {
	if (to.size())
	  return onlyOne;
	isKnown = true;
	name = from;
      } else if (to.size()) {
	isInput = isKnown = true;
	name = to;
      } else
	return "One of 'name', 'from', or 'to' attribute must be present in 'port' element";
      // We don't know the role at all at this point
      if ((err = init(a, name.c_str(), instance, isInput, false, isKnown, index, params)))
	return err;
      return m_parameters.parse(pvl, x, "name", "instance", "from", "to", NULL);
    }

    Assembly::External::
      External()
      : m_index(0), m_count(1) {
      m_role.m_knownRole = false;
      m_role.m_bidirectional = false;
      m_role.m_provider = false;
    }
    // Initialization, whether parsed or not
    const char *Assembly::External::
    init(const char *name, const char *r) {
      if (name)
	m_name = name;
      if (r) {
	if (!strcasecmp(r, "provider") || !strcasecmp(r, "input") || !strcasecmp(r, "consumer") ||
	    !strcasecmp(r, "slave")) {
	  m_role.m_provider = true;
	  m_role.m_knownRole = true;
	} else if (!strcasecmp(r, "user") || !strcasecmp(r, "output") || !strcasecmp(r, "producer") ||
		   !strcasecmp(r, "master")) {
	  m_role.m_provider = false;
	  m_role.m_knownRole = true;
	} else if (!strcasecmp(r, "bidirectional")) {
	  m_role.m_bidirectional = true;
	  m_role.m_knownRole = true;
	} else if (*r)
	  return esprintf("Invalid external role: %s", r);
      }
      return NULL;
    }
    // There are two variants of "external", the one that is a child element
    // of "connection", and the one that is top level as shorthand for a
    // simple externalized instance port.
    const char *Assembly::External::
    parse(ezxml_t x, const char *defaultName, unsigned &n, const PValue *pvl) {
      // First decide on the name
      OE::getOptionalString(x, m_name, "name");
      if (m_name.empty())
	format(m_name, defaultName, n++);
      OE::getOptionalString(x, m_url, "url");
      std::string role;
      OE::getOptionalString(x, role, "role");
      // Parse all attributes except the explicit ones here.
      const char *err;
      if ((err = OE::getNumber(x, "index", &m_index)) ||
	  (err = OE::getNumber(x, "count", &m_count, NULL, 1)) ||
	  (err = init(NULL, role.empty() ? NULL : role.c_str())))
	return err;
      return m_parameters.parse(pvl, x, "name", "url", "provider", "port", "instance", "index", "count", NULL);
    }
    Assembly::Role::Role()
      : m_knownRole(false), m_bidirectional(false), m_provider(false) {
    }
  }
}

