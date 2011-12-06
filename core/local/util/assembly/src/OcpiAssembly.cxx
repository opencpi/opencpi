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

#include "OcpiAssembly.h"
#include "OcpiUtilExceptionApi.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#if 0
#include "OcpiLibraryManager.h"


namespace OCPI {
  namespace Util {
    namespace OA = OCPI::API;
    namespace OE = OCPI::Util::EzXml;
    namespace OL = OCPI::Library;

    Assembly::Assembly(const ezxml_t x)
      : m_nInstances(0), m_instances(NULL), m_nConnections(0), m_connections(NULL), m_xml(x) {
      const char *err;
      if ((err = parse(x)))
	throw Error("Can't parse assembly xml due to: %s", err);
    }
    Assembly::Assembly(const char *file) 
      : m_nInstances(0), m_instances(NULL), m_nConnections(0), m_connections(NULL) {
      const char *err;
      if (!(m_xml = ezxml_parse_file(file)))
	throw Error("Can't open assembly xml file: \"%s\"", file);
      if ((err = parse(m_xml)))
	throw Error("Can't parse assembly xml file (\"%s\") due to: %s", file, err);
    }
    Assembly::~Assembly() {
      delete [] m_instances;
      delete [] m_connections;
    }
    const char *Assembly::parse(ezxml_t ax) {
      const char *err;
      const char *name = ezxml_cattr(ax, "name");
      m_name = name ? name : "unnamed";
      for (ezxml_t ix = ezxml_cchild(ax, "Instance"); ix; ix = ezxml_next(ix))
	m_nInstances++;
      Instance *i = m_instances = new Instance[m_nInstances];
      for (ezxml_t ix = ezxml_cchild(ax, "Instance"); ix; ix = ezxml_next(ix), i++)
	if ((err = i->parse(ix, i - m_instances)))
	  return err;
      for (ezxml_t cx = ezxml_cchild(ax, "Connection"); cx; cx = ezxml_next(cx))
	m_nConnections++;
      Connection *c = m_connections = new Connection[m_nConnections];
      for (ezxml_t  cx = ezxml_cchild(ax, "Connection"); cx; cx = ezxml_next(cx), c++)
	if ((err = c->parse(cx, c - m_connections, *this)))
	  return err;
      return NULL;
    }
    Instance *Assembly::getInstance(const char *name) {
      for (unsigned n = 0; n < m_nInstances; n++)
	if (m_instances[0].m_name == name)
	  return &m_instances[0];
      return NULL;
    }

    Instance::Instance() : m_nProperties(0), m_properties(NULL) {
    }
    Instance::~Instance() {
      delete [] m_properties;
    }
    static const char *doProps(ezxml_t x, unsigned nProperties, AssemblyProperty *properties) {
      const char *err;
      for (ezxml_t px = ezxml_cchild(x, "Property"); px; px = ezxml_next(px))
	nProperties++;
      AssemblyProperty *p = properties = new const char *[nProperties][2];
      for (ezxml_t px = ezxml_cchild(x, "Property"); px; px = ezxml_next(px), p++) {
	if ((err = OE::checkAttrs(px, "name", "value", NULL)))
	  return err;
	if (!((*p)[0] = ezxml_cattr(px, "name")))
	  return "missing \"name\" attribute for instance property";
	if (!((*p)[1] = ezxml_cattr(px, "value")))
	  return "missing \"value\" attribute for instance property";
      }
      return NULL;
    }

    const char *Instance::parse(ezxml_t ix, unsigned ordinal) {
      const char *err;
      if ((err = OE::checkAttrs(ix, "name", "worker", "selection", NULL)))
	return err;
      const char *name = ezxml_cattr(ix, "name");
      if (name)
	m_name = name;
      else
	Misc::formatString(m_name, "worker%u", ordinal);
      name = ezxml_cattr(ix, "worker");
      if (!name)
	return "missing \"worker\" attribute for instance";
      m_specName = name;
      if ((err = doProps(ix, m_nProperties, m_properties)))
	return err;
      return NULL;
    }
    bool Instance::offerImplementation(OL::Implementation &impl) {
      // Here we are being offered an implementation.
      // We return true if this one completely satisfies us.
      // But if we don't like it, or we want to look at all of them, we return false.
      // In the look-for-best case, we have an initial filter, and remember one that passes that filter.
      // 1. Apply the initial go-no-go filter - implement connectivity checking?
      //   But the connectivity check must also perform "selection" on the connected workers.
      //   The recusion is limited though.  But making a candidate choice will be linked to other
      //   specific candidates.  So the recursion may force a candidate for a different assembly instance.
      //   Within a given artifact there may be multiple implementation instances:
      //    We need to qualify:  assembly instances vs. artifact instances.
      //    So different instances may have different connectivity.
      //    Perhaps they should be considered simply different implementations unless they are somehow truly indistinguishable...
      //   The initial filter:
      //   -- we already have checked that the artifact is basically good.
      //   -- we need to know what the readonly property values are for the selection.
      //   -- how much of this can we delegate to the library manager?  Perhaps it can
      //      process the metric expression anyway.
      //      should it parse more of the metadata?  This means it might cache more work for us...
      //      Thus it might offer up the metadata worker - hmmm good...
      //      Should it be doing this iteration/search anyway?  All we are offering from the assembly is connectivity and selection??  What about containers?

    }
				       
    const char *Instance::findMappings(bool firstOnly) {
      OL::Artifact &a =
	OL::Manager::findArtifacts(impl, wParams, connections, inst, *this);
    }
    Connection::Connection()
      : m_nAttachments(0), m_attachments(NULL) {
    }
    Connection::~Connection() {
      delete [] m_attachments;
    }
    const char *Connection::parse(ezxml_t cx, unsigned ordinal, Assembly &ass) {
      const char *err;
      if ((err = OE::checkAttrs(cx, "name", "attach", NULL)))
	return err;
      const char *name = ezxml_cattr(cx, "name");
      if (name)
	m_name = name;
      else
	Misc::formatString(m_name, "conn%u", ordinal);
      for (ezxml_t ax = ezxml_cchild(cx, "attach"); ax; ax = ezxml_next(ax))
	m_nAttachments++;
      Attachment *a = m_attachments = new Attachment[m_nAttachments];
      for (ezxml_t ax = ezxml_cchild(cx, "attach"); ax; ax = ezxml_next(ax), a++)
	if ((err = a->parse(ax, ass)))
	  return err;
      return NULL;
    }
    Attachment::Attachment()
      : m_instance(NULL), m_port(NULL), m_url(NULL), m_nProperties(NULL), m_properties(NULL) {
    }
    const char *Attachment::parse(ezxml_t ax, Assembly &a) {
      const char *err;
      if ((err = OE::checkAttrs(ax, "worker", "url", "external", "port", NULL)))
	return err;
      const char *wName = ezxml_cattr(ax, "worker");
      if (wName) {
	if (ezxml_cattr(ax, "url"))
	  return "attach element has both \"worker\" and \"URL\" attributes";
	if (ezxml_cattr(ax, "external"))
	  return "attach element has both \"worker\" and \"external\" attributes";
	if (!(m_instance = a.getInstance(wName)))
	  return "attach element has worker name that doesn't match any worker";
      } else if ((m_url = ezxml_cattr(ax, "url"))) {
	if (ezxml_cattr(ax, "external"))
	  return "attach element has both \"URL\" and \"external\" attributes";
      } else if (!(m_port = ezxml_cattr(ax, "external")))
	return "an attach element must have a \"worker\", \"URL\", or \"external\" attribute";
      if ((err = doProps(ax, m_nProperties, m_properties)))
	return err;
      return NULL;
    }
  }
}

#endif
