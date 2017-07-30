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

#include <OcpiUtilHttpServer.h>
#include <OcpiUtilHttpMisc.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilMisc.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerNullOutput.h>
#include <OcpiLoggerDebugLogger.h>
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>

OCPI::Logger::NullOutput
OCPI::Util::Http::Server::
g_myDefaultLogger;

OCPI::Logger::ProducerName
OCPI::Util::Http::Server::
g_myProducerName ("OCPI::Util::Http::Server");

OCPI::Util::Http::Server::
Server (OCPI::Util::Vfs::Vfs * fs,
        OCPI::Logger::Logger * logger)
  throw ()
  : m_logger (logger),
    m_fs (fs)
{
  m_inConn = 0;
  m_outConn = 0;

  if (!m_logger) {
    m_logger = &g_myDefaultLogger;
  }
}

OCPI::Util::Http::Server::
~Server ()
  throw ()
{
}

void
OCPI::Util::Http::Server::
setLogger (OCPI::Logger::Logger * out)
  throw ()
{
  m_logger = out;

  if (!m_logger) {
    m_logger = &g_myDefaultLogger;
  }
}

void
OCPI::Util::Http::Server::
setRoot (OCPI::Util::Vfs::Vfs * fs)
  throw ()
{
  m_fs = fs;
}

void
OCPI::Util::Http::Server::
resetConn (std::istream * in, std::ostream * out)
  throw ()
{
  m_state = STATE_REQUEST;
  m_majorVersion = 1;
  m_minorVersion = 1;
  m_inConn = in;
  m_outConn = out;
  m_closeConnection = false;

  OCPI::Logger::DebugLogger debugLogger (*m_logger);
  debugLogger << g_myProducerName
              << OCPI::Logger::Verbosity (2)
              << "Got new connection."
              << std::flush;
}

bool
OCPI::Util::Http::Server::
receiveRequestLine ()
  throw ()
{
  OCPI::Logger::DebugLogger debugLogger (*m_logger);

  m_requestLine =
    OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);

  /*
   * HTTP says we should ignore empty lines
   */

  while (m_requestLine.length() == 0 && m_inConn->good()) {
    m_requestLine =
      OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);
  }

  if (m_inConn->eof()) {
    m_state = STATE_CLOSE;
    return false;
  }

  if (!m_inConn->good()) {
    debugLogger << g_myProducerName
                << OCPI::Logger::Verbosity (2)
                << "Failed to read request. Returning 400 error."
                << std::flush;
    sendError ("400", "Bad Request", "Failed to read request.");
    m_state = STATE_BROKEN;
    return false;
  }

  if (m_requestLine.length() == MAX_LENGTH_OF_HEADER_LINE) {
    debugLogger << g_myProducerName
                << OCPI::Logger::Verbosity (2)
                << "Request line exceeds "
                << MAX_LENGTH_OF_HEADER_LINE
                << " in length. Returning 414 error."
                << std::flush;
    sendError ("414", "Request URI too large");
    m_state = STATE_BROKEN;
    return false;
  }

  debugLogger << g_myProducerName
              << OCPI::Logger::Verbosity (2)
              << "Got request: \""
              << m_requestLine
              << "\"."
              << std::flush;

  /*
   * Determine the request type
   */

  std::string::size_type idx = m_requestLine.find (' ');

  if (idx == std::string::npos) {
    debugLogger << g_myProducerName
                << OCPI::Logger::Verbosity (2)
                << "Failed to parse request line, no space found. "
                << "Returning 414 error."
                << std::flush;
    sendError ("400", "Bad Request", "Does not look like a request.");
    m_state = STATE_BROKEN;
    return false;
  }

  m_requestMethod = m_requestLine.substr (0, idx);

  if (m_requestMethod == "OPTIONS") {
    m_requestType = REQUEST_OPTIONS;
  }
  else if (m_requestMethod == "GET") {
    m_requestType = REQUEST_GET;
  }
  else if (m_requestMethod == "HEAD") {
    m_requestType = REQUEST_HEAD;
  }
  else if (m_requestMethod == "POST") {
    m_requestType = REQUEST_POST;
  }
  else if (m_requestMethod == "PUT") {
    m_requestType = REQUEST_PUT;
  }
  else if (m_requestMethod == "DELETE") {
    m_requestType = REQUEST_DELETE;
  }
  else if (m_requestMethod == "TRACE") {
    m_requestType = REQUEST_TRACE;
  }
  else if (m_requestMethod == "CONNECT") {
    m_requestType = REQUEST_CONNECT;
  }
  else {
    debugLogger << g_myProducerName
                << OCPI::Logger::Verbosity (2)
                << "Unsupported request type: \""
                << m_requestType
                << "\". Returning 501 error."
                << std::flush;

    sendError ("501", "Not Implemented", "Invalid request method.");
    m_requestType = REQUEST_UNKNOWN;
    m_state = STATE_BROKEN;
    return false;
  }

  /*
   * Request URI and version
   */

  std::string::size_type idx2 = m_requestLine.find (' ', idx+1);

  if (idx2 == std::string::npos) {
    m_majorVersion = 0;
    m_minorVersion = 9;
    m_closeConnection = true;
    m_requestURI = m_requestLine.substr (idx+1);
  }
  else {
    m_requestURI = m_requestLine.substr (idx+1, idx2-idx-1);
    std::string versionStr = m_requestLine.substr (idx2+1);

    if (versionStr.length() < 8 ||
        versionStr[0] != 'H' || versionStr[1] != 'T' ||
        versionStr[2] != 'T' || versionStr[3] != 'P' ||
        versionStr[4] != '/' || versionStr[6] != '.') {
      debugLogger << g_myProducerName
                  << OCPI::Logger::Verbosity (2)
                  << "Invalid HTTP version string: \""
                  << versionStr
                  << "\". Returning 400 error."
                  << std::flush;
      sendError ("400", "Bad Request", "Invalid HTTP version string.");
      m_state = STATE_BROKEN;
      return false;
    }

    std::string version = versionStr.substr (5);
    int major, minor;
    major = version[0] - '0';
    minor = version[2] - '0';

    if (major != 1) {
      /*
       * I don't think we can handle HTTP/2.x
       */
      debugLogger << g_myProducerName
                  << OCPI::Logger::Verbosity (2)
                  << "HTTP version not supported: \""
                  << version
                  << "\". Returning 505 error."
                  << std::flush;
      sendError ("505", "HTTP Version Not Supported",
                 "HTTP Version Not Supported.");
      m_state = STATE_BROKEN;
      return false;
    }

    if (major < m_majorVersion ||
        (major == m_majorVersion && minor < m_minorVersion)) {
      m_majorVersion = major;
      m_minorVersion = minor;
    }

    if (m_majorVersion == 1 && m_minorVersion == 0) {
      m_closeConnection = true;
    }
  }

  return true;
}

bool
OCPI::Util::Http::Server::
receiveRequestHeaders ()
  throw ()
{
  if (m_majorVersion == 0 && m_minorVersion == 9) {
    return true;
  }

  OCPI::Logger::DebugLogger debugLogger (*m_logger);

  std::string::size_type colonPos;
  std::string headerLine = OCPI::Util::readline (m_inConn);
  int count=0;

  while (m_inConn->good() && headerLine.length()) {
    if (++count > MAX_NUM_OF_HEADER_LINES) {
      debugLogger << g_myProducerName
                  << OCPI::Logger::Verbosity (2)
                  << "Request exceeds maximum allowed number of header lines: "
                  << MAX_NUM_OF_HEADER_LINES
                  << ". Request too large. Returning 500 error."
                  << std::flush;
      sendError ("500", "Internal Error", "Request too large.");
      m_state = STATE_BROKEN;
      return false;
    }

    if ((colonPos = headerLine.find (':')) == std::string::npos) {
      debugLogger << g_myProducerName
                  << OCPI::Logger::Verbosity (2)
                  << "Invalid header line received: \""
                  << headerLine
                  << "\". Returning 400 error."
                  << std::flush;
      sendError ("400", "Bad Request", "Invalid header line received.");
      m_state = STATE_BROKEN;
      return false;
    }

    /*
     * Field name must be composed of valid characters
     */

    std::string fieldName = headerLine.substr (0, colonPos);

    if (!isToken (fieldName)) {
      debugLogger << g_myProducerName
                  << OCPI::Logger::Verbosity (2)
                  << "Invalid header line received: \""
                  << headerLine
                  << "\". Field name \""
                  << fieldName
                  << "\" is not a token. Returning 400 error."
                  << std::flush;
      sendError ("400", "Bad Request", "Invalid header line received.");
      m_state = STATE_BROKEN;
      return false;
    }

    /*
     * remove leading whitespace from fieldValue
     */

    std::string fieldValue;

    colonPos++;
    while (colonPos < headerLine.length() && isspace (headerLine[colonPos])) {
      colonPos++;
    }

    fieldValue = headerLine.substr (colonPos);

    /*
     * If we saw this header before, concatenate its value
     */

    if (m_requestHeaders.find (fieldName) != m_requestHeaders.end()) {
      m_requestHeaders[fieldName] = m_requestHeaders[fieldName] + "," + fieldValue;
    }
    else {
      m_requestHeaders[fieldName] = fieldValue;
    }

    debugLogger << g_myProducerName
                << OCPI::Logger::Verbosity (3)
                << "Header line received: \""
                << headerLine
                << "\""
                << std::flush;

    headerLine = OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);
  }

  if (!m_inConn->good ()) {
    debugLogger << g_myProducerName
                << OCPI::Logger::Verbosity (2)
                << "Connection broken while reading headers. "
                << "Returning 400 error."
                << std::flush;
    sendError ("400", "Bad Request", "Connection broken while reading headers.");
    return false;
  }

  Headers::const_iterator hit;
  if ((hit = m_requestHeaders.find ("Connection")) != m_requestHeaders.end() &&
      (*hit).second == "close") {
    m_closeConnection = true;
  }

  return true;
}

bool
OCPI::Util::Http::Server::
processGetOrHeadRequest ()
  throw ()
{
  OCPI::Logger::DebugLogger debugLogger (*m_logger);

  std::string fileName, reason;
  std::time_t currentTime, lastModified;
  std::istream * fileStream;
  unsigned long long fileSize;

  debugLogger << g_myProducerName
              << "Got "
              << ((m_requestType == REQUEST_GET) ? "GET" : "HEAD")
              << " request for \""
              << m_requestURI
              << "\"."
              << std::flush;

  try {
    fileName = OCPI::Util::Uri::decode (m_requestURI);
  }
  catch (const std::string & oops) {
    debugLogger << g_myProducerName
                << "Invalid request URI: \""
                << m_requestURI
                << "\": "
                << oops
                << std::flush;
    sendError ("404", "Not Found", "File not found.");
    return false;
  }

  /*
   * Information for certain header fields
   */

  currentTime = std::time (0);

  try {
    fileSize = m_fs->size (fileName);
  }
  catch (const std::string &) {
    fileSize = static_cast<unsigned long long> (-1);
  }

  try {
    lastModified = m_fs->lastModified (fileName);
  }
  catch (...) {
    lastModified = static_cast<time_t> (-1);
  }

  if (lastModified != static_cast<time_t> (-1) &&
      lastModified > currentTime) {
    lastModified = currentTime;
  }

  /*
   * Handle If-Modified-Since header
   */

  Headers::const_iterator ims = m_requestHeaders.find ("If-Modified-Since");

  if (ims != m_requestHeaders.end() && lastModified != (time_t) -1) {
    std::time_t oldTimestamp;

    try {
      oldTimestamp = OCPI::Util::Http::parseHttpDate ((*ims).second);
    }
    catch (...) {
      oldTimestamp = static_cast<time_t> (-1);
    }

    /*
     * If it is the same old file, return 304 Not Modified
     */

    if (oldTimestamp != static_cast<time_t> (-1) &&
        lastModified < oldTimestamp) {
      if (m_majorVersion == 1 && m_minorVersion == 0) {
        *m_outConn << "HTTP/1.0 304 Not Modified\r\n";
      }
      else if (m_majorVersion == 1 && m_minorVersion == 1) {
        *m_outConn << "HTTP/1.1 304 Not Modified\r\n";
      }

      *m_outConn << "Date: "
                 << OCPI::Util::Http::makeHttpDate (currentTime)
                 << "\r\n";
      *m_outConn << "Last-Modified: "
                 << OCPI::Util::Http::makeHttpDate (lastModified)
                 << "\r\n";
      *m_outConn << "\r\n"
                 << std::flush;

      if (m_closeConnection) {
        m_state = STATE_CLOSE;
      }
      else {
        m_state = STATE_REQUEST;
      }

      debugLogger << g_myProducerName
                  << "Document not modified since "
                  << (*ims).second
                  << ". Returning 304."
                  << std::flush;

      return true;
    }

    /*
     * If the file was modified, or if we can't determine the file's
     * timestamp, proceed as if this header field wasn't present.
     */
  }

  /*
   * If the request type is for a HEAD, then we don't need to open
   * the file.
   */

  if (m_requestType == REQUEST_HEAD) {
    fileStream = 0;
    bool exists;
    try {
      exists = m_fs->exists (fileName);
    }
    catch (const std::string & err) {
      debugLogger << g_myProducerName
                  << "Failed to check existence of \""
                  << fileName
                  << "\":"
                  << err
                  << std::flush;
      sendError ("404", "Not Found", "File not found.");
      return true;
    }
    catch (...) {
      // what else to do here? we could try to open the file
      exists = false;
    }
    if (!exists) {
      debugLogger << g_myProducerName
                  << "File does not exist: \""
                  << fileName
                  << "\". Returning 404."
                  << std::flush;
      sendError ("404", "Not Found", "File not found.");
      return true;
    }
  }
  else {
    try {
      fileStream = m_fs->openReadonly (fileName, std::ios_base::binary);
    }
    catch (const std::string & err) {
      debugLogger << g_myProducerName
                  << "Can not open file \""
                  << fileName
                  << "\" for reading: "
                  << err << "."
                  << std::flush;
      sendError ("404", "Not Found", "File not found.");
      return true;
    }
  }

  /*
   * Send Status Line
   */

  bool chunked = false;

  if (m_majorVersion == 1 && m_minorVersion == 0) {
    *m_outConn << "HTTP/1.0 200 Ok\r\n";
  }
  else if (m_majorVersion == 1 && m_minorVersion == 1) {
    *m_outConn << "HTTP/1.1 200 Ok\r\n"
                << "Transfer-Encoding: chunked\r\n";
    chunked = true;
  }

  /*
   * Send Headers
   */

  if (m_majorVersion > 0) {
    *m_outConn << "Date: "
               << OCPI::Util::Http::makeHttpDate (currentTime)
               << "\r\n";

    if (lastModified != (time_t) -1) {
      *m_outConn << "Last-Modified: "
                 << OCPI::Util::Http::makeHttpDate (lastModified)
                 << "\r\n";
    }

    if (fileSize != static_cast<unsigned long long> (-1)) {
      *m_outConn << "Content-Length: " << fileSize << "\r\n";
    }

    *m_outConn << "\r\n" << std::flush;
  }

  /*
   * If the request type was HEAD, we must not send the body. In fact,
   * we didn't even open the file (see above).
   */

  unsigned long long totalBytes = 0;

  if (m_requestType != REQUEST_HEAD) {

    /*
     * Send body
     */

    char buffer[DATA_BUFFER_SIZE];
    std::streamsize amountRead;

    while (!fileStream->eof() && m_outConn->good()) {
      fileStream->read (buffer, DATA_BUFFER_SIZE);
      amountRead = fileStream->gcount ();
      totalBytes += amountRead;

      if (chunked && amountRead) {
        *m_outConn << std::hex << amountRead << std::dec << "\r\n";
      }

      m_outConn->write (buffer, amountRead);

      if (chunked && amountRead) {
        *m_outConn << "\r\n";
      }
    }

    if (m_outConn->good() && chunked) {
      *m_outConn << "0\r\n\r\n" << std::flush;
    }

    if (!m_outConn->good()) {
      debugLogger << g_myProducerName
                  << "Oops. Output is no good after sending "
                  << totalBytes
                  << " bytes."
                  << std::flush;
    }

    try {
      m_fs->close (fileStream);
    }
    catch (const std::string & oops) {
      debugLogger << g_myProducerName
                  << "Oops. Error closing file: "
                  << oops
                  << " (ignored)"
                  << std::flush;
    }
  }

  if (m_closeConnection) {
    m_state = STATE_CLOSE;
  }
  else {
    m_state = STATE_REQUEST;
  }

  debugLogger << g_myProducerName
              << "Request complete, "
              << totalBytes
              << " bytes sent."
              << std::flush;

  return true;
}

bool
OCPI::Util::Http::Server::
processPutRequest ()
  throw ()
{
  OCPI::Logger::DebugLogger debugLogger (*m_logger);

  std::string fileName, reason;
  std::ostream * fileStream;

  debugLogger << g_myProducerName
              << "Got PUT request for \""
              << m_requestURI
              << "\"."
              << std::flush;

  try {
    fileName = OCPI::Util::Uri::decode (m_requestURI);
  }
  catch (const std::string & oops) {
    debugLogger << g_myProducerName
                << "Invalid request URI: \""
                << m_requestURI
                << "\": "
                << oops
                << std::flush;
    sendError ("404", "Not Found", "File not found.");
    return false;
  }

  try {
    fileStream = m_fs->openWriteonly (fileName, std::ios_base::binary | std::ios_base::trunc);
  }
  catch (const std::string & err) {
      debugLogger << g_myProducerName
                  << "Can not open file \""
                  << fileName
                  << "\" for writing: "
                  << err
                  << std::flush;
    sendError ("404", "File Error", "Cannot write file.");
    return false;
  }

  /*
   * Is the file being chunked? Else, is there a content-length header?
   * If neither, then we have to wait for the connection to be closed in
   * order to figure out the eof.
   */

  bool chunked = false;

  if (m_majorVersion == 1 && m_minorVersion == 1) {
    Headers::const_iterator it = m_requestHeaders.find ("Transfer-Encoding");

    if (it != m_requestHeaders.end()) {
      if ((*it).second != "chunked") {
        debugLogger << g_myProducerName
                    << "Unsupported transfer encoding: \""
                    << (*it).second
                    << "\""
                    << std::flush;
        sendError ("404", "Request error", "Unsupported transfer encoding.");
        return false;
      }
      chunked = true;
    }
  }

  bool haveContentLength = false;
  unsigned long long contentLength = static_cast<unsigned long long> (-1);

  {
    Headers::const_iterator it = m_requestHeaders.find ("Content-Length");

    if (it != m_requestHeaders.end()) {
      const char * cl = (*it).second.c_str ();
      char * cep;

      contentLength = std::strtoul (cl, &cep, 10);

      if (cep && *cep) {
        debugLogger << g_myProducerName
                    << "Invalid content-length value: \""
                    << (*it).second
                    << "\" (ignoring)."
                    << std::flush;
      }
      else {
        haveContentLength = true;
      }
    }
  }

  if (!chunked && !haveContentLength) {
    m_closeConnection = true;
  }

  /*
   * Read body
   */

  unsigned long long chunkSize, totalBytes;
  std::streamsize amountToRead, amountRead;
  std::string strChunkSize;
  char buffer[DATA_BUFFER_SIZE], *eptr;
  const char * csptr;

  chunkSize = static_cast<unsigned long long> (-1);
  totalBytes = 0;

  while (m_inConn->good() && fileStream->good()) {
    if (chunked) {
      strChunkSize = OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);

      if (strChunkSize.length() == 0 || strChunkSize.length() == MAX_LENGTH_OF_HEADER_LINE) {
        debugLogger << g_myProducerName
                    << "Error reading chunk size: \""
                    << strChunkSize
                    << "\"."
                    << std::flush;
        sendError ("404", "Upload failure", "Error reading chunk size.");
        return false;
      }

      csptr = strChunkSize.c_str ();
      chunkSize = strtoul (csptr, &eptr, 16);

      if (eptr && *eptr) {
        debugLogger << g_myProducerName
                    << "Error determining chunk size: \""
                    << strChunkSize
                    << "\"."
                    << std::flush;
        sendError ("404", "Upload failure", "Error reading chunk size.");
        return false;
      }

      if (chunkSize == 0) {
        break;
      }
    }

    while ((!chunked || chunkSize > 0) &&
           (!haveContentLength || contentLength > 0) &&
           m_inConn->good() && fileStream->good()) {
      if (chunked) {
        std::streamsize inChunk =
          OCPI::Util::unsignedToStreamsize (chunkSize, true);
        amountToRead = (inChunk < DATA_BUFFER_SIZE) ? inChunk : DATA_BUFFER_SIZE;
      }
      else if (!haveContentLength) {
        std::streamsize inContent =
          OCPI::Util::unsignedToStreamsize (contentLength, true);
        amountToRead = (inContent < DATA_BUFFER_SIZE) ? inContent : DATA_BUFFER_SIZE;
      }
      else {
        amountToRead = DATA_BUFFER_SIZE;
      }

      m_inConn->read (buffer, amountToRead);
      amountRead = m_inConn->gcount ();

      if (chunked) {
        chunkSize -= amountRead;
      }
      else if (haveContentLength) {
        contentLength -= amountRead;
      }

      totalBytes += amountRead;

      fileStream->write (buffer, amountRead);
    }

    if (chunked && chunkSize == 0) {
      strChunkSize = OCPI::Util::readline (m_inConn, MAX_LENGTH_OF_HEADER_LINE);

      if (strChunkSize.length() != 0) {
        debugLogger << g_myProducerName
                    << "Error reading end of chunk size: \""
                    << strChunkSize
                    << "\" (expected empty line)"
                    << std::flush;
        sendError ("404", "Upload failure", "Error reading end of chunk.");
        return false;
      }
    }
  }

  if (!fileStream->good()) {
    debugLogger << g_myProducerName
                << "Oops. Output file is no good after writing "
                << totalBytes
                << " bytes."
                << std::flush;
  }

  /*
   * If we encountered an EOF, then ignore it so that the reply can make
   * it through.
   */

  if (!chunked && !haveContentLength && m_inConn->eof()) {
    m_inConn->clear ();
  }

  try {
    m_fs->close (fileStream);
  }
  catch (const std::string & oops) {
    debugLogger << g_myProducerName
                << "Oops. Error closing file: "
                << oops
                << " (ignored)"
                << std::flush;
  }

  if (m_closeConnection) {
    m_state = STATE_CLOSE;
  }
  else {
    m_state = STATE_REQUEST;
  }

  sendError ("201", "Created", "The file was uploaded.");

  debugLogger << g_myProducerName
              << "Upload complete, "
              << totalBytes
              << " bytes received."
              << std::flush;


  return true;
}

bool
OCPI::Util::Http::Server::
run ()
  throw ()
{
  OCPI::Logger::DebugLogger debugLogger (*m_logger);

  while (m_state == STATE_REQUEST) {
    m_requestHeaders.clear ();
    if (!receiveRequestLine() || !receiveRequestHeaders()) {
      return false;
    }

    switch (m_requestType) {
    case REQUEST_HEAD:
      if (!processGetOrHeadRequest ()) {
        return false;
      }
      break;

    case REQUEST_GET:
      if (!processGetOrHeadRequest ()) {
        return false;
      }
      break;

    case REQUEST_PUT:
      if (!processPutRequest ()) {
        return false;
      }
      break;

    default:
      {
        debugLogger << g_myProducerName
                    << OCPI::Logger::Verbosity (2)
                    << "Invalid request method: \""
                    << m_requestMethod
                    << "\""
                    << std::flush;
        sendError ("501", "Not Implemented", "Method not implemented.");
        m_state = STATE_BROKEN;
        return false;
      }
    }
  }

  debugLogger << g_myProducerName
              << OCPI::Logger::Verbosity (2)
              << "End of connection."
              << std::flush;
  return true;
}

void
OCPI::Util::Http::Server::
sendError (const char * statusCode,
           const char * reasonPhrase,
           const char * message)
  throw ()
{
  if (!m_outConn || !m_outConn->good()) {
    return;
  }

  /*
   * Don't send the status line in pieces, some clients don't like that
   */

  std::string statusLine = "HTTP/1.0 ";
  statusLine += statusCode;
  statusLine += " ";
  statusLine += reasonPhrase;
  statusLine += "\r\n";

  m_outConn->write (statusLine.data(), statusLine.length());

  *m_outConn << "\r\n";
  *m_outConn << "<html>\r\n"
              << "<head>\r\n"
              << "<title>" << reasonPhrase << "</title>\r\n"
              << "</head>\r\n"
              << "<body>\r\n"
              << "<h1>" << statusCode << ": " << reasonPhrase << "</h1>\r\n"
              << (message ? message : reasonPhrase) << "\r\n"
              << "</body>\r\n"
              << "</html>\r\n"
              << std::flush;
}
