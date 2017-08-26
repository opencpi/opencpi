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
 *   This file contains the interface for sequential data distribution
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef DataTransport_Interface_SequentialDataDistribution_H_
#define DataTransport_Interface_SequentialDataDistribution_H_

#include <OcpiIntDataPartition.h>
#include <OcpiIntDataDistribution.h>

namespace OCPI {

  namespace DataTransport {


    class SequentialDataDistribution : public DataDistribution
    {

    public:

      /**********************************
       *  Constructors
       **********************************/
      SequentialDataDistribution( 
                                 DataDistributionMetaData* data, 
                                 OCPI::DataTransport::Circuit* circuit
                                 );

      SequentialDataDistribution( DataDistributionMetaData::DistributionSubType sub_type );

    private:

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
                
  }

}


#endif


