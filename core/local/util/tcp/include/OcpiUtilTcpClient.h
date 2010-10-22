
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

#ifndef OCPIUTILTCPCLIENT_H__
#define OCPIUTILTCPCLIENT_H__

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

#include <OcpiUtilTcpStream.h>
#include <string>

namespace OCPI {
  namespace Util {
    /**
     * \brief Data streams over TCP/IP.
     */

    namespace Tcp {

      /**
       * \brief Client-side TCP/IP data stream to connect to a remote port.
       *
       * Specializes the OCPI::Util::Tcp::Stream class, allowing to connect
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
