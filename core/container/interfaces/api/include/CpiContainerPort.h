// This file exposes the CPI user interface for workers and the ports they own.
#ifndef CPI_CONTAINER_PORT_H
#define CPI_CONTAINER_PORT_H

#include "CpiParentChild.h"
#include "CpiContainerInterface.h"
#include "CpiMetadataWorker.h"


namespace CPI {
  namespace Container {

    class Worker;
    class ExternalPort;

    // Port connection dependency data. 
    struct PortConnectionDesc
    {
      CPI::RDT::Descriptors        data;        // Connection data

      // Cookie used to communicate the orignal port information from the user port
      // back to the provider port.  I.E. to complete your flowcontrol here is the information
      // that you need in the data parameter for your "port".
      PortDesc                  port;   
      
      // Container id that owns this port
      CPI::OS::uint32_t         container_id;

    };


    /**********************************
     * Port data structure
     *********************************/  
    class PortData
    {
    public:

      CPI::Metadata::Port myMetaPort;  // Deep copy to support all constructors
      CPI::OS::uint8_t    external;               // connected externally ?
      PortConnectionDesc  connectionData;      // Port Connection Dependency data

      PortData() : external(0) 
        {
          connectionData.port = reinterpret_cast<PortDesc>(this);
        }

      PortData( CPI::Metadata::Port & pmd ): myMetaPort(pmd) , external(0)
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
      BasePort(CPI::Metadata::Port & metaData );
      virtual ~BasePort();
      CPI::RDT::Desc_t &myDesc; // convenience
      // called after connection parameters have changed.
      virtual void checkConnectParams() = 0;
      void setConnectParams(CPI::Util::PValue *props);
      static void chooseRoles(int32_t &uRole, uint32_t uOptions,
                              int32_t &pRole, uint32_t pOptions);
    public:
      void applyConnectParams(CPI::Util::PValue *props);
    };



    class Port : public BasePort, public CPI::Util::Child<Worker,Port> {

      static const std::string empty;
      // This is here so we own this storage while we pass back references.
      std::string myInitialPortInfo;

    protected:
      bool canBeExternal;
      Interface &myContainer;
      Port(Worker &, CPI::Metadata::Port &mport);

      /*
      Port(Worker &, bool provider);
      */

      virtual ~Port(){}

    public:

    protected:

      // connect inside the container (colocated ports)
      virtual void connectInside(Port &other, CPI::Util::PValue *myProps=NULL, CPI::Util::PValue *otherProps=NULL) = 0;

      // other port is the same container type.  Return true if you do it.
      virtual bool connectLike(Port &other, CPI::Util::PValue *myProps=NULL,  CPI::Util::PValue *otherProps=NULL);

      virtual void finishConnection(CPI::RDT::Descriptors &other) = 0;

    public:
      void establishRoles(CPI::RDT::Descriptors &other);

      virtual void loopback(Port &);

      bool hasName(const char *name);

      inline bool isProvider() { return myMetaPort.provider; }

      inline bool isTwoWay() { return myMetaPort.twoway; }

      //      inline ezxml_t getXml() { return myXml; }
      // Local (possibly among different containers) connection: 1 step operation on the user port
      virtual void connect( Port &other, CPI::Util::PValue *myProps=NULL, CPI::Util::PValue *otherProps=NULL);


      virtual void disconnect( )
        throw ( CPI::Util::EmbeddedException )=0;


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

      virtual const std::string &getInitialProviderInfo(CPI::Util::PValue *p=NULL);

      // Step 2: (after passing initialProviderInfo to user side)
      // Give remote initial provider info to this local user port.
      // Get back user info:
      // Worst case:
      //     final user source info (user-source FIXED at user: 1 of 8))
      //     initial user target info/choices
      // Best case:
      //     we're done.  no further info exchange needed, return 0;

      virtual const std::string &setInitialProviderInfo(CPI::Util::PValue *p, const std::string &);

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
      virtual ExternalPort &connectExternal(const char *name, CPI::Util::PValue *props=NULL, CPI::Util::PValue *uprops=NULL) = 0;

    };



    // The direct interface for non-components to talk directly to the port,
    // in a non-blocking fashion.
    class ExternalBuffer : public CPI::Util::Child<ExternalPort, ExternalBuffer> {

    public:
      ExternalBuffer( ExternalPort& p )
        :CPI::Util::Child<ExternalPort,ExternalBuffer>(p){}
      ExternalBuffer(){}
      // For consumer buffers
      virtual void release() = 0;
      // For producer buffers
      virtual void put(uint8_t opCode, uint32_t length, bool endOfData) = 0;

    protected:
      virtual ~ExternalBuffer(){};

    };

    class ExternalPort : public CPI::Util::Parent<ExternalBuffer> {
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
      virtual void tryFlush() = 0;

      std::string & externalName(){return m_ExName;}

    private:
      std::string m_ExName;


    };
  }
}
#endif



