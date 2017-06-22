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

#include <stdint.h>
#include <iostream>
#include <cstring>
#include <vector>
#include "OcpiUtilVfs.h"
#include "OcpiExprEvaluator.h"
#include "ezxml.h"

namespace OCPI {
  namespace Util {
    struct IdentResolver;
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
         * Parse XML from a modifiable character array this will be owned
         * by this object (ownership transferred in).
         * \param[in] data XML data in a char array with transferred ownership.
         * \return The document root element.
         * \throw std::string If \a data can not be parsed as an XML
         * document.
         *
         * \pre This instance is unused.
         * \post This instance is in use.
         */

        ezxml_t parse (char *data)
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

      extern ezxml_t
	ezxml_firstChild(ezxml_t xml),
	ezxml_nextChild(ezxml_t xml),
        addChild(ezxml_t x, const char *name, unsigned level, const char *txt = NULL,
		 const char *attr1 = NULL, const char *value1 = NULL,
		 const char *attr2 = NULL, const char *value2 = NULL),
	findChildWithAttr(ezxml_t x, const char *cName, const char *aName,
			  const char *value);
      inline char *ezxml_content(ezxml_t x) { return x->txt; }
      extern const char
	*ezxml_parse_file(const char *file, ezxml_t &xml),
	*ezxml_parse_str(char *string, size_t len, ezxml_t &xml),
	*ezxml_tag(ezxml_t xml),
	*checkTag(ezxml_t xml, const char *tag, const char *fmt, ...)
	__attribute__((format(printf, 3, 4))),
	*getRequiredString(ezxml_t x, std::string &s, const char *attr, const char *elem = NULL),
	*ezxml_children(ezxml_t xml, const char* (*func)(ezxml_t child, void *arg), 
			void *arg = NULL),
	*ezxml_children(ezxml_t xml, const char *tag, 
			const char *(*func)(ezxml_t child, void *arg), void *arg = NULL),
	*ezxml_attrs(ezxml_t xml,
		     const char *(*func)(const char *name, const char *value, void *arg),
		     void *arg),
	// true only means its an error to do anything but true, for cases
	// when you are only allowed to "add truth", not set false
        *getBoolean(ezxml_t x, const char *name, bool *b, bool trueOnly = false, 
		    bool *found = NULL),
        *checkAttrs(ezxml_t x, ...),
        *checkElements(ezxml_t x, ...),
	*checkAttrsV(ezxml_t x, const char **attrs),
	*checkAttrsVV(ezxml_t x, ...),
        *checkElements(ezxml_t x, ...),
	*getEnum(ezxml_t x, const char *attr, const char **enums, const char *type, size_t &n,
		 size_t def = 0, bool required = false),
        *getNumber(ezxml_t x, const char *attr, size_t *np,
		   bool *found = NULL, size_t defaultValue = 0,
		   bool setDefault = true, bool required = false),
        *getNumber8(ezxml_t x, const char *attr, uint8_t *np,
		    bool *found = NULL, uint32_t defaultValue = 0,
		    bool setDefault = true),
        *getNumber64(ezxml_t x, const char *attr, uint64_t *np,
		     bool *found = NULL, uint64_t defaultValue = 0,
		     bool setDefault = true, bool required = false),
	*getExprNumber(ezxml_t x, const char *attr, size_t &np, bool *found, std::string &expr,
		       const IdentResolver *resolver);
      extern unsigned
	countChildren(ezxml_t x, const char*cName),
	countAttributes(ezxml_t x);
      extern bool
	receiveXml(int fd, ezxml_t &rx, std::vector<char> &buf, bool &eof, std::string &error),
	sendXml(int fd, std::string &buf, const char *msg, std::string &error),
	inList(const char *item, const char *list),
	hasAttrEq(ezxml_t x, const char *attrName, const char *val),
	// FIXME: move to util:misc if they are not about xml
        getUNum(const char *s, size_t *valp),
        getUNum64(const char *s, const char *end, uint64_t &valp, const char **endp = NULL),
        getNum64(const char *s, const char *end, int64_t &valp, unsigned bits = 0),
        parseBool(const char *a, const char *end, bool *b),
	getOptionalString(ezxml_t x, std::string &s, const char *attr,
			  const char *def = "");
      extern void
	unindent(std::string &in), // strip common indent of lines in text element
	getNameWithDefault(ezxml_t x, std::string &s, const char *fmt, unsigned &ord);
    }
#define EZXML_FOR_ALL_ATTRIBUTES(x, n, v) \
    for (char **_ap = x ? x->attr : 0; _ap && *_ap ? (n = _ap[0], v = _ap[1], 1) : 0; _ap += 2)

  }
}

#endif
