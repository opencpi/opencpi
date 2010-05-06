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

// -*- c++ -*-

/**
 * \file
 *
 * \brief The NullOutput logger to throw away log messages.
 */

#ifndef CPILOGGERNULLOUTPUT_H__
#define CPILOGGERNULLOUTPUT_H__

#include <CpiLoggerLogger.h>

namespace CPI {
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
