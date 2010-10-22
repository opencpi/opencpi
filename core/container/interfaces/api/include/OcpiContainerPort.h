
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
    public:

      OCPI::Metadata::Port myMetaPort;  // Deep copy to support all constructors
      OCPI::OS::uint8_t    external;               // connected externally ?
      PortConnectionDesc  connectionData;      // Port Connection Dependency data

      PortData() : external(0) 
        {
          connectionData.port = reinterpret_cast<PortDesc>(this);
        }

      PortData( OCPI::Metadata::Port & pmd ): myMetaPort(pmd) , external(0)
        {
          connectionData.port = reinterpret_cast<PortDesc>(this);
        }
        PortData & operator = ( const PortData & lhs )
          {
            myMetaPort = lhs.myMetaPort;
            memcpy(&connectionData,&lhs.connectionData,sizeof(PortConnectionDesc));
            external = lhs.external;
            return *this;
          }
        PortData( PortData& lhs ) 
          {
            *this = lhs;
          }
    };


    // The class used by both ExternalPorts (not associated with a worker) and Ports (owned by worker)
    class BasePort : public PortData {
    protected:
      BasePort(OCPI::Metadata::Port & metaData );
      virtual ~BasePort();
      OCPI::RDT::Desc_t &myDesc; // convenience
      // called after connection parameters have changed.
      virtual void checkConnectParams() = 0;
      void setConnectParams(OCPI::Util::PValue *props);
      static void chooseRoles(int32_t &uRole, uint32_t uOptions,
                              int32_t &pRole, uint32_t pOptions);
    public:
      void applyConnectParams(OCPI::Util::PValue *props);
    };



    class Port : public BasePort, public OCPI::Util::Child<Worker,Port> {

      static const std::string empty;


    protected:
      // This is here so we own this storage while we pass back references.
      std::string myInitialPortInfo;
      bool canBeExternal;
      Interface &myContainer;
      Port(Worker &, OCPI::Metadata::Port &mport);

      /*
      Port(Worker &, bool provider);
      */

      virtual ~Port(){}

    public:

    protected:

      // connect inside the container (colocated ports)
      virtual void connectInside(Port &other, OCPI::Util::PValue *myProps=NULL, OCPI::Util::PValue *otherProps=NULL) = 0;

      // other port is the same container type.  Return true if you do it.
      virtual bool connectLike(Port &other, OCPI::Util::PValue *myProps=NULL,  OCPI::Util::PValue *otherProps=NULL);

      virtual void finishConnection(OCPI::RDT::Descriptors &other) = 0;

    public:
      void establishRoles(OCPI::RDT::Descriptors &other);

      virtual void loopback(Port &);

      bool hasName(const char *name);

      inline bool isProvider() { return myMetaPort.provider; }

      inline bool isTwoWay() { return myMetaPort.twoway; }

      //      inline ezxml_t getXml() { return myXml; }
      // Local (possibly among different containers) connection: 1 step operation on the user port
      virtual void connect( Port &other, OCPI::Util::PValue *myProps=NULL, OCPI::Util::PValue *otherProps=NULL);


      virtual void disconnect( )
        throw ( OCPI::Util::EmbeddedException )=0;


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

      virtual const std::string &getInitialProviderInfo(OCPI::Util::PValue *p=NULL);

      // Step 2: (after passing initialProviderInfo to user side)
      // Give remote initial provider info to this local user port.
      // Get back user info:
      // Worst case:
      //     final user source info (user-source FIXED at user: 1 of 8))
      //     initial user target info/choices
      // Best case:
      //     we're done.  no further info exchange needed, return 0;

      virtual const std::string &setInitialProviderInfo(OCPI::Util::PValue *p, const std::string &);

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

      // Connect directly to this port, which creates a UserPort object.
      virtual ExternalPort &connectExternal(const char *name, OCPI::Util::PValue *props=NULL, OCPI::Util::PValue *uprops=NULL) = 0;

    };



    // The direct interface for non-components to talk directly to the port,
    // in a non-blocking fashion.
    class ExternalBuffer : public OCPI::Util::Child<ExternalPort, ExternalBuffer> {

    public:
      ExternalBuffer( ExternalPort& p )
        :OCPI::Util::Child<ExternalPort,ExternalBuffer>(p){}
      ExternalBuffer(){}
      // For consumer buffers
      virtual void release() = 0;
      // For producer buffers
      virtual void put(uint8_t opCode, uint32_t length, bool endOfData) = 0;

    protected:
      virtual ~ExternalBuffer(){};

    };

    class ExternalPort : public OCPI::Util::Parent<ExternalBuffer> {
    protected:
      ExternalPort( const char *name )
        :m_ExName(name){};
      virtual ~ExternalPort(){};
    public:
      // Return zero if there is no buffer ready
      // data pointer may be null if end-of-data is true.
      // This means NO MESSAGE, not a zero length message.
      // I.e. if "data" is null, length is not valid.
      virtual ExternalBuffer *
        getBuffer(uint8_t &opCode, uint8_t *&data, uint32_t &length, bool &endOfData) = 0;
      // Return zero when no buffers are available.
      virtual ExternalBuffer *getBuffer(uint8_t *&data, uint32_t &length) = 0;
      // Use this when end of data indication happens AFTER the last message.
      // Use the endOfData argument to put, when it is known at that time
      virtual void endOfData() = 0;
      // Return whether there are still buffers to send that can't be flushed now.
      virtual bool tryFlush() = 0;

      std::string & externalName(){return m_ExName;}

    private:
      std::string m_ExName;


    };
  }
}
#endif



