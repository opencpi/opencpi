/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
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
#include "OcpiOsFileSystem.h"
#include "OcpiContainerApi.h"

#include "OcpiOsMisc.h"
#include "OcpiUtilValue.h"
#include "OcpiPValue.h"
#include "OcpiTimeEmit.h"
#include "OcpiUtilMisc.h"
#include "ContainerLauncher.h"
#include "OcpiApplication.h"

namespace OC = OCPI::Container;
namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;
namespace OL = OCPI::Library;
namespace OA = OCPI::API;
namespace OCPI {
  namespace API {
    // Deal with a deployment file referencing an app file
    static OL::Assembly &
    createLibraryAssembly(const char *file, ezxml_t &deployment, const PValue *params) {
      std::string appFile(file);
      deployment = NULL;
      ezxml_t xml = NULL;
      do {
	if (!OS::FileSystem::exists(appFile)) {
	  appFile += ".xml";
	  if (!OS::FileSystem::exists(appFile))
	    throw OU::Error("Error: application file %s (or %s) does not exist\n", file,
			    appFile.c_str());
	}
	const char *err = OE::ezxml_parse_file(appFile.c_str(), xml);
	if (err)
	  throw OU::Error("Can't parse application XML file \"%s\": %s", appFile.c_str(), err);
	if (!strcasecmp(ezxml_name(xml), "deployment")) {
	  if ((err = OE::getRequiredString(xml, appFile, "application")))
	    throw OU::Error("For deployment XML file \"%s\": %s", file, err);
	  deployment = xml;
	}
      } while (deployment == xml);
      std::string name;
      OU::baseName(appFile.c_str(), name);
      return *new OL::Assembly(xml, name.c_str(), params);
    }

    ApplicationI::ApplicationI(Application &app, const char *file, const PValue *params)
      : m_assembly(createLibraryAssembly(file, m_deployment, params)), m_apiApplication(app) {
      init(params);
    }
    ApplicationI::ApplicationI(Application &app, const std::string &string, const PValue *params)
      : m_deployment(NULL), m_assembly(*new OL::Assembly(string, params)),
	m_apiApplication(app)  {
      init(params);
    }
    ApplicationI::ApplicationI(Application &app, ezxml_t xml, const char *name,
			       const PValue *params)
      : m_deployment(NULL), m_assembly(*new OL::Assembly(xml, name, params)),
	m_apiApplication(app)  {
      init(params);
    }
    ApplicationI::ApplicationI(Application &app, OL::Assembly &assy, const PValue *params)
      : m_deployment(NULL), m_assembly(assy), m_apiApplication(app) {
      m_assembly++;
      init(params);
    }
    ApplicationI::~ApplicationI() {
      m_assembly--;
      delete [] m_instances;
      delete [] m_bookings;
      delete [] m_deployments;
      delete [] m_bestDeployments;
      delete [] m_properties;
      delete [] m_global2used;
      delete [] m_usedContainers;
      delete [] m_containers;
      if (m_containerApps) {
	for (unsigned n = 0; n < m_nContainers; n++)
	  delete m_containerApps[n];
	delete [] m_containerApps;
      }
      //      delete [] m_workers;
    }
    unsigned ApplicationI::
    addContainer(unsigned container, bool existOk) {
      ocpiAssert(existOk || !(m_allMap & (1 << container)));
      if ((m_allMap & (1 << container)))
	return m_global2used[container];
      m_usedContainers[m_nContainers] = container;
      m_allMap |= 1 << container;
      m_global2used[container] = m_nContainers;
      return m_nContainers++;
    }

    /*
    We made choices during the feasibility analysis, but here we want to add some policy.
    the default allocation will bias toward collocation, so this is basically to 
    spread things out.
    Since exclusive/bitstream allocations are not really adjustable, we just deal with the
    others.
    We haven't remembered ALL deployments, just the "best".
    We have preferred internally connected impls by scoring them up, which has inherently
    consolidated then.
    (preferred collocation)
    So somehow we need to keep the policy while keeping the fixed allocation...
    perhaps we need to record some sort of exclusivity and collocation constraints
    */
    // For dynamic instances only, distribute them according to policy
    void ApplicationI::
    policyMap( Instance *i, CMap & bestMap) {
      // bestMap is a bitmap of best available containers the implementation can be mapped to
      switch ( m_cMapPolicy ) {

      case MaxProcessors:
	// Limit use of processors to m_processors
	// If We have hit the limit, try to re-use.  If we can't, fall through to round robin
	if (m_nContainers >= m_processors)
	  for (unsigned n = 0; n < m_nContainers; n++) {
	    if ( m_currConn >= m_nContainers ) 
	      m_currConn = 0;
	    if (bestMap & (1 << m_usedContainers[m_currConn++])) {
	      i->m_container = m_currConn - 1;
	      return;
	    }
	  }
	// Not at our limit, let RR find the next available
	
      case RoundRobin:
	// Prefer adding a new container to an existing one, but if we can't
	// use a new one, rotate around the existing ones.
	for (unsigned n = 0; n < OC::Manager::s_nContainers; n++)
	  if ((bestMap & (1 << n)) && !(m_allMap & (1 << n))) {
	    m_currConn = m_nContainers;
	    i->m_container = addContainer(n);
	    ocpiDebug("instance %p used new container. best 0x%x curr %u cont %u",
		      i, bestMap, m_currConn, n);
	    return; // We added a new one - and used it
	  }
	// We have to use one we have since only those are feasible
	do {
	  if (++m_currConn >= m_nContainers)
	    m_currConn = 0;
	} while (!(bestMap & (1 << m_usedContainers[m_currConn])));
	i->m_container = m_currConn;
	ocpiDebug("instance %p reuses container. best 0x%x curr %u cont %u",
		  i, bestMap, m_currConn, m_usedContainers[m_currConn]);
	break;

      case MinProcessors:
	// Minimize processor - reuse when possible
	// use a new one, rotate around the existing ones.
	ocpiAssert(m_processors == 0);
	// Try to use first one already used that suits us
	for (unsigned n = 0; n < m_nContainers; n++)
	  if (bestMap & (1 << m_usedContainers[n])) {
	    i->m_container = n;
	    return;
	  }
	// Add one
	unsigned n;
	for (n = 0; n < OC::Manager::s_nContainers; n++)
	  if (bestMap & (1 << n))
	    break;
	i->m_container = addContainer(n);
      }
    }

    // Possible override the original policy in the xml
    void ApplicationI::
    setPolicy(const PValue *params) {
      uint32_t pcount;
      bool rr;
      if (OU::findULong(params, "MaxProcessors", pcount)) {
	m_cMapPolicy = MaxProcessors;
	m_processors = pcount;
      } else if (OU::findULong(params, "MinProcessors", pcount)) {
	m_cMapPolicy = MinProcessors;
	m_processors = pcount;
      } else if (OU::findBool(params, "RoundRobin", rr) && rr) {
	m_cMapPolicy = RoundRobin;
	m_processors = 0;
      }
    }
  
    // Check whether this candidate can be used relative to previous
    // choices for instances it is connected to
    bool ApplicationI::
    connectionsOk(OL::Candidate &c, unsigned instNum) {
      unsigned nPorts = c.impl->m_metadataImpl.nPorts();
      const OU::Assembly::Instance &ui = m_assembly.instance(instNum).m_utilInstance;
      std::string reject;
      OU::format(reject,
		 "For instance \"%s\" for spec \"%s\" rejecting implementation \"%s%s%s\" with score %u "
		 "from artifact \"%s\"",
		 ui.m_name.c_str(),
		 ui.m_specName.c_str(),
		 c.impl->m_metadataImpl.name().c_str(),
		 c.impl->m_staticInstance ? "/" : "",
		 c.impl->m_staticInstance ? ezxml_cattr(c.impl->m_staticInstance, "name") : "",
		 c.score, c.impl->m_artifact.name().c_str());
      for (unsigned nn = 0; nn < nPorts; nn++) {
	OU::Assembly::Port
	  *ap = m_assembly.assyPort(instNum, nn),
	  *other = ap ? ap->m_connectedPort : NULL;
	if (ap &&                          // if the port is even mentioned in the assembly?
	    other &&                       // if the port is connected in the assembly
	    other->m_instance < instNum && // if the other instance has been processed
	    m_assembly.                    // then check for prewired compatibility
	    badConnection(*c.impl, *m_instances[other->m_instance].m_impl, *ap, nn)) {
	  ocpiInfo("%s due to connectivity conflict", reject.c_str());
	  ocpiInfo("Other is instance \"%s\" for spec \"%s\" implementation \"%s%s%s\" "
		    "from artifact \"%s\".",
		    m_assembly.instance(other->m_instance).name().c_str(),
		    m_assembly.instance(other->m_instance).specName().c_str(),
		    m_instances[other->m_instance].m_impl->m_metadataImpl.name().c_str(),
		    m_instances[other->m_instance].m_impl->m_staticInstance ? "/" : "",
		    m_instances[other->m_instance].m_impl->m_staticInstance ?
		    ezxml_cattr(m_instances[other->m_instance].m_impl->m_staticInstance, "name") : "",
		    m_instances[other->m_instance].m_impl->m_artifact.name().c_str());
	  return false;
	}
      }
      // Check for master/slave correctness
      // Note that we know that the impl for a master indicates a slave since this
      // can be checked by the library layer.
      OU::Worker *mImpl = NULL, *sImpl = NULL;
      bool isMaster;
      if (ui.m_hasSlave && ui.m_slave < instNum) {
	mImpl = &c.impl->m_metadataImpl;
	sImpl = &m_instances[ui.m_slave].m_impl->m_metadataImpl;
	isMaster = true;
      } else if (ui.m_hasMaster && ui.m_master < instNum) {
	sImpl = &c.impl->m_metadataImpl;
	mImpl = &m_instances[ui.m_master].m_impl->m_metadataImpl;
	isMaster = false;
      }
      if (sImpl) { // the relationship exists, either way.  We are on the latter instance.
	std::string slaveWkrName = sImpl->name() + "." + sImpl->model();
	if (strcasecmp(mImpl->slave().c_str(), slaveWkrName.c_str())) {
	  // FIXME: make impl namespace part of this. implnames should really be qualified.
	  if (isMaster)
	    ocpiInfo("%s since its indicated slave worker \"%s\" doesn't match slave instance's worker \"%s\"",
		     reject.c_str(), mImpl->slave().c_str(), slaveWkrName.c_str());
	  else
	    ocpiInfo("%s since it doesn't match the worker \"%s\" indicated by the master instance",
		     reject.c_str(), mImpl->slave().c_str());
	  
	  return false;
	}
      }
      return true;
    }

    // FIXME: we assume that if the implementation is not a static instance then it can't conflict
    bool ApplicationI::
    bookingOk(Booking &b, OL::Candidate &c, unsigned n) {
      if (c.impl->m_staticInstance && b.m_artifact &&
	  (b.m_artifact != &c.impl->m_artifact ||
	   b.m_usedImpls & (1 << c.impl->m_ordinal))) {
	ocpiDebug("For instance \"%s\" for spec \"%s\" rejecting implementation \"%s%s%s\" with score %u "
		  "from artifact \"%s\" due to insufficient available containers",
		  m_assembly.instance(n).name().c_str(),
		  m_assembly.instance(n).specName().c_str(),
		  c.impl->m_metadataImpl.name().c_str(),
		  c.impl->m_staticInstance ? "/" : "",
		  c.impl->m_staticInstance ? ezxml_cattr(c.impl->m_staticInstance, "name") : "",
		  c.score, c.impl->m_artifact.name().c_str());
	return false;
      }
      return true;
    }
    
    static void
    checkPropertyValue(const char *name, const OL::Implementation &impl, const char *pName,
		       const char *value, unsigned *&pn, OU::Value *&pv) {
      OU::Property &uProp = impl.m_metadataImpl.findProperty(pName);
      if (uProp.m_isParameter)
	return;
      if (!uProp.m_isInitial && !uProp.m_isWritable)
	throw OU::Error("Cannot set property '%s' for instance '%s'. It is not writable.",
			pName, name);
      const char *err;
      *pn = uProp.m_ordinal; // remember position in property list 
      pv->setType(uProp);    // set the data type of the Value from the metadata property
      if ((err = pv->parse(value)))
	throw OU::Error("Value for property \"%s\" of instance \"%s\" of "
			"component \"%s\" is invalid for its type: %s",
			pName, name, impl.m_metadataImpl.specName().c_str(), err);
      pv++, pn++;
    }
    void ApplicationI::
    checkExternalParams(const char *pName, const OU::PValue *params) {
      // Error check instance assignment parameters for externals
      const char *assign;
      for (unsigned n = 0; OU::findAssignNext(params, pName, NULL, assign, n); ) {
	const char *eq = strchr(assign, '=');
	if (!eq)
	  throw OU::Error("Parameter assignment '%s' is invalid. "
			  "Format is: <external>=<parameter-value>", assign);
	size_t len = eq - assign;
	for (OU::Assembly::ConnectionsIter ci = m_assembly.m_connections.begin();
	     ci != m_assembly.m_connections.end(); ci++) {
	  const OU::Assembly::Connection &c = *ci;
	  if (c.m_externals.size()) {
	    const OU::Assembly::External &e = c.m_externals.front();
	    if (!strncasecmp(assign, e.m_name.c_str(), len)) {
	      assign = NULL;
	      break;
	    }
	  }
	}
	if (assign)
	  throw OU::Error("No external port for %s assignment '%s'", pName, assign);
      }
    }
    // Prepare all the property values for an instance
    void ApplicationI::
    prepareInstanceProperties(unsigned nInstance, const OL::Implementation &impl,
			      unsigned *&pn, OU::Value *&pv) {
      const char *name = m_assembly.instance(nInstance).name().c_str();
      const OU::Assembly::Properties &aProps = m_assembly.instance(nInstance).properties();
      // Prepare all the property values in the assembly, avoiding those in parameters.
      for (unsigned p = 0; p < aProps.size(); p++) {
	const char *pName = aProps[p].m_name.c_str();
	if (aProps[p].m_dumpFile.size()) {
	  // findProperty throws on error if bad name
	  OU::Property &uProp = impl.m_metadataImpl.findProperty(pName);
	  if (!uProp.m_isReadable)
	    throw OU::Error("Cannot dump property '%s' for instance '%s'. It is not readable.",
			    pName, name);
	}
	if (!aProps[p].m_hasValue)
	  continue;
	checkPropertyValue(name, impl, pName, aProps[p].m_value.c_str(), pn, pv);
      }
    }

    void ApplicationI::
    finalizeProperties(const OU::PValue *params) {
      Instance *i = m_instances;
      // Collect and check the property values for each instance.
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	// The chosen, best, feasible implementation for the instance
	const char *name = m_assembly.instance(n).name().c_str();
	const OU::Assembly::Properties &aProps = m_assembly.instance(n).properties();
	size_t nPropValues = aProps.size();
	const char *sDummy;
	// Count any properties that were provided in parameters specific to instance
	for (unsigned nn = 0; OU::findAssignNext(params, "property", name, sDummy, nn); )
	  nPropValues++;
	// Count any parameter properties that were mapped to this instance
	OU::Assembly::MappedProperty *mp = &m_assembly.m_mappedProperties[0];
	unsigned nDummy = 0;
	for (size_t nn = m_assembly.m_mappedProperties.size(); nn; nn--, mp++)
	  if (mp->m_instance == n &&
	      OU::findAssignNext(params, "property", mp->m_name.c_str(), sDummy, nDummy))
	    nPropValues++;
	if (nPropValues) {
	  // This allocation will include dump-only properties, which won't be put into the
	  // array by prepareInstanceProperties
	  OC::Launcher::Instance &li = m_launchInstances[n];
	  li.m_propValues.resize(nPropValues);
	  li.m_propOrdinals.resize(nPropValues);
	  OU::Value *pv = &li.m_propValues[0];
	  unsigned *pn = &li.m_propOrdinals[0];
	  prepareInstanceProperties(n, *i->m_impl, pn, pv);
	  nPropValues = pn - &li.m_propOrdinals[0];
	  li.m_propValues.resize(nPropValues);
	  li.m_propOrdinals.resize(nPropValues);
	}
      }
      // For all instances in the assembly, create the app-level property array
      m_nProperties = m_assembly.m_mappedProperties.size();
      i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++)
	m_nProperties += i->m_impl->m_metadataImpl.m_nProperties;
      // Over allocate: mapped ones plus all the instances' ones
      Property *p = m_properties = new Property[m_nProperties];
      OU::Assembly::MappedProperty *mp = &m_assembly.m_mappedProperties[0];
      for (size_t n = m_assembly.m_mappedProperties.size(); n; n--, mp++, p++) {
	p->m_property =
	  m_instances[mp->m_instance].m_impl->m_metadataImpl.
	  whichProperty(mp->m_instPropName.c_str());
	p->m_name = mp->m_name;
	p->m_instance = mp->m_instance;
	ocpiDebug("Instance %s (%u) property %s (%u) named %s in assembly", 
		  m_assembly.instance(p->m_instance).name().c_str(), p->m_instance,
		  mp->m_instPropName.c_str(), p->m_property, p->m_name.c_str());		    
      }
      i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	unsigned nProps;
	OU::Property *meta = i->m_impl->m_metadataImpl.properties(nProps);
	for (unsigned nn = 0; nn < nProps; nn++, meta++, p++) {
	  p->m_name = m_assembly.instance(n).name() + "." + meta->m_name;
	  p->m_instance = n;
	  p->m_property = nn;
	  ocpiDebug("Instance %s (%u) property %s (%u) named %s", 
		    m_assembly.instance(n).name().c_str(), n,
		    meta->m_name.c_str(), nn, p->m_name.c_str());		    
	  // Record dump file for this property if there is one.
	  const OU::Assembly::Properties &aProps = m_assembly.instance(n).properties();
	  p->m_dumpFile = NULL;
	  for (unsigned nn = 0; nn < aProps.size(); nn++)
	    if (aProps[nn].m_dumpFile.size() &&
		!strcasecmp(aProps[nn].m_name.c_str(),
			    meta->m_name.c_str())) {
	      p->m_dumpFile = aProps[nn].m_dumpFile.c_str();
	      break;
	    }
	}
      }
      assert((size_t)(p - m_properties) == m_nProperties);
    }
    void ApplicationI::
    dumpDeployment(unsigned score, Deployment *deployments) {

      ocpiDebug("Deployment with score %u is:", score);
      Instance *i = m_instances;
      Deployment *d = deployments;
      for (unsigned n = 0; n < m_nInstances; n++, d++, i++)
	ocpiDebug(" Instance %2u: Candidate: %u, Container: %u Instance %s%s%s in %s", 
		  n, d->candidate, d->container,
		  i->m_impl->m_metadataImpl.name().c_str(),
		  i->m_impl->m_staticInstance ? "/" : "",
		  i->m_impl->m_staticInstance ? ezxml_cattr(i->m_impl->m_staticInstance, "name") : "",
		  i->m_impl->m_artifact.name().c_str());
    }

    void ApplicationI::
    doInstance(unsigned instNum, unsigned score) {
      Deployment *d = m_deployments + instNum;
      Instance *i = m_instances + instNum;
      for (unsigned m = 0; m < i->m_nCandidates; m++) {
	OL::Candidate &c = m_assembly.instance(instNum).m_candidates[m];	  
	assert(c.impl);
	i->m_impl = c.impl; // temporary, but needed by (at least) connectionsOk
	ocpiDebug("doInstance %u %u %u", instNum, score, m);
	if (connectionsOk(c, instNum)) {
	  ocpiDebug("doInstance connections ok");
	  for (unsigned cont = 0; cont < OC::Manager::s_nContainers; cont++) {
	    Booking &b = m_bookings[cont];
	    ocpiDebug("doInstance container: cont %u feasible 0x%x", cont, i->m_feasibleContainers[m]);
	    if (i->m_feasibleContainers[m] & (1 << cont) && bookingOk(b, c, instNum)) {
	      d->container = cont;
	      d->candidate = m;
	      unsigned myScore = score + c.score;
	      ocpiDebug("doInstance ok");
	      if (instNum < m_nInstances-1) {
		Booking save = b;
		if (c.impl->m_staticInstance) {
		  b.m_artifact = &c.impl->m_artifact;
		  b.m_usedImpls |= 1 << c.impl->m_ordinal;
		}
		doInstance(instNum + 1, myScore);
		b = save;
	      } else {
		dumpDeployment(myScore, m_deployments);
		if (myScore > m_bestScore) {
		  memcpy(m_bestDeployments, m_deployments, sizeof(Deployment)*m_nInstances);
		  m_bestScore = myScore;
		  ocpiDebug("Setting BEST");
		}
	      }
	      if (!c.impl->m_staticInstance)
		break;
	    }
	  }
	}
      }
    }

    // The algorithmic way to figure out a deployment.
    void ApplicationI::
    planDeployment(const PValue *params) {
      m_bookings = new Booking[OC::Manager::s_nContainers];
      // Set the instance map policy
      setPolicy(params);
      m_deployments = new Deployment[m_nInstances];
      m_bestDeployments = new Deployment[m_nInstances];
      // First pass - make sure there are some containers to support some candidate
      // and remember which containers can support which candidates
      Instance *i = m_instances;
      for (size_t n = 0; n < m_nInstances; n++, i++) {
	//	i->m_libInstance = &m_assembly.instance(n);
	OL::Candidates &cs = m_assembly.instance(n).m_candidates;
	const OU::Assembly::Instance &ai = m_assembly.utilInstance(n);
	i->m_nCandidates = cs.size();
	i->m_feasibleContainers = new CMap[cs.size()];
	std::string container;
	if (!OU::findAssign(params, "container", ai.m_name.c_str(), container))
	  OE::getOptionalString(ai.xml(), container, "container");
	CMap sum = 0;
	for (unsigned m = 0; m < i->m_nCandidates; m++) {
	  m_curMap = 0;        // to accumulate containers suitable for this candidate
	  m_curContainers = 0; // to count suitable containers for this candidate
	  (void)OC::Manager::findContainers(*this, cs[m].impl->m_metadataImpl,
					    container.empty() ? NULL : container.c_str());
	  i->m_feasibleContainers[m] = m_curMap;
	  sum |= m_curMap;
	}
	if (!sum) {
	  if (m_verbose) {
	    fprintf(stderr, "No containers were found for deploying instance '%s' (spec '%s').\n"
		    "The implementations found were:\n",
		    ai.m_name.c_str(), ai.m_specName.c_str());
	    for (unsigned m = 0; m < i->m_nCandidates; m++) {
	      const OL::Implementation &lImpl = *cs[m].impl;
	      OU::Worker &mImpl = lImpl.m_metadataImpl;
	      fprintf(stderr, "  Name: %s, Model: %s, Platform: %s%s%s, File: %s\n",
		      mImpl.name().c_str(),
		      mImpl.model().c_str(),
		      lImpl.m_artifact.platform().c_str(),
		      lImpl.m_staticInstance ? ", Artifact instance: " : "",
		      lImpl.m_staticInstance ? ezxml_cattr(lImpl.m_staticInstance, "name") : "",
		      lImpl.m_artifact.name().c_str());
	    }
	  }
	  throw OU::Error("For instance \"%s\" for spec \"%s\": "
			  "no feasible containers found for %sthe %zu implementation%s found.",
			  ai.m_name.c_str(), ai.m_specName.c_str(),
			  i->m_nCandidates == 1 ? "" : "any of ",
			  i->m_nCandidates,
			  i->m_nCandidates == 1 ? "" : "s");
	}
      }
      
      // Second pass - search for best feasible choice
      // FIXME: we are assuming broadly that dynamic instances have universal connectivity
      // FIXME: we are assuming that an artifact is exclusive if is has static instances.
      // FIXME: we are assuming that if an artifact has a static instance, all of its instances are

      m_bestScore = 0;
      doInstance(0, 0);
      if (m_bestScore == 0)
	throw OU::Error("There are no feasible deployments for the application given the constraints");
      // Record the implementation from the best deployment
      i = m_instances;
      Deployment *d = m_bestDeployments;
      for (unsigned n = 0; n < m_nInstances; n++, i++, d++)
	i->m_impl = m_assembly.instance(n).m_candidates[d->candidate].impl;
      
      // Up to now we have just been "planning" and not doing things.
      // Now invoke the policy method to map the dynamic instances to containers
      // and also add the containers for the static instances
      i = m_instances;
      d = m_bestDeployments;
      for (unsigned n = 0; n < m_nInstances; n++, i++, d++) {
	const OL::Implementation &impl = *i->m_impl;
	if (impl.m_staticInstance) {
	  unsigned cNum = d->container;
	  i->m_container = addContainer(cNum, true);
	} else
	  policyMap(i, i->m_feasibleContainers[d->candidate]);
      }
    }
    // The explicit way to figure out a deployment from a file
    void ApplicationI::
    importDeployment(const char *file, ezxml_t xml, const PValue *params) {
      if (!xml) {
	const char *err = OE::ezxml_parse_file(file, xml);
	if (err)
	  throw OU::Error("Error parsing deployment file: %s", err);
      }
      if (!ezxml_name(xml) || strcasecmp(ezxml_name(xml), "deployment"))
	throw OU::Error("Invalid top level element \"%s\" in deployment file \"%s\"",
			ezxml_name(xml) ? ezxml_name(xml) : "", file);
      Instance *i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	ezxml_t xi;
	const char *iname = m_assembly.instance(n).name().c_str();
	for (xi = ezxml_cchild(xml, "instance"); xi; xi = ezxml_next(xi)) {
	  const char *name = ezxml_cattr(xi, "name");
	  if (!name)
	    throw OU::Error("Missing \"name\" attribute for instance in deployment file \"%s\"",
			    file);
	  if (!strcasecmp(name, iname))
	    break;
	}
	if (!xi)
	  throw
	    OU::Error("Missing instance element for instance \"%s\" in deployment file", iname);
	const char
	  *spec = ezxml_cattr(xi, "spec"),
	  *worker = ezxml_cattr(xi, "worker"),
	  *model = ezxml_cattr(xi, "model"),
	  *artifact = ezxml_cattr(xi, "artifact"),
	  *instance = ezxml_cattr(xi, "instance");
	i->m_containerName = ezxml_cattr(xi, "container");
	if (!spec || !worker || !model || !i->m_containerName || !artifact)
	  throw
	    OU::Error("Missing attributes for instance element \"%s\" in deployment file."
		      "  All of spec/worker/model/container/artifact must be present.", iname);
	/*
	 * Importing a deployment means bypassing the library mechanism, or at least not
	 * relying on it.  Basically we make our own library out of the specific artifacts
	 * indicated in the deployment.
	 */
	OL::Artifact &art = OL::Manager::getArtifact(artifact, NULL);
	OL::Implementation *impl = art.findImplementation(spec, instance);
	if (!impl)
	  throw OU::Error("For deployment instance \"%s\", worker for spec %s/%s not found "
			  " in artifact \"%s\"", iname, spec, instance ? instance : "",
			  artifact);
	i->m_impl = impl;
	if (!m_assembly.instance(n).resolveUtilPorts(*impl, m_assembly))
	  throw OU::Error("Port mismatch for instance \"%s\" in artifact \"%s\"",
			  iname, artifact);
	bool execution;
	if (OU::findBool(params, "execution", execution) && !execution)
	  continue;
	OC::Container *c = OC::Manager::find(i->m_containerName);
	if (!c)
	  throw OU::Error("For deployment instance \"%s\", container \"%s\" was not found",
			  iname, i->m_containerName);
	i->m_container = addContainer(c->ordinal(), true);
      }
    }
    void ApplicationI::
    init(const PValue *params) {
      // In order from class definition except for instance-related
      m_bookings = NULL;
      m_properties = NULL;
      m_nProperties = 0;
      m_curMap = 0;
      m_curContainers = 0;
      m_allMap = 0;
      m_global2used = new unsigned[OC::Manager::s_nContainers];
      m_nContainers = 0;
      m_usedContainers = new unsigned[OC::Manager::s_nContainers];
      m_containers = NULL;    // allocated when we know how many we are using
      m_containerApps = NULL; // ditto
      m_doneWorker = NULL;
      m_cMapPolicy = RoundRobin;
      m_processors = 0;
      m_currConn = OC::Manager::s_nContainers - 1;
      m_launched = false;
      m_deployments = NULL;
      m_bestDeployments = NULL;
      m_verbose = false;
      OU::findBool(params, "verbose", m_verbose);
      OU::findBool(params, "hex", m_hex);
      // Initializations for externals may add instances to the assembly
      initExternals(params);
      // Now that we have added any extra instances for external connections, do
      // instance-related initializations
      m_nInstances = m_assembly.nInstances();
      m_instances = new Instance[m_nInstances];
      // Check that params that reference instances are valid.
      const char *err;
      if ((err = m_assembly.checkInstanceParams("container", params, false)))
	throw OU::Error("%s", err);
      // This array is sized and initialized here since it is needed for property finalization
      m_launchInstances.resize(m_nInstances);
      // We are at the point where we need to either plan or import the deployment.
      const char *dfile = NULL;
      if (m_deployment || OU::findString(params, "deployment", dfile))
	importDeployment(dfile, m_deployment, params);
      else
	planDeployment(params);
      // All the implementation selection is done, so now do the final check of properties
      // since properties can be implementation specific
      finalizeProperties(params);
      Instance *i = m_instances;
      if (m_verbose) {
	fprintf(stderr, "Actual deployment is:\n");
	for (unsigned n = 0; n < m_nInstances; n++, i++) {
	  const OL::Implementation &impl = *i->m_impl;
	  OC::Container &c = OC::Container::nthContainer(m_usedContainers[i->m_container]);
	  std::time_t bd = OS::FileSystem::lastModified(impl.m_artifact.name());
	  char tbuf[30];
	  ctime_r(&bd, tbuf);
	  fprintf(stderr,
		  " Instance %2u %s (spec %s) on %s container %s, using %s%s%s in %s dated %s", 
		  n, m_assembly.instance(n).name().c_str(),
		  m_assembly.instance(n).specName().c_str(),
		  c.m_model.c_str(), c.name().c_str(),
		  impl.m_metadataImpl.name().c_str(),
		  impl.m_staticInstance ? "/" : "",
		  impl.m_staticInstance ? ezxml_cattr(impl.m_staticInstance, "name") : "",
		  impl.m_artifact.name().c_str(), tbuf);
	}
      }
    }
    // Initialize our own database of connections from the OU::Assembly connections
    void ApplicationI::
    initConnections() {
      m_launchConnections.resize(m_assembly.m_connections.size());
      OC::Launcher::Connection *lc = &m_launchConnections[0];
      for (OU::Assembly::ConnectionsIter ci = m_assembly.m_connections.begin();
	   ci != m_assembly.m_connections.end(); ci++, lc++) {
	for (OU::Assembly::Connection::PortsIter pi = (*ci).m_ports.begin();
	     pi != (*ci).m_ports.end(); pi++) {
	  OU::Assembly::Role &r = (*pi).m_role;
	  assert(r.m_knownRole && !r.m_bidirectional);
	  if (r.m_provider) {
	    assert(!lc->m_instIn);
	    lc->m_instIn = &m_launchInstances[pi->m_instance];
	    lc->m_nameIn = pi->m_name.c_str();
	    lc->m_paramsIn.add((*ci).m_parameters, pi->m_parameters);
	    lc->m_launchIn = &lc->m_instIn->m_container->launcher();
	  } else {
	    assert(!lc->m_instOut);
	    lc->m_instOut = &m_launchInstances[pi->m_instance];
	    lc->m_nameOut = pi->m_name.c_str();
	    lc->m_paramsOut.add((*ci).m_parameters, pi->m_parameters);
	    lc->m_launchOut = &lc->m_instOut->m_container->launcher();
	  }
	}
	for (OU::Assembly::ExternalsIter ei = (*ci).m_externals.begin();
	     ei != (*ci).m_externals.end(); ei++) {
	  assert(!lc->m_instIn || !lc->m_instOut);
	  if (ei->m_url.length())
	    lc->m_url = ei->m_url.c_str();
	  if (lc->m_instIn) {
	    lc->m_nameOut = ei->m_name.c_str();
	    lc->m_paramsOut = ei->m_parameters;
	  } else {
	    lc->m_nameIn = ei->m_name.c_str();
	    lc->m_paramsIn = ei->m_parameters;
	  }
	}
      }
    }
    void ApplicationI::
    initInstances() {
      OC::Launcher::Instance *i = &m_launchInstances[0];
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	i->m_container = m_containers[m_instances[n].m_container];
	i->m_containerApp = m_containerApps[m_instances[n].m_container];
	i->m_name = m_assembly.instance(n).name();
	i->m_impl = m_instances[n].m_impl;
	OU::Assembly::Instance &ui = m_assembly.instance(n).m_utilInstance;
	i->m_hasMaster = ui.m_hasMaster;
	if (ui.m_hasSlave)
	  i->m_slave = &m_launchInstances[ui.m_slave];
	if ((unsigned)m_assembly.m_doneInstance == n)
	  i->m_doneInstance = true;
      }
    }
    void ApplicationI::
    initExternals( const PValue * params ) {
      // Check that params that reference externals are valid.
      checkExternalParams("file", params);
      checkExternalParams("device", params);
      checkExternalParams("url", params);
    }
    bool
    ApplicationI::foundContainer(OCPI::Container::Container &c) {
      m_curMap |= 1 << c.ordinal();
      m_curContainers++;
      return false;
    }

    void ApplicationI::
    initialize() {
      m_nInstances = m_assembly.nInstances();
      ocpiDebug("Mapped %zu instances to %d containers", m_nInstances, m_nContainers);

      m_containers = new OC::Container *[m_nContainers];
      m_containerApps = new OC::Application *[m_nContainers];
      for (unsigned n = 0; n < m_nContainers; n++) {
	m_containers[n] = &OC::Container::nthContainer(m_usedContainers[n]);
	m_containerApps[n] = static_cast<OC::Application*>(m_containers[n]->createApplication());
	m_containerApps[n]->setApplication(&m_apiApplication);
      }
      initInstances();
      initConnections();
      typedef std::set<OC::Launcher *> Launchers;
      typedef Launchers::iterator LaunchersIter;
      Launchers launchers;
      for (unsigned n = 0; n < m_nContainers; n++)
	if (launchers.insert(&m_containers[n]->launcher()).second)
	  m_containers[n]->launcher().launch(m_launchInstances, m_launchConnections);
      // Now we have interned our launchers
      bool more;
      do {
	more = false;
	for (LaunchersIter li = launchers.begin(); li != launchers.end(); li++)
	  if (//(*li)->notDone() &&
	      (*li)->work(m_launchInstances, m_launchConnections))
	    more = true;
      } while (more);
      if (m_assembly.m_doneInstance != -1)
	m_doneWorker = m_launchInstances[m_assembly.m_doneInstance].m_worker;
      OC::Launcher::Connection *c = &m_launchConnections[0];
      for (unsigned n = 0; n < m_launchConnections.size(); n++, c++)
	if (!c->m_url && (!c->m_instIn || !c->m_instOut))
	  m_externals.
	    insert(ExternalPair(c->m_instIn ? c->m_nameOut : c->m_nameIn,
				External(*(c->m_input ? c->m_input : c->m_output),
					 c->m_input ? c->m_paramsOut : c->m_paramsIn)));
      m_launched = true;
      if (m_assembly.m_doneInstance != -1)
	m_doneWorker = m_launchInstances[m_assembly.m_doneInstance].m_worker;
    }
    void ApplicationI::start() {

      ocpiDebug("Using %d containers to support the application", m_nContainers );
      ocpiDebug("Starting master workers that are not slaves.");
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->startMasterSlave(true, false); // start masters that are not slaves
      ocpiDebug("Starting master workers that are also slaves.");
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->startMasterSlave(true, true);  // start masters that are slaves
      ocpiDebug("Starting workers that are not masters.");
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->startMasterSlave(false, false); // start non-masters
    };
    void ApplicationI::stop() {
      ocpiDebug("Stopping master workers that are not slaves.");
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->stop(true, false); // start masters that are not slaves
      ocpiDebug("Stopping master workers that are also slaves.");
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->stop(true, true);  // start masters that are slaves
      ocpiDebug("Stopping workers that are not masters.");
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->stop(false, false); // start non-masters
    }
    bool ApplicationI::wait(OS::Timer *timer) {
      if (m_doneWorker) {
	ocpiInfo("Waiting for \"done\" worker, \"%s\", to finish",
		 m_doneWorker->name().c_str());
	return m_doneWorker->wait(timer);
      }
      do {
	bool done = true;
	for (unsigned n = 0; n < m_nContainers; n++)
	  if (!m_containerApps[n]->isDone())
	    done = false;
	if (done)
	  return false;
	OS::sleep(10);
      } while (!timer || !timer->expired());
      return true;
    }

    // Stuff to do after "done" (or perhaps timeout)
    void ApplicationI::finish() {
      Property *p = m_properties;
      for (unsigned n = 0; n < m_nProperties; n++, p++)
	if (p->m_dumpFile) {
	  std::string name, value;
	  m_launchInstances[p->m_instance].m_worker->
	    getProperty(p->m_property, name, value, NULL, m_hex);
	  value += '\n';
	  const char *err = OU::string2File(value, p->m_dumpFile);
	  if (err)
	    throw OU::Error("Error writing '%s' property to file: %s", name.c_str(), err);
	}
    }

    ExternalPort &ApplicationI::getPort(const char *name, const OA::PValue *params) {
      if (!m_launched)
	throw OU::Error("GetPort cannot be called until the application is initialized.");
      Externals::iterator ei = m_externals.find(name);
      if (ei == m_externals.end())
	throw OU::Error("Unknown external port name for application: \"%s\"", name);
      External &ext = ei->second;
      if (ext.m_external) {
	if (params)
	  ocpiInfo("Parameters ignored when getPort called for same port more than once");
      } else {
	OU::PValueList pvs(ext.m_params, params);
	ext.m_external = &ext.m_port.connectExternal(name, pvs);
      }
      return *ext.m_external;
    }


    ExternalPort &ApplicationI::getPort(unsigned index, std::string & name) {
      if (!m_launched)
	throw OU::Error("GetPort cannot be called until the application is initialized.");
      if ( index >= m_externals.size() )
	throw OU::Error("GetPort(int) Index out of range.");
      std::map<const char*, External, OCPI::Util::ConstCharComp>::iterator ei;
      unsigned c=0;
      for ( ei=m_externals.begin(); ei!=m_externals.end(); ei++, c++ ){
	if ( c == index )
	  break;
      }
      if (ei == m_externals.end())
	throw OU::Error("Unknown external port at index: \"%d\"", index);
      External &ext = ei->second;
      if (ext.m_external) {

      } else {
	OU::PValueList pvs(ext.m_params, NULL);
	ext.m_external = &ext.m_port.connectExternal(ei->first, pvs);
      }
      name = ei->first;
      return *ext.m_external;
    }

    size_t ApplicationI::getPortCount() {
      return m_externals.size();
    }


    Worker &ApplicationI::getPropertyWorker(const char *name) {
      Property *p = m_properties;
      for (unsigned n = 0; n < m_nProperties; n++, p++)
	if (!strcasecmp(name, p->m_name.c_str()))
	  return *m_launchInstances[p->m_instance].m_worker;
      throw OU::Error("Unknown application property: %s", name);
    }

    static inline const char *maybePeriod(const char *name) {
      const char *cp = strchr(name, '.');
      return cp ? cp + 1 : name;
    }
    // FIXME:  consolidate the constructors (others are in OcpiProperty.cxx) (have in internal class for init)
    // FIXME:  avoid the double lookup since the first one gets us the ordinal
    Property::Property(Application &app, const char *aname)
      : m_worker(app.getPropertyWorker(aname)),
	m_readVaddr(0), m_writeVaddr(0),
	m_info(m_worker.setupProperty(maybePeriod(aname), m_writeVaddr, m_readVaddr)),
	m_ordinal(m_info.m_ordinal),
	m_readSync(false), m_writeSync(false)
    {
      m_readSync = m_info.m_readSync;
      m_writeSync = m_info.m_writeSync;
    }

    bool ApplicationI::getProperty(unsigned ordinal, std::string &name, std::string &value,
				   bool hex, bool *parp) {
      if (ordinal >= m_nProperties)
	return false;
      Property &p = m_properties[ordinal];
      name = p.m_name;
      OC::Worker &w = *m_launchInstances[p.m_instance].m_worker;
      OU::Property &wp = w.property(p.m_property);
      if (wp.m_isReadable) {
	std::string dummy;
	m_launchInstances[p.m_instance].m_worker->
	  getProperty(p.m_property, dummy, value, NULL, hex);
      } else
	value = "<unreadable>";
      if (parp)
	*parp = wp.m_isParameter;
      return true;
    }

    ApplicationI::Property &ApplicationI::
    findProperty(const char * worker_inst_name, const char * prop_name) {
      std::string nm;
      if (worker_inst_name) {
	nm = worker_inst_name;
	nm += ".";
	nm += prop_name;
      } else {
	nm = prop_name;
	size_t eq = nm.find('=');
	if (eq != nm.npos)
	  nm[eq] = '.';
      }
      Property *p = m_properties;
      for (unsigned n = 0; n < m_nProperties; n++, p++)
	if (!strcasecmp(nm.c_str(), p->m_name.c_str()))
	  return *p;
      throw OU::Error("Unknown application property: %s", nm.c_str());
    }

    void ApplicationI::
    getProperty(const char * worker_inst_name, const char * prop_name, std::string &value,
		bool hex) {
      Property &p = findProperty(worker_inst_name, prop_name);
      std::string dummy;
      m_launchInstances[p.m_instance].m_worker->
	getProperty(p.m_property, dummy, value, NULL, hex);	 
    }

    void ApplicationI::
    setProperty(const char * worker_inst_name, const char * prop_name, const char *value) {
      Property &p = findProperty(worker_inst_name, prop_name);
      m_launchInstances[p.m_instance].m_worker->setProperty(prop_name, value);
    }

    void ApplicationI::
    dumpDeployment(const char *appFile, const std::string &file) {
      FILE *f = file == "-" ? stdout : fopen(file.c_str(), "w");
      if (!f)
	throw OU::Error("Can't open file \"%s\" for deployment output", file.c_str());
      fprintf(f, "<deployment application='%s'>\n", appFile);
      Instance *i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	const OL::Implementation &impl = *i->m_impl;
	OC::Container &c = OC::Container::nthContainer(m_usedContainers[i->m_container]);
	fprintf(f,
		"  <instance name='%s' spec='%s' worker='%s' model='%s' container='%s'\n"
		"            artifact='%s'",
		m_assembly.instance(n).name().c_str(), m_assembly.instance(n).specName().c_str(),
		impl.m_metadataImpl.name().c_str(), c.m_model.c_str(), c.name().c_str(),
		impl.m_artifact.name().c_str());
	if (impl.m_staticInstance)
	  fprintf(f, " instance='%s'", ezxml_cattr(impl.m_staticInstance, "name"));
	fprintf(f, "/>\n");
      }
      fprintf(f, "</deployment>\n");
      if (f != stdout && fclose(f))
	throw OU::Error("Can't close output file \"%s\".  No space?", file.c_str());
    }

    ApplicationI::Instance::Instance() :
      m_impl(NULL), m_container(0), m_feasibleContainers(NULL), m_nCandidates(0) {
    }
    ApplicationI::Instance::~Instance() {
      delete [] m_feasibleContainers;
    }

  }
  namespace API {
    OCPI_EMIT_REGISTER_FULL_VAR( "Get Property", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, pegp ); 
    OCPI_EMIT_REGISTER_FULL_VAR( "Set Property", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, pesp ); 

    Application::
    Application(const char *file, const PValue *params)
      : m_application(*new ApplicationI(*this, file, params)) {
    }
    Application::
    Application(const std::string &string, const PValue *params)
      : m_application(*new ApplicationI(*this, string, params)) {
    }
    Application::
    Application(ApplicationI &i)
      : m_application(i) {
    }
    ApplicationX::
    ApplicationX(ezxml_t xml, const char *name,  const PValue *params)
      : Application(*new ApplicationI(*this, xml, name, params)) {
    }
    Application::
    Application(Application & app,  const PValue *params)
      : m_application(*new ApplicationI(*this, app.m_application.assembly(), params)) {
    }

    Application::
    ~Application() { delete &m_application; }

    void Application::
    initialize() { m_application.initialize(); }

    void Application::
    start() { m_application.start(); }

    void Application::
    stop() { m_application.stop(); }

    bool Application::
    wait( unsigned timeout_us ) {
      OS::Timer *timer = NULL;
      if (timeout_us) 
	timer = new OS::Timer((uint32_t)(timeout_us/1000000ul),
			      (uint32_t)((timeout_us%1000000) * 1000ull));
      bool r = m_application.wait(timer);
      delete timer;
      return r;
    }

    void Application::
    finish() {
      m_application.finish();
    }

    const std::string &Application::name() const {
      return m_application.name();
    }
    ExternalPort &Application::
    getPort(const char *name, const OA::PValue *params) {
      return m_application.getPort(name, params);
    }
    ExternalPort &Application::
    getPort(unsigned index, std::string& name ) {
      return m_application.getPort(index, name );
    }
    size_t Application::
    getPortCount() {
      return m_application.getPortCount();
    }

    bool Application::
    getProperty(unsigned ordinal, std::string &name, std::string &value, bool hex, bool *parp) {
      return m_application.getProperty(ordinal, name, value, hex, parp);
    }

    void Application::
    getProperty(const char* w, const char* p, std::string &value, bool hex) {
      OCPI_EMIT_STATE_NR( pegp, 1 );
      m_application.getProperty(w, p, value, hex);
      OCPI_EMIT_STATE_NR( pegp, 0 );

    }

    void Application::
    getProperty(const char* w, std::string &value, bool hex) {
      OCPI_EMIT_STATE_NR( pegp, 1 );
      m_application.getProperty(NULL, w, value, hex);
      OCPI_EMIT_STATE_NR( pegp, 0 );

    }

    void Application::
    setProperty(const char* w, const char* p, const char *value) {
      OCPI_EMIT_STATE_NR( pesp, 1 );
      m_application.setProperty(w, p, value);
      OCPI_EMIT_STATE_NR( pesp, 0 );

    }
    void Application::
    setProperty(const char* p, const char *value) {
      OCPI_EMIT_STATE_NR( pesp, 1 );
      m_application.setProperty(NULL, p, value);
      OCPI_EMIT_STATE_NR( pesp, 0 );

    }
    Worker &Application::
    getPropertyWorker(const char *name) { return m_application.getPropertyWorker(name); }

    void Application::
    dumpDeployment(const char *appFile, const std::string &file) {
      return m_application.dumpDeployment(appFile, file); }
  }
}
