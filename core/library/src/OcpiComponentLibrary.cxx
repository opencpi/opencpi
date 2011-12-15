#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "OcpiOsFileIterator.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilException.h"
#include "OcpiUtilEzxml.h"
#include "OcpiLibraryManager.h"

// This file is the (loadable) driver for ocpi component libraries.

namespace OL = OCPI::Library;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OX = OCPI::Util::EzXml;
namespace OCPI {
  namespace Library {
    namespace CompLib {
      using OCPI::Util::ApiError;
      class Library;

      // Our concrete artifact class
      class Artifact
	: public OL::ArtifactBase<Library, Artifact>, OU::EzXml::Doc {
	friend class Library;
	char *m_metadata;
	Artifact(Library &lib, const char *name, const OA::PValue *)
	  : ArtifactBase<Library,Artifact>(lib, name),
	    m_metadata(0)
	{	
	  // The returned value must be deleted with delete[];
	  if (!(m_metadata = getMetadata()))
	    throw ApiError("Cannot open or retrieve metadata from file \"",
			   name, "\"", NULL);
	  m_xml = OX::Doc::parse(m_metadata);
	  char *xname = ezxml_name(m_xml);
	  if (!xname || strcmp("artifact", xname))
	    throw ApiError("invalid metadata in binary/artifact file \"", name,
			   "\": no <artifact/>", NULL);
	}
      public:
	~Artifact() {
	  // NO!!!  The inherited class in fact takes responsibility for the
	  // char * string passed to the "parse" method, and uses deletep[] on it!
	  // delete [] m_metadata;
	}
      private:
	// Get the metadata from the end of the file.
	// The length of the appended file is appended on a line starting with X
	// i.e. (cat meta; sh -c 'echo X$4' `ls -l meta`) >> artifact
	// This scheme allows for binary metadata, but we are doing XML now.
	// The returned value must be deleted with delete[];
	char *getMetadata() {
	  char *data = 0;
	  int fd = open(name().c_str(), O_RDONLY);
	  if (fd < 0)
	    throw ApiError("Cannot open file: \"", name().c_str(), "\"", NULL);
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
		long long n = strtoll(cp + 1, &end, 10);
		// strtoll error reporting is truly bizarre
		if (n != LLONG_MAX && n >= 0 && cp[1] && isspace(*end)) {
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
      };

      class Driver;

      // Our concrete library class
      class Library : public OL::LibraryBase<Driver, Library, Artifact> {
	friend class Driver;
	Library(const char *name)
	  : OL::LibraryBase<Driver,Library,Artifact>(name) {}
	// Recursive.
	void doDir(const std::string &dirName) {
	  bool isDir;
	  if (!OS::FileSystem::exists(dirName, &isDir) ||
	      !isDir)
	    throw OU::Error("Directory name in OCPI_LIBRARY_PATH, \"%s\", "
			    "is nonexistent or not a directory", dirName.c_str());

	  static const std::string pattern("*");
	  OS::FileIterator dir(dirName, "*");
	  for (; !dir.end(); dir.next()) {
	    if (dir.isDirectory()) {
	      std::string subDir =
		OS::FileSystem::joinNames(dirName, dir.relativeName());
	      doDir(subDir);
	    } else {
	      std::string absolute = dir.absoluteName();
	      const char *absName = absolute.c_str();
	      unsigned len = strlen(absName), xlen = strlen(".xml");
	      
	      if (len < xlen || strcasecmp(absName + len - xlen, ".xml"))
		// FIXME: supply library level xml for the artifact
		(new Artifact(*this, absName, NULL))->configure();
	    }
	  }
	}
      public:
	// Do a recursive dirctory search for all files.
	void configure(ezxml_t) {
	  doDir(name());
	}
	OCPI::Library::Artifact *
	addArtifact(const char *url, const OCPI::API::PValue *props) {
	  // return NULL if this doesn't look like an artifact we can support
	  Artifact *a = new Artifact(*this, url, props);
	  a->configure(); // FIXME: there could be config info in the platform.xml
	  return a;
	}
      };

      // Our concrete driver class
      const char *component = "component";
      class Driver
	: public OCPI::Library::DriverBase<Driver, Library, component> {
      public:
	void configure(ezxml_t x) {
	  // First we call the base class, which loads explicit libraries.
	  OL::Driver::configure(x);
	  // Now we look in the path environment variable
	  // FIXME: canonicalize the names before dup matching? (i.e. realpath)??
	  const char *path = getenv("OCPI_LIBRARY_PATH");
	  if (path) {
	    char *cp = strdup(path), *last;
	    try {
	      for (char *lp = strtok_r(cp, ":", &last); lp;
		   lp = strtok_r(NULL, ":", &last))
		// We have a library in the path.
		(new Library(lp))->configure(NULL);
	    } catch (...) {
	      free(cp);
	      throw;
	    }
	    free(cp);
	  }
	}
	// Telling the driver to add an artifact is a forced load that does not
	// come from any other library, but the library can return NULL
	// if the file is not the type of file supported by this driver.
	OL::Artifact *addArtifact(const char *url, const OA::PValue *props) {
	  Library *l = firstChild();
	  if (!l)
	    l = new Library(".");
	  return l->addArtifact(url, props);
	}
      };
      RegisterLibraryDriver<Driver> driver;
    }
  }
}
#if 0
// junk
      // fundamentally you want a match between a worker name and a container.
      // for now we are looking for something to run on a container.
      // (which is the inner loop of an app factory anyway)
      // So the lookup will be a container (for resource and metainfo) and a worker name
      // (could be sort of scd id someday).
      // So we have the container offering a platform and the artifact offering a platform.
      // artifacts might run on many platforms, platforms run many artifacts.
      // artifacts have only metadata, while containers can have runtimes.
      // so you want to prune the workers in the library based on metadata, and then
      // offer them all to the containters that have a match later.  So the platform offers up some sort of
      // root pattern to match against, and then the candidate(s) are given to the container for a
      // final check.
      // file suffix will encode enough to be unique:
      // worker, worker-version, os, version, machine/processor, tools, tools-version
      // fft-1_3-linux-RHEL5_6-x86_64-gcc-4_5
      // want to prefer loaded artifacts? does this mean the library should register
      // containers that have loaded it?
      
      // Register the possible workers and instances in this artifact
      // with the library:
      // The map/database has the worker name as the key
      // The value of the key is the artifact/fixed-instance-list pairs.
      // So you lookup based on a worker to see how to match against containers.
      // To match against containers you have these issues:
      // 1. Basic platform match - can the container run the artifact?
      //    Is the target a match: ask container whether it supports the target
      //    Normally for HDL containers, its the "platform"...
      //    Pure metadata basis.
      //    The container manager can hold this map:
      //    For each platform id, which containers support it?
      // 2. Runtime artifact mutex - can the artifact fit on the container?
      //    RCC would say yes always (or for memory), HDL would say 1
      // 3. If there is a specified instance, is it there? (metadata, artifact)
      // 4. If there is a specified instance, is it busy? (runtime)
      // 5. If there is a specified instance, is connectivity ok? (runtime)
      //    Can we find an instance based on connectivity?
      //    For each connection:
      //      if internal
      //         if peer is unassigned, assert collocation constraint
      //         if peer is assigned, we shouldn't get here since higher level
      //          should not try, except if it is ok.
      //      if external, 
      //         if peer is unassigned assert a non-colocation constraint,
      //         if peer is assigned, then it is ok or not.  Need the connection
      //           (common supported endpoints etc.).
      //   Key:  instance decisions require connectivity awareness.
      // If all ok, its a candidate container for the worker.

  leftovers:
Interaction with loading and referencing counting and stickiness.
(loading is specialized).
Inserting metadata into the map of the library.
Extra library for explicitly loaded stuff.
What we need for the container artifact:
- which artifact?
- which instances/workers (fixed and dynamic)
    // We want an instance from an implementation, and optionally,
    // a specifically identified instance of that implementation,
    // when the artifact would contain such a thing.  Here are the cases:
    // Instance tag supplied, no instances in the artifact: error
    // Instance tag supplied, instance not in artifact: error
    // Instance tag supplied, instance already used: error
    // No instance tag supplied, no instances in artifact: ok
    // No instance tag supplied, instances in artifact, all used: error
    // No instance tag supplied, instances in artifact, one is available: ok
#endif
