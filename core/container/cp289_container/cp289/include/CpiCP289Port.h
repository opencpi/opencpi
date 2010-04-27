// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

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


#ifndef CPI_CP289_PORT_H_
#define CPI_CP289_PORT_H_

#include <CpiOsDataTypes.h>
#include <CpiContainerDataTypes.h>
#include <CpiContainerPort.h>
#include <CpiCP289Application.h>
#include <RCC_Worker.h>

namespace DataTransport {
  namespace Interface {
    class Circuit;
  }
}

namespace DtI = DataTransport::Interface;

namespace CPI {

  namespace DataTransport {
    class Buffer;
    class Port;
    class Circuit;
  }


  namespace CP289 {

    class Application;
    struct OpaquePortData;
    class ExternalBuffer;

    class Port : public CPI::Container::Port, public CPI::Util::Parent<CPI::Container::ExternalPort> {
    public:


      Port(CPI::Container::Worker& w, PortData& initialPortData,
	   CPI::Container::PortId pid );
      virtual ~Port();

      void disconnect()
	throw ( CPI::Util::EmbeddedException );

	// Connection routines
	const std::string& getInitialProviderInfo(CPI::Util::PValue*);
	const std::string& setInitialProviderInfo(CPI::Util::PValue*, const std::string&);
	const std::string& setInitialUserInfo(const std::string&);
	const std::string& setFinalProviderInfo(const std::string&);
	void setFinalUserInfo(const std::string&);



#ifdef PORT_COMPLETE
	unsigned int read(uint8_t*, CPI::Container::Port::Metadata*, unsigned int, unsigned int);
	unsigned int write(uint8_t*, CPI::Container::Port::Metadata*, unsigned int, unsigned int);
#endif


	void checkConnectParams(){}
	void connectInside(CPI::Container::Port & other, CPI::Util::PValue * my_props,  CPI::Util::PValue * other_props);
	void finishConnection(CPI::RDT::Descriptors&){}
	CPI::Container::ExternalPort& connectExternal(const char*, CPI::Util::PValue*, CPI::Util::PValue*);

	// Member access methods
	inline CPI::DataTransport::Port *         &dtPort(){return m_dtPort;}
	inline CPI::Container::PortId&            portId(){return m_portId;}
	inline CPI::DataTransport::Circuit *      &circuit(){return m_circuit;}
	inline OpaquePortData *                   &opaque(){return m_opaque;}
	

    private:

	void
	setOutputFlowControl( CPI::Container::PortData * srcPort )
	  throw ( CPI::Util::EmbeddedException );


	void connectInputPort( PortData *    inputPort,    
			       std::string&  lPort,
			       CPI::Util::PValue*       props
			       )
	  throw ( CPI::Util::EmbeddedException );


	void initInputPort();
	void initOutputPort();
	void processPortProperties( CPI::Util::PValue* props );
	void disconnect( CPI::Container::PortData* sp, CPI::Container::PortData* input );


	/**********************************
	 * This method is used to connect an external port decribed in the
	 * the PortDependencyData structure to a output port for the given worker.
	 *********************************/
	CPI::Container::PortConnectionDesc 
	  connectExternalInputPort( 
				    CPI::Container::PortData *     inputPort,
				    CPI::Util::PValue*       props=NULL
				    );

	/**********************************
	 * This method is used to connect the external ports decribed in the
	 * the PortDependencyData structure to output port for the given worker.
	 *********************************/
	void connectInternalInputPort( 
				       CPI::Container::Port *          inputPort,  
				       CPI::Util::PValue*       properties    
				       );


	// Data transport port reference
	CPI::DataTransport::Port   *    m_dtPort;

	// Port id that is used by the caller, it is an ordinal that is less than the maximum
	// number of ports in a worker
	CPI::Container::PortId  m_portId;

	// Our circuit
	CPI::DataTransport::Circuit * m_circuit;   

	// Port properties
	CPI::Util::PValue*  m_props;	       

	// Opaque port data cross link
	OpaquePortData * m_opaque;

	// local shadow port descriptor (string form)
	std::string m_localShadowPort;

	// our descriptor (string form)
	std::string m_ourFinalDescriptor;

	// Our connection cookie
	struct MyConnection {
	  MyConnection():connected(false),nosrcd(false){}
	  void init( PortData * src, bool slocal,  PortData * input, bool tlocal);
	  bool connected;
	  CPI::CP289::Port  *   lsrc;
	  bool                  nosrcd;
	  CPI::CP289::Port  *   linput;
	  CPI::Container::PortData  rsrc;
	  CPI::Container::PortData  rinput;	  
	};
	MyConnection m_connectionCookie;

    };

    // Worker context port information
    struct OpaquePortData {
      RCCPort                    *    cp289Port;
      CPI::CP289::Port           *    port;
      CPI::DataTransport::Buffer *    buffer;
      bool                            readyToAdvance;
      Worker                     *    worker;
      OpaquePortData():cp289Port(NULL),port(NULL),buffer(NULL),readyToAdvance(1),worker(0){}
    };


  }

}

#endif

