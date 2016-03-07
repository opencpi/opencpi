/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdarg.h>
#include <assert.h>
#include <istream>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include "OcpiOsEther.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilException.h"
#include "OcpiUtilMisc.h"

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

namespace OE = OCPI::OS::Ether;
namespace OCPI {
  namespace Util {
std::string
integerToString (int value)
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
unsignedToString (unsigned int value,
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
unsignedToString (unsigned long long value,
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
doubleToString (double value)
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
stringToInteger (const std::string & str)
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
stringToUnsigned (const std::string & str,
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
stringToULongLong (const std::string & str,
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
unsignedToStreamsize (unsigned long long value, bool minusone)
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
readline (std::istream * conn, unsigned int maxSize)
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

caseInsensitiveStringCompare (const std::string & s1,
                              const std::string & s2)
{
  size_t s1l = s1.length ();
  size_t s2l = s2.length ();
  size_t l = (s1l < s2l) ? s1l : s2l;
  size_t i;
  int c;

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
  
  return (int)(s2l - s1l);
}

bool
CaseInsensitiveStringLess::
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
glob (const std::string & str,
                       const std::string & pat)
{
  size_t strIdx = 0, strLen = str.length ();
  size_t patIdx = 0, patLen = pat.length ();
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
isXMLDocument (std::istream * istr)
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

void 
formatAddV(std::string &out, const char *fmt, va_list ap) {
  char *cp;
  vasprintf(&cp, fmt, ap);
  assert(cp); // or better generic memory exception
  out += cp;
  free(cp);
}
// FIXME remove this when all callers are fixed.
void 
formatString(std::string &out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  out.clear();
  formatAddV(out, fmt, ap);
  va_end(ap);
}
void 
format(std::string &out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  out.clear();
  formatAddV(out, fmt, ap);
  va_end(ap);
}
bool 
eformat(std::string &out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  out.clear();
  formatAddV(out, fmt, ap);
  va_end(ap);
  return true;
}
void 
formatAdd(std::string &out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  formatAddV(out, fmt, ap);
  va_end(ap);
}

// FIXME: Use vanilla C file I/O
const char *
file2String(std::string &out, const char *file, char replaceNewLine) {
  FILE *f = fopen(file, "r");
  long size;
  const char *err = NULL;

  if (f &&
      fseek(f, 0, SEEK_END) == 0 &&
      (size = ftell(f)) > 0 &&
      fseek(f, 0, SEEK_SET) == 0) {
    out.reserve(size);
    // To avoid requiring double storage, we chunk the input.
    char buf[4*1024+1];
    bool initial = true;  // for trimming initial which space
    bool newLine = false; // for trimming trailing newline when replacing newlines
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf)-1, f))) {
      buf[n] = '\0';
      if (strlen(buf) < n)
	n = strlen(buf);
      char *cp = buf;
      if (initial) {
	// Trim initial white space
	for (; n && isspace(*cp); n--, cp++)
	  ;
	if (n)
	  initial = false;
      }
      if (replaceNewLine) {
	if (newLine) {
	  out += replaceNewLine;
	  newLine = false;
	}
	char *np = cp;
	for (size_t nn = n; nn; nn--, np++)
	  if (*np == '\n') {
	    if (nn == 1)
	      newLine = true, n--;
	    else
	      *np = replaceNewLine;
	  }
      }
      out.append(cp, n);
    }
    if (ferror(f))
      err = "error reading file";
  } else
    err = "file could not be open for reading";
   if (f)
    fclose(f);
  if (err) {
    out.clear();
    return esprintf("Can't process file \"%s\" for string: %s", file, err);
  }
  return NULL;
}
const char *
string2File(const std::string &in, const char *file) {
  FILE *f = fopen(file, "w");
  size_t n = in.size();

  if (f && fwrite(in.c_str(), 1, n, f) == n && fclose(f) == 0)
    return NULL;
  return esprintf("error writing string value to file '%s'", file);
}

const char *
esprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const char *err = evsprintf(fmt, ap);
  va_end(ap);
  return err;
}

const char *
evsprintf(const char *fmt, va_list ap) {
  char *buf;
  vasprintf(&buf, fmt, ap);
  return buf;
}

// parse an attribute value as a list separated by comma, space or tab
// and call a function with the given arg for each token found
const char *
parseList(const char *list, const char * (*doit)(const char *tok, void *arg), void *arg) {
  const char *err = 0;
  if (list) {
    char
      *mylist = strdup(list),
      *base,
      *last = 0,
      *tok;
    for (base = mylist; (tok = strtok_r(base, ", \t", &last)); base = NULL)
      if ((err = doit(tok, arg)))
        break;
    free(mylist);
  }
  return err;
}

unsigned fls64(uint64_t n) {
  for (int i = sizeof(n)*8; i > 0; i--)
    if (n & ((uint64_t)1 << (i - 1)))
      return i;
  return 0;
}

const std::string &
getSystemId() {
  static std::string *id = NULL;
  if (!id)
    id = new std::string(getSystemAddr().pretty());
  return *id;
}

OCPI::OS::Ether::Address &
getSystemAddr() {
  static bool set = false;
  static OCPI::OS::Ether::Address addr;
  // no static construction
  if (!set) {
    std::string error;
    OE::IfScanner ifs(error);
    if (error.empty()) {
      OE::Interface eif;
      while (ifs.getNext(eif, error))
	if (eif.addr.isEther()) {
	  addr = eif.addr;
	  ocpiDebug("Establishing system identify from interface '%s': %s",
		    eif.name.c_str(), addr.pretty());
	  set = true;
	  break;
	}
      if (error.empty() && !set)
	throw Error("No network interface found to establish a system identify from its MAC address");
    }
    if (error.length())
      throw Error("Error finding a network interface for establishing a system identify: %s",
		  error.c_str());
  }  
  return addr;
}
// This scheme is ours so that it is somewhat readable, xml friendly, and handles NULLs
void
encodeDescriptor(const std::string &s, std::string &out) {
  formatAdd(out, "%zu.", s.length());
  Unparser up;
  const char *cp = s.data();
  for (size_t n = s.length(); n; n--, cp++) {
    if (*cp == '\'')
      out += "&apos;";
    else if (*cp == '&')
      out += "&amp;";
    else
      up.unparseChar(out, *cp, true);
  }
}
void
decodeDescriptor(const char *info, std::string &s) {
  char *end;
  errno = 0;
  size_t infolen = strtoul(info, &end, 0);
  do {
    if (errno || infolen >= 1000 || *end++ != '.')
      break;
    s.resize(infolen);
    const char *start = end;
    end += strlen(start);
    size_t n;
    for (n = 0; n < infolen && *start; n++)
      if (parseOneChar(start, end, s[n]))
	break;
    if (*start || n != infolen)
      break;
    return;
  } while (0);
  throw Error("Invalid Port Descriptor from Container Server in: \"%s\"", info);
}
const char *
baseName(const char *path, std::string &buf) {
  buf.clear();
  if (path) {
    const char *end = path + strlen(path);
    while (end > path && end[-1] == '/')
      end--;
    if (end == path)
      return buf.c_str();
    const char *slash = strrchr(path, '/');
    slash = slash ? slash + 1 : path;
    const char *dot = strrchr(slash, '.');
    if (!dot)
      dot = end;
    buf.assign(slash, dot - slash);
  } else
    buf.clear();
  return buf.c_str();
}
// Search for the given name in a colon separated path
// Set the full constructed path in "result".
// Return true on error
bool
searchPath(const char *path, const char *item, std::string &result, const char *preferred) {
  char *cp = strdup(path), *last;
  for (char *lp = strtok_r(cp, ":", &last); lp;
       lp = strtok_r(NULL, ":", &last)) {
    format(result, "%s/", lp);
    bool isDir;
    if (preferred) {
      size_t len = result.length();
      formatAdd(result, "%s/%s", preferred, item);
      if (OS::FileSystem::exists(result, &isDir))
	return false;
      result.resize(len);
    }
    result += item;
    if (OS::FileSystem::exists(result, &isDir))
      return false;
  }
  return true;
}
  }

}
