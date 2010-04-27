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

#include <CpiLoggerLogger.h>
#include <string>
#include "MessageKeeper.h"

MessageKeeperOutput::MessageKeeperOutputBuf::
MessageKeeperOutputBuf ()
  : m_first (true)
{
}

MessageKeeperOutput::MessageKeeperOutputBuf::
~MessageKeeperOutputBuf ()
{
  if (!m_first) {
    sync ();
  }
}

void
MessageKeeperOutput::MessageKeeperOutputBuf::
setLogLevel (unsigned short logLevel)
{
  m_logLevel = logLevel;
  m_producerName.clear ();
}

void
MessageKeeperOutput::MessageKeeperOutputBuf::
setProducerId (const char * producerId)
{
  m_producerId = producerId;
}

void
MessageKeeperOutput::MessageKeeperOutputBuf::
setProducerName (const char * producerName)
{
  m_producerName = producerName;
}

unsigned short
MessageKeeperOutput::MessageKeeperOutputBuf::
getLogLevel () const
{
  return m_logLevel;
}

std::string
MessageKeeperOutput::MessageKeeperOutputBuf::
getProducerId () const
{
  return m_producerId;
}

std::string
MessageKeeperOutput::MessageKeeperOutputBuf::
getProducerName () const
{
  return m_producerName;
}

std::string
MessageKeeperOutput::MessageKeeperOutputBuf::
getMessage () const
{
  return m_logMessage;
}

int
MessageKeeperOutput::MessageKeeperOutputBuf::
sync ()
{
  m_first = true;
  return 0;
}

std::streambuf::int_type
MessageKeeperOutput::MessageKeeperOutputBuf::
overflow (int_type c)
{
  if (m_first) {
    m_first = false;
    m_logMessage.clear ();
  }

  if (traits_type::eq (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  m_logMessage += traits_type::to_char_type (c);
  return c;
}

std::streamsize
MessageKeeperOutput::MessageKeeperOutputBuf::
xsputn (const char * data, std::streamsize count)
{
  if (m_first) {
    overflow (traits_type::eof());
  }

  m_logMessage.append (data, count);
  return count;
}

MessageKeeperOutput::MessageKeeperOutput ()
  : Logger (m_obuf)
{
}

MessageKeeperOutput::~MessageKeeperOutput ()
{
}

unsigned short
MessageKeeperOutput::getLogLevel () const
{
  return m_obuf.getLogLevel ();
}

std::string
MessageKeeperOutput::getProducerId () const
{
  return m_obuf.getProducerId ();
}

std::string
MessageKeeperOutput::getProducerName () const
{
  return m_obuf.getProducerName ();
}

std::string
MessageKeeperOutput::getMessage () const
{
  return m_obuf.getMessage ();
}
