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
#include "OcpiLibraryAssembly.h"

namespace OCPI {
  namespace Library {
    namespace OU = OCPI::Util;
    // Attributes specific to an application assembly
    static const char *assyAttrs[] = { "maxprocessors", "minprocessors", "roundrobin", "done", NULL};
    Assembly::Assembly(const char *file, const OCPI::Util::PValue *params)
      : OU::Assembly(file, assyAttrs), m_refCount(1) {
      findImplementations(params);
    }
    Assembly::Assembly(const std::string &string, const OCPI::Util::PValue *params)
      : OU::Assembly(string, assyAttrs), m_refCount(1) {
      findImplementations(params);
    }
    Assembly::~Assembly() {
      delete [] m_candidates;
      if (m_assyPorts)
	for (unsigned n = 0; n < m_instances.size(); n++)
	  delete [] m_assyPorts[n];
      delete [] m_assyPorts;
    }

    void
    Assembly::
    operator ++( int )
    {
      m_refCount++;
    }

    void
    Assembly::
    operator --( int )
    {
      if ( --m_refCount == 0 ) {
	delete this;
      }
    }

    // The one-time action for the first implementation for an instance
    void Assembly::
    resolvePorts(const Implementation &i) {
      OU::Port *ports = i.m_metadataImpl.ports(m_nPorts);
      OU::Assembly::Port **ap = m_assyPorts[m_instance] = new OU::Assembly::Port *[m_nPorts];
      for (unsigned n = 0; n < m_nPorts; n++)
	ap[n] = NULL;
      // build the map from implementation port ordinals to util::assembly::ports
      OU::Assembly::Instance &inst = m_instances[m_instance];
      for (std::list<OU::Assembly::Port*>::const_iterator pi = inst.m_ports.begin(); 
	   pi != inst.m_ports.end(); pi++) {
	OU::Port *p = ports;
	bool found = false;
	if ((*pi)->m_name.empty()) {
	  // Resolve empty port names to be unambiguous if possible
	  for (unsigned n = 0; n < m_nPorts; n++, p++)
	    if ((*pi)->m_role.m_provider && p->m_provider || !(*pi)->m_role.m_provider && !p->m_provider) {
	      if (found)
		  throw OU::Error("The '%s' connection at instance '%s' is ambiguous: "
				  " Port name must be specified.",
				  (*pi)->m_role.m_provider ? "input" : "output",
				  m_instances[m_instance].m_name.c_str());
	      // This is a mutable member of a const object.
	      (*pi)->m_name = p->m_name;
	      ap[n] = *pi;
	      found = true;
	    }
	  if (!found)
	    throw OU::Error("There is no %s port for connection at instance '%s'.",
			    (*pi)->m_role.m_provider ? "input" : "output",
			    m_instances[m_instance].m_name.c_str());
	} else {
	  for (unsigned n = 0; n < m_nPorts; n++, p++)
	    if (ports[n].m_name == (*pi)->m_name) {
	      ap[n] = *pi;
	      found = true;
	      break;
	    }
	  if (!found)
	    throw OU::Error("Assembly instance '%s' of worker '%s' has no port named '%s'",
			    inst.m_name.c_str(), i.m_metadataImpl.specName().c_str(),
			    (*pi)->m_name.c_str());
	}
      }
    }


    // The callback for the findImplementations() method below.
    // Return true if we found THE ONE.
    // Set accepted = true, if we actually accepted one
    bool Assembly::
    foundImplementation(const Implementation &i, unsigned score, bool &accepted) {
      ocpiDebug("Considering implementation for instance \"%s\" with spec \"%s\": \"%s%s%s\" "
		"from artifact \"%s\"",
		m_instances[m_instance].m_name.c_str(),
		i.m_metadataImpl.specName().c_str(),
		i.m_metadataImpl.name().c_str(),
		i.m_staticInstance ? "/" : "",
		i.m_staticInstance ? ezxml_cattr(i.m_staticInstance, "name") : "",
		i.m_artifact.name().c_str());
      // The util::assembly only knows port names, not worker port ordinals
      // (because it has not been correlated with any implementations).
      // Here is where we process the matchup between the port names in the util::assembly
      // and the port names in implementations in the libraries
      // This test works even when there are no ports
      if (m_assyPorts[m_instance]) {
	// We have processed an implementation before, just check for consistency
	if (i.m_metadataImpl.nPorts() != m_nPorts) {
	  ocpiInfo("Port number mismatch (%u vs %u) between implementations of worker %s.",
		   i.m_metadataImpl.nPorts(), m_nPorts, i.m_metadataImpl.specName().c_str());
	  return false;
	}
      } else
	// First time for an implementation for this instance
	resolvePorts(i);
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
	    OU::Assembly::Port *ap = m_assyPorts[m_instance][n];
	    if (ap) {
	      // We found the assembly connection port
	      // Now check that the port connected in the assembly has the same
	      // name as the port connected in the artifact
	      if (!ap->m_connectedPort) {
		ocpiDebug("Rejected implementation because artifact has port '%s' connected while "
			  "application doesn't.", p->m_name.c_str());
		return false;
	      }
	      // This check can only be made for the port of the internal connection that is
	      // for a later instance, since null-named ports are resolved as each
	      // instance is processed
	      if (ap->m_connectedPort->m_instance < m_instance &&
		  (ap->m_connectedPort->m_name != c->port->m_name || // port name different
		   m_instances[ap->m_connectedPort->m_instance].m_specName !=
		   c->impl->m_metadataImpl.specName())) {             // or spec name different
		ocpiDebug("Rejected implementation due to incompatible connection on port \"%s\"",
			  p->m_name.c_str());
		ocpiDebug("Artifact connects it to port '%s' of spec '%s', "
			  "but application wants port '%s' of spec '%s'",
			  c->port->m_name.c_str(), c->impl->m_metadataImpl.specName().c_str(),
			  ap->m_connectedPort->m_name.c_str(),
			  m_instances[ap->m_connectedPort->m_instance].m_specName.c_str());
		return false;
	      }
	      bump = 1;; // An implementation with hardwired connections gets a score bump
	    } else {
	      // There is no connection in the assembly for a statically connected impl port
	      ocpiDebug("Rejected implementation because artifact has port '%s' connected while "
			"application doesn't mention it.", p->m_name.c_str());
	      return false;
	    }
	  }
	score += bump;
      }
      // FIXME:  Check consistency between implementations here...
      m_tempCandidates->push_back(Candidate(i, score));
      ocpiDebug("Accepted implementation with score %u", score);
      accepted = true;
      return false;
    }

    // A common method used by constructors
    void Assembly::findImplementations(const OU::PValue *params) {
      m_instance = 0;
      m_maxCandidates = 0;
      m_tempCandidates = m_candidates = new Candidates[m_instances.size()];
      m_assyPorts = new OU::Assembly::Port **[m_instances.size()];
      for (unsigned n = 0; n < m_instances.size(); n++, m_tempCandidates++, m_instance++) {
	OU::Assembly::Instance &inst = m_instances[m_instance];
	m_assyPorts[n] = NULL;
	m_nPorts = 0;
	const char *selection = NULL;
	if (!OU::findAssign(params, "selection", inst.m_name.c_str(), selection))
	  selection = inst.m_selection.empty() ? NULL : inst.m_selection.c_str();

	if (!Manager::findImplementations(*this, inst.m_specName.c_str(), selection))
	  throw OU::Error(!selection ?
			  "No implementations found in any libraries for \"%s\"" :
			  "No acceptable implementations found in any libraries "
			  "for \"%s\" (for selection: '%s')",
			  inst.m_specName.c_str(), selection);
	if (m_tempCandidates->size() > m_maxCandidates)
	  m_maxCandidates = (unsigned)m_tempCandidates->size();
      }
      // Check for interface and connection compatibility.
      // We assume all implementations have the same protocol metadata
      //      unsigned nConns = m_connections.size();
      for (OU::Assembly::ConnectionsIter ci = m_connections.begin(); ci != m_connections.end(); ci++) {
	const OU::Assembly::Connection &c = *ci;
	if (c.m_ports.size() == 2) {
	  const OU::Implementation // implementations on both sides of the connection
	    &i0 = m_candidates[c.m_ports.front().m_instance][0].impl->m_metadataImpl,
	    &i1 = m_candidates[c.m_ports.back().m_instance][0].impl->m_metadataImpl;
	  OU::Port // ports on both sides of the connection
	    *ap0 = i0.findPort(c.m_ports.front().m_name),
	    *ap1 = i1.findPort(c.m_ports.back().m_name);
	  if (!ap0 || !ap1)
	    throw OU::Error("Port name (\"%s\") in connection does not match any port in implementation",
			    (ap0 ? c.m_ports.back() : c.m_ports.front()).m_name.c_str());
	  if (ap0->m_provider == ap1->m_provider)
	    throw OU::Error("Port roles (ports \"%s\" and \"%s\") in connection are incompatible",
			    ap0->m_name.c_str(), ap1->m_name.c_str());
	  // Protocol on both sides of the connection
	  OU::Protocol &p0 = *ap0, &p1 = *ap1;
	  if (p0.m_name.size()  && p1.m_name.size() && p0.m_name != p1.m_name)
	    throw OU::Error("Protocols in connection are incompatible: "
			    "port \"%s\" of instance \"%s\" has protocol \%s\" vs. "
			    "port \"%s\" of instance \"%s\" has protocol \"%s\"",
			    ap0->m_name.c_str(), m_instances[c.m_ports.front().m_instance].m_name.c_str(), p0.m_name.c_str(),
			    ap1->m_name.c_str(), m_instances[c.m_ports.back().m_instance].m_name.c_str(), p1.m_name.c_str());
	  
	  // FIXME:  more robust naming, namespacing, UUIDs, hash etc.
	    
	}
      }
      // Consolidate properties
    }
    // A port is connected in the assembly, and the port it is connected to is on an instance with
    // an already chosen implementation. Now we can check whether this impl conflicts with that one
    // or not
    bool Assembly::
    badConnection(const Implementation &impl, const Implementation &otherImpl,
		  const OU::Assembly::Port &ap, unsigned port) {
      const OU::Port
	&p = impl.m_metadataImpl.port(port),
	&other = *otherImpl.m_metadataImpl.findPort(ap.m_connectedPort->m_name);
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
  }
}
