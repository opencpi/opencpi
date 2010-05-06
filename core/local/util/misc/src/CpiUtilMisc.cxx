#include <CpiUtilMisc.h>
#include <istream>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <assert.h>

/*
 * ----------------------------------------------------------------------
 * Integer to String conversion
 * ----------------------------------------------------------------------
 */

namespace {
  char i2sDigits[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };
}

std::string
CPI::Util::Misc::integerToString (int value)
{
  bool positive = (value >= 0);
  int count=1, tmp;

  if (!positive) {
    value = -value;
    count++;
  }

  for (tmp=value; tmp>9; count++) {
    tmp /= 10;
  }

  std::string result;
  result.resize (count);

  if (!positive) {
    result[0] = '-';
  }

  while (value > 9) {
    result[--count] = i2sDigits[value%10];
    value /= 10;
  }

  // assert (count == positive ? 1 : 2)

  result[--count] = i2sDigits[value%10];
  return result;
}

std::string
CPI::Util::Misc::unsignedToString (unsigned int value,
                                   unsigned int base,
                                   unsigned int mindigits,
                                   char pad)
{
  unsigned int tmp;
  unsigned int count=1;
  unsigned int padLen;

  if (base < 2 || base > 16) {
    throw std::string ("invalid base");
  }

  /*
   * Determine length of number.
   */

  for (tmp=value; tmp>=base; count++) {
    tmp /= base;
  }

  /*
   * Initialize string.
   */

  std::string result;

  if (mindigits > count) {
    padLen = mindigits - count;
    result.resize (mindigits, pad);
  }
  else {
    padLen = 0;
    result.resize (count);
  }

  /*
   * Stringify number.
   */

  while (value >= base) {
    result[--count+padLen] = i2sDigits[value%base];
    value /= base;
  }

  // assert (count == 1 && value < base)

  result[--count+padLen] = i2sDigits[value];
  return result;
}

std::string
CPI::Util::Misc::unsignedToString (unsigned long long value,
                                   unsigned int base,
                                   unsigned int mindigits,
                                   char pad)
{
  unsigned long long tmp;
  unsigned int count=1;
  unsigned int padLen;

  if (base < 2 || base > 16) {
    throw std::string ("invalid base");
  }

  /*
   * Determine length of number.
   */

  for (tmp=value; tmp>=base; count++) {
    tmp /= base;
  }

  /*
   * Initialize string.
   */

  std::string result;

  if (mindigits > count) {
    padLen = mindigits - count;
    result.resize (mindigits, pad);
  }
  else {
    padLen = 0;
    result.resize (count);
  }

  /*
   * Stringify number.
   */

  while (value >= base) {
    result[--count+padLen] = i2sDigits[value%base];
    value /= base;
  }

  // assert (count == 1 && value < base)

  result[--count+padLen] = i2sDigits[value];
  return result;
}

std::string
CPI::Util::Misc::doubleToString (double value)
{
  char tmp[32]; // C Programmer's Disease
  sprintf (tmp, "%g", value);
  return std::string (tmp);
}

/*
 * ----------------------------------------------------------------------
 * String to integer conversion
 * ----------------------------------------------------------------------
 */

int
CPI::Util::Misc::stringToInteger (const std::string & str)
{
  const char * txtPtr = str.c_str ();
  long int value;
  char * endPtr;

  errno = 0;
  value = std::strtol (txtPtr, &endPtr, 10);

  if ((value == 0 && endPtr == txtPtr) || errno == ERANGE || *endPtr) {
    throw std::string ("not an integer");
  }

  int res = static_cast<int> (value);

  if (static_cast<long int> (res) != value) {
    throw std::string ("integer out of range");
  }

  return res;
}

unsigned int
CPI::Util::Misc::stringToUnsigned (const std::string & str,
                                   unsigned int base)
{
  const char * txtPtr = str.c_str ();
  unsigned long int value;
  char * endPtr;

  errno = 0;
  value = std::strtoul (txtPtr, &endPtr, base);

  if ((value == 0 && endPtr == txtPtr) || errno == ERANGE || *endPtr) {
    throw std::string ("not an unsigned integer");
  }

  unsigned int res = static_cast<unsigned int> (value);

  if (static_cast<unsigned long int> (res) != value) {
    throw std::string ("unsigned integer out of range");
  }

  return res;
}

unsigned long long
CPI::Util::Misc::stringToULongLong (const std::string & str,
                                    unsigned int base)
{
  const char * txtPtr = str.c_str ();
  char * endPtr;

  errno = 0;

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
  unsigned long long int value;
  value = std::strtoull (txtPtr, &endPtr, base);
#else
  long int value;
  value = std::strtoul (txtPtr, &endPtr, base);
#endif

  if ((value == 0 && endPtr == txtPtr) || errno == ERANGE || *endPtr) {
    throw std::string ("not an unsigned integer");
  }

  return value;
}

/*
 * ----------------------------------------------------------------------
 * Convert an unsigned long long to a std::streamsize. If the value
 * does not fit, return -1 as an error indication, if minusone is true,
 * else return the maximum value that fits into a std::streamsize.
 * ----------------------------------------------------------------------
 */

std::streamsize
CPI::Util::Misc::unsignedToStreamsize (unsigned long long value, bool minusone)
{
  /*
   * Microsoft C++ generates a "W1" warning, even when static_cast<> is
   * used. But we know what we're doing.
   */

#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable:4244)
#endif

  std::streamsize res = static_cast<std::streamsize> (value);

  if (res < 0 || value != static_cast<unsigned long long> (res)) {
    if (!minusone) {
      return static_cast<std::streamsize> (1<<31);
    }

    return static_cast<std::streamsize> (-1);
  }

#if defined (_MSC_VER)
#pragma warning (pop)
#endif

  return res;
}

/*
 * ----------------------------------------------------------------------
 * Read a line, discard trailing WS
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Misc::readline (std::istream * conn, unsigned int maxSize)
{
  std::string res;
  char c;
  
  while (conn->good() && !conn->eof()) {
    if (!conn->read (&c, 1).good()) {
      break;
    }

    if (c == '\n') {
      break;
    }

    if (res.length() == maxSize) {
      conn->setstate (std::ios_base::failbit);
      break;
    }

    res += c;
  }

  std::string::size_type pos = res.find_last_not_of (" \t\r");

  if (pos == std::string::npos) {
    res.clear ();
  }
  else if (pos != res.length() - 1) {
    res.resize (pos+1);
  }
  
  return res;
}

/*
 * ----------------------------------------------------------------------
 * Case insensitive string comparison
 * ----------------------------------------------------------------------
 */

int
CPI::Util::Misc::
caseInsensitiveStringCompare (const std::string & s1,
                              const std::string & s2)
{
  int s1l = s1.length ();
  int s2l = s2.length ();
  int l = (s1l < s2l) ? s1l : s2l;
  int i, c;

  for (i=0; i<l; i++) {
    if (std::isalpha (s1[i]) && std::isalpha (s2[i])) {
      c = std::tolower (s2[i]) - std::tolower (s1[i]);
    }
    else if (std::isalpha (s1[i])) {
      c = std::tolower (s2[i]) - s1[i];
    }
    else if (std::isalpha (s2[i])) {
      c = s2[i] - std::tolower (s1[i]);
    }
    else {
      c = s2[i] - s1[i];
    }

    if (c != 0) {
      return c;
    }
  }
  
  return s2l - s1l;
}

bool
CPI::Util::Misc::CaseInsensitiveStringLess::
operator() (const std::string & s1,
            const std::string & s2) const
{
  return (caseInsensitiveStringCompare (s1, s2) < 0);
}

/*
 * ----------------------------------------------------------------------
 * See if a glob matches a string
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::Misc::glob (const std::string & str,
                       const std::string & pat)
{
  int strIdx = 0, strLen = str.length ();
  int patIdx = 0, patLen = pat.length ();
  const char * name = str.data ();
  const char * pattern = pat.data ();

  while (strIdx < strLen && patIdx < patLen) {
    if (*pattern == '*') {
      pattern++;
      patIdx++;
      while (*pattern == '*' && patIdx < patLen) {
        pattern++;
        patIdx++;
      }
      while (strIdx < strLen) {
        if (glob (name, pattern)) {
          return true;
        }
        strIdx++;
        name++;
      }
      return (patIdx < patLen) ? false : true;
    }
    else if (*pattern == '?' || *pattern == *name) {
      pattern++;
      patIdx++;
      name++;
      strIdx++;
    }
    else {
      return false;
    }
  }

  while (*pattern == '*' && patIdx < patLen) {
    pattern++;
    patIdx++;
  }

  if (patIdx < patLen || strIdx < strLen) {
    return false;
  }

  return true;
}

/*
 * ----------------------------------------------------------------------
 * Determine if a file is an XML file or not
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::Misc::isXMLDocument (std::istream * istr)
{
  /*
   * We read the first four bytes of the file, which must be "<?xm"
   * for an XML file
   */

  char data[4];
  bool result;

  istr->read (data, 4);

  if (istr->gcount() != 4) {
    // file shorter than 4 bytes. can't be XML.
    istr->seekg (0, std::ios_base::beg);
    return false;
  }

  if (data[0] == '<' && data[1] == '?' &&
      data[2] == 'x' && data[3] == 'm') {
    result = true;
  }
  else {
    result = false;
  }

  istr->seekg (-4, std::ios_base::cur);
  return result;
}
