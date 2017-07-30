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

#include <OcpiOsMutex.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerOStreamAdapter.h>
#include <iostream>

/*
 * ----------------------------------------------------------------------
 * OStreamAdapter
 * ----------------------------------------------------------------------
 */

OCPI::Logger::OStreamAdapter::
OStreamAdapterBuf::OStreamAdapterBuf (Logger * logger,
                                      unsigned short logLevel,
                                      const std::string & producerName,
                                      bool adopt)
  : m_first (true),
    m_adopted (adopt),
    m_logger (logger),
    m_out (logger->m_buf),
    m_logLevel (logLevel),
    m_producerName (producerName)
{
}

OCPI::Logger::OStreamAdapter::
OStreamAdapterBuf::~OStreamAdapterBuf ()
{
  if (!m_first) {
    m_out.pubsync ();
  }

  if (m_adopted) {
    delete m_logger;
  }
}

int
OCPI::Logger::OStreamAdapter::
OStreamAdapterBuf::sync ()
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
OCPI::Logger::OStreamAdapter::
OStreamAdapterBuf::overflow (int_type c)
{
  if (c == '\n') {
    return c;
  }

  m_selfLock.lock ();

  if (m_first) {
    m_out.setLogLevel (m_logLevel);
    m_out.setProducerName (m_producerName.c_str());
    m_first = false;
  }

  if (traits_type::eq_int_type (c, traits_type::eof())) {
    m_selfLock.unlock ();
    return traits_type::not_eof (c);
  }

  std::streambuf::int_type res = m_out.sputc (traits_type::to_char_type (c));
  m_selfLock.unlock ();
  return res;
}

std::streamsize
OCPI::Logger::OStreamAdapter::
OStreamAdapterBuf::xsputn (const char * data,
                           std::streamsize count)
{
  std::streamsize total = 0;
  std::streamsize idx;

  if (traits_type::eq_int_type (overflow (traits_type::eof()), traits_type::eof())) {
    return 0;
  }

  while (count) {
    for (idx=0; data[idx] != '\n' && idx<count; idx++) {
      // empty
    }

    if (idx) {
      if (m_out.sputn (data, idx) != idx) {
        return total;
      }
      data += idx;
      count -= idx;
      total += idx;
    }

    while (count && *data == '\n') {
      count--;
      data++;
      total++;
    }
  }

  return total;
}

OCPI::Logger::OStreamAdapter::
OStreamAdapter (Logger & log,
                unsigned short logLevel,
                const std::string & producerName)
  : std::ostream (&m_buf),
    m_buf (&log, logLevel, producerName, false)
{
}

OCPI::Logger::OStreamAdapter::
OStreamAdapter (Logger * log,
                unsigned short logLevel,
                const std::string & producerName,
                bool adopt)
  : std::ostream (&m_buf),
    m_buf (log, logLevel, producerName, adopt)
{
}

OCPI::Logger::OStreamAdapter::
~OStreamAdapter ()
{
}
