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
 * \brief The OStreamAdapter allows legacy code to log to a Logger.
 */

#ifndef CPILOGGEROSTREAMADAPTER_H__
#define CPILOGGEROSTREAMADAPTER_H__

#include <CpiOsMutex.h>
#include <CpiLoggerLogger.h>
#include <streambuf>
#include <iostream>

namespace CPI {
  namespace Logger {

    /**
     * \brief The OStreamAdapter allows legacy code to log to a Logger.
     *
     * The OStreamAdapter class can be used to integrate "legacy" code
     * that prints log messages to a regular <em>std::ostream</em> into
     * the logging infrastructure.
     *
     * Legacy code that writes to a regular <em>std::ostream</em> is
     * ignorant of the semantics imposed by the Logger class: it does
     * not set the log level or the producer name for each message,
     * and freely prints newlines, when it should rather be using
     * <em>std::flush</em> to indicate the end of a message.
     *
     * The OStreamAdapter class implements the <em>std::ostream</em>
     * interface.  Each line of output is interpreted as a separate
     * log message, tagged with a fixed log level and producer name,
     * and delegated to a Logger instance. In-stream newline
     * characters are discarded.  Since a <em>std::endl</em> implies a
     * newline character followed by a <em>std::flush</em>, the newline
     * can be ignored; the <em>std::flush</em> is then forwarded to
     * the Logger and ends the message.
     *
     * An instance of this class can be widened to <em>std::ostream</em>
     * and be passed to legacy code.
     *
     * It can also be used by code that is too lazy to repeatedly set
     * the same log record fields all over again and rather wishes to set
     * defaults, at the expense of not being able to print multi-line
     * messages.
     *
     * One caveat is that legacy code will not work if it explicitly
     * prints newlines rather than using <em>std::endl</em>.
     *
     * Example:
     *
     * \code
     *   CPI::Logger::Logger & logger = ...
     *   CPI::Logger::OStreamAdapter out (logger,
     *                                    10, // Debugging level 1
     *                                    "Test");
     *   out << "Hello World!" << std::endl;
     * \endcode
     */

    class OStreamAdapter : public std::ostream {
    public:
      /** \cond */
      class OStreamAdapterBuf : public std::streambuf {
      public:
        OStreamAdapterBuf (Logger *,
                           unsigned short,
                           const std::string &,
                           bool);
        ~OStreamAdapterBuf ();

      protected:
        int sync ();
        int_type overflow (int_type = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);

      protected:
        bool m_first;
        bool m_adopted;
        Logger * m_logger;
        Logger::LogBuf & m_out;
        unsigned short m_logLevel;
        std::string m_producerName;
        CPI::OS::Mutex m_selfLock;
      };
      /** \endcond */

    public:
      /**
       * Constructor
       *
       * The \a logger shall have a life span longer than this
       * OStreamAdapter object.
       *
       * \param[in] logger   The logger to log messages to.
       * \param[in] logLevel The log level to use for all log messages.
       * \param[in] producerName The producer name to use for all log
       *                     messages.
       */

      OStreamAdapter (Logger & logger,
                      unsigned short logLevel,
                      const std::string & producerName);

      /**
       * Constructor
       *
       * If \a adopt is false, \a logger shall have a life span longer
       * than this OStreamAdapter object.  If \a adopt is true, then
       * \a logger shall not be used after this object's destruction.
       *
       * \param[in] logger   The logger to log messages to.
       * \param[in] logLevel The log level to use for all log messages.
       * \param[in] producerName The producer name to use for all log
       *                     messages.
       * \param[in] adopt    Whether to adopt the \a logger.  If true
       *                     then \a logger is deleted by this object's
       *                     destructor.  If false, then destruction of
       *                     this object has no effect on \a logger.
       */

      OStreamAdapter (Logger * logger,
                      unsigned short logLevel,
                      const std::string & producerName,
                      bool adopt = false);

      /**
       * Destructor.
       */

      ~OStreamAdapter ();

    protected:
      OStreamAdapterBuf m_buf;
    };

  }
}

#endif
