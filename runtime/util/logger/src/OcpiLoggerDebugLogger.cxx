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
#include <OcpiLoggerDebugLogger.h>
#include <streambuf>
#include <iostream>
#include <string>
#include <map>

namespace {
  typedef std::map<std::string, unsigned int> StringToUIntMap;
  bool g_debugAllProducerNames;
  unsigned int g_debugAllVerbosity;
  StringToUIntMap g_debugProducerNames;
  OCPI::OS::Mutex g_debugProducerNamesLock;
}

/*
 * Enable/Disable debugging
 */

void
OCPI::Logger::debug (const std::string & producerName, unsigned int v)
{
  g_debugProducerNamesLock.lock ();

  if (producerName == "None") {
    g_debugAllProducerNames = false;
    g_debugProducerNames.clear ();
  }
  else if (producerName == "All") {
    if (v) {
      g_debugAllProducerNames = true;
      g_debugAllVerbosity = v;
    }
    else {
      g_debugAllProducerNames = false;
    }
  }
  else {
    if (v) {
      g_debugProducerNames[producerName] = v;
    }
    else {
      StringToUIntMap::iterator it = g_debugProducerNames.find (producerName);

      if (it != g_debugProducerNames.end()) {
        g_debugProducerNames.erase (it);
      }
    }
  }

  g_debugProducerNamesLock.unlock ();
}

/*
 * DebugLogger when debugging enabled
 */

OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::DebugLoggerBuf (Logger::LogBuf & other)
  : m_first (true),
    m_locked (false),
    m_out (other)
{
}

OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::~DebugLoggerBuf ()
{
  if (!m_first) {
    m_out.pubsync ();
  }
}

void
OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setLogLevel (unsigned short)
{
  ocpiAssert (0);
}

void
OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setProducerId (const char * producerId)
{
  m_out.setProducerId (producerId);
}

void
OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setProducerName (const char * producerName)
{
  m_lock.lock ();
  ocpiAssert (m_first);
  m_locked = true;
  m_producerName = producerName;
  m_verbosity = 1;

  g_debugProducerNamesLock.lock ();
  if (g_debugAllProducerNames) {
    m_shouldprint = true;
  }
  else {
    StringToUIntMap::iterator it = g_debugProducerNames.find (m_producerName);
    if (it == g_debugProducerNames.end()) {
      m_shouldprint = false;
    }
    else {
      m_shouldprint = true;
    }
  }
  g_debugProducerNamesLock.unlock ();
}

void
OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setVerbosity (unsigned int v)
{
  ocpiAssert (m_locked);
  ocpiAssert (m_first);

  m_verbosity = v;

  g_debugProducerNamesLock.lock ();

  StringToUIntMap::iterator it = g_debugProducerNames.find (m_producerName);

  if (it != g_debugProducerNames.end()) {
    if ((*it).second >= m_verbosity) {
      m_shouldprint = true;
    }
    else if (g_debugAllProducerNames && g_debugAllVerbosity >= m_verbosity) {
      m_shouldprint = true;
    }
    else {
      m_shouldprint = false;
    }
  }
  else {
    if (g_debugAllVerbosity >= m_verbosity) {
      m_shouldprint = true;
    }
    else {
      m_shouldprint = false;
    }
  }

  g_debugProducerNamesLock.unlock ();
}

int
OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::sync ()
{
  ocpiAssert (m_locked);

  int res;

  if (!m_first) {
    m_first = true;
    res = m_out.pubsync ();
  }
  else {
    res = 0;
  }

  m_locked = false;
  m_lock.unlock ();
  return res;
}

std::streambuf::int_type
OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::overflow (int_type c)
{
  ocpiAssert (m_locked);

  if (!m_shouldprint) {
    if (traits_type::eq_int_type (c, traits_type::eof())) {
      return traits_type::not_eof (c);
    }

    return c;
  }

  if (m_first) {
    m_out.setLogLevel ((unsigned short)(9 + m_verbosity));
    m_out.setProducerName (m_producerName.c_str());
    m_first = false;
  }

  if (traits_type::eq_int_type (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  return m_out.sputc (traits_type::to_char_type (c));
}

std::streamsize
OCPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::xsputn (const char * data, std::streamsize count)
{
  ocpiAssert (m_locked);

  if (!m_shouldprint) {
    return count;
  }

  if (m_first) {
    if (traits_type::eq_int_type (overflow (traits_type::eof()), traits_type::eof())) {
      return 0;
    }
  }

  return m_out.sputn (data, count);
}

OCPI::Logger::DebugEnabled::DebugLogger::
DebugLogger (Logger & delegatee)
  : Logger (m_obuf), m_obuf (delegatee.m_buf)
{
}

OCPI::Logger::DebugEnabled::DebugLogger::
~DebugLogger ()
{
}
