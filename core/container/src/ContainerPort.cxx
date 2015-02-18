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

#include "OcpiOsAssert.h"
#include "OcpiUtilCDR.h"
#include "Container.h"
#include "ContainerWorker.h"
#include "ContainerPort.h"

namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OD = OCPI::DataTransport;
    namespace OR = OCPI::RDT;

    // This base class constructor for generic initialization
    // FIXME: parse buffer count here at least? (check that others don't do it).
    Port::Port(Container &container, const OU::Port &mPort, const OU::PValue *params) :
      LocalPort(container, mPort, params),
      m_canBeExternal(true)
    {
    }
    Port::~Port() {
      container().unregisterBridgedPort(*this);
    }

    //    Container &Port::container() const { return m_container; }

    bool Port::hasName(const char *name) {
      return (name == m_metaPort.m_name );
    }

    // The default behavior is that there is nothing special to do between
    // ports of like containers.
    bool Port::connectLike(Port &other, const OU::PValue *myProps,
			   const OU::PValue *otherProps) {
      (void)other;(void)myProps;(void)otherProps;
      return false;
    }

    void Port::connectURL(const char*, const OU::PValue *, const OU::PValue *) {
      ocpiDebug("connectURL not allowed on this container !!");
      ocpiAssert( 0 );
    }

#if 0
    // This funkiness is to ensure that connection-related (as opposed to port-related) parameters
    // end up on both lists.  FIXME make this automatic storage etc.
    // We assume these connection parameters are initially only in one place
    static void
    mergeConnectParams(const OU::PValue *otherParams,           // potential source of connect params
		       const char *preferred,                   // callers preferred transport
		       const OU::PValue *&toParams,             // params possibly augmented
		       OU::PValue *&newParams) {                // new list if needed
      newParams = NULL;
      const char *transport = NULL;
      if (!OU::findString(otherParams, "protocol", transport) &&
	  !OU::findString(otherParams, "transport", transport) &&
	  !OU::findString(otherParams, "endpoint", transport))
	transport = preferred;
      if (transport) {
	unsigned n = 0;
	newParams = new OU::PValue[toParams->length() + 2];
	for (const OU::PValue *p = toParams; p && p->name; p++, n++)
	  newParams[n] = *p;
	newParams[n].name = "transport";
	newParams[n].vString = transport;
	newParams[n].type = OA::OCPI_String;
	newParams[++n].name = NULL;
	toParams = newParams;
      }
    }
#endif

#if 0
    void Port::
    connect(OA::Port &apiOther, const OU::PValue *myParams, const OU::PValue *otherParams) {
      connect(apiOther, 0, 0, myParams, otherParams);
    }
#endif

#if 0
    // The general case of connecting ports that are managed in the same process.
    void Port::connect(OA::Port &apiOther, size_t otherN, size_t bufferSize,
		       const OU::PValue *myParams, const OU::PValue *otherParams) {
      OU::SelfAutoMutex guard (this);
      Port &other = *static_cast<Port*>(&apiOther);
      if (isProvider())
        if (other.isProvider())
          throw OU::Error("Cannot connect two provider ports");
        else
          other.connect(*this, otherParams, myParams);
      else if (!other.isProvider())
        throw OU::Error("Cannot connect to user ports");
      else {
        Container &otherContainer = other.container();
	// FIXME: Take any connection-related parameters and make sure both parameter lists have them.
        // Containers know how to do internal connections
        if (&m_container == &otherContainer) {
	  other.setConnectParams(otherParams);
          connectInside(other, myParams, otherParams);
          // Container MAY know how to do intercontainer connections between like containers.
	  //        } else if (&container().driver() == &otherContainer.driver() &&
	  //		   connectLike( other, myParams, otherParams))
	  //	  return;
	} else {
	  const char *preferred = getPreferredProtocol();
	  // Check if the output side has a preferred protocol, and if so, set it
	  if (!preferred)
	    preferred = other.getPreferredProtocol();
	  // Ensure that any connect parameters are on both ports' lists
	  OU::PValue *newMyParams, *newOtherParams;
	  mergeConnectParams(otherParams, preferred, myParams, newMyParams);
	  mergeConnectParams(myParams, preferred, otherParams, newOtherParams);
#if 1
	  if (!other.m_canBeExternal)
	    throw OU::Error("Port \"%s\" cannot be connected external to container",
			    other.m_metaPort.m_name.c_str());
	  if (!m_canBeExternal)
	    throw OU::Error("Port \"%s\" cannot be connected external to container",
			    m_metaPort.m_name.c_str());
	  other.setConnectParams(otherParams);
	  setConnectParams(myParams);
	  determineRoles(other.getData().data);

	  other.startConnect(NULL, otherParams, );
	  startConnect(&other.getData().data, myParams);
	  const OR::Descriptors *outDesc;
	  OR::Descriptors feedback;
	  bool done;  // not really used in this local case
	  // try to finish output side, possibly producing flow control feedback
	  if ((outDesc = finishConnect(&other.getData().data, feedback, done)))
	    // try to finish input side, possibly providing some feedback
	    // see setInitialUserInfo below
	    if ((outDesc = other.finishConnect(outDesc, feedback, done)))
	      // in fact more is needed to finish on output side
	      // - like enabling it to start sending since the receiver is now ready
	      // see setFinalProviderInfo below
	      if ((outDesc = finishConnect(outDesc, feedback, done)))
		// see setFinalUserInfo
		other.finishConnect(outDesc, feedback, done);
#else
	  // This sequence is too much overhead, but should still work
	  std::string pInfo, uInfo;
	  other.getInitialProviderInfo(otherN, bufferSize, otherParams, pInfo);
	  // FIXME: make this a proper automatic to avoid exception leaks
	  if (newOtherParams)
	      delete [] newOtherParams;
	  setInitialProviderInfo(otherN, bufferSize, myParams, pInfo, uInfo);
	  if (newMyParams)
	      delete [] newMyParams;
          if (!uInfo.empty()) {
            other.setInitialUserInfo(otherN, uInfo, pInfo);
            if (!pInfo.empty()) {
	      setFinalProviderInfo(otherN, pInfo, uInfo);
              if (!uInfo.empty())
                other.setFinalUserInfo(otherN, uInfo);
            }
	  }
#endif
        }
      }
    }
#endif

#if 0
    // Here are three methods for setting up the local member port when it is in bridged mode,
    // where there is a "message generator" and bridge ports between it and the members of the
    // connected crew.  On input, the "message generator" takes messages from the connected
    // crew members (via bridge ports), and produces messages for the local member's input port.
    // On output, the message generator takes messages from the local member's port and
    // produces and routes resulting messages to the connected crew members via the bridge
    // ports.  The input process is a sort of "gather" and the output process is a sort of
    // "scatter".
    // The local member ports are set up with a "passive" role, and use a "local" endpoint,
    // which allocates buffers on the heap.  The local member ports are set up in three
    // steps:
    // 1. Initialization based on connection and port parameters from the "application" level.
    //    This is called "starting".
    // 2. Receiving the descriptor from the "other side" to negotiate buffer sizes and
    //    performing local resource allocations (e.g. buffers), and sending the results back
    //    to the other side.
    //    This is called "establishing"
    // 3. Receiving the resource allocations from the other side to finalize everything.
    //    This is called "finishing".
    void Port::
    startLocalConnect(const OU::PValue *params) {
      // Do the one-time setup of the local member port.
      // Customize the member port params for the bridged case:
      // transport should be pio (FIXME: heap), nbuffers should match the bridge ports,
      // transfer role should be passive
      OU::PValue pv[] = { OU::PVString("transport", "local"),
			  OU::PVString("xferRole", "passive"), OU::PVEnd };
      OU::PValueList pvl(params, pv);
      setConnectParams(pvl);
    }

    void Port::
    finishLocalConnect(const OR::Descriptors &other) {
      // Force the "other" descriptor to have the local endpoint, and the right role
      // Then copy some things from the real "other" descriptor
      OR::Descriptors myOther = getData().data; // make other descriptor same as ours
      myOther.role = OR::ActiveOnly;            // force the "other" role opposite of passive
      myOther.options = 0;
      myOther.desc.dataBufferSize = other.desc.dataBufferSize;
      determineRoles(myOther);
      startConnect(&myOther, NULL);
      OR::Descriptors dummy;
      bool done;
      finishConnect(&myOther, dummy, done);
    }

    void Port::determineRoles(OR::Descriptors &other) {
      BasicPort::determineRoles(worker().name().c_str(), other);
    }
#endif

    ExternalPort::
    ExternalPort(Launcher::Connection &c, bool isProvider)
      : LocalPort(Container::baseContainer(),
		  isProvider ? *c.m_in.m_metaPort : *c.m_out.m_metaPort,
		  isProvider ? c.m_in.m_params : c.m_out.m_params),
	OU::Child<Application,ExternalPort,externalPort>
	(*(isProvider ? c.m_in.m_containerApp : c.m_out.m_containerApp), *this, NULL)
    {
      prepareOthers(isProvider ? c.m_out.m_scale : c.m_in.m_scale, 1);
#if 0 
      if (isProvider()) {
	// Create the DT input port which is the basis for this external port
	// Apply our params to the basic port
	applyConnectParams(NULL, extParams);
	// Create the DT port using our merged params
	m_dtPort = port.container().getTransport().createInputPort(getData().data);
	// Start the connection process on the worker port
	if (port.isLocal()) {
	  port.applyConnectParams(NULL, portParams); 
	  port.determineRoles(getData().data);
	  port.localConnect(*m_dtPort);
	} else {
	  port.applyConnectParams(&getData().data, portParams);
	  port.determineRoles(getData().data);
	}
	// Finalize the worker's output port, getting back the flow control descriptor
	OR::Descriptors feedback;
	bool done;
	const OR::Descriptors *outDesc = port.finishConnect(&getData().data, feedback, done);
	if (outDesc)
	  m_dtPort->finalize(outDesc, getData().data, NULL, done);
      } else {
	port.applyConnectParams(NULL, portParams);
	if (port.isLocal())
	  m_dtPort = port.container().getTransport().
	    createOutputPort(getData().data, port.dtPort());
	else
	  m_dtPort = port.container().getTransport().
	    createOutputPort(getData().data, port.getData().data);
	port.determineRoles(getData().data);
	OR::Descriptors localShadowPort, feedback;
	bool done;
	const OR::Descriptors *outDesc = m_dtPort->finalize(&port.getData().data, getData().data,
							&localShadowPort, done);
	ocpiCheck(!port.finishConnect(outDesc, feedback, done));
      }
      m_lastBuffer.m_dtPort = m_dtPort;
      delete [] newExtParams;
      delete [] newPortParams;
#endif
    }
    ExternalPort::
    ~ExternalPort() {
    }
#if 0
    const OR::Descriptors *ExternalPort::
    startConnect(const OR::Descriptors *other, OR::Descriptors &buf, bool &done) {
      return NULL;
    }
    const OR::Descriptors *ExternalPort::
    finishConnect(const OR::Descriptors *other, OR::Descriptors &feedback, bool &done) {
      return NULL;
    }
#endif
#if 0
    OA::ExternalPort &Port::
    connectExternal(const char *extName, const OA::PValue *portParams,
		    const OA::PValue *extParams) {
      if (!m_canBeExternal)
        throw OU::ApiError ("For external port \"", extName, "\", port \"",
			    name().c_str(), "\" of worker \"",
			    worker().implTag().c_str(), "/", worker().instTag().c_str(), "/",
			    worker().name().c_str(),
			    "\" is locally connected in the HDL bitstream. ", NULL);
      // This call should get the worker's port ready to be connected.
      return createExternal(extName, !isProvider(), extParams, portParams);
    }
#endif

    // Bridge port constructor also does the equivalent of "startConnect" for itself.
    BridgePort::
    BridgePort(LocalPort &port, const OU::PValue *params)
      : BasicPort(port.container(), port.metaPort(), params)
    {
#if 0
      applyConnection(c);
      LocalPort *other = (isProvider() ? c.m_out : c.m_in).m_port;
      if (!other)
	more = startRemote(c);
      else if (other->m_bridgePorts.size()) {
	// bridge ports on both sides locally.  whoever is last does it with forwarding
	BridgePort *otherBridge =
	  other->m_bridgePorts[(isProvider() ? c.m_in : c.m_out).m_index];
	if (otherBridge)
	  connectInProcess(*otherBridge);
	else
	  other->initialConnect(c);
      } else if (other->isInProcess())
	connectInProcess(*other);
      else
	connectLocal(c);
#endif
    }

    BridgePort::
    ~BridgePort() {
      if (m_dtPort)
	m_dtPort->reset();
    }

#if 0
    const OR::Descriptors *BridgePort::
    startConnect(const OR::Descriptors */*other*/, OR::Descriptors &/*buf*/, bool &/*done*/) {
      ocpiAssert("Unexpected start connect on bridge port" == 0);
      return NULL;
    }
    

    const OR::Descriptors *BridgePort::
    finishConnect(const OR::Descriptors *other, OR::Descriptors &feedback, bool &done) {
      const OR::Descriptors *d = m_dtPort->finalize(other, getData().data, &feedback, done);
      return d;
    }
      // These are the same as an external port
    OA::ExternalBuffer *BridgePort::
    getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end) {
      assert(isProvider() && !m_lastBuffer.m_dtBuffer);
      end = false;
      return (m_lastBuffer.m_dtBuffer = m_dtPort->getNextFullInputBuffer(data, length, opCode)) ?
	&m_lastBuffer : NULL;
    }
    OA::ExternalBuffer *BridgePort::
    getBuffer(uint8_t *&data, size_t &length) {
      assert(!isProvider() && !m_lastBuffer.m_dtBuffer);
      return (m_lastBuffer.m_dtBuffer = m_dtPort->getNextEmptyOutputBuffer(data, length)) ?
	&m_lastBuffer : NULL;
    }
    void BridgePort::
    endOfData() {
      assert("No endOfData support"==0);
    }
    bool BridgePort::
    tryFlush() {
      return false;
    }
    void BridgePort::
    release(OCPI::API::ExternalBuffer &b) {
      b.release();
    }
    void BridgePort::
    put(OCPI::API::ExternalBuffer &b, size_t length, uint8_t opCode, bool /*endOfData*/) {
      b.put(length, opCode, false);
    }
#endif
  }

  namespace API {
    ExternalBuffer::~ExternalBuffer(){}
    ExternalPort::~ExternalPort(){}
    Port::~Port() {}
  }
}
