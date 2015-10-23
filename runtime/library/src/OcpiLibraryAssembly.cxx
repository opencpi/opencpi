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
#include <inttypes.h>
#include <strings.h>
#include "OcpiLibraryAssembly.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilEzxml.h"

namespace OCPI {
  namespace Library {
    namespace OU = OCPI::Util;
    namespace OE = OCPI::Util::EzXml;
    // Attributes specific to an application assembly
    static const char *assyAttrs[] = { "maxprocessors", "minprocessors", "roundrobin", "done", NULL};
    // The instance attributes relevant to app assemblies - we don't really deal with "container" here
    // FIXME: It should be in the upper level
    static const char *instAttrs[] = { "model", "platform", "container", NULL};
    Assembly::Assembly(const char *file, const OCPI::Util::PValue *params)
      : OU::Assembly(file, assyAttrs, instAttrs, params), m_refCount(1) {
      findImplementations(params);
    }
    Assembly::Assembly(const std::string &string, const OCPI::Util::PValue *params)
      : OU::Assembly(string, assyAttrs, instAttrs, params), m_refCount(1) {
      findImplementations(params);
    }
    Assembly::~Assembly() {
      for (size_t n = 0; n < m_instances.size(); n++)
	delete m_instances[n];
    }

    void
    Assembly::
    operator ++( int )
    {
      m_refCount++;
    }

    void
    Assembly::
    operator --(int)
    {
      if (--m_refCount == 0)
	delete this;
    }

    // The util::assembly only knows port names, not worker port ordinals
    // (because it has not been correlated with any implementations).
    // Here is where we process the matchup between the port names in the util::assembly
    // and the port names in implementations in the libraries
    bool Assembly::Instance::
    resolveUtilPorts(const Implementation &i, OU::Assembly &utilAssy) {
      // This test works even when there are no ports
      if (m_assyPorts) {
	// We have processed an implementation before, just check for consistency
	if (i.m_metadataImpl.nPorts() != m_nPorts) {
	  ocpiInfo("Rejected: port number mismatch (%u vs %u) between implementations of worker %s.",
		   i.m_metadataImpl.nPorts(), m_nPorts, i.m_metadataImpl.specName().c_str());
	  return false;
	}
	// FIXME: we should compare more implementation info for compatibility?
	return true;
      }
      // The one-time action for the first implementation found for an instance
      // If it fails for some reason, we just rejected it and wait for another one.
      // We undo any side effects on failure so that if a good match comes later,
      // we can accept it.
      OU::Port *ports = i.m_metadataImpl.ports(m_nPorts);
      OU::Assembly::Port **ap = new OU::Assembly::Port *[m_nPorts];
      for (unsigned n = 0; n < m_nPorts; n++)
	ap[n] = NULL;
      OU::Assembly::Instance &inst = m_utilInstance;
      OU::Port *p;

      // build the map from implementation port ordinals to util::assembly::ports
      for (std::list<OU::Assembly::Port*>::const_iterator pi = inst.m_ports.begin(); 
	   pi != inst.m_ports.end(); pi++) {
	bool found = false;
	if ((*pi)->m_name.empty()) {
	  // Resolve empty port names to be unambiguous if possible
	  p = ports;
	  for (unsigned n = 0; n < m_nPorts; n++, p++)
	    if ((*pi)->m_role.m_provider && p->m_provider ||
		!(*pi)->m_role.m_provider && !p->m_provider) {
	      if (found) {
		  ocpiInfo("Rejected: the '%s' connection at instance '%s' is ambiguous: "
			   " port name must be specified.",
			   (*pi)->m_role.m_provider ? "input" : "output",
			   m_utilInstance.m_name.c_str());
		  goto rejected;
	      }
	      ap[n] = *pi;
	      found = true;
	    }
	  if (!found) {
	    ocpiInfo("Rejected: there is no %s port for connection at instance '%s'.",
		     (*pi)->m_role.m_provider ? "input" : "output",
		     m_utilInstance.m_name.c_str());
	    goto rejected;
	  }
	} else {
	  p = ports;
	  for (unsigned n = 0; n < m_nPorts; n++, p++)
	    if (!strcasecmp(ports[n].m_name.c_str(), (*pi)->m_name.c_str())) {
	      ap[n] = *pi;
	      (*pi)->m_role.m_knownRole = true;
	      (*pi)->m_role.m_provider = p->m_provider;
	      found = true;
	      break;
	    }
	  if (!found) {
	    ocpiInfo("Rejected: assembly instance '%s' of worker '%s' has no port named '%s'",
		     inst.m_name.c_str(), i.m_metadataImpl.specName().c_str(),
		     (*pi)->m_name.c_str());
	    goto rejected;
	  }
	}
      }
      // Final side effects on success
      if (inst.m_externals) {
	// If the OU::Assembly instance specified that unconnected ports
	// should be externalized, we do that now.
	p = ports;
	for (unsigned n = 0; n < m_nPorts; n++, p++) {
	  bool found = false;
	  for (OU::Assembly::Instance::PortsIter pi = inst.m_ports.begin();
	       pi != inst.m_ports.end(); pi++)
	    if (!strcasecmp((*pi)->m_name.c_str(), p->m_name.c_str())) {
	      found = true;
	      break;
	    }
	  // FIXME: should this externalization only apply to spec ports?
	  if (!found) // Not mentioned in the assembly. Add an external.
	    utilAssy.addExternalConnection(inst.m_ordinal, p->m_name.c_str(),
					   p->m_provider, false, true);
	}	  
      }
      p = ports;
      for (unsigned n = 0; n < m_nPorts; n++, p++)
	if (ap[n] && ap[n]->m_name.empty())
	  // This is a mutable member of a const object.
	  ap[n]->m_name = p->m_name;
      m_assyPorts = ap;
      return true;
    rejected:
      delete [] ap;
      return false;
    }


    // The callback for the findImplementations() method below.
    // Return true if we found THE ONE.
    // Set accepted = true, if we actually accepted one
    bool Assembly::
    foundImplementation(const Implementation &i, bool &accepted) {
      if (m_tempInstance->foundImplementation(i, m_model, m_platform, *this))
	accepted = true;
      return false; // we never terminate the search among possibilities...
    }
    // The library assembly instance has a candidate implementation.
    // Check it out, and maybe accept it as a candidate
    bool Assembly::Instance::
    foundImplementation(const Implementation &i, std::string &model, std::string &platform,
			Assembly &assy) {
      ocpiInfo("Considering implementation for instance \"%s\" with spec \"%s\": \"%s%s%s\" "
	       "from artifact \"%s\"",
	       m_utilInstance.m_name.c_str(),
	       i.m_metadataImpl.specName().c_str(),
	       i.m_metadataImpl.name().c_str(),
	       i.m_staticInstance ? "/" : "",
	       i.m_staticInstance ? ezxml_cattr(i.m_staticInstance, "name") : "",
	       i.m_artifact.name().c_str());
      // Check for worker name match
      if (m_utilInstance.m_implName.size() &&
	  strcasecmp(m_utilInstance.m_implName.c_str(), i.m_metadataImpl.name().c_str())) {
	ocpiInfo("Rejected: worker name is \"%s\", while requested worker name is \"%s\"",
		 i.m_metadataImpl.name().c_str(), m_utilInstance.m_implName.c_str());
	return false;
      }
      // Check for model and platform matches
      if (model.size() && strcasecmp(model.c_str(), i.m_metadataImpl.model().c_str())) {
	ocpiInfo("Rejected: model requested is \"%s\", but the model for this implementation is \"%s\"",
		 model.c_str(), i.m_metadataImpl.model().c_str());
	return false;
      }
      unsigned score;
      if (m_utilInstance.m_selection.empty())
	score = 1;
      else {
	OU::ExprValue val;
	const char *err = OU::evalExpression(m_utilInstance.m_selection.c_str(), val, &i.m_metadataImpl);
	if (!err && !val.isNumber)
	  err = "selection expression has string value";
	if (err)
	  throw OU::Error("Error for instance \"%s\" with selection expression \"%s\": %s",
			  m_utilInstance.m_name.c_str(), m_utilInstance.m_selection.c_str(), err);
	if (val.number <= 0) {
	  ocpiInfo("Rejected: selection expression \"%s\" has value: %" PRIi64,
		   m_utilInstance.m_selection.c_str(), val.number);
	  return false;
	}
	score = (unsigned)(val.number < 0 ? 0 : val.number);
      }
      // To this point all the checking has applied to the worker we are looking at.
      // From this point some of the checking may actually apply to the slave if there is one
      // The aspects that could apply to the slave are:
      // 1. Platform choices
      // 2. Property-based selection
      if (platform.size() && strcasecmp(platform.c_str(),
					i.m_metadataImpl.attributes().platform().c_str())) {
	ocpiInfo("Rejected: platform requested is \"%s\", but the platform is \"%s\"",
		 platform.c_str(), i.m_metadataImpl.attributes().platform().c_str());
	return false;
      }
      // Check for property and parameter matches
      // Mentioned Property values have to be initial, and if parameters, they must match
      // values.
      const OU::Assembly::Properties &aProps = m_utilInstance.m_properties;
      for (unsigned ap = 0; ap < aProps.size(); ap++) {
	const char
	  *apName = aProps[ap].m_name.c_str(),
	  *apValue = aProps[ap].m_value.c_str();
	
	OU::Property *up = i.m_metadataImpl.getProperty(apName);
	if (!up) {
	  ocpiInfo("Rejected: initial property \"%s\" not found", apName);
	  return false;
	}
	if (!aProps[ap].m_hasValue)
	  continue; // used by dumpfile
	OU::Property &uProp = *up;
	if (!uProp.m_isWritable && !uProp.m_isParameter) {
	  ocpiInfo("Rejected: initial property \"%s\" was neither writable nor a parameter", apName);
	  return false;
	}
	OU::Value aValue; // FIXME - save this and use it later
	const char *err = uProp.parseValue(apValue, aValue);
	if (err) {
	  ocpiInfo("Rejected: the value \"%s\" for the \"%s\" property, \"%s\", was invalid",
		   apValue, uProp.m_isImpl ? "implementation" : "spec", apName);
	  return false;
	}
	// We know the supplied value is valid.
	if (uProp.m_isParameter) {
	  std::string pStr;
	  assert(uProp.m_default);
	  uProp.m_default->unparse(pStr); // FIXME: canonical value could be cached if props are compared
	  // Now we have canonicalized the default value in pStr.
	  std::string aStr;
	  aValue.unparse(aStr);
	  if (aStr != pStr) {
	    ocpiInfo("Rejected: property \"%s\" is a parameter compiled with value \"%s\"; requested value is \"%s\"",
		     apName, pStr.c_str(), apValue);
	    return false;
	  }
	  ocpiDebug("Requested '%s' parameter value '%s' matched compiled value '%s'",
		    apName, apValue, pStr.c_str());
	}
      }
      if (!resolveUtilPorts(i, assy))
	return false; // we ignore the impl since it is inconsistent with the assy

      // Here is where we know about the assembly and thus can check for
      // some connectivity constraints.  If the implementation has hard-wired connections
      // that are incompatible with the assembly, we ignore it.
      if (i.m_externals) {
	OU::Port::Mask m = 1;
	for (unsigned n = 0; n < OU::Port::c_maxPorts; n++, m <<= 1)
	  if (m & i.m_externals) {
	    // This port cannot be connected to another instance in the same container.
	    // Thus it PRECLUDES other connected instances on the same container.
	    // So the connected instance cannot have an INTERNAL requirement.
	    // But we can't check is here because it is a constraint about
	    // a pair of choices, not just one choice.
	  }
      }
      if (i.m_internals) {
	OCPI::Library::Connection *c = i.m_connections;
	unsigned nPorts;
	OU::Port *p = i.m_metadataImpl.ports(nPorts);
	OU::Port::Mask m = 1;
	unsigned bump = 0;
	for (unsigned n = 0; n < nPorts; n++, m <<= 1, c++, p++)
	  if (m & i.m_internals) {
	    // Find the assembly connection port for this instance and this 
	    // internally/statically connected port
	    OU::Assembly::Port *ap = m_assyPorts[n];
	    if (ap) {
	      // We found the assembly connection port
	      // Now check that the port connected in the assembly has the same
	      // name as the port connected in the artifact
	      if (!ap->m_connectedPort) {
		ocpiInfo("Rejected because artifact has port '%s' connected while "
			 "application doesn't.", p->m_name.c_str());
		return false;
	      }
	      // This check can only be made for the port of the internal connection that is
	      // for a later instance, since null-named ports are resolved as each
	      // instance is processed
	      if (ap->m_connectedPort->m_instance < m_utilInstance.m_ordinal &&
		  (strcasecmp(ap->m_connectedPort->m_name.c_str(),
			      c->port->m_name.c_str()) || // port name different
		   assy.utilInstance(ap->m_connectedPort->m_instance).m_specName !=
		   c->impl->m_metadataImpl.specName())) {             // or spec name different
		ocpiInfo("Rejected due to incompatible connection on port \"%s\"",
			 p->m_name.c_str());
		ocpiInfo("Artifact connects it to port '%s' of spec '%s', "
			 "but application wants port '%s' of spec '%s'",
			 c->port->m_name.c_str(), c->impl->m_metadataImpl.specName().c_str(),
			 ap->m_connectedPort->m_name.c_str(),
			 assy.utilInstance(ap->m_connectedPort->m_instance).m_specName.c_str());
		return false;
	      }
	      bump = 1;; // An implementation with hardwired connections gets a score bump
	    } else {
	      // There is no connection in the assembly for a statically connected impl port
	      ocpiInfo("Rejected because artifact has port '%s' connected while "
		       "application doesn't mention it.", p->m_name.c_str());
	      return false;
	    }
	  }
	score += bump;
      }
      if (m_utilInstance.m_hasSlave && i.m_metadataImpl.slave().empty()) {
	ocpiInfo("Rejected because instance is a proxy, but implementation isn't");
	return false;
      }
      // FIXME:  Check consistency between implementation metadata here...
      m_candidates.push_back(Candidate(i, score));
      ocpiInfo("Accepted implementation with score %u", score);
      return true;
    }

    // A common method used by constructors
    void Assembly::findImplementations(const OU::PValue *params) {
      const char *err;
      if ((err = checkInstanceParams("model", params)) ||
	  (err = checkInstanceParams("platform", params)))
	throw OU::Error("%s", err);
      m_params = params; // for access by callback
      m_maxCandidates = 0;
      // Initialize our instances list from the Util assy, but we might add to it for slaves
      for (unsigned n = 0; n < nUtilInstances(); n++)
	m_instances.push_back(new Instance(utilInstance(n)));
      for (InstancesIter ii = m_instances.begin(); ii != m_instances.end(); ii++) {
	m_tempInstance = *ii;
	OU::Assembly::Instance &inst = m_tempInstance->m_utilInstance;
	// need to deal with params that can filter impls: model and platform
	ezxml_t x = inst.xml();
	if (!OU::findAssign(params, "model", inst.m_name.c_str(), m_model))
	  OE::getOptionalString(x, m_model, "model");
	if (!OU::findAssign(params, "platform", inst.m_name.c_str(), m_platform))
	  OE::getOptionalString(x, m_platform, "platform");
	if (!Manager::findImplementations(*this, inst.m_specName.c_str()))
	  throw OU::Error("No acceptable implementations found in any libraries "
			  "for \"%s\".  Use log level 8 for more detail.",
			  inst.m_specName.c_str());
	if (m_tempInstance->m_candidates.size() > m_maxCandidates)
	  m_maxCandidates = (unsigned)m_tempInstance->m_candidates.size();
      }
      // Check for interface and connection compatibility.
      // We assume all implementations have the same protocol metadata
      //      unsigned nConns = m_connections.size();
      for (OU::Assembly::ConnectionsIter ci = m_connections.begin();
	   ci != m_connections.end(); ci++) {
	const OU::Assembly::Connection &c = *ci;
	if (c.m_ports.size() == 2) {
	  const OU::Worker // implementations on both sides of the connection
	    &i0 = m_instances[c.m_ports.front().m_instance]->m_candidates[0].impl->m_metadataImpl,
	    &i1 = m_instances[c.m_ports.back().m_instance]->m_candidates[0].impl->m_metadataImpl;
	  OU::Port // ports on both sides of the connection
	    *ap0 = i0.findMetaPort(c.m_ports.front().m_name),
	    *ap1 = i1.findMetaPort(c.m_ports.back().m_name);
	  if (!ap0 || !ap1)
	    throw OU::Error("Port name (\"%s\") in connection does not match any port in implementation",
			    (ap0 ? c.m_ports.back() : c.m_ports.front()).m_name.c_str());
	  if (ap0->m_provider == ap1->m_provider)
	    throw OU::Error("Port roles (ports \"%s\" and \"%s\") in connection are incompatible",
			    ap0->m_name.c_str(), ap1->m_name.c_str());
	  // Protocol on both sides of the connection
	  OU::Protocol &p0 = *ap0, &p1 = *ap1;
	  if (p0.m_name.size() && p1.m_name.size() && p0.m_name != p1.m_name)
	    throw OU::Error("Protocols in connection are incompatible: "
			    "port \"%s\" of instance \"%s\" has protocol \%s\" vs. "
			    "port \"%s\" of instance \"%s\" has protocol \"%s\"",
			    ap0->m_name.c_str(),
			    utilInstance(c.m_ports.front().m_instance).m_name.c_str(),
			    p0.m_name.c_str(),
			    ap1->m_name.c_str(),
			    utilInstance(c.m_ports.back().m_instance).m_name.c_str(),
			    p1.m_name.c_str());
	  
	  // FIXME:  more robust naming, namespacing, UUIDs, hash etc.
	    
	}
      }
    }
    // A port is connected in the assembly, and the port it is connected to is on an instance with
    // an already chosen implementation. Now we can check whether this impl conflicts with that one
    // or not
    bool Assembly::
    badConnection(const Implementation &impl, const Implementation &otherImpl,
		  const OU::Assembly::Port &ap, unsigned port) {
      const OU::Port
	&p = impl.m_metadataImpl.port(port),
	&other = *otherImpl.m_metadataImpl.findMetaPort(ap.m_connectedPort->m_name);
      if (impl.m_internals & (1 << port)) {
	// This port is preconnected and the other port is not preconnected to us: we're incompatible
	if (!(otherImpl.m_internals & (1 << other.m_ordinal)) ||
	    otherImpl.m_connections[other.m_ordinal].impl != &impl ||
	    otherImpl.m_connections[other.m_ordinal].port != &p) {
	ocpiDebug("other %p %u %s  m_internals %x, other internals %x", &other, other.m_ordinal, other.m_name.c_str(),
		  impl.m_internals, otherImpl.m_internals);
	ocpiDebug("me %p %u %s", &p, p.m_ordinal, p.m_name.c_str());
	return true;
	}
      } else if (otherImpl.m_internals & (1 << other.m_ordinal))
	// This port is external.  If the other port is connected, we're incompatible
	return true;
      return false;
    }
    Assembly::Instance::
    Instance(OU::Assembly::Instance &utilInstance, Instance *master) 
      : m_utilInstance(utilInstance), m_assyPorts(NULL), m_nPorts(0), m_master(master) {
      // m_assyPorts will be initialized based on first impl found
    }
    Assembly::Instance::
    ~Instance()  {
      delete [] m_assyPorts;
    }
  }
}
