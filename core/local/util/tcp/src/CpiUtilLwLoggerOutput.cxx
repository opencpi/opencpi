#include <string>
#include <CpiOsAssert.h>
#include <CpiOsDataTypes.h>
#include <CpiOsMutex.h>
#include <CpiUtilCDR.h>
#include <CpiUtilIOP.h>
#include <CpiLoggerLogger.h>
#include <CpiUtilTcpClient.h>
#include "CpiUtilLwLoggerOutput.h"

CPI::Util::LwLoggerOutput::Buf::
Buf (const CPI::Util::IOP::IOR & ior)
  : m_first (true),
    m_locked (false),
    m_logLevel (0),
    m_connected (0),
    m_requestId (0),
    m_ior (ior)
{
  m_byteOrder = CPI::Util::CDR::nativeByteorder ();

  try {
    if (m_ior.hasProfile (CPI::Util::IOP::TAG_INTERNET_IOP)) {
      m_profile.decode (m_ior.profileData (CPI::Util::IOP::TAG_INTERNET_IOP));
    }
  }
  catch (const std::string &) {
    m_profile.host.clear ();
  }
}

CPI::Util::LwLoggerOutput::Buf::
~Buf ()
{
  if (m_locked) {
    m_lock.unlock ();
  }
}

void
CPI::Util::LwLoggerOutput::Buf::
setLogLevel (unsigned short logLevel)
{
  m_lock.lock ();
  m_locked = true;
  m_logLevel = logLevel;
  m_producerName.clear ();
  m_logMessage.clear ();
  cpiAssert (m_first);
}

void
CPI::Util::LwLoggerOutput::Buf::
setProducerId (const char * producerId)
{
  m_producerId = producerId;
}

void
CPI::Util::LwLoggerOutput::Buf::
setProducerName (const char * producerName)
{
  cpiAssert (m_locked);
  cpiAssert (m_first);
  m_producerName = producerName;
}

int
CPI::Util::LwLoggerOutput::Buf::
sync ()
{
  bool good = true;

  if (!m_first) {
    cpiAssert (m_locked);
    good = sendMessage();
    m_first = true;
  }

  if (m_locked) {
    m_locked = false;
    m_lock.unlock ();
  }

  return (good ? 0 : -1);
}

std::streambuf::int_type
CPI::Util::LwLoggerOutput::Buf::
overflow (int_type c)
{
  cpiAssert (m_locked);

  if (traits_type::eq (c, traits_type::eof())) {
    return traits_type::not_eof (c);
  }

  m_first = false;
  m_logMessage += traits_type::to_char_type (c);
  return c;
}

std::streamsize
CPI::Util::LwLoggerOutput::Buf::
xsputn (const char * data, std::streamsize count)
{
  cpiAssert (m_locked);
  m_first = false;
  m_logMessage.append (data, count);
  return count;
}

bool
CPI::Util::LwLoggerOutput::Buf::
connectToLogService ()
{
  if (!m_profile.host.length()) {
    return false;
  }

  try {
    m_conn.connect (m_profile.host,
		    m_profile.port);
  }
  catch (const std::string &) {
    return false;
  }

  /*
   * We never want to receive anything from the server -- write_record
   * is a oneway operation.
   */

  try {
    m_conn.shutdown (std::ios_base::in);
  }
  catch (const std::string &) {
  }

  return true;
}

/*
 * Data  47 49 4f 50 01 00 01 00 6f 00 00 00 00 00 00 00 GIOP....o.......
 *       04 00 00 00 00 00 00 00 12 00 00 00 2f 31 31 33 ............/113
 *       38 34 37 31 39 37 34 2f 31 34 33 32 2a 31 66 6f 8471974/1432*1fo
 *       0d 00 00 00 77 72 69 74 65 5f 72 65 63 6f 72 64 ....write_record
 *       00 66 6f 6f 00 00 00 00 0b 00 00 00 70 72 6f 64 .foo........prod
 *       75 63 65 72 49 64 00 66 0d 00 00 00 70 72 6f 64 ucerId.f....prod
 *       75 63 65 72 4e 61 6d 65 00 66 09 00 0b 00 00 00 ucerName.f......
 *       48 65 6c 6c 6f 57 6f 72 6c 64 00                HelloWorld.
 */

bool
CPI::Util::LwLoggerOutput::Buf::
sendMessage ()
{
  CPI::Util::CDR::Encoder request;

  /*
   * Marshal request header.
   */

  request.putULong (0);                       // service_context
  request.putULong (m_requestId);
  request.putBoolean (false);                 // response_expected
  request.putOctetSeq (m_profile.object_key);
  request.putString ("write_record");         // operation
  request.putULong (0);                       // requesting_principal

  /*
   * Marshal request body.
   */

  request.putString (m_producerId);
  request.putString (m_producerName);
  request.putUShort (m_logLevel);
  request.putString (m_logMessage);

  const std::string & requestData = request.data ();
  CPI::OS::uint32_t messageLength = requestData.length();

  /*
   * GIOP header.
   */

  char giopHeader[12];
  giopHeader[0] = 'G';
  giopHeader[1] = 'I';
  giopHeader[2] = 'O';
  giopHeader[3] = 'P';
  giopHeader[4] = 1;                    // GIOP major version
  giopHeader[5] = 0;                    // GIOP minor version
  giopHeader[6] = m_byteOrder ? 1 : 0;  // 0: big endian, 1: little endian
  giopHeader[7] = 0;                    // MsgType_1_0::Request
  std::memcpy (giopHeader+8, reinterpret_cast<char *> (&messageLength), 4);

  /*
   * Connect, if we are not connected yet.
   */

  if (!m_connected) {
    if (!connectToLogService()) {
      return false;
    }
    m_connected = true;
  }

  /*
   * Send the GIOP header.  If this fails, then the server may have
   * closed its connection, e.g., because of a timeout.  In that case,
   * attempt to reconnect and try again.
   */

  m_conn.write (giopHeader, 12);

  if (!m_conn.good()) {
    m_connected = false;

    try {
      m_conn.close ();
    }
    catch (const std::string &) {
      return false;
    }

    if (!connectToLogService()) {
      return false;
    }

    m_connected = true;
    m_conn.write (giopHeader, 12);

    if (!m_conn.good()) {
      return false;
    }
  }

  /*
   * Send request.
   */

  m_conn.write (requestData.data(), requestData.length());

  /*
   * Done.
   */

  m_requestId++;
  return m_conn.good();
}

CPI::Util::LwLoggerOutput::
LwLoggerOutput (const CPI::Util::IOP::IOR & ior)
  : Logger (m_obuf),
    m_obuf (ior)
{
}

CPI::Util::LwLoggerOutput::
~LwLoggerOutput ()
{
}
