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
 * \brief The PrefixInserter adds a prefix to each log message.
 */

#ifndef OCPILOGGERPREFIXINSERTER_H__
#define OCPILOGGERPREFIXINSERTER_H__

#include <OcpiLoggerLogger.h>
#include <string>

namespace OCPI {
  namespace Logger {

    /**
     * \brief The PrefixInserter adds a prefix to each log message.
     *
     * The PrefixInserter class implements the Logger interface.  All
     * log messages are delegated to a secondary Logger instance, after
     * inserting a prefix at the beginning of each message.  A
     * PrefixInserter is usually used locally, e.g., to identify the
     * particular operation that produced the message.
     *
     * When instances of PrefixInserter are chained (i.e., when the
     * "delegatee" is itself a PrefixInserter), the prefixes are
     * concatenated, innermost prefix first.
     */

    class PrefixInserter : public Logger {
    public:
      /** \cond */
      class PrefixInserterBuf : public LogBuf {
      public:
        PrefixInserterBuf (Logger *, const std::string &, bool);
        ~PrefixInserterBuf ();

        void setLogLevel (unsigned short);
        void setProducerId (const char *);
        void setProducerName (const char *);
        void setPrefix (const std::string &);

      protected:
        int sync ();
        int_type overflow (int_type = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);

      protected:
        bool m_first;
        bool m_adopted;
        Logger * m_logger;
        Logger::LogBuf & m_out;
        std::string m_prefix;
      };
      /** \endcond */

    public:
      /**
       * Constructor.
       *
       * The \a logger shall have a life span longer than this
       * PrefixInserter object.
       *
       * \param[in] logger The logger instance to delegate the prefixed
       *                   message to.
       * \param[in] prefix The prefix to insert at the beginning of each
       *                   log message.
       */

      PrefixInserter (Logger & logger, const std::string & prefix);

      /**
       * Constructor.
       *
       * If \a adopt is false, \a logger shall have a life span longer
       * than this PrefixInserter object.  If \a adopt is true, then
       * \a logger shall not be used after this object's destruction.
       *
       * \param[in] logger The logger instance to delegate the prefixed
       *                   message to.
       * \param[in] prefix The prefix to insert at the beginning of each
       *                   log message.
       * \param[in] adopt  Whether to adopt the \a logger.  If true,
       *                   then \a logger is deleted by this object's
       *                   destructor.  If false, then destruction of
       *                   this object has no effect on \a out.
       */

      PrefixInserter (Logger * logger, const std::string & prefix,
                      bool adopt = false);

      /**
       * Destructor.
       *
       * Deletes the delegatee logger, if adopted.
       */

      ~PrefixInserter ();

      /**
       * Change the prefix string.
       *
       * \param[in] prefix The new prefix to use for future log messages.
       */

      void setPrefix (const std::string & prefix);

    protected:
      PrefixInserterBuf m_obuf;
    };

  }
}

#endif
