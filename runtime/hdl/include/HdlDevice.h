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

#ifndef HDL_DEVICE_H
#define HDL_DEVICE_H

#include <inttypes.h>
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
      ROM_WIDTH  = (unsigned)sizeof(RomWord)*CHAR_BIT,
      ROM_WIDTH_BYTES = sizeof(RomWord),
      ROM_NWORDS = (ROM_NBYTES + ROM_WIDTH_BYTES-1) / ROM_WIDTH_BYTES,
      ROM_HEADER_WORDS = 4,
      ROM_HEADER_BYTES = ROM_HEADER_WORDS * (unsigned)sizeof(RomWord);
    class Device {
      friend class Driver;
      HdlUUID m_UUID;
      OCPI::Util::Uuid m_loadedUUID;
      const char *m_metadata;
      ezxml_t m_implXml;
      bool m_old;
    protected:
      std::string
	m_name, m_platform, m_part, m_esn, m_position, m_loadParams, m_protocol, m_arch;
      // This is the protocol-specific part of the endpoint.
      std::string m_endpointSpecific;
      Access m_cAccess;
      Access m_dAccess;
      uint64_t m_endpointSize;
      bool m_isAlive;
      WciControl *m_pfWorker, *m_tsWorker;
      bool m_isFailed; // protection during shutdown
      bool m_verbose;
    public:
      uint32_t m_timeCorrection;
      DataTransfer::EndPoint *m_endPoint;
      static void initAdmin(OccpAdminRegisters &admin, const char *a_platform, HdlUUID &hdlUuid,
			    OCPI::Util::UuidString *uuidString);
    protected:
      Device(const std::string &name, const char *protocol = "", 
	     const OCPI::Util::PValue *params = NULL);
    public:
      virtual ~Device();
      virtual bool init(std::string &error);
      inline Access &properties() const { return m_pfWorker->m_properties; }
      inline Access *timeServer() const {
	return m_tsWorker ? &m_tsWorker->m_properties : NULL;
      }
      inline const char *protocol() const { return m_protocol.c_str(); }
      inline const std::string &name() const { return m_name; }
      inline const std::string &platform() const { return m_platform; }
      inline const std::string &arch() const { return m_arch; }
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
      virtual bool load(const char *name, std::string &err) = 0;
      virtual bool unload(std::string &err) = 0;
      virtual void connect() {}
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
