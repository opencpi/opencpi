
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
 *   This file contains the implementation for the partition class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#include <OcpiIntDataDistribution.h>
#include <OcpiIntDataPartition.h>
#include <OcpiBuffer.h>
#include <OcpiPort.h>
#include <OcpiPortSet.h>

using namespace OCPI::DataTransport;


// Constructors/Destructors
DataPartition::BufferInfo::BufferInfo()
{
  output_offset = 0;
  input_offset = 0;
  length = 0;
  next=last=0;
}
DataPartition::BufferInfo::~BufferInfo()
{
  BufferInfo* info = next;
  while( info ) {
    BufferInfo* del = info;
    info = info->next;
    delete del;
  }
}

//Add another structure
void DataPartition::BufferInfo::add( BufferInfo* bi )
{
  if ( ! next ) {
    next = last = bi;
  }
  else {
    last->next = bi;
    last = bi;
  }
}

// Constructors
DataPartitionMetaData::DataPartitionMetaData()
{
  minAllignment = 8;
  // minimum partion size
  minSize = 0;

  // maximum size
  maxSize = 1024 * 1024;

  // element count
  elementCount = 2048;

  // number of dimensions
  numberOfDims = 1;

  // Scalars per element
  scalarsPerElem = 1;

  // Scalar type
  scalarType = INTEGER;

  // Partition type
  dataPartType = INDIVISIBLE;

  // Modulo size
  moduloSize = 1;

  // Block size
  blockSize = maxSize;

}



DataPartition::DataPartition()
{
  m_data = new DataPartitionMetaData();
}

DataPartition::DataPartition( DataPartitionMetaData* md )
  :m_data(md){}


/**********************************
 * Get the total number of parts that make up the whole for this distribution.
 **********************************/
OCPI::OS::uint32_t DataPartition::getPartsCount(        
                                               PortSet* src_ps, 
                                               PortSet* input_ps )
{
  OCPI::OS::uint32_t  parts_count = 1;
        
  // Get the output info
  DataPartition*  src_part = src_ps->getDataDistribution()->getDataPartition();
  OCPI::OS::uint32_t buf_len = src_ps->getPortFromIndex(0)->getBuffer(0)->getLength();
        
  // Get input info
  DataPartition*  input_part = input_ps->getDataDistribution()->getDataPartition();
        
  if ( src_part->getData()->dataPartType ==  DataPartitionMetaData::INDIVISIBLE ) {
    if ( input_part->getData()->dataPartType ==  DataPartitionMetaData::BLOCK ) {
                        
      parts_count = buf_len / m_data->maxSize;
      parts_count += (buf_len%m_data->maxSize) ? 1 : 0;
                        
    }
  }

  return parts_count;
}


/**********************************
 * Get the total number of individual transfers that are needed.
 **********************************/
OCPI::OS::uint32_t DataPartition::getTransferCount( PortSet* src_ps, PortSet* input_ps )                        
{
  OCPI::OS::uint32_t  trans_count = 1;

  // Get the output info
  DataPartition*  src_part = src_ps->getDataDistribution()->getDataPartition();

  // Get input info
  DataPartition*  input_part = input_ps->getDataDistribution()->getDataPartition();

  if ( src_part->getData()->dataPartType ==  DataPartitionMetaData::INDIVISIBLE ) {
    if ( input_part->getData()->dataPartType ==  DataPartitionMetaData::BLOCK ) {
                        
      OCPI::OS::uint32_t parts_count = getPartsCount( src_ps, input_ps );
      trans_count = (parts_count + (parts_count%input_ps->getPortCount())) / input_ps->getPortCount();

    }
  }

  return trans_count;
}



/**********************************
 * Given the parts sequence, calculate the offset into output buffer
 **********************************/
OCPI::OS::uint32_t DataPartition::getOutputOffset(        
                                                 PortSet* src_ps, 
                                                 PortSet* input_ps,
                                                 OCPI::OS::uint32_t sequence )
{
  OCPI::OS::uint32_t parts_count = getPartsCount( src_ps, input_ps );
  return (src_ps->getBufferLength() / parts_count) * sequence;
}
                




void DataPartition::calculateWholeToParts( 
                                          OCPI::OS::uint32_t        sequence,                        // In - Transfer sequence
                                          Buffer   *src_buf,                            // In - Output buffer
                                          Buffer   *input_buf,                    // In - Input buffer
                                          BufferInfo *buf_info)                        // InOut - Buffer info
{
  int input_port_count = input_buf->getPort()->getPortSet()->getPortCount();
  int input_rank       = input_buf->getPort()->getRank();

  // Calculate the offset into the output buffer
  buf_info->output_offset    = (input_port_count*sequence + input_rank) * m_data->maxSize ;

  // We may not need a transfer 
  if ( buf_info->output_offset >= src_buf->getLength() ) {
    buf_info->output_offset = 0;
    buf_info->length = 0;
  }
  else if ( (buf_info->output_offset+m_data->maxSize) > src_buf->getLength() ) {
    //                buf_info->length = (buf_info->output_offset+m_data->maxSize) - src_buf->getLength();
    buf_info->length = src_buf->getLength() % m_data->maxSize;
  }
  else {
    buf_info->length = m_data->maxSize;
  }
}






void DataPartition::calculatePartsToWhole( 
                                          OCPI::OS::uint32_t            ,                        
                                          Buffer     *,                            
                                          Buffer     *,                    
                                          BufferInfo *)                        
{
        
        
}


void DataPartition::calculatePartsToParts( 
                                          OCPI::OS::uint32_t           ,                        
                                          Buffer   *,                            
                                          Buffer   *,                   
                                          BufferInfo *)                        
{

        
        
}



OCPI::OS::int32_t DataPartition::calculateBufferOffsets( 
                                                       OCPI::OS::uint32_t                    sequence,                                // In - Transfer sequence
                                                       Buffer   *src_buf,                            // In - Output buffer
                                                       Buffer   *input_buf,                    // In - Input buffer
                                                       BufferInfo **buf_info)     // Out - Input buffer offset information
{



  // Get the output info
  OCPI::DataTransport::Port*           src_port = static_cast<OCPI::DataTransport::Port*>(src_buf->getPort());
  OCPI::DataTransport::PortSet*        src_ps   = src_port->getPortSet();
  DataPartition*  src_part = src_ps->getDataDistribution()->getDataPartition();

  // Get input info
  OCPI::DataTransport::Port*         input_port = static_cast<OCPI::DataTransport::Port*>(input_buf->getPort());
  OCPI::DataTransport::PortSet*   input_ps   = input_port->getPortSet();
  DataPartition*  input_part = input_ps->getDataDistribution()->getDataPartition();


  // Create a new buffer info structure
  BufferInfo *input_buf_info = new BufferInfo();
  *buf_info = input_buf_info;


  // Switch on the input partition type
  switch ( src_part->getData()->dataPartType ) {
                
  case DataPartitionMetaData::INDIVISIBLE:
    {

      // Now switch on the output partition type
      switch ( input_part->getData()->dataPartType ) {
                                
                                
        // In this case, there are no parts
      case DataPartitionMetaData::INDIVISIBLE:
        {
          input_buf_info->output_offset = 0;
          input_buf_info->input_offset = 0;
          input_buf_info->length = src_buf->getLength();
        }
        break;
                                
                                
        // Output is whole, input is parts
      case DataPartitionMetaData::BLOCK:
        {
          calculateWholeToParts( sequence, src_buf, input_buf, input_buf_info);
        }
        break;        
      }
                        
    }
    break;
                
                
  case DataPartitionMetaData::BLOCK:
    {
                        
      // Now switch on the output partition type
      switch ( input_part->getData()->dataPartType ) {
                                
        // Output is parts, input is whole
      case DataPartitionMetaData::INDIVISIBLE:
        {
          calculatePartsToWhole( sequence, src_buf, input_buf, input_buf_info);
        }
        break;
                                
                                
        // Output is parts, input is parts
      case DataPartitionMetaData::BLOCK:
        {
          calculatePartsToParts( sequence, src_buf, input_buf, input_buf_info);
        }
        break;
      }
      break;
                        
    }
                

  }


  return 0;
}




// Destructor
DataPartition::~DataPartition()
{
  delete m_data;
}




