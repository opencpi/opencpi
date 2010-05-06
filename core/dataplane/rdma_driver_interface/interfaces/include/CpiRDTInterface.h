#ifndef CPIRDT_INTERFACE_H_
#define CPIRDT_INTERFACE_H_

#include <CpiOsDataTypes.h>

namespace CPI {
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
      NoRole,                 // Role is unspecified (during negotiation)
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
      MaxRole
    };
    // These options are smaller issues than port roles, and may apply across roles
    // The low order bits are used for what roles are possible for a port (during negotiation)
    enum ProtocolOptions {
      FeedbackIsCount = MaxRole, // The doorbell indicating feedback is a count of buffers rather than a constant
      MandatedRole,                 // Role is not a preference, but a mandate
      MaxOption
    };

    struct OutOfBandData {
      CPI::OS::uint64_t  port_id;     // Port Id
      char               oep[128];    // Originators endpoint
    };

    struct Desc_t {
      CPI::OS::uint32_t  nBuffers;
      CPI::OS::uint64_t  dataBufferBaseAddr;
      CPI::OS::uint32_t  dataBufferPitch;
      CPI::OS::uint32_t  dataBufferSize;
      CPI::OS::uint64_t  metaDataBaseAddr;
      CPI::OS::uint32_t  metaDataPitch;
      CPI::OS::uint64_t  fullFlagBaseAddr; 
      CPI::OS::uint32_t  fullFlagSize;
      CPI::OS::uint32_t  fullFlagPitch;
      CPI::OS::uint64_t  fullFlagValue;
      CPI::OS::uint64_t  emptyFlagBaseAddr; // when consumer is passive
      CPI::OS::uint32_t  emptyFlagSize;
      CPI::OS::uint32_t  emptyFlagPitch;
      CPI::OS::uint64_t  emptyFlagValue;

      OutOfBandData       oob;
    };

    struct Descriptors {
      CPI::OS::uint32_t  type;
      CPI::OS::int32_t   role;     // signed to suppress compiler warnings vs. enums
      CPI::OS::uint32_t  options; // bit fields based on role.
      Desc_t desc;
      Descriptors():role(ActiveMessage){}
    };

    // Debug utils
    void printDesc( Desc_t& desc );

  }

}

#endif

