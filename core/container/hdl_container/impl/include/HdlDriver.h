#ifndef HdlDriver_H
#define HdlDriver_H

#include <arpa/inet.h>
#include <string>
#include "OcpiPValue.h"
#include "OcpiContainerManager.h"
#include "EtherDriver.h"
#include "PciDriver.h"

namespace OCPI {
  namespace HDL {
    extern const char *hdl;

#if 0 // obsolete
**************
The issue is that the ether scanner has an open socket for discovery,
  and tryether uses the drivers inherited HasRegisters, which needs its own raw socket.
  Perhaps ether::search can share a socket with hdl::driver?
  We probably need to conserve this socket thingy.
  This means that different containers will be sending/receiving from different addresses.
  Which means at least that we really need that single socket, at least for now perprocess.
  The m_accessor of the driver's hasregisters is a pointer, so perhaps we really don't want an accessor
  per container anyway.  accessor and socket are different things.
  sooo, the accessor object has interesting state, so perhaps its just all about sharing sockets.
  So the accessors per container referene a higher order socket.
  That means we need to coordinate "receives" from this socket - for discovery, and per container.
#endif


    class Container;
    class Driver
      : public OCPI::Container::DriverBase<Driver, Container, hdl>,
	public OCPI::HDL::PCI::Driver,
	OCPI::HDL::Ether::Driver,
	Access, // for temporary probing
	virtual protected OCPI::Util::SelfMutex
    {
      const OCPI::Util::PValue *m_params; // a temporary during discovery
    public:
      void print(const char *name, Access &access);
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the
      // "found" method.
      unsigned search(const OCPI::API::PValue*, const char **exclude);
      bool found(Device &dev, std::string &error);
      // Probe a specific container
      OCPI::Container::Container *probeContainer(const char *which, const OCPI::API::PValue *props);
      OCPI::HDL::Device *
	open(const char *which, bool discovery, std::string &err);
      void close();

      // Create an actual container.
      static OCPI::Container::Container *
      createContainer(Device &dev, ezxml_t config = NULL, const OCPI::Util::PValue *params = NULL);
    };
  }
}
#endif
