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
 *   This file contains the Ocpi implementation for parallel data distribution
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_ParallelDataDistribution_H_
#define OCPI_DataTransport_ParallelDataDistribution_H_

#include <OcpiIntParallelDataDistribution.h>
#include <OcpiTransferTemplate.h> 

namespace OCPI {

  namespace DataTransport {

    class OcpiParallelDataDistribution : public ParallelDataDistribution
    {

    public:

      /**********************************
       * Constructors
       **********************************/
      OcpiParallelDataDistribution( DataDistributionMetaData* data, Circuit* circuit );
      OcpiParallelDataDistribution( DataDistributionMetaData* data );

      /**********************************
       *  Inititialize any transfer data that is required by the 
       *  implementation.
       **********************************/
      virtual void initTransfers();

        
      /**********************************
       * Creates or retreives an existing transfer handle. Based upon our rank and
       * the distribution type, this template will be created with the proper offset(s) into
       * the source, offsets into the target(s) and appropriate control structures.
       **********************************/
      virtual TransferTemplate* getTxTemplate( OCPI::DataTransport::Buffer* src );

    private:

    };
  }

}


#endif


