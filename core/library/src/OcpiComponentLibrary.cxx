#include <stdlib.h>
#include <string.h>
//#include <dirent.h>
#include <climits>
#include "OcpiOsAssert.h"
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
	  : ArtifactBase<Library,Artifact>(lib, *this, name),
	    m_metadata(0)
	{	
	  // The returned value must be deleted with delete[];
	  if (!(m_metadata = getMetadata()))
	    throw OU::Error(OCPI_LOG_DEBUG, "Cannot open or retrieve metadata from file \"%s\"", name);
	  m_xml = OX::Doc::parse(m_metadata);
	  char *xname = ezxml_name(m_xml);
	  if (!xname || strcmp("artifact", xname))
	    throw OU::Error("invalid metadata in binary/artifact file \"%s\": no <artifact/>", name);
	  ocpiDebug("Artifact file %s has artifact metadata", name);
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
	    throw OU::Error("Cannot open file: \"%s\"", name().c_str());
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
      };

      class Driver;

      // Our concrete library class
      class Library : public OL::LibraryBase<Driver, Library, Artifact> {
	friend class Driver;
	Library(const char *name)
	  : OL::LibraryBase<Driver,Library,Artifact>(*this, name) {}

	void doPath(const std::string &libName) {
	  ocpiDebug("Processing library path: %s", libName.c_str());
	  bool isDir;
	  if (!OS::FileSystem::exists(libName, &isDir))
	    ocpiBad("Component library path name in OCPI_LIBRARY_PATH, \"%s\", "
		    "is nonexistent.  It will be ignored", libName.c_str());
	  else if (isDir) {
	    OS::FileIterator dir(libName, "*");
	    for (; !dir.end(); dir.next())
	      doPath(OS::FileSystem::joinNames(libName, dir.relativeName()));
	  } else {
	    const char *name = libName.c_str();
	    size_t len = strlen(name), xlen = strlen(".xml");
	  
	    if (len < xlen || strcasecmp(name + len - xlen, ".xml")) {
	    // FIXME: supply library level xml for the artifact
	    // The log will show which files are not any good.
	      try {
		(new Artifact(*this, name, NULL))->configure();
	      } catch (...) {}
	    }
	  }
	}
      public:
	// Do a recursive dirctory search for all files.
	void configure(ezxml_t) {
	  doPath(name());
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
	    ocpiDebug("OCPI_LIBRARY_PATH is %s", path);
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
