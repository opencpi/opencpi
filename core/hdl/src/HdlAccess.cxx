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
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "OcpiOsAssert.h"
#include "OcpiOsMisc.h"
#include "OcpiUtilMisc.h"
#include "HdlAccess.h"

namespace OCPI {
  namespace HDL {
    namespace OU = OCPI::Util;

    Access::
    Access(volatile uint8_t *registers,  Accessor *accessor, RegisterOffset base)
    //,  volatile uint8_t *buffers)
      : m_accessor(NULL), m_child(false) {
      setAccess(registers, accessor, base); //, buffers);
    }
    // Take the content and ownership away from the other access structure
    Access::
    Access(Access &other) : m_accessor(NULL), m_child(false) {
      setAccess(other.m_registers, other.m_accessor); //, other.m_base);
    }

    Access::
    ~Access() {
      //      delete m_accessor;
    }

    void Access::
    setAccess(volatile uint8_t *registers,  Accessor *accessor,
	      RegisterOffset base, bool child) {
      if (!m_child)
	delete m_accessor;
      m_child = child;
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
			  m_base + offset, true);
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
      ocpiDebug("setBytes %p off %" PRIx64 " from %p to %p bytes %zx",
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
