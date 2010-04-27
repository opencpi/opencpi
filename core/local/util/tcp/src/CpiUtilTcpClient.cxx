#include <CpiUtilTcpClient.h>
#include <CpiOsClientSocket.h>
#include <string>

/*
 * ----------------------------------------------------------------------
 * CPI::Util::Tcp::Client
 * ----------------------------------------------------------------------
 */

CPI::Util::Tcp::Client::Client ()
  throw ()
{
}

CPI::Util::Tcp::Client::Client (const std::string & host, unsigned int port)
    throw (std::string)
{
  connect (host, port);
}

CPI::Util::Tcp::Client::~Client ()
  throw ()
{
}

void
CPI::Util::Tcp::Client::connect (const std::string & host, unsigned int port)
    throw (std::string)
{
  CPI::OS::Socket conn = CPI::OS::ClientSocket::connect (host, port);
  setSocket (conn);
}
