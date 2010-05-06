// -*- c++ -*-

#ifndef CPIUTILTCPSERVER_H__
#define CPIUTILTCPSERVER_H__

/**
 * \file
 * \brief Listen to a local port and accept connections.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiUtilTcpStream.h>
#include <CpiOsServerSocket.h>

namespace CPI {
  namespace Util {
    namespace Tcp {

      /**
       * \brief Listen to a local port and accept connections.
       */

      class Server {
      public:
        /**
         * Contructor. Creates an unbound server socket.  Must be bound
         * using bind().
         *
         * \post The server socket is unbound.
         */

        Server ()
          throw ();

        /**
         * Constructor.
         *
         * Calls bind (portno, reuse).
         *
         * \param[in] portno The local port number to listen to, or 0.
         * \param[in] reuse  Whether to enforce reuse of the port.
         *
         * \throw std::string If the port number is not 0, the reuse
         * parameter is false, and the port is in use.
         * \throw std::string Operating system error.
         *
         * \post The server socket is bound.
         */

        Server (unsigned int portno, bool reuse = false)
          throw (std::string);

        /**
         * Destructor.  Calls close().
         */

        ~Server ()
          throw ();

        /**
         * Bind the socket.
         *
         * When \a portno is set to zero, the auto-selected port number
         * can be discovered using getPortNo().
         *
         * Many operating systems apply a timeout period to closed sockets,
         * during which data for recently-closed sockets is rejected.  This
         * may prevent servers from reusing their ports during a server
         * restart.  This mechanism is bypassed by enabling the \a reuse
         * option.
         *
         * \param[in] portno The local port number to listen to. If zero,
         *                   an available port is auto-selected.
         * \param[in] reuse  If \a portno is non-zero, whether to enforce
         *                   reuse of the port, if it is or has been in
         *                   recent use.
         *
         * \throw std::string If the port number is not 0, the reuse
         * parameter is false, and the port is in use.
         * \throw std::string Operating system error.
         *
         * \pre The server socket is unbound.
         * \post The server socket is bound.
         */

        void bind (unsigned int portno = 0, bool reuse = false)
          throw (std::string);

        /**
         * Wait for and accept a new connection.
         *
         * \param[in] timeout A timeout to wait for a new connection, in
         *                    milliseconds.  If 0, does not block.  If -1,
         *                    potentially waits forever.
         * \return A new CPI::Util::Tcp::Stream instance, if a client
         *         connected before the expiration of the \a timeout.
         *         Returns 0 if the timeout expired.
         *
         * throw std::string If a different thread called close() during
         * the timeout period.
         * \throw std::string Operating system error.
         *
         * \pre The server socket is bound.
         */

        CPI::Util::Tcp::Stream * accept (unsigned long timeout = static_cast<unsigned long> (-1))
          throw (std::string);

        /**
         * The port number that this server socket is bound to.  Returns
         * the \a port parameter to the bind() operation, if it was non-zero,
         * or the port number that was auto-selected.
         *
         * \a return The local port number.
         *
         * \pre The server socket is bound.
         */

        unsigned int getPortNo ()
          throw (std::string);

        /**
         * Close the server socket.
         *
         * If another thread is blocked in accept(), it is woken up and
         * raises an exception in its thread.
         *
         * \pre The server socket is bound.
         * \post The server socket is unbound.
         */

        void close ()
          throw (std::string);

      private:
        bool m_open;
        CPI::OS::ServerSocket m_socket;

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
