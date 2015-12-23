// This file defines the control plane access classes, which is the basis for different
// implementations of the cHDL control plane.

#ifndef HDL_ACCESS_H
#define HDL_ACCESS_H

#include <inttypes.h>
#include <cstddef>
#include "OcpiUtilMisc.h"
#include "DtOsDataTypes.h"

namespace OCPI {
  namespace HDL {
    // FIXME:  when do we check for unexpected errors when the worker said they don't produce errors?
    // FIXME:  perhaps after setup is done?  Should a control op notice the error?
    // FIXME:  after a bunch of register settings?  afterconfig?
    typedef size_t RegisterOffset;
    class Accessor {
    public:
      virtual ~Accessor() {}
      // When the status pointer is zero, throw the exception if it is non-zero
      virtual uint64_t get64(RegisterOffset, uint32_t *status = NULL) = 0;
      virtual uint32_t get32(RegisterOffset, uint32_t *status = NULL) = 0;
      virtual uint16_t get16(RegisterOffset, uint32_t *status = NULL) = 0;
      virtual uint8_t  get8(RegisterOffset,  uint32_t *status = NULL) = 0;
      // the rest return status
      virtual void getBytes(RegisterOffset, uint8_t *, size_t, size_t,
			    uint32_t *status = NULL) = 0;
      virtual void set64(RegisterOffset, uint64_t, uint32_t *status = NULL) = 0;
      virtual void set32(RegisterOffset, uint32_t, uint32_t *status = NULL) = 0;
      virtual void set16(RegisterOffset, uint16_t, uint32_t *status = NULL) = 0;
      virtual void set8(RegisterOffset, uint8_t, uint32_t *status = NULL) = 0;
      virtual void setBytes(RegisterOffset, const uint8_t *, size_t, size_t,
			    uint32_t *status = NULL) = 0;
    };
    class Access {
      friend class WciControl;
      volatile uint8_t *m_registers; // the memory mapped virtual address of the registers
      DtOsDataTypes::Offset m_base;  // the base of the "registers" in their physical address space
      Accessor *m_accessor;          // when no virtual pointer access, the object that does the access
      bool      m_child;             // If I am derived from another access
      //      volatile uint8_t *m_buffers;   // sort of a hack for the data plane until enet dp is available

    public:
      Access(Access &other);
      Access(volatile uint8_t *registers = NULL,  Accessor *accessor = NULL,
	     RegisterOffset base = 0); //, volatile uint8_t *buffers = NULL);
      ~Access();
      inline volatile uint8_t *registers() const { return m_registers; }
      inline bool usable() const { return m_registers || m_accessor; }
      void
      setAccess(volatile uint8_t *registers,  Accessor *accessor = NULL,
		RegisterOffset base = 0, bool child = false);
      void
      closeAccess();

      // Given that I have registers already set up,
      // set up the (other, subsidiary) offsettee to have registers at an offset in my space
      void offsetRegisters(Access &offsettee, size_t offset);

      // Return the offset in the endpoint physical window of this offset in the accessor
      inline DtOsDataTypes::Offset physOffset(size_t offset) {
	return m_base + OCPI_UTRUNCATE(DtOsDataTypes::Offset, offset);
      }
      // The optimization here is to reduce the number of memory references, hence no unrolling
      void getBytes(RegisterOffset offset, uint8_t *to8, size_t bytes,
		    size_t elementBytes) const;

      void setBytes(RegisterOffset offset, const uint8_t *from8, size_t bytes,
		    size_t elementTypes) const;
      inline uint8_t get8RegisterOffset(size_t offset) const {
	ocpiDebug("get8RegisterOffset %p %zx", m_registers, offset);
	uint8_t val = m_registers ? *(volatile uint8_t *)(m_registers + offset) :
	  m_accessor->get8(m_base + offset);
	ocpiDebug("get8RegisterOffset %p %zx -> %x", m_registers, offset, val);
	return val;
      }
      inline uint16_t get16RegisterOffset(size_t offset) const {
	ocpiDebug("get16RegisterOffset %p %zx", m_registers, offset);
	uint16_t val = m_registers ? *(volatile uint16_t *)(m_registers + offset) :
	  m_accessor->get16(m_base + offset);
	ocpiDebug("get16RegisterOffset %p %zx -> %x", m_registers, offset, val);
	return val;
      }
      inline uint32_t get32RegisterOffset(size_t offset) const {
	ocpiDebug("get32RegisterOffset %p %zx", m_registers, offset);
	uint32_t val = m_registers ? *(volatile uint32_t *)(m_registers + offset) :
	  m_accessor->get32(m_base + offset);
	ocpiDebug("get32RegisterOffset %p %zx -> %x", m_registers, offset, val);
	return val;
      }
      inline uint64_t get64RegisterOffset(size_t offset) const {
	ocpiDebug("get64RegisterOffset %p %zx", m_registers, offset);
	uint64_t val = m_registers ? *(volatile uint64_t *)(m_registers + offset) :
	  m_accessor->get64(m_base + offset);
	ocpiDebug("get64RegisterOffset %p %zx -> %" PRIx64, m_registers, offset, val);
	return val;
      }
      inline void set8RegisterOffset(size_t offset, uint8_t val) const {
	ocpiDebug("set8RegisterOffset %p %zx %x", m_registers, offset, val);
	if (m_registers)
	  *(volatile uint8_t *)(m_registers + offset) = val;
	else
	  m_accessor->set8(m_base + offset, val);
      }
      inline void set16RegisterOffset(size_t offset, uint16_t val) const {
	ocpiDebug("set16RegisterOffset %p %zx %x", m_registers, offset, val);
	if (m_registers)
	  *(volatile uint16_t *)(m_registers + offset) = val;
	else
	  m_accessor->set16(m_base + offset, val);
      }
      inline void set32RegisterOffset(size_t offset, uint32_t val) const {
	ocpiDebug("set32RegisterOffset %p %zx %x", m_registers, offset, val);
	if (m_registers)
	  *(volatile uint32_t *)(m_registers + offset) = val;
	else
	  m_accessor->set32(m_base + offset, val);
      }
      inline void set64RegisterOffset(size_t offset, uint64_t val) const{
	ocpiDebug("set64RegisterOffset %p %zx %" PRIx64, m_registers, offset, val);
	if (m_registers)
	  *(volatile uint64_t *)(m_registers + offset) = val;
	else
	  m_accessor->set64(m_base + offset, val);
      }
      inline void getBytesRegisterOffset(size_t offset, uint8_t *bytes,  size_t size,
					 size_t elementBytes) const {
	ocpiDebug("getBytesRegisterOffset %p %zx sz %zx", m_registers, offset, size);
	if (m_registers)
	  getBytes(offset, bytes, size, elementBytes);
	else
	  m_accessor->getBytes(m_base + offset, bytes, size, elementBytes);
      }
      inline void setBytesRegisterOffset(size_t offset, const uint8_t *bytes, size_t size,
					 size_t elementBytes) const {
	ocpiDebug("setBytesRegisterOffset %p %zx sz %zx", m_registers, offset, size);
	if (m_registers)
	  setBytes(offset, bytes, size, elementBytes);
	else
	  m_accessor->setBytes(m_base + offset, bytes, size, elementBytes);
      }
#define get32Register(m, type) get32RegisterOffset(offsetof(type, m))
#define get64Register(m, type) get64RegisterOffset(offsetof(type, m))
#define set32Register(m, type, val) set32RegisterOffset(offsetof(type, m), (val))
#define set64Register(m, type, val) set64RegisterOffset(offsetof(type, m), (val))
#define getRegisterBytes(m, buf, type, elementBytes)					\
      getBytesRegisterOffset(offsetof(type, m), (uint8_t*)(buf), sizeof(((type*)0)->m), \
			     elementBytes)
#define setRegisterBytes(m, buf, type, elementBytes)					\
      setBytesRegisterOffset(offsetof(type, m), (const uint8_t*)(buf), sizeof(((type*)0)->m), \
			     elementBytes)
#define offsetRegister(m, type) physOffset(offsetof(type, m))

    };
  }
}
#endif
