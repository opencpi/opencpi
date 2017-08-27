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

    Port::Port(Container &a_container, const OU::Port &mPort, const OU::PValue *params) :
      LocalPort(a_container, mPort, mPort.m_provider, params),
      m_canBeExternal(true)
    {
    }
    Port::~Port() {
      ocpiDebug("In  Container::Port::~Port()\n");
    }

    bool Port::hasName(const char *a_name) {
      return (a_name == m_metaPort.m_name);
    }

    void Port::portIsConnected() {
      ocpiDebug("Port %s(%u) of worker %s is now connected\n",
		name().c_str(), ordinal(), worker().name().c_str());
      worker().connectPort(ordinal());
    }

    // The default behavior is that there is nothing special to do between
    // ports of like containers.
    bool Port::connectLike(Port &other, const OU::PValue *myProps,
			   const OU::PValue *otherProps) {
      (void)other;(void)myProps;(void)otherProps;
      return false;
    }

    // Older API for compatibility with ctests
    void Port::connect(OCPI::API::Port &other, const OCPI::API::PValue */*myParams*/,
		       const OCPI::API::PValue */*otherParams*/) {
      Launcher::Connection c;
      c.m_in.m_port = isProvider() ? this : &other.containerPort(),
      c.m_out.m_port = isProvider() ? &other.containerPort() : this;
      c.m_bufferSize = OU::Port::determineBufferSize(&c.m_in.m_port->m_metaPort, NULL,
						     &c.m_out.m_port->m_metaPort, NULL, NULL);
      c.m_in.m_port->initialConnect(c);
      if (!c.m_out.m_done)
	c.m_out.m_port->initialConnect(c);
      bool more;
      do {
	more = false;
	if (c.m_out.m_initial.length() || c.m_out.m_final.length()) {
	  if (c.m_in.m_port->finalConnect(c))
	    more = true;
	  c.m_out.m_initial.clear();
	  c.m_out.m_final.clear();
	}
	if (!c.m_in.m_done)
	  more = true;
	if (c.m_in.m_initial.length() || c.m_in.m_final.length()) {
	  if (c.m_out.m_port->finalConnect(c))
	    more = true;
	  c.m_in.m_initial.clear();
	  c.m_in.m_final.clear();
	}
	if (!c.m_out.m_done)
	more = true;
      } while (more);	
      assert(c.m_in.m_done && c.m_out.m_done);
    }    
    void Port::connectURL(const char*, const OU::PValue *, const OU::PValue *) {
      ocpiDebug("connectURL not allowed on this container !!");
      ocpiAssert( 0 );
    }

#if 0
    ExternalPort::
    ExternalPort(const OCPI::Util::Port &mPort, Application &app, size_t nOthers,
		 const OU::PValue *params)
      : LocalPort(Container::baseContainer(), mPort, params),
	OU::Child<Application,ExternalPort,externalPort>(app, *this, NULL) {
      prepareOthers(nOthers, 1);
    }
#else
    ExternalPort::
    ExternalPort(Launcher::Connection &c, bool a_isProvider)
      : LocalPort(Container::baseContainer(),
		  a_isProvider ? *c.m_in.m_metaPort : *c.m_out.m_metaPort, a_isProvider,
		  a_isProvider ? c.m_in.m_params : c.m_out.m_params),
	OU::Child<Application,ExternalPort,externalPort>
	(*(a_isProvider ? c.m_in.m_containerApp : c.m_out.m_containerApp), *this, NULL)
    {
      prepareOthers(a_isProvider ? c.m_out.m_scale : c.m_in.m_scale, 1);
    }
#endif

    ExternalPort::
    ~ExternalPort() {
    }

    // Bridge port constructor also does the equivalent of "startConnect" for itself.
    BridgePort::
    BridgePort(LocalPort &port, const OU::PValue *params)
      : BasicPort(port.container(), port.metaPort(), port.metaPort().m_provider, params)
    {
    }

    BridgePort::
    ~BridgePort() {
    }
  }

  namespace API {
    ExternalBuffer::~ExternalBuffer(){}
    ExternalPort::~ExternalPort(){}
    Port::~Port() {}
  }
}
