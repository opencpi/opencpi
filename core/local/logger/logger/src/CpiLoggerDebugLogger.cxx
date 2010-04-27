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

#include <CpiOsAssert.h>
#include <CpiOsMutex.h>
#include <CpiLoggerLogger.h>
#include <CpiLoggerDebugLogger.h>
#include <streambuf>
#include <iostream>
#include <string>
#include <map>

namespace {
  typedef std::map<std::string, unsigned int> StringToUIntMap;
  bool g_debugAllProducerNames;
  unsigned int g_debugAllVerbosity;
  StringToUIntMap g_debugProducerNames;
  CPI::OS::Mutex g_debugProducerNamesLock;
}

/*
 * Enable/Disable debugging
 */

void
CPI::Logger::debug (const std::string & producerName, unsigned int v)
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

CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::DebugLoggerBuf (Logger::LogBuf & other)
  : m_first (true),
    m_locked (false),
    m_out (other)
{
}

CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::~DebugLoggerBuf ()
{
  if (!m_first) {
    m_out.pubsync ();
  }
}

void
CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setLogLevel (unsigned short)
{
  cpiAssert (0);
}

void
CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setProducerId (const char * producerId)
{
  m_out.setProducerId (producerId);
}

void
CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setProducerName (const char * producerName)
{
  m_lock.lock ();
  cpiAssert (m_first);
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
CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::setVerbosity (unsigned int v)
{
  cpiAssert (m_locked);
  cpiAssert (m_first);

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
CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::sync ()
{
  cpiAssert (m_locked);

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
CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::overflow (int_type c)
{
  cpiAssert (m_locked);

  if (!m_shouldprint) {
    if (traits_type::eq (c, traits_type::eof())) {
      return traits_type::not_eof (c);
    }

    return c;
  }

  if (m_first) {
    m_out.setLogLevel (9 + m_verbosity);
    m_out.setProducerName (m_producerName.c_str());
    m_first = false;
  }

  if (traits_type::eq (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  return m_out.sputc (traits_type::to_char_type (c));
}

std::streamsize
CPI::Logger::DebugEnabled::DebugLogger::
DebugLoggerBuf::xsputn (const char * data, std::streamsize count)
{
  cpiAssert (m_locked);

  if (!m_shouldprint) {
    return count;
  }

  if (m_first) {
    if (traits_type::eq (overflow (traits_type::eof()), traits_type::eof())) {
      return 0;
    }
  }

  return m_out.sputn (data, count);
}

CPI::Logger::DebugEnabled::DebugLogger::
DebugLogger (Logger & delegatee)
  : Logger (m_obuf), m_obuf (delegatee.m_buf)
{
}

CPI::Logger::DebugEnabled::DebugLogger::
~DebugLogger ()
{
}
