// -*- c++ -*-

#ifndef CPIUTILHTTPMISC_H__
#define CPIUTILHTTPMISC_H__

/**
 * \file
 * \brief Some helpers related to HTTP.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiUtilMisc.h>
#include <string>
#include <map>
#include <ctime>

namespace CPI {
  namespace Util {
    /**
     * \brief HTTP client and server functionality.
     */

    namespace Http {

      /**
       * \brief Data type to hold a set of HTTP headers.
       *
       * Maps a header field name to its value.  According to the HTTP
       * specification, header fields are case insensitive, so we use a
       * case insensitive comparator.
       */

      typedef std::map<std::string, std::string,
	CPI::Util::Misc::CaseInsensitiveStringLess> Headers;

      /**
       * Check whether a string is a valid HTTP token.
       *
       * \param[in] str A string.
       * \return   true if \a str is a valid HTTP token, matching
       *           the production rule for <em>token</em> in the
       *           HTTP Basic Rules section.
       *           Returns false otherwise.
       */

      bool isToken (const std::string & str)
	throw ();

      /**
       * Parse a HTTP date/time stamp.
       *
       * \param[in] str A string.
       * \return The timestamp extracted from the string.
       *
       * \throw std::string If \a str does not match the production
       * rule for <em>HTTP-date</em>.
       */

      std::time_t parseHttpDate (const std::string & str)
	throw (std::string);

      /**
       * Create a date/time stamp string suitable for use in the HTTP
       * protocol.
       *
       * \param[in] ts  A timestamp.
       * \return A <em>HTTP-date</em> encoded string.
       */

      std::string makeHttpDate (std::time_t ts)
	throw (std::string);

      /*
       * Create a date/time stamp string, representing the current time,
       * suitable for use in the HTTP protocol.
       *
       * \return A <em>HTTP-date</em> encoded string.
       */

      std::string getHttpTimestamp ()
	throw ();

    }
  }
}

#endif
