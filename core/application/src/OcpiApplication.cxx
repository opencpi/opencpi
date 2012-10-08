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
#include "OcpiOsMisc.h"
#include "OcpiUtilValue.h"
#include "OcpiPValue.h"
#include "OcpiApplication.h"
#include "OcpiTimeEmit.h"

namespace OC = OCPI::Container;
namespace OU = OCPI::Util;
namespace OL = OCPI::Library;
namespace OA = OCPI::API;
namespace OCPI {
  namespace API {
    ApplicationI::ApplicationI(const char *file, const PValue * policy)
      : m_assembly(*new OL::Assembly(file))  {
      init(policy);
    }
    ApplicationI::ApplicationI(const std::string &string, const PValue * policy)
      : m_assembly(*new OL::Assembly(string))  {
      init(policy);
    }
    ApplicationI::ApplicationI(OL::Assembly &assy, const PValue * policy)
      : m_assembly(assy) {
      m_assembly++;
      init(policy);
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
      delete [] m_workers;
    }
    unsigned ApplicationI::
    addContainer(unsigned container) {
      ocpiAssert(!(m_allMap & (1 << container)));
      m_usedContainers[m_nContainers] = container;
      m_allMap |= 1 << container;
      m_global2used[container] = m_nContainers;
      return m_nContainers++;
    }

    /*
    so we made choices during the feasibility analysis, but here we want to add some policy.
    the default allocation will bias toward collocation, so this is basically to 
    spread things out.
    Since exclusive/bitstream allocations are not really adjustable, we just deal with the others.
    we haven't remembered ALL deployments, just the "best".
    we have preferred internally connected impls by scoring them up, which has inherently consolidated then.
    (preferred collocation)
    so somehow we need to keep the policy while keeping the fixed allocation...
    perhaps we need to record some sort of exclusivity and collocation constraints
    the def
    */
    // For dynamic instances only, distribute them according to policy
    void ApplicationI::
    policyMap( Instance * i, CMap & bestMap)
    {
      // bestMap is a bitmap of the best availabe containers that the implementation can be mapped to
      // alliMap is the bitmap of all suitable containers for the implementation
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
	for (unsigned n = 0; n < OC::Container::s_nContainers; n++)
	  if ((bestMap & (1 << n)) && !(m_allMap & (1 << n))) {
	    m_currConn = m_nContainers;
	    i->m_container = addContainer(n);
	    return; // We added a new one - and used it
	  }
	// We have to use one we have since only those are feasible
	do {
	  if (++m_currConn >= OC::Container::s_nContainers)
	    m_currConn = 0;
	} while (!(bestMap & (1 << m_usedContainers[m_currConn])));
	i->m_container = m_currConn;
	ocpiDebug("best 0x%x curr %u cont %u", bestMap, m_currConn, m_usedContainers[m_currConn]);
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
	for (n = 0; n < OC::Container::s_nContainers; n++)
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
      for (unsigned nn = 0; nn < nPorts; nn++) {
	OU::Assembly::Port
	  *ap = m_assembly.assyPort(instNum, nn),
	  *other = ap ? ap->m_connectedPort : NULL;
	if (ap &&                          // if the port is even mentioned in the assembly?
	    other &&                       // if the port is connected in the assembly
	    other->m_instance < instNum &&  // if the other instance has been processed
	    m_assembly.                    // then check for prewired compatibility
	    badConnection(*c.impl, *m_instances[other->m_instance].m_impl, *ap, nn)) {
	  ocpiDebug("For instance \"%s\" for spec \"%s\" rejecting implementation \"%s%s%s\" with score %u "
		    "from artifact \"%s\" due to connectivity conflict",
		    m_assembly.m_instances[instNum].m_name.c_str(),
		    m_assembly.m_instances[instNum].m_specName.c_str(),
		    c.impl->m_metadataImpl.name().c_str(),
		    c.impl->m_staticInstance ? "/" : "",
		    c.impl->m_staticInstance ? ezxml_cattr(c.impl->m_staticInstance, "name") : "",
		    c.score, c.impl->m_artifact.name().c_str());
	  ocpiDebug("Other is instance \"%s\" for spec \"%s\" implementation \"%s%s%s\" "
		    "from artifact \"%s\".",
		    m_assembly.m_instances[other->m_instance].m_name.c_str(),
		    m_assembly.m_instances[other->m_instance].m_specName.c_str(),
		    m_instances[other->m_instance].m_impl->m_metadataImpl.name().c_str(),
		    m_instances[other->m_instance].m_impl->m_staticInstance ? "/" : "",
		    m_instances[other->m_instance].m_impl->m_staticInstance ?
		    ezxml_cattr(m_instances[other->m_instance].m_impl->m_staticInstance, "name") : "",
		    m_instances[other->m_instance].m_impl->m_artifact.name().c_str());
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
		  m_assembly.m_instances[n].m_name.c_str(),
		  m_assembly.m_instances[n].m_specName.c_str(),
		  c.impl->m_metadataImpl.name().c_str(),
		  c.impl->m_staticInstance ? "/" : "",
		  c.impl->m_staticInstance ? ezxml_cattr(c.impl->m_staticInstance, "name") : "",
		  c.score, c.impl->m_artifact.name().c_str());
	return false;
      }
      return true;
    }
    
    void ApplicationI::
    finalizeProperties() {
      Instance *i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	// The chosen, best, feasible implementation for the instance
	const OL::Implementation &impl = *i->m_impl;

	// Check any assembly-supplied initial properties of the
	// instance against the implementation metadata to see if the
	// names and values are valid. We don't save the results, but
	// later we can assume they are valid.
	const OU::Assembly::Properties &aProps = m_assembly.m_instances[n].m_properties;
	unsigned nPropValues = aProps.size();
	if (nPropValues) {
	  OU::Value *pv = i->m_propValues = new OU::Value[nPropValues];
	  unsigned *pn = i->m_propOrdinals = new unsigned[nPropValues];
	  for (unsigned p = 0; p < nPropValues; p++, pv++, pn++) {
	    // findProperty throws on error if bad name
	    OU::Property &uProp = impl.m_metadataImpl.findProperty(aProps[p].m_name.c_str());
	    const char *err;
	    *pn = uProp.m_ordinal; // remember position in property list 
	    pv->setType(uProp);    // set the data type of the Value from the metadata property
	    if ((err = pv->parse(aProps[p].m_value.c_str())))
	      // Someday a std::string-friendly sprintf
	      throw OU::Error("Value \"%s\" for property \"%s\" of instance \"%s\" of "
			      "worker \"%s\" is invalid for its type",
			      aProps[p].m_value.c_str(), aProps[p].m_name.c_str(),
			      m_assembly.m_instances[n].m_name.c_str(),
			      impl.m_metadataImpl.specName().c_str());
	  }
	}
      }
      // For all instances in the assembly
      // FIXME: allow XML expression of specific renamed mappings:
      //       <property name="foo" instance="bar" [property="baz"]/>
      m_nProperties = 0;
      i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++)
	m_nProperties += i->m_impl->m_metadataImpl.m_nProperties;
      Property *p = m_properties = new Property[m_nProperties];
      i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	unsigned nProps;
	OU::Property *mp = i->m_impl->m_metadataImpl.properties(nProps);
	for (unsigned nn = 0; nn < nProps; nn++, mp++, p++) {
	  p->m_name = m_assembly.m_instances[n].m_name + ":" + mp->m_name;
	  p->m_instance = n;
	  p->m_property = nn;
	  ocpiDebug("Instance %s (%u) property %s (%u) named %s", 
		    m_assembly.m_instances[n].m_name.c_str(), n,
		    mp->m_name.c_str(), nn, p->m_name.c_str());		    
	}
      }
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
	OL::Candidate &c = m_assembly.m_candidates[instNum][m];	  
	i->m_impl = c.impl; // temporary, but needed by (at least) connectionsOk
	if (connectionsOk(c, instNum))
	  for (unsigned cont = 0; cont < OC::Container::s_nContainers; cont++) {
	    Booking &b = m_bookings[cont];
	    if (i->m_feasibleContainers[m] & (1 << cont) && bookingOk(b, c, instNum)) {
	      d->container = cont;
	      d->candidate = m;
	      unsigned myScore = score + c.score;
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

    void ApplicationI::
    init( const PValue * policy ) {

      // In order from class definition
      m_nInstances = m_assembly.m_instances.size();
      m_instances = new Instance[m_nInstances];
      m_bookings = new Booking[OC::Container::s_nContainers];
      m_deployments = new Deployment[m_nInstances];
      m_bestDeployments = new Deployment[m_nInstances];
      m_properties = NULL;
      m_nProperties = 0;
      m_curMap = 0;
      m_curContainers = 0;
      m_allMap = 0;
      m_global2used = new unsigned[OC::Container::s_nContainers];
      m_nContainers = 0;
      m_usedContainers = new unsigned[OC::Container::s_nContainers];
      m_containers = NULL;    // allocated when we know how many we are using
      m_containerApps = NULL; // ditto
      m_workers = NULL;
      m_doneWorker = NULL;
      //      m_externalPorts = NULL;
      //      m_externalNames = NULL;
      m_cMapPolicy = RoundRobin;
      m_processors = 0;
      m_currConn = OC::Container::s_nContainers - 1;
      // Set the instance map policy
      setPolicy( policy );


      // First pass - make sure there are some containers to support some candidate
      // and remember which containers can support which candidates
      Instance *i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	OL::Candidates &cs = m_assembly.m_candidates[n];
	i->m_nCandidates = cs.size();
	i->m_feasibleContainers = new CMap[cs.size()];
	//	i->m_containers = new unsigned[i->m_nCandidates];
	//	memset(i->m_containers, 0, sizeof(unsigned) * i->m_nCandidates);
	CMap sum = 0;
	for (unsigned m = 0; m < i->m_nCandidates; m++) {
	  m_curMap = 0;        // to accumulate containers suitable for this candidate
	  m_curContainers = 0; // to count suitable containers for this candidate
	  (void)OC::Manager::findContainers(*this, cs[m].impl->m_metadataImpl);
	  i->m_feasibleContainers[m] = m_curMap;
	  sum |= m_curMap;
	}
	if (!sum)
	  throw OU::Error("For instance \"%s\" for spec \"%s\": "
			  "no feasible containers found for any implementation",
			  m_assembly.m_instances[n].m_name.c_str(),
			  m_assembly.m_instances[n].m_specName.c_str());
      }
      // Second pass - search for best feasible choice
      // FIXME: we are assuming broadly that dynamic instances have universal connectivity
      // FIXME: we are assuming that an artifact is exclusive it is has static instances.
      // FIXME: we are assuming that if an artifact has a static instance, all of its instances are

      m_bestScore = 0;
      doInstance(0, 0);

#if 0
      // The per-container structure that records container commitments as we go
      Booking bookings[OC::Container::s_nContainers];
      // The per-instance structure to record decisions and algorithm state
      Deployment
        deployments[m_nInstances],     // current deployment
	bestDeployments[m_nInstances]; // best deployment

       *d; 
      memset(deployments, 0, sizeof(deployments));
      // Breadth first search for best combinations of candidate/container per instance
      bool foundAny;
      do { // look for overall solutions, and keey trying while we find new ones
	foundAny = false;
	unsigned cumScore = 0;
	i = m_instances;
	d = deployments;
	memset(bookings, 0, sizeof(Booking) * OC::Container::s_nContainers);
	for (unsigned n = 0; n < m_nInstances; n++, i++, d++) {
	  for (unsigned m = 0; m < i->m_nCandidates; m++) {
	    OL::Candidate &c = m_assembly.m_candidates[n][m];	  
	    i->m_impl = c.impl; // temporary, but needed by (at least) connectionsOk
	    if (connectionsOk(c, n))
	      for (unsigned cont = 0; cont < OC::Container::s_nContainers; cont++) {
		Booking &b = bookings[cont];
		if (i->m_feasibleContainers[m] & (1 << cont) && bookingOk(b, c, n)) {
		  // We have a feasible choice here.  Record booking.
		  d->container = cont;
		  d->candidate = m;
		  b.m_artifact = &c.impl->m_artifact;
		  b.m_usedImpls |= 1 << c.impl->m_ordinal;
		  cumScore += c.score;
		  if (n >= m_nInstances-1) {
		    dumpDeployment(cumScore, deployments);
		    if (cumScore > bestScore) {
		      memcpy(bestDeployments, deployments, sizeof(Deployment)*m_nInstances);
		      bestScore = cumScore;
		    }
		  }
		}
	      }
	  }
	}
      } while (foundAny);
#endif
      // Record the implementation from the best deployment
      i = m_instances;
      Deployment *d = m_bestDeployments;
      for (unsigned n = 0; n < m_nInstances; n++, i++, d++)
	i->m_impl = m_assembly.m_candidates[n][d->candidate].impl;
      // All the implementation selection is done, so now do the final check of properties
      // since properties can be implementation specific
      finalizeProperties();

      // Up to now we have just been "planning" and not doing things.
      // Now invoke the policy method to map the dynamic instances to containers
      // First we do a pass that will only map the dynamic implementations
      i = m_instances;
      d = m_bestDeployments;
      for (unsigned n = 0; n < m_nInstances; n++, i++, d++) {
	if (!i->m_impl->m_staticInstance)
	  policyMap(i, i->m_feasibleContainers[d->candidate]);
      }
      // Now add the containers for the static instances
      i = m_instances;
      d = m_bestDeployments;
      ocpiInfo("Final deployment after processor policy applied is:");
      for (unsigned n = 0; n < m_nInstances; n++, i++, d++) {
	const OL::Implementation &impl = *i->m_impl;
	if (impl.m_staticInstance) {
	  unsigned cNum = d->container;
	  i->m_container = m_allMap & (1 << cNum) ? m_global2used[cNum] : addContainer(cNum);
	}
	OC::Container &c = OC::Container::nthContainer(m_usedContainers[i->m_container]);
	ocpiInfo(" Instance %2u %s (spec %s) on %s container %s, using %s%s%s in %s", 
		 n, m_assembly.m_instances[n].m_name.c_str(),
		 m_assembly.m_instances[n].m_specName.c_str(),
		 c.m_model.c_str(), c.name().c_str(),
		 impl.m_metadataImpl.name().c_str(),
		 impl.m_staticInstance ? "/" : "",
		 impl.m_staticInstance ? ezxml_cattr(impl.m_staticInstance, "name") : "",
		 impl.m_artifact.name().c_str());
      }
      // To prepare for remote containers, we need to organize the containers we are
      // using by their server.  Then we essentially synthesize an "app" for each server.
      // so the interface to the server is a subset of the info here.
      // There is the top side and the bottom side.
      // THe bottom side is recogniing that you have "remote" connections.
      // The top side is decomposing and merging etc.
      // Assuming that we send the server an XML assembly, it means we need to express remote
      // connections in that xml.
    }
    bool
    ApplicationI::foundContainer(OCPI::Container::Container &c) {
      m_curMap |= 1 << c.ordinal();
      m_curContainers++;
      return false;
    }

    void ApplicationI::initialize() {
      m_nInstances = m_assembly.m_instances.size();
      ocpiDebug("Mapped %d instances to %d containers", m_nInstances, m_nContainers);

      m_containers = new OC::Container *[m_nContainers];
      m_containerApps = new OC::Application *[m_nContainers];
      m_workers = new OC::Worker *[m_nInstances];
      for (unsigned n = 0; n < m_nContainers; n++) {
	// FIXME: get rid of this cast...
	m_containers[n] = &OC::Container::nthContainer(m_usedContainers[n]);
	m_containerApps[n] = static_cast<OC::Application*>(m_containers[n]->createApplication());
      }
      Instance *i = m_instances;
      for (unsigned n = 0; n < m_nInstances; n++, i++) {
	const OL::Implementation &impl = *i->m_impl;
	OC::Worker &w =
	  m_containerApps[i->m_container]->
	  createWorker(impl.m_artifact,                          // The artifact of the library impl
		       m_assembly.m_instances[n].m_name.c_str(), // The instance name in the assembly
		       impl.m_metadataImpl.m_xml,                // the xml of the impl (from artifact)
		       impl.m_staticInstance,                    // the xml of the fixed instance (from artifact)
		       NULL);                                    // wparams
	m_workers[n] = &w;
	// Now we need to set the initial properties - either from assembly or from defaults
	const OU::Assembly::Properties &aProps = m_assembly.m_instances[n].m_properties;
	unsigned nPropValues = aProps.size();
	for (unsigned p = 0; p < nPropValues; p++)
	  w.setProperty(i->m_propOrdinals[p], i->m_propValues[p]);
	unsigned nProps = impl.m_metadataImpl.m_nProperties;
	OU::Property *prop = impl.m_metadataImpl.m_properties;
	for (unsigned p = 0; p < nProps; p++, prop++)
	  if (prop->m_defaultValue) {
	    bool found = false;
	    for (unsigned m = 0; m < nPropValues; m++)
	      if (i->m_propOrdinals[m] == prop->m_ordinal) {
		found = true;
		break;
	      }
	    if (!found)
	      w.setProperty(prop->m_ordinal, *prop->m_defaultValue);
	  }
      }
      if (m_assembly.m_doneInstance != -1)
	m_doneWorker = m_workers[m_assembly.m_doneInstance];
      unsigned nConns = m_assembly.m_connections.size();
      for (unsigned n = 0; n < nConns; n++) {
	const OU::Assembly::Connection &c = m_assembly.m_connections[n];
	OA::Port &apiPort = m_workers[c.m_ports[0].m_instance]->getPort(c.m_ports[0].m_name.c_str());
	const OU::Assembly::Port &assPort = c.m_ports[0];
	if (c.m_externals.size() == 0)
	  apiPort.connect(m_workers[c.m_ports[1].m_instance]->getPort(c.m_ports[1].m_name.c_str()),
			  assPort.m_parameters, c.m_ports[1].m_parameters);
	else {
	  const OU::Assembly::External &e = c.m_externals[0];
	  if (e.m_url.size())
	    apiPort.connectURL(e.m_url.c_str(), assPort.m_parameters, e.m_parameters);
	  else {
	    //ExternalPort &ep = apiPort.connectExternal(e.m_name.c_str(), assPort.m_parameters, e.m_parameters);
	    
	    m_externals.insert(ExternalPair(e.m_name.c_str(), External(apiPort, e.m_parameters)));
	  }
	}
      }
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
      if (m_doneWorker)
	return m_doneWorker->wait(timer);
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

    ExternalPort &ApplicationI::getPort(const char *name) {
      Externals::iterator ei = m_externals.find(name);
      if (ei == m_externals.end())
	throw OU::Error("Unknown external port name for application: \"%s\"", name);
      External &ext = ei->second;
      if (!ext.m_external)
	ext.m_external = &ext.m_port.connectExternal(name, ext.m_params);
      return *ext.m_external;
    }

    Worker &ApplicationI::getPropertyWorker(const char *name) {
      Property *p = m_properties;
      for (unsigned n = 0; n < m_nProperties; n++, p++)
	if (!strcasecmp(name, p->m_name.c_str()))
	  return *m_workers[p->m_instance];
      throw OU::Error("Unknown application property: %s", name);
    }

    static inline const char *maybeColon(const char *name) {
      const char *cp = strchr(name, ':');
      return cp ? cp + 1 : name;
    }
    // FIXME:  consolidate the constructors (others are in OcpiProperty.cxx) (have in internal class for init)
    // FIXME:  avoid the double lookup since the first one gets us the ordinal
    Property::Property(Application &app, const char *aname)
      : m_worker(app.getPropertyWorker(aname)),
	m_readSync(false), m_writeSync(false), m_writeVaddr(0), m_readVaddr(0),
	m_info(m_worker.setupProperty(maybeColon(aname), m_writeVaddr, m_readVaddr))
    {
      m_readSync = m_info.m_readSync;
      m_writeSync = m_info.m_writeSync;
    }

    bool ApplicationI::getProperty(unsigned ordinal, std::string &name, std::string &value) {
      if (ordinal >= m_nProperties)
	return false;
      Property &p = m_properties[ordinal];
      name = p.m_name;
      std::string dummy;
      return m_workers[p.m_instance]->getProperty(p.m_property, dummy, value);
    }

    ApplicationI::Property &ApplicationI::
    findProperty(const char * worker_inst_name, const char * prop_name) {
      std::string nm(worker_inst_name);
      nm += ":";
      nm += prop_name;
      Property *p = m_properties;
      for (unsigned n = 0; n < m_nProperties; n++, p++)
	if (!strcasecmp(nm.c_str(), p->m_name.c_str()))
	  return *p;
      throw OU::Error("Unknown application property: %s", nm.c_str());
    }

    void ApplicationI::
    getProperty(const char * worker_inst_name, const char * prop_name, std::string &value) {
      Property &p = findProperty(worker_inst_name, prop_name);
      std::string dummy;
      m_workers[p.m_instance]->getProperty(p.m_property, dummy, value);	 
    }

    void ApplicationI::
    setProperty(const char * worker_inst_name, const char * prop_name, const char *value) {
      Property &p = findProperty(worker_inst_name, prop_name);
      m_workers[p.m_instance]->setProperty(prop_name, value);
    }

    ApplicationI::Instance::Instance() :
      m_impl(NULL), m_propValues(NULL), m_propOrdinals(NULL) { //, m_candidate(0) {
    }
    ApplicationI::Instance::~Instance() {
      delete [] m_propValues;
      delete [] m_propOrdinals;
      //      delete [] m_containers;
      delete [] m_feasibleContainers;
    }

  }
  namespace API {
    OCPI_EMIT_REGISTER_FULL_VAR( "Get Property", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, pegp ); 
    OCPI_EMIT_REGISTER_FULL_VAR( "Set Property", OCPI::Time::Emit::u, 1, OCPI::Time::Emit::State, pesp ); 

    Application::Application(const char *file, const PValue * policy)
      : m_application(*new ApplicationI(file,policy)) {
    }
    Application::Application(const std::string &string, const PValue * policy)
      : m_application(*new ApplicationI(string,policy)) {
    }
    Application::Application(Application & app,  const PValue * policy)
      : m_application(*new ApplicationI(app.m_application.assembly(),policy)) {
    }
    Application::~Application() { delete &m_application; }
    void Application::initialize() { m_application.initialize(); }
    void Application::start() { m_application.start(); }
    void Application::stop() { m_application.start(); }
    bool Application::wait( unsigned timeout_us ) {
      OS::Timer *timer = NULL;
      if (timeout_us) 
	timer = new OS::Timer(timeout_us/1000000ul, (timeout_us%1000000) * 1000ull);
      bool r = m_application.wait(timer);
      delete timer;
      return r;
    }
    ExternalPort &Application::getPort(const char *name) { return m_application.getPort(name); }
    bool Application::getProperty(unsigned ordinal, std::string &name, std::string &value) {
      return m_application.getProperty(ordinal, name, value);
    }
    void Application::getProperty(const char* w, const char* p, std::string &value) {
      OCPI_EMIT_STATE_NR( pegp, 1 );
      m_application.getProperty(w, p, value);
      OCPI_EMIT_STATE_NR( pegp, 0 );

    }
    void Application::setProperty(const char* w, const char* p, const char *value) {
      OCPI_EMIT_STATE_NR( pesp, 1 );
      m_application.setProperty(w, p, value);
      OCPI_EMIT_STATE_NR( pesp, 0 );

    }
    Worker &Application::getPropertyWorker(const char *name) { return m_application.getPropertyWorker(name); }

  }
}
