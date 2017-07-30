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

#ifndef OCPIUTILHTTPMISC_H__
#define OCPIUTILHTTPMISC_H__

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

#include <string>
#include <map>
#include <ctime>
#include "OcpiUtilMisc.h"

namespace OCPI {
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
        OCPI::Util::CaseInsensitiveStringLess> Headers;

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
