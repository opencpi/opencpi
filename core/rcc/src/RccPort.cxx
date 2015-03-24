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
 *   RCC port class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */
#include "DtMsgDriver.h"
#include "RccWorker.h"
#include "RccPort.h"

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OD = OCPI::DataTransport;
namespace OR = OCPI::RDT;
namespace DTM = DataTransfer::Msg;

namespace OCPI {
  namespace RCC {

    Port::
    Port(Worker& w, const OU::Port & pmd, const OU::PValue *params, RCCPort &rp)
      :  OC::PortBase<Worker, Port, OCPI::RCC::ExternalPort>(w, *this, pmd, params),
	 /*m_dtPort(NULL), */m_localOther(NULL), //m_params(params),
	 m_rccPort(rp), m_buffer(NULL), m_wantsBuffer(true)
    {
    }

    Port::
    ~Port()
    {
      //      disconnect();
    }
    void Port::
    error(std::string &e) {
      parent().portError(e);
    }

    void Port::
    connectURL(const char */*url*/, const OU::PValue */*myParams*/, const OU::PValue */*otherProps*/)
    {
    }
#if 0
{
      setMode(OCPI::Container::Port::CON_TYPE_MESSAGE);
      
      // See if we have a message driver that is capable of handling this message type
      DTM::XferFactory *factory = 
	DataTransfer::Msg::XferFactoryManager::getFactoryManager().findFactory( url, myProps, otherProps );
      if (!factory)
	throw OU::Error("Message URL not supported: '%s'", url);
      DTM::XferServices * msgService  = factory->getXferServices(m_metaPort, url, myProps, otherProps);
      ocpiAssert ( msgService );
      m_dtPort = NULL; // msgService->getMsgChannel(url,myProps,otherProps);
      parent().portIsConnected(ordinal());
    }
    // We are being told by our local peer that they are being disconnected.
    void Port::
    disconnectInternal( ) {
      TRACE(" OCPI::RCC::RDMAPort::disconnectInternal()");
      ocpiAssert(m_localOther);
      if (!isProvider() && m_dtPort)
	// If we are output, we are going first.  We take down our dtport.
	m_dtPort->reset();
      m_dtPort = NULL;
      parent().m_context->connectedPorts &= ~(1<<ordinal());
      m_localOther = NULL;
    }

    void Port::
    disconnect( ) {
      TRACE(" OCPI::RCC::RDMAPort::disconnect()");
      OU::SelfAutoMutex guard(this); 
      
      // Always output before input
      if (!isProvider()) {
	// We are output
	if (m_dtPort)
	  m_dtPort->reset();
	if (m_localOther)
	  m_localOther->disconnectInternal();
      } else {
	if (m_localOther)
	// Input with a peer - they go first
	  m_localOther->disconnectInternal();
	if (m_dtPort)
	  m_dtPort->reset();
      }
      m_localOther = NULL;
      m_dtPort = NULL;
      parent().m_context->connectedPorts &= ~(1<<ordinal());
    }

    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
    public:
      ExternalPort(Port &port, const char *name, bool isProvider,
		   const OA::PValue *extParams, const OA::PValue *connParams) :
        OC::ExternalPortBase<Port,ExternalPort>(port, *this, name, extParams, connParams, isProvider) {
      }      
      virtual ~ExternalPort() {}
    };
    OC::ExternalPort &Port::
    createExternal(const char *extName, bool isProvider,
		   const OU::PValue *extParams, const OU::PValue *connParams) {
      return *new ExternalPort(*this, extName, isProvider, extParams, connParams);
    }
#endif

#if 0
    // We use the default behavior of basic ports, but do some
    const OR::Descriptors *Port::
    startConnect(const OR::Descriptors *other, bool &done) {
      returnnconst OR::Descriptors *result = OC::Port::startConnect(other, done);
      if (done)
	parent().portIsConnected(ordinal());
      return result;
    }
    const OR::Descriptors *Port::
    finishConnect(const OR::Descriptors *other, OR::Descriptors &buf, bool &done) {
      return OC::Port::finishConnect(other, buf, done);
    }
#endif
#if 0
    void Port::
    portIsConnected() {
      OC::Port::portIsConnected();
      if (!(m_rccPort.connectedCrewSize = nOthers()))
	m_rccPort.connectedCrewSize = 1;
    }

    void Port::
    startConnect(const OR::Descriptors *other, const OU::PValue *params) {
      ocpiAssert(m_mode == CON_TYPE_NONE);
      setMode(CON_TYPE_RDMA);
      ocpiAssert(!m_dtPort);
      if (isProvider())
	m_dtPort = parent().getTransport().createInputPort(getData().data);
      else if (other)
	m_dtPort = parent().getTransport().createOutputPort(getData().data, *other);
      if (m_dtPort) {
	std::string n = parent().implTag() + "_";
	n += parent().instTag() + "_";
	n +=  m_metaPort.m_name;
	m_dtPort->setInstanceName( n.c_str() ); // FIXME: put this in the dt port constructor
	parent().portIsConnected(ordinal());
      }
    }
    // This happens AFTER startConnect 
    void Port::
    localConnect(OD::Port &input) {
      m_dtPort = parent().getTransport().createOutputPort(getData().data, input);
      std::string n = parent().implTag() + "_";
      n += parent().instTag() + "_";
      n +=  m_metaPort.m_name;
      m_dtPort->setInstanceName( n.c_str() ); // FIXME: put this in the dt port constructor
      parent().portIsConnected(ordinal());
    }
    const OR::Descriptors *Port::
    finishConnect(const OR::Descriptors *other, OR::Descriptors &feedback, bool &done) {
      const OR::Descriptors *d = m_dtPort->finalize(other, getData().data, &feedback, done);
      parent().portIsConnected(ordinal());
      m_lastBuffer.m_dtPort = m_dtPort;
      return d;
    }

    // The input/other is already started via startConnect
    void Port::
    connectInside(OC::Port &input, const OU::PValue *myParams, const OU::PValue *otherParams)
    {
      Port &myInput = *static_cast<Port *>(&input);
      ocpiAssert(!m_dtPort);
      // start up the output side with no input information - just for params
      setConnectParams(myParams);
      // We forcibly ignore mandatory transfer roles here:
      input.getData().data.options &= ~OR::MandatedRole;
      getData().data.options |= OR::MandatedRole;
      getData().data.role = OR::ActiveMessage;
      // Perform the final negotiation between the input side with all its
      determineRoles(input.getData().data);
      input.startConnect(NULL, otherParams);
      startConnect(NULL, myParams);
      // Setup the output port, providing the collocated input port info, but NOT finalizing
      localConnect(input.dtPort());
      OR::Descriptors feedback;
      const OR::Descriptors *outDesc;
      bool done;
      if ((outDesc = finishConnect(&input.getData().data, feedback, done)))
	myInput.finishConnect(outDesc, feedback, done);
      m_localOther = &myInput;
      myInput.m_localOther = this;
    }
    // These directly access the "back side" of a worker port.
    // Get a buffer that the worker member has produced
    OA::ExternalBuffer *Port::
    getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end) {
      assert(!isProvider() && !m_lastBuffer.m_dtBuffer);
      end = false;
      return (m_lastBuffer.m_dtBuffer = m_dtPort->getNextFullOutputBuffer(data, length, opCode)) ?
	&m_lastBuffer : NULL;
      return NULL;
    }
    // Get a buffer that the worker member will consume
    OA::ExternalBuffer *Port::
    getBuffer(uint8_t *&data, size_t &length) {
      assert(isProvider() && !m_lastBuffer.m_dtBuffer);
      return (m_lastBuffer.m_dtBuffer = m_dtPort->getNextEmptyInputBuffer(data, length)) ?
	&m_lastBuffer : NULL;
    }
    void Port::
    release(OCPI::API::ExternalBuffer &b) {
      assert(!isProvider() && m_lastBuffer.m_dtBuffer && &m_lastBuffer == &b);
      m_dtPort->releaseOutputBuffer(*m_lastBuffer.m_dtBuffer);
      m_lastBuffer.m_dtBuffer = NULL;
    }
    void Port::
    put(OCPI::API::ExternalBuffer &b, size_t length, uint8_t opCode, bool /*endOfData*/) {
      assert(isProvider() && m_lastBuffer.m_dtBuffer && &b == &m_lastBuffer);
      m_dtPort->sendInputBuffer(*m_lastBuffer.m_dtBuffer, length, opCode);
      m_lastBuffer.m_dtBuffer = NULL;
    }
    // Indicate EOF on worker member's input port
    void Port::
    endOfData() {
    }
    bool Port::
    tryFlush() {
      return false;
    }
#endif
  }
}
