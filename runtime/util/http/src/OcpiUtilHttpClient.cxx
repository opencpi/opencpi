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

/*
 * There are plenty possibilities for improvement. For example, connections
 * are not reused. A Really Good implementation would have a pool of
 * connections that are kept open for a short while, in case a new request
 * comes along. While a connection is in use, new requests would wait
 * briefly, else open a new connection to the server. We could also cache
 * information about whether a server supports HTTP/1.1 or not.
 *
 * At the moment, uploads use HTTP/1.0. This is done because if we cannot
 * find out whether a server supports HTTP/1.1 chunking before a request
 * is complete. So we would need to send a HEAD request first to find out,
 * following up with the PUT/POST request if the server is 1.1 - but else
 * we'd have to close and reconnect.
 *
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  Bugfix: don't pass pointer to uninitialized member to
 *                  std::iostream.
 *
 *     11/10/2008 - Frank Pilhofer
 *                  Bugfix: fix POST request.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiUtilHttpClient.h>
#include <OcpiUtilHttpMisc.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilMisc.h>
#include <streambuf>
#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <cstdlib>
#include <cctype>

/*
 * ----------------------------------------------------------------------
 * Some constants
 * ----------------------------------------------------------------------
 */

const std::string
OCPI::Util::Http::ClientStream::userAgent = "mcClient/0.1";

const std::string
OCPI::Util::Http::ClientStream::defaultContentType = "application/octet-stream";

const std::string
OCPI::Util::Http::ClientStream::defaultCharset = "iso-8859-1";

/*
 * ----------------------------------------------------------------------
 * Exceptions
 * ----------------------------------------------------------------------
 */

OCPI::Util::Http::Redirection::Redirection (int c, const std::string & x)
  : statusCode (c), newLocation (x)
{
}

OCPI::Util::Http::Redirection::operator const std::string & () const
{
  return newLocation;
}

OCPI::Util::Http::HttpError::HttpError (int code,
                                       const std::string & phrase,
                                       const std::string & b)
  : statusCode (code),
    reasonPhrase (phrase),
    body (b)
{
}

OCPI::Util::Http::ClientError::ClientError (int code,
                                           const std::string & phrase,
                                           const std::string & b)
  : HttpError (code, phrase, b)
{
}

OCPI::Util::Http::ServerError::ServerError (int code,
                                           const std::string & phrase,
                                           const std::string & b)
  : HttpError (code, phrase, b)
{
}

/*
 * ----------------------------------------------------------------------
 * OCPI::Util::Http::ClientStream::ClientBuf
 * ----------------------------------------------------------------------
 */

OCPI::Util::Http::ClientStream::ClientBuf::
ClientBuf ()
  throw ()
{
  m_inConn = 0;
  m_outConn = 0;
  m_state = STATE_NOT_CONNECTED;
}

OCPI::Util::Http::ClientStream::ClientBuf::
~ClientBuf ()
  throw ()
{
}

void
OCPI::Util::Http::ClientStream::ClientBuf::
setConn (std::istream * in,
         std::ostream * out,
         void (*shutdown) (void *),
         void * sdopaque)
  throw ()
{
  m_statusCode = 0;
  m_inConn = in;
  m_outConn = out;
  m_chunked = false;
  m_haveLast = false;
  m_havePending = false;
  m_shutdown = shutdown;
  m_sdopaque = sdopaque;
  m_state = STATE_SEND_REQUEST;
  m_closeConnection = false;
  m_majorVersion = 1;
  m_minorVersion = 1;
  m_responseHeaders.clear ();
}

void
OCPI::Util::Http::ClientStream::ClientBuf::
resetConn ()
  throw ()
{
  m_inConn = 0;
  m_outConn = 0;
  m_state = STATE_NOT_CONNECTED;
}

/*
 * Reset in order to start a new request
 */

void
OCPI::Util::Http::ClientStream::ClientBuf::
startRequest (int requestType)
  throw (std::string)
{
  if (!m_outConn || !m_outConn->good()) {
    throw std::string ("no connection");
  }
      
  if (m_state != STATE_SEND_REQUEST) {
    throw std::string ("not in the right mood");
  }

  m_requestType = requestType;
  m_responseHeaders.clear ();
}

void
OCPI::Util::Http::ClientStream::ClientBuf::
sendAnyRequest (const OCPI::Util::Uri & uri,
                const std::string & request,
                const Headers & requestHeaders)
  throw (std::string)
{
  Headers::const_iterator hit;

  /*
   * Don't send the request line in pieces, some servers don't like that.
   */

  std::string requestLine;

  switch (m_requestType) {
  case REQUEST_GET:
    requestLine = "GET ";
    break;

  case REQUEST_HEAD:
    requestLine = "HEAD ";
    break;
    
  case REQUEST_PUT:
    requestLine = "PUT ";
    break;

  case REQUEST_POST:
    requestLine = "POST ";
    break;

  case REQUEST_DELETE:
    requestLine = "DELETE ";
    break;
    
  default:
    throw std::string ("unsupported request type");
  };
  
  requestLine += request;
  requestLine += " HTTP/";
  requestLine += (char) ('0' + m_majorVersion);
  requestLine += ".";
  requestLine += (char) ('0' + m_minorVersion);
  requestLine += "\r\n";

  m_outConn->write (requestLine.data(), requestLine.length());

  if (requestHeaders.find ("Host") == requestHeaders.end()) {
    *m_outConn << "Host: "
               << OCPI::Util::Uri::decode (uri.getHost())
               << "\r\n";
  }

  if (requestHeaders.find ("User-Agent") == requestHeaders.end()) {
    *m_outConn << "User-Agent: " << userAgent << "\r\n";
  }
  
  for (hit = requestHeaders.begin();
       hit != requestHeaders.end() && m_outConn->good();
       hit++) {
    *m_outConn << (*hit).first << ": " << (*hit).second << "\r\n";
  }

  *m_outConn << "\r\n" << std::flush;
  
  if (!m_outConn->good()) {
    throw std::string ("broken connection");
  }

  if ((hit = requestHeaders.find ("Connection")) != requestHeaders.end() &&
      (*hit).second == "close") {
    m_closeConnection = true;
  }
}

/*
 * Receive and process status line
 */

void
OCPI::Util::Http::ClientStream::ClientBuf::
receiveStatus ()
  throw (std::string)
{
  m_status = OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);

  if (!m_inConn->good()) {
    throw std::string ("could not read status");
  }

  if (m_status.substr (0, 5) != "HTTP/") {
    std::string reason = "protocol error, expecting HTTP, got \"";
    reason += m_status;
    reason += "\"";
    throw reason;
  }

  std::string version = m_status.substr (5, 3);

  if (version == "1.1" && m_majorVersion == 1 && m_minorVersion > 0) {
    m_majorVersion = 1;
    m_minorVersion = 1;
  }
  else if (version == "1.0") {
    m_majorVersion = 1;
    m_minorVersion = 0;
    m_closeConnection = true;
  }
  else {
    std::string reason = "can not handle HTTP version ";
    reason += version;
    throw reason;
  }

  std::string code = m_status.substr (9, 3);

  if (code.length() != 3 ||
      (code[0] < '1' || code[0] > '5') ||
      (code[1] < '0' || code[1] > '9') ||
      (code[2] < '0' || code[2] > '9')) {
    std::string reason = "expected status code, got \"";
    reason += code;
    reason += "\"";
    throw reason;
  }

  m_statusCode = std::atoi (code.c_str());
  m_reason = m_status.substr (13);
}

/*
 * Read headers
 */

void
OCPI::Util::Http::ClientStream::ClientBuf::
receiveHeaders ()
  throw (std::string)
{
  std::string::size_type colonPos;
  std::string headerLine =
    OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);
  int count=0;

  while (m_inConn->good() && headerLine.length()) {
    if (++count > MAX_NUM_OF_HEADER_LINES) {
      std::string reason = "implementation limit reached: more than ";
      reason += OCPI::Util::integerToString (MAX_NUM_OF_HEADER_LINES);
      reason += " header lines";
      throw reason;
    }

    if ((colonPos = headerLine.find (':')) == std::string::npos) {
      std::string reason = "illegal header line: \"";
      reason += headerLine;
      reason += "\"";
      throw reason;
    }

    /*
     * Field name must be composed of valid characters
     */

    std::string fieldName = headerLine.substr (0, colonPos);

    if (!OCPI::Util::Http::isToken (fieldName)) {
      std::string reason = "illegal header line: \"";
      reason += headerLine;
      reason += "\"";
      throw reason;
    }

    /*
     * remove leading whitespace from fieldValue
     */

    std::string fieldValue;

    colonPos++;
    while (colonPos < headerLine.length() &&
           std::isspace (headerLine[colonPos])) {
      colonPos++;
    }

    fieldValue = headerLine.substr (colonPos);

    /*
     * If we saw this header before, concatenate its value
     */

    if (m_responseHeaders.find (fieldName) != m_responseHeaders.end()) {
      std::string newValue = m_responseHeaders[fieldName];
      newValue += ",";
      newValue += fieldValue;
      m_responseHeaders[fieldName] = newValue;
    }
    else {
      m_responseHeaders[fieldName] = fieldValue;
    }

    headerLine =
      OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);
  }

  if (!m_inConn->good ()) {
    throw std::string ("connection broken while reading headers");
  }

  Headers::const_iterator hit;
  if ((hit = m_responseHeaders.find ("Connection")) != m_responseHeaders.end() &&
      (*hit).second == "close") {
    m_closeConnection = true;
  }
}

void
OCPI::Util::Http::ClientStream::ClientBuf::
sendRequest (int requestType,
             const OCPI::Util::Uri & uri,
             const Headers & headers)
  throw (std::string)
{
  switch (requestType) {
  case REQUEST_GET:
  case REQUEST_HEAD:
  case REQUEST_DELETE:
    return sendGetOrHeadOrDeleteRequest (requestType, uri, headers);

  case REQUEST_PUT:
  case REQUEST_POST:
    return sendPutOrPostRequest (requestType, uri, headers);
  }
}

void
OCPI::Util::Http::ClientStream::ClientBuf::
sendGetOrHeadOrDeleteRequest (int requestType,
                              const OCPI::Util::Uri & uri,
                              const Headers & headers)
  throw (std::string)
{
  /*
   * Send request
   */

  startRequest (requestType);
  sendAnyRequest (uri, uri.getRequest(), headers);

  /*
   * Read status line and headers
   */

  do {
    receiveStatus ();
    receiveHeaders ();
  }
  while (m_statusCode >= 100 && m_statusCode < 200);

  /*
   * Is there a body to be read?
   */

  if ((m_statusCode >= 100 && m_statusCode < 200) || /* informational */
      m_statusCode == 204 || /* No Content */ 
      m_statusCode == 205 || /* Reset Content */
      m_statusCode == 304 || /* Not Modified */
      requestType == REQUEST_HEAD) { /* must not have a message body */
    /*
     * No body
     */

    if (m_closeConnection) {
      m_state = STATE_CLOSE;
    }
    else {
      m_state = STATE_SEND_REQUEST;
    }

    return;
  }

  /*
   * there is a body to be read
   */

  if (m_majorVersion == 1 && m_minorVersion >= 1 &&
      m_responseHeaders.find ("Transfer-Encoding") != m_responseHeaders.end () &&
      m_responseHeaders["Transfer-Encoding"] == "chunked") {
    m_chunked = true;
    m_firstChunk = true;
    m_chunkRemaining = 0;
  }
  else {
    m_chunked = false;
  }

  Headers::iterator clit = m_responseHeaders.find ("Content-Length");
  if (clit != m_responseHeaders.end ()) {
    const char * cl = (*clit).second.c_str();
    char * cep;
    m_contentLength = std::strtoul (cl, &cep, 10);

    if (cep && *cep) {
      m_contentLength = static_cast<unsigned long long> (-1);
      m_closeConnection = true;
    }
    else {
      m_contentRemaining = m_contentLength;
    }
  }
  else {
    m_contentLength = static_cast<unsigned long long> (-1);
  }

  if (!m_chunked && m_contentLength == static_cast<unsigned long long> (-1)) {
    m_closeConnection = true;
  }

  /*
   * Proceed with the download
   */
      
  m_state = STATE_DOWNLOAD;
}

void
OCPI::Util::Http::ClientStream::ClientBuf::
sendPutOrPostRequest (int requestType,
                      const OCPI::Util::Uri & uri,
                      const Headers & headers)
  throw (std::string)
{
  /*
   * We upload using HTTP/1.0 to avoid conflicts when trying to send a
   * file using HTTP/1.1 to an HTTP/1.0 server. Until we know that the
   * server is HTTP/1.1, we couldn't decide to use chunking or not.
   */

  m_majorVersion = 1;
  m_minorVersion = 0;
  m_closeConnection = true;

  /*
   * Send request
   */

  startRequest (requestType);
  sendAnyRequest (uri, uri.getRequest(), headers);

  /*
   * Prepare to upload message body
   */

  m_uploadRemaining = static_cast<unsigned long long> (-1);
  m_state = STATE_UPLOAD;
  m_chunked = false;
  m_contentLength = static_cast<unsigned long long> (-1);
}

void
OCPI::Util::Http::ClientStream::ClientBuf::
completeUpload ()
  throw (std::string)
{
  if (m_state != STATE_UPLOAD) {
    return;
  }

  /*
   * Flush first
   */

  sync ();

  /*
   * Shutdown sending end, if the other side wants an EOF to signal the
   * end of upload.
   */

  if (m_closeConnection && m_shutdown && !m_chunked) {
    m_shutdown (m_sdopaque);
  }

  /*
   * Todo: - send last chunk when uploading via chunked HTTP/1.1.
   *       - verify content length
   */

  do {
    try {
      receiveStatus ();
      receiveHeaders ();
    }
    catch (...) {
      m_state = STATE_CLOSE;
      throw;
    }
  }
  while (m_statusCode >= 100 && m_statusCode < 200);

  m_state = STATE_DOWNLOAD;
}

int
OCPI::Util::Http::ClientStream::ClientBuf::
getStatusCode ()
  throw (std::string)
{
  completeUpload ();
  return m_statusCode;
}

std::string
OCPI::Util::Http::ClientStream::ClientBuf::
getReasonPhrase ()
  throw (std::string)
{
  completeUpload ();
  return m_reason;
}

const OCPI::Util::Http::Headers &
OCPI::Util::Http::ClientStream::ClientBuf::
getResponseHeaders ()
  throw (std::string)
{
  completeUpload ();
  return m_responseHeaders;
}

unsigned long long
OCPI::Util::Http::ClientStream::ClientBuf::
getContentLength ()
  throw (std::string)
{
  completeUpload ();
  Headers::const_iterator it = m_responseHeaders.find ("Content-Length");

  if (it == m_responseHeaders.end()) {
    throw std::string ("no content-length");
  }

  /*
   * Must ignore content length if chunking is being used
   */

  if (m_chunked) {
    std::string reason = "content-length might be ";
    reason += (*it).second;
    reason += " but HTTP orders us to ignore this info when chunking";
    throw reason;
  }

  const std::string & cls = (*it).second;
  char * ep;

  unsigned long long contentLength = std::strtoul (cls.c_str(), &ep, 10);

  if (ep && *ep) {
    std::string reason = "content-length looks like \"";
    reason += (*it).second;
    reason += "\", which appears unparseable";
    throw reason;
  }

  return contentLength;
}

int
OCPI::Util::Http::ClientStream::ClientBuf::
sync ()
{
  if (m_state != STATE_UPLOAD) {
    return 0;
  }

  return (m_outConn->flush().good() ? 0 : -1);
}

std::streamsize
OCPI::Util::Http::ClientStream::ClientBuf::
showmanyc ()
{
  if (m_state == STATE_UPLOAD) {
    try {
      completeUpload ();
    }
    catch (...) {
      return static_cast<std::streamsize> (-1);
    }
  }

  return (m_havePending ? 1 : 0);
}

std::streambuf::int_type
OCPI::Util::Http::ClientStream::ClientBuf::
underflow_common (bool bump)
{
  if (m_state == STATE_UPLOAD) {
    try {
      completeUpload ();
    }
    catch (...) {
      return traits_type::eof ();
    }
  }

  if (m_state != STATE_DOWNLOAD) {
    return traits_type::eof ();
  }

  if (m_havePending) {
    if (bump) {
      m_haveLast = true;
      m_havePending = false;
    }
    return m_pending;
  }

  /*
   * Are we at the end of a chunk? If this was not the first chunk,
   * then we first have to consume the chunk's trailing CRLF
   */

  if (m_chunked && m_chunkRemaining == 0) {
    if (!m_firstChunk) {
      std::string trailingCRLF =
        OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);
      
      if (!m_inConn->good() || trailingCRLF.length()) {
        // "protocol error, chunk did not end in CRLF";
        m_state = STATE_CLOSE;
        return traits_type::eof ();
      }
    }
    else {
      m_firstChunk = false;
    }

    /*
     * Open the next chunk
     */

    std::string chunkSizeLine =
      OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);
    char * csep;

    m_chunkRemaining = std::strtoul (chunkSizeLine.c_str(), &csep, 16);

    if (csep && *csep) {
      // "protocol error, could not read chunk size";
      m_state = STATE_CLOSE;
      return traits_type::eof ();
    }

    if (m_chunkRemaining == 0) {
      /*
       * last-chunk, read trailer
       */

      try {
        receiveHeaders ();
      }
      catch (...) {
        // ignore, nothing we can do about it
      }

      m_state = STATE_CLOSE;
      return traits_type::eof ();
    }
  }

  /*
   * Read next character
   */

  int_type res;

  if (m_chunked && m_chunkRemaining == 0) {
    res = traits_type::eof ();
  }
  else if (!m_chunked &&
           m_contentLength != static_cast<unsigned long long> (-1) &&
           m_contentRemaining == 0) {
    res = traits_type::eof ();
  }
  else {
    if (bump) {
      m_haveLast = true;
      res = m_pending = m_inConn->get ();
    }
    else {
      res = m_inConn->peek ();
    }
  }

  /*
   * Is this the end?
   */

  if (traits_type::eq_int_type (res, traits_type::eof())) {
    if (m_closeConnection) {
      m_state = STATE_CLOSE;
    }
    else {
      m_state = STATE_SEND_REQUEST;
    }
    return res;
  }
  else {
    if (bump) {
      if (m_chunked) {
        m_chunkRemaining--;
      }
      else if (m_contentLength != static_cast<unsigned long long> (-1)) {
        m_contentRemaining--;
      }
    }
  }

  return res;
}

std::streambuf::int_type
OCPI::Util::Http::ClientStream::ClientBuf::
underflow ()
{
  return underflow_common (0);
}

std::streambuf::int_type
OCPI::Util::Http::ClientStream::ClientBuf::
uflow ()
{
  return underflow_common (1);
}

std::streambuf::int_type
OCPI::Util::Http::ClientStream::ClientBuf::
overflow (std::streambuf::int_type c)
{
  if (m_state != STATE_UPLOAD) {
    return traits_type::eof ();
  }

  if (traits_type::eq_int_type (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  m_outConn->put (traits_type::to_char_type (c));
  return (m_outConn->good() ? c : traits_type::eof());
}

std::streambuf::int_type
OCPI::Util::Http::ClientStream::ClientBuf::
pbackfail (int_type c)
{
  if (m_state != STATE_DOWNLOAD || m_havePending) {
    return traits_type::eof ();
  }

  if (traits_type::eq_int_type (c, traits_type::eof())) {
    if (!m_haveLast) {
      return traits_type::eof ();
    }
    m_havePending = true;
    return traits_type::not_eof (c);
  }

  m_pending = c;
  m_havePending = true;
  return c;
}

std::streamsize
OCPI::Util::Http::ClientStream::ClientBuf::
xsgetn (char * s, std::streamsize n)
{
  if (m_state == STATE_UPLOAD) {
    try {
      completeUpload ();
    }
    catch (...) {
      return 0;
    }
  }

  if (m_state != STATE_DOWNLOAD) {
    return 0;
  }

  std::streamsize toRead, count, remaining, total;

  remaining = n;
  total = 0;

  if (m_havePending && remaining) {
    *s++ = traits_type::to_char_type (m_pending);
    remaining--;
    total++;
    m_havePending = false;
  }

  while (remaining && m_inConn->good() && !m_inConn->eof()) {
    /*
     * If we're at the end of a chunk, ask underflow() to open the next one
     */

    if (m_chunked && m_chunkRemaining == 0) {
      if (traits_type::eq_int_type (underflow(), traits_type::eof())) {
        break;
      }
    }

    /*
     * If at the end of file according to Content-Length, break
     */

    if (!m_chunked &&
        m_contentLength != static_cast<unsigned long long> (-1) &&
        m_contentRemaining == 0) {
      break;
    }

    if (m_chunked) {
      std::streamsize inChunk =
        OCPI::Util::unsignedToStreamsize (m_chunkRemaining, true);
      toRead = (remaining < inChunk) ? remaining : inChunk;
    }
    else if (m_contentLength != static_cast<unsigned long long> (-1)) {
      std::streamsize inContent =
        OCPI::Util::unsignedToStreamsize (m_contentRemaining, true);
      toRead = (remaining < inContent) ? remaining : inContent;
    }
    else {
      toRead = remaining;
    }

    m_inConn->read (s, toRead);
    count = m_inConn->gcount ();

    remaining -= count;
    total += count;
    s += count;

    if (m_chunked) {
      m_chunkRemaining -= count;
    }
    else if (m_contentLength != static_cast<unsigned long long> (-1)) {
      m_contentRemaining -= count;
    }
  }

  if (total > 0) {
    m_haveLast = true;
    m_pending = traits_type::to_int_type (s[-1]);
  }

  return total;
}

std::streamsize
OCPI::Util::Http::ClientStream::ClientBuf::
xsputn (const char * s, std::streamsize n)
{
  if (m_state != STATE_UPLOAD) {
    return 0;
  }

  m_outConn->write (s, n);
  return (m_outConn->good() ? n : 0);
}

/*
 * ----------------------------------------------------------------------
 * OCPI::Util::Http::ClientStream
 * ----------------------------------------------------------------------
 */

OCPI::Util::Http::ClientStream::
ClientStream ()
  throw ()
  : std::iostream (0)
{
  this->init (&m_buf);
  m_connected = false;
  m_uploading = false;
  m_errorReported = false;
  setstate (std::ios_base::badbit);
}

OCPI::Util::Http::ClientStream::
~ClientStream ()
  throw ()
{
  /*
   * We can't do that here, because we depend on a derived class
   * to implement closeConnection. However, that operation is not
   * accessible here, because the derived class' destructor has
   * already completed.
   *
   * This code needs to be in each derived class' destructor.
   *
   * try {
   *   close ();
   * }
   * catch (...) {
   * }
   */

  // ocpiAssert (!m_connected);
}

void
OCPI::Util::Http::ClientStream::
connect (const OCPI::Util::Uri & uri)
  throw (std::string)
{
  if (m_connected) {
    reset ();
  }

  std::istream * inConn;
  std::ostream * outConn;

  openConnection (uri, inConn, outConn);

  m_buf.setConn (inConn, outConn,
                  shutdownForBuf,
                  reinterpret_cast<void *> (this));

  m_connected = true;
}

void
OCPI::Util::Http::ClientStream::
close ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  if (m_connected) {
    m_connected = false;

    try {
      completeUpload ();
    }
    catch (...) {
      try {
        closeConnection ();
      }
      catch (...) {
      }

      m_buf.resetConn ();
      throw;
    }
    
    m_buf.resetConn ();
    closeConnection ();
  }
}

void
OCPI::Util::Http::ClientStream::
reset ()
  throw ()
{
  try {
    close ();
  }
  catch (...) {
  }

  m_connected = false;
  m_uploading = false;
  m_errorReported = false;
  m_mode = (std::ios_base::openmode) 0;
  m_body.clear ();
  m_buf.resetConn ();
  this->setstate (std::ios_base::badbit);
}

void
OCPI::Util::Http::ClientStream::
readBody ()
  throw ()
{
  char bodyBuffer[10240];
  while (this->good() && !this->eof() &&
         m_body.length() < MAX_LENGTH_OF_BODY) {
    this->read (bodyBuffer, 10240);
    m_body.append (bodyBuffer, this->gcount());
  }
}

void
OCPI::Util::Http::ClientStream::
completeUpload ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  if (m_uploading && (m_mode & std::ios_base::out)) {
    m_uploading = false;
    checkRelocation ();
    checkServerError ();

    if (!(m_mode & std::ios_base::in)) {
      readBody ();
    }
  }
}

void
OCPI::Util::Http::ClientStream::
checkRelocation ()
  throw (std::string, Redirection)
{
  if (m_buf.getStatusCode() >= 300 &&
      m_buf.getStatusCode()  < 400 &&
      !m_errorReported && m_connected) {
    const Headers & rh = m_buf.getResponseHeaders ();
    Headers::const_iterator ri = rh.find ("Location");
    
    if (ri != rh.end()) {
      m_connected = false;
      m_errorReported = true;
      this->setstate (std::ios_base::badbit);

      try {
        closeConnection ();
      }
      catch (...) {
      }

      throw Redirection (m_buf.getStatusCode(), (*ri).second);
    }
  }
}

void
OCPI::Util::Http::ClientStream::
checkServerError ()
  throw (std::string, ClientError, ServerError)
{
  if (m_buf.getStatusCode() >= 400 && !m_errorReported && m_connected) {
    readBody ();

    m_connected = false;
    m_errorReported = true;
    this->setstate (std::ios_base::badbit);

    try {
      closeConnection ();
    }
    catch (...) {
    }

    if (m_buf.getStatusCode() >= 400 && m_buf.getStatusCode() < 500) {
      throw ClientError (m_buf.getStatusCode (),
                         m_buf.getReasonPhrase (),
                         m_body);
    }
    else {
      throw ServerError (m_buf.getStatusCode (),
                         m_buf.getReasonPhrase (),
                         m_body);
    }
  }
}

void
OCPI::Util::Http::ClientStream::
head (const OCPI::Util::Uri & uri)
  throw (std::string, Redirection, ClientError, ServerError)
{
  headOrDelete (ClientBuf::REQUEST_HEAD, uri, (std::ios_base::openmode) 0);
}

void
OCPI::Util::Http::ClientStream::
remove (const OCPI::Util::Uri & uri,
        std::ios_base::openmode mode)
  throw (std::string, Redirection, ClientError, ServerError)
{
  if ((mode & std::ios_base::out)) {
    throw std::string ("invalid openmode for delete: out");
  }

  headOrDelete (ClientBuf::REQUEST_DELETE, uri, mode);
}

void
OCPI::Util::Http::ClientStream::
headOrDelete (int requestType,
              const OCPI::Util::Uri & uri,
              std::ios_base::openmode mode)
  throw (std::string, Redirection, ClientError, ServerError)
{
  m_mode = mode;

  connect (uri);

  try {
    m_buf.sendRequest (requestType, uri);
  }
  catch (...) {
    m_connected = false;

    try {
      closeConnection ();
    }
    catch (...) {
    }

    throw;
  }

  m_uploading = false;
  checkRelocation ();
  checkServerError ();
  this->clear ();

  if (!(m_mode & std::ios_base::in)) {
    readBody ();
  }
}

void
OCPI::Util::Http::ClientStream::
get (const OCPI::Util::Uri & uri)
  throw (std::string, Redirection, ClientError, ServerError)
{
  m_mode = std::ios_base::in;

  connect (uri);

  try {
    m_buf.sendRequest (ClientBuf::REQUEST_GET, uri);
  }
  catch (...) {
    m_connected = false;

    try {
      closeConnection ();
    }
    catch (...) {
    }

    throw;
  }

  m_uploading = false;
  checkRelocation ();
  checkServerError ();
  this->clear ();
}

void
OCPI::Util::Http::ClientStream::
put (const OCPI::Util::Uri & uri,
     std::ios_base::openmode mode)
  throw (std::string, Redirection, ClientError, ServerError)
{
  put (uri, std::string(), static_cast<unsigned long long> (-1), mode);
}

void
OCPI::Util::Http::ClientStream::
put (const OCPI::Util::Uri & uri,
     const std::string & a_contentType,
     unsigned long long a_contentLength,
     std::ios_base::openmode mode)
  throw (std::string, Redirection, ClientError, ServerError)
{
  putOrPost (ClientBuf::REQUEST_PUT, uri, a_contentType, a_contentLength, mode);
}

void
OCPI::Util::Http::ClientStream::
post (const OCPI::Util::Uri & uri,
      std::ios_base::openmode mode)
  throw (std::string, Redirection, ClientError, ServerError)
{
  post (uri, std::string(), static_cast<unsigned long long> (-1), mode);
}

void
OCPI::Util::Http::ClientStream::
post (const OCPI::Util::Uri & uri,
      const std::string & a_contentType,
      unsigned long long a_contentLength,
      std::ios_base::openmode mode)
  throw (std::string, Redirection, ClientError, ServerError)
{
  putOrPost (ClientBuf::REQUEST_PUT, uri, a_contentType, a_contentLength, mode);
}

void
OCPI::Util::Http::ClientStream::
putOrPost (int requestType,
           const OCPI::Util::Uri & uri,
           const std::string & a_contentType,
           unsigned long long a_contentLength,
           std::ios_base::openmode mode)
  throw (std::string, Redirection, ClientError, ServerError)
{
  m_mode = mode | std::ios_base::out;
  Headers headers;

  if (a_contentType.length()) {
    headers["Content-Type"] = a_contentType;
  }

  if (a_contentLength != static_cast<unsigned long long> (-1)) {
    headers["Content-Length"] =
      OCPI::Util::unsignedToString (a_contentLength);
  }

  connect (uri);

  try {
    m_buf.sendRequest (requestType, uri, headers);
  }
  catch (...) {
    try {
      closeConnection ();
    }
    catch (...) {
    }

    m_connected = false;
    throw;
  }

  m_uploading = true;
  this->clear ();
}

int
OCPI::Util::Http::ClientStream::
statusCode ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  try {
    completeUpload ();
  }
  catch (...) {
  }

  return m_buf.getStatusCode ();
}

const OCPI::Util::Http::Headers &
OCPI::Util::Http::ClientStream::
responseHeaders ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  try {
    completeUpload ();
  }
  catch (...) {
  }

  return m_buf.getResponseHeaders ();
}

unsigned long long
OCPI::Util::Http::ClientStream::
contentLength ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  try {
    completeUpload ();
  }
  catch (...) {
  }

  return m_buf.getContentLength ();
}

std::string
OCPI::Util::Http::ClientStream::
contentType ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  try {
    completeUpload ();
  }
  catch (...) {
  }

  const Headers & headers = m_buf.getResponseHeaders ();
  Headers::const_iterator it = headers.find ("Content-Type");

  if (it == headers.end()) {
    return defaultContentType;
  }

  /*
   * media-type     = type "/" subtype *( ";" parameter )
   * type           = token
   * subtype        = token
   */

  const std::string & mediaType = (*it).second;
  std::string::size_type colonPos = mediaType.find (';');

  if (colonPos == std::string::npos) {
    return mediaType;
  }

  while (colonPos > 0 && isspace(mediaType[colonPos-1])) {
    colonPos--;
  }

  return mediaType.substr (0, colonPos);
}

/*
 * Look for the Content-Type's charset parameter. If there is no
 * charset parameter, "text" media types default to "iso-8859-1",
 * otherwise we throw an exception.
 */

std::string
OCPI::Util::Http::ClientStream::
charset ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  const Headers & headers = m_buf.getResponseHeaders ();
  Headers::const_iterator it = headers.find ("Content-Type");

  if (it == headers.end()) {
    throw std::string ("no content type, thus no charset, assuming binary content");
  }

  /*
   * media-type     = type "/" subtype *( ";" parameter )
   * type           = token
   * subtype        = token
   */

  const std::string & mediaType = (*it).second;
  std::string::size_type slashPos = mediaType.find ('/');
  std::string type;

  if (slashPos == std::string::npos) {
    type = mediaType;
  }
  else {
    type = mediaType.substr (0, slashPos);
  }

  std::string::size_type colonPos = mediaType.find (';');
  std::string parameters;

  if (colonPos != std::string::npos) {
    colonPos++;
    while (colonPos < mediaType.length() && isspace (mediaType[colonPos])) {
      colonPos++;
    }
    parameters = mediaType.substr (colonPos);
  }

  /*
   * parameter = attribute "=" value
   * attribute = token
   * value     = token | quoted-string
   */

  while (parameters.length()) {
    std::string::size_type idx;
    std::string parameter;
    
    if ((colonPos = parameters.find (';')) == std::string::npos) {
      parameter = parameters;
      parameters.clear ();
    }
    else {
      idx = colonPos;
      while (idx > 0 && isspace (parameters[idx-1])) {
        idx--;
      }
      colonPos++;
      while (colonPos < parameters.length() && isspace (parameters[colonPos])) {
        colonPos++;
      }
      parameter  = parameters.substr (0, idx);
      parameters = parameters.substr (colonPos);
    }

    if ((idx = parameter.find ('=')) == std::string::npos) {
      // should not happen
      continue;
    }

    std::string attribute = parameter.substr (0, idx);
    std::string value = parameter.substr (idx+1);

    if (value.length() && value[0] == '"') {
      idx = value.length();
      while (idx > 0 && value[idx] != '"') {
        idx--;
      }
      value = value.substr (1, idx-1);
    }

    if (attribute == "charset") {
      return value;
    }
  }

  if (type != "text") {
    throw std::string ("no charset, not of text type, so no default");
  }

  /*
   * Default charset for text types
   */

  return defaultCharset;
}

std::time_t
OCPI::Util::Http::ClientStream::
lastModified ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  try {
    completeUpload ();
  }
  catch (...) {
  }

  const Headers & headers = m_buf.getResponseHeaders ();
  Headers::const_iterator it = headers.find ("Last-Modified");

  if (it == headers.end()) {
    throw std::string ("no last-modified header");
  }

  return OCPI::Util::Http::parseHttpDate ((*it).second);
}

const std::string &
OCPI::Util::Http::ClientStream::
body ()
  throw (std::string, Redirection, ClientError, ServerError)
{
  /*
   * The body is only available if there was a server error (4xx or
   * 5xx), or if the mode is out but not in (i.e. the user uploaded
   * a file but did not want to read the response, e.g. after a PUT).
   */

  completeUpload ();

  if (statusCode() < 400 &&
      (!(m_mode & std::ios_base::out) ||
       (m_mode & std::ios_base::in))) {
    throw std::string ("response body not available");
  }

  return m_body;
}

void
OCPI::Util::Http::ClientStream::
shutdownForBuf (void * opaque)
  throw (std::string)
{
  ClientStream * me = reinterpret_cast<ClientStream *> (opaque);
  me->shutdownConnection (std::ios_base::out);
}
