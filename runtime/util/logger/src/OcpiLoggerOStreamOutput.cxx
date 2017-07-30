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

#include <OcpiOsAssert.h>
#include <OcpiOsMutex.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerOStreamOutput.h>
#include <iostream>

/*
 * ----------------------------------------------------------------------
 * OStreamOutput: log to an std::ostream
 * ----------------------------------------------------------------------
 */

OCPI::Logger::OStreamOutput::OStreamOutputBuf::
OStreamOutputBuf (std::ostream * out, bool adopt)
  : m_first (true),
    m_locked (false),
    m_logLevel (0),
    m_out (out),
    m_adopted (adopt)
{
}

OCPI::Logger::OStreamOutput::OStreamOutputBuf::~OStreamOutputBuf ()
{
  if (!m_first) {
    m_out->flush ();
  }

  if (m_locked) {
    m_lock.unlock ();
  }

  if (m_adopted) {
    delete m_out;
  }
}

void
OCPI::Logger::OStreamOutput::OStreamOutputBuf::setLogLevel (unsigned short logLevel)
{
  m_lock.lock ();
  m_locked = true;
  m_logLevel = logLevel;
  m_producerName.clear ();
  ocpiAssert (m_first);
}

void
OCPI::Logger::OStreamOutput::OStreamOutputBuf::setProducerId (const char * producerId)
{
  m_producerId = producerId;
}

void
OCPI::Logger::OStreamOutput::OStreamOutputBuf::setProducerName (const char * producerName)
{
  ocpiAssert (m_locked);
  ocpiAssert (m_first);
  m_producerName = producerName;
}

int
OCPI::Logger::OStreamOutput::OStreamOutputBuf::sync ()
{
  if (!m_first) {
    ocpiAssert (m_locked);
    *m_out << std::endl;
    m_first = true;
  }

  if (m_locked) {
    m_locked = false;
    m_lock.unlock ();
  }

  return (m_out->good() ? 0 : -1);
}

std::streambuf::int_type
OCPI::Logger::OStreamOutput::OStreamOutputBuf::overflow (int_type c)
{
  ocpiAssert (m_locked);

  if (m_first) {
    m_first = false;

    if (m_producerId.length()) {
      *m_out << "(" << m_producerId << ") ";
    }

    if (m_producerName.length()) {
      *m_out << m_producerName << ": ";
    }

    switch (m_logLevel) {
    case 1: *m_out << "SECURITY ALARM: "; break;
    case 2: *m_out << "FAILURE ALARM: "; break;
    case 3: *m_out << "DEGRADED ALARM: "; break;
    case 4: *m_out << "EXCEPTION ERROR: "; break;
    case 5: *m_out << "FLOW CONTROL ERROR: "; break;
    case 6: *m_out << "RANGE ERROR: "; break;
    case 7: *m_out << "USAGE ERROR: "; break;
    case 8: *m_out << "Administrative Event: "; break;
    case 9: *m_out << "Statistic Report: "; break;
    }

    if (m_logLevel >= 10 && m_logLevel < 26) {
      *m_out << "Debug(" << (m_logLevel-9) << "): ";
    }
    else if (m_logLevel >= 26) {
      *m_out << "LogLevel(" << m_logLevel << "): ";
    }
  }

  if (traits_type::eq_int_type (c, traits_type::eof())) {
    if (!m_out->good()) {
      return traits_type::eof ();
    }

    return traits_type::not_eof (c);
  }

  m_out->put (traits_type::to_char_type (c));

  if (!m_out->good()) {
    return traits_type::eof();
  }

  return c;
}

std::streamsize
OCPI::Logger::OStreamOutput::OStreamOutputBuf::xsputn (const char * data, std::streamsize count)
{
  ocpiAssert (m_locked);

  if (m_first) {
    overflow (traits_type::eof());
  }

  m_out->write (data, count);
  return (m_out->good()) ? count : 0;
}

OCPI::Logger::OStreamOutput::OStreamOutput (std::ostream & out)
  : Logger (m_obuf),
    m_obuf (&out, false)
{
}

OCPI::Logger::OStreamOutput::OStreamOutput (std::ostream * out, bool adopt)
  : Logger (m_obuf),
    m_obuf (out, adopt)
{
}

OCPI::Logger::OStreamOutput::~OStreamOutput ()
{
}

