
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
      const Implementation *impl; unsigned score;
      inline Candidate(const Implementation &impl, unsigned score) : impl(&impl), score(score) {}
    };
    typedef std::vector<Candidate> Candidates;   // a set of candidates for an instance

    class Assembly : public OCPI::Util::Assembly,  public ImplementationCallback {
      Candidates *m_tempCandidates;              // set currently being processed (for one instance)
    public:
      Candidates *m_candidates;                  // array of sets indexed by implementation ordinal
      explicit Assembly(const char *file);
      explicit Assembly(const std::string &string);
      ~Assembly();
    private:
      void findImplementations();
      bool foundImplementation(const Implementation &i, unsigned score);
    };
  }
}
#endif
