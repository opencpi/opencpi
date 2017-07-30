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

#ifndef OCPIUTILHTTPSERVER_H__
#define OCPIUTILHTTPSERVER_H__

/**
 * \file
 * \brief Implementation of the server-side HTTP protocol.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiUtilVfs.h>
#include <OcpiUtilHttpMisc.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerNullOutput.h>

namespace OCPI {
  namespace Util {
    namespace Http {

      /**
       * \brief Implementation of the server-side HTTP protocol.
       *
       * This class implements an HTTP 1.1 server connection, i.e., it
       * manages the data exchange with a single client.  This class
       * expects generic std::istream and std::ostream data streams
       * for reading data from and writing data to the client.  The
       * run() operation processes one or more requests from the client,
       * and eventually completes when the connection is closed by
       * the client.  After request processing is complete, the
       * connection can be reset to serve another client.
       *
       * Connection management must be accomplished outside of this
       * class.
       *
       * An instance of type OCPI::Util::Vfs::Vfs is used to read and
       * write files as requested by the client.
       */

      class Server {
      protected:
        /**
         * Server limits.  These should probably be configurable.
         */

        enum {
          MAX_NUM_OF_HEADER_LINES = 256,
          MAX_LENGTH_OF_HEADER_LINE = 4096,
          MAX_UPLOAD_SIZE = 10 * 1024 * 1024
        };

        /** \cond */

        static const std::streamsize DATA_BUFFER_SIZE = 32768;

        enum {
          REQUEST_UNKNOWN,
          REQUEST_OPTIONS,
          REQUEST_GET,
          REQUEST_HEAD,
          REQUEST_POST,
          REQUEST_PUT,
          REQUEST_DELETE,
          REQUEST_TRACE,
          REQUEST_CONNECT
        };

        enum {
          STATE_REQUEST,
          STATE_HEADERS,
          STATE_UPLOAD,
          STATE_DOWNLOAD,
          STATE_CLOSE,
          STATE_BROKEN
        };

        /** \endcond */

      public:
        /**
         * Constructor.
         *
         * \param[in] fs The file system to use for accessing files
         *               requested by clients.
         * \param[in] logger A logger to print log messages to.
         *               If 0, no messages are logged.
         *
         * \pre The server is idle and unconnected.
         */

        Server (OCPI::Util::Vfs::Vfs * fs,
                OCPI::Logger::Logger * logger = 0)
          throw ();

        /**
         * Destructor.
         */

        ~Server ()
          throw ();

        /**
         * Change the log message destination.
         *
         * \param[in] logger A logger to print log messages to.
         *               If 0, no messages are logged.
         *
         * \pre The server is idle.
         */

        void setLogger (OCPI::Logger::Logger * logger)
          throw ();

        /**
         * Change the file system to use for file access.
         *
         * \param[in] fs The new file system.
         *
         * \pre The server is idle.
         */

        void setRoot (OCPI::Util::Vfs::Vfs * fs)
          throw ();

        /**
         * Configure the connection to communicate with the client.
         *
         * Both \a in and \a out may point to the same std::iostream
         * instance.
         *
         * \param[in] in  The stream for incoming data, i.e., for reading
         *                data sent by the client.
         * \param[in] out The stream for outgoing data, i.e., for sending
         *                data to the client.
         *
         * \pre The server is idle.
         * \post The server is idle and connected.
         */

        void resetConn (std::istream * in, std::ostream * out)
          throw ();

        /**
         * Process requests.
         *
         * Reads client requests from the input stream, processes
         * the requests, and sends responses to the output stream.
         *
         * This operation may block; during this time, the server
         * is not idle.
         *
         * \return true if the requests completed successfully, false
         *         if not (e.g., if the connection was broken).
         *
         * \pre The server is idle and connected.
         * \post The server is idle and unconnected.
         */

        bool run ()
          throw ();

      protected:
        /** \cond */
        bool processGetOrHeadRequest ()
          throw ();

        bool processPutRequest ()
          throw ();

        bool receiveRequestLine ()
          throw ();

        bool receiveRequestHeaders ()
          throw ();

        void sendError (const char *, const char *,
                        const char * = 0)
          throw ();
        /** \endcond */

      protected:
        /** \cond */
        std::istream * m_inConn;
        std::ostream * m_outConn;
        OCPI::Logger::Logger * m_logger;
        OCPI::Util::Vfs::Vfs * m_fs;

        /*
         * Current state
         */

        int m_state;
        bool m_closeConnection;

        /*
         * Request header
         */

        int m_requestType;
        std::string m_requestLine;
        std::string m_requestMethod;
        std::string m_requestURI;
        int m_majorVersion, m_minorVersion;
        Headers m_requestHeaders;
        /** \endcond */

      protected:
        /** \cond */
        static OCPI::Logger::NullOutput g_myDefaultLogger;
        static OCPI::Logger::ProducerName g_myProducerName;
        /** \endcond */

      private:
        /**
         * Not implemented.
         */

        Server (const Server &);

        /**
         * Not implemented.
         */

        Server & operator= (const Server &);
      };

    }
  }
}

#endif
