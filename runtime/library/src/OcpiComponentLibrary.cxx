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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <set>
#include "ocpi-config.h"
#include "OcpiOsAssert.h"
#include "OcpiOsFileIterator.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilException.h"
#include "OcpiUtilEzxml.h"
#include "OcpiLibraryManager.h"
#include "OcpiComponentLibrary.h"

// This file is the (loadable) driver for ocpi component libraries, each of which is
// rooted in a file system directory, which becomes its name

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
	: public OL::ArtifactBase<Library, Artifact> {
	friend class Library;
      public:
	Artifact(Library &lib, const char *a_name, char *metadata, std::time_t a_mtime,
		 uint64_t a_length, size_t metaLength, const OA::PValue *)
	  : ArtifactBase<Library,Artifact>(lib, *this, a_name) {
	  const char *err = setFileMetadata(a_name, metadata, a_mtime, a_length, metaLength);
	  if (err)
	    throw OU::Error("Error processing metadata from artifact file: %s: %s", a_name, err);
	}
      };

      class Driver;

      typedef std::set<OS::FileSystem::FileId> FileIds; // unordered set cxx11 is better
      // Our concrete library class
      class Library : public OL::LibraryBase<Driver, Library, Artifact> {
	FileIds &m_fileIds;
	friend class Driver;
	Library(const char *a_name, FileIds &ids)
	  : OL::LibraryBase<Driver,Library,Artifact>(*this, a_name), m_fileIds(ids) {
	}

	public:
	// Do a recursive directory search for all files.
	void configure(ezxml_t) {
	  std::string globbedName;
	  if (OU::globPath(name().c_str(), globbedName))
	    ocpiInfo("Library path pathname \"%s\" is invalid or nonexistent, and ignored",
		    name().c_str());
	  doPath(globbedName);
	}
	OCPI::Library::Artifact *
	addArtifact(const char *url, const OCPI::API::PValue *params) {
	  std::time_t mtime;
	  uint64_t length;
	  size_t metaLength;
	  char *metadata = OCPI::Library::Artifact::getMetadata(url, mtime, length, metaLength);
	  if (!metadata)
	    throw OU::Error(20, "Cannot open or retrieve metadata from file \"%s\"", url);
	  Artifact *a = new Artifact(*this, url, metadata, mtime, length, metaLength, params);
	  a->configure(); // FIXME: there could be config info in the platform.xml
	  // FIXME: return NULL if this doesn't look like an artifact we can support?
	  return a;
	}
      private:
	void doPath(const std::string &a_libName) {
	  //	  ocpiDebug("Processing library path: %s", libName.c_str());
	  bool isDir;
	  OS::FileSystem::FileId file_id;
	  if (!OS::FileSystem::exists(a_libName, &isDir, NULL, NULL, &file_id))
	    ocpiDebug("Path name found in OCPI_LIBRARY_PATH, \"%s\", "
		     "is nonexistent, not a normal file, or a broken link.  It will be ignored",
		     a_libName.c_str());
	  else if (m_fileIds.insert(file_id).second) {
	    ocpiLog(20, "Found ARTIFACT: %s id is: %016" PRIx64 "%016" PRIx64, a_libName.c_str(),
		     file_id.m_opaque[0], file_id.m_opaque[1]);
	    // New id was inserted, and thus was not already there
	    if (isDir) {
	      try { // this is really checking the constructor
		OS::FileIterator dir(a_libName, "*");
		for (; !dir.end(); dir.next())
		  doPath(OS::FileSystem::joinNames(a_libName, dir.relativeName()));
	      } catch(...) {
		ocpiBad("For OCPI_LIBRARY_PATH: failed to enter directory \"%s\".  Permissions?",
			a_libName.c_str());
		return;
	      }
	    } else {
	      const char *l_name = a_libName.c_str();
	      size_t len = strlen(l_name), xlen = strlen(".xml");

	      if (len < xlen || strcasecmp(l_name + len - xlen, ".xml")) {
		// FIXME: supply library level xml for the artifact
		// The log will show which files are not any good.
		try {
		  addArtifact(l_name, NULL);
		} catch (...) {}
	      }
	    }
          }
	}
      };

      // Our concrete driver class
      const char *component = "component";
      class Driver
	: public OCPI::Library::DriverBase<Driver, Library, component> {
	FileIds m_fileIds;
      public:
	void configure(ezxml_t x) {
	  // First we call the base class, which loads explicit libraries.
	  OL::Driver::configure(x);
	}
	unsigned search(const PValue* /*props*/, const char **/*exclude*/,
			bool /* discoveryOnly */) {
	  unsigned n = 0;
	  // Now we look in the path environment variable
	  // FIXME: canonicalize the names before dup matching? (i.e. realpath)??
	  const char *path = getenv("OCPI_LIBRARY_PATH");
	  ocpiDebug("ComponentLibrary search with OCPI_LIBRARY_PATH: %s", path);
	  if (path) {
	    ocpiDebug("OCPI_LIBRARY_PATH is %s", path);
	    char *cp = strdup(path), *last;
	    try {
	      for (char *lp = strtok_r(cp, ":", &last); lp;
		   lp = strtok_r(NULL, ":", &last)) {
		ocpiInfo("Searching directory %s recursively, from OCPI_LIBRARY_PATH", lp);
		// We have a library in the path.
		(new Library(lp, m_fileIds))->configure(NULL);
		n++;
	      }
	    } catch (...) {
	      free(cp);
	      throw;
	    }
	    free(cp);
	  }
	  return n;
	}

	// Telling the driver to add an artifact is a forced load that does not
	// come from any other library, but the library can return NULL
	// if the file is not the type of file supported by this driver.
	OL::Artifact *addArtifact(const char *url, const OA::PValue *props) {
	  Library *l = firstChild();
	  if (!l)
	    l = new Library(".", m_fileIds);
	  return l->addArtifact(url, props);
	}
      };
      RegisterLibraryDriver<Driver> driver;
    }
  }
}
