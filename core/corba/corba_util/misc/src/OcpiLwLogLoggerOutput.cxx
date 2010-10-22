
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
#include <OcpiOsAssert.h>
#include <OcpiOsMutex.h>
#include <OcpiLoggerLogger.h>
#include <OcpiStringifyCorbaException.h>
#include "OcpiLwLogLoggerOutput.h"

/*
 * ----------------------------------------------------------------------
 * OStreamOutput: log to an std::ostream
 * ----------------------------------------------------------------------
 */

OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
Buf ()
  : m_first (true),
    m_locked (false),
    m_logLevel (0),
    m_allLogLevelsEnabled (true),
    m_logService (LogProducer::_nil ())
{
}

OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
Buf (LogProducer_ptr logService)
  : m_first (true),
    m_locked (false),
    m_logLevel (0),
    m_allLogLevelsEnabled (true),
    m_logService (LogProducer::_duplicate (logService))
{
}

OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
~Buf ()
{
  if (m_locked) {
    m_lock.unlock ();
  }
}

void
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
setLogService (LogProducer_ptr logService)
{
  m_lock.lock ();
  m_logService = LogProducer::_duplicate (logService);
  m_lock.unlock ();
}

void
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
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
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
setLogLevel (unsigned short logLevel)
{
  m_lock.lock ();
  m_locked = true;
  m_logLevel = logLevel;
  m_producerName.clear ();
  m_logMessage.clear ();
  ocpiAssert (m_first);
}

void
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
setProducerId (const char * producerId)
{
  m_producerId = producerId;
}

void
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
setProducerName (const char * producerName)
{
  ocpiAssert (m_locked);
  ocpiAssert (m_first);
  m_producerName = producerName;
}

int
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
sync ()
{
  bool good = true;

  if (!m_first && CORBA::is_nil (m_logService)) {
    ocpiAssert (m_locked);
    m_first = true;
    good = false;
  }
  else if (!m_first) {
    ocpiAssert (m_locked);

    if (m_allLogLevelsEnabled ||
        m_enabledLogLevels.find (m_logLevel) != m_enabledLogLevels.end ()) {
      try {
#if defined (OCPI_USES_SCA22)
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
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
overflow (int_type c)
{
  ocpiAssert (m_locked);

  if (traits_type::eq (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  m_first = false;
  m_logMessage += traits_type::to_char_type (c);
  return c;
}

std::streamsize
OCPI::CORBAUtil::LwLogLoggerOutput::Buf::
xsputn (const char * data, std::streamsize count)
{
  ocpiAssert (m_locked);

  m_first = false;
  m_logMessage.append (data, count);
  return count;
}

OCPI::CORBAUtil::LwLogLoggerOutput::
LwLogLoggerOutput ()
  : Logger (m_obuf)
{
  setstate (std::ios_base::badbit);
}

OCPI::CORBAUtil::LwLogLoggerOutput::
LwLogLoggerOutput (LogProducer_ptr logService)
  : Logger (m_obuf),
    m_obuf (logService)
{
  if (CORBA::is_nil (logService)) {
    setstate (std::ios_base::badbit);
  }
}

OCPI::CORBAUtil::LwLogLoggerOutput::
~LwLogLoggerOutput ()
{
}

void
OCPI::CORBAUtil::LwLogLoggerOutput::
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
OCPI::CORBAUtil::LwLogLoggerOutput::
setLogLevels (const LogLevelSequence & levels)
{
  m_obuf.setLogLevels (levels);
}
