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


#ifndef RCC_PORT_H_
#define RCC_PORT_H_

#ifndef WORKER_INTERNAL
#define WORKER_INTERNAL
#endif

#include "RCC_Worker.h"
#include "OcpiPort.h"
#include "ContainerPort.h"
#include "RccApplication.h"
#include "RccContainer.h"

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

    class Port :
      public OCPI::Container::PortBase<OCPI::RCC::Worker, OCPI::RCC::Port, OCPI::RCC::ExternalPort> {
      Port *                                m_localOther; // a connected local (same container) port.
      //      const OCPI::Util::PValue *    m_params;     // Our initial properties
      //      ConnectionMode                m_mode;
      RCCPort                              &m_rccPort;    // The RCC port of this port
      OCPI::API::ExternalBuffer            *m_buffer;     // A buffer in use by this port
      bool                                  m_wantsBuffer; // wants a buffer but does not have one
      //      OCPI::Container::ExternalBuffer m_lastBuffer; // for bridge port interface
      //  invalid state: m_wantsBuffer && m_buffer
      //  The initial state is m_wantsBuffer == true, which implies that there is no way for a worker
      //  to start out NOT requesting any buffers... Someday that should be an option:  i.e. like
      //  optionally connected ports, you have optionally requested ports so that no buffer resources
      //  are used on a port until you specifically request buffers.
    public:
      Port(Worker &w, const OCPI::Util::Port &md, const OCPI::Util::PValue *params, RCCPort &rp);
      virtual ~Port();

      bool isInProcess() const { return true; }
      //void portIsConnected();
      //      void setMode(ConnectionMode mode) { m_mode = mode; }
      void connectURL(const char* url, const OCPI::Util::PValue *myProps,
		      const OCPI::Util::PValue * otherProps);
    private:
      void disconnectInternal();
      void disconnect();
      void error(std::string &e);
      //      inline bool definitionComplete() { return m_dtPort && m_dtPort->isFinalized(); }
    protected:
      // These next methods are required by or override the OCPI::Container::Port implementation
      OCPI::Container::ExternalPort &
      createExternal(const char *extName, bool provider,
		     const OCPI::Util::PValue *extParams,
		     const OCPI::Util::PValue *connParams);
      //      const OCPI::RDT::Descriptors *
      //      startConnect(const OCPI::RDT::Descriptors *other, bool &done);
      //      void localConnect(OCPI::DataTransport::Port &input);
      //      const OCPI::RDT::Descriptors *
      //      finishConnect(const OCPI::RDT::Descriptors */*other*/,
      //		    OCPI::RDT::Descriptors &/*feedback*/, bool &done);
#if 0
      OCPI::DataTransport::Port &dtPort(){ ocpiAssert(m_dtPort); return *m_dtPort;}
      void connectInside(OCPI::Container::Port & other,
			 const OCPI::Util::PValue * myParams,
			 const OCPI::Util::PValue * otherParams);
      // These methods are used in the context of bridge ports and represent the "back side"
      // of these ports - the one NOT facing the worker.
      OCPI::API::ExternalBuffer
        *getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end),
	*getBuffer(uint8_t *&data, size_t &length);
      bool endOfData();
      bool tryFlush();
      void release(OCPI::API::ExternalBuffer &b);
      void put(OCPI::API::ExternalBuffer &b, size_t length, uint8_t opCode, bool /*endOfData*/);
#endif
    public:
      // These methods are called in one place from the worker from C, hence public and inline
      bool requestRcc(size_t max = 0) {
	if (m_buffer)
	  return true;
	m_wantsBuffer = true;
	//	if (!definitionComplete())
	//	  return false;
	// We want a buffer and we don't have one
	try {
	  uint8_t *data;
	  if (isOutput()) {
	    if ((m_buffer = getBuffer(data, m_rccPort.current.maxLength))) {
	      m_rccPort.current.data = (void*)data;
	      m_rccPort.output.length = 
		m_rccPort.useDefaultLength_ ? m_rccPort.defaultLength_ : 
		m_rccPort.current.maxLength;
	      m_rccPort.current.opCode_ =
		m_rccPort.useDefaultOpCode_ ? m_rccPort.defaultOpCode_ :
		m_rccPort.output.u.operation;
	      m_rccPort.current.length_ = m_rccPort.output.length;
	      m_rccPort.current.direct_ = 0;
	    }
	  } else {
	    bool end;
	    if ((m_buffer = getBuffer(data, m_rccPort.current.length_,
				      m_rccPort.current.opCode_, end))) {
	      m_rccPort.current.data = (void*)data;
	      m_rccPort.input.u.operation = m_rccPort.current.opCode_;
	      m_rccPort.input.length = m_rccPort.current.length_;
	    }
	  }
	  if (m_buffer) {
	    if (max && isOutput() && max < m_rccPort.output.length)
	      throw OU::Error("Requested output buffer size is unavailable");
	    m_rccPort.current.portBuffer = m_buffer;
	    m_rccPort.current.containerPort = this;
	    m_wantsBuffer = false;
	    return true;
	  }
	} catch (std::string &e) {
	  error(e);
	}
	return false;
      }

      inline void releaseRcc(RCCBuffer &buffer) {
	ocpiAssert(isProvider() && buffer.portBuffer);
	if (&m_rccPort.current == &buffer) {
	  m_buffer = NULL;
	  m_rccPort.current.data = NULL;
	}
	try {
	  buffer.portBuffer->release();
	} catch (std::string &e) {
	  error(e);
	}
      }

      inline void takeRcc(RCCBuffer *oldBuffer, RCCBuffer &newBuffer) {
	if (isOutput())
	  throw OU::Error("The 'take' container function cannot be used on an output port");
	if (!m_buffer)
	  throw OU::Error("The 'take' container function cannot be called when there is no current buffer");

	newBuffer = m_rccPort.current; // copy the structure
	m_rccPort.current.data = NULL;
	m_buffer = NULL;
	if (oldBuffer) {
	  ocpiAssert(oldBuffer->portBuffer);
	  oldBuffer->portBuffer->release();
	}
	requestRcc();
      }
      // return true if we are ready, and try to make us ready in the process
      inline bool checkReady() {
	return m_buffer ? true : (m_wantsBuffer ? requestRcc() : false);
      }
      inline bool advanceRcc(size_t max) {
	try {
	  if (m_buffer) {
	    if (isOutput())
	      m_buffer->put(m_rccPort.current.length_, m_rccPort.current.opCode_, false,
			    m_rccPort.current.direct_);
	    else
	      m_buffer->release();
	    m_rccPort.current.data = NULL;
	    m_buffer = NULL;
	  }
	  bool ready = requestRcc();
	  if (ready && max && max > m_rccPort.current.maxLength)
	    throw OU::Error("Output buffer request/advance (size %zu) greater than buffer size "
			    " (%zu)", max, m_rccPort.current.maxLength);
	} catch (std::string &e) {
	  error(e);
	}
	return false;
      }

      void sendRcc(RCCBuffer &buffer) {
	ocpiAssert(buffer.portBuffer && buffer.containerPort);
	try {
	  if (isInput())
	    throw OU::Error("The 'send' container function cannot be called on an input port");
	  Port &bufferPort = *buffer.containerPort;
	  if (&buffer == &bufferPort.m_rccPort.current) {
	    bufferPort.m_buffer = NULL;
	    bufferPort.m_rccPort.current.data = NULL;
	    bufferPort.m_wantsBuffer = true;
	  }
	  put(*buffer.portBuffer, buffer.length_, buffer.opCode_, false, buffer.direct_);
	} catch (std::string &e) {
	  error(e);
	}
      }
    };
  }
}
#endif
