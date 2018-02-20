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

#ifndef OCPIUTILTCPCONNECTOR_H__
#define OCPIUTILTCPCONNECTOR_H__

/**
 * \file
 * \brief Implementation of the <em>Connector</em> pattern using TCP/IP streams.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiUtilTcpClient.h>
#include <iostream>
#include <string>

namespace OCPI {
  namespace Util {
    namespace Tcp {

      /**
       * \brief Implementation of the <em>Connector</em> pattern using TCP/IP streams.
       *
       * Implements the <em>Connector</em> pattern using TCP/IP streams.
       * This class can be used with, e.g., the OCPI::Util::Http::Client
       * and the OCPI::Util::Http::HttpFs classes to connect to remote
       * HTTP servers.
       */

      class Connector {
      public:
        enum {
          DEFAULT_PORT = 80
        };

      public:
        /**
         * Constructor.
         *
         * \post The connector is unconnected.
         */

        Connector ()
          throw ();

        /**
         * Destructor.
         *
         * \pre The connector is unconnected.
         */

        ~Connector ()
          throw ();

        /**
         * Connect to a remote \a authority, which must be in appropriate
         * URI format, i.e., the string must be composed of a hostname and
         * an optional port number separated by a colon ("host:port").
         * If the port number is omitted, a default of 80 is assumed.
         *
         * \param[in] authority The authority to connect to, in URI format.
         * \return A stream for communication with the authority.
         *
         * \throw std::string If connection to the authority fails.
         *
         * \pre The connector is unconnected.
         * \post The connector is connected.
         */

        std::iostream * connect (const std::string & authority)
          throw (std::string);

        /**
         * Performs a half-close.  See OCPI::Util::Tcp::Stream::shutdown()
         * for more information.
         *
         * \param[in] mode Whether to close the sending end
         *                 (std::ios:base::out) or the
         *                 receiving end (std::ios_base::in).
         *
         * \pre The connector is connected.
         */

        void shutdown (std::ios_base::openmode mode = std::ios_base::out)
          throw (std::string);

        /**
         * Closes the connection.
         *
         * \pre The connector is connected.
         * \post The connector is unconnected.
         */

        void close ()
          throw (std::string);

        /**
         * The static string, "http".
         */

        static std::string g_scheme;

      protected:
        OCPI::Util::Tcp::Client m_socket;

      private:
        /**
         * Not implemented.
         */

        Connector (const Connector &);

        /**
         * Not implemented.
         */

        Connector & operator= (const Connector &);
      };

    }
  }
}

#endif
