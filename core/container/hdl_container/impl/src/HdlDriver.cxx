/*
 * This file contains driver-level code that does not know about the guts of the 
 * container class.
 */
#include <sys/mman.h>
#include <uuid/uuid.h>
// FIXME: integrate this into our UUID utility properly
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[50]; // darwin has 37 - lousy unsafe interface
#endif
#include "OcpiUtilMisc.h"
#include "OcpiPValue.h"
#include "HdlDriver.h"
#include "HdlContainer.h"
#include "HdlOCCP.h"

namespace OCPI {
  namespace HDL {
    namespace OC = OCPI::Container;
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OP = OCPI::HDL::PCI;


    const char *hdl = "hdl";
    bool Driver::
    checkAdmin(const char *name, std::string &error) {
      uint64_t magic = get64Register(admin.magic, OccpSpace);
      // Shuffle endianness here
      if (magic == OCCP_MAGIC)
	return true;
      ocpiBad("HDL Device '%s' respond, but the OCCP signature: "
	      "magic: 0x%llx (sb 0x%llx)", name, magic, OCCP_MAGIC);
      error = "Magic numbers in admin space do not match";
      return false;
    }

    void Driver::
    print(const char *name, Access &access) {
      HdlUUID myUUID_raw, myUUID;
      access.getRegisterBytes(uuid, &myUUID_raw, OccpAdminRegisters);
      // Capture the UUID info that tells us about the platform
      for (unsigned n = 0; n < sizeof(HdlUUID); n++)
	((uint8_t*)&myUUID)[n] = ((uint8_t *)&myUUID_raw)[(n & ~3) + (3 - (n&3))];
      char tbuf[30], tbuf1[30];
      char platform[sizeof(myUUID.platform)+1];
      char device[sizeof(myUUID.device)+1];
      char load[sizeof(myUUID.load)+1];
      uuid_string_t textUUID;
      time_t bsvbd, bsbd;
      strncpy(platform, myUUID.platform, sizeof(myUUID.platform));
      platform[sizeof(myUUID.platform)] = '\0';
      strncpy(device, myUUID.device, sizeof(myUUID.device));
      device[sizeof(myUUID.device)] = '\0';
      strncpy(load, myUUID.load, sizeof(myUUID.load));
      load[sizeof(myUUID.load)] = '\0';
      uuid_unparse_lower(myUUID.uuid, textUUID);
      bsvbd = access.get32Register(birthday, OccpAdminRegisters);
      bsbd = myUUID.birthday;
      ctime_r(&bsvbd, tbuf);
      tbuf[strlen(tbuf)-1] = 0;
      ctime_r(&bsbd, tbuf1);
      tbuf1[strlen(tbuf1)-1] = 0;
      printf("OpenCPI HDL platform found: '%s': BSV date %s, bitstream date %s, "
	     "platform \"%s\", device \"%s\", UUID %s\n",
	     name, tbuf, tbuf1, platform, device, textUUID);
    }

    /*
    bool Driver::
    foundPCI(const char *name, OP::Bar *bars, unsigned nbars) {
      std::string error;
      return tryPCI(name, bars, nbars, error) != NULL;
    }
    // setup the driver to be ready to access the platform.  Return true on ok
    bool
    openPCI(const char *name, std::string &err) {
      // Real probe
      PCI::Bar bars[2];
      unsigned nbars = 2;
      if (PCI::probe(name, OCFRP0_PCI_VENDOR, OCFRP0_PCI_DEVICE,
		     OCFRP0_PCI_CLASS, OCFRP0_PCI_SUBCLASS, bars, nbars, err))
	if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
	    bars[0].addressSize != 32 || bars[0].size != sizeof(OccpSpace))
	  OU::formatString(err,
			   "Found PCI device w/ good vendor/device/class, "
			   "but BARs are wrong; skipping it; use lspci");
	else {
	  void *bar0 = 0, *bar1 = 0;
	  // PCI config info looks good.  Now check the OCCP signature.
	  if (m_pciMemFd < 0 && (m_pciMemFd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
	    err = "Can't open /dev/mem for PCI device.  Are you using 'sudo -E'?";
	  else if ((bar0 = mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
				m_pciMemFd, bars[0].address)) == (void*)-1)
	    OU::formatString(err, "can't mmap /dev/mem for bar0 at address 0x%llx size 0x%llx",
			     bars[0].address, (unsigned long long)sizeof(OccpSpace));
#if 0
o	  else if ((bar1 = mmap(NULL, bars[1].size, PROT_READ|PROT_WRITE, MAP_SHARED,
				m_pciMemFd, bars[1].address)) == (void*)-1)
	    OU::formatString(err, "can't mmap /dev/mem for bar1 at address 0x%llx size 0x%llx",
			     bars[1].address, bars[1].address);
#endif
	  else {
	    // Set up the drivers accessor for PCI.
	    setRegisters((uint8_t*)bar0);
	    if (checkAdmin(name, err)) {
	      m_bar1 = bar[1];
	      return true;
	    }
	  }
	  if (bar0)
	    munmap(bar0, sizeof(OccpSpace));
#if 0
	  if (bar1)
	    munmap(bar1, bars[1].size);
#endif
	}
      return false;
    }
    closePCI() {
      if (m_registers) {
	munmap(m_registers, sizeof(OccpSpace));
	m_registers = NULL;
      }
      if (m_bar1Vaddr) {
	munmap(m_bar1Vaddr, m_bar1.size);
	m_bar1Vaddr = NULL;
      }
    }
    // This might be called during discovery or separately in a probe or open.
    // Thus there might be a socket ready to use, or not
    bool
    openEther(const char *name, std::string &err, OE::Ether *socket = NULL) {
      const char *slash = strchr(name, '/');
      unsigned n = slash ? slash - name : strlen(name);
      OE::Address addr(slash ? slash + 1 : name);
      OS::Ether::Socket *s = socket;
      
      if (addr.error())
	err = "invalid ethernet MAC address";
      else {
	if (!socket) {
	  // No socket, thus no broadcast discovery happened already
	  assert(!m_socket && !m_accessor);
	  std::string iname(name, n);
	  OE::Interface ifc(iname.c_str(), err);
	  if (err.empty()) {
	    s = new OS::Ether::Socket(ifc, addr, err, OCCP_ETHER_STYPE, OCCP_ETHER_MTYPE);
	    if (err)
	      delete s;
	    else
	      m_socket = s;
	  }
	  EtherControlNop nop;
	  initNop(nop);
	  EtherControlNopResponse response;
	  if (Ether::probe(which, OCCP_ETHER_MTYPE, OCCP_ETHER_STYPE, (uint8_t *)&nop+2, sizeof(nop)-2,
			   ifc, addr, (uint8_t*)&response+2, sizeof(response)-2, err) &&
	      checkNopResponse(response, err)) {
	    OS::Ether::Socket s(ifc, addr, err, OCCP_ETHER_MTYPE, OCCP_ETHER_STYPE);
	    if (err.empty() && (c = tryEther(which, addr, s, err)))
	      return c;
	  }
	  if (Ether::probe(which, OCCP_ETHER_MTYPE, OCCP_ETHER_STYPE, (uint8_t *)&nop+2, sizeof(nop)-2,
			   ifc, addr, (uint8_t*)&response+2, sizeof(response)-2, err) &&
	      checkNopResponse(response, err)) {
	  }
	  if (err.empty()) {
	    setRegisters(NULL, new EtherAccessor(s, addr));
	    return true;
	  }
	}
	return false;
      }
    }
    void
    closeEther() {
      delete m_socket;
      m_socket = NULL;
      delete m_accessor;
      m_accessor = NULL;
    }
    */
    // return true on ok.  caller must close to release resources.
    bool Driver::
    open(const char *which, std::string &name, Access &cAccess, Access &dAccess,
	 std::string &endpoint, std::string &err) {
      lock();
      bool pci = false;
      if (!strncasecmp("PCI:", which, 4)) {
	pci = true;
	which += 4;
      } else if (!strncasecmp("Ether:", which, 6)) {
	which += 6;
      } else {
	unsigned n;
	for (const char *cp = strchr(which, ':'); cp; n++, cp = strchr(cp+1, ':'))
	  ;
	if (n != 5)
	  pci = true;
      }
      if (pci ?
	  PCI::Driver::open(which, name, cAccess, dAccess, endpoint, err) :
	  Ether::Driver::open(which, name, cAccess, dAccess, endpoint, err))
	return true;
      return false;
    }
    void Driver::
    close() {
      closeAccess();
      unlock();
    }
    /*
    OC::Container *Driver::
    tryPCI(const char *name, OP::Bar *bars, unsigned nbars, std::string &err) {
      std::string fullName("PCI:");
      fullName += name;
      if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
          bars[0].addressSize != 32 || bars[0].size != sizeof(OccpSpace))
	err = "Found PCI device w/ good vendor/device/class, but bars are wrong; skipping it; use lspci";
      else {
        void *bar0 = 0, *bar1 = 0;
        // PCI config info looks good.  Now check the OCCP signature.
        if (m_pciMemFd < 0 && (s_pciMemFd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
          err = "Can't open /dev/mem";
        else if ((bar0 = mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
			      m_pciMemFd, bars[0].address)) == (void*)-1)
          err = "can't mmap /dev/mem for bar0";
        else if ((bar1 = mmap(NULL, bars[1].size, PROT_READ|PROT_WRITE, MAP_SHARED,
                              m_pciMemFd, bars[1].address)) == (void*)-1)
          err = "can't mmap /dev/mem for bar1";
        else {
	  // Set up the drivers accessor for PCI.
	  setRegisters((uint8_t*)bar0);
	  if (checkAdmin(name, err)) {
	    bool printOnly;
	    if (OU::findBool(s_props, "printOnly", printOnly) && printOnly)
	      print(fullName.c_str());
	    else {
	      uint64_t endpointPaddr, endpointSize, controlOffset, bufferOffset;
	      if (bars[0].address < bars[1].address) {
		endpointPaddr = bars[0].address;
		endpointSize = bars[1].address + bars[1].size - endpointPaddr;
		controlOffset = 0;
		bufferOffset = bars[1].address - bars[0].address;
	      } else {
		endpointPaddr = bars[1].address;
		endpointSize = bars[0].address + sizeof(OccpSpace) - endpointPaddr;
		controlOffset = bars[0].address - bars[1].address;
		bufferOffset = 0;
	      }
	      std::string endpoint;
	      const char *busId = "0";
	      OU::formatString(endpoint,
			       "ocpi-pci-pio:%s.%lld:%lld.3.10", busId,
			       (long long unsigned)endpointPaddr,
			       (long long unsigned)endpointSize);
	      return createContainer(fullName.c_str(), // container name, usually [PCI|Ether]:etc.
				     (uint8_t*)bar0,   // 0: virtual pointer to control space if available
				     NULL,            // 0: accessor object for control space if no vaddr
				     controlOffset,   // 0: offset in aggregated/single space
				     (uint8_t*)bar1,  // 1:
				     NULL,            // 1:
				     bufferOffset,    // 1:
				     endpoint,
				     getDeviceConfig(name),
				     s_props);
	    }
	  }
	}
	if (bar0)
	  munmap(bar0, sizeof(OccpSpace));
	if (bar1)
	  munmap(bar1, bars[1].size);
      }
      if (err.size())
	ocpiBad("While probing for '%s', got error: %s", name, err.c_str());
      return NULL;
    }


    bool Driver::
    foundEther(const char *which, OS::Ether::Interface &, OS::Ether::Address &addr,
	       const uint8_t *data, OS::Ether::Socket &socket) {
      std::string error, name;
      
      if (checkNopResponse(*(EtherControlNopResponse *)(data-2), error) &&
	  openEther(which, name, error, &socket)) {
	bool printOnly;
	if (OU::findBool(s_props, "printOnly", printOnly) && printOnly)
	  print(name);
	else
	  return createEther(name, addr, socket, error) != NULL;
      }
      return false;
    } 

    OC::Container *Driver::
    tryEther(const char *name, OS::Ether::Address &addr, OS::Ether::Socket &socket, std::string &err) {
      // We need an accessor to hand off to the container.
      EtherAccessor acc(socket, addr);
      setRegisters(NULL, &acc);
      std::string fullName("Ether:");
      fullName += name;
      if (checkAdmin(fullName.c_str(), err)) {
	bool printOnly;
	if (OU::findBool(s_props, "printOnly", printOnly) && printOnly)
	  print(fullName.c_str());
	else {
	  std::string endpoint = acc->addr().pretty();
	  return createContainer(fullName.c_str(),   // container name, usually [PCI|Ether]:etc.
				 NULL,               // 0: virtual pointer to control space if available
				 acc,                // 0: accessor object for control space if no vaddr
				 0,                  // 0: offset in aggregated/single space
				 NULL,               // 1:
				 acc,                // 1:
				 sizeof(OccpSpace),  // 1:
				 endpoint,
				 getDeviceConfig(name),
				 props);
	}
      }
      return NULL;
    }
*/
    bool Driver::
    found(const char *which, Access &cAccess, Access &dAccess, std::string &endpoint, std::string &error) {
      bool printOnly;
      if (OU::findBool(m_params, "printOnly", printOnly) && printOnly) {
	print(which, cAccess);
	return true;
      }
      return createContainer(which, cAccess, dAccess, endpoint, getDeviceConfig(which), m_params) != NULL;
    } 

    unsigned Driver::
    search(const OA::PValue *params, const char **exclude) {
      OU::SelfAutoMutex x(this); // protect m_params etc.
      unsigned count = 0;
      m_params = params;
      std::string error;
      count += Ether::Driver::search(params, exclude, error);
      if (error.empty())
	count += PCI::Driver::search(params, exclude, error);
      if (error.size())
	ocpiBad("In HDL Container driver, got search error: %s", error.c_str());
      return count;
    }
    
    OC::Container *Driver::
    probeContainer(const char *which, const OA::PValue *params) {
      std::string name, error, endpoint;
      Access dAccess;
      if (open(which, name, *this, dAccess, endpoint, error))
	return createContainer(which, *this, dAccess, endpoint, getDeviceConfig(which), params);
      throw OU::Error("While probing %s: %s", which, error.c_str());
    }      
    /*
      bool printOnly;
      if (OU::findBool(s_props, "printOnly", printOnly) && printOnly)
	print(name);
      else
	
	  createEther(fullName.c_str(), addr, socket, error) != NULL;
      std::string err;
      OC::Container *c;
      if (pci) {
	// Real probe
	PCI::Bar bars[2];
	unsigned nbars = 2;
	if (PCI::probe(which, OCFRP0_PCI_VENDOR, OCFRP0_PCI_DEVICE,
		       OCFRP0_PCI_CLASS, OCFRP0_PCI_SUBCLASS, bars, nbars, err) &&
	    (c = tryPCI(which, bars, nbars, err)))
	  return c;
      } else {
	EtherControlNop nop;
	initNop(nop);
	EtherControlNopResponse response;
	OS::Ether::Address addr;
	OS::Ether::Interface ifc;
	if (Ether::probe(which, OCCP_ETHER_MTYPE, OCCP_ETHER_STYPE, (uint8_t *)&nop+2, sizeof(nop)-2,
			 ifc, addr, (uint8_t*)&response+2, sizeof(response)-2, err) &&
	    checkNopResponse(response, err)) {
	    OS::Ether::Socket s(ifc, addr, err, OCCP_ETHER_MTYPE, OCCP_ETHER_STYPE);
	    if (err.empty() && (c = tryEther(which, addr, s, err)))
	      return c;
	}
      }
      if (err.size())
	throw OU::Error("Error probing '%s': %s", name, err.c_str());
      return NULL;
    }

#if 0
    // Create a dummy device emulated by a shared memory buffer
    OC::Container *Driver::
    createDummy(const char *name, const char *df, const OA::PValue *) {
      int fd;
      uint8_t *bar0, *bar1;
      fprintf(stderr, "DF: %s, Page %d, Occp %" PRIsize_t ", SC pagesize %lu off_t %" PRIsize_t " bd %" PRIsize_t "\n",
              df, getpagesize(), sizeof(OccpSpace), sysconf(_SC_PAGE_SIZE),
              sizeof(off_t), sizeof(OC::PortData));
      umask(0);
      ocpiCheck((fd = shm_open(df, O_CREAT | O_RDWR, 0666)) >= 0);
      ocpiCheck(ftruncate(fd, sizeof(OccpSpace) + 64*1024) >= 0);
      ocpiCheck((bar0 = (uint8_t*)mmap(NULL, sizeof(OccpSpace) + 64*1024,
                                       PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))
                != (uint8_t*)-1);
      bar1 = bar0 + sizeof(OccpSpace);
      return createContainer(name, 0, (volatile OccpSpace*)bar0,
			     sizeof(OccpSpace), bar1, 64*1024);
      (void)name; (void)df;
      return 0;
    }

    // Internal driver method
    // Return container pointer OR null, and if NULL, set "err" output arg.
    OC::Container *Driver::
    createPCI(const char *name, PCI::Bar *bars, void *bar0, void *bar1, const OU::PValue *props) {
      uint64_t endpointPaddr, endpointSize, controlOffset, bufferOffset;
      if (bars[0].address < bars[1].address) {
	endpointPaddr = bars[0].address;
	endpointSize = bars[1].address + bars[1].size - endpointPaddr;
	controlOffset = 0;
	bufferOffset = bars[1].address - bars[0].address;
      } else {
	endpointPaddr = bars[1].address;
	endpointSize = bars[0].address + sizeof(OccpSpace) - endpointPaddr;
	controlOffset = bars[0].address - bars[1].address;
	bufferOffset = 0;
      }
      std::string endpoint;
      const char *busId = "0";
      OU::formatString(endpoint,
		       "ocpi-pci-pio:%s.%lld:%lld.3.10", busId,
		       (long long unsigned)endpointPaddr,
		       (long long unsigned)endpointSize);
      return createContainer(name,            // container name, usually [PCI|Ether]:etc.
			     (uint8_t*)bar0,  // 0: virtual pointer to control space if available
			     NULL,            // 0: accessor object for control space if no vaddr
			     controlOffset,   // 0: offset in aggregated/single space
			     (uint8_t*)bar1,  // 1:
			     NULL,            // 1:
			     bufferOffset,    // 1:
			     endpoint,
			     getDeviceConfig(name),
			     props);
    }

    OC::Container *Driver::
    createEther(const char *name, EtherAccessor *acc, const OA::PValue *props) {
      std::string endpoint = acc->addr().pretty();
      return createContainer(name,               // container name, usually [PCI|Ether]:etc.
			     NULL,               // 0: virtual pointer to control space if available
			     acc,               // 0: accessor object for control space if no vaddr
			     0,                  // 0: offset in aggregated/single space
			     NULL,               // 1:
			     acc,               // 1:
			     sizeof(OccpSpace),  // 1:
			     endpoint,
			     getDeviceConfig(name),
			     props);
    }
#endif
    */
    OC::RegisterContainerDriver<Driver> driver;
  }
}
