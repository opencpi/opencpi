#include <CpiOsAssert.h>
#include <CpiOsDataTypes.h>
#include <string>
#include <cstring>
#include "CpiUtilCDR.h"

/*
 * ----------------------------------------------------------------------
 * nativeByteorder()
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::CDR::
nativeByteorder ()
  throw ()
{
  CPI::OS::uint32_t v = 0x01020304;
  char * cv = reinterpret_cast<char *> (&v);

  cpiAssert ((cv[0] == 1 && cv[1] == 2 && cv[2] == 3 && cv[3] == 4) ||
	     (cv[0] == 4 && cv[1] == 3 && cv[2] == 2 && cv[3] == 1));

  return (cv[0] == 1) ? false : true;
}

/*
 * ----------------------------------------------------------------------
 * CDR Encoder
 * ----------------------------------------------------------------------
 */

CPI::Util::CDR::Encoder::
Encoder ()
  throw ()
{
}

void
CPI::Util::CDR::Encoder::
align (unsigned long modulus)
  throw ()
{
  std::string::size_type curpos = m_data.length ();
  std::string::size_type newpos = modulus*((curpos+modulus-1)/modulus);
  m_data.resize (newpos, 'x');
}

void
CPI::Util::CDR::Encoder::
putBoolean (bool value)
  throw ()
{
  if (value) {
    m_data.append ("\1", 1);
  }
  else {
    m_data.append ("\0", 1);
  }
}

void
CPI::Util::CDR::Encoder::
putOctet (unsigned char value)
  throw ()
{
  m_data.append (reinterpret_cast<char *> (&value), 1);
}

void
CPI::Util::CDR::Encoder::
putUShort (CPI::OS::uint16_t value)
  throw ()
{
  align (2);
  m_data.append (reinterpret_cast<char *> (&value), 2);
}

void
CPI::Util::CDR::Encoder::
putULong (CPI::OS::uint32_t value)
  throw ()
{
  align (4);
  m_data.append (reinterpret_cast<char *> (&value), 4);
}
void
CPI::Util::CDR::Encoder::
putLong (CPI::OS::int32_t value)
  throw ()
{
  align (4);
  m_data.append (reinterpret_cast<char *> (&value), 4);
}

void
CPI::Util::CDR::Encoder::
putULongLong (CPI::OS::uint64_t value)
  throw ()
{
  align (8);
  m_data.append (reinterpret_cast<char *> (&value), 8);
}

void
CPI::Util::CDR::Encoder::
putString (const std::string & value)
  throw ()
{
  putULong (value.length() + 1);
  m_data.append (value.data(), value.length());
  m_data.append ("\0", 1);
}

void
CPI::Util::CDR::Encoder::
putOctetSeq (const std::string & value)
  throw ()
{
  putULong (value.length());
  m_data.append (value.data(), value.length());
}

const std::string &
CPI::Util::CDR::Encoder::
data () const
  throw ()
{
  return m_data;
}

/*
 * ----------------------------------------------------------------------
 * CDR Decoder
 * ----------------------------------------------------------------------
 */

CPI::Util::CDR::Decoder::
Decoder (const void * data, unsigned long size)
  throw ()
  : m_pos (0),
    m_len (size),
    m_data (reinterpret_cast<const char *> (data))
{
  m_dataByteorder = m_nativeByteorder = nativeByteorder ();
}

CPI::Util::CDR::Decoder::
Decoder (const std::string & data)
  throw ()
  : m_pos (0),
    m_len (data.length()),
    m_data (data.data())
{
  m_dataByteorder = m_nativeByteorder = nativeByteorder ();
}

void
CPI::Util::CDR::Decoder::
byteorder (bool bo)
  throw ()
{
  m_dataByteorder = bo;
}

bool
CPI::Util::CDR::Decoder::
byteorder () const
  throw ()
{
  return m_dataByteorder;
}

void
CPI::Util::CDR::Decoder::
align (unsigned long modulus)
  throw ()
{
  m_pos = modulus*((m_pos+modulus-1)/modulus);
}

unsigned long
CPI::Util::CDR::Decoder::
remainingData () const
  throw ()
{
  if (m_pos > m_len) {
    return 0;
  }

  return m_len - m_pos;
}

void
CPI::Util::CDR::Decoder::
getBoolean (bool & value)
  throw (InvalidData)
{
  if (m_pos+1 > m_len) {
    throw InvalidData();
  }

  switch (m_data[m_pos]) {
  case '\0':
    value = false;
    break;

  case '\1':
    value = true;
    break;

  default:
    throw InvalidData();
  }

  m_pos++;
}

void
CPI::Util::CDR::Decoder::
getOctet (unsigned char & value)
  throw (InvalidData)
{
  if (m_pos+1 > m_len) {
    throw InvalidData();
  }

  value = static_cast<unsigned char> (m_data[m_pos++]);
}

void
CPI::Util::CDR::Decoder::
getUShort (CPI::OS::uint16_t & value)
  throw (InvalidData)
{
  align (2);

  if (m_pos+2 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const CPI::OS::uint16_t*> (m_data+m_pos);
  }
  else {
    copyswap2 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 2;
}

void
CPI::Util::CDR::Decoder::
getULong (CPI::OS::uint32_t & value)
  throw (InvalidData)
{
  align (4);

  if (m_pos+4 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const CPI::OS::uint32_t*> (m_data+m_pos);
  }
  else {
    copyswap4 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 4;
}
void
CPI::Util::CDR::Decoder::
getLong (CPI::OS::int32_t & value)
  throw (InvalidData)
{
  align (4);

  if (m_pos+4 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const CPI::OS::int32_t*> (m_data+m_pos);
  }
  else {
    copyswap4 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 4;
}

void
CPI::Util::CDR::Decoder::
getULongLong (CPI::OS::uint64_t & value)
  throw (InvalidData)
{
  align (8);

  if (m_pos+8 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const CPI::OS::uint64_t*> (m_data+m_pos);
  }
  else {
    copyswap8 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 8;
}

void
CPI::Util::CDR::Decoder::
getString (std::string & value)
  throw (InvalidData)
{
  CPI::OS::uint32_t length;
  getULong (length);

  if (m_pos+length > m_len) {
    throw InvalidData();
  }

  value.assign (m_data+m_pos, length-1);
  m_pos += length;
}

void
CPI::Util::CDR::Decoder::
getOctetSeq (std::string & value)
  throw (InvalidData)
{
  CPI::OS::uint32_t length;
  getULong (length);

  if (m_pos+length > m_len) {
    throw InvalidData();
  }

  value.assign (m_data+m_pos, length);
  m_pos += length;
}
