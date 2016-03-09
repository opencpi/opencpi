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
	Artifact(Library &lib, const char *name, const OA::PValue *)
	  : ArtifactBase<Library,Artifact>(lib, *this, name) {
	  getFileMetadata(name);
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
      OL::Driver &getDriver() { return OD::Singleton<Driver>::getSingleton(); }
    }
  }
}
