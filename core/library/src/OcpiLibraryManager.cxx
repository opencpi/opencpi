#include <string.h>
#include <dirent.h>
#include "OcpiOsFileIterator.h"
#include "OcpiUtilException.h"
#include "OcpiContainerErrorCodes.h"
#include "OcpiLibraryManager.h"

// This file contains code common to all library drivers

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OC = OCPI::Container; // only for error codes...
namespace OD = OCPI::Driver;

namespace OCPI {
  namespace Library {
    const char *library = "library";

    // The Library Driver Manager class
    Manager::Manager() {
    }
    void Manager::setPath(const char *path) {
      parent().configureOnce();
      m_libraryPath = path;
    }
    std::string &Manager::getPath() {
      parent().configureOnce();
      return m_libraryPath;
    }
    // The high level entry point to search all libraries in search of
    // an artifact that will support the worker and the offered capabilities
    Artifact &Manager::
    findArtifact(const Capabilities &caps, const char *impl,
		 const OCPI::API::PValue *props,
		 const OCPI::API::Connection *conns,
		 const char *&inst) {
      return getSingleton().findArtifactX(caps, impl, props, conns, inst);
    }
    Artifact &Manager::
    findArtifactX(const Capabilities &caps, const char *impl,
		  const OCPI::API::PValue *props,
		  const OCPI::API::Connection *conns,
		  const char *&inst) {
      parent().configureOnce();
      // If some driver already has it in one of its libraries, return it.
      Artifact *a;
      for (Driver *d = firstDriver(); d; d = d->nextDriver())
	if ((a = d->findArtifact(caps, impl, props, conns, inst)))
	  return *a;
      throw OU::EmbeddedException(OC::NO_ARTIFACT_FOR_WORKER,
				  "No artifact found for worker",
				  OC::ApplicationRecoverable);
    }

    Artifact &Manager::getArtifact(const char *url, const OA::PValue *props) {
      return getSingleton().getArtifactX(url, props);
    }

    Artifact &Manager::getArtifactX(const char *url, const OA::PValue *props) {
      parent().configureOnce();
      OCPI::Library::Driver *d;
      Artifact *a;
      // If some driver already has it in one of its libraries, return it.
      for (d = firstDriver(); d; d = d->nextDriver())
	if ((a = d->findArtifact(url)))
	  return *a;
      // The artifact was not found in any driver's libraries
      // Now we need to find a library driver that can deal with this
      // artifact. In this case a driver returning NULL means the driver is
      // passing on the artifact.  If there is an error dealing with the
      // artifact, that will throw an exception.
      for (Driver *d = firstChild(); d; d = d->nextChild())
	if ((a = d->addArtifact(url, props)))
	  return *a;
      throw OU::EmbeddedException(OC::ARTIFACT_UNSUPPORTED,
				  "No library driver supports this file",
				  OC::ApplicationRecoverable);
    }


    // Libraries can be specified in the environment
    // We will be given "librarydrivers"
#if 0
    void Manager::configure(ezxml_t x) {
      ezxml_t lib = NULL;
      if (x) {
	for (lib = ezxml_cchild(x, "library"); lib; lib = ezxml_next(lib)) {
	  const char *name = ezxml_cattr(lib, "name");
	  if (!name)
	    throw ApiError("Missing 'name' attribute for 'library' in system configuration", NULL);
	  (new Library(name))->configure(lib);
	}
      }
    }
#endif
    Driver::
    Driver(const char *name)
      : OD::DriverType<Manager,Driver>(name) {
    }
    Artifact *Driver::
    findArtifact(const Capabilities &caps,
		 const char *impl,
		 const OCPI::API::PValue *props,
		 const OCPI::API::Connection *conns,
		 const char *&inst) {
      for (Library *l = firstLibrary(); l; l = l->nextLibrary())
	return l->findArtifact(caps, impl, props, conns, inst);
      return NULL;
    }

    Library::~Library(){}
    Artifact * Library::
    findArtifact(const Capabilities &caps,
		 const char *impl,
		 const OCPI::API::PValue *props,
		 const OCPI::API::Connection *conns,
		 const char *&inst) {
      for (Artifact *a = firstArtifact(); a; a = a->nextArtifact())
	if (a->meetsRequirements(caps, impl, props, conns, inst))
	  return a;
      return NULL;
    }

    // The artifact base class
    Artifact::Artifact() : m_xml(NULL) {}
    Artifact::~Artifact(){}
    bool Artifact::
    meetsRequirements (const Capabilities &caps,
		       const char *impl,
		       const OCPI::API::PValue *props,
		       const OCPI::API::Connection *conns,
		       const char *&inst) {
      if (m_os == caps.m_os && m_platform == caps.m_platform) {
	WorkerRange range = m_workers.equal_range(impl);
	for (WorkerIter wi = range.first; wi != range.second; wi++) {
	  const Implementation &i = (*wi).second;
	  // FIXME: more complex comparison for FPGAs with connectivity
	  return true;
	}
      }
      return false;
    }
    // Note this XML argument is from the system config file, not the
    // XML attached to this artifact file.
    // But in any case we process the attached metadata here, and not
    // in the constructor, for consistency with the Manager/Driver/Device model
    // Someday we might AUGMENT/OVERRIDE the attached metadata with the
    // system configuration metadata
    void Artifact::configure(ezxml_t x) {
      // Retrieve attributes from artifact xml
      Attributes::parse(m_xml);
      // First insert workers, and then
      for (ezxml_t w = ezxml_cchild(m_xml, "worker"); w; w = ezxml_next(w)) {
	const char *name = ezxml_cattr(w, "name");
	bool instances = false;
	for (ezxml_t i = ezxml_cchild(m_xml, "instance"); i; i = ezxml_next(i))
	  if (!strcmp(name, ezxml_cattr(i, "worker"))) {
	    instances = true;
	    m_workers.insert(WorkerMapPair(name,Implementation(w,i)));
	  }
	if (!instances)
	  m_workers.insert(WorkerMapPair(name,Implementation(w)));
      }
    }
    void parse3(char *s, std::string &s1, std::string &s2,
		std::string &s3) {
      char *temp = strdup(s);
      if ((s = strsep(&temp, "-"))) {
	s1 = s;
	if ((s = strsep(&temp, "-"))) {
	  s2 = s;
	  if ((s = strsep(&temp, "-")))
	    s3 = s;
	}
      }
      free(temp);
    }
    void Attributes::parse(ezxml_t x) {
      const char *cp;
      if ((cp = ezxml_cattr(x, "os"))) m_os = cp;
      if ((cp = ezxml_cattr(x, "osVersion"))) m_osVersion = cp;
      if ((cp = ezxml_cattr(x, "platform"))) m_platform = cp;
      if ((cp = ezxml_cattr(x, "runtime"))) m_runtime = cp;
      if ((cp = ezxml_cattr(x, "runtimeVersion"))) m_runtimeVersion = cp;
      if ((cp = ezxml_cattr(x, "tool"))) m_tool = cp;
      if ((cp = ezxml_cattr(x, "toolVersion"))) m_toolVersion = cp;
      validate();
    }
    void Attributes::parse(const char *pString) {
      std::string junk;
      char *p = strdup(pString), *temp = p, *val;
      
      if ((val = strsep(&temp, "="))) {
	parse3(val, m_os, m_osVersion, junk);
	if ((val = strsep(&temp, "="))) {
	  parse3(val, m_platform, junk, junk);
	  if ((val = strsep(&temp, "="))) {
	    parse3(val, m_tool, m_toolVersion, junk);
	    if ((val = strsep(&temp, "=")))
	      parse3(val, m_runtime, m_runtimeVersion, junk);
	  }
	}
      }
      free(p);
      validate();
    }
    void Attributes::validate() { }
  }
  namespace API {
    void LibraryManager::
    setPath(const char *path) {
      OCPI::Library::Manager::getSingleton().setPath(path);
    }
    std::string LibraryManager::
    getPath() {
      return OCPI::Library::Manager::getSingleton().getPath();
    }
    
  }
}
