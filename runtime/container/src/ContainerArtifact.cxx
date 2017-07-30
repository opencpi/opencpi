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

#include "ContainerWorker.h"
#include "ContainerApplication.h"
#include "ContainerArtifact.h"

namespace OL = OCPI::Library;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;

namespace OCPI {
  namespace Container {
    Artifact::Artifact(OL::Artifact &lart, const OCPI::Util::PValue *) 
      : m_libArtifact(lart) {
      // FIXME: ref count loads from library artifact?
    }
    Artifact::~Artifact() {};
#if 0
    bool Artifact::hasUrl(const char *url) {
      return strcmp(url, myUrl) == 0;
    }
#endif
      // Return the value of a boolean attribute that defaults to false
    static bool boolAttrValue(ezxml_t x, const char *attr) {
      const char *val = ezxml_attr(x, attr);
      if (val) {
        if (!strcmp("true", val) || !strcmp("TRUE", val) || !strcmp("1", val))
          return true;
        if (strcmp("false", val) && strcmp("FALSE", val) && strcmp("0", val) &&
            val[0] != 0)
          throw OU::ApiError("Boolean attribute \"", attr, "\" has bad value \"",
                         val, "\"", NULL);
      }
      return false;
    }
    static bool hasAttrEq(ezxml_t x, const char *attrName, const char *val) {
      const char *attr = ezxml_attr(x, attrName);
      return attr && !strcmp(attr, val);
    }
    static ezxml_t findChildWithAttr(ezxml_t x, const char *cName, const char *aName,
                                     const char *value)
    {
      for (ezxml_t c = ezxml_child(x, cName); c; c = ezxml_next(c))
        if (hasAttrEq(c, aName, value))
          return c;
      return 0;
    }
    // We want an instance from an implementation, and optionally,
    // a specifically identified instance of that implementation,
    // when the artifact would contain such a thing.  Here are the cases:
    // Instance tag supplied, no instances in the artifact: error
    // Instance tag supplied, instance not in artifact: error
    // Instance tag supplied, instance already used: error
    // No instance tag supplied, no instances in artifact: ok
    // No instance tag supplied, instances in artifact, all used: error
    // No instance tag supplied, instances in artifact, one is available: ok
    Worker & Artifact::
    createWorker(Application &app, const char *appInstName,
		 const char *implTag, const char *instTag,
		 const OA::PValue* wProps,
		 const OA::PValue* wParams) {
      ezxml_t impl, inst, xml = m_libArtifact.xml();

      if (!implTag ||
          (!(impl = findChildWithAttr(xml, "worker", "specname", implTag)) &&
	   !(impl = findChildWithAttr(xml, "worker", "name", implTag)))) {
	throw OU::ApiError("No implementation found for \"", implTag, "\"", NULL);
      }

      if (instTag) {
        inst = findChildWithAttr(xml, "instance", "name", instTag);
        if (!inst)
          throw OU::ApiError("no worker instance named \"", instTag,
                             "\" found in this artifact", NULL);
        if (!implTag || !hasAttrEq(inst, "worker", implTag))
          throw OU::ApiError("worker instance named \"", instTag,
                             "\" is not a \"", implTag ? implTag : "<null>",
                             "\" worker", NULL);
	// Is any other worker in the whole container using this implementation instance
	// in this artifact?
	for (WorkersIter wi = m_workers.begin(); wi != m_workers.end(); wi++)
	  if (!strcmp((*wi)->instTag().c_str(), instTag))
	    throw OU::ApiError("worker instance named \"", instTag,
			   "\" already used", NULL);
      } else {
        inst = 0;
        // I didn't specify an instance, so I must want a floating
        // implementation.  Find one.
        if (boolAttrValue(impl, "connected"))
          throw OU::ApiError("specified implementation \"", implTag,
                             "\", is already connected", NULL);

#ifdef JK_LOOK_AT_ME
        ALSO ALLOW NO INSTANCE TAG WHEN THERE IS ONLY ONE ANYWAY
        if (!boolAttrValue(impl, "reusable"))
	  for (WorkersIter wi = m_workers.begin(); wi != m_workers.end(); wi++)
	    if (!strcmp((*wi)->implTag().c_str(), implTag))
	      throw OU::ApiError("non-reusable worker named \"", implTag,
                             "\" already used", NULL);
#endif

      }
      Worker &w = createWorker(app, appInstName, impl, inst, NULL, wParams);
      if (wProps)
	w.setProperties(wProps);
      return w;
    }

    Worker & Artifact::createWorker(Application &app,
				    const char *appInstName,
				    ezxml_t impl, ezxml_t inst, Worker *slave, bool hasMaster,
				    const OCPI::Util::PValue *wParams) {
      Worker &w = app.createWorker(this, appInstName, impl, inst, slave, hasMaster, wParams);
      m_workers.push_back(&w);
      w.initialize();
      return w;
    }
    void Artifact::removeWorker(Worker &w) {
      m_workers.remove(&w);
    }
    bool Artifact::hasArtifact(const void *art) {
      return (OL::Artifact *)(art) == &m_libArtifact;
      }
  }
}
