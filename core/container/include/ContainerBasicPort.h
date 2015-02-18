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

// This file defines base classes used by various types of actual ports,
// like worker/member ports, external ports, and brige ports.
// The classes here are PortData, BasicPort, LocalPort and BufferPort

#ifndef CONTAINER_BASIC_PORT_H
#define CONTAINER_BASIC_PORT_H

#include "OcpiContainerApi.h"

#include "OcpiUtilSelfMutex.h"
#include "OcpiPValue.h"
#include "OcpiRDTInterface.h"
#include "OcpiUtilPort.h"
#include "OcpiParentChild.h"
#include "ContainerLauncher.h"

namespace OCPI {
  namespace DataTransport {
    class Port;
    class BufferUserFacet;
  }
  namespace Container {

    class Worker;
    class ExternalPort;
    class Container;

    // Port connection dependency data: what is communicated between containers
    struct PortConnectionDesc
    {
      OCPI::RDT::Descriptors        data;        // Connection data
    };

    /**********************************
     * Port data structure - thus will be absorbed into BasicPort at some point...
     *********************************/  
    const unsigned DEFAULT_NBUFFERS = 2;
    class PortData {
      OCPI::Util::PortOrdinal m_ordinal;
      bool m_isProvider; // perhaps overriding bidirectional
      PortConnectionDesc *m_connectionData;
      PortConnectionDesc  connectionData;      // Port Connection Dependency data
      // Defaults when no other source provides this.
      // (protocol or port metadata, or port or connection params)

    public:
      PortData(const OCPI::Util::Port &mPort, PortConnectionDesc *desc = NULL);
      virtual ~PortData(){};
      inline bool isProvider() const { return m_isProvider; }
      inline bool isOutput() const {return !isProvider(); }
      inline bool isInput() const {return isProvider(); }
      inline OCPI::Util::PortOrdinal ordinal() const { return m_ordinal; }
      virtual inline PortConnectionDesc &  getData() {
	return m_connectionData ? *m_connectionData : connectionData;
      }
    };

    class BasicPort;
    class ExternalBuffer : public API::ExternalBuffer {
      friend class BasicPort;
      friend class LocalPort;
    protected:
      BasicPort      &m_port;   // which port to I belong to
      bool            m_full;   // This buffer has a complete message in it
      bool            m_eof;    // The message in this buffer is the last before EOF
      uint8_t         m_opCode; // Opcode for the message
      size_t          m_length; // size in bytes of the message
      uint8_t        *m_data;   // The data content of the message, even if zero length
                                // Can be NULL with EOF, indicating an EOF without a message
      size_t          m_direct; // The destination member when "directed".
      // These are specific to the "shim" mode
      ExternalBuffer
        *m_next,         // prewrapped, initialized once, !==NULL indicates shim mode
        *m_zcBack,       // STL-like back - end of queued ZC buffers
        *m_zcQueue,      // Buffer where the queue lives
        *m_zcNext;       // Dual use: in zc-queued bufs: next in queue, in ring buffer: q Front
      // This is specific to the "transport" mode, with a buffer from the transport system
      OCPI::DataTransport::BufferUserFacet *m_dtBuffer;
      // These two are used in either mode

    protected:
      ExternalBuffer(BasicPort &port, ExternalBuffer *next);
    public:
      size_t &length() { return m_length; }
      uint8_t *data() { return m_data; }
      uint8_t &opCode() { return m_opCode; }
      bool &end() { return m_eof; }
      size_t &direct() { return m_direct; }
      void
	release(),
	put(size_t length, uint8_t opCode = 0, bool endOfData = false, size_t direct = 0),
	put(),
	put(ExternalPort &port),
	put(ExternalPort &port, size_t length, uint8_t opCode = 0, bool eof = false, size_t direct = 0);
    };

    // This class has behavior common to worker, external, bridge, shim ports.
    class BasicPort : public PortData,//, public OCPI::API::ExternalPort,
		      public OCPI::API::ExternalPort,
		      protected OCPI::Util::SelfRefMutex {
      friend class Worker;
      friend class Container;
      friend class BridgePort;
      friend class ExternalBuffer;
      // These two are for external port mode.
      ExternalBuffer m_lastBuffer;         // not used by some derived classes
      OCPI::DataTransport::Port *m_dtPort; // not used by some derived classes
      // End external port mode
      // Shim mode.  Slightly clever allocation in order to allocate once for headers and data
      // that is sized at runtime - better locality, no fragmentation, sequential access
      uint8_t *m_allocation; // the heap allocation.  non-NULL indicates shim mode
      size_t m_bufferSize;
      ExternalBuffer *m_next2read, *m_next2write, *m_lastRead, *m_lastWrite;
      // end shim mode
    protected:
      BasicPort *m_forward; // if set, forward worker-side to this other port
      size_t m_nRead, m_nWritten;
      OCPI::RDT::Desc_t &myDesc; // convenience
      const OCPI::Util::Port &m_metaPort;
      Container &m_container;
      
      BasicPort(Container &container, const OCPI::Util::Port &mPort,
		const OCPI::Util::PValue *params);
      virtual ~BasicPort();
      inline const OCPI::Util::Port &metaPort() const { return m_metaPort; }
      Container &container() const { return m_container; }
      // Apply port parameters that are NOT connection parameters that apply to both sides.
      // I.e. apply only one-sided parameters like buffercount.
      void applyPortParams(const OCPI::Util::PValue *params);
      // Apply information in the connection to the descriptor for this port
      void setBufferSize(size_t bufferSize);
      void applyConnection(const Launcher::Connection &c);
      void becomeShim();
      void connectLocal(Launcher::Connection &c);
      void connectInProcess(BasicPort &p);
      void forward2shim(BasicPort &shim);
      virtual bool isInProcess() const = 0;
      virtual void portIsConnected() {}
    public:
      const std::string &name() const { return m_metaPort.m_name; }
      // Start/Finish this side of the connection, and return the right descriptor to return.
      // set "done" true if this side is done and can "operate" (e.g. send messages).
      virtual const OCPI::RDT::Descriptors *
      startConnect(const OCPI::RDT::Descriptors *other, bool &done);
      virtual const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors *other, OCPI::RDT::Descriptors &feedback,
		    bool &done);
      bool startRemote(Launcher::Connection &c);
      bool finishRemote(Launcher::Connection &c);
      // Figure out the transport, interconnect instance id, and roles for a connection
      static void determineTransport(const Transports &in, const Transports &out,
				     const OCPI::Util::PValue *paramsIn,
				     const OCPI::Util::PValue *paramsOut,
				     const OCPI::Util::PValue *connParams,
				     Transport &transport);
      static void packPortDesc(const OCPI::RDT::Descriptors&  port, std::string &out ) throw ();
      static bool unpackPortDesc(const std::string& desc, OCPI::RDT::Descriptors &desc_storage)
        throw ();
      static const char *chooseRoles(OCPI::RDT::PortRole &uRole, unsigned uOptions,
				     OCPI::RDT::PortRole &pRole, unsigned pOptions);
      OCPI::API::ExternalBuffer
        *getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end),
	*getBuffer(uint8_t *&data, size_t &length);
      // Internal methods.
      bool peekOpCode(uint8_t &op);
      ExternalBuffer *getFullBuffer(), *getEmptyBuffer();
         
      bool endOfData();
      bool tryFlush();
      // release most recently gotten input buffer
      void release();
      // put/send the most recently gotten output buffer
      void put(size_t length, uint8_t opCode, bool end, size_t direct = 0);
      // put/send a particular buffer, PERHAPS FROM ANOTHER PORT
      void put(OCPI::API::ExternalBuffer &b);
    };
  }
}
#endif



