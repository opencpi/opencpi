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
 *   This file contains the implementation for sequential data distribution
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#include <OcpiIntSequentialDataDistribution.h>
#include <OcpiIntIndivisiblePartition.h>

using namespace OCPI::DataTransport;


/**********************************
 *  Constructors
 **********************************/
SequentialDataDistribution::SequentialDataDistribution( DataDistributionMetaData* data, 
                                                        OCPI::DataTransport::Circuit* circuit )
  : DataDistribution( data, circuit ){}


// Default is sequential/round robin
SequentialDataDistribution::SequentialDataDistribution( DataDistributionMetaData::DistributionSubType sub_type )
{

  // Distribution type
  m_metaData->distType = DataDistributionMetaData::sequential;

  // Distribution sub-type
  m_metaData->distSubType = sub_type;
        
  // Our partition object, default is whole distribution
  m_metaData->partition = new IndivisiblePartition();

}



