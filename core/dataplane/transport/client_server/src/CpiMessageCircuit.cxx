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

#include <CpiMessageCircuit.h>
#include <CpiBuffer.h>
#include <CpiOsAssert.h>
#include <CpiPort.h>

using namespace CPI::DataTransport;
using namespace DataTransport::Interface;


/**********************************
 * Constructor
 **********************************/
MessageCircuit::MessageCircuit(
			       CPI::DataTransport::Transport* transport,
			       CPI::DataTransport::Circuit* send,		// In - send circuit
			       CPI::DataTransport::Circuit* rcv		// In - recieve circuit
			       )
  :m_transport(transport),m_send(send),m_rcv(rcv),m_full_buffer(NULL)
{
  m_rcv_port = rcv->getInputPortSet(0)->getPort(0);
  m_send_port = send->getOutputPortSet()->getPort(0);
  if ( m_send_port->isShadow() ) {
#ifndef NDEBUG
    printf("*** &&& Rcv port backwards !!!!\n");
#endif
    m_send = rcv;
    m_rcv = send;
    m_rcv_port = m_rcv->getInputPortSet(0)->getPort(0);
    m_send_port = m_send->getOutputPortSet()->getPort(0);
  }
}


MessageCircuit::~MessageCircuit()
{
  m_transport->deleteCircuit(m_rcv->getCircuitId());
  m_transport->deleteCircuit(m_send->getCircuitId());
}

CPI::DataTransport::Buffer* MessageCircuit::getSendMessageBuffer()
{
  return m_send_port->getNextEmptyOutputBuffer();
}




/**********************************
 *  Send a message
 **********************************/
void MessageCircuit::sendMessage( CPI::DataTransport::Buffer* buffer, unsigned int length )
{
  buffer->setNumberOfBytes2Transfer( length );

  // Make sure we get thread time
  m_send->checkQueuedTransfers();
  if ( m_send->canTransferBuffer( buffer, false ) ) {
    m_send->startBufferTransfer( buffer );
  }
  else {
    m_send->queTransfer( buffer );
  }
}


/**********************************
 *  Determines if a message is available 
 *
 *  returns the number of messages.
 **********************************/
bool MessageCircuit::messageAvailable()
{
  if ( m_full_buffer ) {
    return true;
  }
  m_full_buffer = m_rcv_port->getNextFullInputBuffer();
  return m_full_buffer ? true : false;
}


/**********************************
 *  Get a message
 **********************************/
CPI::DataTransport::Buffer* MessageCircuit::getNextMessage()
{
  CPI::DataTransport::Buffer *r_buf=NULL;

  if ( m_full_buffer ) {
    r_buf = m_full_buffer;
    m_full_buffer = NULL;
  }
  else {
    r_buf = m_rcv_port->getNextFullInputBuffer();
  }

  static bool one_time_warning = 0;
  if ( m_rcv->getStatus() == Circuit::Disconnecting ) {
    if ( ! one_time_warning ) {
      printf("WARNING: Circuit is disconnecting\n");
      one_time_warning = 1;
    }

  }

  return r_buf;
}

void MessageCircuit::freeMessage( CPI::DataTransport::Buffer* msg )
{
  m_rcv_port->advance( msg );
}
