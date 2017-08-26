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

// This file exposes the OCPI user interface for workers and the ports they own.
#ifndef CONTAINER_PORT_H
#define CONTAINER_PORT_H

#include "OcpiContainerApi.h"

#include "OcpiUtilSelfMutex.h"
#include "OcpiPValue.h"
#include "OcpiRDTInterface.h"
#include "OcpiUtilPort.h"
#include "OcpiParentChild.h"
#include "ContainerApplication.h"
#include "ContainerLocalPort.h"

namespace OCPI {
  namespace DataTransport {
    class Port;
    class BufferUserFacet;
  }
  namespace Container {

    class Worker;
    class ExternalPort;
    class Container;
    class Launcher;

    // A worker member port managed in this process
    class Port : public LocalPort, public OCPI::API::Port {
      friend class ExternalPort;
      friend class BridgePort;
    protected:
      bool m_canBeExternal;

      Port(Container &container, const OCPI::Util::Port &mport, const OCPI::Util::PValue *params = NULL);
      virtual ~Port();
      bool canBeExternal() const { return m_canBeExternal; }
      virtual const std::string &name() const = 0;
      virtual Worker &worker() const = 0;
      // other port is the same container type.  Return true if you do it.
      virtual bool connectLike(Port &other, const OCPI::Util::PValue *myProps=NULL,
			       const OCPI::Util::PValue *otherProps=NULL);

      virtual void connectURL(const char* url, const OCPI::Util::PValue *myParams,
			      const OCPI::Util::PValue *otherParams);
      void portIsConnected();
    public:
      //      void determineRoles(OCPI::RDT::Descriptors &other);
      inline Port &containerPort() { return *this; }
      // If isLocal(), then this method can be used.
      virtual void localConnect(OCPI::DataTransport::Port &/*input*/) {}
      // If islocal(), then this can be used.
      virtual OCPI::DataTransport::Port &dtPort() {
        assert("Illegal call to dtPort"==0);
        return *(OCPI::DataTransport::Port *)this;
      }
      void disconnect() {}
      
#if 0
      OCPI::API::ExternalPort &
      connectExternal(const char *extName = NULL, const OCPI::Util::PValue *extParams = NULL,
		      const OCPI::Util::PValue *connectParams = NULL) {
	(void)extName;(void)extParams;(void)connectParams;
	return *(OCPI::API::ExternalPort*)NULL;
      }
#endif
    protected:
      bool hasName(const char *name);
    public:
      // Local (possibly among different containers) connection: 1 step operation on the user port
      void connect(OCPI::API::Port &other, const OCPI::API::PValue *myParams = NULL,
		   const OCPI::API::PValue *otherParams = NULL);
#if 0
      void connect(Launcher::Connection &c);
#endif

      // Connect to a URL based port.  This is currently used for DDS but may also be used for CORBA etc.

    };

    extern const char *port;
    template<class Wrk, class Prt, class Ext>
    class PortBase
      : public OCPI::Util::Child<Wrk, Prt, port>,
	public OCPI::Util::Parent<Ext>,
        public Port {
    protected:
      PortBase<Wrk,Prt,Ext>(Wrk &a_worker, Prt &prt, const OCPI::Util::Port &mport,
			    const OCPI::Util::PValue *params)
      : OCPI::Util::Child<Wrk,Prt,port>(a_worker, prt, mport.m_name.c_str()),
	Port(a_worker.parent().container(), mport, params) {}
      inline Worker &worker() const { return OCPI::Util::Child<Wrk,Prt,port>::parent(); }
    public:
      const std::string &name() const { return OCPI::Util::Child<Wrk,Prt,port>::name(); }
    };

    // The direct interface for non-components to talk directly to the port,
    // in a non-blocking fashion.
    extern const char *externalPort;
    class ExternalPort :
      public LocalPort, public OCPI::Util::Child<Application,ExternalPort,externalPort>
    {
      friend class LocalLauncher;
      friend class LocalPort;
    protected:
      ExternalPort(Launcher::Connection &c, bool isProvider);
      virtual ~ExternalPort();
      bool isInProcess(LocalPort */*other*/) const { return true; }
      bool canBeExternal() const { return true; }
    };

    // This class is for objects that implement fan-in or fan-out connectivity for a 
    // local member port.  E.g. is this local port is connected to 4 other member ports,
    // this local port will have 4 local bridge ports.
    // One class for both input and output, but with different constructors
    class BridgePort : public BasicPort {
      friend class LocalPort;
    protected:
      BridgePort(LocalPort &port, const OCPI::Util::PValue *params);
      ~BridgePort();
      bool canBeExternal() const { return true; }
    };
  }
}
#endif



