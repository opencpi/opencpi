/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <climits>
#include <set>
#include "ocpi-config.h"
#include "OcpiUtilException.h"
#include "OcpiLibraryManager.h"
#include "LibrarySimple.h"
#include "OcpiComponentLibrary.h"
#include "OcpiOsAssert.h"

// This file contains code common to all library drivers

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OD = OCPI::Driver;
namespace OE = OCPI::Util::EzXml;

namespace OCPI {
  namespace Library {
    const char *library = "library";
    // This is intended to force this "driver" to be statically linked into this library
    const char **complib OCPI_USED = &CompLib::component;
    static OCPI::Driver::Registration<Manager> lm;
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
      throw OU::Error("No usable artifact found in any library in OCPI_LIBRARY_PATH (%s), "
		      "for worker implementing \"%s\"", getenv("OCPI_LIBRARY_PATH"), specName);
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
#if 0
      // The artifact was not found in any driver's libraries
      // Now we need to find a library driver that can deal with this
      // artifact. In this case a driver returning NULL means the driver is
      // passing on the artifact.  If there is an error dealing with the
      // artifact, that will throw an exception.
      for (Driver *d = firstChild(); d; d = d->nextChild())
	if ((a = d->addArtifact(url, params)))
	  return *a;
#else
      // If the artifact is not found, we will put it in the "simple" library that
      // is not associated with any directory.
      if ((a = Simple::getDriver().addArtifact(url, params)))
	return *a;
#endif
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
      if (!val.isNumber())
	throw OU::Error("selection expression has string value");
      if (score)
	// FIXME test if overflow
	*score = (unsigned)(val.getNumber() < 0 ? 0: val.getNumber()); // force non-negative
      return val.getNumber() > 0;
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
    void Manager::printArtifactsX(const Capabilities &caps, bool dospecs) {
      parent().configureOnce();
      std::set<const char *, OCPI::Util::ConstCharComp> specs;
      for (Driver *d = firstDriver(); d; d = d->nextDriver())
	for (Library *l = d->firstLibrary(); l; l = l->nextLibrary())
	  for (Artifact *a = l->firstArtifact(); a; a = a->nextArtifact())
	    if (a->meetsCapabilities(caps)) {
	      if (dospecs)
		a->printSpecs(specs);
	      else
		printf("%s\n", a->name().c_str());
	    }
    }
    // Call a function for all workers in all artifacts
    void Manager::doWorkers(void (*func)(OU::Worker &)) {
      parent().configureOnce();
      std::set<const char *, OCPI::Util::ConstCharComp> specs;
      for (Driver *d = firstDriver(); d; d = d->nextDriver())
	for (Library *l = d->firstLibrary(); l; l = l->nextLibrary())
	  for (Artifact *a = l->firstArtifact(); a; a = a->nextArtifact()) {
	    const Implementation *i;
	    for (unsigned n = 0; (i = a->getImplementation(n)); n++)
	      func(i->m_metadataImpl);
	  }
    }
    // Find one good implementation, return true if one is found that satisfies the criteria
    bool Manager::findImplementation(const char *specName, const char *selectCriteria,
				     const Implementation *&impl) {
      struct mine : public ImplementationCallback {
	const Implementation *&m_impl;
	const char *m_selection;
	mine(const Implementation *&a_impl, const char *selection)
	  : m_impl(a_impl), m_selection(selection) {}
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
	for (lib = ezxml_cchild(x, "library"); lib; lib = ezxml_cnext(lib)) {
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
	  m_externals(0), m_internals(0), m_connections(NULL), m_ordinal(ordinal),
	  m_inserted(false)
    {
      OE::getBoolean(instance, "inserted", &m_inserted);
    }
    Implementation::
    ~Implementation() {
      delete [] m_connections;
    }

    void Implementation::
    setConnection(OU::Port &myPort, Implementation *otherImpl,
		  OU::Port *otherPort) {
      ocpiDebug("Setting connection in %s on %s port %s with other %s port %s",
	       m_artifact.name().c_str(), m_metadataImpl.cname(), myPort.m_name.c_str(),
	       otherImpl ? otherImpl->m_metadataImpl.cname() : "none",
	       otherPort ? otherPort->m_name.c_str() : "none");
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
    Driver(const char *a_name)
      : OD::DriverType<Manager,Driver>(a_name, *this) {
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
    Artifact::
    Artifact()
      : m_metadata(NULL), m_mtime(0), m_length(0), m_xml(NULL), m_nImplementations(0),
	m_metaImplementations(NULL), m_nWorkers(0) {}
    Artifact::~Artifact() {
      for (WorkerIter wi = m_workers.begin(); wi != m_workers.end(); wi++)
	delete (*wi).second;
      delete [] m_metaImplementations;
      ezxml_free(m_xml);
      delete [] m_metadata;
    }
    // static utility function
    // Get the metadata from the end of the file.
    // The length of the appended file is appended on a line starting with X
    // i.e. (cat meta; sh -c 'echo X$4' `ls -l meta`) >> artifact
    // This scheme allows for binary metadata, but we are doing XML now.
    // The returned value must be deleted with delete[];
    char *Artifact::
    getMetadata(const char *name, std::time_t &mtime, uint64_t &length, size_t &metaLength) {
      char *data = 0;
      // nonblock so we don't hang on opening a FIFO
      // but we also don't want to waste time on a "stat" system call before "open"
      int fd = open(name, O_RDONLY|O_NONBLOCK);
      if (fd < 0)
	throw OU::Error("Cannot open file: \"%s\"", name);
      struct stat info;
      if (fstat(fd, &info)) {
	close(fd);
	throw OU::Error("Cannot get modification time: \"%s\" (%d)", name, errno);
      }
      if ((info.st_mode & S_IFMT) != S_IFREG) {
	close(fd);
	throw OU::Error("File: \"%s\" is not a normal file", name);
      }
      mtime = info.st_mtime;
      length = info.st_size;
      char buf[64/3+4]; // octal + \r + \n + null
      const size_t bufsize = sizeof(buf)-1; // Ensure trailing null character
      buf[bufsize] = '\0';
      off_t fileLength, second, third;
      if ((fileLength = lseek(fd, 0, SEEK_END)) != -1 &&
	  // I have no idea why the off_t cast below is required,
	  // but without it, the small negative number is not sign extended...
	  // on MACOS gcc v4.0.1 with 64 bit off_t
	  (second = lseek(fd, -(off_t)bufsize, SEEK_CUR)) != -1 &&
	  (third = read(fd, buf, bufsize)) == (ssize_t)bufsize) {
	for (char *cp = &buf[bufsize-1]; cp >= buf; cp--)
	  if (*cp == 'X' && isdigit(cp[1])) {
	    char *end;
	    long l = strtol(cp + 1, &end, 10);
	    off_t n = (off_t)l;
	    // strtoll error reporting is truly bizarre
	    if (l != LONG_MAX && l > 0 && cp[1] && isspace(*end)) {
	      metaLength = n + (&buf[bufsize] - cp);
	      off_t metaStart = fileLength - metaLength;
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
      (void) close(fd);
      return data;
    }

    // Given metadata in string form, parse it up, shortly after construction
    // The ownership of metadat is passed in here.
    const char *Artifact::
    setFileMetadata(const char *a_name, char *metadata, std::time_t a_mtime, uint64_t a_length,
		    size_t metaLength) {
      m_metadata = metadata; // take ownership in all cases
      const char *err = OE::ezxml_parse_str(metadata, strlen(metadata), m_xml);
      if (err)
	return OU::esprintf("error parsing artifact metadata from \"%s\": %s", a_name, err);
      char *xname = ezxml_name(m_xml);
      if (!xname || strcasecmp("artifact", xname))
	return OU::esprintf("invalid metadata in binary/artifact file \"%s\": no <artifact>",
			    a_name);
      const char *l_uuid = ezxml_cattr(m_xml, "uuid");
      if (!l_uuid)
	return OU::esprintf("no uuid in binary/artifact file \"%s\"", a_name);
      m_mtime = a_mtime;
      m_length = a_length;
      m_metaLength = metaLength;
      library().registerUuid(l_uuid, this);
      ocpiDebug("Artifact file %s has artifact metadata", a_name);
      return NULL;
    }
    void Artifact::
    getFileMetadata(const char *a_name) {
      std::time_t l_mtime;
      uint64_t l_length;
      size_t metaLength;
      char *metadata = getMetadata(a_name, l_mtime, l_length, metaLength);
      if (!metadata)
	throw OU::Error(20, "Cannot open or retrieve metadata from file \"%s\"", a_name);
      const char *err = setFileMetadata(a_name, metadata, l_mtime, l_length, metaLength);
      if (err)
	throw OU::Error("Error processing metadata from artifact file [manager]: %s: %s", a_name, err);
    }

    const Implementation *Artifact::
    getImplementation(unsigned n) const {
      unsigned nn = 0;
      for (WorkerIter wi = m_workers.begin(); wi != m_workers.end(); ++wi, ++nn)
	if (nn == n)
	  return wi->second;
      return NULL;
    }
    Implementation *Artifact::
    findImplementation(const char *specName, const char *staticInstance) {
      WorkerRange range = m_workers.equal_range(specName);
      for (WorkerIter wi = range.first; wi != range.second; wi++) {
	Implementation &impl = *wi->second;
	if (impl.m_staticInstance) {
	  if (staticInstance) {
	    const char *l_name = ezxml_cattr(impl.m_staticInstance, "name");
	    if (l_name && !strcasecmp(l_name, staticInstance))
	      return &impl;
	  }
	} else if (!staticInstance)
	  return &impl;
      }
      return NULL;
    }
    // Is this artifact for a container with these capabilities?
    // I.e. caps is what I am looking for
    bool Artifact::
    meetsCapabilities(const Capabilities &caps) {
      if (caps.m_dynamic != m_dynamic)
	return false;
      if (caps.m_platform.size())
	return m_platform == caps.m_platform;
      assert(caps.m_arch.size());
      return m_arch == caps.m_arch &&
	(caps.m_os.empty() || m_os == caps.m_os) &&
	(caps.m_osVersion.empty() || m_osVersion == caps.m_osVersion);
    }
    bool Artifact::
    meetsRequirements(const Capabilities &caps,
		      const char *specName,
		      const OCPI::API::PValue * /*props*/,
		      const char *selectCriteria,
		      const OCPI::API::Connection * /*conns*/,
		      const char *& /* artInst */,
		      unsigned & score) {
      if (meetsCapabilities(caps)) {
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
      ocpiDebug("Added implementation in artifact \"%s\" spec \"%s\" name \"%s\" inst \"%s\"",
		name().c_str(), metaImpl.specName().c_str(), metaImpl.cname(),
		staticInstance ? ezxml_cattr(staticInstance, "name") : "no-instance-name");

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
      for (ezxml_t w = ezxml_cchild(m_xml, "worker"); w; w = ezxml_cnext(w), metaImpl++, n++) {
	metaImpl->m_ordinal = n;
	const char *err = metaImpl->parse(w, this);
	if (err)
	  throw OU::Error("Error processing implementation metadata for %s: %s",
			  name().c_str(), err);
	bool haveInstances = false;
	for (ezxml_t i = ezxml_cchild(m_xml, "instance"); i; i = ezxml_cnext(i))
	  if (!strcasecmp(metaImpl->cname(), ezxml_cattr(i, "worker"))) {
	    haveInstances = true;
	    instances[ezxml_cattr(i, "name")] = addImplementation(*metaImpl, i);
	  }
	for (ezxml_t i = ezxml_cchild(m_xml, "io"); i; i = ezxml_cnext(i))
	  if (!strcasecmp(metaImpl->cname(), ezxml_cattr(i, "worker"))) {
	    haveInstances = true;
	    instances[ezxml_cattr(i, "name")] = addImplementation(*metaImpl, i);
	  }
	if (!haveInstances)
	  (void)addImplementation(*metaImpl, NULL);
      }
      // Record connectivity in the artifact: what is external, and what is
      // internal, and if internal, who is connected to whom
      for (ezxml_t conn = ezxml_child(m_xml, "connection"); conn; conn = ezxml_cnext(conn)) {
        const char
          *fromX = ezxml_attr(conn,"from"), // instance with user port
          *toX = ezxml_attr(conn,"to"),     // instance with provider port
          *out = ezxml_attr(conn, "out"),  // user port name
          *in = ezxml_attr(conn, "in");    // provider port name
        if (!fromX || !toX || !out || !in)
	  throw OU::Error("Invalid artifact XML: connection has bad attributes");
	OU::Port *fromP = NULL, *toP = NULL; // quiet warnings
	InstanceIter
	  fromI = instances.find(fromX),
	  toI = instances.find(toX);
	Implementation
	  *fromImpl = fromI == instances.end() ? NULL : fromI->second,
	  *toImpl = toI == instances.end() ? NULL : toI->second;
	if ((fromImpl && !(fromP = fromImpl->m_metadataImpl.findMetaPort(out))) ||
	    (toImpl && !(toP = toImpl->m_metadataImpl.findMetaPort(in))))
	  throw OU::Error("Invalid artifact XML: \"to\" or \"from\" port not found for connection");
	if (fromImpl) {
	  fromImpl->setConnection(*fromP, toImpl, toP);
	  if (toImpl)
	    toImpl->setConnection(*toP, fromImpl, fromP);
	} else if (toImpl)
	  toImpl->setConnection(*toP);
      }
      // Now that all the local instances are connected in the artifact, make a pass that
      // removes any inserted adapters.
      for (InstanceIter ii = instances.begin(); ii != instances.end(); ++ii) {
	Implementation &i = *ii->second;
	if (i.m_staticInstance)
	  for (unsigned nn = 0; nn < i.m_metadataImpl.nPorts(); ++nn)
	    if (i.m_internals & (1<<nn)) {
	      Implementation &otherImpl = *i.m_connections[nn].impl;
	      if (otherImpl.m_inserted) {
		// Big assumption that adapters only have two ports
		unsigned other = i.m_connections[nn].port->m_ordinal ? 0 : 1;
		if (otherImpl.m_internals & (1 << other)) {
		  i.m_connections[nn] = otherImpl.m_connections[other];
		  otherImpl.m_connections[other].impl = &i;
		  otherImpl.m_connections[other].port = &i.m_metadataImpl.getPorts()[nn];
		} else {
		  // other side of the adapter is external so this is external
		  i.m_internals &= ~(1 << nn);
		  i.m_externals |= 1 << nn;
		}
	      }
	    }
      }

    }
    void Artifact::
    printSpecs(std::set<const char *, OCPI::Util::ConstCharComp> &specs) const {
      for (WorkerIter wi = m_workers.begin(); wi != m_workers.end(); wi++)
	if (specs.insert((*wi).second->m_metadataImpl.specName().c_str()).second)
	  printf("%s\n", (*wi).second->m_metadataImpl.specName().c_str());
    }

    Capabilities::Capabilities() : m_dynamic(false) {}
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
