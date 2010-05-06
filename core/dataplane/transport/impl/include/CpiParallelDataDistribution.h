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
 *   This file contains the Cpi implementation for parallel data distribution
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_ParallelDataDistribution_H_
#define CPI_DataTransport_ParallelDataDistribution_H_

#include <CpiIntParallelDataDistribution.h>
#include <CpiTransferTemplate.h> 

namespace CPI {

  namespace DataTransport {

    class CpiParallelDataDistribution : public ParallelDataDistribution
    {

    public:

      /**********************************
       * Constructors
       **********************************/
      CpiParallelDataDistribution( DataDistributionMetaData* data, Circuit* circuit );
      CpiParallelDataDistribution( DataDistributionMetaData* data );

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
      virtual TransferTemplate* getTxTemplate( CPI::DataTransport::Buffer* src );

    private:

    };
  }

}


#endif


