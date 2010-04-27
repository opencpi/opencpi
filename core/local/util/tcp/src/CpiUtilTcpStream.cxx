/*
 * Bidirectional TCP/IP data stream using the std::iostream interface.
 *
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  Bugfix: don't pass pointer to uninitialized member to
 *                  std::iostream.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <CpiUtilTcpStream.h>
#include <CpiOsAssert.h>
#include <CpiOsSocket.h>
#include <streambuf>
#include <iostream>
#include <string>

/*
 * ----------------------------------------------------------------------
 * CPI::Util::Tcp::Stream::StreamBuf
 * ----------------------------------------------------------------------
 */

CPI::Util::Tcp::Stream::StreamBuf::
StreamBuf ()
{
  m_inputBuffer = 0;
  m_inputBufferSize = 0;
}

CPI::Util::Tcp::Stream::StreamBuf::
StreamBuf (CPI::OS::Socket & sock, std::ios_base::openmode mode)
{
  m_mode = std::ios_base::binary & ~std::ios_base::binary;;
  m_inputBuffer = 0;
  m_inputBufferSize = 0;
  setSocket (sock, mode);
}

CPI::Util::Tcp::Stream::StreamBuf::
~StreamBuf ()
{
  delete [] m_inputBuffer;
}

void
CPI::Util::Tcp::Stream::StreamBuf::
setSocket (CPI::OS::Socket & sock, std::ios_base::openmode mode)
{
  m_mode = mode;
  m_socket = sock;
  setg (0, 0, 0);
}

CPI::OS::Socket &
CPI::Util::Tcp::Stream::StreamBuf::
getSocket ()
{
  return m_socket;
}

std::streambuf::int_type
CPI::Util::Tcp::Stream::StreamBuf::
underflow ()
{
  /*
   * Opened for reading?
   */
     
  if (!(m_mode & std::ios_base::in)) {
    return traits_type::eof ();
  }

  /*
   * Is there any reason why underflow is being called?
   */
    
  if (gptr() && gptr() < egptr()) {
    return traits_type::to_int_type (*gptr());
  }
    
  /*
   * Allocate a buffer if there is none yet
   */

  if (!m_inputBuffer) {
    m_inputBuffer = new char_type[INPUT_BUFFER_SIZE];
    m_inputBufferSize = INPUT_BUFFER_SIZE;
  }

  /*
   * Read some data
   */

  unsigned long long count;

  try {
    count = m_socket.recv (m_inputBuffer, m_inputBufferSize);
  }
  catch (const std::string &) {
    return traits_type::eof ();
  }

  if (count == 0) {
    return traits_type::eof ();
  }

  setg (m_inputBuffer, m_inputBuffer, m_inputBuffer + count);
  return traits_type::to_int_type (*m_inputBuffer);
}

std::streambuf::int_type
CPI::Util::Tcp::Stream::StreamBuf::
overflow (int_type i)
{
  /*
   * Opened for writing?
   */
     
  if (!(m_mode & std::ios_base::out)) {
    return traits_type::eof ();
  }

  if (traits_type::eq_int_type (i, traits_type::eof())) {
    return traits_type::not_eof (i);
  }

  char c = traits_type::to_char_type (i);
  unsigned long long count;

  try {
    count = m_socket.send (&c, 1);
  }
  catch (const std::string &) {
    return traits_type::eof ();
  }

  if (count != 1) {
    return traits_type::eof ();
  }

  return i;
}

std::streamsize
CPI::Util::Tcp::Stream::StreamBuf::
xsputn (const char * s, std::streamsize n)
{
  /*
   * Opened for writing?
   */

  if (!(m_mode & std::ios_base::out)) {
    return traits_type::eof ();
  }
     
  std::streamsize remaining = n;
  unsigned long long count;

  while (remaining > 0) {
    try {
      count = m_socket.send (s, remaining);
    }
    catch (const std::string &) {
      return n - remaining;
    }

    if (count == 0) {
      return n - remaining;
    }

    unsigned int icount = static_cast<unsigned int> (count);
    cpiAssert (static_cast<unsigned long long> (icount) == count);

    remaining -= icount;
    s += count;
  }

  return n - remaining;
}

/*
 * ----------------------------------------------------------------------
 * CPI::Util::Tcp::Stream
 * ----------------------------------------------------------------------
 */

CPI::Util::Tcp::Stream::Stream ()
  throw ()
  : std::iostream (0)
{
  this->init (&m_buf);
  m_mode = std::ios_base::binary & ~std::ios_base::binary;
}

CPI::Util::Tcp::Stream::Stream (CPI::OS::Socket & sock,
				std::ios_base::openmode mode)
  throw (std::string)
  : std::iostream (0)
{
  this->init (&m_buf);
  m_mode = std::ios_base::binary & ~std::ios_base::binary;;
  setSocket (sock, mode);
}

CPI::Util::Tcp::Stream::~Stream ()
  throw ()
{
  if (m_mode) {
    close ();
  }
}

void
CPI::Util::Tcp::Stream::setSocket (CPI::OS::Socket & sock,
				   std::ios_base::openmode mode)
  throw (std::string)
{
  if (!mode) {
    throw std::string ("invalid mode");
  }

  m_mode = mode;
  m_shutdownWhenClosed = false;
  m_buf.setSocket (sock, mode);
}

void
CPI::Util::Tcp::Stream::linger (bool opt)
  throw (std::string)
{
  if (!m_mode) {
    throw std::string ("not connected");
  }
  m_buf.getSocket().linger (opt);
}

CPI::Util::Tcp::Stream *
CPI::Util::Tcp::Stream::dup (bool shutdownWhenClosed,
			     std::ios_base::openmode shutdownMode)
  throw (std::string)
{
  if (!m_mode) {
    throw std::string ("not connected");
  }
  CPI::OS::Socket duped = m_buf.getSocket().dup();
  CPI::Util::Tcp::Stream * newStream =
    new CPI::Util::Tcp::Stream (duped, m_mode);
  newStream->m_shutdownWhenClosed = shutdownWhenClosed;
  newStream->m_shutdownMode = shutdownMode;
  return newStream;
}

void
CPI::Util::Tcp::Stream::shutdown (std::ios_base::openmode mode)
  throw (std::string)
{
  if (!m_mode) {
    throw std::string ("not connected");
  }

  if (!(mode & std::ios_base::in) && !(mode & std::ios_base::out)) {
    throw std::string ("bad mode");
  }

  if ((mode & std::ios_base::in)) {
    m_buf.getSocket().shutdown (false);
  }
  else if ((mode & std::ios_base::out)) {
    m_buf.getSocket().shutdown (true);
  }
}

void
CPI::Util::Tcp::Stream::close ()
  throw (std::string)
{
  if (!m_mode) {
    throw std::string ("not connected");
  }

  if (m_shutdownWhenClosed) {
    shutdown (m_shutdownMode);
  }

  m_mode = std::ios_base::binary & ~std::ios_base::binary;
  m_buf.getSocket().close ();
}

unsigned int
CPI::Util::Tcp::Stream::getPortNo ()
  throw (std::string)
{
  if (!m_mode) {
    throw std::string ("not connected");
  }

  return m_buf.getSocket().getPortNo ();
}

void
CPI::Util::Tcp::Stream::getPeerName (std::string & host, unsigned int & port)
  throw (std::string)
{ 
  if (!m_mode) {
    throw std::string ("not connected");
  }

  m_buf.getSocket().getPeerName (host, port);
}
