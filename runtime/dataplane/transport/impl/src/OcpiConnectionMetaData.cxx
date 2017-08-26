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
 * Abstract:
 *   This file contains the implementation for the OCPI connection meta data class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <OcpiRDTInterface.h>
#include <OcpiParallelDataDistribution.h>
#include <OcpiIntSequentialDataDistribution.h>
#include <OcpiPortSetMetaData.h>
#include <OcpiTransportGlobal.h>
#include <OcpiTransport.h>
#include <OcpiConnectionMetaData.h>
#include <stdlib.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace DtI;
using namespace OCPI::OS;


// This constructor is used to create a simple point to point Whole/Par ->Par/Whole circuit
ConnectionMetaData::ConnectionMetaData( EndPoint *source_ep, EndPoint* target_ep, 
					int  buf_count,
					int  buf_len)
  : m_portSetMd(0) 
{

  // Create the source port 
  PortSetMetaData* psmd = new PortSetMetaData( true,0,new ParallelDataDistribution(), 
                                               buf_count, buf_len, 
                                               this);
  m_portSetMd.push_back( psmd );
  PortMetaData* pmd = new PortMetaData(0, true, source_ep, target_ep, psmd);
  psmd->addPort(pmd);

  // Our connection distribution
  dataDistribution = new OCPI::DataTransport::ParallelDataDistribution();

  // Create the destination port
  psmd = new PortSetMetaData(   false,1,new ParallelDataDistribution(), 
                                buf_count, buf_len,
                                this);

  m_portSetMd.push_back( psmd );
  pmd = new PortMetaData( 1,false,target_ep,source_ep,psmd);
  psmd->addPort(pmd);


}


// Output-only
ConnectionMetaData::ConnectionMetaData( DataTransfer::EndPoint &outputEp,
					OCPI::RDT::Descriptors& outputDesc)
  : m_portSetMd(0) 
{

  // Create the output port 
  PortSetMetaData* psmd = new PortSetMetaData(true,0,new ParallelDataDistribution(), 
					      outputDesc.desc.nBuffers, outputDesc.desc.dataBufferSize, this);
  m_portSetMd.push_back( psmd );
  PortMetaData* pmd = new PortMetaData(0,true, &outputEp, outputDesc, psmd);
  psmd->addPort(pmd);

  // Our connection distribution
  dataDistribution = new OCPI::DataTransport::ParallelDataDistribution();

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

