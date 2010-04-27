// -*- c++ -*-

#ifndef CPIUTILTCPCLIENT_H__
#define CPIUTILTCPCLIENT_H__

/**
 * \file
 * \brief Client-side TCP/IP data stream to connect to a remote port.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiUtilTcpStream.h>
#include <string>

namespace CPI {
  namespace Util {
    /**
     * \brief Data streams over TCP/IP.
     */

    namespace Tcp {

      /**
       * \brief Client-side TCP/IP data stream to connect to a remote port.
       *
       * Specializes the CPI::Util::Tcp::Stream class, allowing to connect
       * to a remote port.  Indirectly inherits std::iostream.  After the
       * connection is established, data can be read and written using the
       * usual std::iostream interface.
       */

      class Client : public Stream {
      public:
	/**
	 * Constructor.
	 *
	 * This class creates an unconnected instance.  connect() must be
	 * called to connect to a remote port.
	 *
	 * \post The stream is unconnected.
	 */

	Client ()
	  throw ();

	/**
	 * Constructor.  Creates a socket that is connected to a remote peer.
	 *
	 * Calls connect (\a host, \a port).
	 *
	 * \param[in] host The name or dotted IP address of the remote host
	 *                 to connect to.
	 * \param[in] port The port number of the remote service to connect
	 *                 to.
	 *
	 * \post The stream is connected.
	 *
	 * \throw std::string If the connection can not be established.
	 */

	Client (const std::string & host, unsigned int port)
	  throw (std::string);

	/**
	 * Destructor.
	 */

	~Client ()
	  throw ();

	/**
	 * Connect the socket, if it was not connected upon construction.
	 *
	 * \param[in] host The name or dotted IP address of the remote host
	 *                 to connect to.
	 * \param[in] port The port number of the remote service to connect
	 *                 to.
	 *
	 * \post The stream is connected.
	 *
	 * \throw std::string If the connection can not be established.
	 */

	void connect (const std::string & host, unsigned int port)
	  throw (std::string);

      private:
	/**
	 * Not implemented.
	 */

	Client (const Client &);

	/**
	 * Not implemented.
	 */

	Client & operator= (const Client &);
      };

    }
  }
}

#endif
