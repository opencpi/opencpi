#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <climits>
#include "OcpiOsFileIterator.h"
#include "OcpiUtilException.h"
#include "OcpiContainerErrorCodes.h"
#include "OcpiLibraryManager.h"
#include <OcpiOsAssert.h>
//#include <OcpiExpParser.h>

// This file contains code common to all library drivers

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OC = OCPI::Container; // only for error codes...
namespace OD = OCPI::Driver;
namespace OE = OCPI::Util::EzXml;

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
    findArtifact(const Capabilities &caps, const char *specName,
		 const OCPI::API::PValue *params,
		 const char *selectCriteria,
		 const OCPI::API::Connection *conns,
		 const char *&artInst) {
      return getSingleton().findArtifactX(caps, specName, params, selectCriteria, conns, artInst);
    }
    Artifact &Manager::
    findArtifactX(const Capabilities &caps, const char *specName,
		  const OCPI::API::PValue *params,
		  const char *selectCriteria,
		  const OCPI::API::Connection *conns,
		  const char *&artInst) {
      parent().configureOnce();
      // If some driver already has it in one of its libraries, return it.
      Artifact *a;
      for (Driver *d = firstDriver(); d; d = d->nextDriver())
	if ((a = d->findArtifact(caps, specName, params, selectCriteria, conns, artInst)))
	  return *a;
      throw OU::Error("No usable artifact found in any library in OCPI_LIBRARY_PATH, "
		      "for worker implementing \"%s\"", specName);
    }

    Artifact &Manager::getArtifact(const char *url, const OA::PValue *params) {
      return getSingleton().getArtifactX(url, params);
    }

    Artifact &Manager::getArtifactX(const char *url, const OA::PValue *params) {
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
	if ((a = d->addArtifact(url, params)))
	  return *a;
      throw OU::EmbeddedException(OU::ARTIFACT_UNSUPPORTED,
				  "No library driver supports this file",
				  OU::ApplicationRecoverable);
    }

    // Inform the manager about an implementation
    void Manager::addImplementation(Implementation &impl) {
      m_implementations.insert(WorkerMapPair(impl.m_metadataImpl.specName().c_str(), &impl));
    }
    static bool
    satisfiesSelection(const char *selection, unsigned *score, OU::Worker &impl) {
      OU::ExprValue val;
      const char *err = OU::evalExpression(selection, val, &impl);
      if (err)
	throw OU::Error("Error parsing selection expression: %s", err);
      if (!val.isNumber)
	throw OU::Error("selection expression has string value");
      if (score)
	// FIXME test if overflow
	*score = (unsigned)(val.number < 0 ? 0: val.number); // force non-negative
      return val.number > 0;
    }

    // Find (and callback with) implementations for specName and selectCriteria
    // Return true if any were found
    bool Manager::findImplementationsX(ImplementationCallback &icb, const char *specName) {
      parent().configureOnce();
      bool found = false;
      WorkerRange range = m_implementations.equal_range(specName);
      for (WorkerIter wi = range.first; wi != range.second; wi++) {
	Implementation &impl = *wi->second;
	if (icb.foundImplementation(impl, found))
	  break;
      }
      return found;
    }
    // Find one good implementation, return true if one is found that satisfies the criteria
    bool Manager::findImplementation(const char *specName, const char *selectCriteria,
				     const Implementation *&impl) {
      struct mine : public ImplementationCallback {
	const Implementation *&m_impl;
	const char *m_selection;
	mine(const Implementation *&impl, const char *selection)
	  : m_impl(impl), m_selection(selection) {}
	bool foundImplementation(const Implementation &i, bool &accepted) {
	  if (m_selection && !satisfiesSelection(m_selection, NULL, i.m_metadataImpl))
	    return false;
	  m_impl = &i;
	  accepted = true;
	  return true;
	}
      } cb(impl, selectCriteria);
      return findImplementationsX(cb, specName);
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
    Implementation::
    Implementation(Artifact &art, OU::Worker &i, ezxml_t instance, unsigned ordinal)
	: m_artifact(art), m_metadataImpl(i), m_staticInstance(instance),
	  m_externals(0), m_internals(0), m_connections(NULL), m_ordinal(ordinal)
    {}

    void Implementation::
    setConnection(OU::Port &myPort, Implementation *otherImpl,
		  OU::Port *otherPort) {
      if (otherImpl) {
	m_internals |= 1 << myPort.m_ordinal;
	if (!m_connections)
	  m_connections = new Connection[m_metadataImpl.nPorts()];
	m_connections[myPort.m_ordinal].impl = otherImpl;
	m_connections[myPort.m_ordinal].port = otherPort;
      } else {
	m_externals |= 1 << myPort.m_ordinal;
      }
    }
    Driver::
    Driver(const char *name)
      : OD::DriverType<Manager,Driver>(name, *this) {
    }
    Artifact *Driver::
    findArtifact(const Capabilities &caps,
		 const char *specName,
		 const OCPI::API::PValue *params,
		 const char *selectCriteria,
		 const OCPI::API::Connection *conns,
		 const char *&artInst) {
      Artifact *a = NULL;
      for (Library *l = firstLibrary(); !a && l; l = l->nextLibrary())
	a = l->findArtifact(caps, specName, params, selectCriteria, conns, artInst);
      return a;
    }

    Library::Library(const std::string &name) : m_name(name) {
      if (!s_firstLibrary)
	s_firstLibrary = this;
    }
    Library::~Library(){}
    Library *Library::s_firstLibrary;
    Artifact * Library::
    findArtifact(const Capabilities &caps,
		 const char *specName,
		 const OCPI::API::PValue *params,
		 const char *selectCriteria,
		 const OCPI::API::Connection *conns,
		 const char *&artInst) {

      Artifact * best=NULL;
      unsigned score, best_score=0;
      for (Artifact *a = firstArtifact(); a; a = a->nextArtifact()) {
	if (a->meetsRequirements(caps, specName, params, selectCriteria, conns, artInst, score)) {
	  if ( selectCriteria ) {
	    if ( score >= best_score ) {
	      best = a;
	      best_score = score;
	    }
	  }
	  else {
	    return a;
	  }
	}
      }
      return best;
    }
    Artifact * Library::
    findArtifact(const char *uuid) {
      ArtifactsIter ai = m_artifacts.find(uuid);
      return ai == m_artifacts.end() ? NULL : ai->second;
    }

    // The artifact base class
    Artifact::Artifact() : m_xml(NULL), m_nImplementations(0), m_metaImplementations(NULL), m_nWorkers(0) {}
    Artifact::~Artifact() {
      for (WorkerIter wi = m_workers.begin(); wi != m_workers.end(); wi++)
	delete (*wi).second;
      delete [] m_metaImplementations;
    }
    // Get the metadata from the end of the file.
    // The length of the appended file is appended on a line starting with X
    // i.e. (cat meta; sh -c 'echo X$4' `ls -l meta`) >> artifact
    // This scheme allows for binary metadata, but we are doing XML now.
    // The returned value must be deleted with delete[];
    char *Artifact::
    getMetadata(const char *name, std::time_t &mtime) {
	  char *data = 0;
	  int fd = open(name, O_RDONLY);
	  if (fd < 0)
	    throw OU::Error("Cannot open file: \"%s\"", name);
	  struct stat info;
	  if (fstat(fd, &info))
	    throw OU::Error("Cannot get modification time: \"%s\" (%d)", name, errno);
	  mtime = info.st_mtime;
	  char buf[64/3+4]; // octal + \r + \n + null
	  off_t fileLength, second, third;
	  if (fd >= 0 &&
	      (fileLength = lseek(fd, 0, SEEK_END)) != -1 &&
	      // I have no idea why the off_t caste below is required,
	      // but without it, the small negative number is not sign extended...
	      // on MACOS gcc v4.0.1 with 64 bit off_t
	      (second = lseek(fd, -(off_t)sizeof(buf), SEEK_CUR)) != -1 &&
	      (third = read(fd, buf, sizeof(buf))) == sizeof(buf)) {
	    for (char *cp = &buf[sizeof(buf)-2]; cp >= buf; cp--)
	      if (*cp == 'X' && isdigit(cp[1])) {
		char *end;
		long l = strtol(cp + 1, &end, 10);
		off_t n = (off_t)l;
		// strtoll error reporting is truly bizarre
		if (l != LONG_MAX && l > 0 && cp[1] && isspace(*end)) {
		  off_t metaStart = fileLength - sizeof(buf) + (cp - buf) - n;
		  if (lseek(fd, metaStart, SEEK_SET) != -1) {
		    data = new char[n + 1];
		    if (read(fd, data, n) == n)
		      data[n] = '\0';
		    else {
		      delete [] data;
		      data = 0;
		    }
		  }
		}
		break;
	      }
	  }
	  if (fd >= 0)
	    (void)close(fd);
	  return data;
	}

    bool Artifact::
    meetsRequirements (const Capabilities &caps,
		       const char *specName,
		       const OCPI::API::PValue * /*props*/,
		       const char *selectCriteria,
		       const OCPI::API::Connection * /*conns*/,
		       const char *& /* artInst */,
		       unsigned & score ) {
      if (m_os == caps.m_os && m_osVersion == caps.m_osVersion &&
	  m_platform == caps.m_platform) {
	WorkerRange range = m_workers.equal_range(specName);

	for (WorkerIter wi = range.first; wi != range.second; wi++) {
	  //	  const Implementation &i = (*wi).second;
	  // FIXME: more complex comparison for FPGAs with connectivity
	  //	  const char *model = ezxml_cattr(wi->second->m_worker, "model");
	  if (caps.m_model != wi->second->m_metadataImpl.model())
	    continue;
	  // Now we will test the selection criteria 
	  return !selectCriteria || satisfiesSelection(selectCriteria, &score, wi->second->m_metadataImpl);
	}
      }
      return false;
    }
    Implementation *Artifact::addImplementation(OU::Worker &metaImpl, ezxml_t staticInstance) {
      Implementation *impl = new Implementation(*this, metaImpl, staticInstance, m_nWorkers++);
      // Record in the artifact's mapping
      m_workers.insert(WorkerMapPair(metaImpl.specName().c_str(), impl));
      // Record in the globalmapping
      Manager::getSingleton().addImplementation(*impl);
      return impl;
    }
    // Note this XML argument is from the system config file, not the
    // XML attached to this artifact file.
    // But in any case we process the attached metadata here, and not
    // in the constructor, for consistency with the Manager/Driver/Device model
    // Someday we might AUGMENT/OVERRIDE the attached metadata with the
    // system configuration metadata
    void Artifact::configure(ezxml_t /* x */) {
      // Retrieve attributes from artifact xml
      Attributes::parse(m_xml);
      // Loop over all the implementations
      m_nImplementations = OE::countChildren(m_xml, "worker");
      OU::Worker *metaImpl = m_metaImplementations = new OU::Worker[m_nImplementations];
      typedef std::map<const char*, Implementation *, OU::ConstCharComp> InstanceMap;
      typedef InstanceMap::iterator InstanceIter;
      InstanceMap instances; // record static instances for connection tracking
      unsigned n = 0;
      for (ezxml_t w = ezxml_cchild(m_xml, "worker"); w; w = ezxml_next(w), metaImpl++, n++) {
	metaImpl->m_ordinal = n;
	const char *err = metaImpl->parse(w, this);
	if (err)
	  throw OU::Error("Error processing implementation metadata for %s: %s",
			  name().c_str(), err);
	bool haveInstances = false;
	for (ezxml_t i = ezxml_cchild(m_xml, "instance"); i; i = ezxml_next(i))
	  if (!strcasecmp(metaImpl->name().c_str(), ezxml_cattr(i, "worker"))) {
	    haveInstances = true;
	    instances[ezxml_cattr(i, "name")] = addImplementation(*metaImpl, i);
	  }
	if (!haveInstances)
	  (void)addImplementation(*metaImpl, NULL);
      }
      // Record connectivity in the artifact: what is external, and what is
      // internal, and if internal, who is connected to whom
      for (ezxml_t conn = ezxml_child(m_xml, "connection"); conn; conn = ezxml_next(conn)) {
        const char
          *fromX = ezxml_attr(conn,"from"), // instance with user port
          *toX = ezxml_attr(conn,"to"),     // instance with provider port
          *out = ezxml_attr(conn, "out"),  // user port name
          *in = ezxml_attr(conn, "in");    // provider port name
        if (!fromX || !toX || !out || !in)
	  throw OU::Error("Invalid artifact XML: connection has bad attributes");
	OU::Port *fromP, *toP;
	InstanceIter
	  fromI = instances.find(fromX),
	  toI = instances.find(toX);
	Implementation
	  *fromImpl = fromI == instances.end() ? NULL : fromI->second,
	  *toImpl = toI == instances.end() ? NULL : toI->second;
	if (fromImpl && !(fromP = fromImpl->m_metadataImpl.findMetaPort(out)) ||
	    toImpl && !(toP = toImpl->m_metadataImpl.findMetaPort(in)))
	  throw OU::Error("Invalid artifact XML: \"to\" or \"from\" port not found for connection");
	if (fromImpl) {
	  fromImpl->setConnection(*fromP, toImpl, toP);
	  if (toImpl)
	    toImpl->setConnection(*toP, fromImpl, fromP);
	} else if (toImpl)
	  toImpl->setConnection(*toP);
      }
    }
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
