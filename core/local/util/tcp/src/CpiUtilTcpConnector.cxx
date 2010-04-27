#include <CpiUtilTcpConnector.h>
#include <CpiUtilTcpClient.h>
#include <CpiUtilMisc.h>
#include <iostream>
#include <string>
#include <cstdlib>

/*
 * ----------------------------------------------------------------------
 * CPI::Util::Tcp::Connector
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Tcp::Connector::g_scheme = "http";

CPI::Util::Tcp::Connector::Connector ()
  throw ()
{
}

CPI::Util::Tcp::Connector::~Connector ()
  throw ()
{
}

std::iostream *
CPI::Util::Tcp::Connector::connect (const std::string & authority)
  throw (std::string)
{
  std::string::size_type pos = authority.rfind (':');
  std::string host, portAsString;
  unsigned int portno;

  if (pos == std::string::npos) {
    host = authority;
    portno = DEFAULT_PORT;
  }
  else {
    host = authority.substr (0, pos);
    portAsString = authority.substr (pos+1);
    portno = std::atoi (portAsString.c_str());

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
    reason += CPI::Util::Misc::integerToString (portno);
    reason += ": ";
    reason += connReason;
    throw reason;
  }

  return &m_socket;
}

void
CPI::Util::Tcp::Connector::shutdown (std::ios_base::openmode mode)
  throw (std::string)
{
  m_socket.shutdown (mode);
}

void
CPI::Util::Tcp::Connector::close ()
  throw (std::string)
{
  m_socket.close ();
}

