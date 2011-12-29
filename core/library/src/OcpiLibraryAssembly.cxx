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
    Assembly::Assembly(const char *file)
      : OU::Assembly(file) {
      findImplementations();
    }
    Assembly::Assembly(const std::string &string)
      : OU::Assembly(string) {
      findImplementations();
    }
    Assembly::~Assembly() {
      delete [] m_candidates;
    }
    //The callback for the findImplementations() method below.
    bool Assembly::foundImplementation(const Implementation &i, unsigned score) {
      m_tempCandidates->push_back(Candidate(i, score));
      return false;
    }
    // A common method used by constructors
    void Assembly::findImplementations() {
      m_tempCandidates = m_candidates = new Candidates[m_instances.size()];
      Instance *i = &m_instances[0];
      for (unsigned n = 0; n < m_instances.size(); n++, m_tempCandidates++, i++)
	if (!Manager::findImplementations(*this, i->m_specName.c_str(),
					  i->m_selection.empty() ? NULL : i->m_selection.c_str()))
	  throw OU::Error(i->m_selection.empty() ?
			  "No implementations found in any libraries for \"%s\"" :
			  "No acceptable implementations found in any libraries "
			  "for \"%s\" (for selection: \"%s\")",
			  i->m_specName.c_str(), i->m_selection.c_str());

      // Check for interface compatibility.
      // We assume all implementations have the same protocol metadata
      unsigned nConns = m_connections.size();
      for (unsigned n = 0; n < nConns; n++) {
	const OU::Assembly::Connection &c = m_connections[n];
	if (c.m_ports.size() == 2) {
	  const OU::Implementation // implementations on both sides of the connection
	    &i0 = m_candidates[c.m_ports[0].m_instance][0].impl->m_metadataImpl,
	    &i1 = m_candidates[c.m_ports[1].m_instance][0].impl->m_metadataImpl;
	  OU::Port // ports on both sides of the connection
	    *ap0 = i0.findPort(c.m_ports[0].m_name),
	    *ap1 = i1.findPort(c.m_ports[1].m_name);
	  if (!ap0 || !ap1)
	    throw OU::Error("Port name (\"%s\") in connection does not match any port in implementation",
			    (ap0 ? c.m_ports[1] : c.m_ports[0]).m_name.c_str());
	  if (ap0->m_provider == ap1->m_provider)
	    throw OU::Error("Port roles (ports \"%s\" and \"%s\") in connection are incompatible",
			    ap0->m_name.c_str(), ap1->m_name.c_str());
	  // Protocol on both sides of the connection
	  OU::Protocol &p0 = *ap0, &p1 = *ap1;
	  if (p0.m_name.size()  && p1.m_name.size() && p0.m_name != p1.m_name)
	    throw OU::Error("Protocols (ports \"%s\" protocol \%s\" vs. port \"%s\" protocol \"%s\") "
			    "in connection are incompatible",
			    p0.m_name.c_str(), p1.m_name.c_str());
	  
	  // FIXME:  more robust naming, namespacing, UUIDs, hash etc.
	    
	}
      }
      
      // Consolidate properties
      
    }
  }
}
