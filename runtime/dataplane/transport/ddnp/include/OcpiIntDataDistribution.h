
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
 * Abstract:
 *   This file contains the Interface for the data distribution class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 */

#ifndef DataTransport_Interface_DataDistribution_H_
#define DataTransport_Interface_DataDistribution_H_

#include <OcpiIntDataPartition.h>


namespace OCPI {

  namespace DataTransport {

    // Forward references
    class Port;
    class Buffer;
    class Circuit;

    struct DataDistributionMetaData 
    {

      /**********************************
       *  Distribution type
       **********************************/
      enum DistributionType {
        parallel,
        sequential
      };

      /**********************************
       *  Distribution sub-type
       **********************************/
      enum DistributionSubType {
        round_robin,
        random_even,
        random_statistical,
        first_available,
        least_busy
      };

      /**********************************
       *  Constructors
       **********************************/
      DataDistributionMetaData( DataPartition* part );

      // Default distribution parallel/whole
      DataDistributionMetaData();

      /**********************************
       *  Destructor
       **********************************/
      virtual ~DataDistributionMetaData();

      // Distribution type
      enum DistributionType distType;

      // Distribution sub-type
      enum DistributionSubType distSubType;

      // Our partition object
      DataPartition* partition;

    };


    /**********************************
     * This is the data distribution interface whose
     * implementation is responsible for performing the
     * setup and perhaps the actual distribution of the
     * data.
     **********************************/
    class DataDistribution
    {

    public:

      /**********************************
       *  Constructors
       **********************************/
      DataDistribution( DataDistributionMetaData* meta_data, OCPI::DataTransport::Circuit* circuit  );

      // Used for test
      DataDistribution();

      /**********************************
       *  Destructor
       **********************************/
      virtual ~DataDistribution();

      /**********************************
       *  Get the data partition info
       **********************************/
      void setDataPartition( DataPartition* part );
      DataPartition* getDataPartition();

      /**********************************
       *  Get the meta-data class
       **********************************/
      DataDistributionMetaData* getMetaData();

    protected:

      // Data partition meta data
      DataDistributionMetaData* m_metaData;

      // Our circuit, we need this since we need to get the other Data distribution
      // information from it
      OCPI::DataTransport::Circuit* m_circuit;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline void DataDistribution::setDataPartition( DataPartition* part ){m_metaData->partition=part;}
    inline DataPartition* DataDistribution::getDataPartition(){return m_metaData->partition;}
    inline DataDistributionMetaData* DataDistribution::getMetaData(){return m_metaData;}

  }

}


#endif


