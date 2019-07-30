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
 *   This file contains the OCPI template generator implementation.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 */

#include <cstddef>
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "XferEndPoint.h"
#include "OcpiPortSet.h"
#include "OcpiBuffer.h"
#include "OcpiOutputBuffer.h"
#include "OcpiInputBuffer.h"
#include "OcpiTransferController.h"
#include "OcpiIntDataDistribution.h"
#include "OcpiTemplateGenerators.h"

#define FORMAT_TRANSFER_EC_RETHROW( sep, tep )                                \
  char buf[512];                                                        \
  strcpy(buf, tep->getEndPoint().name().c_str());                        \
  strcat(buf, " -> ");                                                        \
  strcat( buf, sep->getEndPoint().name().c_str() );                        \
  throw OCPI::Util::EmbeddedException ( UNABLE_TO_CREATE_TX_REQUEST, buf );


using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace DtI;
namespace OS = OCPI::OS;
namespace DDT = DtOsDataTypes;

TransferTemplateGenerator::
TransferTemplateGenerator()
  :m_zcopyEnabled(true)
{
  // Empty
}

TransferTemplateGenerator::
~TransferTemplateGenerator()
{
  // Empty
}


// Call appropriate creator
void  
TransferTemplateGenerator::
create( Transport* transport, PortSet* output, PortSet* input, TransferController* cont )
{
  bool generated=false;

  // We need to create transfers for every output and destination port 
  // the exists in the context of this container.  
  for (PortOrdinal s_n=0; s_n<output->getPortCount(); s_n++ ) {

    Port* s_port = output->getPort(s_n);


    if ( s_port->isShadow() ) {
      continue;
    }

    ocpiDebug("s port endpoints = %s, %s, %s", 
           s_port->getRealShemServices()->endPoint().name().c_str(),
           s_port->getShadowShemServices()->endPoint().name().c_str(),           
           s_port->getLocalShemServices()->endPoint().name().c_str() );


      // Create a DD specific transfer template
      createOutputTransfers(s_port,input,cont);

      // Create a broadcast template for pair
      createOutputBroadcastTemplates(s_port,input,cont);

    generated = true;

  }

  for (PortOrdinal t_n = 0; t_n < input->getPortCount(); t_n++) {

    Port* t_port = input->getPort(t_n);

    ocpiDebug("t port endpoints = %s, %s, %s", 
	      t_port->getRealShemServices()->endPoint().name().c_str(),
	      t_port->getShadowShemServices()->endPoint().name().c_str(),           
	      t_port->getLocalShemServices()->endPoint().name().c_str() );


    // If the output port is not local, but the transfer role requires us to move data, we need to create transfers
    // for the remote port
    if (!transport->isLocalEndpoint(t_port->getRealShemServices()->endPoint()))
      break;

    // Create the input transfers
    createInputTransfers(output,t_port,cont);

    // Create the inputs broadcast transfers
    createInputBroadcastTemplates(output,t_port,cont);
    
    generated = true;

  }

  if ( ! generated ) {
    ocpiAssert("PROGRAMMING ERROR!! no templates were generated for this circuit !!\n"==0);
  }

}

// These methods are hooks to add additional transfers
bool TransferTemplateGenerator::addTransferPreData( XferRequest*, TDataInterface&)
{
  return false;
}


bool TransferTemplateGenerator::addTransferPostData( XferRequest*, TDataInterface& )
{
  return false;
}

bool TransferTemplateGenerator::addTransferPreState( XferRequest*, TDataInterface& )          
{
  return false;
}


// Create the input broadcast template for this set
void TransferTemplateGenerator::createInputBroadcastTemplates(PortSet* output, 
                                                               Port* input, 
                                                               TransferController* cont
                                                               )
{
  // We need to create the transfers to tell all of our shadow ports that a buffer
  // became available, 
  BufferOrdinal n_t_buffers = input->getBufferCount();

  // We need a transfer template to allow a transfer for each input buffer to its
  // associated shadows
  for ( BufferOrdinal t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

    // input buffer
    InputBuffer* t_buf = static_cast<InputBuffer*>(input->getBuffer(t_buffers));
    int t_tid = t_buf->getTid();

    // Create a template
    OcpiTransferTemplate* temp = new OcpiTransferTemplate(0);


    //Add the template to the controller
    ocpiDebug("*&*&* Adding template for tpid = %d, ttid = %u, template = %p", 
           input->getPortId(), t_tid, temp);

    cont->addTemplate( temp, 0, 0, input->getPortId(), t_tid, true, TransferController::INPUT );

    struct PortMetaData::InputPortBufferControlMap *input_offsets = 
      &input->getMetaData()->m_bufferData[t_tid].inputOffsets;

    // We need to setup a transfer for each shadow, they exist in the unique output circuits
    for (PortOrdinal n = 0; n < output->getPortCount(); n++) {

      // Since the shadows only exist in the circuits with instances of real
      // output ports, the offsets are indexed via the output port ordinals.
      // If the output is co-located with us, no shadow exists.
      Port* s_port = output->getPort(n);
      int s_pid = s_port->getMailbox();

      // We dont need to do anything for co-location
      if ( m_zcopyEnabled && s_port->supportsZeroCopy( input ) ) {
        temp->addZeroCopyTransfer( NULL, t_buf );

	//h
	continue;


      }

      ocpiDebug("CreateInputBroadcastTransfers: localStateOffset 0x%llx", 
             (long long)input_offsets->localStateOffset);
      ocpiDebug("CreateInputBroadcastTransfers: RemoteStateOffsets %p",
             input_offsets->myShadowsRemoteStateOffsets);
      ocpiDebug("CreateInputBroadcastTransfers: s_pid %d", s_pid);

      // Create the copy in the template
      XferRequest* ptransfer =
	input->getTemplate(input->getEndPoint(), s_port->getEndPoint()).createXferRequest();
      try {
        ptransfer->copy (
			 input_offsets->localStateOffset,
			 input_offsets->myShadowsRemoteStateOffsets[s_pid],
			 sizeof(BufferState),
			 XferRequest::DataTransfer );
      }
      catch ( ... ) {
        FORMAT_TRANSFER_EC_RETHROW( input, s_port );
      }
      
      // Add the transfer to the template
      temp->addTransfer( ptransfer );

    } // end for each input buffer
  } // end for each output port
}



void 
TransferTemplateGenerator::
createOutputBroadcastTemplates( Port* s_port, PortSet* input,
                                                               TransferController* cont )
{
  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = static_cast<OutputBuffer*>(s_port->getOutputBuffer(s_buffers));
    int s_tid = s_buf->getTid();
    int t_tid;

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // input buffer
      InputBuffer* t_buf = static_cast<InputBuffer*>(input->getPort(0)->getInputBuffer(t_buffers));
      t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplate(0);

      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant

      ocpiDebug("output port id = %d, buffer id = %d, input id = %d", 
             s_port->getPortId(), s_tid, t_tid );
      ocpiDebug("Template address = %p", temp);

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, true, TransferController::OUTPUT );

      /*
       *  This transfer is used to mark the local input shadow buffer as full
       */

      struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
        &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

      // We need to setup a transfer for each input port. 
      ocpiDebug("Number of input ports = %d", n_t_ports);
      for ( n=0; n<n_t_ports; n++ ) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = static_cast<InputBuffer*>(t_port->getBuffer(t_buffers));

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

          ocpiDebug("** ZERO COPY TransferTemplateGenerator::createOutputBroadcastTemplates from %p, to %p",
		    s_buf, t_buf);
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }
	XferRequest* ptransfer =
	  s_port->getTemplate(s_port->getEndPoint(), t_port->getEndPoint()).createXferRequest();
        try {
	  ptransfer->copy (
			   output_offsets->bufferOffset,
			   input_offsets->bufferOffset,
			   output_offsets->bufferSize,
			   XferRequest::DataTransfer );

          // Create the transfer that copys the output meta-data to the input meta-data
	  ptransfer->copy (
			   output_offsets->metaDataOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			   input_offsets->metaDataOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			   sizeof(OCPI::OS::int64_t),
			   XferRequest::MetaDataTransfer );


          // Create the transfer that copys the output state to the remote input state
          ptransfer->copy (
			   output_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			   input_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			   sizeof(BufferState),
			   XferRequest::FlagTransfer );

        }
        catch ( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }


        // Add the transfer 
        temp->addTransfer( ptransfer );

      } // end for each input buffer


      // And now to all other outputs

      // A output braodcast must also send to all other outputs to update them as well
      // Now we need to pass the output control baton onto the next output port
      XferRequest* ptransfer2=NULL;
      for (PortOrdinal ns=0; ns<s_port->getPortSet()->getPortCount(); ns++ ) {
        Port* next_sp = 
          static_cast<Port*>(s_port->getPortSet()->getPortFromIndex(ns));
        if ( next_sp == s_port ) {
          continue;
        }

        struct PortMetaData::OutputPortBufferControlMap *next_output_offsets = 
          &next_sp->getMetaData()->m_bufferData[s_tid].outputOffsets;
	ptransfer2 =
	  s_port->getTemplate(s_port->getEndPoint(), next_sp->getEndPoint()).createXferRequest();
        // Create the transfer from out output contol state to the next
        try {
          ptransfer2->copy (
			   output_offsets->portSetControlOffset,
			   next_output_offsets->portSetControlOffset,
			   sizeof(OutputPortSetControl),
			   XferRequest::FlagTransfer );
	}
	catch( ... ) {
	  FORMAT_TRANSFER_EC_RETHROW( s_port, next_sp );
	}
      }

      // Add the transfer 
      if ( ptransfer2 ) {
	temp->addTransfer( ptransfer2 );
      }

    } // end for each output buffer

  }  // end for each input port

}





// This base class provides a default pattern for the input buffers which is to 
// braodcast a input buffers availability to all shadows
void TransferTemplateGenerator::createInputTransfers(PortSet* output, Port* input,
                                                      TransferController* cont )

{
  // We need to create the transfers to tell all of our shadow ports that a buffer
  // became available, 
  size_t n_t_buffers = input->getBufferCount();

  // We need a transfer template to allow a transfer for each input buffer to its
  // associated shadows
  for ( size_t t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

    // input buffer
    InputBuffer* t_buf = input->getInputBuffer(t_buffers);
    int t_tid = t_buf->getTid();

    // Create a template
    OcpiTransferTemplate* temp = new OcpiTransferTemplate(0);

    ocpiDebug("*&*&* Adding template for tpid = %d, ttid = %d, template = %p", 
           input->getPortId(), t_tid, temp);

    //Add the template to the controller
    cont->addTemplate( temp, 0, 0, input->getPortId(), t_tid, false, TransferController::INPUT );

    struct PortMetaData::InputPortBufferControlMap *input_offsets = 
      &input->getMetaData()->m_bufferData[t_tid].inputOffsets;

    // Since there may be multiple output ports on 1 processs, we need to make sure we dont send
    // more than 1 time
    int sent[MAX_PCONTRIBS];
    memset(sent,0,sizeof(int)*MAX_PCONTRIBS);

    // We need to setup a transfer for each shadow, they exist in the unique output circuits
    for (PortOrdinal n = 0; n < output->getPortCount(); n++) {

      // Since the shadows only exist in the circuits with instances of real
      // output ports, the offsets are indexed via the output port ordinals.
      // If the output is co-located with us, no shadow exists.
      Port* s_port = output->getPort(n);
      int s_pid = s_port->getRealShemServices()->endPoint().mailBox();

      if ( sent[s_pid] ) {
        continue;
      }

      // If we are creating a template for whole transfers, we do not recognize anything
      // but output port 0
      if ( (s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel) &&
           s_port->getRank() != 0 ) {
        continue;
      }

      // Attach zero-copy for co-location
      if ( m_zcopyEnabled && s_port->supportsZeroCopy( input ) ) {
        ocpiDebug("Adding Zery copy for input response");
        temp->addZeroCopyTransfer( NULL, t_buf );
	continue;
      }
      sent[s_pid] = 1;
      XferRequest  *ptransfer =
	input->getTemplate(input->getEndPoint(), s_port->getEndPoint()).createXferRequest();
      try {
        // Create the copy in the template
        ptransfer->copy (
			 input_offsets->localStateOffset +
			 (OCPI_SIZEOF(DDT::Offset, BufferState) * MAX_PCONTRIBS) +
			 (OCPI_SIZEOF(DDT::Offset, BufferState)* input->getPortId()),
			 input_offsets->myShadowsRemoteStateOffsets[s_pid],
			 sizeof(BufferState),
			 XferRequest::FlagTransfer );
      }
      catch( ... ) {
        FORMAT_TRANSFER_EC_RETHROW( input, s_port );
      }

      // Add the transfer to the template
      temp->addTransfer( ptransfer );

    } // end for each input buffer

  } // end for each output port

}


// Here is where all of the work is performed
 TransferController* 
   TransferTemplateGenerator::createTemplates( Transport* transport, 
                                                                 PortSet* output, 
                                                                 PortSet* input, TransferController* temp_controller  )
{
  // Do the work
  create(transport, output,input,temp_controller);

  // return the controller
  return temp_controller;        
}




TransferTemplateGeneratorPattern1::
~TransferTemplateGeneratorPattern1()
{
  // Empty
}


// Create transfers for output port for the pattern w[p] -> w[p]
void TransferTemplateGeneratorPattern1::createOutputTransfers( Port* s_port, PortSet* input,
                                                              TransferController* cont )
{

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // input buffer
      InputBuffer* t_buf = input->getPort(0)->getInputBuffer(t_buffers);
      int t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplate(1);


      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant
      ocpiDebug("output port id = %d, buffer id = %d, input id = %d\n", 
             s_port->getPortId(), s_tid, t_tid);
      ocpiDebug("Template address = %p\n", temp);

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

      /*
       *  This transfer is used to mark the local input shadow buffer as full
       */

      // We need to setup a transfer for each input port. 
      ocpiDebug("Number of input ports = %d\n", n_t_ports);

      for ( n=0; n<n_t_ports; n++ ) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = t_port->getInputBuffer(t_buffers);

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

          ocpiDebug("** ZERO COPY TransferTemplateGeneratorPattern1::createOutputTransfers from %p, to %p",
		    s_buf, t_buf);
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }

        // Create the transfer that copys the output data to the input data
        XferRequest* ptransfer =
	  s_port->getTemplate(s_port->getEndPoint(), t_port->getEndPoint()).createXferRequest();
        try {
          ptransfer->copy (
			   output_offsets->bufferOffset,
			   input_offsets->bufferOffset,
			   output_offsets->bufferSize,
			   XferRequest::DataTransfer );

	  DtOsDataTypes::Offset	metaOffset =
	    output_offsets->metaDataOffset +
	    s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData);
	  uint32_t options = t_port->getMetaData()->m_descriptor.options;

	  if (!(options & (1 << FlagIsMeta)))
	    // Create the transfer that copys the output meta-data to the input meta-data
	    ptransfer->copy(metaOffset,
			    input_offsets->metaDataOffset +
			    s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			    sizeof(OCPI::OS::int64_t),
			    XferRequest::MetaDataTransfer);
	  // The flag transfer which could be three things
	  ptransfer->copy(// source offset, depends on mode
			  options & (1 << FlagIsCounting) ?
			  metaOffset + OCPI_OFFSETOF(DDT::Offset, RplMetaData, timestamp) :
			  options & (1 << FlagIsMeta) ?
			  metaOffset + OCPI_OFFSETOF(DDT::Offset, RplMetaData, xferMetaData) :
			  output_offsets->localStateOffset +
			  OCPI_SIZEOF(DDT::Offset, BufferState) * MAX_PCONTRIBS +
			  s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			  // destination offset
			  input_offsets->localStateOffset +
			  s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			  sizeof(BufferState),
			  XferRequest::FlagTransfer);
        } catch( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Add the transfer 
        temp->addTransfer( ptransfer );

      } // end for each input buffer

    } // end for each output buffer

  }  // end for each input port

}


void TransferTemplateGeneratorPattern1::createInputTransfers(PortSet* output, Port* input,
                                                              TransferController* cont )

{

  // For this pattern, we can use the base class implementation which broadcasts a
  // input buffers availability
  TransferTemplateGenerator::createInputTransfers( output, input, cont );
}


class OcpiTransferTemplateAFC : public OcpiTransferTemplate
{
public:
  OcpiTransferTemplateAFC(OCPI::OS::uint32_t id)
    :OcpiTransferTemplate(id){}
  virtual ~OcpiTransferTemplateAFC(){}
  virtual bool isSlave()
  {
    ocpiDebug("*******I Am A slave port");
    return true;
  }
};






// Create transfers for a output port that has a ActiveFlowControl role.  This means that the 
// the only transfer that takes place is the "flag" transfer.  It is the responibility of the 
// remote "pull" port to tell us when our output buffer becomes free.
void 
TransferTemplateGeneratorPattern1AFC::
createOutputTransfers(OCPI::DataTransport::Port* s_port, 
                          OCPI::DataTransport::PortSet* input,
                          TransferController* cont )
{

  //  ocpiAssert(!"pattern1AFC output");
  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  PortOrdinal n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    s_buf->setSlave();
    int s_tid = s_buf->getTid();

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // input buffer
      InputBuffer* t_buf = input->getPort(0)->getInputBuffer(t_buffers);
      int t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplateAFC(1);


      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant
      ocpiDebug("output port id = %d, buffer id = %d, input id = %d", 
             s_port->getPortId(), s_tid, t_tid);
      ocpiDebug("Template address = %p", temp);

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

      // We need to setup a transfer for each input port. 
      ocpiDebug("Number of input ports = %d", n_t_ports);

      for (PortOrdinal n = 0; n < n_t_ports; n++) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = t_port->getInputBuffer(t_buffers);

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

          ocpiDebug("** ZERO COPY TransferTemplateGeneratorPattern1AFC::createOutputTransfers from %p, to %p",
		    s_buf, t_buf);
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }

        // Create the transfer that copys the output data to the input data
        XferRequest* ptransfer =
	  s_port->getTemplate(s_port->getEndPoint(), t_port->getEndPoint()).createXferRequest();
        // Note that in the ActiveFlowControl mode we only send the state to indicate that our
        // buffer is ready for the remote actor to pull data.
        try {
	  ptransfer->copy(t_port->getMetaData()->m_descriptor.options & (1 << FlagIsMeta) ?
			  output_offsets->metaDataOffset +
			  s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData) +
			  OCPI_OFFSETOF(DDT::Offset, RplMetaData, xferMetaData) :
			  output_offsets->localStateOffset +
			  s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			  input_offsets->localStateOffset +
			  s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			  sizeof(BufferState),
			  XferRequest::FlagTransfer);
        } catch( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Add the transfer 
        temp->addTransfer( ptransfer );

      } // end for each input buffer

    } // end for each output buffer

  }  // end for each input port

}





// This base class provides a default pattern for the input buffers which is to 
// braodcast a input buffers availability to all shadows
void TransferTemplateGeneratorPattern1AFC::createInputTransfers(PortSet* output, Port* input,
                                                      TransferController* cont )

{
  // We need to create the transfers to tell all of our shadow ports that a buffer
  // became available, 
  BufferOrdinal n_t_buffers = input->getBufferCount();

  // We need a transfer template to allow a transfer for each input buffer to its
  // associated shadows
  for ( BufferOrdinal t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

    // input buffer
    InputBuffer* t_buf = input->getInputBuffer(t_buffers);
    int t_tid = t_buf->getTid();

    // Create a template
    OcpiTransferTemplate* temp = new OcpiTransferTemplateAFC(0);

    ocpiDebug("*&*&* Adding template for tpid = %d, ttid = %d, template = %p", 
           input->getPortId(), t_tid, temp);

    //Add the template to the controller
    cont->addTemplate( temp, 0, 0, input->getPortId(), t_tid, false, TransferController::INPUT );
 
    // Since there may be multiple output ports on 1 processs, we need to make sure we dont send
    // more than 1 time
    int sent[MAX_PCONTRIBS];
    memset(sent,0,sizeof(int)*MAX_PCONTRIBS);

    // We need to setup a transfer for each shadow, they exist in the unique output circuits
    for (PortOrdinal n=0; n<output->getPortCount(); n++ ) {

      // Since the shadows only exist in the circuits with instances of real
      // output ports, the offsets are indexed via the output port ordinals.
      // If the output is co-located with us, no shadow exists.
      Port* s_port = output->getPort(n);
      int s_pid = s_port->getRealShemServices()->endPoint().mailBox();

      if ( sent[s_pid] ) {
        continue;
      }

      // If we are creating a template for whole transfers, we do not recognize anything
      // but output port 0
      if ( (s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel) &&
           s_port->getRank() != 0 ) {
        continue;
      }

      // Attach zero-copy for co-location
      if ( m_zcopyEnabled && s_port->supportsZeroCopy( input ) ) {
        ocpiDebug("Adding Zery copy for input response");
        temp->addZeroCopyTransfer( NULL, t_buf );
      }

      sent[s_pid] = 1;


    } // end for each input buffer

  } // end for each output port

}

void 
TransferTemplateGeneratorPattern1Passive::
createOutputTransfers(OCPI::DataTransport::Port */*s_port*/,
		      OCPI::DataTransport::PortSet */*input*/,
		      TransferController */*cont*/ ) {
}
void TransferTemplateGeneratorPattern1Passive::
createInputTransfers(PortSet */*output*/, Port */*input*/, TransferController */*cont*/) {
}

// In AFC mode, the shadow port is responsible for pulling the data from the real output port, and then
// Telling the output port that its buffer is empty.
void TransferTemplateGeneratorPattern1AFCShadow::createOutputTransfers( Port* s_port, PortSet* input,
                                                              TransferController* cont )
{
  ocpiAssert("pattern1AFCshadow output"==0);

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      // input buffer
      InputBuffer* t_buf = input->getPort(0)->getInputBuffer(t_buffers);
      int t_tid = t_buf->getTid();

      // Create a template
      OcpiTransferTemplate* temp = new OcpiTransferTemplateAFC(1);


      // Add the template to the controller, for this pattern the output port
      // and the input ports remains constant
      ocpiDebug("output port id = %d, buffer id = %d, input id = %d", 
             s_port->getPortId(), s_tid, t_tid);
      ocpiDebug("Template address = %p", temp);

      cont->addTemplate( temp, s_port->getPortId(),
                         s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

      /*
       *  This transfer is used to mark the local input shadow buffer as full
       */

      // We need to setup a transfer for each input port. 
      ocpiDebug("Number of input ports = %d", n_t_ports);

      for ( n=0; n<n_t_ports; n++ ) {

        // Get the input port
        Port* t_port = input->getPort(n);
        t_buf = t_port->getInputBuffer(t_buffers);

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

          ocpiDebug("** ZERO COPY TransferTemplateGeneratorPattern1::createOutputTransfers from %p, to %p",
		    s_buf, t_buf);
          temp->addZeroCopyTransfer( s_buf, t_buf );
          continue;
        }
        // Create the transfer that copys the output data to the input data
        XferRequest* ptransfer =
	  s_port->getTemplate(s_port->getEndPoint(), t_port->getEndPoint()).createXferRequest();
        try {

          // Create the data buffer transfer
          ptransfer->copy (
			   output_offsets->bufferOffset,
			   input_offsets->bufferOffset,
			   output_offsets->bufferSize,
			   XferRequest::DataTransfer );

          // Create the transfer that copies the output meta-data to the input meta-data 
          ptransfer->copy (
			   output_offsets->metaDataOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			   input_offsets->metaDataOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			   sizeof(OCPI::OS::int64_t),
			   XferRequest::MetaDataTransfer  );

          // FIXME.  We need to allocate a separate local flag for this.  (most dma engines dont have
          // an immediate mode).
          // Create the transfer that copies our state to back to the output to indicate that its buffer
          // is now available for re-use.
          ptransfer->copy (
			   input_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			   output_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			   sizeof(BufferState),
			   XferRequest::FlagTransfer );

        }
        catch( ... ) {
          FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
        }

        // Add the transfer 
        temp->addTransfer( ptransfer );

      } // end for each input buffer

    } // end for each output buffer

  }  // end for each input port

}






/*
 *
 ************************************************************************************
 *
 * Pattern 2
 *
 ************************************************************************************
 *
 */

// Create transfers for input port
void 
TransferTemplateGeneratorPattern2::
createInputTransfers(PortSet* output, 
                      Port* input, 
                      TransferController* cont )

{

  // For this pattern, we can use the base class implementation which broadcasts a
  // input buffers availability
  TransferTemplateGenerator::createInputTransfers( output, input, cont );

}




// Create transfers for output port
void 
TransferTemplateGeneratorPattern2::
createOutputTransfers(Port* s_port, 
                      PortSet* input,
                      TransferController* cont  )
{

  ocpiAssert("pattern2 output"==0);
  /*
   *        For a WP / SW transfer, we need to be capable of transfering from any
   *  output buffer to any one input buffer.
   */

  ocpiDebug("In createOutputTransfers, pattern #2");

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();

    // Now we need to create a template for each input port
    for ( n=0; n<n_t_ports; n++ ) {

      // Get the input port
      Port* t_port = input->getPort(n);

      // We need a transfer template to allow a transfer to each input buffer
      for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

        // Get the buffer for the current port
        // input buffer
        InputBuffer* t_buf = t_port->getInputBuffer(t_buffers);
        int t_tid = t_buf->getTid();

        // Create a template
        OcpiTransferTemplate* temp = new OcpiTransferTemplate(2);

        // Add the template to the controller, for this pattern the output port
        // and the input ports remains constant

        ocpiDebug("output port id = %d, buffer id = %d, input id = %d", 
               s_port->getPortId(), s_tid, t_tid);
        ocpiDebug("Template address = %p", temp);
        cont->addTemplate( temp, s_port->getPortId(),
                           s_tid, t_port->getPortId() ,t_tid, false, TransferController::OUTPUT );

        struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
          &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

        struct PortMetaData::InputPortBufferControlMap *input_offsets = 
          &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

        TDataInterface tdi(s_port,s_tid,t_port,t_tid);

        // We need to determine if this can be a Zero copy transfer.  If so, 
        // we dont need to create a transfer template
        bool standard_transfer = true;
        if ( m_zcopyEnabled && s_port->supportsZeroCopy( t_port ) ) {

          ocpiDebug("** ZERO COPY TransferTemplateGeneratorPattern2::createOutputTransfers from %p, to %p",
		    s_buf, t_buf);
          temp->addZeroCopyTransfer( s_buf, t_buf );
          standard_transfer = false;
        }

        XferRequest::Flags flags;
        XferRequest* ptransfer =
	  s_port->getTemplate(s_port->getEndPoint(), t_port->getEndPoint()).createXferRequest();

        // Pre-data transfer hook
        bool added = addTransferPreData( ptransfer, tdi );
        flags =added ? XferRequest::None : XferRequest::DataTransfer;


        // Create the transfer that copys the output data to the input data
        if ( standard_transfer ) {

          try {
            ptransfer->copy (
			     output_offsets->bufferOffset,
			     input_offsets->bufferOffset,
			     output_offsets->bufferSize,
			     (XferRequest::Flags)(flags | XferRequest::DataTransfer ) );
          }
          catch ( ... ) {
            FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
          }

        }

        // Post-data transfer hook
        addTransferPostData( ptransfer, tdi);

        // Create the transfer that copys the output meta-data to the input meta-data
        if ( standard_transfer ) {

          try {
            ptransfer->copy (
			     output_offsets->metaDataOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			     input_offsets->metaDataOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			     sizeof(OCPI::OS::int64_t),
			     XferRequest::MetaDataTransfer );
          }
          catch ( ... ) {
            FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
          }

        }

        // Pre-state transfer hook
        addTransferPreState(  ptransfer, tdi );

        // Create the transfer that copys the output state to the remote input state
        if ( standard_transfer ) {
          try {
            ptransfer->copy (
			     output_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			     input_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			     sizeof(BufferState),
			     XferRequest::FlagTransfer );
          }
          catch ( ... ) {
            FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
          }
        }

        // Add the transfer 
	temp->addTransfer( ptransfer );

      } // end for each input buffer

    } // end for each input port

  }  // end for each output buffer
}




/*
 *
 ************************************************************************************
 *
 * Pattern 3
 *
 ************************************************************************************
 *
 */
bool TransferTemplateGeneratorPattern3::addTransferPreState( XferRequest* pt, TDataInterface& tdi)
{

  ocpiDebug("*** In TransferTemplateGeneratorPattern3::addTransferPreState()");

  // We need to update all of the shadow buffers for all "real" output ports
  // to let them know that the input buffer for this input port has been allocated

  struct PortMetaData::InputPortBufferControlMap *input_offsets = 
    &tdi.t_port->getMetaData()->m_bufferData[tdi.t_tid].inputOffsets;

  int our_smb_id = tdi.s_port->getMailbox();

  PortSet *sps = static_cast<PortSet*>(tdi.s_port->getPortSet());
  for (PortOrdinal n = 0; n < sps->getPortCount(); n++) {

    Port* shadow_port = static_cast<Port*>(sps->getPortFromIndex(n));
    int idx = shadow_port->getMailbox();

    // We need to ignore self transfers
    if ( idx == our_smb_id) {
      continue;
    }

    // A shadow for a output may not exist if they are co-located
    if ( input_offsets->myShadowsRemoteStateOffsets[idx] != 0 ) {

      ocpiDebug("TransferTemplateGeneratorPattern3::addTransferPreState mapping shadow offset to 0x%llx", 
             (long long)input_offsets->myShadowsRemoteStateOffsets[idx]);

      XferRequest* ptransfer =
	tdi.s_port->getTemplate(tdi.s_port->getEndPoint(), shadow_port->getEndPoint()).
	createXferRequest();
      pt->group(ptransfer);

      try {

        // Create the transfer that copys the local shadow buffer state to the remote
        // shadow buffers state
        ptransfer->copy (
			 input_offsets->myShadowsRemoteStateOffsets[our_smb_id],
			 input_offsets->myShadowsRemoteStateOffsets[idx],
			 sizeof(BufferState),
			 XferRequest::FlagTransfer );

      }
      catch ( ... ) {
        FORMAT_TRANSFER_EC_RETHROW( tdi.s_port, shadow_port );
      }

    }
  }

  // Now we need to pass the output control baton onto the next output port
  int next_output = (tdi.s_port->getPortId() + 1) % sps->getPortCount();
  Port* next_sp = static_cast<Port*>(sps->getPortFromIndex(next_output));

  struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
    &tdi.s_port->getMetaData()->m_bufferData[tdi.s_tid].outputOffsets;

  struct PortMetaData::OutputPortBufferControlMap *next_output_offsets = 
    &next_sp->getMetaData()->m_bufferData[tdi.s_tid].outputOffsets;

  XferRequest * ptransfer2 =
    tdi.s_port->getTemplate(tdi.s_port->getEndPoint(), next_sp->getEndPoint()).
    createXferRequest();

  // Create the transfer from out output contol state to the next

  try {
    ptransfer2->copy (
		      output_offsets->portSetControlOffset,
		      next_output_offsets->portSetControlOffset,
		      sizeof(OutputPortSetControl),
		      XferRequest::FlagTransfer );
  }
  catch ( ... ) {
    FORMAT_TRANSFER_EC_RETHROW(  tdi.s_port, next_sp );
  }

  pt->group(ptransfer2);

  return true;
}



/*
 *
 ************************************************************************************
 *
 * Pattern 4
 *
 ************************************************************************************
 *
 */

// Create transfers for input port
void TransferTemplateGeneratorPattern4::createInputTransfers(PortSet* output, 
                                                              Port* input, 
                                                              TransferController* cont )

{
  // For this pattern, we can use the base class implementation which broadcasts a
  // input buffers availability
  TransferTemplateGenerator::createInputTransfers( output, input, cont );
}



// Create transfers for output port
void 
TransferTemplateGeneratorPattern4::
createOutputTransfers(Port* s_port, 
                      PortSet* input,
                      TransferController* cont  )
{

  ocpiAssert("pattern4 output"==0);
  /*
   *        For a WP / P(Parts) transfer, we need to be capable of transfering from any
   *  output buffer to all input port buffers.
   */

  ocpiDebug("In TransferTemplateGeneratorPattern4::createOutputTransfers");

  // Since this is a whole output distribution, only port 0 of the output
  // set gets to do anything.
  if ( s_port->getPortSet()->getDataDistribution()->getMetaData()->distType == DataDistributionMetaData::parallel &&
       s_port->getRank() != 0 ) {
    return;
  }

  int n;
  PortSet* output = s_port->getPortSet();
  int n_s_buffers = output->getBufferCount();
  int n_t_buffers = input->getBufferCount();
  int n_t_ports = input->getPortCount();

  DataPartition* dpart = input->getDataDistribution()->getDataPartition();

  // Get the number of transfers that it is going to take to satisfy this output
  int n_transfers_per_output_buffer = dpart->getTransferCount(output, input);

  // For this pattern we need a transfer count that is a least as large as the number
  // of input ports, since we have to inform everyone about the end of whole
  if ( m_markEndOfWhole ) {
    if ( n_transfers_per_output_buffer < n_t_ports ) {

      //                  n_transfers_per_output_buffer = n_t_ports;

    }
  }

  // Get the total number of parts that make up the whole
  int parts_per_whole = dpart->getPartsCount(output, input);

  ocpiDebug("** There are %d transfers to complete this set", n_transfers_per_output_buffer);
  ocpiDebug("There are %d parts per whole", parts_per_whole );

  // We need a transfer template to allow a transfer from each output buffer to every
  // input buffer for this pattern.
  int sequence;
  OcpiTransferTemplate *root_temp=NULL, *temp=NULL;
  for ( int s_buffers=0; s_buffers<n_s_buffers; s_buffers++ ) {

    // output buffer
    OutputBuffer* s_buf = s_port->getOutputBuffer(s_buffers);
    int s_tid = s_buf->getTid();
    int part_sequence;

    // We need a transfer template to allow a transfer to each input buffer
    for ( int t_buffers=0; t_buffers<n_t_buffers; t_buffers++ ) {

      //Each output buffer may need more than 1 transfer to satisfy itself
      sequence = 0;
      part_sequence = 0;
      for ( int transfer_count=0; transfer_count<n_transfers_per_output_buffer; 
            transfer_count++, sequence++ ) {

        // Get the input port
        Port* in_port = input->getPort(0);
        InputBuffer* t_buf = in_port->getInputBuffer(t_buffers);
        int t_tid = t_buf->getTid();

        // We need to be capable of transfering the gated transfers to all input buffers
        for ( int t_gated_buffer=0; t_gated_buffer<n_t_buffers+1; t_gated_buffer++ ) {

          // This may be gated transfer
          if ( (transfer_count == 0) && (t_gated_buffer == 0)) {

            temp = new OcpiTransferTemplate(4);
            root_temp = temp;

            // Add the template to the controller, 
            cont->addTemplate( temp, s_port->getPortId(),
                               s_tid, 0 ,t_tid, false, TransferController::OUTPUT );

          }
          else {
            part_sequence = transfer_count * n_t_ports;
            temp = new OcpiTransferTemplate(4);
            root_temp->addGatedTransfer( sequence, temp, 0, t_tid);
          }


          // We need to setup a transfer for each input port. 
          for ( n=0; n<n_t_ports; n++ ) {

            // Get the input port
            Port* t_port = input->getPort(n);
            t_buf = t_port->getInputBuffer(t_buffers);

            struct PortMetaData::OutputPortBufferControlMap *output_offsets = 
              &s_port->getMetaData()->m_bufferData[s_tid].outputOffsets;

            struct PortMetaData::InputPortBufferControlMap *input_offsets = 
              &t_port->getMetaData()->m_bufferData[t_tid].inputOffsets;

            // Since this is a "parts" transfer, we dont allow zero copy
            XferRequest* ptransfer =
	      s_port->getTemplate(s_port->getEndPoint(), t_port->getEndPoint()).
	      createXferRequest();

            // Now we need to go to the data partition class and ask for some offsets
            DataPartition::BufferInfo *bi_tmp, *buffer_info;
            dpart->calculateBufferOffsets( transfer_count, s_buf, t_buf, &buffer_info);


            bi_tmp = buffer_info;
            int total_bytes=0;
            while ( bi_tmp ) {

              try {

                // Create the transfer that copys the output data to the input data
                ptransfer->copy (
				 output_offsets->bufferOffset + bi_tmp->output_offset,
				 input_offsets->bufferOffset + bi_tmp->input_offset,
				 bi_tmp->length,
				 XferRequest::DataTransfer );

                total_bytes += bi_tmp->length;

              }
              catch ( ... ) {
                FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
              }

              bi_tmp = bi_tmp->next;
            }
            delete buffer_info;

            // At this point we need to tell the template what needs to be inserted into
            // the output meta-data prior to transfer, this includes the actual number
            // of bytes that were transfered and the end of whole indicator.
            bool end_of_whole;
            if (  transfer_count == (n_transfers_per_output_buffer-1) ) {
              end_of_whole = true;
            }
            else {
              end_of_whole = false;
            }
            temp->presetMetaData( s_buf->getMetaDataByIndex( t_port->getPortId() ),
                                  total_bytes, end_of_whole, parts_per_whole, part_sequence++ );


            try {

              // Create the transfer that copys the output meta-data to the input meta-data
              ptransfer->copy (
			       output_offsets->metaDataOffset + t_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			       input_offsets->metaDataOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferMetaData),
			       sizeof(OCPI::OS::uint64_t),
			       XferRequest::MetaDataTransfer );

              // Create the transfer that copys the output state to the remote input state
              ptransfer->copy (
			       output_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			       input_offsets->localStateOffset + s_port->getPortId() * OCPI_SIZEOF(DDT::Offset, BufferState),
			       sizeof(BufferState),
			       XferRequest::FlagTransfer );
            }
            catch ( ... ) {
              FORMAT_TRANSFER_EC_RETHROW( s_port, t_port );
            }

            // Add the transfer 
            temp->addTransfer( ptransfer );

          } // end for each input port

          t_buf = in_port->getInputBuffer(t_gated_buffer%n_t_buffers);
          t_tid = t_buf->getTid();

        } // for each gated buffer

      } // end for each input buffer

    } // end for n transfers

  }  // end for each output buffer 

}


