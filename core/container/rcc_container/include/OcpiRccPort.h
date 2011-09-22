
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


/*
 * Abstact:
 *   This file contains the interface and data structures for the JTRS DSP Worker.
 *
 * Revision History: 
 *
 *    06/23/09  John Miller
 *    Added code to handle RCC_ERROR and RCC_FATAL return codes.
 * 
 *    06/01/05  John Miller
 *    Initial revision
 *
 */


#ifndef OCPI_RCC_PORT_H_
#define OCPI_RCC_PORT_H_

#include <OcpiOsDataTypes.h>
#include <OcpiContainerDataTypes.h>
#include <OcpiContainerPort.h>
#include <OcpiRccApplication.h>
#include <OcpiBuffer.h>
#include <RCC_Worker.h>

namespace DataTransport {
  namespace Interface {
    class Circuit;
  }
}

namespace DataTransfer {
  namespace Msg {
    class MsgChannel;
  }
}

namespace DtI = DataTransport::Interface;

namespace OCPI {

  namespace DataTransport {
    class Buffer;
    class Port;
    class Circuit;
  }


  namespace RCC {

    class Application;
    class Worker;
    class ExternalPort;
    struct OpaquePortData;
    class ExternalBuffer;
    class PortDelegator;


    // This port does the actual work;
    class Port;
    class PortDelegator : public OCPI::Container::PortBase<Worker, Port, ExternalPort> {
    public:
      PortDelegator(  Worker& w, const OCPI::Util::PValue *props, const OCPI::Metadata::Port & pmd, const char* endpoint );
      virtual ~PortDelegator();

      virtual void checkConnectParams(){}
      virtual void setMode( ConnectionMode ){}
      virtual void connectInside( OCPI::Container::Port &, const OCPI::Util::PValue *, const OCPI::Util::PValue *){}
      virtual void finishConnection(OCPI::RDT::Descriptors &){}
      virtual bool definitionComplete(){return true;}

      virtual OpaquePortData *                     &opaque(){return m_opaque;}
      virtual inline OCPI::OS::Mutex               &mutex() { return m_mutex;}
      inline OCPI::Metadata::PortOrdinal           portOrdinal(){return m_portOrdinal;}
      inline bool isOutput() {return !m_metaPort.provider;}

      // Port control methods
      virtual void advance( OCPI::DataTransport::BufferUserFacet* buffer, uint32_t opcode=0, uint32_t length=0 )=0;
      virtual bool hasEmptyOutputBuffer()=0;
      virtual OCPI::DataTransport::BufferUserFacet* getNextEmptyOutputBuffer()=0;
      virtual bool  hasFullInputBuffer()=0;
      virtual OCPI::DataTransport::BufferUserFacet* getNextFullInputBuffer()=0;
      virtual void sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* src_buf, unsigned int len )=0;
      virtual uint32_t getBufferCount()=0;
      virtual OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t tid )=0;
      virtual uint32_t getBufferLength() = 0;
     
    protected:
      OCPI::DataTransport::Port *m_dtPort;
      OCPI::Metadata::PortOrdinal  m_portOrdinal;
      OCPI::OS::Mutex &m_mutex;
      OpaquePortData * m_opaque;
    };


    class Port : public PortDelegator  {

    public:

      Port( Worker& w, const OCPI::Util::PValue *props, const OCPI::Metadata::Port & pmd,
	    const char * endpoint);
      virtual ~Port();

      virtual inline OCPI::OS::Mutex &mutex() 
      { 
	if(m_delegateTo) {
	  return m_delegateTo->mutex();
	}
	return mutex();
      }

      virtual inline OCPI::Container::PortConnectionDesc &  getData()
      {
	ocpiAssert( m_delegateTo );
	return m_delegateTo->getData();
      }

      void setMode( ConnectionMode mode );
      virtual void connect( OCPI::Container::Port &other, const OCPI::API::PValue *myProps=NULL,
			    const OCPI::API::PValue *otherProps=NULL);

      virtual inline void connectURL( const char* url, const OCPI::Util::PValue *myProps,
				      const OCPI::Util::PValue * otherProps)
      {

	setMode( OCPI::Container::Port::CON_TYPE_MESSAGE );
	m_delegateTo->connectURL( url, myProps, otherProps );
      }

      virtual inline void disconnect()
        throw ( OCPI::Util::EmbeddedException )
      {
	if(m_delegateTo) {
	  m_delegateTo->disconnect();
	}
      }

      virtual inline OCPI::API::ExternalPort& connectExternal(const char* n, const OCPI::Util::PValue* p,
							      const OCPI::Util::PValue* po)
      {
	// only supported in RDMA mode
	if ( ! m_delegateTo ) {
	  setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	}
	return m_delegateTo->connectExternal(n,p,po);
      }

      virtual inline bool definitionComplete(){
	if(m_delegateTo) {
	  return m_delegateTo->definitionComplete();
	}
	return false;
      }
      virtual void checkConnectParams()
      {
	if(m_delegateTo) {
	  m_delegateTo->checkConnectParams();
	}

      }
      virtual void connectInside(OCPI::Container::Port & other, const OCPI::Util::PValue * my_props,
				 const OCPI::Util::PValue * other_props)
      {
	ocpiAssert ( m_delegateTo);
	m_delegateTo->connectInside(other,my_props,other_props);
      }
      virtual void finishConnection(OCPI::RDT::Descriptors& d)
      {
	ocpiAssert ( m_delegateTo);
	m_delegateTo->finishConnection(d);
      }

      // Connection routines
      virtual const std::string& getInitialProviderInfo(const OCPI::API::PValue*p)
      {
	// only supported in RDMA mode
	if ( ! m_delegateTo ) setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	return m_delegateTo->getInitialProviderInfo(p);
      }
      virtual const std::string& setInitialProviderInfo(const OCPI::API::PValue*p, const std::string&s)
      {
	// only supported in RDMA mode
	if ( ! m_delegateTo ) setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	return m_delegateTo->setInitialProviderInfo(p,s);
      }
      virtual const std::string& setInitialUserInfo(const std::string&s) 
      {
	// only supported in RDMA mode
	if ( ! m_delegateTo ) setMode( OCPI::Container::Port::CON_TYPE_RDMA );
	return m_delegateTo->setInitialUserInfo(s);
      }
      virtual const std::string& setFinalProviderInfo(const std::string&s) 
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->setFinalProviderInfo(s);
      }
      virtual void setFinalUserInfo(const std::string&s)
      {
	ocpiAssert ( m_delegateTo);
	m_delegateTo->setFinalUserInfo(s);
      }



      // Port control methods
      virtual inline OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t tid )
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->getBuffer( tid );
      }

      virtual inline uint32_t getBufferLength()
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->getBufferLength();
      }

      virtual inline uint32_t getBufferCount()
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->getBufferCount();
      }

      virtual inline void sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* src_buf, unsigned int len )
      {
	ocpiAssert ( m_delegateTo);
	m_delegateTo->sendZcopyInputBuffer(src_buf,len);
      }

      virtual inline void advance( OCPI::DataTransport::BufferUserFacet* buffer, uint32_t opcode=0, uint32_t length=0 )
      {
	ocpiAssert ( m_delegateTo);
	m_delegateTo->advance(buffer,opcode,length);
      }
      virtual inline bool hasEmptyOutputBuffer()
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->hasEmptyOutputBuffer();
      }
      virtual inline OCPI::DataTransport::BufferUserFacet* getNextEmptyOutputBuffer()
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->getNextEmptyOutputBuffer();
      }
      virtual inline bool  hasFullInputBuffer()
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->hasFullInputBuffer();
      }
      virtual inline OCPI::DataTransport::BufferUserFacet* getNextFullInputBuffer()
      {
	ocpiAssert ( m_delegateTo);
	return m_delegateTo->getNextFullInputBuffer();
      }

      inline OpaquePortData *                   &opaque()
	{
	  if ( m_delegateTo ) {
	    return m_delegateTo->opaque();
	  }
	  return  m_opaque;
	}

    protected:


      // Our initial properties
      const OCPI::Util::PValue * m_props;

      PortDelegator *  m_delegateTo;

      // Our initial endpoint
      std::string m_endpoint;

    };


    class MessagePort : public PortDelegator  {    

    public:
      MessagePort( Worker& w, const OCPI::Util::PValue *props, const OCPI::Metadata::Port & pmd );
      
      void connectURL( const char* url, const OCPI::Util::PValue *myProps,
		       const OCPI::Util::PValue *otherProps);
      inline void disconnect()
        throw ( OCPI::Util::EmbeddedException );
      OCPI::Container::ExternalPort& connectExternal(const char*, const OCPI::Util::PValue*,
						     const OCPI::Util::PValue*);
      void advance( OCPI::DataTransport::BufferUserFacet* buffer, uint32_t opcode, uint32_t length );
      bool hasEmptyOutputBuffer();
      OCPI::DataTransport::BufferUserFacet*  getNextEmptyOutputBuffer();
      bool  hasFullInputBuffer();
      OCPI::DataTransport::BufferUserFacet*  getNextFullInputBuffer();
      void sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* src_buf, unsigned int len );
      OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t index );
      uint32_t getBufferLength();
      uint32_t getBufferCount();

      virtual ~MessagePort();
    private:
      DataTransfer::Msg::MsgChannel *m_msgChannel;
    };



    class RDMAPort;
    class RDMAPort : public PortDelegator  {
    public:

      friend class Port;

      RDMAPort( Worker& w, const OCPI::Util::PValue *props, const OCPI::Metadata::Port & pmd,
		const char * endpoint );
      virtual ~RDMAPort();

      inline OpaquePortData *  &opaque() {return m_opaque;}

      void disconnect()
        throw ( OCPI::Util::EmbeddedException );

      // Connection routines
      const std::string& getInitialProviderInfo(const OCPI::API::PValue*);
      const std::string& setInitialProviderInfo(const OCPI::API::PValue*, const std::string&);
      const std::string& setInitialUserInfo(const std::string&);
      const std::string& setFinalProviderInfo(const std::string&);
      void setFinalUserInfo(const std::string&);
      void checkConnectParams(){}
      void connectInside(OCPI::Container::Port & other, const OCPI::Util::PValue * my_props,
			 const OCPI::Util::PValue * other_props);
      void finishConnection(OCPI::RDT::Descriptors&){}
      OCPI::Container::ExternalPort& connectExternal(const char*, const OCPI::Util::PValue*,
						     const OCPI::Util::PValue*);
      virtual void connect( OCPI::Container::Port &other, const OCPI::API::PValue *myProps=NULL,
			    const OCPI::API::PValue *otherProps=NULL);        
      virtual void connectURL( const char* url, const OCPI::Util::PValue *myProps,
			       const OCPI::Util::PValue *otherProps);
      bool definitionComplete();
      
      // Port Control methods      
      void advance( OCPI::DataTransport::BufferUserFacet * buffer, uint32_t opcode, uint32_t length );
      bool hasEmptyOutputBuffer();
      OCPI::DataTransport::BufferUserFacet* getNextEmptyOutputBuffer();
      bool  hasFullInputBuffer();
      OCPI::DataTransport::BufferUserFacet* getNextFullInputBuffer();
      void sendZcopyInputBuffer( OCPI::DataTransport::BufferUserFacet* src_buf, unsigned int len );
      OCPI::DataTransport::BufferUserFacet* getBuffer( uint32_t index );
      uint32_t getBufferLength();
      uint32_t getBufferCount();
      
      // Member access methods
      inline OCPI::DataTransport::Circuit *      &circuit(){return m_circuit;}

    private:

      inline OCPI::DataTransport::Port * dtPort(){return m_dtPort;}

      void
        setOutputFlowControl( OCPI::Container::PortData * srcPort )
	throw ( OCPI::Util::EmbeddedException );

      void connectInputPort( PortData *    inputPort,    
			     std::string&  lPort,
			     const OCPI::Util::PValue*       props
			     )
	throw ( OCPI::Util::EmbeddedException );

      void initInputPort();
      void initOutputPort();
      void processPortProperties( const OCPI::Util::PValue* props );
      void disconnect( OCPI::Container::PortData* sp, OCPI::Container::PortData* input );


      /**********************************
       * This method is used to connect an external port decribed in the
       * the PortDependencyData structure to a output port for the given worker.
       *********************************/
      OCPI::Container::PortConnectionDesc 
	connectExternalInputPort( 
				 OCPI::Container::PortData *     inputPort,
				 const OCPI::Util::PValue*       props=NULL
				 );

      /**********************************
       * This method is used to connect the external ports decribed in the
       * the PortDependencyData structure to output port for the given worker.
       *********************************/
      void connectInternalInputPort( 
				    OCPI::Container::Port *          inputPort,  
				    const OCPI::Util::PValue*       properties    
				    );

      // Our circuit
      OCPI::DataTransport::Circuit * m_circuit;   

      // local shadow port descriptor (string form)
      std::string m_localShadowPort;

      // our descriptor (string form)
      std::string m_ourFinalDescriptor;

      // Our connection cookie
      struct MyConnection {
	MyConnection():connected(false),nosrcd(false){}
	void init( PortData * src, bool slocal,  PortData * input, bool tlocal);
	bool connected;
	RDMAPort  *   lsrc;
	bool                  nosrcd;
	RDMAPort  *   linput;
	OCPI::Container::PortData  rsrc;
	OCPI::Container::PortData  rinput;          
      };
      MyConnection m_connectionCookie;

    };

    // Worker context port information
    struct OpaquePortData {
      RCCPort                     *    cp289Port;
      PortDelegator               *    port;
      OCPI::DataTransport::BufferUserFacet  *    buffer;
      bool                            readyToAdvance;
      OpaquePortData():cp289Port(NULL),port(NULL),buffer(NULL),readyToAdvance(1) {}
    };


  }

}

#endif

