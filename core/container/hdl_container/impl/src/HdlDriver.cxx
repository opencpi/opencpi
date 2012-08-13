/*
 * This file contains driver-level code that does not know about the guts of the 
 * container class.
 */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
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
    static bool
    checkAdmin(const std::string &name, Access &cAccess, std::string &error) {
      uint64_t magic = cAccess.get64Register(magic, OccpAdminRegisters);
      // Shuffle endianness here
      if (magic == OCCP_MAGIC)
	return true;
      ocpiBad("HDL Device '%s' responds, but the OCCP signature: "
	      "magic: 0x%" PRIx64 " (sb 0x%" PRIx64 ")", name.c_str(), magic, OCCP_MAGIC);
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

    // return true on ok.  caller must close to release resources.
    OCPI::HDL::Device *Driver::
    open(const char *which, std::string &err) {
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
      Device *dev = pci ? PCI::Driver::open(which, err) : Ether::Driver::open(which, err);
      if (checkAdmin(dev->name(), dev->cAccess(), err))
	return dev;
      delete dev;
      return NULL;
    }

    void Driver::
    close() {
      closeAccess();
      unlock();
    }

    bool Driver::
    found(Device &dev,  std::string &/*error*/) {
      bool printOnly;
      const char *name = dev.name().c_str();
      if (OU::findBool(m_params, "printOnly", printOnly) && printOnly) {
	print(name, dev.cAccess());
	delete &dev;
	return true;
      }
      ezxml_t config = getDeviceConfig(name);
      if (!config && !strncmp("PCI:", name, 4)) // compatibility
	config = getDeviceConfig(name+4);
      return createContainer(dev, config, m_params) != NULL;
    } 

    unsigned Driver::
    search(const OA::PValue *params, const char **exclude) {
      OU::SelfAutoMutex x(this); // protect m_params etc.
      unsigned count = 0;
      m_params = params;
      std::string error;
      count += Ether::Driver::search(params, exclude, error);
      if (error.size()) {
	ocpiBad("In HDL Container driver, got ethernet search error: %s", error.c_str());
	error.clear();
      }
      count += PCI::Driver::search(params, exclude, error);
      if (error.size())
	ocpiBad("In HDL Container driver, got pci search error: %s", error.c_str());
      return count;
    }
    
    OC::Container *Driver::
    probeContainer(const char *which, const OA::PValue *params) {
      std::string error;
      Device *dev;
      if ((dev = open(which, error))) {
	const char *name = dev->name().c_str();
	ezxml_t config = getDeviceConfig(name);
	if (!config)
	  config = getDeviceConfig(name);
	if (!config && !strncmp("PCI:", name, 4))
	  config = getDeviceConfig(name + 4);
	return createContainer(*dev, getDeviceConfig(name), params);
      }
      throw OU::Error("While probing %s: %s", which, error.c_str());
    }      

    OC::RegisterContainerDriver<Driver> driver;
  }
}
