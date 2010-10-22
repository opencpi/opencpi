
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

#ifndef OCPIUTILTCPSTREAM_H__
#define OCPIUTILTCPSTREAM_H__

/**
 * \file
 * \brief Bidirectional TCP/IP data stream using the std::iostream interface.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiOsSocket.h>
#include <streambuf>
#include <iostream>
#include <string>

namespace OCPI {
  namespace Util {
    namespace Tcp {

      /**
       * \brief Bidirectional TCP/IP data stream using the std::iostream interface.
       *
       * This class implements a bidirectional data stream over TCP/IP,
       * using the std::iostream interface.  Instances of this class are
       * usually not created explicitly, but rather as a client-side
       * socket (see OCPI::Util::Tcp::Client, which derives from this
       * class), or when a server socket accepts a new connection (see
       * OCPI::Util::Tcp::Server and its OCPI::Util::Tcp::Server::accept()
       * operation).
       *
       * When the port is connected, data can be read and written using
       * the usual std::iostream interface.
       */

      class Stream : public std::iostream {
      public:
        /** \cond */

        /*
         * Internal helper class implementing the std::streambuf interface.
         */

        class StreamBuf : public std::streambuf  {
        protected:
          enum {
            INPUT_BUFFER_SIZE = 4096
          };

        public:
          StreamBuf ();
          StreamBuf (OCPI::OS::Socket &, std::ios_base::openmode);
          ~StreamBuf ();

          void setSocket (OCPI::OS::Socket &, std::ios_base::openmode);
          OCPI::OS::Socket & getSocket ();

        protected:
          int_type underflow ();
          int_type overflow (int_type);
          std::streamsize xsputn (const char *, std::streamsize);

        protected:
          char * m_inputBuffer;
          int_type m_inputBufferSize;
          std::ios_base::openmode m_mode;
          OCPI::OS::Socket m_socket;

        private:
          StreamBuf (const StreamBuf &);
          StreamBuf & operator= (const StreamBuf &);
        };

        /** \endcond */

      public:
        /**
         * Constructor. Creates an unconnected stream.
         *
         * \post The stream is unconnected, and must be connected using
         * the setSocket() operation.
         */

        Stream ()
          throw ();

        /**
         * Constructor. Creates a connected stream.
         *
         * \param[in] sock The connected socket.
         * \param[in] mode Whether the socket is open for reading
         *                 (std::ios_base::in), writing (std::ios_base::out)
         *                  or both (std::ios_base::in|std::ios_base::out).
         *
         * \post The stream is connected.
         * \post The socket \a sock shall not be used.
         */

        Stream (OCPI::OS::Socket & sock, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
          throw (std::string);

        /**
         * Destructor. Calls close() if connected.
         */

        ~Stream ()
          throw ();

        /*
         * Sets the connected socket, if the stream was not connected upon
         * construction.
         *
         * \param[in] sock The connected socket.
         * \param[in] mode Whether the socket is open for reading
         *                 (std::ios_base::in), writing (std::ios_base::out)
         *                  or both (std::ios_base::in|std::ios_base::out).
         *
         * \pre The stream is unconnected.
         * \post The stream is connected.
         * \post The socket \a sock shall not be used.
         */

        void setSocket (OCPI::OS::Socket & sock, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
          throw (std::string);

        /**
         * Configure the behavior upon close().  If the linger option is
         * true, then close() will block until the peer has acknowledged
         * receipt of all data.  If the linger option is false, then
         * close() returns immediately, leaving the potential for data
         * loss.
         *
         * \param[in] opt Whether to "linger" at close().
         *
         * \pre The stream is connected.
         */

        void linger (bool opt)
          throw (std::string);

        /**
         * Create a duplicate of the socket.
         *
         * The duplicate and the original can be used equivalently.  This may
         * be done, e.g., to use one copy for sending and one for receiving
         * data.
         *
         * \param[in] shutdownWhenClosed If true, then closing the duplicate
         *            implies shutting down the socket using \a shutdownMode.
         * \param[in] shutdownMode If \a shutdownWhenClosed is true, this
         *            parameter indicates whether to shutdown the receiving
         *            end (std::ios_base::in) end, or the sending end
         *            (std::ios_base::out) end.  This parameter is ignored
         *            if \a shutdownWhenClosed is false.
         * \return    The duplicate socket, which must eventually be
         *            deleted.
         *
         * \pre The stream is connected.
         */

        Stream * dup (bool shutdownWhenClosed = false,
                      std::ios_base::openmode shutdownMode = std::ios_base::out)
          throw (std::string);

        /**
         * Performs a half-close.
         *
         * Shutting down the sending end informs the peer that no more data
         * will be sent or received.  If the sending end is shut down, the
         * peer will receive an end of file indication when trying to read
         * from the stream.  If the receiving end is shut down, the peer
         * will be signaled a "broken pipe" error when trying to write to
         * the stream.  In both cases, data can still flow in the opposite
         * direction.
         *
         * For example, in an HTTP 1.0 connection, the client usually shuts
         * down the sending end of its socket after sending the request,
         * thus informing the server that the request is complete. The
         * client can then read the response from the socket.
         *
         * A half-close affects all duplicates of a socket.
         *
         * Note that shutting down both ends is not equivalent to closing
         * the socket, which must always be done separately.
         *
         * \param[in] mode Whether to close the sending end
         *                 (std::ios:base::out) or the
         *                 receiving end (std::ios_base::in).
         *
         * \pre The stream is connected.
         */

        void shutdown (std::ios_base::openmode mode = std::ios_base::out)
          throw (std::string);

        /**
         * Close the socket.
         *
         * If the linger option is enabled (see linger()), then close() blocks
         * util the peer has acknowledged reception of all data that was sent.
         * If the linger option is disabled, then close() does not wait and
         * succeeds immediately.  In the latter case, it is possible that an
         * error goes undetected, if the peer breaks while the local socket
         * still attempts to send unsent data.
         *
         * \throw std::string If the connection was broken and unsent data
         * remains in the socket's internal buffers.
         *
         * \pre The stream is connected.
         * \post The stream is unconnected.
         */

        void close ()
          throw (std::string);

        /**
         * The local port number.
         *
         * \return The port number of the local end of the connection.
         *
         * \pre The stream is connected.
         */

        unsigned int getPortNo ()
          throw (std::string);

        /**
         * The host name and port number of the remote peer.
         *
         * \param[out] host The name of remote endpoint's host, or its
         *                  dotted IP address, if no host name is available.
         * \param[out] port The port number of the remote end of the
         *                  connection.
         */

        void getPeerName (std::string & host, unsigned int & port)
          throw (std::string);

        /** \cond */

      protected:
        StreamBuf m_buf;

      protected:
        bool m_shutdownWhenClosed;
        std::ios_base::openmode m_mode;
        std::ios_base::openmode m_shutdownMode;

        /** \endcond */

      private:
        /**
         * Not implemented.
         */

        Stream (const Stream &);

        /**
         * Not implemented.
         */

        Stream & operator= (const Stream &);
      };

    }
  }
}

#endif
