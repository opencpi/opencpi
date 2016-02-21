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
    }

    //    Container &Port::container() const { return m_container; }

    bool Port::hasName(const char *name) {
      return (name == m_metaPort.m_name );
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

    void Port::connectURL(const char*, const OU::PValue *, const OU::PValue *) {
      ocpiDebug("connectURL not allowed on this container !!");
      ocpiAssert( 0 );
    }

    ExternalPort::
    ExternalPort(Launcher::Connection &c, bool isProvider)
      : LocalPort(Container::baseContainer(),
		  isProvider ? *c.m_in.m_metaPort : *c.m_out.m_metaPort,
		  isProvider ? c.m_in.m_params : c.m_out.m_params),
	OU::Child<Application,ExternalPort,externalPort>
	(*(isProvider ? c.m_in.m_containerApp : c.m_out.m_containerApp), *this, NULL)
    {
      prepareOthers(isProvider ? c.m_out.m_scale : c.m_in.m_scale, 1);
    }

    ExternalPort::
    ~ExternalPort() {
    }

    // Bridge port constructor also does the equivalent of "startConnect" for itself.
    BridgePort::
    BridgePort(LocalPort &port, const OU::PValue *params)
      : BasicPort(port.container(), port.metaPort(), params)
    {
    }

    BridgePort::
    ~BridgePort() {
      if (m_dtPort)
	m_dtPort->reset();
    }
  }

  namespace API {
    ExternalBuffer::~ExternalBuffer(){}
    ExternalPort::~ExternalPort(){}
    Port::~Port() {}
  }
}
