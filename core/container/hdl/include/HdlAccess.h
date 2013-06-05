// This file defines the control plane access classes, which is the basis for different
// implementations of the cHDL control plane.

#ifndef HDL_ACCESS_H
#define HDL_ACCESS_H
#include <cstdlib>
#include <stdint.h>
#include <string>
#include "OcpiUuid.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "DtOsDataTypes.h"
#include "HdlOCCP.h"
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
      virtual void getBytes(RegisterOffset, uint8_t *, size_t, uint32_t *status = NULL) = 0;
      virtual void set64(RegisterOffset, uint64_t, uint32_t *status = NULL) = 0;
      virtual void set32(RegisterOffset, uint32_t, uint32_t *status = NULL) = 0;
      virtual void set16(RegisterOffset, uint16_t, uint32_t *status = NULL) = 0;
      virtual void set8(RegisterOffset, uint8_t, uint32_t *status = NULL) = 0;
      virtual void setBytes(RegisterOffset, const uint8_t *, size_t, uint32_t *status = NULL) = 0;
    };
    class Access {
      friend class WciControl;
      volatile uint8_t *m_registers; // the memory mapped virtual address of the registers
      DtOsDataTypes::Offset m_base;  // the base of the "registers" in their physical address space
      Accessor *m_accessor;          // when no virtual pointer access, the object that does the access
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
		RegisterOffset base = 0); //, volatile uint8_t *buffers = NULL);
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
      void getBytes(RegisterOffset offset, uint8_t *to8, size_t bytes) const;

      void setBytes(RegisterOffset offset, const uint8_t *from8, size_t bytes) const;
      inline uint8_t get8RegisterOffset(size_t offset) const {
	return m_registers ? *(volatile uint8_t *)(m_registers + offset) :
	  m_accessor->get8(m_base + offset);
      }
      inline uint16_t get16RegisterOffset(size_t offset) const {
	return m_registers ? *(volatile uint16_t *)(m_registers + offset) :
	  m_accessor->get16(m_base + offset);
      }
      inline uint32_t get32RegisterOffset(size_t offset) const {
	return m_registers ? *(volatile uint32_t *)(m_registers + offset) :
	  m_accessor->get32(m_base + offset);
      }
      inline uint64_t get64RegisterOffset(size_t offset) const {
	return m_registers ? *(volatile uint64_t *)(m_registers + offset) :
	  m_accessor->get64(m_base + offset);
      }
      inline void set8RegisterOffset(size_t offset, uint8_t val) const {
	if (m_registers)
	  *(volatile uint8_t *)(m_registers + offset) = val;
	else
	  m_accessor->set8(m_base + offset, val);
      }
      inline void set16RegisterOffset(size_t offset, uint16_t val) const {
	if (m_registers)
	  *(volatile uint16_t *)(m_registers + offset) = val;
	else
	  m_accessor->set16(m_base + offset, val);
      }
      inline void set32RegisterOffset(size_t offset, uint32_t val) const {
	if (m_registers)
	  *(volatile uint32_t *)(m_registers + offset) = val;
	else
	  m_accessor->set32(m_base + offset, val);
      }
      inline void set64RegisterOffset(size_t offset, uint64_t val) const{
	if (m_registers)
	  *(volatile uint64_t *)(m_registers + offset) = val;
	else
	  m_accessor->set64(m_base + offset, val);
      }
      inline void getBytesRegisterOffset(size_t offset, uint8_t *bytes,  size_t size) const {
	if (m_registers)
	  getBytes(offset, bytes, size);
	else
	  m_accessor->getBytes(offset, bytes, size);
      }
      inline void setBytesRegisterOffset(size_t offset, const uint8_t *bytes, size_t size) const {
	if (m_registers)
	  setBytes(offset, bytes, size);
	else
	  m_accessor->setBytes(offset, bytes, size);
      }
#define get32Register(m, type) get32RegisterOffset(offsetof(type, m))
#define get64Register(m, type) get64RegisterOffset(offsetof(type, m))
#define set32Register(m, type, val) set32RegisterOffset(offsetof(type, m), (val))
#define set64Register(m, type, val) set64RegisterOffset(offsetof(type, m), (val))
#define getRegisterBytes(m, buf, type) \
      getBytesRegisterOffset(offsetof(type, m), (uint8_t*)(buf), sizeof(((type*)0)->m))
#define setRegisterBytes(m, buf, type) \
      setBytesRegisterOffset(offsetof(type, m), (const uint8_t*)(buf), sizeof(((type*)0)->m))
#define offsetRegister(m, type) physOffset(offsetof(type, m))

    };
    // This class represents a raw HDL device before it is a container, or when there is no
    // need to create a container (utilities, discovery etc.).
    // It is specialized by the access paths and driver issues (for pci, ethernet etc.)
    class Device {
      HdlUUID m_UUID;
      OCPI::Util::Uuid m_loadedUUID;
    protected:
      std::string m_name, m_platform, m_part, m_esn, m_position, m_loadParams, m_protocol;
      // This is the protocol-specific part of the endpoint.
      std::string m_endpointSpecific;
      Access m_cAccess;
      Access m_dAccess;
      uint64_t m_endpointSize;
      bool m_isAlive;
      Device(std::string &name, const char *protocol = "");
    public:
      uint32_t m_timeCorrection;
      virtual ~Device();
      inline const char *protocol() const { return m_protocol.c_str(); }
      inline const std::string &name() const { return m_name; }
      inline const std::string &platform() const { return m_platform; }
      inline const std::string &esn() const { return m_esn; }
      inline const std::string &part() const { return m_part; }
      inline Access &cAccess() { return m_cAccess; };
      inline Access &dAccess() { return m_dAccess; };
      inline std::string &endpointSpecific() { return m_endpointSpecific; }
      inline uint64_t endpointSize() { return m_endpointSize; }
      inline bool isAlive() { return m_isAlive; }
      bool isLoadedUUID(const std::string &uuid);
      void getUUID();
      virtual void load(const char *name) = 0;
      // This methd has a required base class implementation.
      // If it is overridden, the base class method must be called from there.
      // (probably early, as it retrieves a variety of generic information from either the
      //  device itself or the config info)
      // It is called shortly after construction returns and allows the device
      // to do any finalization
      // Return true on error
      virtual bool configure(ezxml_t config, std::string &err);
      void print();
    };
    
  }
}
#endif
