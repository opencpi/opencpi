/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This file defines the control plane access classes, which is the basis for different
// implementations of the cHDL control plane.

#ifndef HDL_ACCESS_H
#define HDL_ACCESS_H

#include <inttypes.h>
#include <cstddef>
#include "OcpiUtilMisc.h"
#include "XferEndPoint.h"

namespace OCPI {
  namespace HDL {
    // FIXME:  when do we check for unexpected errors when the worker said they don't produce errors?
    // FIXME:  perhaps after setup is done?  Should a control op notice the error?
    // FIXME:  after a bunch of register settings?  afterconfig?
    typedef size_t RegisterOffset;
    class Accessor {
    public:
      virtual ~Accessor() {}
      virtual uint32_t get(RegisterOffset offset, size_t bytes, uint32_t *status = NULL) = 0;
      virtual uint64_t get64(RegisterOffset, uint32_t *status = NULL) = 0;
      virtual void
	getBytes(RegisterOffset, uint8_t *, size_t, size_t, uint32_t *status = NULL,
		 bool string = false) = 0,
	set64(RegisterOffset, uint64_t, uint32_t *status = NULL) = 0,
	setBytes(RegisterOffset, const uint8_t *, size_t, size_t, uint32_t *status = NULL) = 0,
	set(RegisterOffset offset, size_t bytes, uint32_t data, uint32_t *status = NULL) = 0;
    };
    class Access {
      friend class WciControl;
      volatile uint8_t *m_registers; // the memory mapped virtual address of the registers
      DtOsDataTypes::Offset m_base;  // the base of the "registers" in their physical address space
      Accessor *m_accessor;          // when no virtual pointer access, the object that does the access
      bool      m_child;             // If I am derived from another access
      //      volatile uint8_t *m_buffers;   // for the data plane until enet dp is available

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
      void getBytes(RegisterOffset offset, uint8_t *to8, size_t bytes, size_t elementBytes,
		    bool string) const;
      void setBytes(RegisterOffset offset, const uint8_t *from8, size_t bytes,
		    size_t elementTypes) const;
      inline uint8_t get8RegisterOffset(size_t offset) const {
	ocpiDebug("get8RegisterOffset %p %zx", m_registers, offset);
	uint8_t val =
	  m_registers ? *(volatile uint8_t *)(m_registers + offset) :
	  (uint8_t)m_accessor->get(m_base + offset, sizeof(uint8_t));
	ocpiDebug("get8RegisterOffset %p %zx -> %x", m_registers, offset, val);
	return val;
      }
      inline uint16_t get16RegisterOffset(size_t offset) const {
	ocpiDebug("get16RegisterOffset %p %zx", m_registers, offset);
	uint16_t val =
	  m_registers ? *(volatile uint16_t *)(m_registers + offset) :
	  (uint16_t)m_accessor->get(m_base + offset, sizeof(uint16_t));
	ocpiDebug("get16RegisterOffset %p %zx -> %x", m_registers, offset, val);
	return val;
      }
      inline uint32_t get32RegisterOffset(size_t offset) const {
	ocpiDebug("get32RegisterOffset %p %zx", m_registers, offset);
	uint32_t val = m_registers ? *(volatile uint32_t *)(m_registers + offset) :
	  m_accessor->get(m_base + offset, sizeof(uint32_t));
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
	  m_accessor->set(m_base + offset, sizeof(uint8_t), val);
      }
      inline void set16RegisterOffset(size_t offset, uint16_t val) const {
	ocpiDebug("set16RegisterOffset %p %zx %x", m_registers, offset, val);
	if (m_registers)
	  *(volatile uint16_t *)(m_registers + offset) = val;
	else
	  m_accessor->set(m_base + offset, sizeof(uint16_t), val);
      }
      inline void set32RegisterOffset(size_t offset, uint32_t val) const {
	ocpiDebug("set32RegisterOffset %p %zx %x", m_registers, offset, val);
	if (m_registers)
	  *(volatile uint32_t *)(m_registers + offset) = val;
	else
	  m_accessor->set(m_base + offset, sizeof(uint32_t), val);
      }
      inline void set64RegisterOffset(size_t offset, uint64_t val) const{
	ocpiDebug("set64RegisterOffset %p %zx %" PRIx64, m_registers, offset, val);
	if (m_registers)
	  *(volatile uint64_t *)(m_registers + offset) = val;
	else
	  m_accessor->set64(m_base + offset, val);
      }
      inline void getBytesRegisterOffset(size_t offset, uint8_t *bytes,  size_t size,
					 size_t elementBytes, bool string = false) const {
	ocpiDebug("getBytesRegisterOffset %p %zx sz %zx", m_registers, offset, size);
	if (m_registers)
	  getBytes(offset, bytes, size, elementBytes, string);
	else
	  m_accessor->getBytes(m_base + offset, bytes, size, elementBytes, NULL, string);
      }
      inline void setBytesRegisterOffset(size_t offset, const uint8_t *bytes, size_t size,
					 size_t elementBytes) const {
	ocpiDebug("setBytesRegisterOffset %p %zx sz %zx", m_registers, offset, size);
	if (m_registers)
	  setBytes(offset, bytes, size, elementBytes);
	else
	  m_accessor->setBytes(m_base + offset, bytes, size, elementBytes);
      }
#define get8Register(m, type) get8RegisterOffset(offsetof(type, m))
#define get16Register(m, type) get16RegisterOffset(offsetof(type, m))
#define get32Register(m, type) get32RegisterOffset(offsetof(type, m))
#define get64Register(m, type) get64RegisterOffset(offsetof(type, m))
#define set8Register(m, type, val) set8RegisterOffset(offsetof(type, m), (val))
#define set16Register(m, type, val) set16RegisterOffset(offsetof(type, m), (val))
#define set32Register(m, type, val) set32RegisterOffset(offsetof(type, m), (val))
#define set64Register(m, type, val) set64RegisterOffset(offsetof(type, m), (val))
#define getRegisterBytes(m, buf, type, elementBytes, string)			\
      getBytesRegisterOffset(offsetof(type, m), (uint8_t*)(buf), sizeof(((type*)0)->m), \
			     elementBytes, string)
#define setRegisterBytes(m, buf, type, elementBytes)					\
      setBytesRegisterOffset(offsetof(type, m), (const uint8_t*)(buf), sizeof(((type*)0)->m), \
			     elementBytes)
#define offsetRegister(m, type) physOffset(offsetof(type, m))

    };
  }
}
#endif
