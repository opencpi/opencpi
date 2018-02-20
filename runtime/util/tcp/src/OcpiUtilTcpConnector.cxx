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

#include <OcpiUtilTcpConnector.h>
#include <OcpiUtilTcpClient.h>
#include <OcpiUtilMisc.h>
#include <iostream>
#include <string>
#include <cstdlib>

/*
 * ----------------------------------------------------------------------
 * OCPI::Util::Tcp::Connector
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Tcp::Connector::g_scheme = "http";

OCPI::Util::Tcp::Connector::Connector ()
  throw ()
{
}

OCPI::Util::Tcp::Connector::~Connector ()
  throw ()
{
}

std::iostream *
OCPI::Util::Tcp::Connector::connect (const std::string & authority)
  throw (std::string)
{
  std::string::size_type pos = authority.rfind (':');
  std::string host, portAsString;
  uint16_t portno;

  if (pos == std::string::npos) {
    host = authority;
    portno = DEFAULT_PORT;
  }
  else {
    host = authority.substr (0, pos);
    portAsString = authority.substr (pos+1);
    portno = (uint16_t)std::atoi (portAsString.c_str());

    if (portno == 0) {
      std::string reason = "bad port number: \"";
      reason += portAsString;
      reason += "\"";
      throw reason;
    }
  }

  try {
    m_socket.connect (host.c_str(), portno);
  }
  catch (const std::string & connReason) {
    std::string reason = "cannot connect to ";
    reason += host;
    reason += ":";
    reason += OCPI::Util::integerToString (portno);
    reason += ": ";
    reason += connReason;
    throw reason;
  }

  return &m_socket;
}

void
OCPI::Util::Tcp::Connector::shutdown (std::ios_base::openmode mode)
  throw (std::string)
{
  m_socket.shutdown (mode);
}

void
OCPI::Util::Tcp::Connector::close ()
  throw (std::string)
{
  m_socket.close ();
}

