
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
#include "OcpiContainerApi.h"
#include "OcpiContainerInterface.h"
#include "OcpiMetadataWorker.h"


namespace OCPI {
  namespace Container {

    class Worker;
    class ExternalPort;

    // Port connection dependency data. 
    struct PortConnectionDesc
    {
      OCPI::RDT::Descriptors        data;        // Connection data

      // Cookie used to communicate the orignal port information from the user port
      // back to the provider port.  I.E. to complete your flowcontrol here is the information
      // that you need in the data parameter for your "port".
      PortDesc                  port;   
      
      // Container id that owns this port
      OCPI::OS::uint32_t         container_id;

    };


    /**********************************
     * Port data structure
     *********************************/  
    class PortData
    {
      bool m_isProvider; // perhaps overriding bidirectional
    public:
      inline bool isProvider() { return m_isProvider; }
      OCPI::OS::uint8_t    external;               // connected externally ?
      PortConnectionDesc  connectionData;      // Port Connection Dependency data

      PortData() : external(0) 
        {
          connectionData.port = reinterpret_cast<PortDesc>(this);
        }

    PortData( bool isProvider )
      : m_isProvider(isProvider), external(0)
        {
          connectionData.port = reinterpret_cast<PortDesc>(this);
        }
      // FIXME: why is his just not the default constructor?
        PortData & operator = ( const PortData & lhs )
          {
            memcpy(&connectionData,&lhs.connectionData,sizeof(PortConnectionDesc));
            external = lhs.external;
            return *this;
          }
      // FIXME: where is this happening?
        PortData( PortData& lhs ) 
#if 0 // a test to see if it is used anywhere
          {
            *this = lhs;
          }
#else
	;
#endif
    };


    // The class used by both ExternalPorts (not associated with a worker) and Ports (owned by worker)
    class BasicPort : public PortData {
    protected:
#if 0
      // These are values that are initialized from metadata, but can be overriden at runtime,
      // or by the specific port type
      uint32_t m_minBufferSize;
      uint32_t m_minBufferCount;
      uint32_t m_maxBufferSize;
#endif
      OCPI::RDT::Desc_t &myDesc; // convenience
      const OCPI::Metadata::Port &m_metaPort;

      BasicPort(const OCPI::Metadata::Port & metaData, bool isProvider);
      virtual ~BasicPort();
      inline const OCPI::Metadata::Port &metaPort() const { return m_metaPort; }
      // called after connection parameters have changed.
      virtual void checkConnectParams() = 0;
      void setConnectParams(const OCPI::Util::PValue *props);
      static void chooseRoles(int32_t &uRole, uint32_t uOptions,
                              int32_t &pRole, uint32_t pOptions);
    public:
      void applyConnectParams(const OCPI::Util::PValue *props);
    };



    class Port : public BasicPort, public OCPI::API::Port {

      static const std::string s_empty;
    protected:
      Container &m_container;
      std::string m_initialPortInfo;
      bool m_canBeExternal;
      // This is here so we own this storage while we pass back references.
      Port(Container &container, const OCPI::Metadata::Port &mport, bool isProvider,
	   const OCPI::Util::PValue *props = NULL);
      virtual ~Port(){}
      Container &container() const;

      // connect inside the container (colocated ports)
      virtual void connectInside(Port &other, const OCPI::Util::PValue *myProps=NULL,
				 const OCPI::Util::PValue *otherProps=NULL) = 0;

      // other port is the same container type.  Return true if you do it.
      virtual bool connectLike(Port &other, const OCPI::Util::PValue *myProps=NULL,
			       const OCPI::Util::PValue *otherProps=NULL);

      virtual void finishConnection(OCPI::RDT::Descriptors &other) = 0;

    public:
      void establishRoles(OCPI::RDT::Descriptors &other);

      void loopback(OCPI::API::Port &);

      bool hasName(const char *name);

      //      inline bool isTwoWay() { return m_metaPort.twoway; }

      //      inline ezxml_t getXml() { return myXml; }
      // Local (possibly among different containers) connection: 1 step operation on the user port
      void connect( OCPI::API::Port &other, const OCPI::Util::PValue *myProps,
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

      virtual const std::string &getInitialProviderInfo(const OCPI::Util::PValue *p);

      // Step 2: (after passing initialProviderInfo to user side)
      // Give remote initial provider info to this local user port.
      // Get back user info:
      // Worst case:
      //     final user source info (user-source FIXED at user: 1 of 8))
      //     initial user target info/choices
      // Best case:
      //     we're done.  no further info exchange needed, return 0;

      virtual const std::string &setInitialProviderInfo(const OCPI::Util::PValue *p, const std::string &);

      // Step 3: (after passing initialUserInfo to provider side)
      // Give remote initial user info to this local provider port.
      // Get back final provider info:
      // Worst case:
      //     (user-source FIXED at provider: 2 of 8)
      //     (provider-target FIXED at provider: 3 of 8)
      //     (provider-source FIXED at provider: 4 of 8)
      // Best case:
      //     we're done, no further info exchange needed, return 0;

      virtual const std::string &setInitialUserInfo(const std::string &);

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

      virtual const std::string &setFinalProviderInfo(const std::string &);

      // Step 5: (after passing finalUserInfo to provider side)
      // Worst case:
      //     (user-target FIXED at provider: 8 of 8);
      virtual void setFinalUserInfo(const std::string &);
    };

    typedef OCPI::API::ExternalBuffer ExternalBuffer;

    // The direct interface for non-components to talk directly to the port,
    // in a non-blocking fashion.
    class ExternalPort : public BasicPort, public OCPI::API::ExternalPort {
    protected:
      ExternalPort(const OCPI::Metadata::Port & metaPort, bool isProvider,
		   const OCPI::Util::PValue *props);
      virtual ~ExternalPort(){};
      void checkConnectParams();
    };
  }
}
#endif



