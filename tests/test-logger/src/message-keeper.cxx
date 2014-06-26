
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


#include <string>

#include "OcpiLoggerLogger.h"

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

  if (traits_type::eq_int_type (c, traits_type::eof())) {
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
