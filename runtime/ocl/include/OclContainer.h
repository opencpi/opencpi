#ifndef _OCL_CONTAINER_H_
#define _OCL_CONTAINER_H_
#include <stdint.h>
#include <set>
#include <string>
#include "ContainerManager.h"

namespace OCPI {
  namespace OCL {


    void compile(size_t nSources, const char **mapped_sources, off_t *sizes,
		 const char **includes, const char **defines, const char *output,
		 const char *target, bool verbose);

    class Port;
    class Artifact;
    class Application;
    class DeviceContext;
    class Driver;
    class Device;
    class Container
      : public OCPI::Container::ContainerBase<Driver, Container, Application, Artifact> {
      friend class Port;
      friend class Driver;
      friend class Artifact;

    private:
      OCPI::OCL::Device& m_device; // owned by the container

    protected:
      Container(OCPI::OCL::Device &device, const ezxml_t config = NULL,
		const OCPI::Util::PValue *params = NULL);
    public:
      ~Container();

      bool supportsImplementation(OCPI::Util::Worker &i);
      OCPI::Container::Container::DispatchRetCode dispatch(DataTransfer::EventManager* event_manager)
        throw (OU::EmbeddedException);

      OCPI::Container::Artifact &
	createArtifact(OCPI::Library::Artifact &lart, const OCPI::API::PValue *artifactParams);
      OCPI::API::ContainerApplication *
	createApplication(const char *name, const OCPI::Util::PValue *props)
	throw ( OCPI::Util::EmbeddedException );

      bool needThread() { return true; }
      bool portsInProcess() { return true; }
      OCPI::OCL::Device &device() { return m_device; }
#if 0
      void loadArtifact(const std::string &pathToArtifact,
			const OCPI::API::PValue* artifactParams);
      void unloadArtifact(const std::string& pathToArtifact);
#endif
    }; // End: class Container
    extern const char *ocl;
    class Driver : public OCPI::Container::DriverBase<Driver, Container, ocl> {
      friend class ExternalPort;

    public:
      Driver();
      static uint8_t s_logLevel;
      // This is the standard driver discovery routine
      unsigned search(const OCPI::API::PValue*, const char** exclude, bool discoveryOnly);
      // This is our special one that does some extra stuff...
      unsigned search(const OCPI::API::PValue*, const char **exclude, bool discoveryOnly,
		      const char *type, Device **found, std::set<std::string> *targets);
      // Find a device of this type (for compilers)
      Device &find(const char *target);
      OCPI::Container::Container* probeContainer(const char* which, std::string &error,
						 const OCPI::API::PValue* props);
      Device *open(const char *name, bool verbose, bool print, std::string &error);
    };
  }
}

#endif
