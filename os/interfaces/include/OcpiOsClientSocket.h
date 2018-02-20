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

#ifndef OCPIOSCLIENTSOCKET_H__
#define OCPIOSCLIENTSOCKET_H__

/**
 * \file
 *
 * \brief Operations related to client-side BSD sockets.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include "OcpiOsSocket.h"

namespace OCPI {
  namespace OS {

    /**
     * \brief Client-side socket functionality.
     */

    namespace ClientSocket {

      /**
       * Creates a new socket that is connected to a remote socket.
       *
       * \param[in] remoteHost The host name or IP address of the remote host.
       * \param[in] remotePort The port number of the socket to connect to on
       *                  the remote host.
       * \return A new socket instance if successful.
       *
       * \throw std::string If unsuccessful to open a socket or to connect
       * to the remote host.  The string contains a human-readable error
       * message.
       */

      OCPI::OS::Socket connect (const std::string & remoteHost,
				uint16_t remotePort,
				bool udp = false)
        throw (std::string);

    };

  }
}

#endif
