
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


/*
 * Definitions for an assembly that is processed against the available libraries
 * to look for available implementations, and to filter based on selection criteria.
 * This assembly adds the library (and implementation candidates) aspect to the
 * underlying OU::Assembly, which is just about representing the metadata
 * independent of the library aspect of implementations
 */
#ifndef OCPI_LIBRARY_ASSEMBLY_H
#define OCPI_LIBRARY_ASSEMBLY_H

#include <string>
#include "OcpiLibraryManager.h"
#include "OcpiUtilAssembly.h"

namespace OCPI {
  namespace Library {
    // This class represents a candidate implementation for an instance.
    struct Candidate {
      // The impl is not a reference since std::vector must copy it :-(
      const Implementation *impl;
      unsigned score;
      inline Candidate(const Implementation &impl, unsigned score) : impl(&impl), score(score) {}
    };
    typedef std::vector<Candidate> Candidates;   // a set of candidates for an instance

    // A library::assembly adds value to the underlying/inherited util::assembly
    // By finding candidate implementations in the available artifact libraries.
    class Assembly : public OCPI::Util::Assembly,  public ImplementationCallback {
      unsigned m_instance;			  // ordinal of current instance being processed
      unsigned m_nPorts;                          // nPorts of impls of instance being processed
      std::string m_model;                        // used during implementation processing
      std::string m_platform;
      const PValue *m_params;                     // params of assembly during parsing (not copied)
      unsigned m_maxCandidates;                   // maximum number of candidates for any instance
      Candidates *m_tempCandidates;               // candidates currently being processed (for one instance)
      OCPI::Util::Assembly::Port ***m_assyPorts;  // a map by spec port ordinal to util::assembly ports
    public:
      Candidates *m_candidates;                   // array of candidate sets, indexed by instance ordinal
      explicit Assembly(const char *file, const OCPI::Util::PValue *params);
      explicit Assembly(const std::string &string, const OCPI::Util::PValue *params);
      ~Assembly();
      bool badConnection(const Implementation &impl, const Implementation &otherImpl,
			 const OCPI::Util::Assembly::Port &ap, unsigned port);
      inline Port *assyPort(unsigned inst, unsigned port) { return m_assyPorts[inst][port];}
      // Reference counting
      void operator ++( int );
      void operator --( int );
      
    private:
      bool resolvePorts(const Implementation &i);
      void findImplementations(const OCPI::Util::PValue *params);
      bool foundImplementation(const Implementation &i, bool &accepted);
      int m_refCount;
    };
  }
}
#endif
