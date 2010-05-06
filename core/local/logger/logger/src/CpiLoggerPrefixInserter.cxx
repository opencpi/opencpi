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
#include <CpiLoggerPrefixInserter.h>
#include <iostream>
#include <string>

/*
 * ----------------------------------------------------------------------
 * PrefixInserter: add a prefix to each log message
 * ----------------------------------------------------------------------
 */

CPI::Logger::PrefixInserter::
PrefixInserterBuf::PrefixInserterBuf (Logger * logger,
                                      const std::string & pfx,
                                      bool adopt)
  : m_first (true),
    m_adopted (adopt),
    m_logger (logger),
    m_out (logger->m_buf),
    m_prefix (pfx)
{
}

CPI::Logger::PrefixInserter::
PrefixInserterBuf::~PrefixInserterBuf ()
{
  if (!m_first) {
    m_out.pubsync ();
  }

  if (m_adopted) {
    delete m_logger;
  }
}

void
CPI::Logger::PrefixInserter::
PrefixInserterBuf::setLogLevel (unsigned short logLevel)
{
  m_out.setLogLevel (logLevel);
}

void
CPI::Logger::PrefixInserter::
PrefixInserterBuf::setProducerId (const char * producerId)
{
  m_out.setProducerId (producerId);
}

void
CPI::Logger::PrefixInserter::
PrefixInserterBuf::setProducerName (const char * producerName)
{
  m_out.setProducerName (producerName);
}

void
CPI::Logger::PrefixInserter::
PrefixInserterBuf::setPrefix (const std::string & pfx)
{
  m_prefix = pfx;
}

int
CPI::Logger::PrefixInserter::
PrefixInserterBuf::sync ()
{
  int res;

  if (!m_first) {
    m_first = true;
    res = m_out.pubsync ();
  }
  else {
    res = 0;
  }

  return res;
}

std::streambuf::int_type
CPI::Logger::PrefixInserter::
PrefixInserterBuf::overflow (int_type c)
{
  if (m_first) {
    std::streamsize pp = m_out.sputn (m_prefix.data(), m_prefix.length());
    if (pp < 0 || static_cast<std::string::size_type> (pp) != m_prefix.length()) {
      return traits_type::eof();
    }

    m_first = false;
  }

  if (traits_type::eq (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  return m_out.sputc (traits_type::to_char_type (c));
}

std::streamsize
CPI::Logger::PrefixInserter::
PrefixInserterBuf::xsputn (const char * data, std::streamsize count)
{
  if (m_first) {
    overflow (traits_type::eof ());
  }

  return m_out.sputn (data, count);
}

CPI::Logger::PrefixInserter::
PrefixInserter (Logger & log, const std::string & prefix)
  : Logger (m_obuf),
    m_obuf (&log, prefix, false)
{
}

CPI::Logger::PrefixInserter::
PrefixInserter (Logger * log,
                const std::string & prefix,
                bool adopt)
  : Logger (m_obuf),
    m_obuf (log, prefix, adopt)
{
}

CPI::Logger::PrefixInserter::~PrefixInserter ()
{
}

void
CPI::Logger::PrefixInserter::setPrefix (const std::string & prefix)
{
  m_obuf.setPrefix (prefix);
}

