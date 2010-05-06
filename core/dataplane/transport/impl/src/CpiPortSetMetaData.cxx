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
 *   This file contains the implementation for the Cpi port meta data class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 */

#include <CpiOsAssert.h>
#include <CpiPortSetMetaData.h>
#include <CpiPortMetaData.h>
#include <CpiIntDataDistribution.h>
#include <CpiConnectionMetaData.h>

using namespace CPI::DataTransport;
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
                 CPI::OS::uint32_t ps_id, 
                 DataDistribution* dd, 
                 ConnectionMetaData* c,
                 CPI::RDT::Descriptors* port_dep, 
                 CPI::OS::uint32_t cid, 
                 int port_count, 
                 int buffer_count,
                 int buffer_size,
                 const char* our_ep  )
  :CU::Child<ConnectionMetaData,PortSetMetaData>( *c ),
   m_connection(c)
{
  CPI::OS::int32_t n;
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
    PortMetaData* pmd =  new PortMetaData( n+1, false, our_ep ,NULL, port_dep[n], cid, this );
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
  :CU::Child<ConnectionMetaData,PortSetMetaData>( *c ),
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





                




