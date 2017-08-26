/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Abstact:
 *   This file contains the implementation for the Ocpi port meta data class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 */

#include <OcpiOsAssert.h>
#include <OcpiPortSetMetaData.h>
#include <OcpiPortMetaData.h>
#include <OcpiIntDataDistribution.h>
#include <OcpiConnectionMetaData.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;


/**********************************
 * Add a new port definition
 **********************************/
PortMetaData* 
PortSetMetaData::
addPort( PortMetaData* pmd )
{
  pmd->m_portSetMd = this;
  m_portMd.push_back( pmd );
  return pmd;
}

PortSetMetaData::
PortSetMetaData( bool src, 
                 OCPI::OS::uint32_t ps_id, 
                 DataDistribution* dd, 
                 ConnectionMetaData* c,
                 DataTransfer::EndPoint &inputEp,
                 const OCPI::RDT::Descriptors* inputDesc, 
		 //                 OCPI::OS::uint32_t cid, 
                 int port_count, 
                 int buffer_count,
                 int buffer_size,
                 DataTransfer::EndPoint &outputEp  )
  :CU::Child<ConnectionMetaData,PortSetMetaData>( *c, *this ),
   m_connection(c)
{
  OCPI::OS::int32_t n;
  std::string nuls;

  // single buffered default
  bufferCount = buffer_count;
  output = src;
  bufferLength = buffer_size;
  user_data = 0;
  portSetId = ps_id;
  dataDistribution = dd;

  // First we will create the generic ones
  for ( n=0; n<port_count; n++ ) {
    PortMetaData* pmd =  new PortMetaData( n+1, outputEp, inputEp, inputDesc[n], /*cid,*/ this );
    addPort( pmd );

  }
}

PortSetMetaData::
PortSetMetaData(    bool src, 
                    int ps_id, 
                    DataDistribution* dd, 
                    int buffer_count,
                    int buffer_size,
                    ConnectionMetaData* c )
  :CU::Child<ConnectionMetaData,PortSetMetaData>( *c, *this ),
   m_connection(c)
{

  // single buffered default
  output = src;
  bufferCount = buffer_count;
  bufferLength = buffer_size;
  user_data = 0;
  portSetId = ps_id;
  dataDistribution = dd;
}

PortSetMetaData::
~PortSetMetaData()
{
  delete  dataDistribution;
}





                




