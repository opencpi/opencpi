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

// -*- c++ -*-

/**
 * \brief Log to the Lightweight Logging Service
 */

#ifndef OCPIUTILLWLOGGEROUTPUT_H__
#define OCPIUTILLWLOGGEROUTPUT_H__

#include <string>
#include <OcpiOsMutex.h>
#include <OcpiUtilIOP.h>
#include <OcpiUtilIIOP.h>
#include <OcpiLoggerLogger.h>
#include <OcpiUtilTcpClient.h>

namespace OCPI {
  namespace Util {

    class LwLoggerOutput : public ::OCPI::Logger::Logger {
    public:
      class Buf : public LogBuf {
      public:
        Buf (const OCPI::Util::IOP::IOR &);
        ~Buf ();

        void setLogLevel (unsigned short);
        void setProducerId (const char *);
        void setProducerName (const char *);

      protected:
        int sync ();
        int_type overflow (int_type = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);

      protected:
        bool connectToLogService ();
        bool sendMessage ();

      protected:
        // logger information
        bool m_first;
        bool m_locked;
        unsigned short m_logLevel;
        std::string m_producerId;
        std::string m_producerName;
        std::string m_logMessage;
        OCPI::OS::Mutex m_lock;

      protected:
        // connection information
        bool m_connected;
        bool m_byteOrder;
        unsigned int m_requestId;
        OCPI::Util::IOP::IOR m_ior;
        OCPI::Util::Tcp::Client m_conn;
        OCPI::Util::IIOP::ProfileBody m_profile;
      };

    public:
      LwLoggerOutput (const OCPI::Util::IOP::IOR & ior);
      ~LwLoggerOutput ();

    protected:
      Buf m_obuf;
    };

  }
}

#endif
