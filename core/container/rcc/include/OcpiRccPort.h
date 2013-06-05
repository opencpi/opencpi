/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
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

/*
 * Abstact:
 *   This file contains the declarations for the ports of RCC workers
 *
 * Revision History: 
 *
 *    06/23/09  John Miller
 *    Added code to handle RCC_ERROR and RCC_FATAL return codes.
 * 
 *    06/01/05  John Miller
 *    Initial revision
 *
 */


#ifndef OCPI_RCC_PORT_H_
#define OCPI_RCC_PORT_H_

#ifndef WORKER_INTERNAL
#define WORKER_INTERNAL
#endif

#include <OcpiPort.h>
#include <OcpiBuffer.h>
#include <OcpiContainerPort.h>
#include <RCC_Worker.h>


namespace DataTransfer {
  namespace Msg {
    class MsgChannel;
  }
}

namespace OCPI {

  namespace DataTransport {
    class Buffer;
    class Port;
  }


  namespace RCC {

    class Application;
    class Worker;
    class ExternalPort;
    class ExternalBuffer;

    class Port : public OCPI::Container::PortBase<Worker, Port, ExternalPort> {
      OCPI::DataTransport::Port *           m_dtPort;
      Port *                                m_localOther; // a connected local (same container) port.
      const OCPI::Util::PValue *            m_params;     // Our initial properties
      ConnectionMode                        m_mode;
      RCCPort                              &m_rccPort;    // The RCC port of this port
      OCPI::DataTransport::BufferUserFacet *m_buffer;     // A buffer in use by this port
      bool                                  m_wantsBuffer; // wants a buffer but does not have one
      //  invalid state: m_wantsBuffer && m_buffer
      //  The initial state is m_wantsBuffer == true, which implies that there is no way for a worker
      //  to start out NOT requesting any buffers... Someday that should be an option:  i.e. like
      //  optionally connected ports, you have optionally requested ports so that no buffer resources
      //  are used on a port until you specifically request buffers.
    public:
      Port( Worker& w, const OCPI::Metadata::Port & pmd, const OCPI::Util::PValue *params, RCCPort &rp);
      virtual ~Port();

      bool isLocal() const { return true; }
      void setMode(ConnectionMode mode) { m_mode = mode; }
      void connectURL(const char* url, const OCPI::Util::PValue *myProps,
		      const OCPI::Util::PValue * otherProps);
    private:
      void disconnectInternal();
      void disconnect();
      void error(std::string &e);
      inline bool definitionComplete() { return m_dtPort && m_dtPort->isFinalized(); }
    protected:
      // These next methods are required by or override the OCPI::Container::Port implementation
      OCPI::Container::ExternalPort &
      createExternal(const char *extName, bool provider,
		     const OCPI::Util::PValue *extParams,
		     const OCPI::Util::PValue *connParams);
      
      void startConnect(const OCPI::RDT::Descriptors *, const OCPI::Util::PValue * my_props);
      void localConnect(OCPI::DataTransport::Port &input);
      const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors &/*other*/, OCPI::RDT::Descriptors &/*feedback*/);
      OCPI::DataTransport::Port &dtPort(){ ocpiAssert(m_dtPort); return *m_dtPort;}
      void connectInside(OCPI::Container::Port & other, const OCPI::Util::PValue * my_props);
    public:
      // These next methods are called (in one place) from the worker from C, hence public and inline
      inline void release( OCPI::DataTransport::BufferUserFacet* buffer) {
	ocpiAssert(isProvider());
	if (m_buffer == buffer) {
	  m_buffer = NULL;
	  m_rccPort.current.data = NULL;
	}
	ocpiAssert(m_dtPort);
	try {
	  m_dtPort->releaseInputBuffer(buffer);
	} catch (std::string &e) {
	  error(e);
	}
      }
      inline void take( RCCBuffer *oldBuffer, RCCBuffer *newBuffer) {
	*newBuffer = m_rccPort.current;
	m_rccPort.current.data = NULL;
	m_buffer = NULL;
	if ( oldBuffer ) {
	  OCPI::DataTransport::BufferUserFacet* old = oldBuffer->containerBuffer;
	  Port *bp = static_cast<Port *>(old->m_ud);
	  bp->release(old);
	}
	request();
      }
      // return true if we are ready, and try to make us ready in the process
      inline bool checkReady() {
	return m_buffer ? true : (m_wantsBuffer ? request() : false);
      }
      bool request() {
	if (m_buffer)
	  return true;
	m_wantsBuffer = true;
	if (!definitionComplete())
	  return false;
	// We want a buffer and we don't have one
	try {
	  if (isOutput()) {
	    if ((m_buffer = m_dtPort->getNextEmptyOutputBuffer(m_rccPort.current.data,
							       m_rccPort.current.maxLength)))
	      m_rccPort.output.length = m_rccPort.current.maxLength;
	  } else {
	    uint8_t opcode;
	    if ((m_buffer = m_dtPort->getNextFullInputBuffer(m_rccPort.current.data,
							     m_rccPort.input.length,
							     opcode)))
	      m_rccPort.input.u.operation = opcode;
	  }
	} catch (std::string &e) {
	  error(e);
	}
	if (m_buffer) {
	  m_rccPort.current.containerBuffer = m_buffer;
	  m_buffer->m_ud = this;
	  m_wantsBuffer = false;
	  return true;
	}
	return false;
      }

      inline bool advance() {
	try {
	  if (m_buffer) {
	    isOutput() ?
	      m_dtPort->sendOutputBuffer(m_buffer, m_rccPort.output.length, m_rccPort.output.u.operation) :
	      m_dtPort->releaseInputBuffer(m_buffer);
	    m_rccPort.current.data = NULL;
	    m_buffer = NULL;
	  }
	  return request();
	} catch (std::string &e) {
	  error(e);
	}
	return false;
      }

      void send(OCPI::DataTransport::BufferUserFacet* buffer, size_t length,
		uint8_t opcode) {
	Port *bufferPort = static_cast<Port*>(buffer->m_ud);
	try {
	  if (bufferPort == this) {
	    m_dtPort->sendOutputBuffer(buffer, length, opcode);
	    if (buffer == m_buffer) {
	      m_buffer = NULL;
	      m_rccPort.current.data = NULL;
	      // If we send the current buffer, it is an implicit advance
	      m_wantsBuffer = true;
	    }
	  } else {
	    // Potential zero copy
	    m_dtPort->sendZcopyInputBuffer(static_cast<OCPI::DataTransport::Buffer*>(buffer),
					   length, opcode);
	    if (bufferPort->m_buffer == buffer) {
	      bufferPort->m_buffer = NULL;
	      bufferPort->m_rccPort.current.data = NULL;
	      // If we send the current buffer, it is an implicit advance
	      bufferPort->m_wantsBuffer = true;
	    }
	  }
	} catch (std::string &e) {
	  error(e);
	}
      }
    };
  }
}
#endif
