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

#include "OcpiUtilExceptionApi.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilAssembly.h"

namespace OCPI {
  namespace Util {
    namespace OA = OCPI::API;
    namespace OE = OCPI::Util::EzXml;

    Assembly::Assembly(const char *file)
      : m_xml(ezxml_parse_file(file)), m_copy(NULL) {
      if (!m_xml)
	throw Error("Can't open or xml-parse assembly xml file: \"%s\"", file);
      const char *err = parse(m_xml);
      if (err)
	throw Error("Error parsing assembly xml file (\"%s\") due to: %s", file, err);
    }
    Assembly::Assembly(const std::string &string) {
      m_copy = new char[string.size() + 1];
      strcpy(m_copy, string.c_str());
      if (!(m_xml = ezxml_parse_str(m_copy, string.size())))
	throw Error("Can't xml-parse assembly xml string");
      const char *err = parse(m_xml);
      if (err)
	throw Error("Error parsing assembly xml string due to: %s", err);
    }
    Assembly::~Assembly() {
      if (m_xml)
	ezxml_free(m_xml);
      delete [] m_copy;
    }

    unsigned Assembly::s_count = 0;

    const char *Assembly::parse(ezxml_t ax) {
      const char *err;
      if ((err = OE::checkElements(ax, "instance", "connection", "policy", NULL)))
	return err;
      
      ezxml_t  p = ezxml_cchild(ax, "policy");
      if ( p ) {
	const char * tmp = ezxml_attr(p, "mapping" );
	if ( tmp ) policy.name = tmp;
	tmp  = ezxml_attr(p, "processors");	
	if ( tmp ) {
	  policy.nprocs = atoi(tmp);
	}
      }
	
      OE::getNameWithDefault(ax, m_name, "unnamed%u", s_count);
      m_instances.resize(OE::countChildren(ax, "Instance"));
      Instance *i = &m_instances[0];
      for (ezxml_t ix = ezxml_cchild(ax, "Instance"); ix; ix = ezxml_next(ix), i++)
	if ((err = i->parse(ix, ax)))
	  return err;
      m_connections.resize(OE::countChildren(ax, "Connection"));
      Connection *c = &m_connections[0];
      unsigned n = 0;
      for (ezxml_t  cx = ezxml_cchild(ax, "Connection"); cx; cx = ezxml_next(cx), c++, n++)
	if ((err = c->parse(cx, *this, n)))
	  return err;
      return NULL;
    }

    const char * Assembly::getInstance(const char *name, unsigned &n) {
      for (n = 0; n < m_instances.size(); n++)
	if (m_instances[n].m_name == name)
	  return NULL;
      return esprintf("No instance named \"%s\" found", name);
    }

    const char *Assembly::Property::parse(ezxml_t x) {
      const char *err = OE::getRequiredString(x, m_name, "name", "property");
      if (!err)
	err = OE::getRequiredString(x, m_value, "value", "property");
      return err;
    }
    const char *Assembly::Instance::parse(ezxml_t ix, ezxml_t ax) {
      const char *err = OE::getRequiredString(ix, m_specName, "worker", "instance");
      if (err ||
	  (err = OE::checkElements(ix, "property", NULL)))
	return err;
      OE::getOptionalString(ix, m_name, "name");
      OE::getOptionalString(ix, m_selection, "selection");
      // default is worker%d unless there is only one, in which case it is "worker".
      if (m_name.empty()) {
	unsigned me = 0, n = 0;
	for (ezxml_t x = ezxml_cchild(ax, "instance"); x; x = ezxml_next(x))
	  if (m_specName == ezxml_attr(x, "worker")) {
	    n++;
	    if (x == ix)
	      me = n;
	  }
	if (n > 1)
	  Misc::formatString(m_name, "%s%u", m_specName.c_str(), me);
	else
	  m_name = m_specName;
      }
      m_properties.resize(OE::countChildren(ix, "property"));
      Property *p = &m_properties[0];
      for (ezxml_t px = ezxml_cchild(ix, "property"); px; px = ezxml_next(px), p++)
	if ((err = p->parse(px)))
	  return err;
      return m_parameters.parse(ix, "name", "worker", "selection", NULL);
    }

    const char *Assembly::Connection::parse(ezxml_t cx, Assembly &a, unsigned &n) {
      const char *err;
      if ((err = OE::checkElements(cx, "port", "external", NULL)) ||
	  (err = OE::checkAttrs(cx, "name", "transport", NULL)))
	return err;
      
      OE::getNameWithDefault(cx, m_name, "conn%u", n);
      if ((err = m_parameters.parse(cx, "name", NULL)))
	return err;

      m_externals.resize(OE::countChildren(cx, "external"));
      External *e = &m_externals[0];
      unsigned nExt = 0;
      for (ezxml_t x = ezxml_cchild(cx, "external"); x; x = ezxml_next(x), e++, nExt++)
	if ((err = e->parse(x, nExt, m_parameters)))
	  return err;

      m_ports.resize(OE::countChildren(cx, "port"));
      if (m_ports.size() < 1)
	return "no ports found under connection";
      Port *p = &m_ports[0];
      for (ezxml_t x = ezxml_cchild(cx, "port"); x; x = ezxml_next(x), p++)
	if ((err = p->parse(x, a, m_parameters)))
	  return err;
      return NULL;
    }
    const char *Assembly::Port::parse(ezxml_t x, Assembly &a, const PValue *pvl) {
      const char *err;
      std::string iName;
      if ((err = OE::checkElements(x, NULL)) ||
	  (err = OE::getRequiredString(x, m_name, "name", "port")) ||
	  (err = OE::getRequiredString(x, iName, "instance", "port")) ||
	  (err = a.getInstance(iName.c_str(), m_instance)))
	return err;
      // Parse all attributes except the explicit ones here.
      return m_parameters.parse(pvl, x, "name", "instance", NULL);
    }
    const char *Assembly::External::parse(ezxml_t x, unsigned &n, const PValue *pvl) {
      OE::getNameWithDefault(x, m_name, "ext%u", n);
      OE::getOptionalString(x, m_url, "url");
      const char *err;
      if ((err = OE::getBoolean(x, "provider", &m_provider)))
	return err;
      // Parse all attributes except the explicit ones here.
      return m_parameters.parse(pvl, x, "name", "url", "provider", NULL);
    }
  }
}

