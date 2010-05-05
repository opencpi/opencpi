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
 *   This file contains the implementation for the Cpi port set.   A port set is 
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

#include <CpiPortSet.h>
#include <CpiPort.h>
#include <CpiCircuit.h>
#include <CpiTransferController.h>

using namespace CPI::DataTransport;
using namespace DataTransfer;
using namespace CPI::OS;


 /**********************************
  * Constructors
  *********************************/
PortSet::
PortSet( PortSetMetaData* psmd, Circuit* circuit )
  :CU::Child<Circuit,PortSet>( *circuit ),
   m_transferController(NULL),
   m_circuit(circuit)
 {
   m_data.ports.noShuffle();
   m_data.psMetaData = psmd;
   m_data.portCount = 0;
   m_data.outputPortRank = 0;

 #ifndef NDEBUG
   printf("In PortSet::PortSet()\n");
 #endif

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

#ifndef NDEBUG
  printf("PortSet::update: size = %d\n", psmd->m_portMd.size() );
#endif

  for ( unsigned int n=0; n<psmd->m_portMd.size(); n++ ) {

    PortMetaData* pmd = static_cast<PortMetaData*>(psmd->m_portMd[n]);
    CPI::DataTransport::Port* port = static_cast<CPI::DataTransport::Port*>(this->getPortFromOrdinal( pmd->id ));
    if ( !port ) {
      port = new CPI::DataTransport::Port( pmd, this );
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
getPortFromIndex( CPI::OS::int32_t idx )
{
  int f=0;
  for ( CPI::OS::uint32_t n=0; n<m_data.ports.size(); n++) {
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
 * Get Cpi port
 *********************************/
CPI::DataTransport::Port* PortSet::getPort( CPI::OS::uint32_t idx )
{
  CPI::OS::uint32_t f=0;
  for ( CPI::OS::uint32_t n=0; n<m_data.ports.size(); n++) {
    if (m_data.ports[n] ) {
      if ( f == idx ) {
	return static_cast<CPI::DataTransport::Port*>(m_data.ports[n]);
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
  Port *p = getPortFromIndex(0);
  cpiAssert( p->getMetaData()->m_shadowPortDescriptor.role == CPI::RDT::ActiveFlowControl );

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

