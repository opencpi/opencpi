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

#ifndef OCPIRDT_INTERFACE_H_
#define OCPIRDT_INTERFACE_H_

#include <stdint.h>
#include "XferEndPoint.h"

namespace OCPI {
  namespace RDT {

    enum PortDescriptorTypes {
      ConsumerDescT = 1,
      ConsumerFlowControlDescT,
      ProducerDescT
    };

    // These roles are not supported for all protocols, but those that need it
    // specify it.  Roughly, the order is the order of "goodness" when there is
    // no other basis for choosing a role
    enum PortRole {
      ActiveMessage,     // Port will move data
                         // For a consumer, this means pulling data from the producer.
                         // For a producer, this means pushing data to the consumer.
      ActiveFlowControl, // Port will not move data, but will be active in providing feedback
                         // For a consumer, this means telling the producer when buffers
                         //   become available to fill/push-to.
                         // For a producer, this means telling the consumer when buffers
                         // become available to empty/pull-from.
      ActiveOnly,        // Port can only be active, not a target for anything
      Passive,           // Port is passive, needs other side to access all status and
                         // indicate new buffer state
      MaxRole,           // Number of valid roles
      NoRole             // Role is unspecified (during negotiation)
    };
#define OCPI_RDT_ROLE_NAMES \
  "ActiveMessage", "ActiveFlowControl", "ActiveOnly", "Passive", "MaxRole", "NoRole"    
#define OCPI_RDT_OTHER_ROLES \
    OCPI::RDT::ActiveFlowControl, OCPI::RDT::ActiveMessage, OCPI::RDT::Passive, \
    OCPI::RDT::ActiveOnly

    // These options are smaller issues than port roles, and may apply across roles
    // The low order bits are used for what roles are possible for a port (during negotiation)
    enum ProtocolOptions {
      FeedbackIsCount = MaxRole, // The doorbell a count of buffers rather than a constant
      MandatedRole,              // Role is not a preference, but a mandate
      FlagIsMeta,                // Flag is compressed metadata
      FlagIsCounting,            // Flag is an incrementing counter
      FlagIsMetaOptional,        // This mode is optional: FIXME have a more general scheme
      MaxOption
    };
    
    struct OutOfBandData {
      uint64_t               port_id;     // Port Id
      char                   oep[256];    // Originators endpoint
      uint64_t               cookie;      // Optional opaque value for endpoint connection cookie
      // These values are information common to all endpoints
      uint64_t               address;     // Base address of endpoint in its address space (usually 0)
      DtOsDataTypes::Offset  size;        // EndpointSize
      DtOsDataTypes::MailBox mailBox;     // endpoint mailbox
      DtOsDataTypes::MailBox maxCount;    // Number of mailboxes in communication domain
    };

    struct Desc_t {
      uint32_t               nBuffers;
      DtOsDataTypes::Offset  dataBufferBaseAddr; // address in endpoint
      uint32_t               dataBufferPitch;
      uint32_t               dataBufferSize;
      DtOsDataTypes::Offset  metaDataBaseAddr;
      uint32_t               metaDataPitch;
      DtOsDataTypes::Offset  fullFlagBaseAddr; 
      uint32_t               fullFlagSize;       // size to transfer, must be <= sizeof(Flag)
      uint32_t               fullFlagPitch;
      DtOsDataTypes::Flag    fullFlagValue;
      DtOsDataTypes::Offset  emptyFlagBaseAddr;  // when consumer is passive
      uint32_t               emptyFlagSize;      // size to transfer, must be <= sizeof(Flag)
      uint32_t               emptyFlagPitch;
      DtOsDataTypes::Flag    emptyFlagValue;
      OutOfBandData          oob;
    };

    struct Descriptors {
      uint32_t  type;
      uint32_t  role;    // signed to suppress compiler warnings vs. enums (NOT)
      uint32_t  options; // bit fields based on role.
      Desc_t    desc;
      Descriptors();
    };
    typedef Descriptors Descriptor;
    // Debug utils
    void printDesc( Desc_t& desc );

  }
  // FIXME:  merge these at some point
  namespace DataTransport {
    using namespace RDT;
  }
}

#endif

