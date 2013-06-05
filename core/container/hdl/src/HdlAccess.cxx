#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "OcpiUuid.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "HdlAccess.h"

namespace OCPI {
  namespace HDL {
    namespace OE = OCPI::Util::EzXml;
    namespace OU = OCPI::Util;

    Access::
    Access(volatile uint8_t *registers,  Accessor *accessor, RegisterOffset base)
    //,  volatile uint8_t *buffers)
      : m_accessor(NULL) {
      setAccess(registers, accessor, base); //, buffers);
    }
    // Take the content and ownership away from the other access structure
    Access::
    Access(Access &other) : m_accessor(NULL) {
      setAccess(other.m_registers, other.m_accessor); //, other.m_base);
    }

    Access::
    ~Access() {
      //      delete m_accessor;
    }

    void Access::
    setAccess(volatile uint8_t *registers,  Accessor *accessor,
	      RegisterOffset base) { //, volatile uint8_t *buffers) {
      delete m_accessor;
      m_registers = registers;
      m_accessor = accessor;
      m_base = OCPI_UTRUNCATE(DtOsDataTypes::Offset, base);
      //      m_buffers = buffers;
    }

    void Access::
    closeAccess() {
      //      delete m_accessor;
      m_accessor = NULL;
    }

    void Access::
    offsetRegisters(Access &offsettee, size_t offset) {
      offsettee.setAccess(m_registers ? m_registers + offset : 0,
			  m_accessor,
			  m_base + offset);
    }

    void Access::
    getBytes(RegisterOffset offset, uint8_t *to8, size_t bytes) const {
      volatile uint8_t *from8 = m_registers + offset;
      if (bytes >= 8 && !(((intptr_t)to8 | offset) & 7)) {
	uint64_t *to64 = (uint64_t *)to8;
	volatile uint64_t *from64 = (uint64_t *)from8;
	do
	  *to64++ = *from64++;
	while ((bytes -= 8) >= 8);
	to8 = (uint8_t*)to64;
	from8 = (uint8_t *)from64;
      }
      if (bytes >= 4 && !(((intptr_t)to8 | offset) & 3)) {
	uint32_t *to32 = (uint32_t *)to8;
	volatile uint32_t *from32 = (uint32_t *)from8;
	do
	  *to32++ = *from32++;
	while ((bytes -= 4) >= 4);
	to8 = (uint8_t*)to32;
	from8 = (uint8_t *)from32;
      }
      while (bytes)
	*to8++ = *from8++, bytes--;
    }
    
    void Access::
    setBytes(RegisterOffset offset, const uint8_t *from8, size_t bytes) const {
      volatile uint8_t *to8 = m_registers + offset;
      if (bytes >= 8 && !(((intptr_t)from8 | offset) & 7)) {
	uint64_t *from64 = (uint64_t *)from8;
	volatile uint64_t *to64 = (uint64_t *)to8;
	do
	  *to64++ = *from64++;
	while ((bytes -= 8) >= 8);
	to8 = (uint8_t*)to64;
	from8 = (uint8_t *)from64;
      }
      if (bytes >= 4 && !(((intptr_t)from8 | offset) & 3)) {
	uint32_t *from32 = (uint32_t *)from8;
	volatile uint32_t *to32 = (uint32_t *)to8;
	do
	  *to32++ = *from32++;
	while ((bytes -= 4) >= 4);
	to8 = (uint8_t*)to32;
	from8 = (uint8_t *)from32;
      }
      while (bytes)
	*to8++ = *from8++, bytes--;
    }
    Device::
    Device(std::string &name, const char *protocol)
      : m_name(name), m_protocol(protocol), m_isAlive(true) {
    }
    Device::
    ~Device() {}

    void Device::
    getUUID() {
      HdlUUID myUUIDtmp;
      m_cAccess.getRegisterBytes(admin.uuid, &myUUIDtmp, OccpSpace);
      // Fix the endianness
      for (unsigned n = 0; n < sizeof(HdlUUID); n++)
	((uint8_t*)&m_UUID)[n] = ((uint8_t *)&myUUIDtmp)[(n & ~3) + (3 - (n&3))];
      memcpy(m_loadedUUID, m_UUID.uuid, sizeof(m_loadedUUID));
    }

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
      // Some generic initialization.
      time_t bd = m_cAccess.get32Register(admin.birthday, OccpSpace);
      char tbuf[30];
      ocpiInfo("HDL Device: %s, with bitstream birthday: %s", m_name.c_str(), ctime_r(&bd, tbuf));
      // Capture the UUID info that tells us about the platform
      getUUID();
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

      time_t bsbd;
      //      bsvbd = m_cAccess.get32Register(birthday, OccpAdminRegisters);
      bsbd = m_UUID.birthday;

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
  }
}
