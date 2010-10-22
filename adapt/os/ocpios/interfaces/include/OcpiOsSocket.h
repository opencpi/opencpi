
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

#ifndef OCPIOSSOCKET_H__
#define OCPIOSSOCKET_H__

/**
 * \file
 *
 * \brief Bidirectional communication between two peers.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Use 64-bit type for our opaque data, to ensure
 *                  alignment.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiOsDataTypes.h>
#include <string>

namespace OCPI {
  namespace OS {

    /**
     * \brief Bidirectional communication between two peers.
     *
     * Socket allows bidirectional communication between two peers.
     * Sockets can not be instantiated.  Socket instances are returned
     * by OCPI::OS::ClientSocket::connect() and
     * OCPI::OS::ServerSocket::accept().
     */

    class Socket {
    public:
      /**
       * Constructor: Initialize an unconnected socket.
       * 
       * The only way to "connect" a socket instance is using the
       * assignment operator, assigning this instance from a
       * socket that was returned from a ClientSocket::connect()
       * or ServerSocket::accept() operation.
       *
       * \post The socket is not connected.
       */

      Socket ()
        throw ();

      /**
       * Constructor: For internal use only.
       */

      Socket (const OCPI::OS::uint64_t * opaque)
        throw (std::string);

      /**
       * Copy constructor: Assigns ownership of the \a other socket to
       *                   this instance. After this, \a other may not
       *                   be used.
       *
       * \param[in] other A socket instance.
       *
       * \pre \a other shall be connected.
       * \post This socket is connected.  \a other is unconnected.
       */

      Socket (const Socket & other)
        throw ();

      /**
       * Assignment operator: Assigns ownership of the \a other socket to
       *                      this instance. After this, \a other may not
       *                      be used.
       *
       * \param[in] other A socket instance.
       *
       * \pre \a other shall be connected.
       * \post This socket is connected.  \a other is unconnected.
       */

      Socket & operator= (const Socket & other)
        throw ();

      /**
       * Destructor.
       *
       * \pre The socket shall be unconnected.
       */

      ~Socket ()
        throw ();

      /**
       * Receives data from the peer.
       *
       * Blocks until at least one octet can be read from the socket. Then
       * reads as much data as is available, up to at most \a amount octets.
       *
       * \param[out] buffer The memory location where to put the received data.
       * \param[in] amount  The maximum number of octets to read from the peer.
       * \return       The number of octets actually read, which may be
       *               less than "amount". A return value of zero indicates
       *               end of data (i.e., the other end has closed or shut
       *               down its end of the connection).
       *
       * \throw std::string In case of error, such as a broken connection.
       *
       * \pre The socket shall be connected.
       */

      unsigned long long recv (char * buffer, unsigned long long amount)
        throw (std::string);

      /**
       * Sends data to the peer.
       *
       * Blocks until at least one octet can be written to the socket. Then
       * writes as much data as is possible without blocking, up to at most
       * \a amount octets.
       *
       * \param[in] data    The data to send.
       * \param[in] amount  The maximum number of octets to send to the peer.
       * \return       The number of octets actually written, which may be
       *               less than \a amount.
       *
       * \throw std::string In case of error, such as a broken connection.
       *
       * \pre The socket shall be connected.
       */

      unsigned long long send (const char * data, unsigned long long amount)
        throw (std::string);

      /**
       * Returns the socket's local port number.
       *
       * \return The socket's local port number.
       *
       * \throw std::string Operating system error.
       *
       * \pre The socket shall be connected.
       */

      unsigned int getPortNo ()
        throw (std::string);

      /**
       * Returns the host name and the port number of the remote peer.
       *
       * \param[out] peerHost The name of the remote host.
       * \param[out] peerPort The port number of the peer on the remote host.
       *
       * \throw std::string Operating system error.
       *
       * \pre The socket shall be connected.
       */

      void getPeerName (std::string & peerHost,
                        unsigned int & peerPort)
        throw (std::string);

      /**
       * Configure behavior upon close().
       *
       * If the linger option is turned on (opt==true), then close() blocks
       * util all the peer has acknowledged reception of all data that was
       * sent to it, or throws an exception if the peer does not process
       * all data. If the linger option is turned off (opt==false), then
       * close() does not wait. In this case, it may be possible that an
       * error goes undetected, if the peer breaks after the connection
       * was closed.
       *
       * \param[in] opt Whether to "linger" at close().
       *
       * \throw std::string Operating system error.
       *
       * \pre The socket shall be connected.
       */

      void linger (bool opt = true)
        throw (std::string);

      /**
       * Performs a half-close.
       *
       * Shutting down the sending end informs the peer that no more data
       * will be sent (if the peer then calls recv() on its end, it will
       * return 0). The socket can continue receiving data. If the
       * receiving end is closed, and the peer sends more data, the peer
       * will be signaled a "broken pipe" error.
       *
       * For example, in HTTP 1.0 connections, the client usually shuts
       * down the sending end of its socket after sending the request,
       * thus informing the server of the request's completion. The
       * client can then read the response from the socket.
       *
       * Note that shutting down both ends is not equivalent to closing
       * the socket, which must always be done separately.
       *
       * \param[in] sendingEnd Whether to close the sending end (true) or the
       *                  receiving end (false).
       *
       * \throw std::string Operating system error.
       *
       * \pre The socket shall be connected.
       */

      void shutdown (bool sendingEnd = true)
        throw (std::string);

      /**
       * Closes (disconnects) the socket.
       *
       * A socket must be closed before it can be destructed.
       *
       * If the "linger" option is true, blocks while unsent data lingers
       * in the socket's internal buffers, until that data is acknowledged
       * by the peer.
       *
       * \throw std::string If unsent data remains in the socket's
       * buffers, and the peer stopped receiving data.
       *
       * \pre The socket shall be connected.
       * \post The socket is unconnected.
       */

      void close ()
        throw (std::string);

      /**
       * Creates a duplicate of the socket.
       *
       * The duplicate and the original can be used equivalently. This may
       * be done, e.g., to use one copy for sending and one for receiving
       * data.
       *
       * The shutdown() operation affects all duplicates of a socket. The
       * close() operation only affects one instance.
       *
       * \throw std::string Operating system error.
       *
       * \pre The socket shall be connected.
       * \post Both this socket and the newly created socket are connected.
       */

      Socket dup ()
        throw (std::string);

    protected:
      OCPI::OS::uint64_t m_osOpaque[1];
    };

  }
}

#endif
