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
 * Definitions for the top level application, whose workers are executed on a
 * variety of containers.  The "Application" object is a runtime thing that is an instance of
 * an assembly.  Thus the assembly is the template for the application.
 *
 * This file is NOT an exposed API file.
 */
#ifndef OCPI_APPLICATION_H
#define OCPI_APPLICATION_H

#include <string>
#include <map>
#include "OcpiUtilMisc.h"
#include "OcpiLibraryAssembly.h"
#include "ContainerManager.h"
#include "ContainerApplication.h"
#include "ContainerLauncher.h"
#include "OcpiApplicationApi.h"

namespace OCPI {
  namespace Container {
    class Launcher;
    class LocalLauncher;
    class RemoteLauncher;
  }
  namespace API {

    class ApplicationI : public OCPI::Container::Callback {
      typedef OCPI::Container::Container::CMap CMap;
      struct Deployment {
	size_t m_scale;                                // scale of this deployment
	unsigned m_container;                          // when scale==1, the chosen one
	const OCPI::Library::Implementation *m_impl;   // implementation on each container
	unsigned *m_containers;                        // containers used by this deployment
	const OCPI::Library::Implementation **m_impls; // implementation on each container
	CMap m_feasible;
	Deployment();
	~Deployment();
	Deployment &operator=(const Deployment &d);
	void set(size_t scale, unsigned *containers,
		 const OCPI::Library::Implementation **impls, CMap map);
      };
      // This structure is used during deployment planning.
      struct Instance {
	Deployment m_deployment;      // Current deployent when generating all of them
	Deployment m_bestDeployment;
	CMap *m_feasibleContainers;   // map per candidate, from findContainers
	size_t m_nCandidates;         // convenience
	// The launcher info for this application instance;
	OCPI::Container::Launcher::Crew m_crew;
	// Support for collecting candidate together that have the same actual implementation
	// code even though they may be compiled for different targets.
	// map is from the qualified impl name to a list of candidate indices.
	typedef std::list<unsigned> Candidates;
	typedef Candidates::iterator CandidatesIter;
	typedef std::map<std::string, Candidates> ScalableCandidates;
	typedef ScalableCandidates::value_type ScalablePair;
	typedef ScalableCandidates::iterator ScalableCandidatesIter;
	ScalableCandidates m_scalableCandidates;
	// This array is of indices in the usedcontainer array, not global indices
	unsigned m_usedContainer;       // when scale==1 this is it
	unsigned *m_usedContainers;     // container for each crew member
	unsigned m_firstMember;         // index of first member in launch members
	void collectCandidate(OCPI::Library::Candidate &c, unsigned n);
	void finalizePortParam(OU::Assembly::Instance &ui, const OCPI::Util::PValue *params,
			       const char *param);
	Instance();
	~Instance();
      };
      struct Booking {
	OCPI::Library::Artifact *m_artifact;
	CMap m_usedImpls;         // which fixed implementations in the artifact are in use
	Booking() : m_artifact(NULL), m_usedImpls(0) {}
      };
      OCPI::Library::Assembly &m_assembly;
      size_t m_nInstances;
      Instance *m_instances;
      // The instance objects for the launcher
      OCPI::Container::Launcher::Members m_launchMembers;
      OCPI::Container::Launcher::Connections m_launchConnections;
      Booking *m_bookings;
      // This class represents a mapping from an externally visible property of the assembly
      // to an individual property of an instance. It must be at this layer
      // (not util::assembly or library::assembly) because it potentially depends on the 
      // implementation specific properties of the implementations selected.
      struct Property {
	std::string m_name;     // qualified name (instance[.:=]property) or mapped name
	unsigned m_instance;    // ordinal of instance in assembly
	unsigned m_property;    // ordinal of property in implememtation of instance
	const char *m_dumpFile; // pointer to dump file if one was specified
      } *m_properties;
      size_t m_nProperties;
      CMap m_curMap;              // A temporary that indicates possible containers for a candidate
      unsigned m_curContainers;   // A temporary that counts containers for a candidate
      CMap m_allMap;              // A map of all containers chosen/used
      unsigned *m_global2used;         // A map from global ordinals to our "used" orinals
      unsigned m_nContainers;     // how many containers have been used
      unsigned *m_usedContainers; // A map from used container to global container

      // Now the runtime state.
      OCPI::Container::Container **m_containers;      // the actual containers we are using
      OCPI::Container::Application **m_containerApps; // per used container, the container app

      // This structure captures info to allow the connection to be made AFTER launch.
      // It is basically a map from the name of the external assembly port to the
      // launcher connection that includes that external assembly port.
      struct External {
	OCPI::Container::Launcher::Connection &m_connection;
	OCPI::Container::LocalPort *m_external; // created (later) from connectExternal.
	inline External(OCPI::Container::Launcher::Connection &conn)
	  : m_connection(conn), m_external(NULL) {}
      };
      typedef std::map<const char*, External, OCPI::Util::ConstCharComp> Externals;
      typedef std::pair<const char*, External> ExternalPair;
      Externals m_externals;
      Instance *m_doneInstance;

      enum CMapPolicy {
	RoundRobin,
	MinProcessors,
	MaxProcessors
      };
      CMapPolicy m_cMapPolicy;
      unsigned   m_processors;
      unsigned m_currConn;
      unsigned m_bestScore;
      bool m_hex;
      bool m_launched;
      Application &m_apiApplication;

      void init(const OCPI::API::PValue *params);
      void initExternals(const OCPI::API::PValue *params);
      void setLaunchPort(OCPI::Container::Launcher::Port &p, const OCPI::Util::Port *mp,
			 const OCPI::Util::PValue *connParams, const std::string &name,
			 const OCPI::Util::PValue *portParams,
			 const OCPI::Container::Launcher::Member *member,
			 const OCPI::Util::Assembly::External *ep, size_t scale, size_t index);
      void initLaunchConnections();
      void initLaunchMembers();
      // return our used-container ordinal
      unsigned addContainer(unsigned container);
      unsigned getUsedContainer(unsigned container);
      bool connectionsOk(OCPI::Library::Candidate &c, unsigned instNum);
      void finalizeProperties(const OCPI::Util::PValue *params);
      void finalizePorts(const OCPI::Util::PValue *params);
      void finalizeExternals();
      bool bookingOk(Booking &b, OCPI::Library::Candidate &c, unsigned n);
      //      CMap assessCandidate(Instance &i, OCPI::Library::Candidate &c, unsigned &backUp);
      void policyMap( Instance * i, CMap & bestMap);
      void setPolicy(const OCPI::API::PValue *params);
      Property &findProperty(const char * worker_inst_name, const char * prop_name);
      void dumpDeployment(unsigned score);
      void doScaledInstance(unsigned instNum, unsigned score);
      void deployInstance(unsigned instNum, unsigned score, size_t scale,
			  unsigned *containers, const OCPI::Library::Implementation **impls,
			  CMap feasible);
      void doInstance(unsigned instNum, unsigned score);
      //      void checkInstanceParams(const char *pName, const OCPI::Util::PValue *params,
      //			       bool checkMapped = false);
      void checkExternalParams(const char *pName, const OCPI::Util::PValue *params);
      void prepareInstanceProperties(unsigned nInstance, const OCPI::Library::Implementation &impl,
				     unsigned *&pn, OCPI::Util::Value *&pv);
    public:
      explicit ApplicationI(OCPI::API::Application &app, const char *file, const OCPI::API::PValue *params = NULL);
      explicit ApplicationI(OCPI::API::Application &app, const std::string &string, const OCPI::API::PValue *params = NULL);
      explicit ApplicationI(OCPI::API::Application &app, OCPI::Library::Assembly &, const OCPI::API::PValue *params = NULL);
      ~ApplicationI();
      OCPI::Library::Assembly & assembly() { return m_assembly; }
      bool foundContainer(OCPI::Container::Container &i);
#if 0
      OCPI::Container::Worker &createWorker(const Instance &i, OCPI::Container::Worker *slave,
					    bool remote = false);
#endif
      void initialize();
      void start();
      void stop();
      bool wait(OCPI::OS::Timer *);
      void finish();
      ExternalPort &getPort(const char *, const OCPI::API::PValue *);
      friend struct Property;
      Worker &getPropertyWorker(const char *name);
      bool getProperty(unsigned ordinal, std::string &name, std::string &value, bool hex, bool *parp);
      void getProperty(const char * wname, const char * pname, std::string &value, bool hex);
      void setProperty(const char* worker_name, const char* prop_name, const char *value);
      // This will be used for the port connection protocol
      OCPI::Container::Port &getRemote(const char *);
    };
  }
}
#endif
