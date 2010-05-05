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
 *   This file contains the implementation for the CPI connection meta data class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <CpiRDTInterface.h>
#include <CpiParallelDataDistribution.h>
#include <CpiIntSequentialDataDistribution.h>
#include <CpiPortSetMetaData.h>
#include <CpiTransportGlobal.h>
#include <CpiTransport.h>
#include <CpiConnectionMetaData.h>
#include <stdlib.h>

using namespace CPI::DataTransport;
using namespace DataTransfer;
using namespace DtI;
using namespace CPI::OS;


// This constructor is used to create a simple point to point Whole/Par ->Par/Whole circuit
ConnectionMetaData::ConnectionMetaData( const char* source_ep, const char* target_ep, 
					      int  buf_count,
					      int  buf_len
					      )
  : m_portSetMd(0) 
{

  // Create the source port 
  PortSetMetaData* psmd = new PortSetMetaData( true,0,new ParallelDataDistribution(), 
					       buf_count, buf_len, 
					       this);
  PortMetaData* pmd;
  m_portSetMd.push_back( psmd );
  pmd = new PortMetaData(0,true,source_ep,target_ep,psmd);
  psmd->addPort(pmd);

  // Create the destination port
  psmd = new PortSetMetaData(   false,1,new ParallelDataDistribution(), 
				buf_count, buf_len,
				this);

  m_portSetMd.push_back( psmd );
  pmd = new PortMetaData( 1,false,target_ep,source_ep,psmd);
  psmd->addPort(pmd);

  // Our connection distribution
  dataDistribution = new CPI::DataTransport::ParallelDataDistribution();

}



ConnectionMetaData::ConnectionMetaData( CPI::RDT::Descriptors& sPort )
  : m_portSetMd(0) 
{

  // Create the source port 
  PortSetMetaData* psmd = new PortSetMetaData(true,0,new ParallelDataDistribution(), 
						    sPort.desc.nBuffers, sPort.desc.dataBufferSize, this);
  m_portSetMd.push_back( psmd );
  PortMetaData* pmd = new PortMetaData(0,true,sPort,NULL,psmd);

  psmd->addPort(pmd);

  // Our connection distribution
  dataDistribution = new CPI::DataTransport::ParallelDataDistribution();

}


/**********************************
 * Add a target port set
 *********************************/
void ConnectionMetaData::addPortSet( PortSetMetaData* psmd )
{
  m_portSetMd.push_back(psmd);

}
     

/**********************************
 * Destructor
 *********************************/
ConnectionMetaData::~ConnectionMetaData()
{
  delete dataDistribution;
}

