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

// -*- c++ -*-

#ifndef OCPIUTILURI_H__
#define OCPIUTILURI_H__

/**
 * \file
 * \brief Manipulation of Universal Resource Identifiers (URI).
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <string>
#include <cctype>

namespace OCPI {
  namespace Util {

    /**
     * \brief Manipulation of Universal Resource Identifiers (URI).
     *
     * This class represents Universal Resource Identifiers (URIs) as
     * defined by RFC 2396.
     *
     * Entire URIs are always <em>escaped</em>, i.e., unreserved
     * characters shall be encoded as a percent character followed
     * by two hexadecimal digits, e.g., "%20" for a space character.
     * The individual URI components returned by the various
     * accessor functions must be unescaped using
     * OCPI::Util::Uri::decode() before being processed.
     * OCPI::Util::Uri::encode() can be used to escape components
     * while constructing a URI.
     */

    class Uri {
    public:
      /**
       * Constructor.
       *
       * Initialize an empty URI.
       */

      Uri ()
        throw ();

      /**
       * Constructor.
       *
       * Initialize the URI from a string.
       *
       * \param[in] str String in URI format.
       *
       * \throw std::string If \a can not be parsed as a URI.
       */

      explicit Uri (const char * str)
        throw (std::string);

      /**
       * Constructor.
       *
       * Initialize the URI from a string.
       *
       * \param[in] str String in URI format.
       *
       * \throw std::string If \a can not be parsed as a URI.
       */

      explicit Uri (const std::string & str)
        throw (std::string);

      /**
       * Copy constructor.
       *
       * \param[in] other Another Uri object.
       */

      Uri (const Uri & other)
        throw ();

      /**
       * \name Assignment operators.
       *
       * Re-initialize the URI from a string, or from another Uri object.
       */

      //@{

      /**
       * Assignment operator.  Re-initializes the Uri from a string.
       *
       * \param[in] str String in URI format.
       * \return *this
       *
       * \throw std::string If \a can not be parsed as a URI.
       */

      Uri & operator= (const char * str)
        throw (std::string);

      /**
       * Assignment operator.  Re-initializes the Uri from a string.
       *
       * \param[in] str String in URI format.
       * \return *this
       *
       * \throw std::string If \a can not be parsed as a URI.
       */

      Uri & operator= (const std::string & str)
        throw (std::string);

      /**
       * Assignment operator.  Copies another Uri.
       *
       * \param[in] other Another Uri object.
       * \return *this
       */

      Uri & operator= (const Uri & other)
        throw ();

      //@}

      /**
       * \name Resolve a URI relative to a base URI.
       *
       * If the right-hand side URI is absolute, this is equivalent to an
       * assignment. If the rght-side URI is relative, the object in
       * *this is interpreted as the base URI, and the right-side URI
       * is resolved relative to it.
       */

      //@{

      /**
       * Addition operator.  If \a str is an absolute URI, then \a str
       * is copied (equivalent to the assignment operator).  If \a str
       * is a relative URI, then this Uri is interpreted as the base
       * URI, \a str is resolved relative to it, and the result is
       * stored in this URI.
       *
       * \param[in] str A relative or absolute URI.
       * \return *this
       *
       * \throw std::string If \a str can not be parsed as a URI.
       * \throw std::string If this Uri is relative.
       *
       * \pre isAbsolute() is true.
       */

      Uri & operator+= (const char * str)
        throw (std::string);

      /**
       * Addition operator.  If \a str is an absolute URI, then \a str
       * is copied (equivalent to the assignment operator).  If \a str
       * is a relative URI, then this Uri is interpreted as the base
       * URI, \a str is resolved relative to it, and the result is
       * stored in this URI.
       *
       * \param[in] str A relative or absolute URI.
       * \return *this
       *
       * \throw std::string If \a str can not be parsed as a URI.
       * \throw std::string If this Uri is relative.
       *
       * \pre isAbsolute() is true.
       */

      Uri & operator+= (const std::string & str)
        throw (std::string);

      /**
       * Addition operator.  If \a other is an absolute URI, then \a other
       * is copied (equivalent to the assignment operator).  If \a other
       * is a relative URI, then this Uri is interpreted as the base
       * URI, \a other is resolved relative to it, and the result is
       * stored in this URI.
       *
       * \param[in] other A relative or absolute URI.
       * \return *this
       *
       * \throw std::string If this Uri is relative.
       *
       * \pre isAbsolute() is true.
       */

      Uri & operator+= (const Uri & other)
        throw (std::string);

      //@}

      /**
       * \name Determine if URI is absolute or relative.
       */

      //@{

      /**
       * \return True if the URI is absolute, false otherwise.
       */

      bool isAbsolute () const
        throw ();

      /**
       * \return True if the URI is relative, false otherwise.
       */

      bool isRelative () const
        throw ();

      //@}

      /**
       * \name Retrieve parts of an URI.
       */

      //@{

      /**
       * \return The entire URI.
       */

      const std::string & get () const
        throw ();

      /**
       * Return the scheme component of the URI, i.e., the string before
       * the colon.  E.g., <tt>http</tt> for the <tt>http://mc.com/</tt>
       * URI.
       *
       * \return The scheme component of the URI.
       */

      const std::string & getScheme () const
        throw ();

      /**
       * Return the request component of the URI, i.e., everything
       * after the authority, including the query, but excluding the
       * fragment. The request component is normally sent to an HTTP
       * server as the <em>Request-URI</em>.
       *
       * \return The request component of the URI, or "/" if the URI
       * did not contain a request component.
       */

      const std::string & getRequest () const
        throw ();

      /**
       * Return the path component of the URI, i.e., the string
       * that follows the authority, excluding the query and fragment.
       *
       * \return The path component of the URI, which may be the
       * empty string.
       */

      const std::string & getPath () const
        throw ();

      /**
       * Return the file name component of the URI, i.e., the last
       * segment of the path.  The file name may be empty if
       * the path ends in a slash "/" character.
       *
       * \return The file name component of the URI, which may be the
       * empty string.
       */

      const std::string & getFileName () const
        throw ();

      /**
       * Return the query component of the URI, i.e., the string
       * that follows the "?".
       *
       * \return The query component of the URI, which may be the
       * empty string.
       */

      const std::string & getQuery () const
        throw ();

      /**
       * Return the authority component of the URI.  This is the
       * string between the double slash "//" that follows the scheme
       * and the next slash.  It includes the userinfo and hostport
       * components.
       *
       * \return The authority component of the URI.
       *
       * \pre isAbsolute() is true.
       */

      const std::string & getAuthority () const
        throw ();

      /**
       * Return the userinfo component of the URI, which is the part
       * of the authority that precedes the at sign "@", if present.
       *
       * \return The userinfo component of the URI, which may be the
       * empty string.
       */

      const std::string & getUserinfo () const
        throw ();

      /**
       * Return the hostport component of the URI, which is the part
       * of the authority that follows the at sign "@", if present,
       * or is equivalent to the authority if it does not contain an
       * at sign.
       *
       * If the hostport component contains a colon ":" character,
       * then the part that precedes the colon is the host, and the
       * part that follows the colon is the port.
       *
       * \return The authority component of the URI.
       *
       * \pre isAbsolute() is true.
       */

      const std::string & getHostport () const
        throw ();

      /**
       * Return the host component of the URI, which is the part
       * of the authority that identifies a server that hosts the
       * data associated with this URI.
       *
       * \return The host component of the URI.
       *
       * \pre isAbsolute() is true.
       */

      const std::string & getHost () const
        throw ();

      /**
       * Return the port component of the URI, which is the part
       * of the authority identifies the server port number, if present.
       * Protocols may use a default port number if this component is
       * the empty string.
       *
       * \return The port component of the URI, which may be the
       * empty string.
       */

      const std::string & getPort () const
        throw ();

      /**
       * Return the fragment component of the URI, i.e., the string
       * that follows the "#" character.
       *
       * \return The fragment component of the URI, which may be the
       * empty string.
       */

      const std::string & getFragment () const
        throw ();

      //@}

      /**
       * \name Iterate over path segments in an URI.
       */

      //@{

      /**
       * \return The number of segments in the URI path component.
       */

      unsigned int getNumPathSegments () const
        throw ();

      /**
       * \param[in] n The index of the desired path segment.
       * \return The \a n th segment of the URI path component.  The last
       * path segment may be the empty string if the path ends in a slash.
       *
       * \pre \a n &lt; getNumPathSegments()
       */

      std::string getPathSegment (unsigned int n) const
        throw ();

      //@}

      /**
       * \name Helpers to escape-encode and decode strings.
       */

      //@{

      /**
       * Escape-decodes a string.
       *
       * Replaces all <em>%xx</em> sequences with the original character.
       *
       * \param[in] str A string.
       * \return The string with all escaped characters replaced with the
       * unescaped character.
       */

      static std::string decode (const char * str)
        throw (std::string);

      /**
       * Escape-decodes a string.
       *
       * Replaces all <em>%xx</em> sequences with the original character.
       *
       * \param[in] str A string.
       * \return The string with all escaped characters replaced with the
       * unescaped character.
       */

      static std::string decode (const std::string & str)
        throw (std::string);

      /**
       * Escape-encodes a string.
       *
       * Replaces all characters in the string that are not unreserved
       * or otherwise allowed with their equivalent escape sequence.
       *
       * \param[in] str A string.
       * \param[in] allowedChars If not 0, a null-terminated sequence of
       * characters that need not be escaped, in addition to the default
       * set of unreserved characters for URIs.
       * \return The string with all escaped characters replaced with the
       * unescaped character.
       */

      static std::string encode (const char * str,
                                 const char * allowedChars = 0)
        throw (std::string);

      /**
       * Escape-encodes a string.
       *
       * Replaces all characters in the string that are not unreserved
       * or otherwise allowed with their equivalent escape sequence.
       *
       * \param[in] str A string.
       * \param[in] allowedChars If not 0, a null-terminated sequence of
       * characters that need not be escaped, in addition to the default
       * set of unreserved characters for URIs.
       * \return The string with all escaped characters replaced with the
       * unescaped character.
       */

      static std::string encode (const std::string & str,
                                 const char * allowedChars = 0)
        throw (std::string);

      /**
       * Escape-encodes a string.
       *
       * Replaces all characters in the string that are not unreserved
       * or otherwise allowed with their equivalent escape sequence.
       *
       * \param[in] str A string.
       * \param[in] allowedChars A sequence of characters that need not
       * be escaped, in addition to the default set of unreserved
       * characters for URIs.
       * \return The string with all escaped characters replaced with the
       * unescaped character.
       */

      static std::string encode (const std::string & str,
                                 const std::string & allowedChars)
        throw (std::string);

      //@}

      /**
       * Test if one URIs is the prefix of another, allowing for different
       * escape-encodings (e.g. if one URI has '~' encoded as '%7f', and
       * the other one hasn't).
       *
       * \param[in] fullURI The URL to test.
       * \param[in] baseURI The presumed base URI.
       * \param[out] prefix If not 0, and if \a baseURI is a prefix of
       * \a fullURI, returns the part of \a fullURI that is equivalent
       * to \a baseURI.
       * \param[out] tail If not 0, and if \a baseURI is a prefix of
       * \a fullURI, returns the part of \a fullURI after the
       * \a baseURI.
       *
       * \return true If \a baseURI is a prefix to \a fullURI.
       */

      static bool isPrefix (const std::string & fullURI,
                            const std::string & baseURI,
                            std::string * prefix = 0,
                            std::string * tail = 0)
        throw ();

      /**
       * \name RFC 2396 Character set helpers.
       */

      //@{

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>alpha</em> character set.
       */

      static bool isalpha (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>digit</em> character set.
       */

      static bool isdigit (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>alphanum</em> character set.
       */

      static bool isalphanum (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>uric</em> character set.
       */

      static bool isuric (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>reserved</em> character set.
       */

      static bool isreserved (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>unreserved</em> character set.
       */

      static bool isunreserved (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>mark</em> character set.
       */

      static bool ismark (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>hex</em> character set.
       */

      static bool ishex (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>control</em> character set.
       */

      static bool iscontrol (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>space</em> character set.
       */

      static bool isspace (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>delim</em> character set.
       */

      static bool isdelim (char c) throw ();

      /**
       * \param[in] c A character.
       * \return True if \a c is part of the <em>unwise</em> character set.
       */

      static bool isunwise (char c) throw ();

      //@}

      /*
       * End of public interface
       */

    private:
      /** \cond */

      void parse (const std::string &)
        throw (std::string);

      void resolve (const Uri &)
        throw (std::string);

      static std::string normalizePath (const std::string &)
        throw ();

      /** \endcond */

    private:
      /** \cond */
      std::string uri;
      std::string scheme;
      std::string authority;
      std::string userinfo;
      std::string hostport;
      std::string host;
      std::string port;
      std::string path;
      std::string filename;
      std::string query;
      std::string request;
      std::string fragment;
      /** \endcond */
    };

  }
}

/*
 * Inline implementations of helpers
 */

inline bool
OCPI::Util::Uri::isalpha (char c)
  throw ()
{
  return (std::isalpha (c) ? true : false);
}

inline bool
OCPI::Util::Uri::isdigit (char c)
  throw ()
{
  return (std::isdigit (c) ? true : false);
}

inline bool
OCPI::Util::Uri::ishex (char c)
  throw ()
{
  return (std::isxdigit (c) ? true : false);
}

inline bool
OCPI::Util::Uri::isalphanum (char c)
  throw ()
{
  return (std::isalnum (c) ? true : false);
}

inline bool
OCPI::Util::Uri::isreserved (char c)
  throw ()
{
  switch (c) {
  case ';':
  case '/':
  case '?':
  case ':':
  case '@':
  case '&':
  case '=':
  case '+':
  case '$':
  case ',':
    return true;
  }

  return false;
}

inline bool
OCPI::Util::Uri::ismark (char c)
  throw ()
{
  switch (c) {
  case '-':
  case '_':
  case '.':
  case '!':
  case '~':
  case '*':
  case '\'':
  case '(':
  case ')':
    return true;
  }

  return false;
}

inline bool
OCPI::Util::Uri::isunreserved (char c)
  throw ()
{
  return isalphanum (c) || ismark (c);
}

inline bool
OCPI::Util::Uri::isuric (char c)
  throw ()
{
  return isreserved (c) || isunreserved (c) || (c == '%');
}

inline bool
OCPI::Util::Uri::iscontrol (char c)
  throw ()
{
  return (c <= 0x1f) || (c == 0x7f);
}

inline bool
OCPI::Util::Uri::isspace (char c)
  throw ()
{
  return (c == 0x20);
}

inline bool
OCPI::Util::Uri::isdelim (char c)
  throw ()
{
  switch (c) {
  case '<':
  case '>':
  case '#':
  case '%':
  case '"':
    return true;
  }

  return false;
}

inline bool
OCPI::Util::Uri::isunwise (char c)
  throw ()
{
  switch (c) {
  case '{':
  case '}':
  case '|':
  case '\\':
  case '[':
  case ']':
  case '`':
    return true;
  }

  return false;
}

#endif
