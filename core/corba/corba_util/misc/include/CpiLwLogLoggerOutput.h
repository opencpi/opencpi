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
 * \file
 * \brief Log to the Lightweight Logging service.
 *
 * Revision History:
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2 log service.
 *
 *     08/14/2008 - Frank Pilhofer
 *                  Initial version.
 */

#ifndef CPILWLOGLOGGEROUTPUT_H__
#define CPILWLOGLOGGEROUTPUT_H__

#include <CpiOsMutex.h>
#include <CpiLoggerLogger.h>
#include <set>

#if defined (CPI_USES_SCA22)
#include <LogService.h>
#else
#include <CosLwLogProducer.h>
#endif

namespace CPI {
  namespace CORBAUtil {

    class LwLogLoggerOutput : public ::CPI::Logger::Logger {
    public:
#if defined (CPI_USES_SCA22)
      typedef ::LogService::Log LogProducer;
      typedef ::LogService::Log_ptr LogProducer_ptr;
      typedef ::LogService::Log_var LogProducer_var;
      typedef ::LogService::LogLevelType LogLevelType;
      typedef ::LogService::LogLevelSequenceType LogLevelSequence;
#else
      typedef ::CosLwLog::LogProducer LogProducer;
      typedef ::CosLwLog::LogProducer_ptr LogProducer_ptr;
      typedef ::CosLwLog::LogProducer_var LogProducer_var;
      typedef unsigned short LogLevelType;
      typedef ::CosLwLog::LogLevelSequence LogLevelSequence;
#endif

      class Buf : public LogBuf {
      public:
        Buf ();
        Buf (LogProducer_ptr logService);
        ~Buf ();

        void setLogService (LogProducer_ptr logService);
        void setLogLevels (const LogLevelSequence & levels);

        void setLogLevel (unsigned short);
        void setProducerId (const char *);
        void setProducerName (const char *);

      protected:
        int sync ();
        int_type overflow (int_type = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);

      protected:
        typedef std::set<unsigned short> LogLevelSet;

      protected:
        bool m_first;
        bool m_locked;
        unsigned short m_logLevel;
        std::string m_producerId;
        std::string m_producerName;
        std::string m_logMessage;
        CPI::OS::Mutex m_lock;
        bool m_allLogLevelsEnabled;
        LogLevelSet m_enabledLogLevels;
        LogProducer_var m_logService;
      };

    public:
      LwLogLoggerOutput ();
      LwLogLoggerOutput (LogProducer_ptr logService);
      ~LwLogLoggerOutput ();

      void setLogService (LogProducer_ptr logService);
      void setLogLevels (const LogLevelSequence & levels);

    protected:
      Buf m_obuf;
    };

  }
}

#endif
