/*
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>
#include <lzma.h>
#include "HdlDevice.h"

#define USE_LZMA 1
namespace OCPI {
  namespace HDL {
    namespace OE = OCPI::Util::EzXml;
    namespace OU = OCPI::Util;

    // The derived class will set up accessors after this constructor is done
    // So we can't perform accesses until that time, which is the "init" call.
    Device::
    Device(const std::string &a_name, const char *a_protocol, const OU::PValue *params)
      : m_metadata(NULL), m_implXml(NULL), m_old(false), m_name(a_name), m_protocol(a_protocol),
	m_isAlive(true), m_pfWorker(NULL), m_tsWorker(NULL), m_isFailed(false), m_verbose(false),
	m_timeCorrection(0), m_endPoint(NULL) {
      OU::findBool(params, "verbose", m_verbose);
      memset((void*)&m_UUID, 0, sizeof(m_UUID));
    }
    Device::
    ~Device() {
      delete m_pfWorker;
      delete m_tsWorker;
      if (m_implXml)
	ezxml_free(m_implXml);
      if (m_metadata)
	free((void*)m_metadata);
    }

    static sigjmp_buf jmpbuf;
    static void catchBusError(int) { siglongjmp(jmpbuf, 1); }
    // Called from derived constructor after accessors have been set up.
    // Also called after bitstream loading.
    bool Device::
    init(std::string &err) {
      m_isAlive = false;
      sig_t old = signal(SIGBUS, catchBusError); // FIXME: we could make this thread safe
      uint64_t magic = 0x0BAD1BADDEADBEEF;
      try {
	if (sigsetjmp(jmpbuf, 1) == 0) {
	  magic = m_cAccess.get64Register(magic, OccpAdminRegisters);
	} else {
	  ocpiBad("HDL Device '%s' gets a bus error on probe: ", m_name.c_str());
	  err = "bus error on probe";
	}
      } catch (std::string &e) {
	ocpiBad("HDL Device '%s' gets error '%s' on probe: ", e.c_str(), m_name.c_str());
	err = "access exception on probe";
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
      if (!m_pfWorker) {
	ocpiDebug("HDL::Device::init: platform worker does not exist, first access in process");
	m_pfWorker = new WciControl(*this, "platform", "pf_i", 0, true);
	m_tsWorker = new WciControl(*this, "time_server", "ts_i", 1, true);
      }
      if (m_pfWorker->isReset()) {
	ocpiDebug("Platform worker is in reset, initializing it (unreset, initialize, start)");
	m_old = false;
	m_pfWorker->init(true, true);
	m_tsWorker->init(true, true);
	if ((m_pfWorker->controlOperation(OU::Worker::OpInitialize, err)) ||
	    (m_tsWorker->controlOperation(OU::Worker::OpInitialize, err)) ||
	    (m_pfWorker->controlOperation(OU::Worker::OpStart, err)) ||
	    (m_tsWorker->controlOperation(OU::Worker::OpStart, err)))
	  return true;
      }
      m_isAlive = true;
      if (configure(NULL, err))
	return true;
      return false;
    }
    void Device::
    getUUID() {
      HdlUUID myUUIDtmp;
      if (m_old)
	m_cAccess.getRegisterBytes(admin.uuid, &myUUIDtmp, OccpSpace, 8, false);
      else
	m_pfWorker->m_properties.getBytesRegisterOffset(0, (uint8_t *)&myUUIDtmp,
							sizeof(HdlUUID), 8);
      // Fix the endianness
      for (unsigned n = 0; n < sizeof(HdlUUID); n++)
	((uint8_t*)&m_UUID)[n] = ((uint8_t *)&myUUIDtmp)[(n & ~3) + (3 - (n&3))];
      memcpy(m_loadedUUID, m_UUID.uuid, sizeof(m_loadedUUID));
    }
    RomWord Device::
    getRomWord(uint16_t n) {
      m_pfWorker->m_properties.set16RegisterOffset(sizeof(HdlUUID) + sizeof(uint64_t), n);
      return OCPI_UTRUNCATE(RomWord,
			    m_pfWorker->m_properties.get32RegisterOffset(sizeof(HdlUUID) +
									 sizeof(uint64_t) +
									 sizeof(uint32_t)));
    }
    static const unsigned MAXXMLBYTES = ROM_NBYTES * 16;
#ifndef USE_LZMA
    static voidpf zalloc(voidpf , uInt items, uInt size) {
      return malloc(items * size);
    }
    static void zfree(voidpf , voidpf data) {
      free(data);
    }
#endif
    bool Device::
    getMetadata(std::vector<char> &xml, std::string &err) {
      RomWord rom[ROM_NWORDS];
      if ((rom[0] = getRomWord(0)) != 1 ||
	  (rom[1] = getRomWord(1)) >= ROM_NBYTES ||
	  (rom[2] = getRomWord(2)) >= MAXXMLBYTES) {
	OU::format(err, "Metadata ROM appears corrupted: 0x%x 0x%x 0x%x",
		   rom[0], rom[1], rom[2]);
	return true;
      }
      xml.resize(rom[2]);
      rom[3] = getRomWord(3);
      uint16_t nWords = OCPI_UTRUNCATE(uint16_t, (rom[1] + sizeof(RomWord) - 1)/sizeof(RomWord));
      for (uint16_t n = ROM_HEADER_WORDS; n < ROM_HEADER_WORDS + nWords; n++)
	rom[n] = getRomWord(n);
#if USE_LZMA
      lzma_ret lr;
      uint64_t memlimit = UINT64_MAX;
      size_t in_pos = 0, out_pos = 0;
      if ((lr = lzma_stream_buffer_decode(&memlimit, // ptr to max memory to use during decode
					  0,         // flags
					  NULL,      // allocator if not malloc/free
					  (uint8_t *)&rom[4],        // input buffer
					  &in_pos,   // updated input index
					  rom[1],   // size of input
					  (uint8_t *)&xml[0],       // output buffer
					  &out_pos,  // updated output index
					  rom[2])) == LZMA_OK) {
	if (out_pos != rom[2]) {
	  OU::format(err, "length on decompressed data: is %zx, "
		     "should be %" PRIx32, out_pos, rom[2]);
	  return true;
	}
	return false;
      }
      OU::format(err, "Unsuccessful lzma decompression from config ROM: %u", lr);
      for (unsigned i = 0; i < 10; i++)
	ocpiInfo("bad ROM[%u]: 0x%08x", i, rom[i]);
      return true;
#else
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
		     "should be %" PRIx32 "/%" PRIx32,
		     zs.adler, zs.total_out, rom[3], rom[2]);
	  return true;
	}
	return false;
      }
      err = "Unsuccessful decompression from rom contents";
      return true;
#endif
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
      ctime_r(&bd, tbuf);
      tbuf[strlen(tbuf)-1] = '\0'; // nuke \n
      ocpiInfo("HDL Device: %s, with bitstream birthday: %s", m_name.c_str(), tbuf);
      // Capture the UUID info that tells us about the platform
      unsigned n;
      for (n = 0; m_UUID.platform[n] && n < sizeof(m_UUID.platform); n++)
	;
      if (n > 2)
	m_platform.assign(m_UUID.platform, n);
      else if (m_UUID.platform[0] == '\240' && m_UUID.platform[1] == 0)
	m_platform = "ml605";
      if (!isprint(m_platform[0])) {
	ocpiInfo("HDL Device '%s' responds, but the platform type name is garbage: ",
		m_name.c_str());
	error = "Platform name in admin space is garbage";
	return true;
      }

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
	std::string myPlatform, device;
	OE::getOptionalString(config, myPlatform, "platform");
	OE::getOptionalString(config, myPlatform, "device");
	if (!myPlatform.empty() && myPlatform != m_platform) {
	  OU::formatString(error, "Discovered platform (%s) doesn't match configured platform (%s)",
		  m_platform.c_str(), myPlatform.c_str());
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

      printf("OpenCPI HDL device found: '%s': %s%s, "
	     "platform \"%s\", part \"%s\", UUID %s\n",
	     m_name.c_str(), bsbd ? "bitstream date " : "",
	     bsbd ? tbuf : "No loaded bitstream",
	     m_platform.c_str(), m_part.c_str(), textUUID);
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
		    Access &a_properties) {
      if (index >= OCCP_MAX_WORKERS)
	throw OU::Error("Invalid occpIndex property");
      // FIXME:  check runtime for connected worker
      m_cAccess.offsetRegisters(worker, (intptr_t)(&((OccpSpace*)0)->worker[index]));
      m_cAccess.offsetRegisters(a_properties,(intptr_t)(&((OccpSpace*)0)->config[index]));
    }
    void Device::
    releaseWorkerAccess(size_t /* index */,
			Access & /* worker */,
			Access & /* properties */) {
    }
    DataTransfer::EndPoint &Device::
    getEndPoint() {
      return m_endPoint ? *m_endPoint :
	*(m_endPoint = &DataTransfer::getManager().
	  allocateProxyEndPoint(m_endpointSpecific.c_str(),
				OCPI_UTRUNCATE(size_t, m_endpointSize)));
    }
    void Device::
    connect(DataTransfer::EndPoint &/*ep*/, OCPI::RDT::Descriptors &/*mine*/,
	    const OCPI::RDT::Descriptors &/*other*/) {
    }
    // Static
    void Device::
    initAdmin(OccpAdminRegisters &admin, const char *a_platform, HdlUUID &hdlUuid,
	      OU::UuidString *uuidString) {
      memset(&admin, 0, sizeof(admin));
#define unconst32(a) (*(uint32_t *)&(a))
#define unconst64(a) (*(uint64_t *)&(a))
      unconst64(admin.magic) = OCCP_MAGIC;
      unconst32(admin.revision) = 0;
      unconst32(admin.birthday) = OCPI_UTRUNCATE(uint32_t, time(0));
      unconst32(admin.pciDevice) = 0;
      unconst32(admin.attention) = 0;
      unconst32(admin.status) = 0;
      admin.scratch20 = 0xf00dface;
      admin.scratch24 = 0xdeadbeef;
      admin.control = 0;
      unconst32(admin.reset) = 0;
      unconst32(admin.timeStatus) = 0;
      admin.timeControl = 0;
      admin.time = 0;
      admin.timeDelta = 0;
      unconst32(admin.timeClksPerPps) = 0;
      unconst64(admin.present) = 1;
      unconst64(admin.attention) = 1;
      unconst32(admin.numRegions) = 1;
      unconst32(admin.regions[0]) = 0;
      OU::Uuid l_uuid;
      OU::generateUuid(l_uuid);
      if (uuidString) {
	OU::uuid2string(l_uuid, *uuidString);
	ocpiDebug("Emulator UUID: %s", *uuidString);
      }
      HdlUUID temp;
      temp.birthday = OCPI_UTRUNCATE(uint32_t, time(0) + 1);
      memcpy(temp.uuid, l_uuid, sizeof(l_uuid));
      strcpy(temp.platform, a_platform);
      strcpy(temp.device, "devemu");
      strcpy(temp.load, "ld");
      strcpy(temp.dna, "\001\002\003\004\005\006\007");
      for (unsigned n = 0; n < sizeof(HdlUUID); n++)
	((uint8_t *)&hdlUuid)[n] = ((uint8_t *)&temp)[(n & ~3) + (3 - (n & 3))];
    }
  }
}
