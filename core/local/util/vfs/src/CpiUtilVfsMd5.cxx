#include <CpiUtilVfsMd5.h>
#include <CpiUtilVfs.h>
#include <iostream>
#include "md5.h"

/*
 * Default buffer size
 */

namespace {
  enum {
    DEFAULT_BUFFER_SIZE = 65536
  };
}

/*
 * ----------------------------------------------------------------------
 * Checksums
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Vfs::md5 (std::istream * str, std::streamsize count)
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
                amount);

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
CPI::Util::Vfs::md5 (Vfs * fs, const std::string & fileName)
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

