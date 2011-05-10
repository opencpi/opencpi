#ifndef OCPI_LIBRARY_MANAGER_H
#define OCPI_LIBRARY_MANAGER_H
/*
librarymanager: init from environment, but have setPath.
  library: has directory
artifact
  API: given a name, find the list of artifacts that match that name.
  Q: should different implementations of the same component be found in different libraries?  
  I suppose a given library should be tagged as to whether its implementations should be combined with others.
  If the specs had uuids, then you could know whether the different implementations belonged to the same contract.  For now we can rely on specnames.

  SO the tagging can be:  allow other libs to find impls I have
  or allow me to back up up stream sparse libraries.
  The SPD concept is a package of things with the same spec, which sucks because it

  doesn't have the library concept.
  So if our library is simply a bag of artifacts that can be scanned, we could indeed "rehash" the whole search path each time.
  How hard would UUIDs on specs really be?  How about URLs like xml schema or java classes - yes much better.  So an "id" attribute could be good enough.

  So given that each library can offer multiple implementations, the shodow affect could just add mode impls.  Thus there is a sort of tree structure to the artifacts that contain a component, so that two that serve the same purpose are ordered.
That would mean that a hash of all the properties could map to the same bucket.


*/
// This file contains the common definitions for library drivers

#include <map>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilUUID.h"
#include "OcpiDriverManager.h"
#include "OcpiContainerApi.h"
#include "OcpiLibraryApi.h"
namespace OCPI {
  namespace Library {
    using OCPI::Util::Child;
    using OCPI::Util::Parent;
    using OCPI::Util::PValue;

    class Driver;   // base class of all library drivers
    // This structure describes the capabilities of a container
    // as it relates to looking at an artifact's metadata to see if 
    // implementations in the artifact can in fact run on that
    // container.  It is defined here in the library facility (and not
    // in the container facility) since it is indeed a toss-up, but
    // this allows thie library facility to not depend on containers.
    // Eventually we will extended to multiple supported aspects
    // (like multiple compiler versions or something).
    struct Capabilities {
      std::string m_os;
      std::string m_osVersion;
      std::string m_platform;
      std::string m_runtime;
      std::string m_runtimeVersion;
    };

    // Attributes of an artifact
    class Attributes {
    protected:
	std::string
	  m_os, m_osVersion,
	  m_platform,
	  m_tool, m_toolVersion,
	  m_runtime, m_runtimeVersion,;
      // Parse from target string
      void parse(const char *pString);
      // Parse from xml
      void parse(ezxml_t x);
      void validate();
    };
    struct Implementation {
      ezxml_t m_worker;
      ezxml_t m_instance;
      inline Implementation(ezxml_t w, ezxml_t i = NULL)
	: m_worker(w), m_instance(i) {}
    };
    struct Comp {
      inline bool operator() (const char *lhs, const char *rhs) const {
      return strcmp(lhs, rhs) < 0;
      }
    };
    // Note due to xml persistence we don't need strings in the map
    // but this multimap stuff is pretty ugly
    typedef std::multimap<const char *, Implementation, Comp> WorkerMap;
    typedef std::pair<const char *, Implementation> WorkerMapPair;
    typedef WorkerMap::const_iterator WorkerIter;
    typedef std::pair<WorkerIter,WorkerIter> WorkerRange;
    class Artifact : public Attributes {
    protected:
      ezxml_t m_xml;
      WorkerMap m_workers;
      Artifact();
      virtual ~Artifact();
    public:
      void configure(ezxml_t x = NULL);
      // Can this artifact run on something with these capabilities?
      virtual bool meetsRequirements (const Capabilities &caps,
				      const char *impl,
				      const OCPI::API::PValue *props,
				      const OCPI::API::Connection *conns,
				      const char *&inst);
      inline const ezxml_t xml() const { return m_xml; }
      virtual const std::string &name() const = 0;
      virtual Artifact *nextArtifact() = 0;
    };

    // The manager/owner of all library drivers
    extern const char *library;
    class Manager : public OCPI::Driver::ManagerBase<Manager, Driver, library> {
      std::string m_libraryPath;
      friend class OCPI::API::LibraryManager;
      Artifact &getArtifactX(const char *url, const OCPI::API::PValue *props);
      Artifact &findArtifactX(const Capabilities &caps,
			      const char *impl,
			      const OCPI::API::PValue *props,
			      const OCPI::API::Connection *conns,
			      const char *&inst);
      void setPath(const char *);
    protected:
    public:
      Manager();
      std::string &getPath();
      // Ask the library manager to get this artifact.
      // It might be in a library, and if not it needs to be established,
      // in a "pseudo library" for artifacts that are specifically loaded.
      // The properties can be used to affect the establishment, or check 
      // against aspects of artifacts that are already established.
      static Artifact &getArtifact(const char *url,
				   const OCPI::API::PValue *props = 0);
      // Find an artifact for a worker on a container
      // First, container capabilities (in a form established by this
      // library facility), then worker info.
      static Artifact &findArtifact(const Capabilities &caps,
				    const char *impl,
				    const OCPI::API::PValue *props,
				    const OCPI::API::Connection *conns,
				    const char *&inst);
    };
    class Library;
    // This is the base class for all library drivers
    class Driver : public OCPI::Driver::DriverType<Manager,Driver> {
      //      virtual Library *findLibrary(const char *url) = 0;
    protected:
      Driver(const char *);
    public:
      virtual Library *firstLibrary() = 0;
      virtual Artifact *findArtifact(const char *url) = 0;
      // Return NULL if the artifact is not supported by the driver
      virtual Artifact *addArtifact(const char *url,
				    const OCPI::API::PValue *props) = 0;
      Artifact *findArtifact(const Capabilities &caps,
			     const char *impl,
			     const OCPI::API::PValue *props,
			     const OCPI::API::Connection *conns,
			     const char *&inst);
    };
    // This is the template inherited by concrete library drivers
    template <class ConcDri, class ConcreteLibrary, const char *&name>
    class DriverBase :
      public OCPI::Driver::DriverBase
      <Manager, Driver, ConcDri, ConcreteLibrary, name>
    {
      inline Library *firstLibrary() {
	return
	  OCPI::Driver::
	  DriverBase<Manager,Driver,ConcDri,ConcreteLibrary, name>::
	  firstDevice();
      }
#if 0
      virtual Library *findLibrary(const char *url) {
	return Parent<ConcreteLibrary>::findChildByName(url);
      }
#endif
      virtual Artifact *findArtifact(const char *url) {
	for (ConcreteLibrary *l = Parent<ConcreteLibrary>::firstChild();
	     l; l = l->nextChild())
	  return l->findArtifact(url);
	return NULL;
      }
    };
    // This is the base class for all libraries for all drivers.
    class Library {
    public:
      Artifact *
      findArtifact(const Capabilities &caps,
		   const char *impl,
		   const OCPI::API::PValue *props,
		   const OCPI::API::Connection *conns,
		   const char *&inst);
      virtual Artifact *firstArtifact() = 0;
      virtual Library *nextLibrary() = 0;
    };
    // The template class inherited by all concrete libraries.
    template <class Dri, class Lib, class Art>
    class LibraryBase :
      public OCPI::Driver::DeviceBase<Dri,Lib>,
      public OCPI::Util::Parent<Art>,
      public Library
    {
    public:
      inline Artifact *firstArtifact() { return Parent<Art>::firstChild(); }
      inline Library *nextLibrary() {
	return OCPI::Driver::DeviceBase<Dri,Lib>::nextDevice();
      }
      inline Artifact *findArtifact(const char *url) {
	return Parent<Art>::findChildByName(url);
      }
    protected:
      LibraryBase<Dri, Lib, Art>(const char *childName)
      : OCPI::Driver::DeviceBase<Dri, Lib>(childName) {}
    };

    // This is the template inherited by concrete artifacts
    template <class Lib, class Art>
    class ArtifactBase :
      public OCPI::Util::Child<Lib, Art>,
      public Artifact
    {
    protected:
      ArtifactBase<Lib, Art>(Lib &lib, const char *name)
      : OCPI::Util::Child<Lib,Art>(lib, name) {}
    public:
      inline Artifact *nextArtifact() {
	return OCPI::Util::Child<Lib,Art>::nextChild();
      }
      inline const std::string &name() const {
	return OCPI::Util::Child<Lib,Art>::name();
      }
    };
    template <class Dri>
    class RegisterLibraryDriver : public OCPI::Driver::Registration<Dri>{};
  }
}
#endif
