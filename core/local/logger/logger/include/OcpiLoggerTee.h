
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


// -*- c++ -*-

/**
 * \file
 *
 * \brief The Tee replicates log messages to multiple loggers.
 */

#ifndef OCPILOGGERTEE_H__
#define OCPILOGGERTEE_H__

#include <OcpiLoggerLogger.h>
#include <OcpiOsMutex.h>
#include <string>
#include <vector>

namespace OCPI {
  namespace Logger {

    /**
     * \brief The Tee replicates log messages to multiple loggers.
     *
     * The Tee class implements the Logger interface.  All log messages
     * are replicated to zero or more secondary Logger instances. The
     * Tee class can thus be used to, e.g., send log messages to
     * std::cerr, a log file, and a remote Logging Service at the
     * same time.
     */

    class Tee : public Logger {
    public:
      /** \cond */
      class TeeBuf : public LogBuf {
      public:
        TeeBuf ();
        ~TeeBuf ();

        void addOutput (Logger *, bool, bool, bool);

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
          bool ignoreErrors;
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
       * A Tee object is created with an empty list of delegatees.  In
       * this state, the Tee can be used as a Logger, but log messages
       * will be discarded (i.e., replicated to all zero loggers).
       */

      Tee ();

      /**
       * Destructor.
       *
       * Deletes all adopted loggers.
       */

      ~Tee ();

      /**
       * Add a delegatee that will receive future log messages.
       *
       * The \a delegatee shall have a life span longer than this Tee
       * object.
       *
       * \param[in] delegatee The logger to log messages to.
       * \param[in] retry     If false, then the Tee will not log to
       *                      \a delegatee for as long as its good()
       *                      operation returns false; resetting that
       *                      flag may then require external action.
       *                      If the true, the Tee will reset
       *                      \a delegatee's state at the beginning
       *                      of each log message (using its clear()
       *                      operation), and re-attempt to send the
       *                      new message to that Logger.
       * \param[in] ignoreErrors If false, then any error writing a
       *                      message to \a delegatee causes the Tee
       *                      to fail (i.e., good() will return false).
       *                      If true, then any error writing to
       *                      \a delegatee is ignored.
       */

      void addOutput (Logger & delegatee,
                      bool retry = false,
                      bool ignoreErrors = false);

      /**
       * Add a delegatee that will receive future log messages.
       *
       * If \a adopt is false, \a delegatee shall have a life span longer
       * than this Tee object.  If \a adopt is true, then
       * \a delegatee shall not be used after this object's destruction.
       *
       * \param[in] delegatee The logger to log messages to.
       * \param[in] adopt     Whether to adopt the \a delegatee.  If true
       *                      then \a delegatee is deleted by this object's
       *                      destructor.  If false, then destruction of
       *                      this object has no effect on \a delegatee.
       * \param[in] retry     If false, then the Tee will not log to
       *                      \a delegatee for as long as its good()
       *                      operation returns false; resetting that
       *                      flag may then require external action.
       *                      If the true, the Tee will reset
       *                      \a delegatee's state at the beginning
       *                      of each log message (using its clear()
       *                      operation), and re-attempt to send the
       *                      new message to that Logger.
       * \param[in] ignoreErrors If false, then any error writing a
       *                      message to \a delegatee causes the Tee
       *                      to fail (i.e., good() will return false).
       *                      If true, then any error writing to
       *                      \a delegatee is ignored.
       */

      void addOutput (Logger * delegatee,
                      bool adopt = false,
                      bool retry = false,
                      bool ignoreErrors = false);

    protected:
      TeeBuf m_obuf;
    };

  }
}

#endif
