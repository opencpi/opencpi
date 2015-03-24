#ifndef HdlContainer_H
#define HdlContainer_H
#include "Container.h"
#include "HdlDriver.h"

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
      OCPI::Time::Emit m_hwEvents;
      uint64_t m_lastTick;
      friend class WciControl;
      friend class Worker;
      friend class Driver;
      friend class Port;
      friend class Artifact;
    protected:
      Container(OCPI::HDL::Device &device, ezxml_t config = NULL, const OCPI::API::PValue *params = NULL);
    public:
      virtual ~Container();
      bool portsInProcess() { return false; }
      inline uint64_t getMyTicks() {
	return
	  m_device.isAlive() ? 
	  (m_lastTick = swap32(m_device.properties().get64RegisterOffset(sizeof(HdlUUID))) + hdlDevice().m_timeCorrection) :
	  m_lastTick;
      }
    protected:
      inline HDL::Device &hdlDevice() const { return m_device; }
    public:
      bool connectInside(OCPI::Container::BasicPort &in, OCPI::Container::BasicPort &out);
      void start();
      void stop();
      OCPI::Container::Artifact &
	createArtifact(OCPI::Library::Artifact &lart, const OCPI::API::PValue *artifactParams);
      OCPI::API::ContainerApplication *
	createApplication(const char *name, const OCPI::Util::PValue *props)
	throw ( OCPI::Util::EmbeddedException );
      bool needThread();
#if 0
      const char * preferredTransport(bool remote, bool producer, OCPI::RDT::PortRole &role,
				      uint32_t &options) const;
#endif
    };
  }
}
#endif
