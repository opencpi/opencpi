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

#include <OcpiOsAssert.h>
#include <OcpiOsDataTypes.h>
#include <string>
#include <cstring>
#include "OcpiUtilCDR.h"

/*
 * ----------------------------------------------------------------------
 * nativeByteorder()
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::CDR::
nativeByteorder ()
  throw ()
{
  OCPI::OS::uint32_t v = 0x01020304;
  char * cv = reinterpret_cast<char *> (&v);

  ocpiAssert ((cv[0] == 1 && cv[1] == 2 && cv[2] == 3 && cv[3] == 4) ||
             (cv[0] == 4 && cv[1] == 3 && cv[2] == 2 && cv[3] == 1));

  return (cv[0] == 1) ? false : true;
}

/*
 * ----------------------------------------------------------------------
 * CDR Encoder
 * ----------------------------------------------------------------------
 */

OCPI::Util::CDR::Encoder::
Encoder ()
  throw ()
{
}

void
OCPI::Util::CDR::Encoder::
align (unsigned long modulus)
  throw ()
{
  std::string::size_type curpos = m_data.length ();
  std::string::size_type newpos = modulus*((curpos+modulus-1)/modulus);
  m_data.resize (newpos, 'x');
}

void
OCPI::Util::CDR::Encoder::
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
OCPI::Util::CDR::Encoder::
putOctet (unsigned char value)
  throw ()
{
  m_data.append (reinterpret_cast<char *> (&value), 1);
}

void
OCPI::Util::CDR::Encoder::
putUShort (OCPI::OS::uint16_t value)
  throw ()
{
  align (2);
  m_data.append (reinterpret_cast<char *> (&value), 2);
}

void
OCPI::Util::CDR::Encoder::
putULong (OCPI::OS::uint32_t value)
  throw ()
{
  align (4);
  m_data.append (reinterpret_cast<char *> (&value), 4);
}
void
OCPI::Util::CDR::Encoder::
putLong (OCPI::OS::int32_t value)
  throw ()
{
  align (4);
  m_data.append (reinterpret_cast<char *> (&value), 4);
}

void
OCPI::Util::CDR::Encoder::
putULongLong (OCPI::OS::uint64_t value)
  throw ()
{
  align (8);
  m_data.append (reinterpret_cast<char *> (&value), 8);
}

void
OCPI::Util::CDR::Encoder::
putString (const std::string & value)
  throw ()
{
  putULong ((uint32_t)(value.length() + 1));
  m_data.append (value.data(), value.length());
  m_data.append ("\0", 1);
}

void
OCPI::Util::CDR::Encoder::
putOctetSeq (const std::string & value)
  throw ()
{
  putULong ((uint32_t)value.length());
  m_data.append (value.data(), value.length());
}

const std::string &
OCPI::Util::CDR::Encoder::
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

OCPI::Util::CDR::Decoder::
Decoder (const void * data, unsigned long size)
  throw ()
  : m_pos (0),
    m_len (size),
    m_data (reinterpret_cast<const char *> (data))
{
  m_dataByteorder = m_nativeByteorder = nativeByteorder ();
}

OCPI::Util::CDR::Decoder::
Decoder (const std::string & data)
  throw ()
  : m_pos (0),
    m_len (data.length()),
    m_data (data.data())
{
  m_dataByteorder = m_nativeByteorder = nativeByteorder ();
}

void
OCPI::Util::CDR::Decoder::
byteorder (bool bo)
  throw ()
{
  m_dataByteorder = bo;
}

bool
OCPI::Util::CDR::Decoder::
byteorder () const
  throw ()
{
  return m_dataByteorder;
}

void
OCPI::Util::CDR::Decoder::
align (unsigned long modulus)
  throw ()
{
  m_pos = modulus*((m_pos+modulus-1)/modulus);
}

unsigned long
OCPI::Util::CDR::Decoder::
remainingData () const
  throw ()
{
  if (m_pos > m_len) {
    return 0;
  }

  return m_len - m_pos;
}

void
OCPI::Util::CDR::Decoder::
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
OCPI::Util::CDR::Decoder::
getOctet (unsigned char & value)
  throw (InvalidData)
{
  if (m_pos+1 > m_len) {
    throw InvalidData();
  }

  value = static_cast<unsigned char> (m_data[m_pos++]);
}

void
OCPI::Util::CDR::Decoder::
getUShort (OCPI::OS::uint16_t & value)
  throw (InvalidData)
{
  align (2);

  if (m_pos+2 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const OCPI::OS::uint16_t*> (m_data+m_pos);
  }
  else {
    copyswap2 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 2;
}

void
OCPI::Util::CDR::Decoder::
getULong (OCPI::OS::uint32_t & value)
  throw (InvalidData)
{
  align (4);

  if (m_pos+4 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const OCPI::OS::uint32_t*> (m_data+m_pos);
  }
  else {
    copyswap4 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 4;
}
void
OCPI::Util::CDR::Decoder::
getLong (OCPI::OS::int32_t & value)
  throw (InvalidData)
{
  align (4);

  if (m_pos+4 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const OCPI::OS::int32_t*> (m_data+m_pos);
  }
  else {
    copyswap4 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 4;
}

void
OCPI::Util::CDR::Decoder::
getULongLong (OCPI::OS::uint64_t & value)
  throw (InvalidData)
{
  align (8);

  if (m_pos+8 > m_len) {
    throw InvalidData();
  }

  if (m_dataByteorder == m_nativeByteorder) {
    value = *reinterpret_cast<const OCPI::OS::uint64_t*> (m_data+m_pos);
  }
  else {
    copyswap8 (reinterpret_cast<char *> (&value), m_data+m_pos);
  }

  m_pos += 8;
}

void
OCPI::Util::CDR::Decoder::
getString (std::string & value)
  throw (InvalidData)
{
  OCPI::OS::uint32_t length;
  getULong (length);

  if (m_pos+length > m_len) {
    throw InvalidData();
  }

  value.assign (m_data+m_pos, length-1);
  m_pos += length;
}

void
OCPI::Util::CDR::Decoder::
getOctetSeq (std::string & value)
  throw (InvalidData)
{
  OCPI::OS::uint32_t length;
  getULong (length);

  if (m_pos+length > m_len) {
    throw InvalidData();
  }

  value.assign (m_data+m_pos, length);
  m_pos += length;
}
