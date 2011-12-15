
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
#include "OcpiUtilValue.h"
#include "OcpiApplication.h"

namespace OC = OCPI::Container;
namespace OU = OCPI::Util;
namespace OL = OCPI::Library;
namespace OA = OCPI::API;
namespace OCPI {
  namespace API {
    ApplicationI::ApplicationI(const char *file)
      : m_assembly(*new OL::Assembly(file)), m_ownAssembly(true) {
      init();
    }
    ApplicationI::ApplicationI(const std::string &string)
      : m_assembly(*new OL::Assembly(string)), m_ownAssembly(true) {
      init();
    }
    ApplicationI::ApplicationI(const OL::Assembly &assy)
      : m_assembly(assy), m_ownAssembly(false) {
      init();
    }
    ApplicationI::~ApplicationI() {
      if (m_ownAssembly)
	delete &m_assembly;
      if (m_containerApps) {
	for (unsigned n = 0; n < m_nContainers; n++)
	  delete m_containerApps[n];
	delete [] m_containerApps;
      }
      delete [] m_workers;
      delete [] m_usedContainers;
      delete [] m_instances;
      delete [] m_containers;
    }
    bool ApplicationI::foundContainer(OCPI::Container::Container &c) {
      m_curMap |= 1 << c.ordinal();
      return false;
    }
    void ApplicationI::init() {
      unsigned
	nInstances = m_assembly.m_instances.size(),
	currConn = 0; // for rotating placement
      m_instances = new Instance[nInstances];
      m_allMap = m_curMap = 0;   // accumulate all containers actually used
      m_nContainers = 0;
      m_usedContainers = new unsigned[nInstances]; // over allocated - could use less
      m_containers = NULL;
      m_containerApps = NULL;
      m_workers = NULL;
      m_externalPorts = NULL;
      m_externalNames = NULL;

      // For all instances in the assembly
      Instance *i = m_instances;
      for (unsigned n = 0; n < nInstances; n++, i++) {
	OL::Candidates &cs = m_assembly.m_candidates[n];
	CMap
	  sum = 0,     // accumulate possible containers over candidates
	  bestMap = 0; // save map of best candidate.  initialized to kill warning
	unsigned bestScore = 0;
	// For all candidate implementations knwon to be suitable for this instance
	for (unsigned m = 0; m < cs.size(); m++) {
	  OL::Candidate &c = cs[m];
	  m_curMap = 0; // to accumulate containers suitable for this candidate
	  (void)OC::Manager::findContainers(*this, cs[m].impl->m_metadataImpl);
	  // If the candidate has feasible containers, evaluate based on score.
	  if (m_curMap && c.score > bestScore) {
	    bestScore = c.score;
	    i->m_impl = c.impl;
	    bestMap = m_curMap;
	  }
	  sum |= m_curMap;
	}
	if (!sum)
	  throw OU::Error(cs.size() > 1 ?
			  "There are no containers for any of the %d suitable implementations "
			  "for instance %s of component (spec) %s" :
			  "There are no containers for the %d suitable implementation "
			  "for instance %s of component (spec) %s",
			  cs.size(), m_assembly.m_instances[n].m_name.c_str(),
			  m_assembly.m_instances[n].m_specName.c_str());
	// Now that we have an implementation we can finally check any assembly-supplied
	// initial properties of the instance against the implementation metadata to
	// see if the names and values are valid. We don't save the results,
	// but later we can assume they are valid.
	const OL::Implementation &impl = *i->m_impl;
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
	// Now we must select the container for the best candidate.
	// As a default, we will rotate through the possible containers to spread them out
	// So, find the next (rotating) container for the best candidate
	CMap map;
	for (map = 1 << currConn; !(map & bestMap); map = 1 << currConn)
	  if (++currConn >= OC::Container::maxContainer)
	    currConn = 0;
	if (!(m_allMap & map)) {
	  // A container we have not used yet.
	  m_allMap |= map;
	  i->m_container = m_nContainers;
	  // Note we save the container ordinal, not the container pointer,
	  // simply to allow the container to go away after this API (construction)
	  // and before the next (initialize).  At least we can check for errors.
	  m_usedContainers[m_nContainers++] = currConn;
	} else
	  for (unsigned c = 0; c < m_nContainers; c++)
	    if (m_usedContainers[c] == currConn) {
	      i->m_container = c;
	      break;
	    }
      }
    }
    void ApplicationI::initialize() {
      unsigned nInstances = m_assembly.m_instances.size();
      m_containers = new OC::Container *[m_nContainers];
      m_containerApps = new OC::Application *[m_nContainers];
      m_workers = new OC::Worker *[nInstances];
      for (unsigned n = 0; n < m_nContainers; n++) {
	OC::Container &c = OC::Container::nthContainer(m_usedContainers[n]);
	m_containers[n] = &c;
	// FIXME: get rid of this cast...
	m_containerApps[n] = static_cast<OC::Application*>(c.createApplication());
      }
      Instance *i = m_instances;
      for (unsigned n = 0; n < nInstances; n++, i++) {
	const OL::Implementation &impl = *i->m_impl;
	OC::Worker &w =
	  m_containerApps[i->m_container]->
	  createWorker(impl.m_artifact,                          // The artifact of the library impl
		       m_assembly.m_instances[n].m_name.c_str(), // The instance name in the assembly
		       impl.m_metadataImpl.m_xml,                // the xml of the impl (from artifact)
		       impl.m_instance,                          // the xml of the fixed instance (from artifact)
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
	    ExternalPort &ep = apiPort.connectExternal(e.m_name.c_str(), assPort.m_parameters, e.m_parameters);
	    m_externals.insert(ExternalPair(e.m_name.c_str(), &ep));
	  }
	}
      }
    }
    void ApplicationI::start() {
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->start();
    };
    void ApplicationI::stop() {
      for (unsigned n = 0; n < m_nContainers; n++)
	m_containerApps[n]->stop();
    }
    ExternalPort &ApplicationI::getPort(const char *name) {
      Externals::iterator ei = m_externals.find(name);
      if (ei == m_externals.end())
	throw OU::Error("Unknown external port name for application: \"%s\"", name);
      return *ei->second;
    }

    ApplicationI::Instance::Instance() :
      m_impl(NULL), m_propValues(NULL), m_propOrdinals(NULL) {
    }
    ApplicationI::Instance::~Instance() {
      delete [] m_propValues;
      delete [] m_propOrdinals;
    }

  }
  namespace API {

    Application::Application(const char *file)
      : m_application(*new ApplicationI(file)) {
    }
    Application::Application(const std::string &string)
      : m_application(*new ApplicationI(string)) {
    }
    void Application::initialize() { m_application.initialize(); }
    void Application::start() { m_application.start(); }
    void Application::stop() { m_application.start(); }
    ExternalPort &Application::getPort(const char *name) { return m_application.getPort(name); }
    Application::~Application() { delete &m_application; }
  }
}
