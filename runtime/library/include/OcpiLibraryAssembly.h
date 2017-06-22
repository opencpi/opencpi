
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
      inline Candidate(const Implementation &a_impl, unsigned a_score)
	: impl(&a_impl), score(a_score) {}
    };
    typedef std::vector<Candidate> Candidates;   // a set of candidates for an instance
    typedef Candidates::iterator CandidatesIter;

    // A library::assembly adds value to the underlying/inherited util::assembly
    // By finding candidate implementations in the available artifact libraries,
    // and perhaps adding proxy slave instances
    class Assembly : public OCPI::Util::Assembly,  public ImplementationCallback {
      // This class is the library layer's instance.
      // It usually just references the OU::Assembly instance, but in the case of
      // proxies, the library layer may add instances for slaves.
    public:
      struct Instance {
	OCPI::Util::Assembly::Instance &m_utilInstance; // lower level assy instance structure
	OCPI::Util::Assembly::Port **m_assyPorts;       // map impl port ordinal to OU assy port
                                                	// we do the map based on the first impl
	Candidates m_candidates;                        // The candidate impls for this instance
	unsigned m_nPorts;
	Instance *m_master;                             // The master if this is a slave
	Instance(OCPI::Util::Assembly::Instance &utilInstance, Instance *master = NULL);
	~Instance();
	
	bool resolveUtilPorts(const Implementation &i, OCPI::Util::Assembly &a);
	bool checkConnectivity(Candidate &c, Assembly &assy);
	bool foundImplementation(const Implementation &i, std::string &model,
				 std::string &platform);
        void strip_pf(std::string&) const;
	const std::string &name() const { return m_utilInstance.m_name; }
	const std::string &specName() const { return m_utilInstance.m_specName; }
	const OCPI::Util::Assembly::Properties &properties() const {
	  return m_utilInstance.m_properties;
	}
      };
    private:
      typedef std::vector<Instance *> Instances;
      typedef Instances::iterator InstancesIter;
      std::string m_model;                        // used during implementation processing
      std::string m_platform;                     // ditto
      const PValue *m_params;                     // params of assembly during parsing (not copied)
      unsigned m_maxCandidates;                   // maximum number of candidates for any instance
      Instance *m_tempInstance;                   // our instance currently being processed
      Instances m_instances;                      // This layer's instances
      bool      m_deployed;                       // deployment decisions are already made
    public:
      explicit Assembly(const char *file, const OCPI::Util::PValue *params);
      explicit Assembly(const std::string &string, const OCPI::Util::PValue *params);
      explicit Assembly(ezxml_t xml, const char *name, const OCPI::Util::PValue *params);
      ~Assembly();
      Instance &instance(size_t n) { return *m_instances[n]; }
      size_t nInstances() { return m_instances.size(); }
      bool badConnection(const Implementation &impl, const Implementation &otherImpl,
			 const OCPI::Util::Assembly::Port &ap, unsigned port);
      Port *assyPort(unsigned inst, unsigned port) {
	assert(m_instances[inst]->m_assyPorts);
	return m_instances[inst]->m_assyPorts[port];
    }
      // Reference counting
      void operator ++( int );
      void operator --( int );
      
      const char *getPortAssignment(const char *pName, const char *assign, unsigned &instn,
				    unsigned &portn, const OCPI::Util::Port *&port,
				    const char *&value);
    private:
      void addInstance(const OCPI::Util::PValue *params);
      const char *addFileIoInstances(const OCPI::Util::PValue *params);
      void findImplementations(const OCPI::Util::PValue *params);
      bool foundImplementation(const Implementation &i, bool &accepted);
      int m_refCount;
    };
  }
}
#endif
