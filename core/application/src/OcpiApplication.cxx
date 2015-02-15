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
namespace OT = DataTransport;
namespace OCPI {
  namespace API {
    ApplicationI::ApplicationI(Application &app, const char *file, const PValue *params)
      : m_assembly(*new OL::Assembly(file, params)), m_apiApplication(app) {
      init(params);
    }
    ApplicationI::ApplicationI(Application &app, const std::string &string, const PValue *params)
      : m_assembly(*new OL::Assembly(string, params)), m_apiApplication(app)  {
      init(params);
    }
    ApplicationI::ApplicationI(Application &app, OL::Assembly &assy, const PValue *params)
      : m_assembly(assy), m_apiApplication(app) {
      m_assembly++;
      init(params);
    }
    ApplicationI::~ApplicationI() {
      m_assembly--;
      delete [] m_instances;
      delete [] m_bookings;
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
    addContainer(unsigned container) {
      ocpiAssert(!(m_allMap & (1 << container)));
      return getUsedContainer(container);
    }
    unsigned ApplicationI::
    getUsedContainer(unsigned container) {
      if (m_allMap & (1 << container))
	return m_global2used[container];
      m_usedContainers[m_nContainers] = container;
      m_allMap |= 1 << container;
      m_global2used[container] = m_nContainers;
      return m_nContainers++;
    }
    /*
     * We made choices during the feasibility analysis, but here we want to add some policy.
     * The default allocation will bias toward collocation, so this is basically to 
     * spread things out.
     * Since exclusive/bitstream allocations are not really adjustable, we just deal with the
     * others.
     * we haven't remembered ALL deployments, just the "best".
     * we have preferred internally connected impls by scoring them up, which has inherently
     * consolidated then.
     * (preferred collocation)
    */
    void ApplicationI::
    policyMap(Instance *i, CMap &bestMap)
    {
      // bestMap is bitmap of best available containers that the implementation can be mapped to
      // allMap is the bitmap of all suitable containers for the implementation
      switch ( m_cMapPolicy ) {

      case MaxProcessors:
	// Limit use of processors to m_processors
	// If We have hit the limit, try to re-use.  If we can't, fall through to round robin
	if (m_nContainers >= m_processors)
	  for (unsigned n = 0; n < m_nContainers; n++) {
	    if ( m_currConn >= m_nContainers ) 
	      m_currConn = 0;
	    if (bestMap & (1 << m_usedContainers[m_currConn++])) {
	      i->m_usedContainer = m_currConn - 1;
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
	    i->m_usedContainer = addContainer(n);
	    ocpiDebug("instance %p used new container. best 0x%x curr %u cont %u",
		      i, bestMap, m_currConn, n);
	    return; // We added a new one - and used it
	  }
	// We have to use one we have since only those are feasible
	do {
	  if (++m_currConn >= m_nContainers)
	    m_currConn = 0;
	} while (!(bestMap & (1 << m_usedContainers[m_currConn])));
	i->m_usedContainer = m_currConn;
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
	    i->m_usedContainer = n;
	    return;
	  }
	// Add one
	unsigned n;
	for (n = 0; n < OC::Manager::s_nContainers; n++)
	  if (bestMap & (1 << n))
	    break;
	i->m_usedContainer = addContainer(n);
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
	    other->m_instance < instNum) { // if the other instance has been processed
	  const OL::Implementation &otherImpl =
	    *m_instances[other->m_instance].m_deployment.m_impls[0];
	  // then check for prewired compatibility
	  if (m_assembly.badConnection(*c.impl, otherImpl, *ap, nn)) {
	    ocpiInfo("%s due to connectivity conflict", reject.c_str());
	    ocpiInfo("Other is instance \"%s\" for spec \"%s\" implementation \"%s%s%s\" "
		     "from artifact \"%s\".",
		     m_assembly.instance(other->m_instance).name().c_str(),
		     m_assembly.instance(other->m_instance).specName().c_str(),
		     otherImpl.m_metadataImpl.name().c_str(),
		     otherImpl.m_staticInstance ? "/" : "",
		     otherImpl.m_staticInstance ?
		     ezxml_cattr(otherImpl.m_staticInstance, "name") : "",
		     otherImpl.m_artifact.name().c_str());
	    return false;
	  }
	}
      }
      // Check for master/slave correctness
      // Note that we know that the impl for a master indicates a slave since this
      // can be checked by the library layer.
      OU::Worker *mImpl = NULL, *sImpl = NULL;
      bool isMaster;
      if (ui.m_hasSlave && ui.m_slave < instNum) {
	mImpl = &c.impl->m_metadataImpl;
	sImpl = &m_instances[ui.m_slave].m_deployment.m_impl->m_metadataImpl;
	isMaster = true;
      } else if (ui.m_hasMaster && ui.m_master < instNum) {
	sImpl = &c.impl->m_metadataImpl;
	mImpl = &m_instances[ui.m_master].m_deployment.m_impl->m_metadataImpl;
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
	  i->m_crew.m_propValues.resize(nPropValues);
	  i->m_crew.m_propOrdinals.resize(nPropValues);
	  OU::Value *pv = &i->m_crew.m_propValues[0];
	  unsigned *pn = &i->m_crew.m_propOrdinals[0];
	  // Note that for scaled instances we assume the impls are compatible as far as
	  // properties go.  FIXME:  WE MUST CHECK COMPILED VALUES WHEN COMPARING IMPLES
	  prepareInstanceProperties(n, *i->m_bestDeployment.m_impls[0], pn, pv);
	  nPropValues = pn - &i->m_crew.m_propOrdinals[0];
	  i->m_crew.m_propValues.resize(nPropValues);
	  i->m_crew.m_propOrdinals.resize(nPropValues);
	}
      }
      // For all instances in the assembly, create the app-level property array
      m_nProperties = m_assembly.m_mappedProperties.size();
      i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++)
	m_nProperties += i->m_bestDeployment.m_impls[0]->m_metadataImpl.m_nProperties;
      // Over allocate: mapped ones plus all the instances' ones
      Property *p = m_properties = new Property[m_nProperties];
      OU::Assembly::MappedProperty *mp = &m_assembly.m_mappedProperties[0];
      for (size_t n = m_assembly.m_mappedProperties.size(); n; n--, mp++, p++) {
	p->m_property =
	  m_instances[mp->m_instance].m_bestDeployment.m_impls[0]->m_metadataImpl.
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
	OU::Property *meta = i->m_bestDeployment.m_impls[0]->m_metadataImpl.properties(nProps);
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
    }

    void ApplicationI::
    finalizeExternals() {
      // External ports that are not connected explicitly to anything need to be associated
      // with the base container in this process, so we make sure we are using it.
      for (OU::Assembly::ConnectionsIter ci = m_assembly.m_connections.begin();
	   ci != m_assembly.m_connections.end(); ci++)
	if (ci->m_externals.size() && ci->m_externals.front().m_url.empty())
	  getUsedContainer(OC::Container::baseContainer().ordinal());
    }

    void ApplicationI::
    dumpDeployment(unsigned score) {
      ocpiDebug("Deployment with score %u is:", score);
      Instance *i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	const OL::Implementation &li = *i->m_deployment.m_impls[0];
	if (i->m_deployment.m_scale == 1)
	  ocpiDebug(" Instance %2u: Container: %u Instance %s%s%s in %s", 
		    n, i->m_deployment.m_containers[0],
		    li.m_metadataImpl.name().c_str(),
		    li.m_staticInstance ? "/" : "",
		    li.m_staticInstance ? ezxml_cattr(li.m_staticInstance, "name") : "",
		    li.m_artifact.name().c_str());
	else {
	  ocpiDebug(" Instance %2u: Scale factor: %zu", n, i->m_deployment.m_scale);
	  for (unsigned j = 0; j < i->m_deployment.m_scale; j++) {
	    const OL::Implementation &li = *i->m_deployment.m_impls[j];
	    ocpiDebug("   Member %2u: Container: %u Instance %s%s%s in %s",
		      j, i->m_deployment.m_containers[j], li.m_metadataImpl.name().c_str(),
		      li.m_staticInstance ? "/" : "",
		      li.m_staticInstance ? ezxml_cattr(li.m_staticInstance, "name") : "",
		      li.m_artifact.name().c_str());
	  }
	}
      }
    }

    // After deciding on a possible instance deployment, record it and recurse for next one.
    // We record the implementation (possibly an array of them in the scaled case).
    // We record the container(s), and the feasible container map too for the unscaled case
    void ApplicationI::
    deployInstance(unsigned instNum, unsigned score, size_t scale,
		   unsigned *containers, const OL::Implementation **impls, CMap feasible) {
      m_instances[instNum].m_deployment.set(scale, containers, impls, feasible);
      ocpiDebug("doInstance ok");
      if (instNum < m_nInstances-1) {
	instNum++;
	if (scale == 1 && (*impls)->m_staticInstance) {
	  // FIXME: We don't deal with static instances on scaled instances yet
	  Booking
	    &b = m_bookings[*containers],
	    save = b;
	  b.m_artifact = &(*impls)->m_artifact;
	  b.m_usedImpls |= 1 << (*impls)->m_ordinal;
	  doInstance(instNum, score);
	  b = save;
	} else
	  doInstance(instNum, score);
      } else {
	dumpDeployment(score);
	if (score > m_bestScore) {
	  Instance *i = m_instances;
	  for (unsigned n = 0; n < m_nInstances; n++, i++)
	    i->m_bestDeployment = i->m_deployment;
	  m_bestScore = score;
	  ocpiDebug("Setting BEST");
	}
      }
    }

    void ApplicationI::
    doScaledInstance(unsigned instNum, unsigned score) {
      Instance *i = m_instances + instNum;
      OL::Assembly::Instance &li = m_assembly.instance(instNum);
      const OU::Assembly::Instance &ui = li.m_utilInstance;
      for (Instance::ScalableCandidatesIter sci = i->m_scalableCandidates.begin();
	   sci != i->m_scalableCandidates.end(); sci++) {
	CMap map = 0;
	for (Instance::CandidatesIter ci = sci->second.begin(); ci != sci->second.end(); ci++)
	  map |= i->m_feasibleContainers[*ci];
	size_t nFeasible = 0, nCollocated, nUsed, scale;
	for (unsigned cont = 0; cont < OC::Manager::s_nContainers; cont++)
	  if (map & (1 << cont))
	    nFeasible++;
	const char *err =
	  ui.m_collocation.apply(li.m_scale, nFeasible, nCollocated, nUsed, scale);
	if (err) {
	  ocpiInfo("Scalable implementation %s rejected due to collocation constraints: %s",
		   li.m_candidates[sci->second.front()].impl->m_metadataImpl.name().c_str(),
		   err);
	  continue;
	}
	if (scale != li.m_scale) {
	  ocpiInfo("Scaling of instance %s changed from %zu to %zu due to constraints",
		   ui.m_name.c_str(), li.m_scale, scale);
	  li.m_scale = scale;
	}
	unsigned *containers = new unsigned[scale];
	const OL::Implementation **impls = new const OL::Implementation*[scale];
	unsigned nMember = 0;
	for (Instance::CandidatesIter ci = sci->second.begin(); ci != sci->second.end(); ci++) {
	  CMap map = i->m_feasibleContainers[*ci];
	  for (unsigned cont = 0; cont < OC::Manager::s_nContainers; cont++)
	    if (map & (1 << cont))
	      for (unsigned n = 0; n < nCollocated; n++) {
		containers[nMember] = cont;
		impls[nMember] = li.m_candidates[*ci].impl;
		if (++nMember == scale)
		  goto out;
	      }
	}
      out:
	deployInstance(instNum, score + li.m_candidates[sci->second.front()].score, scale,
		       containers, impls, map); // the map isn't really relevant yet...
      }
    }

    void ApplicationI::
    doInstance(unsigned instNum, unsigned score) {
      OL::Assembly::Instance &li = m_assembly.instance(instNum);
      if (li.m_scale > 1)
	doScaledInstance(instNum, score);
      else {
	Instance &i = m_instances[instNum];
	for (unsigned m = 0; m < i.m_nCandidates; m++) {
	  OL::Candidate &c = li.m_candidates[m];	  
	  ocpiDebug("doInstance %u %u %u", instNum, score, m);
	  if (connectionsOk(c, instNum)) {
	    ocpiDebug("doInstance connections ok");
	    for (unsigned cont = 0; cont < OC::Manager::s_nContainers; cont++) {
	      ocpiDebug("doInstance container: cont %u feasible 0x%x", cont,
			i.m_feasibleContainers[m]);
	      if (i.m_feasibleContainers[m] & (1 << cont) &&
		  bookingOk(m_bookings[cont], c, instNum)) {
		deployInstance(instNum, score + c.score, 1, &cont, &c.impl,
			       i.m_feasibleContainers[m]);
		if (!c.impl->m_staticInstance)
		  break;
	      }
	    }
	  }
	}
      }
    }

    void ApplicationI::Instance::
    collectCandidate(OL::Candidate &c, unsigned n) {
      OU::Worker &w = c.impl->m_metadataImpl;
      std::string qname(w.package());
      qname += ".";
      qname += w.name();
      ScalableCandidatesIter sci = m_scalableCandidates.find(qname);
      if (sci == m_scalableCandidates.end())
	sci = m_scalableCandidates.insert(ScalablePair(qname, Candidates())).first;
      sci->second.push_back(n);
    }

    void ApplicationI::
    init(const PValue * params) {
      // In order from class definition except for instance-related
      m_bookings = new Booking[OC::Manager::s_nContainers];
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
      //      m_workers = NULL;
      m_doneInstance = NULL;
      //      m_externalPorts = NULL;
      //      m_externalNames = NULL;
      m_cMapPolicy = RoundRobin;
      m_processors = 0;
      m_currConn = OC::Manager::s_nContainers - 1;
      m_launched = false;

      // Set the instance map policy
      setPolicy(params);

      bool verbose = false;
      OU::findBool(params, "verbose", verbose);
      OU::findBool(params, "hex", m_hex);
      // Initializations for externals may add instances to the assembly someday...
      initExternals(params);
      // Now that we have added any extra instances for external connections, do
      // instance-related initializations
      m_nInstances = m_assembly.nInstances();
      m_instances = new Instance[m_nInstances];
      // Check that params that reference instances are valid.
      const char *err;
      if ((err = m_assembly.checkInstanceParams("container", params, false)) ||
	  (err = m_assembly.checkInstanceParams("scale", params, false)))
	throw OU::Error("%s", err);
      // First pass - make sure there are some containers to support some candidate
      // and remember which containers can support which candidates
      Instance *i = m_instances;
      for (size_t n = 0; n < m_nInstances; n++, i++) {
	OL::Candidates &cs = m_assembly.instance(n).m_candidates;
	const OU::Assembly::Instance &ai = m_assembly.utilInstance(n);
	i->m_nCandidates = cs.size();
	i->m_feasibleContainers = new CMap[cs.size()];
	const char *container;
	if (!OU::findAssign(params, "container", ai.m_name.c_str(), container))
	  container = ezxml_cattr(ai.xml(), "container");
	CMap sum = 0;
	for (unsigned m = 0; m < i->m_nCandidates; m++) {
	  m_curMap = 0;        // to accumulate containers suitable for this candidate
	  m_curContainers = 0; // to count suitable containers for this candidate
	  (void)OC::Manager::findContainers(*this, cs[m].impl->m_metadataImpl, container);
	  i->m_feasibleContainers[m] = m_curMap;
	  sum |= m_curMap;
	  if (m_curMap && m_assembly.instance(n).m_scale > 1)
	    i->collectCandidate(cs[m], m);
	}
	if (!sum) {
	  if (verbose) {
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
      // Up to now we have just been "planning" and not doing things.
      // Now invoke the policy method to map the dynamic instances to containers
      // First we do a pass that will only map the dynamic unscaled implementations
      i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++)
	if (i->m_bestDeployment.m_scale <= 1 && !i->m_bestDeployment.m_impl->m_staticInstance)
	  policyMap(i, i->m_bestDeployment.m_feasible);
      // Now add the containers for the static instances and the scaled instances
      i = m_instances;
      if (verbose)
	fprintf(stderr, "Actual deployment is:\n");
      for (unsigned n = 0; n < m_nInstances; n++, i++)
	if (i->m_bestDeployment.m_scale > 1) {
	  i->m_usedContainers = new unsigned[i->m_bestDeployment.m_scale];
	  for (unsigned s = 0; s < i->m_bestDeployment.m_scale; s++)
	    i->m_usedContainers[s] = getUsedContainer(i->m_bestDeployment.m_containers[s]);
	  if (verbose) {
	    fprintf(stderr,
		    "  Instance %2u %s (spec %s) on %s containers:\n", 
		    n, m_assembly.instance(n).name().c_str(),
		    m_assembly.instance(n).specName().c_str(),
		    OC::Container::nthContainer(i->m_bestDeployment.m_containers[0]).
		    m_model.c_str());
	    const OL::Implementation **impl = i->m_bestDeployment.m_impls;
	    for (unsigned s = 0; s < i->m_bestDeployment.m_scale; s++, impl++) {
	      OC::Container &c =
		OC::Container::nthContainer(i->m_bestDeployment.m_containers[s]);
	      fprintf(stderr, "    Member %3u: container %2u: %s using impl %s%s%s in %s\n",
		      s, i->m_bestDeployment.m_containers[s], c.name().c_str(),
		      (**impl).m_metadataImpl.name().c_str(),
		      (**impl).m_staticInstance ? "/" : "",
		      (**impl).m_staticInstance ?
		      ezxml_cattr((**impl).m_staticInstance, "name") : "",
		      (**impl).m_artifact.name().c_str());
	    }
	  }
	} else {
	  i->m_usedContainers = &i->m_usedContainer;
	  const OL::Implementation &impl = *i->m_bestDeployment.m_impls[0];
	  if (impl.m_staticInstance)
	    i->m_usedContainer = getUsedContainer(i->m_bestDeployment.m_container);
	  OC::Container &c = OC::Container::nthContainer(m_usedContainers[i->m_usedContainer]);
	  if (verbose)
	    fprintf(stderr,
		    "  Instance %2u %s (spec %s) on %s container %u: %s, using %s%s%s in %s\n", 
		    n, m_assembly.instance(n).name().c_str(),
		    m_assembly.instance(n).specName().c_str(),
		    c.m_model.c_str(), i->m_bestDeployment.m_container, c.name().c_str(),
		    impl.m_metadataImpl.name().c_str(),
		    impl.m_staticInstance ? "/" : "",
		    impl.m_staticInstance ? ezxml_cattr(impl.m_staticInstance, "name") : "",
		    impl.m_artifact.name().c_str());
	}
      // All the implementation selection is done, so now do the final check of properties
      // since properties can be implementation specific.  This includes parsing values.
      finalizeProperties(params);
      finalizeExternals();
    }

    void ApplicationI::
    setLaunchPort(OC::Launcher::Port &p, const OU::Port *mp, const OU::PValue *connParams,
		  const std::string &name, const OU::PValue *portParams,
		  const OC::Launcher::Member *member, const OU::Assembly::External *ep,
		  size_t scale, size_t index) {
      p.m_scale = scale;
      p.m_index = index;
      p.m_member = member;
      p.m_metaPort = mp;
      if (member) {
	//	p.m_container = p.m_member->m_container;
	p.m_name = name.c_str();
	p.m_params.add(connParams, portParams);
	p.m_container = member->m_container;
      } else if (ep) {
	p.m_params = ep->m_parameters;
	if (ep->m_url.length())
	  p.m_url = ep->m_url.c_str();
	else {
	  p.m_name = ep->m_name.c_str();
	  p.m_container = &OC::Container::baseContainer();
	  p.m_containerApp = 
	    m_containerApps[getUsedContainer(p.m_container->ordinal())];
	  p.m_launcher = &OC::Container::baseContainer().launcher();
	}
      }
      if (p.m_container)
	p.m_launcher = &p.m_container->launcher();
    }

    // Initialize the launcher database of connections from the OU::Assembly connections
    void ApplicationI::
    initLaunchConnections() {
      // For each instance connection we need to compute how many members each side will
      // connect to on the other side.  I.e. at each member port, how many on the other
      // side will it be talking to.  In most cases you talk to everyone on the other side.
      // Basically we need a function which returns which on the other side we will talk
      // to.  We'll use a map.
      // Pass 1: figure out how many member connections we will have, and 
      // negotiate the buffer size.
      unsigned nMemberConnections = 0;
      for (OU::Assembly::ConnectionsIter ci = m_assembly.m_connections.begin();
	   ci != m_assembly.m_connections.end(); ci++) {
	Instance *iIn = NULL, *iOut = NULL;
	for (OU::Assembly::Connection::PortsIter pi = ci->m_ports.begin();
	     pi != ci->m_ports.end(); pi++) {
	  OU::Assembly::Role &r = pi->m_role;
	  assert(r.m_knownRole && !r.m_bidirectional);
	  (r.m_provider ? iIn : iOut) = &m_instances[pi->m_instance];
	}
#if 1
	nMemberConnections += (iIn ? iIn->m_crew.m_size : 1) * (iOut ? iOut->m_crew.m_size : 1);
      }
      // Pass 1a: count the connections required that are internal to an instance crew
      Instance *i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	const OU::Worker &firstImpl = i->m_bestDeployment.m_impls[0]->m_metadataImpl;
	unsigned nPorts;
	OU::Port *p = firstImpl.ports(nPorts);
	for (unsigned n = 0; n < nPorts; n++, p++)
	  if (p->m_isInternal) {
	    if (!p->m_isOptional || i->m_bestDeployment.m_scale > 1)
	      nMemberConnections += i->m_bestDeployment.m_scale * i->m_bestDeployment.m_scale;
	    p++, n++; // always skip one after an internal since that's the other half.
	  }
      }     
      // Pass 2: make the array and fill it in, also negotiate buffer sizes and transports
      m_launchConnections.resize(nMemberConnections);
      OC::Launcher::Connection *lc = &m_launchConnections[0];
      for (OU::Assembly::ConnectionsIter ci = m_assembly.m_connections.begin();
	   ci != m_assembly.m_connections.end(); ci++) {
	const OU::Assembly::Port *aIn = NULL, *aOut = NULL;
	Instance *iIn = NULL, *iOut = NULL;
	OU::Port *pIn = NULL, *pOut = NULL;
	size_t inScale = 1, outScale = 1;
	for (OU::Assembly::Connection::PortsIter pi = ci->m_ports.begin();
	     pi != ci->m_ports.end(); pi++) {
	  Instance *i = &m_instances[pi->m_instance];
	  OU::Port *p =
	    i->m_bestDeployment.m_impls[0]->m_metadataImpl.findMetaPort(pi->m_name.c_str());
	  assert(p);
	  if (pi->m_role.m_provider) {
	    aIn = &*pi;
	    iIn = i;
	    pIn = p;
	    inScale = i->m_crew.m_size;
#else
	for (OU::Assembly::ExternalsIter ei = (*ci).m_externals.begin();
	     ei != (*ci).m_externals.end(); ei++) {
	  assert(!lc->m_instIn || !lc->m_instOut);
	  if (ei->m_url.length())
	    lc->m_url = ei->m_url.c_str();
	  if (lc->m_instIn) {
	    lc->m_nameOut = ei->m_name.c_str();
	    lc->m_paramsOut = ei->m_parameters;
#endif
	  } else {
	    aOut = &*pi;
	    iOut = i;
	    pOut = p;
	    outScale = i->m_crew.m_size;
	  }	    
	}
	OU::Assembly::External *e = NULL;
	const OU::PValue *eParams = NULL;
	if (ci->m_externals.size()) {
	  e = &ci->m_externals.front();
	  eParams = e->m_parameters;
	  if (pIn)
	    pOut = pIn;
	  else
	    pIn = pOut;
	  m_externals.insert(ExternalPair(e->m_name.c_str(), External(*lc)));
	}
	// Resolve the buffer size for this connection, to apply to all member connections
	size_t bufferSize =
	  OU::Port::determineBufferSize(pIn, aIn ? aIn->m_parameters.list() : NULL,
					pOut, aOut ? aOut->m_parameters.list() : NULL,
					ci->m_parameters.list());
	const OU::PValue *connParams = ci->m_parameters;
	for (unsigned nIn = 0; nIn < inScale; nIn++) {
	  OC::Launcher::Member *mIn = aIn ? &m_launchMembers[iIn->m_firstMember + nIn] : NULL;
	  for (unsigned nOut = 0; nOut < outScale; nOut++, lc++) {
	    OC::Launcher::Member *mOut =
	      aOut ? &m_launchMembers[iOut->m_firstMember + nOut] : NULL;
	    lc->m_bufferSize = bufferSize;
	    setLaunchPort(lc->m_in, pIn, connParams, aIn->m_name, aIn->m_parameters,
			  mIn, e, inScale, nIn);
	    setLaunchPort(lc->m_out, pOut, connParams, aOut->m_name, aOut->m_parameters,
			  mOut, e, outScale, nOut);
	    // Now finalize the transport selection
	    // FIXME: cache results for same inputs
	    // Check for collocated ports
	    if (lc->m_in.m_container && lc->m_out.m_container &&
		lc->m_in.m_container != lc->m_out.m_container)
	      OC::BasicPort::
		determineTransport(lc->m_in.m_container->transports(),
				   lc->m_out.m_container->transports(),
				   aIn ? (const OU::PValue *)aIn->m_parameters : eParams,
				   aOut ? (const OU::PValue *)aOut->m_parameters : eParams,
				   ci->m_parameters, lc->m_transport);
	  }
	}
      }
      // Pass 2a: add the internal connections
      i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	const OU::Worker &firstImpl = i->m_bestDeployment.m_impls[0]->m_metadataImpl;
	size_t scale = i->m_bestDeployment.m_scale;
	unsigned nPorts;
	OU::Port *p = firstImpl.ports(nPorts);
	for (unsigned n = 0; n < nPorts; n++, p++)
	  if (p->m_isInternal) {
	    if (!p->m_isOptional || i->m_bestDeployment.m_scale > 1) {
	      // FIXME: any point in allowing buffer count override?
	      size_t bufferSize = OU::Port::determineBufferSize(p, NULL, p + 1, NULL, NULL);
	      for (unsigned nIn = 0; nIn < scale; nIn++) {
		OC::Launcher::Member *mIn = &m_launchMembers[i->m_firstMember + nIn];
		for (unsigned nOut = 0; nOut < scale; nOut++, lc++) {
		  OC::Launcher::Member *mOut = &m_launchMembers[i->m_firstMember + nOut];
		  lc->m_bufferSize = bufferSize;
		  setLaunchPort(lc->m_in, p, NULL, p->m_name, NULL, mIn, NULL, scale, nIn);
		  setLaunchPort(lc->m_out, p+1, NULL, (p+1)->m_name, NULL, mOut, NULL, scale, nOut);
		}	    
	      }
	    }
	    p++, n++; // always skip one after an internal since that's the other half.
	  }
      }
    }
    // Create the instance array for the launcher, which is flattened to have an instance
    // per member rather than an instance per app instance.
    void ApplicationI::
    initLaunchMembers() {
      Instance *i = m_instances;
      unsigned nMembers = 0;
      for (unsigned n = 0; n < m_nInstances; n++, nMembers += i->m_bestDeployment.m_scale, i++)
	i->m_firstMember = nMembers;
      m_launchMembers.resize(nMembers);
      i = m_instances;
      OC::Launcher::Member *li = &m_launchMembers[0];
      for (unsigned n = 0; n < m_nInstances; n++, i++)
	for (unsigned m = 0; m < i->m_bestDeployment.m_scale; m++, li++) {
	  li->m_containerApp = m_containerApps[i->m_usedContainers[m]];
	  li->m_container = m_containers[i->m_usedContainers[m]];
	  if (i->m_bestDeployment.m_scale == 1)
	    li->m_name = m_assembly.instance(n).name();
	  else
	    OU::format(li->m_name, "%s.%u", m_assembly.instance(n).name().c_str(), m);
	  li->m_impl = i->m_bestDeployment.m_impls[m];
	  OU::Assembly::Instance &ui = m_assembly.instance(n).m_utilInstance;
	  li->m_hasMaster = ui.m_hasMaster;
	  assert(!ui.m_hasMaster || i->m_bestDeployment.m_scale == 1);
	  if ((unsigned)m_assembly.m_doneInstance == n)
	    li->m_doneInstance = true;
	  assert(!ui.m_hasSlave || i->m_bestDeployment.m_scale == 1);
	  if (ui.m_hasSlave)
	    li->m_slave = &m_launchMembers[m_instances[ui.m_slave].m_firstMember];
	  li->m_member = m;
	  i->m_crew.m_size = i->m_bestDeployment.m_scale;
	  li->m_crew = &i->m_crew;
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
      initLaunchMembers();
      initLaunchConnections();
      typedef std::set<OC::Launcher *> Launchers;
      typedef Launchers::iterator LaunchersIter;
      Launchers launchers;
      for (unsigned n = 0; n < m_nContainers; n++)
	if (launchers.insert(&m_containers[n]->launcher()).second)
	  m_containers[n]->launcher().launch(m_launchMembers, m_launchConnections);
      // Now we have interned our launchers
      bool more;
      do {
	more = false;
	for (LaunchersIter li = launchers.begin(); li != launchers.end(); li++)
	  if (//(*li)->notDone() &&
	      (*li)->work(m_launchMembers, m_launchConnections))
	    more = true;
      } while (more);
      if (m_assembly.m_doneInstance != -1)
	m_doneInstance = &m_instances[m_assembly.m_doneInstance];
      //      m_launchMembers[m_instances[m_assembly.m_doneInstance].m_firstMember].m_worker;
#if 0
      for (unsigned n = 0; n < m_launchConnections.size(); n++) {
	OC::Launcher::Connection &c = m_launchConnections[n];
	if (!c.m_url && (!c.m_in.m_member || !c.m_out.m_member))
	  m_externals.insert(ExternalPair(c.m_in.m_member ? c.m_out.m_name : c.m_in.m_name,
					  External(c)));
      }
#endif
      m_launched = true;
    }
    void ApplicationI::start() {

      ocpiDebug("Using %d containers to support the application", m_nContainers );

      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->start();
    };
    void ApplicationI::stop() {
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->stop();
    }
    bool ApplicationI::wait(OS::Timer *timer) {
      if (m_doneInstance) {
	OC::Launcher::Member *m = &m_launchMembers[m_doneInstance->m_firstMember];
	if (m->m_crew->m_size > 1) {
	  ocpiInfo("Waiting for \"done\" worker, \"%s\" (%zu members), to finish",
		   m->m_worker->name().c_str(), m->m_crew->m_size);
	  do {
	    bool done = true;
	    m = &m_launchMembers[m_doneInstance->m_firstMember];
	    for (unsigned n = (unsigned)m->m_crew->m_size; n; n--, m++)
	      if (!m->m_worker->isDone()) {
		done = false;
		break;
	      }
	    if (done)
	      return false;
	    OS::sleep(1000);
	  } while (!timer || !timer->expired());
	} else {
	  ocpiInfo("Waiting for \"done\" worker, \"%s\", to finish",
		   m->m_worker->name().c_str());
	  return m->m_worker->wait(timer);
	}
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
	  m_launchMembers[m_instances[p->m_instance].m_firstMember].m_worker->
	    getProperty(p->m_property, name, value, NULL, m_hex);
	  value += '\n';
	  const char *err = OU::string2File(value, p->m_dumpFile);
	  if (err)
	    throw OU::Error("Error writing '%s' property to file: %s", name.c_str(), err);
	}
    }

    // Get an external port to use corresponding to an external port defined in the assembly.
    // This can happen after launch and can have new information for the connection
    // (e.g. transport) as well as for this particular external port (e.g. buffercount).
    // This means we need to parse the params on the fly here since they may be different
    // from what was in the assembly.
    ExternalPort &ApplicationI::
    getPort(const char *name, const OA::PValue *params) {
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
	if (params)
	  throw OU::Error("Parameters ignored for external port in assembly");
	ext.m_external = ext.m_connection.m_in.m_port;
      }
      return *static_cast<OC::ExternalPort*>(ext.m_external);
    }

    Worker &ApplicationI::getPropertyWorker(const char *name) {
      Property *p = m_properties;
      for (unsigned n = 0; n < m_nProperties; n++, p++)
	if (!strcasecmp(name, p->m_name.c_str()))
	  return *m_launchMembers[m_instances[p->m_instance].m_firstMember].m_worker;
      throw OU::Error("Unknown application property: %s", name);
    }

    static inline const char *maybePeriod(const char *name) {
      const char *cp = strchr(name, '.');
      return cp ? cp + 1 : name;
    }
    // FIXME:  consolidate the constructors (others are in OcpiProperty.cxx) (have in internal class for init)
    // FIXME:  avoid the double lookup since the first one gets us the ordinal
    Property::Property(Application &app, const char *aname)
<<<<<<< .merge_file_fFW4Hk
      : m_worker(app.getPropertyWorker(aname)), m_readVaddr(NULL), m_writeVaddr(NULL),
	m_info(m_worker.setupProperty(maybePeriod(aname), m_writeVaddr, m_readVaddr)),
	m_ordinal(m_info.m_ordinal), m_readSync(m_info.m_readSync),
	m_writeSync(m_info.m_writeSync) {
=======
      : m_worker(app.getPropertyWorker(aname)),
	m_readSync(false), m_writeSync(false), m_writeVaddr(0), m_readVaddr(0),
	m_info(m_worker.setupProperty(maybePeriod(aname), m_writeVaddr, m_readVaddr)),
	m_ordinal(m_info.m_ordinal)
    {
      m_readSync = m_info.m_readSync;
      m_writeSync = m_info.m_writeSync;
>>>>>>> .merge_file_zGcAg0
    }

    bool ApplicationI::getProperty(unsigned ordinal, std::string &name, std::string &value,
				   bool hex, bool *parp) {
      if (ordinal >= m_nProperties)
	return false;
      Property &p = m_properties[ordinal];
      name = p.m_name;
      OC::Worker &w = *m_launchMembers[m_instances[p.m_instance].m_firstMember].m_worker;
      OU::Property &wp = w.property(p.m_property);
      if (wp.m_isReadable) {
	std::string dummy;
	m_launchMembers[m_instances[p.m_instance].m_firstMember].m_worker->
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
      m_launchMembers[m_instances[p.m_instance].m_firstMember].m_worker->
	getProperty(p.m_property, dummy, value, NULL, hex);	 
    }

    void ApplicationI::
    setProperty(const char * worker_inst_name, const char * prop_name, const char *value) {
      Property &p = findProperty(worker_inst_name, prop_name);
      m_launchMembers[m_instances[p.m_instance].m_firstMember].m_worker->
	setProperty(prop_name, value);
    }

    ApplicationI::Instance::Instance() :
      m_feasibleContainers(NULL), m_nCandidates(0), m_usedContainer(0), m_usedContainers(NULL),
      m_firstMember(0) {
    }
    ApplicationI::Instance::~Instance() {
      delete [] m_feasibleContainers;
      if (m_usedContainers != &m_usedContainer)
	delete [] m_usedContainers;
    }
    ApplicationI::Deployment::
    Deployment()
      : m_scale(0), m_container(0), m_impl(NULL), m_containers(NULL), m_impls(NULL),
	m_feasible(0) {
    }
    ApplicationI::Deployment::
    ~Deployment() {
      if (m_scale > 1) {
	delete [] m_containers;
	delete [] m_impls;
      }
    }
    void ApplicationI::Deployment::
    set(size_t scale, unsigned *containers, const OL::Implementation **impls, CMap feasible) {
      if (scale > 1) {
	delete [] m_containers;
	delete [] m_impls;
      } else {
	m_container = *containers;
	containers = &m_container;
	m_impl = *impls;
	impls = &m_impl;
      }
      m_scale = scale;
      m_containers = containers;
      m_impls = impls;
      m_feasible = feasible;
    }
    ApplicationI::Deployment &ApplicationI::Deployment::
    operator=(const ApplicationI::Deployment &d) {
      delete [] m_containers;
      delete [] m_impls;
      m_scale = d.m_scale;
      m_container = d.m_container;
      m_impl = d.m_impl;
      m_feasible = d.m_feasible;
      if (m_scale > 1) {
	m_containers = new unsigned[m_scale];
	m_impls = new const OL::Implementation *[m_scale];
	for (unsigned n = 0; n < m_scale; n++) {
	  m_containers[n] = d.m_containers[n];
	  m_impls[n] = d.m_impls[n];
	}
      } else {
	m_containers = &m_container;
	m_impls = &m_impl;
      }
      return *this;
    }
  }
  namespace API {
    OCPI_EMIT_REGISTER_FULL_VAR( "Get Property", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, pegp ); 
    OCPI_EMIT_REGISTER_FULL_VAR( "Set Property", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, pesp ); 

    Application::Application(const char *file, const PValue *params)
      : m_application(*new ApplicationI(*this, file, params)) {
    }
    Application::Application(const std::string &string, const PValue *params)
      : m_application(*new ApplicationI(*this, string, params)) {
    }
    Application::Application(Application & app,  const PValue *params)
      : m_application(*new ApplicationI(*this, app.m_application.assembly(), params)) {
    }
    Application::~Application() { delete &m_application; }
    void Application::initialize() { m_application.initialize(); }
    void Application::start() { m_application.start(); }
    void Application::stop() { m_application.stop(); }
    bool Application::wait( unsigned timeout_us ) {
      OS::Timer *timer = NULL;
      if (timeout_us) 
	timer = new OS::Timer((uint32_t)(timeout_us/1000000ul), (uint32_t)((timeout_us%1000000) * 1000ull));
      bool r = m_application.wait(timer);
      delete timer;
      return r;
    }
    void Application::finish() {
      m_application.finish();
    }
    ExternalPort &Application::getPort(const char *name, const OA::PValue *params) {
      return m_application.getPort(name, params);
    }
    bool Application::getProperty(unsigned ordinal, std::string &name, std::string &value,
				  bool hex, bool *parp) {
      return m_application.getProperty(ordinal, name, value, hex, parp);
    }
    void Application::getProperty(const char* w, const char* p, std::string &value, bool hex) {
      OCPI_EMIT_STATE_NR( pegp, 1 );
      m_application.getProperty(w, p, value, hex);
      OCPI_EMIT_STATE_NR( pegp, 0 );

    }
    void Application::getProperty(const char* w, std::string &value, bool hex) {
      OCPI_EMIT_STATE_NR( pegp, 1 );
      m_application.getProperty(NULL, w, value, hex);
      OCPI_EMIT_STATE_NR( pegp, 0 );

    }
    void Application::setProperty(const char* w, const char* p, const char *value) {
      OCPI_EMIT_STATE_NR( pesp, 1 );
      m_application.setProperty(w, p, value);
      OCPI_EMIT_STATE_NR( pesp, 0 );

    }
    void Application::setProperty(const char* p, const char *value) {
      OCPI_EMIT_STATE_NR( pesp, 1 );
      m_application.setProperty(NULL, p, value);
      OCPI_EMIT_STATE_NR( pesp, 0 );

    }
    Worker &Application::getPropertyWorker(const char *name) { return m_application.getPropertyWorker(name); }

  }
}
