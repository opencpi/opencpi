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
 *   This file contains the Interface for the Cpi port set.   A port set is 
 *   a group of ports that share a common set of attributes and are all part
 *   of the same circuit.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_PortSet_H_
#define CPI_DataTransport_PortSet_H_

#include <CpiTransportConstants.h>
#include <CpiPort.h>
#include <CpiPortSetMetaData.h>
#include <CpiParentChild.h>

namespace  CPI {
  namespace DataTransport {
    class TransferController;
  }
}

namespace CPI {

  namespace DataTransport {

    // Forward references
    class Circuit;
    class Port;
    class Buffer;

    class PortSet : public CPI::Util::Child<Circuit,PortSet>, public CPI::Util::Parent<Port>
    {

    public:

      // Our data structure
      struct PortSetData  {

        // Constructors
        PortSetData()
          :portCount(0),ports(1),outputPortRank(0){}

        PortSetData( PortSetMetaData* p );

        // Destructor
        virtual ~PortSetData();

        // our meta data 
        PortSetMetaData* psMetaData;

        // Number of ports
        CPI::OS::int32_t portCount;

        // Sparse list of ports, index-able via the port ordinal
        CPI::Util::VList ports;

        // Output port
        CPI::OS::uint32_t outputPortRank;

      };

      /**********************************
       * Constructors
       *********************************/
      PortSet( 
              PortSetMetaData* psmd,                // In - Port set meta-data
              Circuit* circuit );                // In - Parent circuit


      /**********************************
       * Destructor
       *********************************/
      virtual ~PortSet();

      /**********************************
       * Updates the port set with addition meta-data information
       *********************************/
      void update( PortSetMetaData* data );
      Port* getPort( CPI::OS::uint32_t idx );
      inline Port* getPortFromOrdinal( PortOrdinal id )
        {
          if ( (CPI::OS::uint32_t)id >= m_data.ports.size() ) {
            return NULL;
          }
          return static_cast<Port*>(m_data.ports[id]);
        }

      /**********************************
       * Get/Set transfer controller
       *********************************/
      TransferController* getTxController();
      void setTxController ( TransferController* t );

      /**********************************
       * Get the port count
       *********************************/
      CPI::OS::uint32_t getPortCount();

      /**********************************
       * Get the port set meta-data
       *********************************/
      PortSetMetaData* getPsMetaData() {return m_data.psMetaData;}

      /**********************************
       * Get the port 
       *********************************/
      Port* getPortFromIndex( CPI::OS::int32_t idx );

      /**********************************
       * Get the number of buffers
       *********************************/
      CPI::OS::uint32_t getBufferCount();

      /**********************************
       * Get buffer length
       *********************************/
      CPI::OS::uint32_t getBufferLength();

      /**********************************
       * Get the port set id
       *********************************/
      CPI::OS::int32_t getPortSetId();

      /**********************************
       * Is this a output port set ?
       *********************************/
      bool isOutput();
      CPI::OS::uint32_t &outputPortRank();

      /**********************************
       * Add a port to the set
       *********************************/
      void add( Port* port );

      /**********************************
       * Get our parent circuit
       *********************************/
      CPI::DataTransport::Circuit* getCircuit();

      /**********************************
       * Get the data distribution object for this set
       *********************************/
      CPI::DataTransport::DataDistribution* getDataDistribution();


      /**********************************
       * Informs the shadow port that it can now queue a pull data
       * transfer from the real port.
       *********************************/      
      Buffer* pullData( Buffer* buffer );

    protected:

      // Output port set
      bool m_output;

      // Our transfer template
      TransferController* m_transferController;

      // Our data
      PortSetData m_data;

      // Our parent circuit
      CPI::DataTransport::Circuit* m_circuit;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline TransferController* PortSet::getTxController(){return m_transferController;}
    inline CPI::OS::uint32_t PortSet::getPortCount(){return m_data.portCount;}
    inline CPI::OS::uint32_t PortSet::getBufferCount(){return m_data.psMetaData->bufferCount;}
    inline CPI::OS::uint32_t PortSet::getBufferLength(){return m_data.psMetaData->bufferLength;}
    inline bool PortSet::isOutput(){return m_data.psMetaData->output;}
    inline CPI::OS::uint32_t& PortSet::outputPortRank(){return m_data.outputPortRank;}
    inline CPI::DataTransport::Circuit* PortSet::getCircuit(){ return m_circuit;}
    inline CPI::OS::int32_t PortSet::getPortSetId(){return m_data.psMetaData->portSetId;}
    inline CPI::DataTransport::DataDistribution* PortSet::getDataDistribution(){return m_data.psMetaData->dataDistribution;}


  }

}


#endif
