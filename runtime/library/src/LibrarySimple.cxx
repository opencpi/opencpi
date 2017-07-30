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

// This file implements a "simple" library, meaning one that is simply given artifacts manually,
// and which has no "root directory"

#include "OcpiUtilException.h"
#include "OcpiUtilEzxml.h"
#include "OcpiLibraryManager.h"
#include "LibrarySimple.h"

namespace OL = OCPI::Library;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OD = OCPI::Driver;

namespace OCPI {
  namespace Library {
    namespace Simple {
      class Library;

      // Our concrete artifact class
      class Artifact
	: public OL::ArtifactBase<Library, Artifact> {
	friend class Library;
      public:
	Artifact(Library &lib, const char *a_name, const OA::PValue *)
	  : ArtifactBase<Library,Artifact>(lib, *this, a_name) {
	  getFileMetadata(a_name);
	}
	~Artifact() {}
      };
	  
      class Driver;

      // Our concrete library class
      class Library : public OL::LibraryBase<Driver, Library, Artifact> {
	friend class Driver;
	Library()
	  : OL::LibraryBase<Driver,Library,Artifact>(*this, "simple") {
	}
      public:
	OCPI::Library::Artifact *
	addArtifact(const char *url, const OCPI::API::PValue *props) {
	  Artifact *a = new Artifact(*this, url, props);
	  a->configure(); // FIXME: there could be config info in the platform.xml
	  return a;
	}
      };

      // Our concrete driver class
      const char *simple = "simple";
      class Driver
	: public OCPI::Library::DriverBase<Driver, Library, simple> {
      public:
	// Telling the driver to add an artifact is a forced load that does not
	// come from any other library, but the library can return NULL
	// if the file is not the type of file supported by this driver.
	OL::Artifact *addArtifact(const char *url, const OA::PValue *props) {
	  Library *l = firstChild();
	  if (!l)
	    l = new Library();
	  return l->addArtifact(url, props);
	}
      };
      RegisterLibraryDriver<Driver> driver;
      OL::Driver &getDriver() { return OU::Singleton<Driver>::getSingleton(); }
    }
  }
}
