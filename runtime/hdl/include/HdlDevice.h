// This file defines the control plane access classes, which is the basis for different
// implementations of the cHDL control plane.

#ifndef HDL_DEVICE_H
#define HDL_DEVICE_H
#include <stdint.h>

#include <climits>
#include "OcpiUuid.h"
#include "DtTransferInternal.h"
#include "OcpiRDTInterface.h"
#include "HdlWciControl.h"
#include "HdlAccess.h"

namespace OCPI {
  namespace HDL {
    // This class represents a raw HDL device before it is a container, or when there is no
    // need to create a container (utilities, discovery etc.).
    // It is specialized by the access paths and driver issues (for pci, ethernet etc.)
    typedef uint32_t RomWord;
    static const unsigned
      ROM_NBYTES = 8*1024,
      ROM_WIDTH  = sizeof(RomWord)*CHAR_BIT,
      ROM_WIDTH_BYTES = sizeof(RomWord),
      ROM_NWORDS = (ROM_NBYTES + ROM_WIDTH_BYTES-1) / ROM_WIDTH_BYTES,
      ROM_HEADER_WORDS = 4,
      ROM_HEADER_BYTES = ROM_HEADER_WORDS * sizeof(RomWord);
    class Device {
      HdlUUID m_UUID;
      OCPI::Util::Uuid m_loadedUUID;
      const char *m_metadata;
      ezxml_t m_implXml;
      bool m_old;
    protected:
      std::string m_name, m_platform, m_part, m_esn, m_position, m_loadParams, m_protocol;
      // This is the protocol-specific part of the endpoint.
      std::string m_endpointSpecific;
      Access m_cAccess;
      Access m_dAccess;
      uint64_t m_endpointSize;
      bool m_isAlive;
      WciControl *m_pfWorker;
      bool m_isFailed; // protection during shutdown
    public:
      uint32_t m_timeCorrection;
      DataTransfer::EndPoint *m_endPoint;
    protected:
      Device(const std::string &name, const char *protocol = "");
    public:
      virtual ~Device();
      bool init(std::string &error);
      inline Access &properties() const { return m_pfWorker->m_properties; }
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
      inline bool isFailed() { return m_isFailed; }
      bool isLoadedUUID(const std::string &uuid);
      void getUUID();
      RomWord getRomWord(uint16_t n);
      virtual bool getMetadata(std::vector<char> &xml, std::string &err);
      virtual void load(const char *name) = 0;
      virtual void connect() {}
      virtual void unload() = 0;
      virtual bool needThread() const { return false; };
      virtual bool run() { return true; }
      void getWorkerAccess(size_t index,
			   Access &worker,
			   Access &properties);
      void releaseWorkerAccess(size_t index,
			       Access & worker,
			       Access & properties);
      // Methods when the container is making connections and the device needs to know about it,
      // mostly for simulation.
      virtual DataTransfer::EndPoint &getEndPoint();
      virtual void connect(DataTransfer::EndPoint &ep, OCPI::RDT::Descriptors &mine,
			   const OCPI::RDT::Descriptors &other);
      virtual uint32_t dmaOptions(ezxml_t icImplXml, ezxml_t icInstXml, bool isProvider) = 0;
      // This method has a required base class implementation.
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
