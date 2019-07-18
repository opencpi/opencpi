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

#ifndef OCPIOSSERVERSOCKET_H__
#define OCPIOSSERVERSOCKET_H__

/**
 * \file
 *
 * \brief A server-side BSD-style socket.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include <cstddef>
#include <OcpiOsDataTypes.h>
#include <OcpiOsEther.h>
#include <OcpiOsSocket.h>
namespace OCPI {
  namespace OS {

    /**
     * \brief A server-side BSD-style socket.
     *
     * A server-side BSD socket. A server socket is bound to a local
     * port, and listens for client connections. When a client connects,
     * a new socket connection is created, which can be used for
     * bi-directional communication. After communication has been
     * established, both ends (client-side and server-side) are
     * equivalent.
     */

    class ServerSocket {
    public:

      /**
       * Constructor: Initializes an unbound server socket.
       *
       * \post The server socket is unbound.
       */

      ServerSocket ()
        throw ();

      /**
       * Constructor: Initializes the instance, then calls
       * #bind(\a portNo, \a reuse) to bind to a port.
       *
       * \param[in] portNo Port number to bind to.
       * \param[in] reuse Whether to reuse the port.
       *
       * \throw std::string See bind().
       *
       * \post The server socket is bound.
       */

      ServerSocket (uint16_t portNo, bool reuse = false)
        throw (std::string);

      /**
       * Destructor.
       *
       * \pre The server socket must be unbound (closed) before destruction.
       */

      ~ServerSocket ()
        throw ();

      /**
       * Retrieve the address of the server socket.
       */
      void getAddr(Ether::Address &addr);
      /**
       * Bind the server socket to a local port.
       *
       * \param[in] portNo The local port number to bind to. Must be a
       *              number less than 65536. If zero, the operating system
       *              auto-selects an available port.
       * \param[in] reuse  Whether to "reuse" the port if it is already
       *              in use.  Ignored if \a portNo is 0.
       *
       * \throw std::string if the server socket can not be bound.
       *
       * \pre The server socket is unbound.
       * \post The server socket is bound.
       *
       * \note Typically, an operating system reserves a port for a brief
       * period of time (in the magnitude of 30 seconds to 2 minutes) after
       * it has been in use, to catch any remaining packets that may still
       * be in flight.  If the \a reuse option is false, this may block
       * a server from restarting during this period.  This can be avoided
       * by enabling the \a reuse option.  However, enabling the \a reuse
       * option may cause unpredictable behavior if the port is in use by
       * an active process.
       */

      void
      bind (uint16_t portNo = 0, bool reuse = false, bool udp = false,
	    bool loopback = false)
        throw (std::string);

      /**
       * Returns the port number that the server socket is bound to.
       *
       * \return The port number that the server socket is bound to.
       *         This is the same number as the \a portNo that was passed
       *         to bind(), if different from zero. If \a portNo was zero,
       *         returns the port number selected by the operating system.
       *
       * \pre The server socket is bound.
       */

      uint16_t getPortNo ()
        throw (std::string);

      /**
       * Waits for and accepts a new client-side connection.
       *
       * If wait() returned true, accept() is guaranteed not to block.
       *
       * \return A new socket instance that can be used for communication
       *         to the connecting party.
       *
       * \throw std::string If accepting the connection fails, including
       * if the server socket is closed from another thread.
       *
       * \pre The server socket is bound.
       */

      void accept (OCPI::OS::Socket &sock)
        throw (std::string);

      /**
       * Waits for a connection request.
       *
       * \param[in] timeout A timeout, in milliseconds, to wait for a new
       *               connection request. If zero, does not block.
       *               The magic value -1 represents infinity.
       * \return       true if a client requested a connection before
       *               the timeout expired. false if the timeout
       *               expired, and no clients attempted connection.
       *
       * \throw std::string If the waiting failed, including if the
       * server socket is closed from another thread.
       *
       * \pre The server socket is bound.
       */

      bool wait(long timeout = -1)
        throw (std::string);

      /**
       * Closes (unbinds) the server socket.
       *
       * The server socket may be closed to interrupt another thread
       * that is blocked in wait(). In that case, wait() will raise
       * an exception.
       *
       * throw std::string Operating system error.
       *
       * \pre The server socket is bound.
       * \post The server socket is unbound.
       */

      void close ()
        throw (std::string);

      int fd()
	throw();
      size_t sendmsg (const void * iovect, int flags ) throw (std::string);
      size_t sendto (const char * data, size_t amount, int flags,  char * src_addr,
		     size_t addrlen) throw (std::string);
      size_t recvfrom(char  *buf, size_t amount, int flags, char *  src_addr,
		      size_t *addrlen, unsigned timeoutms = 0) throw (std::string);
    protected:
      OCPI::OS::uint64_t m_osOpaque[1];

    private:
      unsigned m_timeoutms;
      /**
       * Not implemented.
       */

      ServerSocket (const ServerSocket &);

      /**
       * Not implemented.
       */

      ServerSocket & operator= (const ServerSocket &);
    };

  }
}

#endif
