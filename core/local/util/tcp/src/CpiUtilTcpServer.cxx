#include <CpiUtilTcpServer.h>
#include <CpiUtilTcpStream.h>
#include <CpiOsServerSocket.h>
#include <CpiOsSocket.h>
#include <string>

/*
 * ----------------------------------------------------------------------
 * CPI::Util::Tcp::Server
 * ----------------------------------------------------------------------
 */

CPI::Util::Tcp::Server::Server ()
  throw ()
{
  m_open = false;
}

CPI::Util::Tcp::Server::Server (unsigned int portno, bool reuse)
  throw (std::string)
{
  m_open = false;
  bind (portno, reuse);
}

CPI::Util::Tcp::Server::~Server ()
  throw ()
{
  if (m_open) {
    close ();
  }
}

void
CPI::Util::Tcp::Server::bind (unsigned int portNo, bool reuse)
  throw (std::string)
{
  if (m_open) {
    throw std::string ("already bound");
  }

  m_socket.bind (portNo, reuse);
  m_open = true;
}

CPI::Util::Tcp::Stream *
CPI::Util::Tcp::Server::accept (unsigned long timeout)
  throw (std::string)
{
  if (!m_open) {
    throw std::string ("not bound");
  }

  if (!m_socket.wait (timeout)) {
    return 0;
  }

  CPI::OS::Socket conn = m_socket.accept ();
  return new CPI::Util::Tcp::Stream (conn);
}

unsigned int
CPI::Util::Tcp::Server::getPortNo ()
  throw (std::string)
{
  if (!m_open) {
    throw std::string ("not bound");
  }

  return m_socket.getPortNo ();
}

void
CPI::Util::Tcp::Server::close ()
  throw (std::string)
{
  if (!m_open) {
    throw std::string ("not bound");
  }

  m_open = false;
  m_socket.close ();
}
