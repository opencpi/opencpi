
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
 *   This file contains the implementation for the Ocpi port set.   A port set is 
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

#include <OcpiPortSet.h>
#include <OcpiPort.h>
#include <OcpiCircuit.h>
#include <OcpiTransferController.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace OCPI::OS;


 /**********************************
  * Constructors
  *********************************/
PortSet::
PortSet( PortSetMetaData* psmd, Circuit* circuit )
  :CU::Child<Circuit,PortSet>( *circuit ),
   OCPI::Time::Emit( circuit, "PortSet" ),
   m_transferController(NULL),
   m_circuit(circuit)
 {
   m_data.ports.noShuffle();
   m_data.psMetaData = psmd;
   m_data.portCount = 0;
   m_data.outputPortRank = 0;

   ocpiDebug("In PortSet::PortSet()");

   // cache our meta data
   m_output = m_data.psMetaData->output;

  // Update the port set
  update( psmd );
}

void PortSet::add( Port* port )
{
  m_data.ports.insertToPosition( port, port->getPortId() );
  m_data.portCount++;
}

/**********************************
 * Update ports that have complated meta-data
 *********************************/
void 
PortSet::
update( PortSetMetaData* psmd )
{
  // Here we will create our ports

  ocpiDebug("PortSet::update: size = %d", psmd->m_portMd.size() );

  for ( unsigned int n=0; n<psmd->m_portMd.size(); n++ ) {

    PortMetaData* pmd = static_cast<PortMetaData*>(psmd->m_portMd[n]);
    OCPI::DataTransport::Port* port = static_cast<OCPI::DataTransport::Port*>(this->getPortFromOrdinal( pmd->id ));
    if ( !port ) {
      port = new OCPI::DataTransport::Port( pmd, this );
      this->add( port );
    }
  }
}


/**********************************
 * Set the controller
 *********************************/
void 
PortSet::
setTxController ( TransferController* t )
{
  m_transferController=t;
}



PortSet::
PortSetData::
~PortSetData()
{
  // Empty
}

Port* 
PortSet::
getPortFromIndex( OCPI::OS::int32_t idx )
{
  int f=0;
  for ( OCPI::OS::uint32_t n=0; n<m_data.ports.size(); n++) {
    if (m_data.ports[n] ) {
      if ( f == idx ) {
        return static_cast<Port*>(m_data.ports[n]);
      }
      f++;
    }
  }
  return NULL;
}


/**********************************
 * Get Ocpi port
 *********************************/
OCPI::DataTransport::Port* PortSet::getPort( OCPI::OS::uint32_t idx )
{
  OCPI::OS::uint32_t f=0;
  for ( OCPI::OS::uint32_t n=0; n<m_data.ports.size(); n++) {
    if (m_data.ports[n] ) {
      if ( f == idx ) {
        return static_cast<OCPI::DataTransport::Port*>(m_data.ports[n]);
      }
      f++;
    }
  }
  return NULL;
}


Buffer* 
PortSet::
pullData( Buffer* buffer )
{ 
  ( void ) buffer;
  Port *p = getPortFromIndex(0);
  ocpiAssert( p->getMetaData()->m_shadowPortDescriptor.role == OCPI::RDT::ActiveFlowControl );

  // We got notified because the output port indicated that it has data available on this
  // buffer, so Q the transfer
  Buffer * b = p->getNextEmptyOutputBuffer();
  b->send();
  return b;
}


PortSet::
~PortSet()
{
  for ( int n =0; n< m_data.portCount; n++ ) {
    Port *p = getPortFromIndex(n);
    delete p;
  }
  m_data.ports.destroyList();

  if ( !m_output &&  m_transferController ) {
       delete m_transferController;
  }

}

