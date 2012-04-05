
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
 *   This file contains the Interface for the Ocpi port set.   A port set is 
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

#ifndef OCPI_DataTransport_PortSet_H_
#define OCPI_DataTransport_PortSet_H_

#include <OcpiTransportConstants.h>
#include <OcpiPort.h>
#include <OcpiPortSetMetaData.h>
#include <OcpiParentChild.h>
#include <OcpiTimeEmit.h>

namespace  OCPI {
  namespace DataTransport {
    class TransferController;
  }
}

namespace OCPI {

  namespace DataTransport {

    // Forward references
    class Circuit;
    class Port;
    class Buffer;

    class PortSet : public OCPI::Util::Child<Circuit,PortSet>, public OCPI::Util::Parent<Port>,
      public OCPI::Time::Emit
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
        OCPI::OS::int32_t portCount;

        // Sparse list of ports, index-able via the port ordinal
        OCPI::Util::VList ports;

        // Output port
        OCPI::OS::uint32_t outputPortRank;

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
      Port* getPort( OCPI::OS::uint32_t idx );
      inline Port* getPortFromOrdinal( PortOrdinal id )
        {
          if ( (OCPI::OS::uint32_t)id >= m_data.ports.size() ) {
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
      OCPI::OS::uint32_t getPortCount();

      /**********************************
       * Get the port set meta-data
       *********************************/
      PortSetMetaData* getPsMetaData() {return m_data.psMetaData;}

      /**********************************
       * Get the port 
       *********************************/
      Port* getPortFromIndex( OCPI::OS::int32_t idx );

      /**********************************
       * Get the number of buffers
       *********************************/
      OCPI::OS::uint32_t getBufferCount();

      /**********************************
       * Get buffer length
       *********************************/
      OCPI::OS::uint32_t getBufferLength();

      /**********************************
       * Get the port set id
       *********************************/
      OCPI::OS::int32_t getPortSetId();

      /**********************************
       * Is this a output port set ?
       *********************************/
      bool isOutput();
      OCPI::OS::uint32_t &outputPortRank();

      /**********************************
       * Add a port to the set
       *********************************/
      void add( Port* port );

      /**********************************
       * Get our parent circuit
       *********************************/
      OCPI::DataTransport::Circuit* getCircuit();

      /**********************************
       * Get the data distribution object for this set
       *********************************/
      OCPI::DataTransport::DataDistribution* getDataDistribution();


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
      OCPI::DataTransport::Circuit* m_circuit;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline TransferController* PortSet::getTxController(){return m_transferController;}
    inline OCPI::OS::uint32_t PortSet::getPortCount(){return m_data.portCount;}
    inline OCPI::OS::uint32_t PortSet::getBufferCount(){return m_data.psMetaData->bufferCount;}
    inline OCPI::OS::uint32_t PortSet::getBufferLength(){return m_data.psMetaData->bufferLength;}
    inline bool PortSet::isOutput(){return m_data.psMetaData->output;}
    inline OCPI::OS::uint32_t& PortSet::outputPortRank(){return m_data.outputPortRank;}
    inline OCPI::DataTransport::Circuit* PortSet::getCircuit(){ return m_circuit;}
    inline OCPI::OS::int32_t PortSet::getPortSetId(){return m_data.psMetaData->portSetId;}
    inline OCPI::DataTransport::DataDistribution* PortSet::getDataDistribution(){return m_data.psMetaData->dataDistribution;}


  }

}


#endif
