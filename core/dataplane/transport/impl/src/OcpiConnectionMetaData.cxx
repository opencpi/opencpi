
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
  dataDistribution = new OCPI::DataTransport::ParallelDataDistribution();

}



ConnectionMetaData::ConnectionMetaData( OCPI::RDT::Descriptors& sPort )
  : m_portSetMd(0) 
{

  // Create the source port 
  PortSetMetaData* psmd = new PortSetMetaData(true,0,new ParallelDataDistribution(), 
                                                    sPort.desc.nBuffers, sPort.desc.dataBufferSize, this);
  m_portSetMd.push_back( psmd );
  PortMetaData* pmd = new PortMetaData(0,true,sPort,NULL,psmd);

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

