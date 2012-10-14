#include "HdlAccess.h"

namespace OCPI {
  namespace HDL {
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
      m_base = base;
      //      m_buffers = buffers;
    }

    void Access::
    closeAccess() {
      //      delete m_accessor;
      m_accessor = NULL;
    }

    void Access::
    offsetRegisters(Access &offsettee, unsigned offset) {
      offsettee.setAccess(m_registers ? m_registers + offset : 0,
			  m_accessor,
			  m_base + offset);
    }

    void Access::
    getBytes(RegisterOffset offset, uint8_t *to8, unsigned bytes) const {
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
    setBytes(RegisterOffset offset, const uint8_t *from8, unsigned bytes) const {
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
      : m_name(name), m_protocol(protocol) {
    }
    Device::
    ~Device() {}
  }
}
