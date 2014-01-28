#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <zlib.h>
#include "OcpiUuid.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "HdlDevice.h"

namespace OCPI {
  namespace HDL {
    namespace OE = OCPI::Util::EzXml;
    namespace OU = OCPI::Util;

    // The derived class will set up accessors after this constructor is done
    // So we can't perform accesses until that time, which is the "init" call.
    Device::
    Device(std::string &name, const char *protocol)
      : m_metadata(NULL), m_implXml(NULL), m_old(false), m_name(name), m_protocol(protocol), m_isAlive(true),
	m_pfWorker(NULL) {
    }
    Device::
    ~Device() {
      delete m_pfWorker;
      if (m_implXml)
	ezxml_free(m_implXml);
      if (m_metadata)
	free((void*)m_metadata);
    }

    static sigjmp_buf jmpbuf;
    static void catchBusError(int) { siglongjmp(jmpbuf, 1); }
    // Called from derived constructor after accessors have been set up.
    // Also called after bitstream loading.
    // The forLoad arg is saying that this is done prior to loading so
    // the device doesn't really have to be happy
    bool Device::
    init(std::string &err) {
      uint64_t magic;
      sig_t old; // FIXME: we could make this thread safe
      try {
	if (sigsetjmp(jmpbuf, 1) == 0) {
	  old = signal(SIGBUS, catchBusError);
	  magic = m_cAccess.get64Register(magic, OccpAdminRegisters);
	} else {
	  ocpiBad("HDL Device '%s' gets a bus error on probe: ", m_name.c_str());
	  err = "bus error on probe";
	}
      } catch (...) {
	ocpiBad("HDL Device '%s' gets access exception on probe: ", m_name.c_str());
	err = "access exception on probe";
      }
      signal(SIGBUS, old);
      if (err.size())
	return true;
      // Shuffle endianness here
      if (magic != OCCP_MAGIC) {
	ocpiBad("HDL Device '%s' responds, but the OCCP signature: "
		"magic: 0x%" PRIx64 " (sb 0x%" PRIx64 ")", m_name.c_str(), magic, OCCP_MAGIC);
	err = "Magic numbers in admin space do not match";
	return true;
      }
      if (m_pfWorker) {
	m_old = false;
	m_pfWorker->init(true);
      } else
	m_pfWorker = new WciControl(*this, "platform", "pf_i", 0, true);
      // Need to conditionalize this
      if ((m_pfWorker->controlOperation(OU::OpInitialize, err)) ||
	  (m_pfWorker->controlOperation(OU::OpStart, err))) {
	// Compatibility hack
	m_old = true;
	err.clear();
	ocpiInfo("For HDL Device '%s' no platform worker responds.  Assuming old-style bitstream.",
		 m_name.c_str());
	return false;
      }
      return false;
    }
    void Device::
    getUUID() {
      HdlUUID myUUIDtmp;
      if (m_old)
	m_cAccess.getRegisterBytes(admin.uuid, &myUUIDtmp, OccpSpace);
      else
	m_pfWorker->m_properties.getBytesRegisterOffset(0, (uint8_t *)&myUUIDtmp, sizeof(HdlUUID));
      // Fix the endianness
      for (unsigned n = 0; n < sizeof(HdlUUID); n++)
	((uint8_t*)&m_UUID)[n] = ((uint8_t *)&myUUIDtmp)[(n & ~3) + (3 - (n&3))];
      memcpy(m_loadedUUID, m_UUID.uuid, sizeof(m_loadedUUID));
    }
    uint32_t Device::
    getRomWord(uint16_t n) {
      m_pfWorker->m_properties.set16RegisterOffset(sizeof(HdlUUID) + sizeof(uint64_t), n);
      return m_pfWorker->m_properties.get32RegisterOffset(sizeof(HdlUUID) + sizeof(uint64_t) +
							  sizeof(uint32_t));
    }
    static const unsigned NROMWORDS = 1024;
    static const unsigned MAXXMLBYTES = NROMWORDS * 10;
    static voidpf zalloc(voidpf , uInt items, uInt size) {
      return malloc(items * size);
    }
    static void zfree(voidpf , voidpf data) {
      free(data);
    }
    bool Device::
    getMetadata(std::vector<char> &xml, std::string &err) {
      uint32_t rom[NROMWORDS];
      if ((rom[0] = getRomWord(0)) != 1 ||
	  (rom[1] = getRomWord(1)) >= NROMWORDS*sizeof(uint32_t) ||
	  (rom[2] = getRomWord(2)) >= MAXXMLBYTES) {
	err = "Metadata ROM appears corrupted";
	return true;
      }
      xml.resize(rom[2]);
      rom[3] = getRomWord(3);
      uint16_t nWords = OCPI_UTRUNCATE(uint16_t, (rom[1] + sizeof(uint32_t) - 1)/sizeof(uint32_t));
      for (uint16_t n = 4; n < 4 + nWords; n++)
	rom[n] = getRomWord(n);
      z_stream zs;  
      zs.zalloc = zalloc;
      zs.zfree = zfree;
      zs.data_type = Z_TEXT;
      zs.next_in = (unsigned char *)&rom[4];
      zs.avail_in = (uInt)rom[1];
      zs.next_out = (unsigned char *)&xml[0];
      zs.avail_out = (uInt)rom[2];
      if (inflateInit(&zs) == Z_OK &&
	  inflate(&zs, Z_FINISH) == Z_STREAM_END &&
	  inflateEnd(&zs) == Z_OK) {
	if (zs.adler != rom[3] || zs.total_out != rom[2]) {
	  OU::format(err, "bad checksum or length on decompressed data: is %lx/%lx, "
		     "should be %"PRIx32"/%"PRIx32,
		     zs.adler, zs.total_out, rom[3], rom[2]);
	  return true;
	}
      } else {
	err = "Unsuccessful decompression from rom contents";
	return true;
      }
      return false;
    }

    // Called initially or after reloading
    // Return true on error
    bool Device::
    configure(ezxml_t config, std::string &error) {
      uint64_t magic = m_cAccess.get64Register(magic, OccpAdminRegisters);
      // Shuffle endianness here
      if (magic != OCCP_MAGIC) {
	ocpiBad("HDL Device '%s' responds, but the OCCP signature: "
		"magic: 0x%" PRIx64 " (sb 0x%" PRIx64 ")", m_name.c_str(), magic, OCCP_MAGIC);
	error = "Magic numbers in admin space do not match";
	return true;
      }
      getUUID();
      // Some generic initialization.
      time_t bd = m_UUID.birthday;
      char tbuf[30];
      ocpiInfo("HDL Device: %s, with bitstream birthday: %s", m_name.c_str(), ctime_r(&bd, tbuf));
      // Capture the UUID info that tells us about the platform
      unsigned n;
      for (n = 0; m_UUID.platform[n] && n < sizeof(m_UUID.platform); n++)
	;
      if (n > 2)
	m_platform.assign(m_UUID.platform, n);
      else if (m_UUID.platform[0] == '\240' && m_UUID.platform[1] == 0)
	m_platform = "ml605";
      for (n = 0; m_UUID.device[n] && n < sizeof(m_UUID.device); n++)
	;
      if (n > 2)
	m_part.assign(m_UUID.device, n);
      else if (m_UUID.device[0] == '`' && m_UUID.device[1] == 0)
	m_part = "xc6vlx240t";
      for (n = 0; m_UUID.load[n] && n < sizeof(m_UUID.load); n++)
	;
      if (n > 1)
	m_loadParams.assign(m_UUID.load, n);
	
      if (config) {
	// what do I not know about this?
	// usb port for jtag loading
	// part type to look for artifacts
	// esn for checking/asserting that
	OE::getOptionalString(config, m_esn, "esn");
	std::string platform, device;
	OE::getOptionalString(config, platform, "platform");
	OE::getOptionalString(config, platform, "device");
	if (!platform.empty() && platform != m_platform) {
	  OU::formatString(error, "Discovered platform (%s) doesn't match configured platform (%s)",
		  m_platform.c_str(), platform.c_str());
	  return true;
	}
	if (!device.empty() && device != m_part) {
	  OU::formatString(error, "Discovered device (%s) doesn't match configured device (%s)",
			   m_part.c_str(), device.c_str());
	  return true;
	}
	OE::getOptionalString(config, m_position, "position");
      }
      return false;
    }
    void Device::
    print() {
      OU::UuidString textUUID;
      OU::uuid2string(m_UUID.uuid, textUUID);

      time_t bsbd = m_UUID.birthday;

      char tbuf[30];
      ctime_r(&bsbd, tbuf);
      tbuf[strlen(tbuf)-1] = 0;

      printf("OpenCPI HDL device found: '%s': bitstream date %s, "
	     "platform \"%s\", part \"%s\", UUID %s\n",
	     m_name.c_str(), tbuf, m_platform.c_str(), m_part.c_str(), textUUID);
    }
    bool Device::
    isLoadedUUID(const std::string &uuid) {
      OU::UuidString parsed;
      OU::uuid2string(m_loadedUUID, parsed);
      ocpiDebug("UUID check: want %s have %s", uuid.c_str(), parsed);
      return uuid == parsed;
    }
    // friends
    void Device::
    getWorkerAccess(size_t index,
		    Access &worker,
		    Access &properties) {
      if (index >= OCCP_MAX_WORKERS)
	throw OU::Error("Invalid occpIndex property");
      // FIXME:  check runtime for connected worker
      m_cAccess.offsetRegisters(worker, (intptr_t)(&((OccpSpace*)0)->worker[index]));
      m_cAccess.offsetRegisters(properties,(intptr_t)(&((OccpSpace*)0)->config[index]));
    }
    void Device::
    releaseWorkerAccess(size_t /* index */,
			Access & /* worker */,
			Access & /* properties */) {
    }
  }
}
