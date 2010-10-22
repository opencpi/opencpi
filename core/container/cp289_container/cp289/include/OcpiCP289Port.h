
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


#ifndef OCPI_CP289_PORT_H_
#define OCPI_CP289_PORT_H_

#include <OcpiOsDataTypes.h>
#include <OcpiContainerDataTypes.h>
#include <OcpiContainerPort.h>
#include <OcpiCP289Application.h>
#include <RCC_Worker.h>

namespace DataTransport {
  namespace Interface {
    class Circuit;
  }
}

namespace DtI = DataTransport::Interface;

namespace OCPI {

  namespace DataTransport {
    class Buffer;
    class Port;
    class Circuit;
  }


  namespace CP289 {

    class Application;
    struct OpaquePortData;
    class ExternalBuffer;

    class Port : public OCPI::Container::Port, public OCPI::Util::Parent<OCPI::Container::ExternalPort> {
    public:

      Port(OCPI::Container::Worker& w, OCPI::Metadata::Port & pmd, const char * endpoint );
      virtual ~Port();

      void disconnect()
        throw ( OCPI::Util::EmbeddedException );

        // Connection routines
        const std::string& getInitialProviderInfo(OCPI::Util::PValue*);
        const std::string& setInitialProviderInfo(OCPI::Util::PValue*, const std::string&);
        const std::string& setInitialUserInfo(const std::string&);
        const std::string& setFinalProviderInfo(const std::string&);
        void setFinalUserInfo(const std::string&);

        void checkConnectParams(){}
        void connectInside(OCPI::Container::Port & other, OCPI::Util::PValue * my_props,  OCPI::Util::PValue * other_props);
        void finishConnection(OCPI::RDT::Descriptors&){}
        OCPI::Container::ExternalPort& connectExternal(const char*, OCPI::Util::PValue*, OCPI::Util::PValue*);

        // Member access methods
        inline OCPI::DataTransport::Port *         &dtPort(){return m_dtPort;}
        inline OCPI::Container::PortId             portId(){return m_portId;}
        inline OCPI::DataTransport::Circuit *      &circuit(){return m_circuit;}
        inline OpaquePortData *                   &opaque(){return m_opaque;}

	virtual void connect( OCPI::Container::Port &other, OCPI::Util::PValue *myProps=NULL, OCPI::Util::PValue *otherProps=NULL);        

    private:

        void
        setOutputFlowControl( OCPI::Container::PortData * srcPort )
          throw ( OCPI::Util::EmbeddedException );


        void connectInputPort( PortData *    inputPort,    
                               std::string&  lPort,
                               OCPI::Util::PValue*       props
                               )
          throw ( OCPI::Util::EmbeddedException );


        void initInputPort();
        void initOutputPort();
        void processPortProperties( OCPI::Util::PValue* props );
        void disconnect( OCPI::Container::PortData* sp, OCPI::Container::PortData* input );


        /**********************************
         * This method is used to connect an external port decribed in the
         * the PortDependencyData structure to a output port for the given worker.
         *********************************/
        OCPI::Container::PortConnectionDesc 
          connectExternalInputPort( 
                                    OCPI::Container::PortData *     inputPort,
                                    OCPI::Util::PValue*       props=NULL
                                    );

        /**********************************
         * This method is used to connect the external ports decribed in the
         * the PortDependencyData structure to output port for the given worker.
         *********************************/
        void connectInternalInputPort( 
                                       OCPI::Container::Port *          inputPort,  
                                       OCPI::Util::PValue*       properties    
                                       );


        // Data transport port reference
        OCPI::DataTransport::Port   *    m_dtPort;

        // Port id that is used by the caller, it is an ordinal that is less than the maximum
        // number of ports in a worker
        OCPI::Container::PortId  m_portId;

        // Our circuit
        OCPI::DataTransport::Circuit * m_circuit;   

        // Port properties
        OCPI::Util::PValue*  m_props;               

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
          OCPI::CP289::Port  *   lsrc;
          bool                  nosrcd;
          OCPI::CP289::Port  *   linput;
          OCPI::Container::PortData  rsrc;
          OCPI::Container::PortData  rinput;          
        };
        MyConnection m_connectionCookie;

    };

    // Worker context port information
    struct OpaquePortData {
      RCCPort                    *    cp289Port;
      OCPI::CP289::Port           *    port;
      OCPI::DataTransport::Buffer *    buffer;
      bool                            readyToAdvance;
      Worker                     *    worker;
      OpaquePortData():cp289Port(NULL),port(NULL),buffer(NULL),readyToAdvance(1),worker(0){}
    };


  }

}

#endif

