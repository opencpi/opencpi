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

#include <OcpiUtilUri.h>
#include <string>
#include <cstring>
#include <cctype>

/*
 * ----------------------------------------------------------------------
 * Escape-encode and decode strings.
 * ----------------------------------------------------------------------
 */

namespace {

  std::string
  decodeURI (const char * uri, size_t length)
    throw ()
  {
    std::string res;
    int c, x1, x2;

    while (length) {
      if (length >= 2 && *uri == '%' &&
          OCPI::Util::Uri::ishex (*(uri+1)) && OCPI::Util::Uri::ishex (*(uri+2))) {
        x1 = *++uri;
        x2 = *++uri;
        c  = ((x1 >= 'A' && x1 <= 'F') ? (x1 - 'A' + 10) :
              (x1 >= 'a' && x1 <= 'f') ? (x1 - 'a' + 10) : (x1 - '0')) << 4;
        c |= ((x2 >= 'A' && x2 <= 'F') ? (x2 - 'A' + 10) :
              (x2 >= 'a' && x2 <= 'f') ? (x2 - 'a' + 10) : (x2 - '0'));
        res += (char) c;
        length -= 2;
      }
      else {
        res += *uri;
      }
      length--;
      uri++;
    }

    return res;
  }

  std::string
  encodeURI (const char * uri, size_t length,
             const char * allowedChars, size_t numAllowedChars)
    throw ()
  {
    std::string res;
    int x1, x2;

    bool isallowed=false;
    unsigned int isai;

    while (length) {
      if (allowedChars) {
        isallowed = false;
        for (isai=0; isai < numAllowedChars; isai++) {
          if (allowedChars[isai] == *uri) {
            isallowed = true;
            break;
          }
        }
      }

      if (!OCPI::Util::Uri::isunreserved (*uri) && !isallowed) {
        x1 = (unsigned char) *uri >> 4;
        x2 = (unsigned char) *uri & 15;
        res += '%';
        res += (char)((x1 > 9) ? ('a' + x1 - 10) : ('0' + x1));
        res += (char)((x2 > 9) ? ('a' + x2 - 10) : ('0' + x2));
      }
      else {
        res += *uri;
      }

      length--;
      uri++;
    }

    return res;
  }
}

std::string
OCPI::Util::Uri::decode (const char * uri)
  throw (std::string)
{
  if (!uri) {
    throw std::string ("null uri");
  }

  return decodeURI (uri, std::strlen (uri));
}

std::string
OCPI::Util::Uri::decode (const std::string & uri)
  throw (std::string)
{
  return decodeURI (uri.data(), uri.length());
}

std::string
OCPI::Util::Uri::encode (const char * uri, const char * allowedChars)
  throw (std::string)
{
  if (!uri) {
    throw std::string ("null uri");
  }

  return encodeURI (uri, std::strlen (uri), allowedChars,
                    allowedChars ? std::strlen (allowedChars) : 0);
}

std::string
OCPI::Util::Uri::encode (const std::string & uri, const char * allowedChars)
  throw (std::string)
{
  return encodeURI (uri.data(), uri.length(), allowedChars,
                    allowedChars ? std::strlen (allowedChars) : 0);
}

std::string
OCPI::Util::Uri::encode (const std::string & uri, const std::string & allowedChars)
  throw (std::string)
{
  return encodeURI (uri.data(), uri.length(),
                    allowedChars.data(), allowedChars.length());
}

/*
 * ----------------------------------------------------------------------
 * Escape-encode and decode strings.
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::Uri::isPrefix (const std::string & fullURI,
                          const std::string & baseURI,
                          std::string * prefix, std::string * tail)
  throw ()
{
  std::string::size_type idx1, idx2;
  int c, x1, x2;

  idx1 = idx2 = 0;

  while (idx1 < fullURI.length() && idx2 < baseURI.length()) {
    if (fullURI[idx1] == '%' && baseURI[idx2] == '%') {
      if (idx1+3 > fullURI.length() || idx2+3 > baseURI.length() ||
          tolower(fullURI[idx1+1]) != tolower(baseURI[idx2+1]) ||
          tolower(fullURI[idx1+2]) != tolower(baseURI[idx2+2])) {
        return false;
      }
      idx1 += 3;
      idx2 += 3;
    }
    else if (fullURI[idx1] == '%') {
      if (idx1+3 > fullURI.length()) {
        return false;
      }

      x1 = fullURI[idx1+1];
      x2 = fullURI[idx1+2];
      c  = ((x1 >= 'A' && x1 <= 'F') ? (x1 - 'A' + 10) :
            (x1 >= 'a' && x1 <= 'f') ? (x1 - 'a' + 10) : (x1 - '0')) << 4;
      c |= ((x2 >= 'A' && x2 <= 'F') ? (x2 - 'A' + 10) :
            (x2 >= 'a' && x2 <= 'f') ? (x2 - 'a' + 10) : (x2 - '0'));

      if (c != baseURI[idx2]) {
        return false;
      }

      idx1 += 3;
      idx2++;
    }
    else if (baseURI[idx2] == '%') {
      if (idx2+3 > baseURI.length()) {
        return false;
      }

      x1 = baseURI[idx2+1];
      x2 = baseURI[idx2+2];
      c  = ((x1 >= 'A' && x1 <= 'F') ? (x1 - 'A' + 10) :
            (x1 >= 'a' && x1 <= 'f') ? (x1 - 'a' + 10) : (x1 - '0')) << 4;
      c |= ((x2 >= 'A' && x2 <= 'F') ? (x2 - 'A' + 10) :
            (x2 >= 'a' && x2 <= 'f') ? (x2 - 'a' + 10) : (x2 - '0'));

      if (c != fullURI[idx1]) {
        return false;
      }

      idx1++;
      idx2 += 3;
    }
    else if (fullURI[idx1] != baseURI[idx2]) {
      return false;
    }
    else {
      idx1++;
      idx2++;
    }
  }

  if (idx2 < baseURI.length()) {
    return false;
  }

  if (prefix) {
    *prefix = fullURI.substr (0, idx1);
  }

  if (tail) {
    *tail = fullURI.substr (idx1);
  }

  return true;
}

/*
 * ----------------------------------------------------------------------
 * URI class begins here
 * ----------------------------------------------------------------------
 */

/*
 * Constructors and assignment operators
 */

OCPI::Util::Uri::Uri ()
  throw ()
{
}

OCPI::Util::Uri::Uri (const char * theuri)
  throw (std::string)
{
  parse (std::string (theuri));
}

OCPI::Util::Uri::Uri (const std::string & theuri)
  throw (std::string)
{
  parse (theuri);
}

OCPI::Util::Uri::Uri (const OCPI::Util::Uri & theuri)
  throw ()
{
  parse (theuri.get());
}

OCPI::Util::Uri &
OCPI::Util::Uri::operator= (const char * theuri)
  throw (std::string)
{
  parse (std::string (theuri));
  return *this;
}

OCPI::Util::Uri &
OCPI::Util::Uri::operator= (const std::string & theuri)
  throw (std::string)
{
  parse (theuri);
  return *this;
}

OCPI::Util::Uri &
OCPI::Util::Uri::operator= (const OCPI::Util::Uri & theuri)
  throw ()
{
  parse (theuri.get());
  return *this;
}

/*
 * Resolve an URI, using ourselves as the base URI
 */

OCPI::Util::Uri &
OCPI::Util::Uri::operator+= (const char * theuri)
  throw (std::string)
{
  resolve (Uri (theuri));
  return *this;
}

OCPI::Util::Uri &
OCPI::Util::Uri::operator+= (const std::string & theuri)
  throw (std::string)
{
  resolve (Uri (theuri));
  return *this;
}

OCPI::Util::Uri &
OCPI::Util::Uri::operator+= (const OCPI::Util::Uri & theuri)
  throw (std::string)
{
  resolve (theuri);
  return *this;
}

/*
 * Determine if URI is absolute or relative
 */

bool
OCPI::Util::Uri::isAbsolute () const
  throw ()
{
  if (scheme.length()) {
    return true;
  }

  return false;
}

bool
OCPI::Util::Uri::isRelative () const
  throw ()
{
  return !isAbsolute ();
}

const std::string &
OCPI::Util::Uri::get () const
  throw ()
{
  return uri;
}

const std::string &
OCPI::Util::Uri::getScheme () const
  throw ()
{
  return scheme;
}

const std::string &
OCPI::Util::Uri::getPath () const
  throw ()
{
  return path;
}

const std::string &
OCPI::Util::Uri::getFileName () const
  throw ()
{
  return filename;
}

const std::string &
OCPI::Util::Uri::getQuery () const
  throw ()
{
  return query;
}

const std::string &
OCPI::Util::Uri::getRequest () const
  throw ()
{
  return request;
}

const std::string &
OCPI::Util::Uri::getAuthority () const
  throw ()
{
  return authority;
}

const std::string &
OCPI::Util::Uri::getUserinfo () const
  throw ()
{
  return userinfo;
}

const std::string &
OCPI::Util::Uri::getHostport () const
  throw ()
{
  return hostport;
}

const std::string &
OCPI::Util::Uri::getHost () const
  throw ()
{
  return host;
}

const std::string &
OCPI::Util::Uri::getPort () const
  throw ()
{
  return port;
}

const std::string &
OCPI::Util::Uri::getFragment () const
  throw ()
{
  return fragment;
}

unsigned int
OCPI::Util::Uri::getNumPathSegments () const
  throw ()
{
  std::string::size_type segBegin, segEnd;
  unsigned int count = 0;

  if (path.length() == 0) {
    return 0;
  }
  else if (path.length() && path[0] == '/') {
    segBegin = 1;
  }
  else {
    segBegin = 0;
  }

  do {
    segEnd = path.find ('/', segBegin);

    if (segEnd == std::string::npos) {
      segBegin = segEnd;
    }
    else {
      segBegin = segEnd + 1;
    }

    count++;
  }
  while (segBegin != std::string::npos);

  return count;
}

std::string
OCPI::Util::Uri::getPathSegment (unsigned int num) const
  throw ()
{
  std::string::size_type segBegin, segEnd;
  unsigned int count = 0;

  if (path.length() == 0) {
    return std::string();
  }
  else if (path.length() && path[0] == '/') {
    segBegin = 1;
  }
  else {
    segBegin = 0;
  }

  do {
    segEnd = path.find ('/', segBegin);

    if (count == num) {
      if (segEnd == std::string::npos) {
        return path.substr (segBegin);
      }
      else {
        return path.substr (segBegin, segEnd - segBegin);
      }
    }

    if (segEnd == std::string::npos) {
      segBegin = segEnd;
    }
    else {
      segBegin = segEnd + 1;
    }

    count++;
  }
  while (segBegin != std::string::npos);

  return std::string();
}

/*
 * Parse an encoded URI into its components
 */

void
OCPI::Util::Uri::parse (const std::string & data)
  throw (std::string)
{
  std::string::size_type idx, beg, pos;
  std::string::size_type uriLength = data.length ();
  uri = data;

  /*
   * Parse scheme.
   */

  if (uriLength && isalpha (uri[0])) {
    for (idx=1; idx<uriLength; idx++) {
      if (!(isalpha (uri[idx]) || isdigit (uri[idx]) ||
            uri[idx] == '+' || uri[idx] == '-' || uri[idx] == '.')) {
        break;
      }
    }

    if (idx < uriLength && uri[idx] == ':') {
      scheme = uri.substr (0, idx++);
    }
    else {
      scheme.clear ();
      idx = 0;
    }
  }
  else {
    idx = 0;
  }

  /*
   * Parse authority.
   */

  if (idx+1 < uriLength && uri[idx] == '/' && uri[idx+1] == '/') {
    for (beg = idx += 2; idx < uriLength; idx++) {
      if (!(isunreserved (uri[idx]) || uri[idx] == '%' || uri[idx] == '$' ||
            uri[idx] == ',' || uri[idx] == ';' || uri[idx] == ':' ||
            uri[idx] == '@' || uri[idx] == '&' || uri[idx] == '=' ||
            uri[idx] == '+' || uri[idx] == '$')) {
        break;
      }
    }

    authority = uri.substr (beg, idx-beg);

    if ((pos = authority.find ('@')) != std::string::npos) {
      userinfo = authority.substr (0, pos);
      hostport = authority.substr (pos+1);
    }
    else {
      userinfo.clear ();
      hostport = authority;
    }

    if ((pos = hostport.find (':')) != std::string::npos) {
      host = hostport.substr (0, pos);
      port = hostport.substr (pos+1);
    }
    else {
      host = hostport;
      port.clear ();
    }
  }
  else {
    authority.clear ();
    userinfo.clear ();
    hostport.clear ();
    host.clear ();
    port.clear ();
  }

  /*
   * Parse absolute or relative path.
   */

  for (beg=idx; idx < uriLength; idx++) {
    if (!(isunreserved (uri[idx]) ||
          uri[idx] == '%' || uri[idx] == '/' || uri[idx] == ';' ||
          uri[idx] == ':' || uri[idx] == '@' || uri[idx] == '&' ||
          uri[idx] == '=' || uri[idx] == '+' || uri[idx] == '$' ||
          uri[idx] == ',')) {
      break;
    }
  }

  path = uri.substr (beg, idx-beg);

  if (path.length()) {
    request = path;
  }
  else {
    request = "/";
  }

  if ((pos = path.rfind ('/')) != std::string::npos) {
    filename = path.substr (pos+1);
  }
  else if (pos != path.length() - 1) {
    filename = path;
  }
  else {
    filename.clear ();
  }

  /*
   * Parse query.
   */

  if (idx < uriLength && uri[idx] == '?') {
    for (beg = ++idx; idx < uriLength; idx++) {
      if (!isuric (uri[idx])) {
        break;
      }
    }

    query = uri.substr (beg, idx-beg);
    request += "?";
    request += query;
  }
  else {
    query.clear ();
  }

  /*
   * Parse fragment.
   */

  if (idx < uriLength && uri[idx] == '#') {
    for (beg = ++idx; idx < uriLength; idx++) {
      if (!isuric (uri[idx])) {
        break;
      }
    }

    fragment = uri.substr (beg, idx-beg);
  }
  else {
    fragment.clear ();
  }

  /*
   * This should be the end, i.e. idx==uri.length(). If not, then the
   * parsing above got stuck on some unexpected character.
   */

  if (idx < uriLength) {
    std::string reason = "Invalid character: ";
    switch (uri[idx]) {
    case 9:  reason += "(tab)"; break;
    case 32: reason += "(space)"; break;
    default: {
      char c1, c2;
      int x1, x2;
      x1 = (unsigned char) uri[idx] >> 4;
      x2 = (unsigned char) uri[idx] & 15;
      c1 = (char)(((x1 > 9) ? ('a' + x1 - 10) : ('0' + x1)));
      c2 = (char)(((x2 > 9) ? ('a' + x2 - 10) : ('0' + x2)));

      reason += "'";
      reason += "%";
      reason += c1;
      reason += c2;
      reason += "'";
      break;
    }
    }
    throw reason;
  }
}

void
OCPI::Util::Uri::resolve (const OCPI::Util::Uri & other)
  throw (std::string)
{
  if (other.isAbsolute()) {
    operator= (other);
    return;
  }

  if (isRelative()) {
    throw std::string ("cannot resolve against relative URI");
  }

  if (other.authority.length()) {
    authority = other.authority;
    hostport = other.hostport;
    host = other.host;
    port = other.port;
    path = other.path;
    filename = other.filename;
    query = other.query;
  }
  else if (other.path.length() > 0 && other.path[0] == '/') {
    path = other.path;
    filename = other.filename;
    query = other.query;
  }
  else if (other.path.length() || other.query.length()) {
    std::string::size_type pos;
    std::string newpath;

    if ((pos = path.rfind ('/')) != std::string::npos) {
      newpath = path.substr (0, pos+1);
    }
    else {
      // should not happen, as a base URI should always use an absolute path
      newpath = "/";
    }

    newpath += other.path;

    path = normalizePath (newpath);
    filename = other.filename;
    query = other.query;
  }
  else if (other.query.length()) {
    query = other.query;
  }

  if (path.length()) {
    request = path;
  }
  else {
    request = "/";
  }

  if (query.length()) {
    request += "?";
    request += query;
  }

  fragment = other.fragment;

  /*
   * rebuild full URI
   */

  uri = scheme;
  uri += "://";
  uri += authority;
  uri += path;

  if (query.length()) {
    uri += '?';
    uri += query;
  }

  if (fragment.length()) {
    uri += '#';
    uri += fragment;
  }
}

/*
 * Normalize a path, removing instances of <segment>/..
 */

std::string
OCPI::Util::Uri::normalizePath (const std::string & path)
  throw ()
{
  std::string::size_type beg, pos, last;
  std::string res, seg;

  if (path.length() == 0 || path[0] != '/') {
    return path;
  }

  beg = 1;

  do {
    if ((pos = path.find ('/', beg)) != std::string::npos) {
      seg = path.substr (beg, pos-beg);
      beg = pos+1;
    }
    else {
      seg = path.substr (beg);
    }

    if (seg.length() == 1 && seg[0] == '.') {
      if (pos == std::string::npos) {
        res += "/";
      }
      continue;
    }
    else if (seg.length() == 2 && seg[0] == '.' && seg[1] == '.') {
      if ((last = res.rfind ('/')) != std::string::npos) {
        res = res.substr (0, last);
        if (pos == std::string::npos) {
          res += "/";
        }
      }
    }
    else {
      res += "/";
      res += seg;
    }
  }
  while (pos != std::string::npos);

  if (res.length() == 0) {
    res = '/';
  }

  return res;
}
