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
 *   This file contains the Interface for the partition class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport__DataPartition_H_
#define CPI_DataTransport__DataPartition_H_

#include <CpiOsDataTypes.h>
#include <DtOsDataTypes.h>

namespace CPI {

  namespace DataTransport {

    // Forward references
    class Port;
    class PortSet;
    class Buffer;

    /**********************************
     *  Data Partition meta data structure
     **********************************/
    struct DataPartitionMetaData {

      /**********************************
       *  Default constructor
       **********************************/
      DataPartitionMetaData();

      /**********************************
       *  Partition type
       **********************************/
      enum PartitionType {
        INDIVISIBLE,
        BLOCK
      };

      /**********************************
       *  Scalar types
       **********************************/
      enum ScalarType {
        CHAR,
        UCHAR,
        SHORT,
        USHORT,
        INTEGER,
        UINTEGER,
        LONG,
        ULONG,
        FLOAT,
        DOUBLE
      };

      // minimum allignment
      CPI::OS::uint32_t minAllignment;

      // minimum partion size
      CPI::OS::uint32_t minSize;

      // maximum partition size
      CPI::OS::uint32_t maxSize;

      // Modulo size
      CPI::OS::uint32_t moduloSize;

      // element count
      CPI::OS::uint32_t elementCount;

      // number of dimensions
      CPI::OS::uint32_t numberOfDims;

      // Scalars per element
      CPI::OS::uint32_t scalarsPerElem;

      // Scalar type
      enum ScalarType scalarType;

      // Partition type
      enum PartitionType dataPartType;

      // Block size
      CPI::OS::uint32_t blockSize;

    };


    /**********************************
     *  Data Partition class definition
     **********************************/
    class DataPartition
    {

    public:

      /**********************************
       *  Constructors
       **********************************/
      DataPartition();  // Default is whole distribution
      DataPartition( DataPartitionMetaData* md );

      /**********************************
       *  Destructor
       **********************************/
      virtual ~DataPartition();

      /**********************************
       * Given the inherit distribution and partition information
       * calculate the offsets into the requested buffers for distribution.
       *
       * returns 0 on success.
       *
       * Note: It is the responsibility of the caller to "delete" the allocated 
       * BufferInfo structure.
       **********************************/
      struct BufferInfo {
        DtOsDataTypes::Offset output_offset;
        DtOsDataTypes::Offset input_offset;
        CPI::OS::uint32_t length;
        BufferInfo* next;
        BufferInfo* last;

        // Constructors/Destructors
        BufferInfo();
        ~BufferInfo();

        //Add another structure
        void add( BufferInfo* bi );
      };

      virtual CPI::OS::int32_t calculateBufferOffsets( 
                                                      CPI::OS::uint32_t                   sequence,        // In - Transfer sequence
                                                      Buffer     *src_buf,                            // In - Output buffer
                                                      Buffer     *input_buf,                        // In - Input buffer
                                                      BufferInfo **input_buf_info);      // Out - Input buffer offset information

      /**********************************
       * Get the total number of individual transfers that are needed to transfer
       * all of the parts from the output to the inputs.
       **********************************/
      CPI::OS::uint32_t getTransferCount(        
                                         PortSet* src_ps,                                        // In - Output port set
                                         PortSet* input_ps );                                // In - Input port set

      /**********************************
       * Get the total number of parts that make up the whole for this distribution.
       **********************************/
      CPI::OS::uint32_t getPartsCount(        
                                      PortSet* src_ps,                                    // In - Output port set
                                      PortSet* input_ps );                            // In - Input port set

      /**********************************
       * Given the parts sequence, calculate the offset into output buffer
       **********************************/
      CPI::OS::uint32_t getOutputOffset(        
                                        PortSet* src_ps,                                     // In - Output port set
                                        PortSet* input_ps,                                // In - Input port set
                                        CPI::OS::uint32_t sequence );                        // In - The sequence within the set

      /**********************************
       * Get our data structure
       **********************************/
      DataPartitionMetaData* getData();


    protected:

      // Our data partition meta data
      DataPartitionMetaData* m_data;

      /**********************************
       * method for calculating whole to parts transfers
       **********************************/
      void calculateWholeToParts( 
                                 CPI::OS::uint32_t        sequence,                        // In - Transfer sequence
                                 Buffer          *src_buf,                        // In - Output buffer
                                 Buffer          *input_buf,                // In - Input buffer
                                 BufferInfo      *buf_info);                        // InOut - Buffer info

      /**********************************
       * method for calculating parts to whole transfers
       **********************************/
      void calculatePartsToWhole( 
                                 CPI::OS::uint32_t                sequence,        // In - Transfer sequence
                                 Buffer   *src_buf,                            // In - Output buffer
                                 Buffer   *input_buf,                    // In - Input buffer
                                 BufferInfo *buf_info);                        // InOut - Buffer info

      /**********************************
       * method for calculating parts to parts transfers
       **********************************/
      void calculatePartsToParts( 
                                 CPI::OS::uint32_t        sequence,            // In - Transfer sequence
                                 Buffer   *src_buf,                            // In - Output buffer
                                 Buffer   *input_buf,                    // In - Input buffer
                                 BufferInfo *buf_info);                        // InOut - Buffer info

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline DataPartitionMetaData* DataPartition::getData(){return m_data;}

  }
}


#endif


