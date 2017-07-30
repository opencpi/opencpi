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

/*
 * Abstract:
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
namespace DTM = DataTransfer::Msg;

namespace OCPI {
  namespace RCC {

    Port::
    Port(Worker& w, const OU::Port & pmd, const OU::PValue *params, RCCPort &rp)
      : OC::PortBase< Worker, Port, ExternalPort>
	(w, *this, pmd, pmd.m_provider,
	 (1 << OCPI::RDT::ActiveFlowControl) | (1 << OCPI::RDT::ActiveMessage), params),
	m_dtPort(NULL), m_localOther(NULL), //m_params(params),
	m_mode(OC::Port::CON_TYPE_NONE), m_rccPort(rp), m_buffer(NULL), m_wantsBuffer(true) {
      // FIXME: deep copy params?
      // Initialize rccPort with aspects based on metadata
      if (pmd.nOperations() <= 1) {
	m_rccPort.useDefaultOpCode_ = true;
	if (pmd.nOperations() == 1) {
	  m_rccPort.useDefaultLength_ = true;
	  OU::Operation &o = pmd.operations()[0];
	  if (o.nArgs()) {
	    m_rccPort.defaultLength_ = pmd.m_minBufferSize;
	    if (o.nArgs() > 1) {
	      OU::Member &m = o.args()[o.nArgs() - 1];
	      if (m.isSequence())
		m_rccPort.sequence = &m;
	    }
	  }
	}
      }
    }

    Port::
    ~Port()
    {
      disconnect();
    }
    void Port::
    error(std::string &e) {
      parent().portError(e);
    }

    void Port::
    connectURL(const char* url, const OU::PValue *myProps,
	       const OU::PValue *otherProps)
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
      ExternalPort(Port &port, const char *a_name, bool a_isProvider,
		   const OA::PValue *extParams, const OA::PValue *connParams) :
        OC::ExternalPortBase<Port,ExternalPort>(port, *this, a_name, extParams, connParams,
						a_isProvider) {
      }      
      virtual ~ExternalPort() {}
    };
    OC::ExternalPort &Port::
    createExternal(const char *extName, bool a_isProvider,
		   const OU::PValue *extParams, const OU::PValue *connParams) {
      return *new ExternalPort(*this, extName, a_isProvider, extParams, connParams);
    }

    // For an output port, other == NULL signifies the collocated special case
    void Port::
    startConnect(const OCPI::RDT::Descriptors *other, const OU::PValue *params) {
      ocpiAssert(m_mode == CON_TYPE_NONE);
      setMode(CON_TYPE_RDMA);
      ocpiAssert(!m_dtPort);
      if (isProvider())
	m_dtPort = parent().getTransport().createInputPort(getData().data, params );
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
    const OCPI::RDT::Descriptors *Port::
    finishConnect(const OCPI::RDT::Descriptors &other,
		  OCPI::RDT::Descriptors &feedback) {
      const OCPI::RDT::Descriptors *d = NULL;
      if (isProvider())
	m_dtPort->finalize(other, getData().data);
      else
	d = m_dtPort->finalize(other, getData().data, &feedback);
      parent().portIsConnected(ordinal());
      return d;
    }

    // The input/other is already started via startConnect
    void Port::
    connectInside(OC::Port & input, const OU::PValue *myParams, const OU::PValue *otherParams)
    {
      Port &myInput = *static_cast<Port *>(&input);
      ocpiAssert(!m_dtPort);
      // start up the output side with no input information - just for params
      setConnectParams(myParams);
      // We forcibly ignore mandatory transfer roles here:
      input.getData().data.options &= ~OCPI::RDT::MandatedRole;
      getData().data.options |= OCPI::RDT::MandatedRole;
      getData().data.role = OCPI::RDT::ActiveMessage;
      // Perform the final negotiation between the input side with all its
      determineRoles(input.getData().data);
      input.startConnect(NULL, otherParams);
      startConnect(NULL, myParams);
      // Setup the output port, providing the collocated input port info, but NOT finalizing
      localConnect(input.dtPort());
      OCPI::RDT::Descriptors feedback;
      const OCPI::RDT::Descriptors *outDesc;
      if ((outDesc = finishConnect(input.getData().data, feedback)))
	myInput.finishConnect(*outDesc, feedback);
      m_localOther = &myInput;
      myInput.m_localOther = this;
    }
  }
}
