
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
#ifndef OCPI_CONTAINER_PORT_H
#define OCPI_CONTAINER_PORT_H

#include "OcpiParentChild.h"
#include "OcpiUtilSelfMutex.h"
#include "OcpiPValue.h"
#include "OcpiRDTInterface.h"
#include "OcpiMetadataPort.h"
#include "OcpiContainerApi.h"

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
     * Port data structure
     *********************************/  
    const unsigned DEFAULT_NBUFFERS = 2;
    const unsigned DEFAULT_BUFFER_SIZE = 2*1024;
    class PortData
    {
      OCPI::Metadata::PortOrdinal m_ordinal;
      bool m_isProvider; // perhaps overriding bidirectional
      PortConnectionDesc *m_connectionData;
      PortConnectionDesc  connectionData;      // Port Connection Dependency data
      // Defaults when no other source provides this.
      // (protocol or port metadata, or port or connection params)

    public:
      virtual ~PortData(){};
      inline bool isProvider() { return m_isProvider; }
      inline bool isOutput() {return !isProvider(); }
      inline bool isInput() {return isProvider(); }
      inline OCPI::Metadata::PortOrdinal ordinal() { return m_ordinal; }
      virtual inline PortConnectionDesc &  getData() {
	return m_connectionData ? *m_connectionData : connectionData;
      }
      PortData(const OCPI::Metadata::Port &mPort, bool isProvider, unsigned xferOptions,
	       const OCPI::Util::PValue *params = NULL, PortConnectionDesc *desc = NULL);
      void setPortParams(const OCPI::Metadata::Port &mPort, const OCPI::Util::PValue *params);
    };


    // The class used by both ExternalPorts (not associated with a worker) and Ports (owned by worker)
    class BasicPort : public PortData, protected OCPI::Util::SelfRefMutex {
    public:
      inline const OCPI::Metadata::Port &metaPort() const { return m_metaPort; }
    protected:

      OCPI::RDT::Desc_t &myDesc; // convenience
      const OCPI::Metadata::Port &m_metaPort;

      BasicPort(const OCPI::Metadata::Port &mPort, bool isProvider, unsigned xferOptions,
		OCPI::OS::Mutex &mutex,	const OCPI::Util::PValue *params, PortConnectionDesc *desc = NULL);
      virtual ~BasicPort();
      // called after connection parameters have changed.
      virtual void startConnect(const OCPI::RDT::Descriptors *other, const OCPI::Util::PValue *params);
      void setConnectParams(const OCPI::Util::PValue *params);
      static void chooseRoles(int32_t &uRole, uint32_t uOptions,
                              int32_t &pRole, uint32_t pOptions);
    public:
      void applyConnectParams(const OCPI::RDT::Descriptors *other, const OCPI::Util::PValue *props);
    };



    class Port : public BasicPort, public OCPI::API::Port {

      friend class ExternalPort;
      static const std::string s_empty;
    protected:
      Container &m_container;
      std::string m_initialPortInfo;
      bool m_canBeExternal;
      // This is here so we own this storage while we pass back references.
      Port(Worker &worker, const OCPI::Metadata::Port &mport, bool isProvider,
	   unsigned options, const OCPI::Util::PValue *params = NULL, PortConnectionDesc *desc = NULL);
      virtual ~Port(){}
      // Convenience navigation
      Container &container() const;
      virtual const std::string &name() const = 0;
      virtual Worker &worker() const = 0;
      // The implementation tells us whether the port is in the process and uses dt ports
      virtual bool isLocal() const  = 0;//{ return false; }
      // connect inside the container (colocated ports)
      virtual void connectInside(Port &other, const OCPI::Util::PValue *myProps=NULL) = 0;

      // other port is the same container type.  Return true if you do it.
      virtual bool connectLike(Port &other, const OCPI::Util::PValue *myProps=NULL,
			       const OCPI::Util::PValue *otherProps=NULL);

      // Finish this side of the connection, and return the right descriptor to return.
      virtual const OCPI::RDT::Descriptors *finishConnect(const OCPI::RDT::Descriptors &other,
							     OCPI::RDT::Descriptors &feedback) = 0;
      // Return true and fill in the string if you want a protocol
      virtual const char *getPreferredProtocol() { return NULL; }

      // Create a container-specific external port
      virtual ExternalPort &createExternal(const char *extName, bool isProvider,
					   const OCPI::Util::PValue *extParams,
					   const OCPI::Util::PValue *connParams) = 0;
    public:
      // If isLocal(), then this method can be used.
      virtual void localConnect(OCPI::DataTransport::Port &/*input*/) {}
      // If islocal(), then this can be used.
      virtual OCPI::DataTransport::Port &dtPort() { return *(OCPI::DataTransport::Port *)0;}
      
      /**
         @brief
         packPortDesc

         This method is used to "pack" a port descriptor into a string that
         can be sent over a wire.        

         @param [ in ] port
         Port to be packed.

         @retval std::string packed port descriptor

         ****************************************************************** */
      static void packPortDesc(const OCPI::RDT::Descriptors&  port, std::string &out )
        throw ();


      /**
         @brief
         unpackPortDesc

         This method is used to "unpack" a port descriptor into a Port.


         @param [ in ] desc
         String descriptor previously created with "packPort *".

         @param [ in ] pd
         Unpacked port descriptor.

         @retval bool true if method successful.

         ****************************************************************** */
      static bool unpackPortDesc( const std::string& desc, OCPI::RDT::Descriptors &desc_storage )
        throw ();
      OCPI::API::ExternalPort &connectExternal(const char *extName = NULL, const OCPI::Util::PValue *extParams = NULL,
					       const OCPI::Util::PValue *connectParams = NULL);

    protected:
      void determineRoles(OCPI::RDT::Descriptors &other);

      void loopback(OCPI::API::Port &);

      bool hasName(const char *name);

      // This is a hook for implementations to specialize the port
      enum ConnectionMode {CON_TYPE_NONE, CON_TYPE_RDMA, CON_TYPE_MESSAGE};
      virtual void setMode( ConnectionMode mode ) = 0;

    public:
      // Local (possibly among different containers) connection: 1 step operation on the user port
      virtual void connect( OCPI::API::Port &other, const OCPI::API::PValue *myProps,
			    const OCPI::API::PValue *otherProps);


      // Connect to a URL based port.  This is currently used for DDS but may also be used for CORBA etc.
      void connectURL( const char* url, const OCPI::Util::PValue *myProps,
		       const OCPI::Util::PValue *otherProps);

      // Local connection within a container
      // Remote connection: up to 5 steps! worst case.
      // Names are chosen for worst case.
      // "final" means all needed info, implying resource commitments
      // "initial" can mean only choices, without resource commitments
      //           but may contain "final" info in some nice cases.

      // Step 1:
      // Get initial info/choices about this local (provider) port
      // Worst case we have:
      //     only provider target choices, no commitments
      //     no provider->user information at all
      // Best case: (only one method for user->provider)
      //     final user->provider target info and provider->user target info

      virtual  void getInitialProviderInfo(const OCPI::Util::PValue *p, std::string &out);

      // Step 2: (after passing initialProviderInfo to user side)
      // Give remote initial provider info to this local user port.
      // Get back user info:
      // Worst case:
      //     final user source info (user-source FIXED at user: 1 of 8))
      //     initial user target info/choices
      // Best case:
      //     we're done.  no further info exchange needed, return 0;

      virtual void setInitialProviderInfo(const OCPI::Util::PValue *p, const std::string &, std::string &out);

      // Step 3: (after passing initialUserInfo to provider side)
      // Give remote initial user info to this local provider port.
      // Get back final provider info:
      // Worst case:
      //     (user-source FIXED at provider: 2 of 8)
      //     (provider-target FIXED at provider: 3 of 8)
      //     (provider-source FIXED at provider: 4 of 8)
      // Best case:
      //     we're done, no further info exchange needed, return 0;

      virtual void setInitialUserInfo(const std::string &, std::string &out);

      // Step 4: (after passing finalProviderInfo to user side)
      // Give remote final provider info to this local user port.
      // Get back final user info:
      // Worst case:
      //     (provider-target FIXED at user: 5 of 8)
      //     (provider-source FIXED at user: 6 of 8)
      //     (user-target FIXED at user: 7 of 8)
      //     final provider->user target info
      // Best case:
      //     we're done, no further info exchange needed, return 0;

      virtual void setFinalProviderInfo(const std::string &, std::string &out);

      // Step 5: (after passing finalUserInfo to provider side)
      // Worst case:
      //     (user-target FIXED at provider: 8 of 8);
      virtual void setFinalUserInfo(const std::string &);
    };

    extern const char *port;
    template<class Wrk, class Prt, class Ext>
    class PortBase
      : public OCPI::Util::Child<Wrk, Prt, port>,
      public OCPI::Util::Parent<Ext>,
        public Port {
    protected:
      PortBase<Wrk,Prt,Ext>(Wrk &worker, Prt &prt, const OCPI::Metadata::Port &mport, bool isProvider,
			    unsigned xferOptions, const OCPI::Util::PValue *params,
			    PortConnectionDesc *desc = NULL)
      : OCPI::Util::Child<Wrk,Prt,port>(worker, prt, mport.name), Port(worker, mport,
						      isProvider, xferOptions, params, desc) {}
      inline Worker &worker() const { return OCPI::Util::Child<Wrk,Prt,port>::parent(); }
    public:
      const std::string &name() const { return OCPI::Util::Child<Wrk,Prt,port>::name(); }
    };

    class ExternalBuffer : API::ExternalBuffer {
      friend class ExternalPort;
      OCPI::DataTransport::BufferUserFacet *m_dtBuffer;
      OCPI::DataTransport::Port *m_dtPort;
      ExternalBuffer();
      void release();
      void put( size_t length, uint8_t opCode, bool /*endOfData*/);
    };

    // The direct interface for non-components to talk directly to the port,
    // in a non-blocking fashion.  This class is the base for all implementations
    class ExternalPort : public BasicPort, public OCPI::API::ExternalPort {
      OCPI::DataTransport::Port *m_dtPort;
      ExternalBuffer m_lastBuffer;
    protected:
      ExternalPort(Port &port, bool isProvider, const OCPI::Util::PValue *extParams,
		   const OCPI::Util::PValue *connParams);
      virtual ~ExternalPort();
      OCPI::API::ExternalBuffer
        *getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end),
	*getBuffer(uint8_t *&data, size_t &length);
      void endOfData();
      bool tryFlush();
    };

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
    };
  }
}
#endif



