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

      Port(Container &container, const OCPI::Util::Port &mport,
	   const OCPI::Util::PValue *params = NULL);
      virtual ~Port();
      bool canBeExternal() const { return m_canBeExternal; }
      virtual const std::string &name() const = 0;
      virtual Worker &worker() const = 0;
#if 0
      // connect inside the container (colocated ports)
      // return true if it happened, and false if it wasn't and needs generic treatment
      //      virtual bool connectInside(Launcher::Connection &/*c*/);
      virtual void connectInside(Port &other,
				 const OCPI::Util::PValue *myParams,
				 const OCPI::Util::PValue *otherParams) = 0;
#endif
      // other port is the same container type.  Return true if you do it.
      virtual bool connectLike(Port &other, const OCPI::Util::PValue *myProps=NULL,
			       const OCPI::Util::PValue *otherProps=NULL);

      virtual void connectURL(const char* url, const OCPI::Util::PValue *myParams,
			      const OCPI::Util::PValue *otherParams);
#if 0
      // Create a container-specific external port
      virtual ExternalPort &createExternal(const char *extName, bool isProvider,
					   const OCPI::Util::PValue *extParams,
					   const OCPI::Util::PValue *connParams) = 0;
      void startLocalConnect(const OCPI::Util::PValue *extParams);
      void finishLocalConnect(const OCPI::RDT::Descriptors &other);
#endif
      void portIsConnected();
    public:
      //      void determineRoles(OCPI::RDT::Descriptors &other);
      inline Port &containerPort() { return *this; }
      // If isLocal(), then this method can be used.
      virtual void localConnect(OCPI::DataTransport::Port &/*input*/) {}
      // If islocal(), then this can be used.
      virtual OCPI::DataTransport::Port &dtPort() { return *(OCPI::DataTransport::Port *)0;}
      void disconnect() {}
      
      OCPI::API::ExternalPort &
      connectExternal(const char *extName = NULL, const OCPI::Util::PValue *extParams = NULL,
		      const OCPI::Util::PValue *connectParams = NULL) {
	(void)extName;(void)extParams;(void)connectParams;
	return *(OCPI::API::ExternalPort*)NULL;
      }
#if 0
      OCPI::API::ExternalPort &connectExternal(Launcher::Connection &c);
#endif

    protected:

#if 0
      void loopback(OCPI::API::Port &);
#endif
      bool hasName(const char *name);

#if 0
      // This is a hook for implementations to specialize the port
      enum ConnectionMode {CON_TYPE_NONE, CON_TYPE_RDMA, CON_TYPE_MESSAGE};
      virtual void setMode( ConnectionMode mode ) = 0;
#endif

    public:
      // Local (possibly among different containers) connection: 1 step operation on the user port
      void connect(OCPI::API::Port &other, const OCPI::API::PValue *myParams = NULL,
		   const OCPI::API::PValue *otherParams = NULL) {
	(void)other;(void)myParams;(void)otherParams;
      }
#if 0

      void connect(OCPI::API::Port &other, size_t otherN, size_t bufferSize,
		   const OCPI::API::PValue *myParams, const OCPI::API::PValue *otherParams);
#endif
      void connect(Launcher::Connection &c);


      // Connect to a URL based port.  This is currently used for DDS but may also be used for CORBA etc.

    };

    extern const char *port;
    template<class Wrk, class Prt, class Ext>
    class PortBase
      : public OCPI::Util::Child<Wrk, Prt, port>,
	public OCPI::Util::Parent<Ext>,
        public Port {
    protected:
      PortBase<Wrk,Prt,Ext>(Wrk &worker, Prt &prt, const OCPI::Util::Port &mport,
			    const OCPI::Util::PValue *params)
      : OCPI::Util::Child<Wrk,Prt,port>(worker, prt, mport.m_name.c_str()),
	Port(worker.parent().container(), mport, params) {}
      inline Worker &worker() const { return OCPI::Util::Child<Wrk,Prt,port>::parent(); }
    public:
      const std::string &name() const { return OCPI::Util::Child<Wrk,Prt,port>::name(); }
    };

    // The direct interface for non-components to talk directly to the port,
    // in a non-blocking fashion.  This class is the base for all implementations
    extern const char *externalPort;
    class ExternalPort :
      public LocalPort, public OCPI::Util::Child<Application,ExternalPort,externalPort>
    {
      friend class LocalLauncher;
      friend class LocalPort;
    protected:
      ExternalPort(Launcher::Connection &c, bool isProvider);
      ExternalPort(Port &port, bool isProvider, const OCPI::Util::PValue *extParams,
		   const OCPI::Util::PValue *connParams);
      virtual ~ExternalPort();
      bool isInProcess() const { return true; }
      bool canBeExternal() const { return true; }
#if 0
      const OCPI::RDT::Descriptors *
      startConnect(const OCPI::RDT::Descriptors *other, OCPI::RDT::Descriptors &buf,
		   bool &done);
      const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors *other, OCPI::RDT::Descriptors &feedback,
		    bool &done);
#endif
    };
#if 0
    extern const char *externalPort;
    template<class Prt, class Ext>
    class ExternalPortBase
      : public OCPI::Util::Child<Prt,Ext,externalPort>,
        public ExternalPort {
    protected:
      ExternalPortBase<Prt,Ext>(Prt &port, Ext &ext, const char *name,
				const OCPI::Util::PValue *extParams,
				const OCPI::Util::PValue *connParams,
				bool isProvider)
      : OCPI::Util::Child<Prt,Ext,externalPort>(port, ext, name),
	ExternalPort(port, isProvider, extParams, connParams) {}
    public:
      const std::string &name() const { return OCPI::Util::Child<Prt,Ext,externalPort>::name(); }
    };
#endif

    // This class is for objects that implement fan-in or fan-out connectivity for a 
    // local member port.  E.g. is this local port is connected to 4 other member ports,
    // this local port will have 4 local bridge ports.
    // One class for both input and output, but with different constructors
    class BridgePort : public BasicPort {
      friend class LocalPort;
    protected:
      BridgePort(LocalPort &port, const OCPI::Util::PValue *params);
      ~BridgePort();
      bool isInProcess() const { return true; }
      bool canBeExternal() const { return true; }
#if 0
      const OCPI::RDT::Descriptors *startConnect(const OCPI::RDT::Descriptors *other,
						 OCPI::RDT::Descriptors &buf, bool &done);
						 
      // Finish the output side of the connection, and return the right descriptor to return.
      const OCPI::RDT::Descriptors *finishConnect(const OCPI::RDT::Descriptors *other,
						  OCPI::RDT::Descriptors &feedback, bool &done);
#endif
    };
  }
}
#endif



