#include "CpiContainerInterface.h"
#include "CpiArtifact.h"
#include "CpiApplication.h"
#include "CpiPValue.h"


namespace CPI {
  namespace Container {
    Application::Application(Interface &i, const char *name, CPI::Util::PValue *) :
      CPI::Util::Child<Interface, Application>(i), m_name(name) {
    }
    Application::~Application() {
    }

    Artifact & Application::loadArtifact(const char *url, CPI::Util::PValue *artifactParams ) {
      return myParent->loadArtifact(url, artifactParams);
    }

    // Convenience if caller doesn't want Artifact objects.
    Worker &Application::createWorker(const char *url, CPI::Util::PValue *aParams,
                                      const void *entry, const char *inst,
                                      CPI::Util::PValue *wParams) {
      Artifact &a = loadArtifact(url, aParams);
      return a.createWorker(*this, (const char*)entry, inst, wParams);
    }
    Worker &Application::createWorker(Artifact &a, const char* impl, const char *inst, 
                                      CPI::Util::PValue *wParams) {
      return a.createWorker(*this, impl, inst, wParams);
    }
  }
}
