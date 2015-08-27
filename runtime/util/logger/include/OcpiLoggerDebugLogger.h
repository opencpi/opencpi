
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
 * \brief Debug Logger to conditionally log debug messages.
 *
 * The OCPI::Logger::DebugLogger class wraps a OCPI::Logger::Logger
 * instance, and it is used to log debug messages.  If the NDEBUG
 * preprocessor symbol is not defined, OCPI::Logger::DebugLogger
 * inherits from OCPI::Logger::Logger, and can be used as described
 * above; all debug messages (for which debug message logging is
 * enabled, see below) are delegated to the wrapped Logger instance
 * for output.  If the NDEBUG preprocessor symbol is defined,
 * DebugLogger does not inherit from Logger, but overloads all
 * output operators with empty inline implementations, so that an
 * intelligent compiler can optimize away all debug messages.
 * Therefore, there is no need to conditionally compile debug
 * messages.
 *
 * OCPI::Logger::DebugLogger instances may only be used locally,
 * i.e., entirely within a function.  OCPI::Logger::DebugLogger shall
 * not be used as a parameter type.  Instances of DebugLogger shall
 * not be widened to Logger (e.g., by passing it as a parameter
 * expecting a Logger).  Instead, the wrapped Logger "target" should
 * be passed to other functions, so that they can then create a
 * OCPI::Logger::DebugLogger instance themselves.
 *
 * The semantics for printing debug messages via DebugLogger are
 * slightly different than for log messages using OCPI::Logger::Logger.
 * Debug messages must not start with a log level, which is implicitly
 * set using the message's verbosity value, see below.  Each debug
 * message must start with a producer name.
 *
 * After setting the producer name, but before writing the debug
 * message content, the message's <em>verbosity</em> value may be
 * set by emitting an object of type OCPI::Logger::Verbosity.  The
 * idea is to only print debug messages whose verbosity does not
 * exceed a configurable level.  The higher the level of detail that
 * is reported by a message, the higher the verbosity value should
 * be.   If no verbosity value is set for a message, it defaults to 1.
 *
 * E.g., a Web browser application could use verbosity level 1
 * for messages relating to a Web page, level 2 for messages
 * relating to items on the Web page (such as frames and images),
 * level 3 for connection setup, and 4 for data transmission.
 *
 * By default, debug messages are disabled, i.e., will not be printed.
 * Debug messages for certain producer names can be enabled by calling
 * OCPI::Logger::debug() with the desired producer name and verbosity
 * value.  This is cumulative, i.e., after OCPI::Logger::debug("ProducerA")
 * and OCPI::Logger::debug("ProducerB"), debug messages for both producers
 * are enabled.  Calling OCPI::Logger::debug("All") enables debug message
 * logging for all producers.  Calling OCPI::Logger::debug("None") disables
 * debug message logging for all producers.  The debug message logging
 * preferences are shared by all threads within the same process.
 *
 * The DebugLogger class is defined twice, once as
 * OCPI::Logger::DebugEnabled::DebugLogger, and once as
 * OCPI::Logger::DebugDisabled::DebugLogger.
 * Depending on the compile-time setting of NDEBUG, the one or the other
 * is imported into the OCPI::Logger namespace.  This way, debug-enabled
 * and debug-disabled symbols do not conflict, and code that was compiled
 * with NDEBUG defined can be linked with code that was compiled with
 * NDEBUG not defined.
 *
 * Since doxygen does not interpret C++'s "using" statement, the
 * OCPI::Logger::DebugLogger class does not show up in this documentation.
 *
 * Example:
 *
 * \code
 * void
 * test (OCPI::Logger::Logger & logger)
 * {
 *   // Create a DebugLogger object that delegates to logger.
 *   OCPI::Logger::DebugLogger debug (logger);
 *   // Write a debug message with the default verbosity level of 1.
 *   debug << OCPI::Logger::ProducerName ("Test")
 *         << "A debug message."
 *         << std::flush;
 *   // Write a debug message with a verbosity level of 2.
 *   debug << OCPI::Logger::ProducerName ("Test")
 *         << OCPI::Logger::Verbosity (2)
 *         << "Another more verbose debug message."
 *         << std::flush;
 * }
 *
 * int
 * main (int, char *[])
 * {
 *   OCPI::Logger::OStreamOutput logger (std::cout);
 *   test (logger); // will print no debug messages, not enabled yet
 *   OCPI::Logger::debug ("Test", 1);
 *   test (logger); // will print the first debug message
 *   OCPI::Logger::debug ("All", 99);
 *   test (logger); // will print both debug messages
 *   return 0;
 * }
 * \endcode
 */

#ifndef OCPILOGGERDEBUGLOGGER_H__
#define OCPILOGGERDEBUGLOGGER_H__

#include <OcpiOsAssert.h>
#include <OcpiOsMutex.h>
#include <OcpiLoggerLogger.h>
#include <streambuf>
#include <iostream>
#include <string>
#include <cassert>

namespace OCPI {
  namespace Logger {

    /**
     * \brief Verbosity Level
     *
     * A Verbosity object is used to assign a verbosity level to a
     * debug message, using the \<\< operator.  The verbosity level
     * shall be assigned after setting the message's producer name,
     * but before the debug message content.
     *
     * Note that only 17 logging levels (10-26) are reserved for
     * debugging.  The use of verbosity levels greater than 17 should
     * be coordinated with the use of user-defined log levels, see
     * OCPI::Logger::LogLevel.
     *
     * A debug message's verbosity defaults to 1.
     *
     * Example:
     *
     * \code
     *   OCPI::Logger::Logger & logger = ...
     *   OCPI::Logger::DebugLogger debug (logger);
     *   debug << OCPI::Logger::ProducerName ("Test")
     *         << OCPI::Logger::Verbosity (2)
     *         << "Debug message."
     *         << std::flush;
     * \endcode
     *
     * The above example uses a temporary Verbosity object.  It is also
     * possible to create Verbosity objects on the stack for re-use.
     */

    struct Verbosity {
      /**
       * Constructor.
       *
       * \param[in] level The verbosity level.  Shall be greater than 0.
       */

      explicit Verbosity (unsigned int level);
      unsigned int m_verbosity;
    };

    /**
     * Control debugging output.
     *
     * By default, debug messages are disabled, i.e., messages "printed"
     * to a DebugLogger instance are not delegated to the Logger instance
     * (the "delegatee" parameter to the DebugLogger constructor).
     *
     * Debug messages can be enabled based on a debug message's producer
     * name attribute, for certain verbosity levels.
     *
     * The debug() function enables or disables logging for debug messages
     * that match producer name given in the \a producerName parameter, and
     * whose verbosity level is less than or equal to the \a verbosity value.
     *
     * Debug message logging, once enabled for some producer name, can be
     * disabled by calling debug() with the same producer name, and
     * \a verbosity* set to zero.
     *
     * The debug() function returns the previous verbosity setting for
     * this producer name.  It returns zero when debug message logging
     * was not enabled for this producer name.
     *
     * If the magic string "All" is passed as the \a producerName,
     * debug message logging is enabled for all messages, regardless
     * of their producer name, up to the given verbosity level.
     *
     * If the magic string "None" is passed as the \a producerName,
     * debugg message logging is disabled for all messages.  All
     * prior settings are reset.  In this case, the verbosity level
     * is ignored.
     *
     * This function has no effect if NDEBUG is defined.
     *
     * \param[in] producerName A producer name matching the producer name
     *                         that will be used by debug messages, or the
     *                         magic strings "All" or "None".
     * \param[in] verbosity    The verbosity level.  Debug messages whose
     *                         producer name equals \a producerName will
     *                         be logged if its verbosity level is less or
     *                         equal to \a verbosity.
     */

    void debug (const std::string & producerName, unsigned int verbosity = 1);

    namespace DebugEnabled {

      /**
       * \brief DebugLogger when NDEBUG is not defined.
       *
       * When NDEBUG is defined, this class is imported into the
       * OCPI::Logger namespace and becomes OCPI::Logger::DebugLogger.
       *
       * All log messages are delegated to a logger instance.
       */

      class DebugLogger : virtual public Logger {
      public:
        /** \cond */
        class DebugLoggerBuf : public LogBuf {
        public:
          DebugLoggerBuf (Logger::LogBuf &);
          ~DebugLoggerBuf ();
        
          void setLogLevel (unsigned short);
          void setProducerId (const char *);
          void setProducerName (const char *);
          void setVerbosity (unsigned int);

        protected:
          int sync ();
          int_type overflow (int_type = std::streambuf::traits_type::eof ());
          std::streamsize xsputn (const char *, std::streamsize);

        protected:
          bool m_first;
          bool m_locked;
          Logger::LogBuf & m_out;
          bool m_shouldprint;
          std::string m_producerName;
          unsigned int m_verbosity;
          OCPI::OS::Mutex m_lock;
        };
        /** \endcond */

      public:
        /**
         * Constructor.
         *
         * \param[in] delegatee The logger to delegate debug messages to,
         *                      if the message matches the criteria configured
         *                      via OCPI::Logger::debug().
         */

        DebugLogger (Logger & delegatee);

        /**
         * Destructor.
         */

        ~DebugLogger ();

        /**
         * Set the current message's verbosity level.  This function
         * shall not be called directly.  It is only called from the
         * output operator for OCPI::Logger::Verbosity.
         */

        void setVerbosity (unsigned int level);

      protected:
        DebugLoggerBuf m_obuf;
      };

    }

    namespace DebugDisabled {

      /**
       * \brief DebugLogger when NDEBUG is defined.
       *
       * When NDEBUG is defined, this class is imported into the
       * OCPI::Logger namespace and becomes OCPI::Logger::DebugLogger.
       * All output operators are redefined to have empty inline
       * implementations, so that no output is generated.  Intelligent
       * compilers will optimize away all code related to DebugLogger.
       */

      class DebugLogger {
      public:
        /**
         * Constructor.
         *
         * \param[in] delegatee This parameter is ignored.  It is present
         *                      so that code does not need to be modified
         *                      when debugging is enabled or disabled.
         */

        DebugLogger (Logger & delegatee);
      };

    }

    /*
     * Depending on the setting on NDEBUG, either DebugLogger is
     * imported into the OCPI::Logger namespace, for others to use
     * as OCPI::Logger::DebugLogger.
     */

#if defined NDEBUG
    using namespace DebugDisabled;
#else
    using namespace DebugEnabled;
#endif

}
}

/** \cond */
inline
OCPI::Logger::Verbosity::Verbosity (unsigned int v)
  : m_verbosity (v)
{
}

/*
 * DebugLogger when NDEBUG is not defined
 */

inline
void
OCPI::Logger::DebugEnabled::DebugLogger::setVerbosity (unsigned int v)
{
  m_obuf.setVerbosity (v);
}

inline
OCPI::Logger::DebugEnabled::DebugLogger &
operator<< (OCPI::Logger::DebugEnabled::DebugLogger & l,
            OCPI::Logger::Level::LwLogLevel)
{
  ocpiAssert (0);
  return l;
}

inline
OCPI::Logger::DebugEnabled::DebugLogger &
operator<< (OCPI::Logger::DebugEnabled::DebugLogger & l,
            const OCPI::Logger::LogLevel &)
{
  ocpiAssert (0);
  return l;
}

inline
OCPI::Logger::DebugEnabled::DebugLogger &
operator<< (OCPI::Logger::DebugEnabled::DebugLogger & l,
            const OCPI::Logger::ProducerName & producerName)
{
  l.setProducerName (producerName.m_producerName);
  return l;
}

inline
OCPI::Logger::DebugEnabled::DebugLogger &
operator<< (OCPI::Logger::DebugEnabled::DebugLogger & l,
            const OCPI::Logger::Verbosity & v)
{
  l.setVerbosity (v.m_verbosity);
  return l;
}

/*
 * DebugLogger when NDEBUG is defined
 */

inline
OCPI::Logger::DebugDisabled::DebugLogger::DebugLogger (Logger &)
{
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, bool)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, short)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, unsigned short)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, int)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, unsigned int)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, long)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, unsigned long)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, long long)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, unsigned long long)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, float)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, double)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, long double)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, const void *)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l,
            std::ios & (*) (std::ios &))
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l,
            std::ostream & (*) (std::ostream &))
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, char)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, const char *)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l, const std::string &)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l,
            OCPI::Logger::Level::LwLogLevel)
{
  ocpiAssert (0);
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l,
            const OCPI::Logger::LogLevel &)
{
  ocpiAssert (0);
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l,
            const OCPI::Logger::ProducerName &)
{
  return l;
}

inline
OCPI::Logger::DebugDisabled::DebugLogger &
operator<< (OCPI::Logger::DebugDisabled::DebugLogger & l,
            const OCPI::Logger::Verbosity &)
{
  return l;
}

/** \endcond */

#endif
