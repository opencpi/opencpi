#ifndef OCPI_RCC_DRIVER_H
#define OCPI_RCC_DRIVER_H
#include <OcpiContainerManager.h>
namespace OCPI {
  namespace RCC {
    extern const char *rcc;
    class Container;
    class Driver : public OCPI::Container::DriverBase<Driver, Container, rcc> {
      //      OCPI::DataTransport::TransportGlobal *m_tpg_events, *m_tpg_no_events;
      unsigned m_count;
    public:
      Driver() throw();
      OCPI::Container::Container *
	probeContainer(const char *which, const OCPI::API::PValue *props)
	throw ( OCPI::Util::EmbeddedException );
      // Per driver discovery routine to create devices
      unsigned
	search(const OCPI::API::PValue* props, const char **exclude)
	throw ( OCPI::Util::EmbeddedException );
      ~Driver() throw ( );
    };
  }
}
#endif
