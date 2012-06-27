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
	OCPI::HDL::PCI::Driver,
	OCPI::HDL::Ether::Driver,
	Access, // for temporary probing
	virtual protected OCPI::Util::SelfMutex
    {
      /*
      friend class ExternalPort; // for pcimemfd
      // The fd for PCI DMA and PIO mapped memory, until we have a driver to restrict it.
      const OCPI::API::PValue *m_props; // a temporary during discovery
      int m_pciMemFd;
      OCPI::PCI::Bar m_bar1;
      void *m_bar1Vaddr;
      OCPI::OS::Ether *m_socket; // master socket for all containers to use
      need a set of sockets for different interfaces
      need a read call so a container can ask to receive for its address
      need queues so that mutexed received that receive the other guys packet can queue it up for
      them so that when they come in they get it.  Perhaps this also means that we can swap receive buffers,
	i.e. assuming there is only one outstanding.  So if I get yours, I just swap with you, and take yours.
	so we mutex the receive, and then receive, then maybe swap, etc. and if we swap, unlock the mutex
    in the loop.
  and the OS receive call should take a payload length max so we can allocate small control plane packets. 
  let's try to use the core mutex, although it would hange in receive - but not with a timeout...

      
      bool open(const char *name, std::string &err);
      void close();
#if 0
      // Create a container
      OCPI::Container::Container *
      createPCI(const char *name, OCPI::PCI::Bar *bars, void *bar0, void *bar1,
		const OCPI::API::PValue *props);

      // Create a container
      OCPI::Container::Container *
      createEther(const char *name, EtherAccessor *acc, const OCPI::API::PValue *props);

      // PCI::Bar *bars, unsigned nbars,
      // Create a dummy container using shm_open for the device
      OCPI::Container::Container *
      createDummy(const char *name, const char *df, const OCPI::API::PValue *);
#endif
      OCPI::Container::Container *
      tryPCI(const char *name, OCPI::PCI::Bar *bars, unsigned nbars, std::string &err);

      OCPI::Container::Container *
      tryEther(const char *name, OCPI::OS::Ether::Address &addr,
	       OCPI::OS::Ether::Socket &socket, std::string &err);

      // Callback function for PciScanner::search which creates a container
      bool
      foundPCI(const char *name, OCPI::PCI::Bar *bars, unsigned nbars);

      // Callback function for EtherScanner::search which creates a container
      bool
      foundEther(const char *name, OCPI::OS::Ether::Interface &ifc, OCPI::OS::Ether::Address &addr,
		 const uint8_t *data, OCPI::OS::Ether::Socket &socket);

      */
      const OCPI::Util::PValue *m_params; // a temporary during discovery
      bool checkAdmin(const char *name, std::string &err);
    public:
      void print(const char *name, Access &access);
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the
      // "found" method.
      unsigned search(const OCPI::API::PValue*, const char **exclude);
      virtual bool found(const char *name, Access &cAccess, Access &dAccess,
			 std::string &endpoint, std::string &error);
      // Probe a specific container
      OCPI::Container::Container *probeContainer(const char *which, const OCPI::API::PValue *props);
      bool
      open(const char *which, std::string &name, Access &cAccess, Access &dAccess,
	   std::string &endpoint, std::string &err);
      void close();

      // Create an actual container.
      static OCPI::Container::Container *
      createContainer(const char *name,
		      Access &controlAccess, // control plane accessor for the container, copied in
		      Access &dataAccess,    // data plane accessor for the container, copied in
		      std::string &endpoint, // data plane endpoint for other containers.
		      ezxml_t config = NULL, const OCPI::Util::PValue *params = NULL);
    };
  }
}
#endif
