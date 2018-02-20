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
 * \file
 *
 * \brief The NullOutput logger to throw away log messages.
 */

#ifndef OCPILOGGERNULLOUTPUT_H__
#define OCPILOGGERNULLOUTPUT_H__

#include <OcpiLoggerLogger.h>

namespace OCPI {
  namespace Logger {

    /**
     * \brief The NullOutput logger to throw away log messages.
     *
     * The NullOutput class implements the Logger interface.  All log
     * messages are consumed and discarded (i.e., printed to the
     * <em>null device</em>.
     */

    class NullOutput : public Logger {
    public:
      /** \cond */
      class NullOutputBuf : public Logger::LogBuf {
      public:
        ~NullOutputBuf ();
        void setLogLevel (unsigned short);
        void setProducerId (const char *);
        void setProducerName (const char *);

      protected:
        int_type overflow (int_type = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);
      };
      /** \endcond */

    public:
      /**
       * Constructor.
       */

      NullOutput ();

      /**
       * Destructor.
       */

      ~NullOutput ();

    protected:
      NullOutputBuf m_obuf;
    };

  }
}

#endif
