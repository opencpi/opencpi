#ifndef HdlDriver_H
#define HdlDriver_H

#include <arpa/inet.h>
#include <string>
#include "OcpiPValue.h"
#include "OcpiUtilEzxml.h"
#include "OcpiContainerManager.h"
#include "EtherDriver.h"
#include "PciDriver.h"
#include "SimDriver.h"
#include "BusDriver.h"

namespace OCPI {
  namespace HDL {
    extern const char *hdl;

    class Container;
    class Driver
      : public OCPI::Container::DriverBase<Driver, Container, hdl>,
	OCPI::HDL::PCI::Driver,
	OCPI::HDL::Ether::Driver,
	OCPI::HDL::Sim::Driver,
	OCPI::HDL::Bus::Driver,
	Access, // for temporary probing
	virtual protected OCPI::Util::SelfMutex
    {
      const OCPI::Util::PValue *m_params; // a temporary during discovery
      bool setup(Device &dev, ezxml_t &config, std::string &err);
    public:
      void print(const char *name, Access &access);
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the
      // "found" method.
      unsigned search(const OCPI::API::PValue*, const char **exclude, bool discoveryOnly);
      bool found(Device &dev, std::string &error);
      // Probe a specific container
      OCPI::Container::Container *probeContainer(const char *which, std::string &error,
						 const OCPI::API::PValue *props);
      OCPI::HDL::Device *
	open(const char *which, bool discovery, bool forLoad, std::string &err);
      void close();

      // Create an actual container.
      static OCPI::Container::Container *
      createContainer(Device &dev, ezxml_t config = NULL, const OCPI::Util::PValue *params = NULL);
    };
  }
}
#endif
