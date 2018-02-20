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
 * \mainpage The OCPI::Logger Logging Infrastructure
 *
 * The Logger library provides a logging infrastructure, inspired by the
 * Lightweight Log Service, using the same semantics as the iostream
 * library.
 *
 * Using an abstract base class, actual output may be redirected to a
 * null device, any std::ostream (such as std::cout), a remote logging
 * service, etc.
 *
 * \section intro Introduction
 *
 * Log messages are arbitrary text (unlimited in size), tagged with an
 * integer <em>log level</em>, a <em>producer name</em> and a per-instance
 * <em>producer id</em> string.
 *
 * Loggers are derived from std::ostream, so log messages can be printed
 * using its \<\< operators.
 *
 * The <em>producer id</em> field is intended as a system-wide identifier
 * of the reporting component (process).  The producer id is usually set at
 * startup and remains unchanged.
 *
 * Log messages may optionally be tagged with a <em>producer name</em>,
 * which identifies the subsystem within the component.  If a message
 * is not assigned a producer name, it defaults to the empty string.
 *
 * Logger classes with the <em>Output</em> suffix (e.g.,
 * <em>OStreamOutput</em>) are <em>leaf</em> classes in that they write
 * log messages to some logging destination.  Other classes in this
 * namespace delegate output to secondary loggers.
 *
 * Usually, a single logger instance is instantiated near the beginning
 * of a program, e.g., in main().  Ideally, the type of logger is
 * influenceable by user choice (e.g., via a configuration file).  This
 * instance is then passed, as a polymorphic reference or pointer to the
 * OCPI::Logger::Logger base class, to all functions and classes that
 * produce log messages.  This way, log messages can be printed without
 * being aware of the log message's final destination.
 *
 * \section base The Logger Base Class
 *
 * OCPI::Logger::Logger &mdash; The abstract base class.
 *
 * \section out Output Loggers
 *
 * These classes implement the Logger interface by logging a message to
 * some logging destination.
 *
 * OCPI::Logger::NullOutput &mdash; Send log messages to the "null device".\n
 * OCPI::Logger::OStreamOutput &mdash; Send log messages to any std::ostream.\n
 *
 * \section filters Filters
 *
 * These classes implement the Logger interface by delegating message
 * output to one or more secondary Logger instances.
 *
 * OCPI::Logger::PrefixInserter &mdash; Add a prefix to each log message.\n
 * OCPI::Logger::Tee &mdash; Send log messages to multiple loggers.\n
 * OCPI::Logger::Fallback &mdash; Fall back to another logger if the first one fails.\n
 *
 * \section other Others
 *
 * OCPI::Logger::OStreamAdapter &mdash; Adapt legacy code that logs to an std::ostream.\n
 *
 * \section debug Debug Logger
 *
 * OcpiLoggerDebugLogger.h &mdash; Conditionally log debugging messages.\n
 *
 * Depending on compile-time definition of the the NDEBUG preprocessor
 * symbol, either OCPI::Logger::DebugEnabled::DebugLogger or
 * OCPI::Logger::DebugDisabled::DebugLogger is imported into the
 * OCPI::Logger namespace and becomes OCPI::Logger::DebugLogger.
 * Because doxygen does not interpret the C++ "using" statement,
 * OCPI::Logger::DebugLogger does not show in this documentation.
 * Please refer to the header file OcpiLoggerDebugLogger.h for more
 * information.
 *
 * \section notes Notes
 *
 * This library does not provide a Logger implementation to log to a
 * remote Lightweight Log Service, so that this library does not have
 * a CORBA dependency.
 */

#ifndef OCPILOGGERLOGGER_H__
#define OCPILOGGERLOGGER_H__

#include <streambuf>
#include <iostream>
#include <string>

/**
 * \file
 *
 * \brief The OCPI::Logger::Logger base class and helpers.
 */

namespace OCPI {
  namespace Logger {

    namespace Level {
      /**
       * \brief Predefined Logging Levels
       *
       * These are the named logging levels defined by the <em>Lightweight
       * Logging</em> specification for levels 1 to 9.
       *
       * See OCPI::Logger::LogLevel for user-defineable log levels.
       *
       * To assign a log level to a particular message, use the "\<\<"
       * operator.  The log level shall be assigned at the beginning of
       * each message.
       *
       * Example:
       *
       * \code
       *   OCPI::Logger::Logger & logger = ...
       *   logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
       *          << "Hello World!"
       *          << std::flush
       * \endcode
       */

      enum LwLogLevel {
        SECURITY_ALARM = 1,
        FAILURE_ALARM = 2,
        DEGRADED_ALARM = 3,
        EXCEPTION_ERROR = 4,
        FLOW_CONTROL_ERROR = 5,
        RANGE_ERROR = 6,
        USAGE_ERROR = 7,
        ADMINISTRATIVE_EVENT = 8,
        STATISTIC_REPORT = 9
      };
    }

    /**
     * \brief User-defined Logging Levels
     *
     * The logging level is an unsigned 16 bit integer that categorizes
     * messages.  The <em>Lightweight Logging</em> specification defines
     * names for levels 1 to 9 (see OCPI::Logger::Level::LwLogLevel).
     * Levels 10 to 26 are reserved for debugging.
     *
     * The value provided is recorded in the logging record and provided
     * to the consumer at retrieval, but it has no particular meaning or
     * side effects during storage of the record in the Log.  No ordering
     * is implied by the log level's magnitude - i.e., a log level of 100
     * is not meant to be more or less "severe" than log level 99.
     *
     * Some care should be taken to synchronize log level values within
     * a domain.
     *
     * To assign a log level to a particular message, use the \<\<
     * operator.  The log level shall be assigned at the beginning of
     * each message.
     *
     * Example:
     *
     * \code
     *   OCPI::Logger::Logger & logger = ...
     *   logger << OCPI::Logger::LogLevel (42)
     *          << "Hello World!"
     *          << std::flush
     * \endcode
     *
     * The above example uses a temporary LogLevel object.  It is also
     * possible to create LogLevel objects on the stack for re-use.
     */

    struct LogLevel {
      /**
       * Constructor.
       *
       * \param[in] level The user-defined log level.
       */

      explicit LogLevel (unsigned short level);
      unsigned short m_level;
    };

    /**
     * \brief Producer Name
     *
     * This field identifies the producer of a log record in textual
     * format.  This field is assigned by the log producer, thus is not
     * unique within the Domain (e.g., multiple instances of an
     * application will assign the same name to the ProducerName field).
     *
     * The producer name field for a message can be set by emitting a
     * ProducerName object.  This shall be done after setting the log
     * level but before emitting any log message content.  Otherwise,
     * the ProducerName field defaults to the empty string.
     *
     * Example:
     *
     * \code
     *   OCPI::Logger::Logger & logger = ...
     *   logger << // set log level
     *          << OCPI::Logger::ProducerName ("Test")
     *          << // write log message content
     *          << std::flush
     * \endcode
     *
     * The above example uses a temporary ProducerName object.  It is
     * also possible to create ProducerName objects on the stack for
     * re-use.
     */

    struct ProducerName {
      /**
       * Constructor.
       *
       * The \a name shall have a life span greater than the ProducerName
       * object.
       *
       * \param[in] name Construct the producer name from a null-terminated
       *                 string.
       */

      explicit ProducerName (const char * name);

      /**
       * Constructor.
       *
       * The \a name shall not be modified or destructed during the life
       * span of this object.
       *
       * \param[in] name Construct the producer name from an STL string.
       */

      explicit ProducerName (const std::string & name);

      /**
       * Pointer to the producer name.  Shall not be modified after
       * construction.
       */

      const char * m_producerName;
    };

    /**
     * \brief Abstract base class for all loggers.
     *
     * The Logger class is an abstract base class.  Derived classes realize
     * specific output targets for log messages, but are usually passed to
     * functions as a reference or pointer to this base class, so that log
     * messages can be produced without being aware of the actual logging
     * destination.  The Logger class is derived from <em>std::ostream</em>.
     *
     * Given a logger instance, a log message is produced in the following
     * sequence:
     *
     * -# Set the message's <em>log level</em> by emitting (using the \<\<
     *    operator) a OCPI::Logger::LogLevel object or one of the predefined
     *    OCPI::Logger::Level::LwLogLevel enumeration values.
     * -# Optionally, set the <em>producer name</em> by emitting (using the
     *    \<\< operator) a OCPI::Logger::ProducerName object.
     * -# Print the log message contents, using the \<\< operator or any
     *    other applicable std::ostream operation.
     * -# End the log message by flushing the output; this is usually done
     *    by emitting <em>std::flush</em> object using the \<\< operator.
     *
     * Log messages should not be line-oriented and thus
     * should not contain any line breaks.  The logger that eventually prints
     * a message (on the console, in a file, in a dialog box) will take
     * care of breaking long messages across multiple lines, if necessary.
     * In other cases, the tools that are used to view log messages will do
     * the proper formatting.
     *
     * To print a line break despite the above advice, output "\n".
     *
     * Note that <em>std::endl</em> implies writing a newline character
     * followed by a <em>std::flush</em>.  Thus, <em>std::endl</em> also
     * ends a log message, but not before inserting a stray newline
     * character at the end of the message.
     *
     * Derived from <em>std::ostream</em>, Logger instances use the
     * <em>std::ios::rdstate()</em> bitmask to report or propagate errors.
     * In case of an error, the <em>badbit</em> is set, <em>good()</em>
     * returns false, <em>bad()</em> and <em>fail()</em> return true.
     *
     * Logger instances are not seekable.
     *
     * Usually, the only Logger function that is called directly by user
     * code is the setProducerId() function to set the process-wide
     * producer id.
     *
     * Example:
     *
     * \code
     *   OCPI::Logger::Logger & logger = ...
     *   logger << OCPI::Logger::Level::ADMINISTRATIVE_EVENT
     *          << OCPI::Logger::ProducerName ("Test")
     *          << "Hello World!"
     *          << std::flush;
     * \endcode
     */

    class Logger : public std::ostream {
    public:
      /** \cond */
      class LogBuf : public std::streambuf {
      public:
        virtual ~LogBuf ();
        virtual void setLogLevel (unsigned short) = 0;
        virtual void setProducerId (const char *) = 0;
        virtual void setProducerName (const char *) = 0;
      };
      /** \endcond */

    public:
      /**
       * Constructor.
       *
       * Not user callable.
       */

      Logger (LogBuf &);

      /**
       * Destructor.
       *
       * Not user callable.
       */

      ~Logger ();

      /**
       * Set the log level.  This is usually called via the output
       * operators for OCPI::Logger::Level::LwLogLevel or
       * OCPI::Logger::LogLevel.
       */

      void setLogLevel (unsigned short);

      /**
       * Set the producer id.  This is usually set only once at startup
       * (near the construction of a process-wide Logger instance) and
       * remains unchanged throughout the life cycle of the process.
       */

      void setProducerId (const char *);

      /**
       * Set the log level.  This is usually called via the
       * output operator for OCPI::Logger::ProducerName.
       */

      void setProducerName (const char *);

    public:
      LogBuf & m_buf;
    };

  }
}

/*
 * Some inline implementations
 */

inline
OCPI::Logger::LogLevel::LogLevel (unsigned short logLevel)
{
  m_level = logLevel;
}

inline
OCPI::Logger::ProducerName::ProducerName (const char * producerName)
  : m_producerName (producerName)
{
}

inline
OCPI::Logger::ProducerName::ProducerName (const std::string & producerName)
  : m_producerName (producerName.c_str())
{
}

inline
void
OCPI::Logger::Logger::setLogLevel (unsigned short logLevel)
{
  m_buf.setLogLevel (logLevel);

  /*
   * This works around a bug in the WindRiver/Microsoft standard C++
   * library.  (They both seem to be derived from Dinkumware and bear
   * a copyright mark from P.J. Plauger.  The Microsoft VC++ 7.1 one
   * seems to be version "V3.13:0009" while the WindRiver Workbench
   * 2.5 one seems to be version "V4.00:1278".)
   *
   * The issue is the implementation of ostream::flush.  The C++
   * standard says: "If rdbuf() is not a null pointer, calls rdbuf()->
   * pubsync()."
   *
   * The implementation, however, does:
   *
   *   if (!ios_base::fail() && _Myios::rdbuf()->pubsync() == -1)
   *     _State |= ios_base::badbit;     // sync failed
   *
   * The difference being that rdbuf()->pubsync() is not called when
   * ios_base::fail() is true, i.e., when the stream is not good.
   *
   * To ensure thread-safety, some logger implementations acquire a lock
   * during this Logger::setLogLevel() operation, and release it in the
   * streambuf::pubsync() operation.
   *
   * However, if the stream is or becomes "bad" (i.e., failbit is set,
   * e.g., because of an unsuccessful ostream::write() operation), then
   * due to the above logic, streambuf::pubsync() is never called and
   * hence the acquired lock is never released, and all future attempts
   * to write to the stream will deadlock.
   *
   * This would not happen if ostream::flush() was implemented as
   * prescribed by the standard, where calling of streambuf::pubsync()
   * is not preconditioned on the status of the stream.
   *
   * I have not been able to come up with a race condition-free workaround
   * for affected implementations of Logger; my best solution was to clear
   * the failbit here.  Implementations still have to make sure that the
   * failbit is not set while composing a message.
   *
   * Critics may argue that the std::ostream interface is not designed to
   * be thread-safe anyway.
   */

  clear ();
}

inline
void
OCPI::Logger::Logger::setProducerId (const char * producerId)
{
  m_buf.setProducerId (producerId);
}

inline
void
OCPI::Logger::Logger::setProducerName (const char * producerName)
{
  m_buf.setProducerName (producerName);
}

/**
 * Output operator to set a predefined log level.
 */

inline
OCPI::Logger::Logger &
operator<< (OCPI::Logger::Logger & logger, OCPI::Logger::Level::LwLogLevel logLevel)
{
  logger.setLogLevel (static_cast<unsigned short> (logLevel));
  return logger;
}

/**
 * Output operator to set a user-defined log level.
 */

inline
OCPI::Logger::Logger &
operator<< (OCPI::Logger::Logger & logger, const OCPI::Logger::LogLevel & logLevel)
{
  logger.setLogLevel (logLevel.m_level);
  return logger;
}

/**
 * Output operator to set a message's producer name.
 */

inline
OCPI::Logger::Logger &
operator<< (OCPI::Logger::Logger & logger, const OCPI::Logger::ProducerName & producerName)
{
  logger.setProducerName (producerName.m_producerName);
  return logger;
}

#endif
