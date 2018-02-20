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
 * \brief Fallback falls back to another logger, if one fails.
 */

#ifndef OCPILOGGERFALLBACK_H__
#define OCPILOGGERFALLBACK_H__

#include <OcpiLoggerLogger.h>
#include <OcpiOsMutex.h>
#include <string>
#include <vector>

namespace OCPI {
  namespace Logger {

    /**
     * \brief Fallback falls back to another logger, if one fails.
     *
     * The Fallback class implements the Logger interface.  It maintains
     * a list of secondary Logger instances.  When a log message is written,
     * the Fallback class delivers the message to the first delegatee that
     * accepts it.
     *
     * I.e., as long as the primary logger is good (its good() operation
     * returns true), it receives all messages.  If the primary logger is
     * not good, then the Fallback logger tries subsequent logger instances
     * until the log message can be succcessfully written to one of them.
     *
     * The order of precedence is established by the order in which Logger
     * instances are added using the addOutput() operation.
     *
     * The Fallback logger allows to, e.g., set up logging to a remote
     * Logging Service, but to fall back to logging to <em>std::cout</em>
     * when the remote Logging Service fails.
     *
     * Writing the log message succeeds if the message can be delivered
     * to any Logger instance.  Writing the log message fails if the set
     * of Logger instances is empty, or if all Logger instances are in error.
     */

    class Fallback : public Logger {
    public:
      /** \cond */
      class FallbackBuf : public LogBuf {
      public:
        FallbackBuf ();
        ~FallbackBuf ();

        void addOutput (Logger *, bool, bool);

        void setLogLevel (unsigned short);
        void setProducerId (const char *);
        void setProducerName (const char *);

      protected:
        int sync ();
        int_type overflow (int_type = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);

      protected:
        struct Delegatee {
          Logger * delegatee;
          bool adopted;
          bool retry;
        };

        typedef std::vector<Delegatee> Delegatees;

        bool m_first;
        bool m_locked;
        Delegatees m_delegatee;
        unsigned short m_logLevel;
        std::string m_producerName;
        std::string m_logMessage;
        OCPI::OS::Mutex m_lock;
        OCPI::OS::Mutex m_selfLock;
      };
      /** \endcond */

    public:
      /**
       * Constructor.
       *
       * A Fallback object is created with an empty list of delegatees.
       * In this state, logging messages will fail, because the messages
       * can not be successfully delivered to any of the zero loggers.
       */

      Fallback ();

      /**
       * Destructor.
       *
       * Deletes all adopted loggers.
       */

      ~Fallback ();

      /**
       * Add a delegatee that may receive future log messages.
       *
       * The \a delegatee shall have a life span longer than this Fallback
       * object.
       *
       * \param[in] delegatee The logger to log messages to.
       * \param[in] retry     If false, then the Fallback will not log to
       *                      \a delegatee for as long as its good()
       *                      operation returns false; resetting that
       *                      flag may then require external action.
       *                      If the true, the Fallback will reset
       *                      \a delegatee's state at the beginning
       *                      of each log message (using its clear()
       *                      operation), and re-attempt to send the
       *                      new message to that Logger.
       */

      void addOutput (Logger & delegatee, bool retry = false);

      /**
       * Add a delegatee that will receive future log messages.
       *
       * If \a adopt is false, \a delegatee shall have a life span longer
       * than this Fallback object.  If \a adopt is true, then
       * \a delegatee shall not be used after this object's destruction.
       *
       * \param[in] delegatee The logger to log messages to.
       * \param[in] adopt     Whether to adopt the \a delegatee.  If true
       *                      then \a delegatee is deleted by this object's
       *                      destructor.  If false, then destruction of
       *                      this object has no effect on \a delegatee.
       * \param[in] retry     If false, then the Fallback will not log to
       *                      \a delegatee for as long as its good()
       *                      operation returns false; resetting that
       *                      flag may then require external action.
       *                      If the true, the Fallback will reset
       *                      \a delegatee's state at the beginning
       *                      of each log message (using its clear()
       *                      operation), and re-attempt to send the
       *                      new message to that Logger.
       */

      void addOutput (Logger * delegatee, bool adopt = false, bool retry = false);

    protected:
      FallbackBuf m_obuf;
    };

  }
}

#endif
