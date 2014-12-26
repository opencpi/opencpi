
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

#include <OcpiUtilTcpServer.h>
#include <OcpiUtilTcpStream.h>
#include <OcpiOsServerSocket.h>
#include <OcpiOsSocket.h>
#include <string>

/*
 * ----------------------------------------------------------------------
 * OCPI::Util::Tcp::Server
 * ----------------------------------------------------------------------
 */

OCPI::Util::Tcp::Server::Server ()
  throw ()
{
  m_open = false;
}

OCPI::Util::Tcp::Server::Server (uint16_t portno, bool reuse)
  throw (std::string)
{
  m_open = false;
  bind (portno, reuse);
}

OCPI::Util::Tcp::Server::~Server ()
  throw ()
{
  if (m_open) {
    close ();
  }
}

void
OCPI::Util::Tcp::Server::bind (uint16_t portNo, bool reuse)
  throw (std::string)
{
  if (m_open) {
    throw std::string ("already bound");
  }

  m_socket.bind (portNo, reuse);
  m_open = true;
}

OCPI::Util::Tcp::Stream *
OCPI::Util::Tcp::Server::accept (unsigned long timeout)
  throw (std::string)
{
  if (!m_open) {
    throw std::string ("not bound");
  }

  if (!m_socket.wait (timeout)) {
    return 0;
  }

  OCPI::OS::Socket conn;
  m_socket.accept(conn);
  return new OCPI::Util::Tcp::Stream(conn);
}

uint16_t
OCPI::Util::Tcp::Server::getPortNo ()
  throw (std::string)
{
  if (!m_open) {
    throw std::string ("not bound");
  }

  return m_socket.getPortNo ();
}

void
OCPI::Util::Tcp::Server::close ()
  throw (std::string)
{
  if (!m_open) {
    throw std::string ("not bound");
  }

  m_open = false;
  m_socket.close ();
}
