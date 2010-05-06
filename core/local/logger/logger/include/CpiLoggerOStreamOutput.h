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
 * \brief The OStreamOutput logger to log to an std::ostream.
 */

#ifndef CPILOGGEROSTREAMOUTPUT_H__
#define CPILOGGEROSTREAMOUTPUT_H__

#include <CpiOsMutex.h>
#include <CpiLoggerLogger.h>

namespace CPI {
  namespace Logger {

    /**
     * \brief The OStreamOutput logger to log to an std::ostream.
     *
     * The OStreamOutput class implements the Logger interface.  All
     * log messages are written to a <em>std::ostream</em> output stream.
     *
     * A mutex is used to synchronize output if the same OStreamOutput
     * instance is used by multiple threads.  This synchronization does
     * not work if multiple OStreamOutput objects are constructed to
     * reference the same stream, or if the stream is used concurrently
     * with the OStreamOutput object.
     */

    class OStreamOutput : public Logger {
    public:
      /** \cond */
      class OStreamOutputBuf : public LogBuf {
      public:
        OStreamOutputBuf (std::ostream *, bool);
        ~OStreamOutputBuf ();

        void setLogLevel (unsigned short);
        void setProducerId (const char *);
        void setProducerName (const char *);

      protected:
        int sync ();
        int_type overflow (int_type = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);

      protected:
        bool m_first;
        bool m_locked;
        unsigned short m_logLevel;
        std::string m_producerId;
        std::string m_producerName;
        std::ostream * m_out;
        bool m_adopted;
        CPI::OS::Mutex m_lock;
      };
      /** \endcond */

    public:
      /**
       * Constructor.
       *
       * The output stream \a out shall have a life span longer than
       * this OStreamOutput object.
       *
       * \param[in] out The output stream to log messages to.
       */

      OStreamOutput (std::ostream & out);

      /**
       * Constructor.
       *
       * If \a adopt is false, \a out shall have a life span longer
       * than this OStreamOutput object.  If \a adopt is true, then
       * \a out shall not be used after this object's destruction.
       *
       * \param[in] out The output stream to log messages to.
       * \param[in] adopt Whether to adopt the output stream.  If true,
       *                  then \a out is deleted by this object's
       *                  destructor.  If false, then destruction of
       *                  this object has no effect on \a out.
       */

      OStreamOutput (std::ostream * out, bool adopt);

      /**
       * Destructor.
       *
       * Deletes the output stream, if it was adopted.
       *
       * Behavior is undefined if the object is destructed while a
       * message is being produced, i.e., if a log level has been
       * set but the message has not been flushed yet.
       */

      ~OStreamOutput ();

    protected:
      OStreamOutputBuf m_obuf;
    };

  }
}

#endif
