
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


//#define SLEEP_DEBUG
//#define DEBUG_WITH_EMULATOR

/*
 * Abstact:
 *   This file contains the OCPI circuit implementation.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 *    John Miller - 6/15/09
 *    Fixed Coverity issues.
 *
 */

#include <string.h>
#include <DtTransferInternal.h>
#include <DtHandshakeControl.h>
#include <OcpiPortMetaData.h>
#include <OcpiParallelDataDistribution.h>
#include <OcpiTransportConstants.h>
#include <OcpiDataDistribution.h>
#include <OcpiPort.h>
#include <OcpiTransport.h>
#include <OcpiCircuit.h>
#include <OcpiBuffer.h>
#include <OcpiPortSet.h>
#include <OcpiTransferTemplate.h>
#include <OcpiOutputBuffer.h>
#include <OcpiInputBuffer.h>
#include <OcpiTransferController.h>
#include <OcpiTemplateGenerators.h>
#include <OcpiList.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiRDTInterface.h>
#include <OcpiTimeEmit.h>
#include <OcpiPullDataDriver.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <OcpiTimeEmitCategories.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace OCPI::OS;
using namespace ::DataTransport::Interface;
namespace DtI = ::DataTransport::Interface;

// our init flag
OCPI::OS::uint32_t OCPI::DataTransport::Circuit::m_init=0;


/**********************************
 * Constructors
 *********************************/
OCPI::DataTransport::Circuit::
Circuit( 
        OCPI::DataTransport::Transport* t,
        CircuitId& id,
        ConnectionMetaData* connection, 
        PortOrdinal src_ps[],
        PortOrdinal dest_pss[])
  :  CU::Child<OCPI::DataTransport::Transport,Circuit>(*t),
     OCPI::Time::Emit(t, "Circuit"),
     m_transport(t), m_status(Unknown),
     m_ready(false),m_updated(false),
     m_outputPs(0), m_inputPs(0),  m_metaData(connection) ,m_portsets_init(0),
     m_protocol(NULL), m_protocolSize(0), m_protocolOffset(0)
{
  ( void ) src_ps;
  ( void ) dest_pss;
  m_ref_count = 1;
  m_circuitId = id;
  m_openCircuit=true;
  m_templatesGenerated = false;
  m_fromQ = false;

  ocpiDebug(" In Circuit::Circuit() this %p id is 0x%x\n", this, id);

  // static init stuff goes here
  if ( t->m_transportGlobal->m_Circuitinit == false ) {
    t->m_transportGlobal->m_Circuitinit = true;

    // This is where we fill in the matrix that is used to get the transfer
    // temlpate generators
    m_transport->m_transportGlobal->m_gen_temp_gen = new TransferTemplateGeneratorNotSupported();
    int a,b,c,d,e,f,g,y;
    for(a=0;a<2;a++)
      for(b=0;b<2;b++)
        for(c=0;c<2;c++)
          for(d=0;d<2;d++)
            for(e=0;e<2;e++)
              for(f=0;f<OCPI::RDT::MaxRole;f++)
                for(g=0;g<OCPI::RDT::MaxRole;g++)
                  m_transport->m_transportGlobal->m_templateGenerators[a][b][c][d][e][f][g] = 
                    m_transport->m_transportGlobal->m_gen_temp_gen;

    m_transport->m_transportGlobal->m_gen_pat1 = new TransferTemplateGeneratorPattern1( );
    m_transport->m_transportGlobal->m_gen_pat1AFC = new TransferTemplateGeneratorPattern1AFC( );
    m_transport->m_transportGlobal->m_gen_pat1AFCShadow = new TransferTemplateGeneratorPattern1AFCShadow( );
    m_transport->m_transportGlobal->m_gen_pat2 = new TransferTemplateGeneratorPattern2( );
    m_transport->m_transportGlobal->m_gen_pat3 = new TransferTemplateGeneratorPattern3( );
    m_transport->m_transportGlobal->m_gen_pat4 = new TransferTemplateGeneratorPattern4( );

    //        output dd : input dd : output part : input part : "output shadow" : output role : input role
    m_transport->m_transportGlobal->m_templateGenerators[DataDistributionMetaData::parallel][DataDistributionMetaData::parallel]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveFlowControl] 
      = m_transport->m_transportGlobal->m_gen_pat1;
    m_transport->m_transportGlobal->m_templateGenerators[DataDistributionMetaData::parallel][DataDistributionMetaData::parallel]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [true] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveFlowControl] 
      = m_transport->m_transportGlobal->m_gen_pat1;


    // If the output port is AFC, the same transfer pattern generator is used for any input port role
    for( y=0; y<OCPI::RDT::MaxRole; y++) {

      m_transport->m_transportGlobal->m_templateGenerators[DataDistributionMetaData::parallel][DataDistributionMetaData::parallel]
        [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
        [false] [OCPI::RDT::ActiveFlowControl] [y] 
        = m_transport->m_transportGlobal->m_gen_pat1AFC;

      m_transport->m_transportGlobal->m_templateGenerators[DataDistributionMetaData::parallel][DataDistributionMetaData::parallel]
        [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
        [true] [OCPI::RDT::ActiveFlowControl] [y] 
        = m_transport->m_transportGlobal->m_gen_pat1AFCShadow;
    }




    // Fixme.  All other DD&P patterns have not yet beed ported to the new port "roles" paradigm
    m_transport->m_transportGlobal->m_templateGenerators[DataDistributionMetaData::parallel][DataDistributionMetaData::sequential]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveFlowControl] 
      = m_transport->m_transportGlobal->m_gen_pat2;

    m_transport->m_transportGlobal->m_templateGenerators[DataDistributionMetaData::sequential][DataDistributionMetaData::sequential]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveFlowControl] 
      = m_transport->m_transportGlobal->m_gen_pat3;

    m_transport->m_transportGlobal->m_templateGenerators[DataDistributionMetaData::parallel][DataDistributionMetaData::parallel]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::BLOCK] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveFlowControl] 
      = m_transport->m_transportGlobal->m_gen_pat4;
        

    // Same thing for the transfer controllers
    m_transport->m_transportGlobal->m_gen_control = new TransferControllerNotSupported();
    for(a=0;a<2;a++)
      for(b=0;b<2;b++)
        for(c=0;c<2;c++)
          for(d=0;d<2;d++)
            for(e=0;e<2;e++)
              for(f=0;f<OCPI::RDT::MaxRole;f++)
                for(g=0;g<OCPI::RDT::MaxRole;g++)
                  m_transport->m_transportGlobal->m_transferControllers[a][b][c][d][e][f][g]
                    = m_transport->m_transportGlobal->m_gen_control;

    
    m_transport->m_transportGlobal->m_cont1 = new TransferController1();
    m_transport->m_transportGlobal->m_cont1AFCShadow = new TransferController1AFCShadow();
    m_transport->m_transportGlobal->m_cont2 = new TransferController2();
    m_transport->m_transportGlobal->m_cont3 = new TransferController3();
    m_transport->m_transportGlobal->m_cont4 = new TransferController4();

    //        output dd : input dd : output part : input part : "output shadow" : output role : input role
    m_transport->m_transportGlobal->m_transferControllers[DataDistributionMetaData::parallel] [DataDistributionMetaData::parallel]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveFlowControl] 
      = m_transport->m_transportGlobal->m_cont1;

    m_transport->m_transportGlobal->m_transferControllers[DataDistributionMetaData::parallel] [DataDistributionMetaData::parallel]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [true] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveFlowControl] 
      = m_transport->m_transportGlobal->m_cont1;

    // If the output port is AFC, the same controller is used for any input port role
    for( y=0; y<OCPI::RDT::MaxRole; y++) {
      m_transport->m_transportGlobal->m_transferControllers[DataDistributionMetaData::parallel] [DataDistributionMetaData::parallel]
        [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
        [true] [OCPI::RDT::ActiveFlowControl] [y] 
        = m_transport->m_transportGlobal->m_cont1AFCShadow;

      m_transport->m_transportGlobal->m_transferControllers[DataDistributionMetaData::parallel] [DataDistributionMetaData::parallel]
        [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
        [false] [OCPI::RDT::ActiveFlowControl] [y] 
        = m_transport->m_transportGlobal->m_cont1AFCShadow;
    }





    // Fixme.  All other DD&P patterns have not yet beed ported to the new port "roles" paradigm
    m_transport->m_transportGlobal->m_transferControllers[DataDistributionMetaData::parallel][DataDistributionMetaData::sequential]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveMessage] 
      = m_transport->m_transportGlobal->m_cont2;

    m_transport->m_transportGlobal->m_transferControllers[DataDistributionMetaData::sequential][DataDistributionMetaData::sequential]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::INDIVISIBLE] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveMessage] 
      = m_transport->m_transportGlobal->m_cont3;

    m_transport->m_transportGlobal->m_transferControllers[DataDistributionMetaData::parallel][DataDistributionMetaData::parallel]
      [DataPartitionMetaData::INDIVISIBLE][DataPartitionMetaData::BLOCK] 
      [false] [OCPI::RDT::ActiveMessage] [OCPI::RDT::ActiveMessage] 
      = m_transport->m_transportGlobal->m_cont4;

  }
  m_init++;

  // Now we can initialize our port sets.
  update();

  m_lastPortSet = 0;
  m_maxPortOrd = 0;

  for ( OCPI::OS::uint32_t psc=0; psc<m_metaData->m_portSetMd.size(); psc++ ) {
    m_maxPortOrd+= static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[psc])->m_portMd.size();
  }

  ocpiDebug("Circuit::Circuit: po = %d", m_maxPortOrd );

  if ( m_maxPortOrd > 1 ) {
    Port* port = this->getOutputPortSet()->getPortFromIndex(0);

    if (port->m_data->real_location_string.length() ) {
      ocpiDebug("Circuit is closed 1: id %x", getCircuitId());
      m_openCircuit = false;
    }
  }

  // *** NOTE  ***, for now we are ignoring the port sub-set cases


  /*
   * There are effectively two critical pieces to the construction of 
   * the circuit, the first is the transfer template setup. The templates 
   * provide an efficient mechanism by which transfers can take place from 
   * any output buffer to any inputs port buffer.  The second piece is the
   * transfer controller.
   */

}


void 
OCPI::DataTransport::Circuit::
finalize( const char* endpoint )
{
  OCPI::DataTransport::ConnectionMetaData* cmd = getConnectionMetaData();
  OCPI::DataTransport::PortSetMetaData* psmd = static_cast<OCPI::DataTransport::PortSetMetaData*>(cmd->m_portSetMd[0]);
  psmd->getPortInfo(0)->real_location_string = endpoint;
  updatePort( getOutputPortSet()->getPort(0) );
}
      

/**********************************
 * Destructor
 *********************************/
OCPI::DataTransport::Circuit::
~Circuit()
{

#ifdef DD_N_P_SUPPORTED
  // First we will try to tell all of the input ports that we are going away.
  for ( OCPI::OS::uint32_t n=0; n<this->getOutputPortSet()->getPortCount(); n++ ) {
    printf("spc = %d\n", this->getOutputPortSet()->getPortCount());
    OCPI::DataTransport::Port* port = 
      static_cast<OCPI::DataTransport::Port*>(this->getOutputPortSet()->getPortFromIndex(n));
    printf("port = %d\n", port );
    if ( ! port->isShadow() ) {
      OutputBuffer* buffer = port->getOutputBuffer(0);
      buffer->getMetaData()->endOfCircuit = true;
      this->broadcastBuffer( buffer );
    }
  }
#endif

  // Since our children have a reference to our metadata, we need to make sure they
  // get removed first
  PortSet* ps = firstChild();
  while ( ps ) {
    delete ps;
    ps = firstChild();
  }

  delete m_metaData;

  if (m_protocol)
    delete [] m_protocol;
  //  ocpiAssert( m_ref_count == 0 );
}


void OCPI::DataTransport::Circuit::
updateInputs( void* data )
{
  m_updated = true;
  DataTransfer::ContainerComms::RequestUpdateCircuit* update = 
    static_cast<DataTransfer::ContainerComms::RequestUpdateCircuit*>(data);

  // This is a request coming from the primary output port (ordinal 0).  It is telling us
  // about the other ports in the circuit and also about its endpoint and offsets so that
  // we can let it know when we have consumed a buffer that it sent ot us.

  // Port set ordinal 0 is always the output set, we need to update ours 
  static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[0])->getPortInfo(0)->real_location_string = update->output_end_point;

  this->getOutputPortSet()->getPort(0)->m_data->m_bufferData->outputOffsets.portSetControlOffset=
    update->senderOutputControlOffset;
  this->getOutputPortSet()->getPort(0)->initialize();

  // Now update the input port set with all of the real information associated with the circuit
  PortSetMetaData* input_ps = static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[1]);

  // For DRI we need placholders for all of the inputs
  unsigned int n;
  for (n=input_ps->m_portMd.size(); n<update->tPortCount; n++ ) {

    OCPI::RDT::Descriptors tdesc;
    strcpy(tdesc.desc.oob.oep,update->output_end_point);
    PortMetaData* sp = new PortMetaData( n,false,tdesc,NULL,input_ps);

    input_ps->m_portMd.push_back( sp );
  }

  OCPI::DataTransport::Port* port = 
    static_cast<OCPI::DataTransport::Port*>(this->getInputPortSet(0)->getPortFromOrdinal(update->receiverPortId));

  port->m_data->remoteCircuitId = update->senderCircuitId;
  port->m_data->remotePortId    = update->senderPortId;
  port->m_externalState = DataTransport::Port::WaitingForShadowBuffer;


  // Go through each port and make sure they are updated
  OCPI::DataTransport::PortSet* ps = getOutputPortSet();
  OCPI::OS::uint32_t i;
  OCPI::OS::uint32_t sports=0, tports=0;
  for ( i=0; i<ps->getPortCount(); i++ ) {
    ps->getPort(i)->update();
    sports++;
  }
  for ( OCPI::OS::uint32_t m=0; m<getInputPortSetCount(); m++) {
    ps = getInputPortSet(m);
    for ( i=0; i<ps->getPortCount(); i++ ) {
      ps->getPort(i)->update();
      tports++;
    }
  }

  m_maxPortOrd = 0;
  for ( OCPI::OS::uint32_t psc=0; psc<m_metaData->m_portSetMd.size(); psc++ ) {
    m_maxPortOrd+=static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[psc])->m_portMd.size();
  }

  // make sure we have a closed circuit
  if ( sports>0 && tports>0 ) {
    ocpiDebug("Circuit is closed 2: id %x", getCircuitId());
    m_openCircuit = false;
  }
                  
}


Port *    
OCPI::DataTransport::Circuit::
getOutputPort(int ord)
{
  return getOutputPortSet()->getPortFromOrdinal(ord);
}

bool 
OCPI::DataTransport::Circuit::
updateInputs()
{
  OCPI::DataTransport::Port* output_port = 
    static_cast<OCPI::DataTransport::Port*>(getOutputPortSet()->getPortFromIndex(0));

  // Make the request to get our offset
  SMBResources* s_res = XferFactoryManager::getFactoryManager().getSMBResources( output_port->getEndpoint() );

  XferMailBox xmb( output_port->getMailbox() );
  if ( ! xmb.mailBoxAvailable(s_res) ) {
    return false;
  }

  // Do for all input ports
  OCPI::Util::VList& tps = getInputPortSets();
  PortSet* ps;
  for ( OCPI::OS::uint32_t n=0; n<getInputPortSetCount(); n++ ) {
    ps = static_cast<PortSet*>(tps[n]);
    for ( OCPI::OS::uint32_t i=0; i<ps->getPortCount(); i++ ) {
      OCPI::DataTransport::Port* port = static_cast<OCPI::DataTransport::Port*>(ps->getPortFromIndex(i));
      // Ignore local ports
      if ( m_transport->isLocalEndpoint( port->getRealShemServices()->endpoint()->end_point.c_str() ) ) {
        if ( port->getCircuit()->getOutputPortSet()->getPortFromIndex(0) != output_port ) {
          ocpiBad("**** ERROR We have a local connection to a different circuit !!");
          throw OCPI::Util::EmbeddedException (
                                              INTERNAL_PROGRAMMING_ERROR1, 
                                              "We have a local connection to a different circuit" );
        }
        continue;
      }

      // Wait for our mailbox to become free
      while( ! xmb.mailBoxAvailable(s_res) ) {
        OCPI::OS::sleep(0);
        ocpiDebug("Waiting for our mailbox to be cleared !!");
      }

      SMBResources* t_res = 
        XferFactoryManager::getFactoryManager().getSMBResources( static_cast<OCPI::DataTransport::Port*>(ps->getPortFromIndex(i))->getEndpoint() );
      DataTransfer::ContainerComms::MailBox* mb = xmb.getMailBox( s_res );
      mb->request.reqUpdateCircuit.type = DataTransfer::ContainerComms::ReqUpdateCircuit;
      mb->request.reqUpdateCircuit.senderCircuitId = port->getCircuit()->getCircuitId();
      mb->request.reqUpdateCircuit.senderPortId = port->getPortId();
      mb->request.reqUpdateCircuit.receiverCircuitId = port->m_data->remoteCircuitId;
      mb->request.reqUpdateCircuit.receiverPortId = port->m_data->remotePortId;
      mb->request.reqUpdateCircuit.senderOutputPortId = output_port->getPortId();
      mb->request.reqUpdateCircuit.senderOutputControlOffset = 
        output_port->m_data->m_bufferData->outputOffsets.portSetControlOffset;
      strcpy(mb->request.reqUpdateCircuit.output_end_point, output_port->getEndpoint()->end_point.c_str() );
      mb->request.reqUpdateCircuit.tPortCount = ps->getPortCount();
      mb->return_offset = -1;
      mb->return_size = 0;
      mb->returnMailboxId = output_port->getMailbox();
      ocpiDebug("ReqUpdateCircuit for %x", getCircuitId());
      xmb.makeRequest( s_res, t_res );
    }
  }

  m_maxPortOrd = 0;
  for ( OCPI::OS::uint32_t psc=0; psc<m_metaData->m_portSetMd.size(); psc++ ) {
    m_maxPortOrd+=static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[psc])->m_portMd.size();
  }

  if ( m_maxPortOrd > 1 ) {
    ocpiDebug("Circuit is closed 3: id %x", getCircuitId());
    m_openCircuit = false;
  }
  return true;
}


namespace OCPI {
  namespace DataTransport {

    class PullDataDriverIP : public PullDataDriver
    {

    private:
      volatile OCPI::OS::uint8_t* current_src_buffer_ptr;
      volatile OCPI::OS::uint64_t* current_src_metaData_ptr;
      volatile OCPI::OS::uint32_t* current_empty_flag_ptr;

    public:

      PullDataDriverIP( PullDataInfo* pd )
        :PullDataDriver(pd),sbuf_index(0)
      {
        current_src_buffer_ptr = pull_data_info->src_buffer;
        current_src_metaData_ptr = pull_data_info->src_metaData;
        current_empty_flag_ptr = pull_data_info->empty_flag;
      }

      bool checkBufferEmpty( OCPI::OS::uint8_t* buffer_data, OCPI::OS::uint32_t tlen, OCPI::OS::uint64_t& metaData ) {

#ifdef SLEEP_DEBUG
          // for debug
          OCPI::OS::sleep(500);
#endif
        
        tlen = tlen < pull_data_info->pdesc.desc.dataBufferSize ? tlen : pull_data_info->pdesc.desc.dataBufferSize;

        if ( pull_data_info->pdesc.type == OCPI::RDT::ProducerDescT ) {

#ifdef DEBUG_WITH_EMULATOR
          while( *current_empty_flag_ptr != 0 ) OCPI::OS::sleep(1);
#endif

          // get the number of output buffers available
          OCPI::OS::uint32_t src_buffers_available = *pull_data_info->src_flag;

#ifndef NDEBUG
          printf("FPGA producer has %d buffers available, tlen = %d\n", 
                 src_buffers_available, tlen );
#endif
          /*
           * NOTE:  This will only work with synch. PIO. otherwise there is a race condition here.
           */
          if ( src_buffers_available == 0 ) {
            return true;
          }

          OCPI::OS::uint32_t* tgt_buffer = (OCPI::OS::uint32_t*)buffer_data;

#ifndef NDEBUG
          printf("Moving data from FPGA producer to local consumer, tgt buf = %p\n", tgt_buffer);
          printf("current_src = %p tlen = %u, fw = %d\n", current_src_buffer_ptr, tlen, *current_src_buffer_ptr );
          printf("The meta data word = %" PRIu64 "\n", *current_src_metaData_ptr );
#endif

#ifndef IP_SUPPORTS_64BIT_ACCESS
          volatile OCPI::OS::uint32_t* src = (volatile OCPI::OS::uint32_t*)current_src_buffer_ptr;
          for ( unsigned int n=0; n<(tlen/4); n++ ) {
            tgt_buffer[n] = src[n];
          }
#else
          memcpy((void*)tgt_buffer,(void*)current_src_buffer_ptr,tlen);
#endif

          metaData = *current_src_metaData_ptr;

          // Tell the IP it can advance.
          // Note that the transfer engine will send this when this buffer gets consumed
          // *current_empty_flag_ptr = 1;

          sbuf_index++;
          if ( sbuf_index >=  pull_data_info->pdesc.desc.nBuffers ) {
            current_src_buffer_ptr = pull_data_info->src_buffer;
            current_src_metaData_ptr = pull_data_info->src_metaData;
            current_empty_flag_ptr = pull_data_info->empty_flag;
            sbuf_index = 0;
          }
          else {
            current_src_buffer_ptr += pull_data_info->pdesc.desc.dataBufferPitch;
            current_src_metaData_ptr += pull_data_info->pdesc.desc.metaDataPitch/8;
            current_empty_flag_ptr += pull_data_info->pdesc.desc.emptyFlagPitch;
          }

        }
        else {

#ifdef DEBUG_WITH_EMULATOR
          while( *pull_data_info->src_flag != 0 ) OCPI::OS::sleep(1);
#endif

          // get the number of input buffers available
          OCPI::OS::uint32_t buffers_available = *pull_data_info->empty_flag;
      
          if ( buffers_available == 0 ) {
            return false;
          }

#ifndef NDEBUG
          printf("FPGA consumer has %d buffers available\n", 
                 buffers_available );
#endif

          return true;
        }
        return false;
      }

    private:
      unsigned int  sbuf_index;

    };
  }
}



OCPI::DataTransport::PullDataDriver* 
OCPI::DataTransport::Circuit::
createPullDriver( const OCPI::RDT::Descriptors& pdesc)
{

  PullDataDriver *pdd=NULL;
  SMBResources* sres=NULL;
  SMBResources* tres=NULL;
  if ( pdesc.type == OCPI::RDT::ProducerDescT ) {
    if ( pdesc.desc.oob.oep[0] != 0 ) {
      try {
        std::string s(pdesc.desc.oob.oep);
        sres =   XferFactoryManager::getFactoryManager().getSMBResources(s);
        if ( ! sres ) {
          throw OCPI::Util::EmbeddedException (
                                              OCPI::DataTransport::UNSUPPORTED_ENDPOINT, 
                                              pdesc.desc.oob.oep );
        }
      }
      catch(...) {
        throw OCPI::Util::EmbeddedException (
                                            OCPI::DataTransport::UNSUPPORTED_ENDPOINT, 
                                            pdesc.desc.oob.oep );
      }
    }

    tres = m_transport->getEndpointResources( pdesc.desc.oob.oep );
    PullDataInfo* pull_data_info = new PullDataInfo;
    // Get the local and remote vaddrs
    pull_data_info->src_buffer = (volatile OCPI::OS::uint8_t*)
      sres->sMemServices->map( pdesc.desc.dataBufferBaseAddr, 
                               pdesc.desc.dataBufferPitch == 0 ? 
                               pdesc.desc.dataBufferSize : pdesc.desc.dataBufferPitch * pdesc.desc.nBuffers );

    pull_data_info->src_metaData = (volatile OCPI::OS::uint64_t*)
      sres->sMemServices->map( pdesc.desc.metaDataBaseAddr,
                               pdesc.desc.metaDataPitch == 0 ? 
                               sizeof(OCPI::OS::uint64_t) : pdesc.desc.metaDataPitch * pdesc.desc.nBuffers );

    pull_data_info->src_flag = (volatile OCPI::OS::uint32_t*)
      sres->sMemServices->map( pdesc.desc.fullFlagBaseAddr,
                               pdesc.desc.fullFlagPitch == 0 ? 
                               sizeof(OCPI::OS::uint32_t) : pdesc.desc.fullFlagPitch * pdesc.desc.nBuffers );

    pull_data_info->empty_flag = (OCPI::OS::uint32_t*)
      sres->sMemServices->map( pdesc.desc.emptyFlagBaseAddr,
                               pdesc.desc.emptyFlagPitch == 0 ? 
                               sizeof(OCPI::OS::uint32_t) : pdesc.desc.emptyFlagPitch * pdesc.desc.nBuffers );
    memcpy(&pull_data_info->pdesc, &pdesc, sizeof(OCPI::RDT::Descriptors));
    pdd = new OCPI::DataTransport::PullDataDriverIP( pull_data_info );
  }
  else {

    printf("Circuit::createPullDriver() creating thread for Consumer(1)\n");
    if ( pdesc.desc.oob.oep[0] != 0 ) {
      try {
        std::string s(pdesc.desc.oob.oep);
        tres =   XferFactoryManager::getFactoryManager().getSMBResources(s);
        if ( ! tres ) {
          throw OCPI::Util::EmbeddedException (
                                              OCPI::DataTransport::UNSUPPORTED_ENDPOINT, 
                                              pdesc.desc.oob.oep );
        }
      }
      catch(...) {
        throw OCPI::Util::EmbeddedException (
                                            OCPI::DataTransport::UNSUPPORTED_ENDPOINT, 
                                            pdesc.desc.oob.oep );
      }
    }

    PullDataInfo* pull_data_info = new PullDataInfo;
    pull_data_info->empty_flag = (OCPI::OS::uint32_t*)
      tres->sMemServices->map( pdesc.desc.emptyFlagBaseAddr,
                               pdesc.desc.emptyFlagPitch == 0 ? 
                               sizeof(OCPI::OS::uint32_t) : pdesc.desc.emptyFlagPitch * pdesc.desc.nBuffers );

    pull_data_info->src_flag = (volatile OCPI::OS::uint32_t*)
      tres->sMemServices->map( pdesc.desc.fullFlagBaseAddr,
                               pdesc.desc.fullFlagPitch == 0 ? 
                               sizeof(OCPI::OS::uint32_t) : pdesc.desc.fullFlagPitch * pdesc.desc.nBuffers );
    memcpy(&pull_data_info->pdesc, &pdesc, sizeof(OCPI::RDT::Descriptors));
    pdd = new OCPI::DataTransport::PullDataDriverIP( pull_data_info );
  }

  return pdd;

}



/**********************************
 * Sets the feedback descriptor for this port.
 *********************************/
void 
OCPI::DataTransport::Circuit::
setFlowControlDescriptor( OCPI::DataTransport::Port* p, const OCPI::RDT::Descriptors& pdesc )
{

  // If the output port for this circuit is a shadow, now is a good time to 
  // make sure it gets initialized
  OCPI::DataTransport::Port* sport = m_outputPs->getPort(0);

#ifndef NDEBUG
  printf("<< Output port Full buffer flag = 0x%llx >> \n", (long long)pdesc.desc.emptyFlagValue );
  printf("<< Channel = %d\n", (int)((pdesc.desc.emptyFlagValue>>32) & 0xfff) );
#endif

#ifndef NDEBUG
  printf("Setting the shadow port ep to %s\n", pdesc.desc.oob.oep );
#endif

  if ( pdesc.desc.oob.oep[0] != 0 ) {
    // Port set ordinal 0 is always the output set, we need to update ours 
    static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[0])->getPortInfo(0)->real_location_string = 
      pdesc.desc.oob.oep;
  }
  
  // Update the output shadow port with the real ports descriptor.  This is only used at the 
  // input port to determine the role of the output and if needed, the pull data info.
  sport->getMetaData()->m_shadowPortDescriptor = pdesc;

  sport->update();

  //  BIG NOTE !!  We are cheating here, this needs to be fixed for DD&P!! This is only used in mode
  //  4 of ocpi but the port will not go ready until it is non zero
  sport->m_portDependencyData.offsets[0].outputOffsets.portSetControlOffset = 1;
  p->setFlowControlDescriptorInternal( pdesc );
  if ( m_maxPortOrd <= 1 ) {
    ocpiAssert(0);
  }

  if ( pdesc.desc.oob.oep[0] != 0 ) {
    try {
      std::string s(pdesc.desc.oob.oep);
      SMBResources* res =   XferFactoryManager::getFactoryManager().getSMBResources(s);
      if ( res ) {
        res->sMemServices->endpoint()->event_id = (int)((pdesc.desc.emptyFlagValue>>32) & 0xfff);
      }
    }
    catch(...) {
      printf("ERROR: Unable to set event id\n");
    }
  }

  // At this point we are setup, however if the output port is passive, we need to
  // Create a driver for it.
  if ( pdesc.role == OCPI::RDT::Passive ) {
    printf("Found a passive Output Port, creating a pull driver!!\n");
    PullDataDriver *pd = createPullDriver( pdesc );
    static_cast<OCPI::DataTransport::Port*>(p)->attachPullDriver( pd );
  }

  //  m_openCircuit = false;
}



void 
OCPI::DataTransport::Circuit::
reset()
{
  m_openCircuit = true;
}



/**********************************
 * Updates a port in the circuit
 *********************************/
OCPI::DataTransport::Port* 
OCPI::DataTransport::Circuit::
updatePort( OCPI::DataTransport::Port* p )
{

  PortSetMetaData* md = static_cast<PortSetMetaData*>
    (m_metaData->m_portSetMd[0]);

  PortMetaData* pmd = static_cast<PortMetaData*>(md->m_portMd[0]);
  pmd->init( );

  p->update();
  if ( m_maxPortOrd > 1 ) {
    ocpiDebug("Circuit is closed 4: id %x", getCircuitId());
    m_openCircuit = false;
  }

  return p;
}

/**********************************
 * Adds a port to the circuit
 *********************************/
OCPI::DataTransport::Port* 
OCPI::DataTransport::Circuit::
addPort( PortMetaData* pmd )
{

  // Create the new port
  PortSetMetaData* psmd;
  if ( pmd->output ) {
    psmd = static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[0]);
  }
  else {
    if ( m_metaData->m_portSetMd.size() < 2 ) {

      // Create a new port set description
      psmd = new PortSetMetaData(false, 1, new ParallelDataDistribution(), 1, 1024, m_metaData);

      // Add the port set to the connection
      m_metaData->addPortSet( psmd );
      update();

    }
    else {
      psmd = static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[1]);
    }
  }

  PortMetaData* spmd = psmd->addPort( pmd );
  PortSet * sps = static_cast<PortSet*>(m_inputPs[0]);
  Port* port = new Port( spmd, sps );
  sps->add( port );
  m_maxPortOrd++;
  ocpiDebug("Circuit is closed 5: id %x", getCircuitId());
  m_openCircuit = false;

  return port;
}


void
OCPI::DataTransport::Circuit::
getUserPortFlowControlDescriptor( OCPI::RDT::Descriptors * fb, unsigned int idx )
{
  PortSet * sps = static_cast<PortSet*>(m_inputPs[0]);
  if ( idx >= sps->getPortCount() ) {
    return;
  }

  Port* port = this->getOutputPortSet()->getPortFromIndex(idx);
  const char* ep = port->getEndpoint()->end_point.c_str();


  // If our output is in "ActiveFlowControl" mode, we do not need
  // to tell the input about our input shadow buffers since they
  // do not need to indicate when they have consumed data, we dont care
  // because we are not managing the transfers from our output to their
  // inputs.  Instead, we will send them the output descriptor so that 
  // they can "pull" data from us, and indicate when the data transfer has
  // completed.
  if ( getOutputPortSet()->getPort(0)->getMetaData()->m_descriptor.role == OCPI::RDT::ActiveFlowControl ) {
    *fb = getOutputPortSet()->getPort(0)->getMetaData()->m_descriptor;
    fb->type = OCPI::RDT::ProducerDescT;
  }
  else {
    *fb = sps->getPort(idx)->getMetaData()->m_shadowPortDescriptor;
    fb->type = OCPI::RDT::ConsumerFlowControlDescT;
  }
  strncpy(fb->desc.oob.oep,ep,127 );
  fb->desc.oob.oep[127] = 0;
}



/**********************************
 * Adds a port to the circuit
 *********************************/
void
OCPI::DataTransport::Circuit::
addInputPort( const OCPI::RDT::Descriptors& pdesc, const char* our_ep)
{

#ifndef NDEBUG
  printf("<< Input port Full buffer flag = 0x%llx >> \n", (long long)pdesc.desc.fullFlagValue );
  printf("<< Channel = %d\n", (int)((pdesc.desc.fullFlagValue>>32) & 0xfff) );
#endif

  // Create a new port set description
  PortSetMetaData* psmd;
  if ( m_metaData->m_portSetMd.getElementCount() <= 1 ) {
    psmd = new PortSetMetaData( false, 1, new ParallelDataDistribution(), m_metaData,
                                &pdesc, this->getCircuitId(), 1, 1, 1024, our_ep );
    m_metaData->addPortSet( psmd );
    update();
  }
  else {
    psmd = static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[1]);
    int ord = 1 + psmd->m_portMd.size();
    std::string nuls;
    PortMetaData* spsmd = 
      new PortMetaData( ord,false,our_ep,NULL,pdesc,getCircuitId(),psmd );
    psmd->addPort( spsmd );
    PortSet * sps = static_cast<PortSet*>(m_inputPs[0]);
    Port* sport = 
      new OCPI::DataTransport::Port( spsmd, static_cast<OCPI::DataTransport::PortSet*>(this->getInputPortSet(0)) );
    sps->add( sport );
    ord++;
  }

  ocpiAssert(pdesc.desc.oob.oep[0]);
  if ( pdesc.desc.oob.oep[0] ) {
    SMBResources* res = m_transport->getEndpointResources(  pdesc.desc.oob.oep );
    if ( res ) {
      res->sMemServices->endpoint()->event_id = (int)((pdesc.desc.fullFlagValue>>32) & 0xfff);
    }
  }

#if 0
  // This stuff is now deferred to "finalize"

  // Since we now have a complete circuit description, we need to tell the input ports
  // about us and about each other so they can complete their circuits
  ocpiAssert(fb);
  if ( !fb ) {
    while ( ! updateInputs() ) {
    }
  }
  else {
    getUserPortFlowControlDescriptor(fb,0);
    ocpiDebug("Circuit is closed 6: id %x", getCircuitId());
    m_openCircuit = false;
  }

  // Now that we have our ports created, we need to see if this is a "pull" protocol
  // If so, we need to create a pull driver for this port.
  if ( pdesc.role == OCPI::RDT::Passive ) {
    OCPI::DataTransport::PortSet* portset = (OCPI::DataTransport::PortSet*)this->getInputPortSet(0);
    OCPI::DataTransport::Port* port = portset->getPort(0);
    printf("Found a Passive Consumer Port !!\n");
    PullDataDriver* pd = createPullDriver( pdesc, port );
    static_cast<OCPI::DataTransport::Port*>(port)->attachPullDriver( pd );
  }
#endif
}


/**********************************
 * Determines if a circuit is ready to be initialized
 *********************************/
bool 
OCPI::DataTransport::Circuit::
ready()
{
  if (m_openCircuit)
    return false;
  if (m_ready)
    return true;

  OCPI::OS::uint32_t n,y;

  // We need to go through each port and determine if its buffers are complete
  OCPI::DataTransport::PortSet* s_ps = getOutputPortSet();
  for (n=0; n<s_ps->getPortCount(); n++ ) {
    OCPI::DataTransport::Port* sp = s_ps->getPort(n);
    if ( ! sp->ready() ) {
      return false;
    }
  }

  // Now do the input port sets
  for (y=0; y<this->getInputPortSetCount(); y++ ) {
                
    OCPI::DataTransport::PortSet* t_ps = getInputPortSet(y);
    for (n=0; n<t_ps->getPortCount(); n++ ) {
      OCPI::DataTransport::Port* tp = t_ps->getPort(n);
      if ( ! tp->ready() ) {
        return false;
      }
    }
  }
  m_ready = true;
  ocpiDebug("Circuit %x is READY", getCircuitId());
  initializeDataTransfers();
  return true;
}

#if 0
/**********************************
 * Updates an existing connection with a new port
 *********************************/
bool 
OCPI::DataTransport::Circuit::
updateConnection(OCPI::DataTransport::Port*,OCPI::OS::uint32_t)
{
  return ready();
}
#endif
/**********************************
 * Initialize transfers
 *********************************/
void 
OCPI::DataTransport::Circuit::
initializeDataTransfers()
{

// DEBUG CODE !!
#ifdef NDEBUG1
  ocpiAssert ( ready() );

  //Go through the input ports and make sure we have the buffers allocated
  for ( OCPI::OS::uint32_t n=0; n<this->m_connection->getInputPortSetCount(); n++ ) {
    OCPI::DataTransport::PortSet* portset = (OCPI::DataTransport::PortSet*)this->m_connection->getInputPortSet(n);
    for ( OCPI::OS::uint32_t y=0; y<portset->getPortCount(); y++ ) {
      OCPI::DataTransport::Port* port = portset->getPort(y);
      for ( OCPI::OS::uint32_t b=0; b<port->getNumBuffers(); b++ ) {

        InputBuffer* tb = port->getInputBuffer(b);
        if ( tb ) {
          tb->dumpOffsets();
          tb->update(0);
          tb->dumpOffsets();

        }
      }
    }
  }

  OCPI::DataTransport::Port* sport = m_outputPs->getPort(0);
  for ( OCPI::OS::uint32_t y=0; y<m_outputPs->getPortCount(); y++ ) {
    OCPI::DataTransport::Port* port = m_outputPs->getPort(y);
    for ( OCPI::OS::uint32_t b=0; b<port->getNumBuffers(); b++ ) {

      OutputBuffer* tb = port->getOutputBuffer(b);
      if ( tb ) {
        tb->dumpOffsets();
        tb->update(0);
        tb->dumpOffsets();
      }
    }
  }

#endif


  // When a circuit gets created, we have everything that we need including, distribution, 
  // partition, and late binding information within the connection class to be capable 
  // of creating all of the transfers templates that are required.  So here is where we
  // will perform the init.

  // Select the template generators
  if ( ! m_templatesGenerated && ! m_openCircuit ) {
    createCircuitTemplateGenerators();
    m_templatesGenerated = true;
  }

}

/**********************************
 * This method is used to select the transfer template generators that are needed 
 * within this circuit.
 *********************************/
void 
OCPI::DataTransport::Circuit::
createCircuitTemplateGenerators()
{
  OCPI::OS::uint32_t n;

  // We need a template generator per output/input pair
  OCPI::DataTransport::PortSet *output_port_set = m_outputPs;
  DataDistribution* output_dd = output_port_set->getDataDistribution();
  DataPartition* output_part = output_dd->getDataPartition();

  for ( n=0; n<getInputPortSetCount(); n++ ) {

    // Input info
    OCPI::DataTransport::PortSet *input_port_set = getInputPortSet(n);
    DataDistribution* input_dd = input_port_set->getDataDistribution();
    DataPartition* input_part = input_dd->getDataPartition();

    // Now get our controller
    bool whole = output_port_set->getDataDistribution()->getMetaData()->distType ==
      DataDistributionMetaData::parallel ? true : false;

    Port
      *oport = output_port_set->getPort(0),
      *iport = input_port_set->getPort(0);

    int32_t
      output_role = oport->getMetaData()->m_descriptor.role,
      input_role = iport->getMetaData()->m_descriptor.role;
#ifndef NDEBUG
    printf("output port role = %d\n", output_role);
    printf("input port role = %d\n", input_role);
#endif

    ocpiAssert( output_role < OCPI::RDT::MaxRole );
    ocpiAssert( input_role < OCPI::RDT::MaxRole );


    // Fixme for DD&P, we need to get "our" port when scaled to support "mixed" role transfers.
    TransferController* controller = m_transport->m_transportGlobal->m_transferControllers
      [output_dd->getMetaData()->distType]
      [input_dd->getMetaData()->distType]
      [output_part->getData()->dataPartType]
      [input_part->getData()->dataPartType]
      [output_port_set->getPort(0)->isShadow()]
      [output_role]
      [input_role]
      ->
      createController( output_port_set, input_port_set, whole);

    // Here is where we specialize the template generators
    m_transport->m_transportGlobal->m_templateGenerators
      [output_dd->getMetaData()->distType]
      [input_dd->getMetaData()->distType]
      [output_part->getData()->dataPartType]
      [input_part->getData()->dataPartType]
      [output_port_set->getPort(0)->isShadow()]
      [output_port_set->getPort(0)->getMetaData()->m_descriptor.role]
      [input_port_set->getPort(0)->getMetaData()->m_descriptor.role]
      ->
      createTemplates( m_transport, output_port_set, input_port_set, controller);
          
    // Attach the controller
    input_port_set->setTxController( controller );

    // For now we will add the last controler to the output port set.
    // This may change as the design evolves.  It is primarily used to
    // determine what the next output buffer should be
    output_port_set->setTxController( controller );

  }

}


void 
OCPI::DataTransport::Circuit::
checkIOZCopyQ()
{
  int total = 0;


  for ( OCPI::OS::uint32_t n=0; n<m_maxPortOrd; n++ ) {

    OCPI::OS::uint32_t n_queued = m_queuedInputOutputTransfers[n].getElementCount();
    total += n_queued;

    if ( n_queued == 0 ) {
      continue;
    }

    OCPI::DataTransport::Buffer* input_buffer = 
      static_cast<Buffer*>(m_queuedInputOutputTransfers[n].getEntry(0));

    if( input_buffer->m_zCopyPort->hasEmptyOutputBuffer() ) {
      QInputToOutput( input_buffer->m_zCopyPort, input_buffer, input_buffer->getLength() );
      m_queuedInputOutputTransfers[n].remove( input_buffer );
    }
  }
}


void 
OCPI::DataTransport::Circuit::
QInputToWaitForOutput( 
                      Port* out_port,
                      Buffer* input_buf,
                      OCPI::OS::uint32_t )
{

#ifdef DEBUG_L2
  printf("Queuing Input->Output buffer %d, owned by port %d, prepend = %d\n", 
         src_buf->getTid(), src_buf->getPort()->getPortId(), prepend );
#endif

  // Attach the input buffer to the output port
  ocpiAssert( input_buf->m_zCopyPort == NULL );
  input_buf->m_zCopyPort = out_port;
  m_queuedInputOutputTransfers[out_port->getPortId()].insert(input_buf);

}


void 
OCPI::DataTransport::Circuit::
QInputToOutput( 
               Port* out_port,
               Buffer* input_buf,
               OCPI::OS::uint32_t )
{

  // Get the next available output buffer
  ocpiAssert( out_port->hasEmptyOutputBuffer() );

  Buffer* tbuf = out_port->getNextEmptyOutputBuffer();
  ocpiAssert( tbuf );
  ocpiAssert( tbuf->m_attachedZBuffer == NULL );
  Buffer* cbuf = static_cast<Buffer*>(tbuf);

  memcpy((void*)&(tbuf->getMetaData()->ocpiMetaDataWord),
	 (void*)&(input_buf->getMetaData()->ocpiMetaDataWord),sizeof(RplMetaData));

  // Modify the output port transfer to point to the input buffer output
  OCPI::DataTransport::PortSet* ps = static_cast<OCPI::DataTransport::PortSet*>(out_port->getPortSet());
  ps->getTxController()->modifyOutputOffsets( tbuf, input_buf, false);

  // The input is already a zcopy so no work here
  if ( !cbuf->m_zeroCopyFromBuffer  ) {
    tbuf->m_attachedZBuffer = input_buf;
    input_buf->m_attachedZBuffer = tbuf;
  }
  else {
    input_buf->m_zCopyPort = NULL;
    tbuf->m_zCopyPort = NULL;
  }


  // Now start the transfer if possible
  if ( canTransferBuffer( tbuf ) ) {
    startBufferTransfer( tbuf );
  }
  else {
    queTransfer( tbuf );
  }

}


// This method sends a workers input buffer to the input of one of its output ports.
void OCPI::DataTransport::Circuit::
sendZcopyInputBuffer( 
                     Port* out_port,
                     Buffer* input_buf,
                     OCPI::OS::uint32_t len )
{
  QInputToWaitForOutput( out_port, input_buf, len);
}

OCPI::OS::uint32_t 
OCPI::DataTransport::Circuit::
checkQueuedTransfers()
{

  OCPI::OS::uint32_t total=0;
        
  for ( OCPI::OS::uint32_t n=0; n<m_maxPortOrd; n++ ) {
                
    OCPI::OS::uint32_t n_queued = m_queuedTransfers[n].getElementCount();
    total += n_queued;
                
#ifdef DEBUG_L2
    printf("In checkQueuedTransfers, there are %d tranfers queued\n", n_queued);
#endif
    if ( n_queued == 0 ) {
      continue;
    }
#ifdef QUE_CHECK
    // que integrity check
    Buffer* b[MAX_PORT_COUNT];
    for (OCPI::OS::int32_t i=0; i<n_queued; i++ ) {
      b[i] = 
        static_cast<Buffer*>(get_entry(&m_queuedTransfers, i));
    }
    if ( (n_queued == 2 ) && (b[0] == b[1]) ) {
      printf("ERROR **** que contains multiple copies of the same buffer !!!!\n");
      Sleep( 2000 );
    }
#endif
                
#ifdef DEBUG_L2
    printf("******* There are %d queued transfers\n", n_queued);
#endif
                
    Buffer* buffer = 
      static_cast<Buffer*>(m_queuedTransfers[n].getEntry(0));
                
#ifdef DEBUG_L2
    printf("buffer is owned by port %d\n", buffer->getPort()->getPortId() );
#endif
                
    if ( canTransferBuffer( buffer, true ) ) {
                        
#ifdef DEBUG_L2
      printf("Starting queued transfer, buffer id = %d, (%d) transfers total\n", 
             buffer->getTid(), n_queued );
#endif
                        
      m_queuedTransfers[n].remove( buffer );
      startBufferTransfer( buffer );
      total--;
    }
  }

  // Now check to see if there are any I/O ZCopies to deal with
  checkIOZCopyQ();

  return total;
}



#ifdef DONE
void
OCPI::DataTransport::Circuit:
checkLameTransfers()
{

  // Check passive ports
  // If we are a consumer, we need to do a remote poll and pull data
  // If we are a producer, we need to do a remote poll to determine if there
  // are any empty buffers available

  // Check active flow control 
  // If we are a consumer, we need to check our local flag and pull data from the producer
  // If we are a producer, we need to do nothing.

}
#endif






/**********************************
 * This method causes the buffer to be transfered to the
 * inputs if a transfer can take place.  If not, the circuit is
 * responsible for queing the request.
 *
 * This method returns -1 if an error occurs.
 *********************************/
void 
OCPI::DataTransport::Circuit::
startBufferTransfer( Buffer* src_buf )
{

  if ( m_openCircuit ) {
    return;
  }

#ifdef DEBUG_L2
  printf("*** Transfering, buffer id = %d, from port %d\n", 
         src_buf->getTid(), src_buf->getPort()->getPortId() );
#endif

  Buffer *buffer = static_cast<Buffer*>(src_buf);

  if ( !buffer->getPort()->isShadow() && buffer->getMetaData()->broadCast == 1 ) {
    broadcastBuffer( src_buf );
    return;
  }


#ifdef QUE_CHECK
  static int seq=1;
  if ( ((Buffer*)src_buf)->sequence() != seq ) {
    printf("*** ERROR **** found a buffer out of sequence, buf(%d), seq(%d)!!!\n",
           ((Buffer*)src_buf)->sequence(), seq);
    Sleep( 2000 );
  }
  seq++;
#endif

  // We need to start a transfer for each input port set
  for ( OCPI::OS::uint32_t n=0; n<this->getQualifiedInputPortSetCount(); n++ ) {
    OCPI::DataTransport::PortSet* t_port = this->getQualifiedInputPortSet(n);
    if ( (t_port->getTxController()->produce( src_buf )) ) {
      this->queTransfer( src_buf, true );
    }

  }
  static_cast<Buffer*>(src_buf)->setInUse( false );

  // For sequention connection distribution
  m_lastPortSet++;
  m_lastPortSet = m_lastPortSet%getInputPortSetCount();

}



/***********************************
 * This method braodcasts a buffer to all of the input ports in 
 * in the circuit.
 *********************************/
void 
OCPI::DataTransport::Circuit::
broadcastBuffer( Buffer* src_buf )
{
  if ( m_openCircuit ) {
    return;
  }

#ifdef DEBUG_L2
  printf("Broadcasting, buffer id = %d\n", src_buf->getTid() );
#endif

  Buffer *buffer = static_cast<Buffer*>(src_buf);
  buffer->getMetaData()->broadCast = 1;

  // We need to start a transfer for each input port set
  for ( OCPI::OS::uint32_t n=0; n<this->getInputPortSetCount(); n++ ) {
    OCPI::DataTransport::PortSet* t_port = this->getInputPortSet(n);
    t_port->getTxController()->produce( src_buf, true );
  }
  static_cast<Buffer*>(src_buf)->setInUse( false );

  // For sequention connection distribution
  m_lastPortSet++;
  m_lastPortSet = m_lastPortSet%getInputPortSetCount();

}


/**********************************
 * Que the transfer for this buffer for later.
 *********************************/
void 
OCPI::DataTransport::Circuit::
queTransfer( Buffer* src_buf, bool prepend )
{

#ifdef DEBUG_L2
  printf("Queuing buffer %d, owned by port %d, prepend = %d\n", 
         src_buf->getTid(), src_buf->getPort()->getPortId(), prepend );
#endif

  if ( ! prepend ) {
    m_queuedTransfers[src_buf->getPort()->getPortId()].insert(src_buf);
  }
  else {
    m_queuedTransfers[src_buf->getPort()->getPortId()].prepend(src_buf);
  }


#ifdef QUE_CHECK
  // debug
  OCPI::OS::int32_t n_queued = get_nentries(&m_queuedTransfers);
  Buffer* b1  = 
    static_cast<Buffer*>(get_entry(&m_queuedTransfers, 0));
  Buffer* b2  = 
    static_cast<Buffer*>(get_entry(&m_queuedTransfers, 1));
  if ( b1 && b2 ) {
    if ( b1->sequence() >= b2->sequence() ) {
      printf("**** ERROR !! buffers are out of sequence in list\n");
      Sleep( 2000 );
    }
  }
#endif

}

/**********************************
 * These methods are used to control the input port sets that are used to 
 * receive data based upon the circuits data distribution type.  If the circuits
 * DD type is parallel, then 
 *********************************/
OCPI::OS::uint32_t 
OCPI::DataTransport::Circuit::
getQualifiedInputPortSetCount( bool queued )
{
  if ( queued ) {
    return getInputPortSetCount();
  }

  int ps_count;
  DataDistribution* dd = m_metaData->dataDistribution;
  if ( dd->getMetaData()->distType == DataDistributionMetaData::parallel ) {
    ps_count = this->getInputPortSetCount();
  }
  else {
    ps_count = 1;
  }
        
  // If we have a sequential distribution type, we can only send to one port set
  return ps_count;
}


OCPI::DataTransport::PortSet* 
OCPI::DataTransport::Circuit::
getQualifiedInputPortSet(OCPI::OS::uint32_t n, bool queued)
{

  if ( queued ) {
    return getInputPortSet(n);
  }

  OCPI::DataTransport::PortSet* ps = NULL;
  DataDistribution* dd = m_metaData->dataDistribution;
  if ( dd->getMetaData()->distType == DataDistributionMetaData::parallel ) {
    ps = getInputPortSet(n);
  }
  else {
#ifndef NDEBUG
    printf("Sending to ps %d ONLY\n", m_lastPortSet );
#endif
    ps = getInputPortSet( m_lastPortSet );

#ifndef NDEBUG
    printf("ps = %p\n", ps );
#endif

  }

  return ps;
}



/**********************************
 * This method is reponsible for determining if a buffer can be 
 * transfered.  If not, the caller is responsible for queuing the
 * transfer.
 *********************************/
bool 
OCPI::DataTransport::Circuit::
canTransferBuffer( Buffer* src_buf, bool queued_transfer )
{
  if ( m_openCircuit ) {
    return true;
  }

  // We need to ask the transfer controller if it is OK to
  // to start a transfer while there are other transfers in the queue
  if ( ! queued_transfer ) { 

    unsigned int pid = src_buf->getPort()->getPortId();
    if ( pid  >= MAX_PCONTRIBS) {
      printf("ERROR, attempt to transfer a bad buffer \n");
      return false;
    }

    OCPI::OS::uint32_t n_queued = m_queuedTransfers[src_buf->getPort()->getPortId()].getElementCount();
    if ( n_queued ) {
      for ( OCPI::OS::uint32_t n=0; n<this->getQualifiedInputPortSetCount(queued_transfer); n++ ) {
        OCPI::DataTransport::PortSet* t_port = this->getQualifiedInputPortSet(n, queued_transfer);
        if ( ! t_port->getTxController()->canTransferBufferWhileOthersAreQueued()) {
          return false;
        }
      }
    }
  }

  // If this is either a queued transfer, or the buffer is being broadcast, we
  // need to make sure that ALL inputs are available regardless of the transfer 
  // mode.
  Buffer* b = static_cast<Buffer*>(src_buf);
  bool worst_case;
  if (src_buf->getPort()->isShadow() ) {
    worst_case = false;
  }
  else {
    worst_case = queued_transfer | (b->getMetaData()->broadCast == 1) ? true : false;
  }

  // We need to go to each input port set and determine if we can procuce
  for ( OCPI::OS::uint32_t n=0; n<this->getQualifiedInputPortSetCount(worst_case); n++ ) {
    OCPI::DataTransport::PortSet *input_port_set = getQualifiedInputPortSet(n, worst_case);
    if ( ! input_port_set->getTxController()->canProduce( src_buf ) ) {
      return false;
    }
  }

  OCPI_EMIT_CAT_("Input Buffer Ready",OCPI_EMIT_CAT_TUNING,OCPI_EMIT_CAT_TUNING_DP);

  return true;
}


#ifdef DEBUG_CIRCUIT
void 
OCPI::DataTransport::Circuit::
debugDump()
{
  OCPI::OS::uint32_t n;
  printf("\n\nDebug dump for circuit\n");
  printf("  Circuit is %s\n", this->m_openCircuit ? "open" : "closed" );
  printf("  There are %d ports in the output port set\n", getOutputPortSet()->getPortCount() );
  for ( n=0; n<getOutputPortSet()->getPortCount(); n++ ) {
    OCPI::DataTransport::Port* port = static_cast<OCPI::DataTransport::Port*>(this->getOutputPortSet()->getPortFromIndex(n));
    port->debugDump();
  }
  printf("  \nThere are %d input port sets\n", getInputPortSetCount() );
  for ( n=0; n<getInputPortSetCount(); n++ ) {
    printf("  Input port set %d, has %d ports\n",n,getInputPortSet(n)->getPortCount());
    for ( OCPI::OS::uint32_t m=0; m<getInputPortSet(n)->getPortCount(); m++ ) {
      OCPI::DataTransport::Port* port = static_cast<OCPI::DataTransport::Port*>(this->getInputPortSet(n)->getPortFromIndex(m));
      port->debugDump();
    }
  }
}
#else
void 
OCPI::DataTransport::Circuit::
debugDump(){}
#endif


void 
Circuit::
update()
{
  unsigned int n;
  for ( n=m_portsets_init; n<m_metaData->m_portSetMd.size(); n++ ) {
    OCPI::DataTransport::PortSet* ps=NULL;
    try {
      ps = 
        new OCPI::DataTransport::PortSet( static_cast<PortSetMetaData*>(m_metaData->m_portSetMd[n]),
                                         this );
    }
    catch ( ... ) {
      throw;
    }
    this->addPortSet( ps );
    m_portsets_init++;
  }
}

void 
Circuit::
addPortSet( PortSet* port_set )
{
  if ( port_set->isOutput() ) {
    m_outputPs = port_set;
  }
  else {
    m_inputPs.push_back(port_set);
  }
}

void 
Circuit::
attach()
{
  m_ref_count++;  
}

void 
Circuit::
release()
{
  m_ref_count--;
  if (   m_ref_count == 0 ) {
    m_transport->deleteCircuit( getCircuitId() );
  }
}
