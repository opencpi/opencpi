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

/**
 * Log to the Lightweight Logging service.
 *
 * Revision History:
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2 log service.
 *
 *     08/14/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>
#include <string>
#include <CpiOsAssert.h>
#include <CpiOsMutex.h>
#include <CpiLoggerLogger.h>
#include <CpiStringifyCorbaException.h>
#include "CpiLwLogLoggerOutput.h"

/*
 * ----------------------------------------------------------------------
 * OStreamOutput: log to an std::ostream
 * ----------------------------------------------------------------------
 */

CPI::CORBAUtil::LwLogLoggerOutput::Buf::
Buf ()
  : m_first (true),
    m_locked (false),
    m_logLevel (0),
    m_allLogLevelsEnabled (true),
    m_logService (LogProducer::_nil ())
{
}

CPI::CORBAUtil::LwLogLoggerOutput::Buf::
Buf (LogProducer_ptr logService)
  : m_first (true),
    m_locked (false),
    m_logLevel (0),
    m_allLogLevelsEnabled (true),
    m_logService (LogProducer::_duplicate (logService))
{
}

CPI::CORBAUtil::LwLogLoggerOutput::Buf::
~Buf ()
{
  if (m_locked) {
    m_lock.unlock ();
  }
}

void
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
setLogService (LogProducer_ptr logService)
{
  m_lock.lock ();
  m_logService = LogProducer::_duplicate (logService);
  m_lock.unlock ();
}

void
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
setLogLevels (const LogLevelSequence & levels)
{
  CORBA::ULong nl = levels.length ();

  m_lock.lock ();

  m_allLogLevelsEnabled = false;
  m_enabledLogLevels.clear ();
  for (CORBA::ULong li=0; li<nl; li++) {
    m_enabledLogLevels.insert (static_cast<unsigned short> (levels[li]));
  }

  m_lock.unlock ();
}

void
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
setLogLevel (unsigned short logLevel)
{
  m_lock.lock ();
  m_locked = true;
  m_logLevel = logLevel;
  m_producerName.clear ();
  m_logMessage.clear ();
  cpiAssert (m_first);
}

void
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
setProducerId (const char * producerId)
{
  m_producerId = producerId;
}

void
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
setProducerName (const char * producerName)
{
  cpiAssert (m_locked);
  cpiAssert (m_first);
  m_producerName = producerName;
}

int
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
sync ()
{
  bool good = true;

  if (!m_first && CORBA::is_nil (m_logService)) {
    cpiAssert (m_locked);
    m_first = true;
    good = false;
  }
  else if (!m_first) {
    cpiAssert (m_locked);

    if (m_allLogLevelsEnabled ||
	m_enabledLogLevels.find (m_logLevel) != m_enabledLogLevels.end ()) {
      try {
#if defined (CPI_USES_SCA22)
	::LogService::Log::ProducerLogRecordSequence plrs (1);
	plrs.length (1);
	::LogService::ProducerLogRecordType & plr = plrs[0];

	plr.producerId = m_producerId.c_str ();
	plr.producerName = m_producerName.c_str ();
	plr.level = static_cast< ::LogService::LogLevelType > (m_logLevel - 1);
	plr.logData = m_logMessage.c_str ();

	m_logService->writeRecords (plrs);
#else
	::CosLwLog::ProducerLogRecord plr;
	plr.producerId = m_producerId.c_str ();
	plr.producerName = m_producerName.c_str ();
	plr.level = m_logLevel;
	plr.logData = m_logMessage.c_str ();

	m_logService->write_record (plr);
#endif
      }
      catch (const CORBA::Exception &) {
	good = false;
      }
    }

    m_first = true;
  }

  if (m_locked) {
    m_locked = false;
    m_lock.unlock ();
  }

  return (good ? 0 : -1);
}

std::streambuf::int_type
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
overflow (int_type c)
{
  cpiAssert (m_locked);

  if (traits_type::eq (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  m_first = false;
  m_logMessage += traits_type::to_char_type (c);
  return c;
}

std::streamsize
CPI::CORBAUtil::LwLogLoggerOutput::Buf::
xsputn (const char * data, std::streamsize count)
{
  cpiAssert (m_locked);

  m_first = false;
  m_logMessage.append (data, count);
  return count;
}

CPI::CORBAUtil::LwLogLoggerOutput::
LwLogLoggerOutput ()
  : Logger (m_obuf)
{
  setstate (std::ios_base::badbit);
}

CPI::CORBAUtil::LwLogLoggerOutput::
LwLogLoggerOutput (LogProducer_ptr logService)
  : Logger (m_obuf),
    m_obuf (logService)
{
  if (CORBA::is_nil (logService)) {
    setstate (std::ios_base::badbit);
  }
}

CPI::CORBAUtil::LwLogLoggerOutput::
~LwLogLoggerOutput ()
{
}

void
CPI::CORBAUtil::LwLogLoggerOutput::
setLogService (LogProducer_ptr logService)
{
  m_obuf.setLogService (logService);

  if (CORBA::is_nil (logService)) {
    setstate (std::ios_base::badbit);
  }
  else {
    clear ();
  }
}

void
CPI::CORBAUtil::LwLogLoggerOutput::
setLogLevels (const LogLevelSequence & levels)
{
  m_obuf.setLogLevels (levels);
}
