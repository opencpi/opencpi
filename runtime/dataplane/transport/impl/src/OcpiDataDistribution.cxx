
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
 *   This file contains the implementation for the OCPI data distribution base class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <OcpiCircuit.h>
#include <OcpiDataDistribution.h>
#include <OcpiPort.h>
#include <stdlib.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;


/**********************************
 *  Constructors
 **********************************/
OcpiDataDistribution::OcpiDataDistribution( DataDistributionMetaData* meta_data, 
                                          OCPI::DataTransport::Circuit* circuit )
  : DataDistribution( meta_data, circuit )
{
  // Empty
}
      

/**********************************
 *  Destructor
 **********************************/
OcpiDataDistribution::~OcpiDataDistribution()
{
  // Empty
}

/**********************************
 *  Inititialize any transfer data that is required by the 
 *  implementation.
 **********************************/
void OcpiDataDistribution::initSourceBuffers()
{

#ifdef DD_AND_P

  // Get the source ports dd object
  PortSet* source_portset = circuit_->GetSourcePortSet();
  PortSource* source_port = circuit_->GetOurSourcePort();
  DataDistribution* src_dd = source_portset->GetDataDistribution();

  // For every target port within each port set, we need to calculate some meta data based
  // upon the targets parameters
  for ( int n=0; n<circuit_->GetTargetPortSetCount(); n++ ) {

    // Get the port set
    PortSet* target_portset = circuit_->GetTargetPortSet(n);


    // for each port within the port set
    for ( int y=0; y<target_portset->GetPortCount(); y++ ) {

      // Get the next relevant port
      Port* target_port = target_portset->GetPort(y);
      if ( ! circuit_->IsPartOfCircuit( target_port ) ) {
        continue;
      }

      // Do for all target buffers
      for ( int target_buffer_tid=0; 
            target_buffer_tid<target_portset->GetNumBuffers(); target_buffer_tid++ ) {

        // Do for all source buffers
        for ( int source_buffer_tid=0;
              source_buffer_tid<source_portset->GetNumBuffers(); source_buffer_tid++ ) {
                                        
          // Create a new template
          OcpiTransferMetaData* tmd = new OcpiTransferMetaData();
                                        
                                        
          /* Set the source buffer's rank in the buffer group */
          tmd->buffer->rank = source_port->GetRank();
                                        
          /* Set the target rank */
          tmd->buffer->target_rank = target_port->GetRank() % target_port->max_rank;
          tmd->buffer->target_tid = (source_port->rank / target_port->max_rank) % target_port->nbuffers;
                                        
#ifndef NDEBUG
          printf("buffer 0x%08lx target_rank %d src rank %d tgt max rank %d\n",
                 tmd->buffer, tmd->buffer->target_rank, source_port->rank,
                 target_port->max_rank);
#endif
                                        
          /* Set the max rank in the buffer group */
          tmd->buffer->max_rank = source_portset->GetPortCount();
          tmd->buffer->max_buffers = source_portset->GetNumBuffers();
          tmd->buffer->max_bgindex = source_port->max_bgindex;
          tmd->buffer->target_max_rank = tgt_p->max_rank;
          tmd->buffer->target_max_buffers = target_port->max_buffers;
          tmd->buffer->target_max_bgindex = target_port->max_bgindex;
                                        
          /* Set the element size */
          tmd->buffer->element_size = source_port->element_size;
          tmd->buffer->scalar_size = source_port->scalar_size;
          tmd->buffer->nscalars = source_port->nscalars;
                                        
          /* Calculate the source partition offset */
          OCPI::OS::int32_t src_poffset = 
            (src_p->index * 2 * sizeof(Buffer_State) * src_p->nbuffers);
                                        
          /* Calculate the offset and size for the mapping */
          OCPI::OS::uint32_t offset = src_p->rstate_offset;
          OCPI::OS::uint32_t size = 2 * sizeof(Buffer_State) * src_p->nbuffers;
                                        
          void *rstate_va;
                                        
          /* Map the remote state area */
          if (rc = tmd->buffer->info->Map (offset, size, &rstate_va))
            {
              /* Set the error details */
              pp->error = 1;
              pp->mcos_return_code = rc;
                                                
              return 1;
            }
                                        
                                        



          // Each target needs the source ports rank
          //                    source_portset->SetSourceRank( source_port->GetRank() );


          // Set the transfer meta data template for this transfer
          source_port->SetTransferMetaData( source_buffer_tid, 
                                            target_port->GetPortId(), 
                                            target_buffer_tid, tmd);

        }
      }
    }
  }

#endif
}


/**********************************
 *  Inititialize any transfer data that is required by the 
 *  implementation.
 **********************************/
void OcpiDataDistribution::initTransfers()
{

}


/**********************************
 * Creates or retreives an existing transfer handle. Based upon our rank and
 * the distribution type, this template will be created with the proper offset(s) into
 * the source, offsets into the target(s) and appropriate control structures.
 **********************************/
TransferTemplate*  OcpiDataDistribution::getTxTemplate( Buffer* src )
{
  ( void ) src;
  return NULL;
}
