
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
      OCPI::Library::Assembly &m_assembly;

      size_t m_nInstances;
      // This structure is used during deployment planning.
      struct Instance {
	const OCPI::Library::Implementation *m_impl; // The chosen, best implementation
	unsigned m_container;                        // LOCAL ordinal - among those we are using
	CMap *m_feasibleContainers;                  // map per candidate, from findContainers
	size_t m_nCandidates;                        // convenience
	Instance();
	~Instance();
      } *m_instances;
      // The instance objects for the launcher
      OCPI::Container::Launcher::Instances m_launchInstances;
#if 0
      // State of a connection in the application as it comes into being
      // For now, only support one-to-one connections
      struct Connection {
	OCPI::Util::Assembly::Connection &m_assyConnection;     // reference to xml conn
	OCPI::Util::Assembly::Port *m_assyInput, *m_assyOutput; // null if external
	OCPI::Util::Assembly::External *m_external;             // input or output
	Port *m_input, *m_output;                               // ports or null
	
	std::string m_ipi, m_fpi, m_iui, m_fui;
	// Other status?
	Connection(OCPI::Util::Assembly::Connection &c);
      };
      typedef std::list<Connection> Connections;
      typedef Connections::iterator ConnectionsIter;
      Connections m_connections;
#endif
      OCPI::Container::Launcher::Connections m_launchConnections;
      struct Booking {
	OCPI::Library::Artifact *m_artifact;
	CMap m_usedImpls;         // which fixed implementations in the artifact are in use
	Booking() : m_artifact(NULL), m_usedImpls(0) {}
      } *m_bookings;

      struct Deployment {
	unsigned candidate;            // running counter of where we are in candidates list
	unsigned container;            // running counters of where we are in containers for candidate
      } *m_deployments, *m_bestDeployments;
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
      // External ports - recorded until we know whether it will be a ExternalPort, or a remote port
      struct External {
	Port &m_port; // The internal worker port
	const PValue *m_params; //  Connection parameters from the OU::Assembly
	ExternalPort *m_external; // The external port created from connectExternal.
	inline External(Port &port, const PValue *params)
	  : m_port(port), m_params(params), m_external(NULL) {}
      };
      typedef std::map<const char*, External, OCPI::Util::ConstCharComp> Externals;
      typedef std::pair<const char*, External> ExternalPair;
      Externals m_externals;
      //      OCPI::Container::Worker **m_workers;
      OCPI::Container::Worker *m_doneWorker;
      // OCPI::Container::ExternalPort **m_externalPorts;
      // const char **m_externalNames;

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
      void initConnections();
      void initInstances();
      // return our used-container ordinal
      unsigned addContainer(unsigned container);
      bool connectionsOk(OCPI::Library::Candidate &c, unsigned instNum);
      void finalizeProperties(const OCPI::Util::PValue *params);
      bool bookingOk(Booking &b, OCPI::Library::Candidate &c, unsigned n);
      //      CMap assessCandidate(Instance &i, OCPI::Library::Candidate &c, unsigned &backUp);
      void policyMap( Instance * i, CMap & bestMap);
      void setPolicy(const OCPI::API::PValue *params);
      Property &findProperty(const char * worker_inst_name, const char * prop_name);
      void dumpDeployment(unsigned score, Deployment *dep);
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
      Worker &getPropertyWorker(const char *name, const char *&pname);
      bool getProperty(unsigned ordinal, std::string &name, std::string &value, bool hex,
		       bool *parp, bool *cachedp, bool uncached);
      void getProperty(const char * wname, const char * pname, std::string &value, bool hex);
      void setProperty(const char* worker_name, const char* prop_name, const char *value);
      // This will be used for the port connection protocol
      OCPI::Container::Port &getRemote(const char *);
    };
  }
}
#endif
