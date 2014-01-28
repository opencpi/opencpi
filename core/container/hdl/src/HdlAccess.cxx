#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "OcpiOsAssert.h"
#include "OcpiOsMisc.h"
#include "OcpiUuid.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
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
      if (bytes >= 2 && !(((intptr_t)to8 | offset) & 1)) {
	uint16_t *to16 = (uint16_t *)to8;
	volatile uint16_t *from16 = (uint16_t *)from8;
	do
	  *to16++ = *from16++;
	while ((bytes -= 2) >= 2);
	to8 = (uint8_t*)to16;
	from8 = (uint8_t *)from16;
      }
      while (bytes)
	*to8++ = *from8++, bytes--;
    }
    
    void Access::
    setBytes(RegisterOffset offset, const uint8_t *from8, size_t bytes) const {
      volatile uint8_t *to8 = m_registers + offset;
      ocpiDebug("setBytes %p off %"PRIx64" from %p to %p bytes %zx",
		this, (uint64_t)offset, from8, to8, bytes);
      if (bytes >= 8 && !(((intptr_t)from8 | offset) & 7)) {
	ocpiDebug("setBytes 64 bits: %zx", bytes);
	uint64_t *from64 = (uint64_t *)from8;
	volatile uint64_t *to64 = (uint64_t *)to8;
	do {
	  *to64++ = *from64++;
	} while ((bytes -= 8) >= 8);
	to8 = (uint8_t*)to64;
	from8 = (uint8_t *)from64;
      }
      if (bytes >= 4 && !(((intptr_t)from8 | offset) & 3)) {
	ocpiDebug("setBytes 32 bits: %zx", bytes);
	uint32_t *from32 = (uint32_t *)from8;
	volatile uint32_t *to32 = (uint32_t *)to8;
	do
	  *to32++ = *from32++;
	while ((bytes -= 4) >= 4);
	to8 = (uint8_t*)to32;
	from8 = (uint8_t *)from32;
      }
      if (bytes >= 2 && !(((intptr_t)from8 | offset) & 1)) {
	ocpiDebug("setBytes 16 bits: %zx", bytes);
	uint16_t *from16 = (uint16_t *)from8;
	volatile uint16_t *to16 = (uint16_t *)to8;
	do {
	  //	  ocpiDebug("setBytes 16 bits before : %zx offset %zx val %x", bytes, to8 - (m_registers + offset), *to16);
	  *to16 = *from16++;
	  //	  ocpiDebug("setBytes 16 bits after  : %zx offset %zx val %x", bytes, to8 - (m_registers + offset), *to16);
	  to16++;
	} while ((bytes -= 2) >= 2);
	to8 = (uint8_t*)to16;
	from8 = (uint8_t *)from16;
      }
      if (bytes)
	ocpiDebug("setBytes 8 bits: %zx", bytes);
      while (bytes)
	*to8++ = *from8++, bytes--;
    }
  }
}
