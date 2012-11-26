#ifndef HdlContainer_H
#define HdlContainer_H
#include <uuid/uuid.h>
#include "HdlOCCP.h"
#include "OcpiContainerInterface.h"

namespace OCPI {
  namespace HDL {
    class Application;
    class Artifact;
    // We inherit Access to simply ensure that control accesses are as fast as possible.
    class Container
      : public OCPI::Container::ContainerBase<Driver, Container, Application, Artifact>,
	private Access, public OCPI::Time::Emit::TimeSource {
      HDL::Device &m_device;        // the underlying device that we own
      //      DataTransfer::EndPoint &m_endpoint; // the data plane endpoint of the device
      std::string m_part, m_esn, m_position, m_loadParams;
      uuid_t m_loadedUUID;
      uint32_t m_timeCorrection;
      OCPI::Time::Emit m_hwEvents;
      friend class WciControl;
      friend class Driver;
      friend class Port;
      friend class Artifact;
    protected:
      Container(OCPI::HDL::Device &device, ezxml_t config = NULL, const OCPI::API::PValue *params = NULL);
    public:
      virtual ~Container();
      inline uint64_t getMyTicks() {
	return swap32(get64Register(admin.time, OccpSpace)) + m_timeCorrection;
      }
    private:
      static inline uint64_t swap32(uint64_t x) {return (x <<32) | (x >> 32); }
      void initTime();
    protected:
      bool isLoadedUUID(const std::string &uuid);
      void getWorkerAccess(unsigned index,
			   Access &worker,
			   Access &properties);
      void releaseWorkerAccess(unsigned index,
			       Access & worker,
			       Access & properties);
      inline HDL::Device &hdlDevice() { return m_device; }
    public:
      void start();
      void stop();
      OCPI::Container::Artifact &
	createArtifact(OCPI::Library::Artifact &lart, const OCPI::API::PValue *artifactParams);
      OCPI::API::ContainerApplication *
	createApplication(const char *name, const OCPI::Util::PValue *props)
	throw ( OCPI::Util::EmbeddedException );
      bool needThread();
    };
  }
}
#endif
