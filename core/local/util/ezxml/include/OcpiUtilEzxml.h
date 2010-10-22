
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

// -*- c++ -*-

#ifndef OCPI_UTIL_EZXML_H__
#define OCPI_UTIL_EZXML_H__

/**
 * \file
 * \brief Thin C++ wrapper for ezxml to ease I/O and memory management.
 *
 * Revision History:
 *
 *     09/29/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>
#include <OcpiUtilVfs.h>
#include "ezxml.h"

namespace OCPI {
  namespace Util {

    /**
     * \brief Defines the OCPI::Util::EzXml::Doc class.
     */

    namespace EzXml {

      /**
       * \brief Convenience wrapper class for the ezxml library.
       *
       * It supports parsing XML files from various sources and takes
       * care of memory management (i.e., releasing the ezxml-allocated
       * memory in its destructor).
       */

      class Doc {
      public:
        /**
         * Default constructor.
         *
         * \post This instance is unused.
         */

        Doc ()
          throw ();

	// reduce warnings about pointer members
	Doc (const Doc &)
	  throw ();

        /**
         * Constructor to parse XML from a string.
         * Calls parse(\a data).
         *
         * After construction, call getRootNode() to get access to
         * the document's root element.
         *
         * \param[in] data XML data.
         * \throw std::string If \a data can not be parsed as an XML
         * document.
         *
         * \post This instance is in use.
         */

        Doc (const std::string & data)
          throw (std::string);

        /**
         * Constructor to parse XML from an input stream.
         * Calls parse(\a in).
         *
         * After construction, call getRootNode() to get access to
         * the document's root element.
         *
         * \param[in] in An input stream containing XML data starting at
         * the current position.
         * \throw std::string If an I/O error occurs, or if the data that
         * is read from the stream can not be parsed as an XML document.
         *
         * \post This instance is in use.
         */

        Doc (std::istream * in)
          throw (std::string);

        /**
         * Constructor to parse XML from a file.
         * Calls parse(\a fs, \a fileName).
         *
         * After construction, call getRootNode() to get access to
         * the document's root element.
         *
         * \param[in] fs The file system that contains the XML file.
         * \param[in] fileName The name of the XML file within that file
         * system.
         * \throw std::string If an I/O error occurs, or if the data that
         * is read from the file can not be parsed as an XML document.
         *
         * \post This instance is in use.
         */

        Doc (OCPI::Util::Vfs::Vfs & fs, const std::string & fileName)
          throw (std::string);

        /**
         * Destructor.  Releases the parsed document.
         */

        ~Doc ()
          throw ();

        /**
         * Parse XML from a string.
         *
         * \param[in] data XML data.
         * \return The document root element.
         * \throw std::string If \a data can not be parsed as an XML
         * document.
         *
         * \pre This instance is unused.
         * \post This instance is in use.
         */

        ezxml_t parse (const std::string & data)
          throw (std::string);

        /**
         * Parse XML from an input stream.
         *
         * \param[in] in An input stream containing XML data starting at
         * the current position.
         * \return The document root element.
         * \throw std::string If an I/O error occurs, or if the data that
         * is read from the stream can not be parsed as an XML document.
         *
         * \pre This instance is unused.
         * \post This instance is in use.
         */

        ezxml_t parse (std::istream * in)
          throw (std::string);

        /**
         * Parse XML from a file.
         *
         * \param[in] fs The file system that contains the XML file.
         * \param[in] fileName The name of the XML file within that file
         * system.
         * \return The document root element.
         * \throw std::string If an I/O error occurs, or if the data that
         * is read from the file can not be parsed as an XML document.
         *
         * \pre This instance is unused.
         * \post This instance is in use.
         */

        ezxml_t parse (OCPI::Util::Vfs::Vfs & fs, const std::string & fileName)
          throw (std::string);

        /**
         * Get the document root element.
         *
         * \return The document root element.
         *
         * \pre This instance is in use.
         */

        ezxml_t getRootNode ()
          throw ();

      private:
        char * m_doc;
        ezxml_t m_rootNode;
      };

      extern const char
        *getBoolean(ezxml_t x, const char *name, bool *b),
        *checkAttrs(ezxml_t x, ...),
        *getNumber(ezxml_t x, const char *attr, uint32_t *np, bool *found,
		   uint32_t defaultValue, bool setDefault = true),
        *getNumber64(ezxml_t x, const char *attr, uint64_t *np, bool *found,
		     uint64_t defaultValue, bool setDefault = true);
      extern bool
        getUNum(const char *s, uint32_t *valp),
        getUNum64(const char *s, uint64_t *valp),
        parseBool(const char *a, unsigned length, bool *b);
    }

  }
}
// Move this somewhere sensible and integrate with CC::ApiError etc. FIXME
extern const char *esprintf(const char *fmt, ...);

#endif
