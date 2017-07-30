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

#include <OcpiUtilVfsMd5.h>
#include <OcpiUtilVfs.h>
#include <iostream>
#include "md5.h"

/*
 * Default buffer size
 */

namespace {

  const std::streamsize DEFAULT_BUFFER_SIZE = 65536;
}

/*
 * ----------------------------------------------------------------------
 * Checksums
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Vfs::md5 (std::istream * str, std::streamsize count)
  throw (std::string)
{
  char buffer[DEFAULT_BUFFER_SIZE];
  unsigned char hashValue[16];
  md5_state_t hash;

  md5_init (&hash);

  while (!str->eof() && count) {
    std::streamsize maxCount, amount;

    if (count != -1) {
      maxCount = (DEFAULT_BUFFER_SIZE > count) ? DEFAULT_BUFFER_SIZE : count;
    }
    else {
      maxCount = DEFAULT_BUFFER_SIZE;
    }

    str->read (buffer, maxCount);

    if (!str->good()) {
      break;
    }

    amount = str->gcount ();

    md5_append (&hash,
                reinterpret_cast<unsigned char *> (buffer),
                (unsigned)amount);

    if (count != -1) {
      count -= amount;
    }
  }

  md5_finish (&hash, hashValue);

  if (str->bad()) {
    throw std::string ("error reading from stream");
  }

  static char hexDigit[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };

  char hexHash[32];
  char *hhptr = hexHash;

  for (int i=0; i<16; i++) {
    *hhptr++ = hexDigit[hashValue[i] >> 4];
    *hhptr++ = hexDigit[hashValue[i] & 15];
  }

  return std::string (hexHash, 32);
}

std::string
OCPI::Util::Vfs::md5 (Vfs * fs, const std::string & fileName)
  throw (std::string)
{
  std::istream * str = fs->openReadonly (fileName, std::ios_base::binary);
  std::string res;

  try {
    res = md5 (str);
  }
  catch (...) {
    try {
      fs->close (str);
    }
    catch (...) {
    }

    throw;
  }

  fs->close (str);
  return res;
}

