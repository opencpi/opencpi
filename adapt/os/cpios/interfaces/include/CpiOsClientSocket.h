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

#ifndef CPIOSCLIENTSOCKET_H__
#define CPIOSCLIENTSOCKET_H__

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
#include "CpiOsSocket.h"

namespace CPI {
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

      CPI::OS::Socket connect (const std::string & remoteHost,
                               unsigned int remotePort)
        throw (std::string);

    };

  }
}

#endif
