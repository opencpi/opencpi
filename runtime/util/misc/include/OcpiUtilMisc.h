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

#ifndef OCPIUTILMISC_H__
#define OCPIUTILMISC_H__

/**
 * \file
 * \brief Miscellaneous utilities.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <strings.h>
#include <assert.h>
#include <inttypes.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <istream>
#include <functional>
#include <vector>

#include "OcpiOsAssert.h"
#include "OcpiOsEther.h"

#ifndef NDEBUG
  class Trace {
  public:
    std::string ts;
    Trace(const char* e) 
      : ts(e)
    {
      ocpiDebug("Entering (%s)\n", ts.c_str() );
    }

    ~Trace()
    {
      ocpiDebug("Leaving (%s)\n", ts.c_str() );    
    }
  };
#define TRACE(s) Trace t(s)
#else
#define TRACE(s)
#endif

// OCPI_OFFSETOF is here because offsetof doesn't work on members that are structures
#if defined(NDEBUG)
#define OCPI_UTRUNCATE(type, val) ((type)(val))
#define OCPI_STRUNCATE(type, val) ((type)(val))
#define OCPI_SIZEOF(utype, stype) ((utype)sizeof(stype))
#define OCPI_OFFSETOF(utype, member, stype) ((utype)&((stype *)0)->member)
#define OCPI_SIZE_T_DIFF(larger, smaller) ((size_t)((larger) - (smaller)))
#else
#define OCPI_UTRUNCATE(type, val) \
  (static_cast<type>(OCPI::Util::utruncate((uint64_t)(val), sizeof(type))))
#define OCPI_STRUNCATE(type, val) \
  (static_cast<type>(OCPI::Util::struncate((int64_t)(val), sizeof(type))))
#define OCPI_SIZEOF(utype, stype) \
  ((utype)OCPI::Util::utruncate((uint64_t)sizeof(stype), sizeof(utype)))
#define OCPI_OFFSETOF(utype, stype, member)				\
  ((utype)OCPI::Util::utruncate((uint64_t)&((stype *)0)->member, sizeof(utype)))
#define OCPI_SIZE_T_DIFF(larger, smaller) \
  OCPI::Util::size_t_diff((larger) - (smaller))
#endif

namespace OCPI {
  namespace Util {

    /**
     * \brief Miscellaneous utilities.
     */
    inline uint64_t utruncate(uint64_t val, size_t bytes) {
      ocpiAssert(bytes == sizeof(val) ||
		 val <= ~((uint64_t)-1 << (bytes*8)));
      return val;
    }
    inline int64_t struncate(int64_t val, size_t bytes) {
      ocpiAssert(bytes == sizeof(val) ||
		 (val < 0 ?
		  val >= (int64_t)-1 << (bytes*8-1) :
		  val <= ~((int64_t)-1 << (bytes*8-1))));
      return val;
    }
    inline size_t size_t_diff(ssize_t diff) {
      ocpiAssert(diff >= 0);
      return (size_t)(diff);
    }

    int64_t struncate(int64_t val, unsigned bytes);
      /**
       * \brief Convert an integer to a string.
       *
       * \param[in] value An integer value.
       * \return  A string containing the decimal representation of the value.
       */

      std::string integerToString (int value);

      /**
       * \brief Convert an unsigned integer to a string.
       *
       * \param[in] value An unsigned integer value.
       * \param[in] base  The base for representing \a value.  Must be between
       *                  2 and 16, inclusive. E.g., 10 is for decimal, 16 for
       *                  hexadecimal.
       * \param[in] mindigits The minimum of digits that the string
       *                  representation shall have.  If \a value can be
       *                  represented with fewer digits, it is left-padded
       *                  using the \a pad character.
       * \param[in] pad   The character to pad the string with, if necessary.
       * \return          A string representation of \a value, using base
       *                  \a base, having a length of at least \a mindigits.
       */

      std::string unsignedToString (unsigned int value,
                                    unsigned int base = 10,
                                    unsigned int mindigits = 0,
                                    char pad = '0');

      /**
       * \brief Convert an unsigned long long integer to a string.
       *
       * \param[in] value An unsigned integer value.
       * \param[in] base  The base for representing \a value.  Must be between
       *                  2 and 16, inclusive. E.g., 10 is for decimal, 16 for
       *                  hexadecimal.
       * \param[in] mindigits The minimum of digits that the string
       *                  representation shall have.  If \a value can be
       *                  represented with fewer digits, it is left-padded
       *                  using the \a pad character.
       * \param[in] pad   The character to pad the string with, if necessary.
       * \return          A string representation of \a value, using base
       *                  \a base, having a length of at least \a mindigits.
       */

      std::string unsignedToString (unsigned long long value,
                                    unsigned int base = 10,
                                    unsigned int mindigits = 0,
                                    char pad = '0');

      /**
       * \brief Convert a floating-point value to a string.
       *
       * \param[in] value A floating-point value.
       * \return          A decimal representation of the value.
       */

      std::string doubleToString (double value);

      /**
       * \brief Convert a string to an integer.
       *
       * \param[in] value A string containing a decimal representation of
       *                  an integer number.
       * \return          The value.
       *
       * \throw std::string If the value can not be interpreted as a
       * number.
       */

      int stringToInteger (const std::string & value);

      /**
       * \brief Convert a string to an unsigned integer.
       *
       * \param[in] value A string containing a decimal representation
       *                  of an unsigned integer number.
       * \param[in] base  The base.  Must be between 2 and 16, inclusive.
       * \return          The value.
       *
       * \throw std::string If the value can not be interpreted as a
       * number.
       */

      unsigned int stringToUnsigned (const std::string & value,
                                     unsigned int base = 10);

      /**
       * \brief Convert a string to an unsigned integer.
       *
       * \param[in] value A string containing a decimal representation
       *                  of an unsigned integer number.
       * \param[in] base  The base.  Must be between 2 and 16, inclusive.
       * \return          The value.
       *
       * \throw std::string If the value can not be interpreted as a
       * number.
       */
#if 0 // DISABLED

      unsigned long long stringToULongLong (const std::string & value,
                                            unsigned int base = 10);

      /**
       * \brief Convert an unsigned long long position to a <em>std::streamsize</em> value.
       *
       * <em>std::streamsize</em> is an implementation-defined,
       * signed type that may be of lesser width than unsigned long long.
       * This function provides control over the return value if the
       * unsigned long long position value exceeds the value range of
       * <em>std::streamsize</em>.
       *
       * \param[in] pos      The position to convert.
       * \param[in] minusone What to do if the \a pos value exceeds the value
       *                range of <em>std::streamsize</em>.  If true, then -1
       *                will be returned as an error indicator.  If false,
       *                then the maximum positive value for
       *                <em>std::streamsize</em> is returned.
       * \return        An <em>std::streamsize</em> value with the same
       *                value as \a pos, if possible, or -1 or the maximum
       *                positive value for <em>std::streamsize</em>,
       *                as requested using the \a minusone parameter.
       */
#endif
      std::streamsize unsignedToStreamsize (unsigned long long pos,
                                            bool minusone = true);

      /**
       * \brief Read a line from an input stream to the next LF.
       *
       * Read a line from an input stream, up to and including the next
       * <tt>LF</tt> character.  Trailing whitespace characters (including
       * the <tt>LF</tt> and any <tt>CR</tt>, tab and space characters) are
       * discarded.
       *
       * Up to \a maxSize characters are read.  If no end-of-line character
       * is found after reading \a maxSize characters, the \a failbit is set
       * on \a conn, and the data that was read up to that point is returned.
       *
       * \param[in] conn    The input stream from which to read.
       * \param[in] maxSize The maximum amount of characters to read, or
       *                    -1 for no maximum.
       * \return            The data that was read.
       */

      std::string readline (std::istream * conn, unsigned int maxSize = static_cast<unsigned int> (-1));

      /**
       * \brief Case insensitive string comparison.
       *
       * \param[in] s1  The first string.
       * \param[in] s2  The second string.
       * \return   The result of a case-insensitive comparison of \a s1 and
       *           \a s2.  A negative value if \a s1 is lexicographically less
       *           than \a s2.  A positive value if \a s1 is lexicographically
       *           greater than \a s2.  Zero if the strings are the same,
       *           excluding case.
       */

      int caseInsensitiveStringCompare (const std::string & s1,
                                        const std::string & s2);

      /**
       * \brief Predicate to compare two strings, ignoring case.
       *
       * This class can be used, e.g., wherever the STL expects a
       * comparator class, as in the <em>std::set</em> and <em>std::map</em>
       * templates.  E.g.,
       *
       * \code
       *   typedef std::map<std::string, std::string,
       *                    CaseInsensitiveStringLess>
       *     CaseInsensitiveMap;
       * \endcode
       */

      class CaseInsensitiveStringLess : public std::binary_function<std::string, std::string, bool>
      {
      public:
        bool operator() (const std::string & s1, const std::string & s2) const;
      };

      /**
       * \brief Test if a string matches a glob-style pattern.
       *
       * \param[in] str     A string.
       * \param[in] pattern A glob-style pattern.
       * \return            true if \a str matches \a pattern,
       *                    false if not.
       */

      bool glob (const std::string & str, const std::string & pattern);

      /**
       * \brief Determine if a file is an XML document.
       *
       * This operation reads data from the stream.  After reaching a
       * conclusion, the stream pointer is rewound to the beginning.
       * If resetting the read pointer fails, e.g., if the stream is
       * not seekable, the caller may have to close and re-open the
       * file before processing it.
       *
       * \param[in] str A stream.
       * \return   true if the data in the stream looks like an XML
       *           document, false if not.
       *
       * \pre The read pointer shall be at position zero.
       * \post The read pointer is at position zero, or the failbit is
       * set on \a str (because repositioning the read pointer failed).
       */

      bool isXMLDocument (std::istream * str);

      /**
       * \brief do vasprintf into a std::string
       *
       * \param[inout] out is referenced std::string where the output goes
       *
       */

      void
	encodeXmlAttrSingle(const std::string &s, std::string &out, bool raw = false),
	encodeDescriptor(const char *iname, const std::string &s, std::string &out),
	decodeDescriptor(const char *info, std::string &s),
	formatString(std::string &out, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
      // return value is simply a convenience of out.c_str()
      const char
	*format(std::string &out, const char *fmt, ...) __attribute__((format(printf, 2, 3))),
	*formatAdd(std::string &out, const char *fmt, ...) __attribute__((format(printf, 2, 3))),
	*formatAddV(std::string &out, const char *fmt, va_list ap);
      bool
	// This one just returns true as a convenience for the error handling protocol
	// that sets an error string and returns true if an error occurred.
	eformat(std::string &out, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
      // Return an error string (caller can throw if desired)
      const char
	*parseList(const char *list, const char * (*doit)(const char *tok, void *arg),
		   void *arg = NULL),
	*file2String(std::string &out, const char *file, char replaceNewLine = 0),
	*file2String(std::string &out, const char *file, const char *start, const char *middle, 
		     const char *end),
	*string2File(const std::string &in, const char *file, bool leaveExisting = false,
		     bool onlyIfDifferent = false, bool makeExecutable = false),
	*evsprintf(const char *fmt, va_list ap),
	*esprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
      void
	ewprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2))); // for warnings
      inline const char *file2String(std::string &out, const std::string &file,
				     char replaceNewLine = 0) {
	return file2String(out, file.c_str(), replaceNewLine);
      }
      inline const char *string2File(const std::string &in, const std::string &file,
				     bool leaveExisting = false, bool onlyIfDifferent = false) {
	return string2File(in, file.c_str(), leaveExisting, onlyIfDifferent);
      }

      // Simple wrapper for strsep, allowing empty tokens if desired.
      // When empty tokens not allowed, consecutive delimiters are simply consumed
      // Usage is:
      //    for (OU::TokenIter li(input); li.token(); li.next()) { use li.token(); }
      class TokenIter {
	char *m_copy, *m_ptr;
	const char *m_token, *m_delims;
	bool m_allowEmpty;
      public:
        TokenIter(const char *list, const char *delims = ", \t\n", bool allowEmpty = false);
        TokenIter(const std::string &list, const char *delims = ", \t\n", bool allowEmpty = false);
	~TokenIter();
	inline const char *token() const { return m_token; }
	void next();
      };

      // These are comparison object for use in STL classes
      struct ConstCharComp {
	inline bool operator() (const char *lhs, const char *rhs) const {
	  return strcmp(lhs, rhs) < 0;
	}
      };
      struct ConstCharCaseComp {
	inline bool operator() (const char *lhs, const char *rhs) const {
	  return strcasecmp(lhs, rhs) < 0;
	}
      };
      struct ConstStringComp {
        inline bool operator() (const std::string &lhs, const std::string &rhs) const {
          return strcmp(lhs.c_str(), rhs.c_str()) < 0;
        }
      };
      struct ConstStringCaseComp {
        inline bool operator() (const std::string &lhs, const std::string &rhs) const {
          return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
        }
      };
      /* DO NOT USE THESE - they are invalid in containers like "map" which
       * want a "less" compare and assume equal when !(a<b) and !(b<a).
       * See also: http://www.cplusplus.com/reference/map/map/ (Compare)
      struct ConstCharEqual {
	inline bool operator() (const char *lhs, const char *rhs) const {
	  return strcmp(lhs, rhs) == 0;
	}
      };
      struct ConstCharCaseEqual {
	inline bool operator() (const char *lhs, const char *rhs) const {
	  return strcasecmp(lhs, rhs) == 0;
	}
      };
      */
      /* These need to be replaced if needed because they are poorly implemented ("abc" == "cba"):
      struct ConstCharHash {
	inline size_t operator() (const char *s) const {
	  size_t h = 0;
	  while (*s) h += (unsigned char)*s++;
	  return h;
	}
      };
      struct ConstCharCaseHash {
	inline size_t operator() (const char *s) const {
	  size_t h = 0;
	  while (*s) h += (unsigned char)tolower(*s++);
	  return h;
	}
      };
      */
      inline size_t roundUp (size_t value, size_t align) {
	return ((value + (align - 1)) / align) * align;
      }
      inline uint32_t swap32(uint32_t value) {
	return
	  (((value) & 0xff) << 24) | (((value) & 0xff00) << 8) |
	  (((value) & 0xff0000) >> 8) | (((value) >> 24));
      }
      unsigned fls64(uint64_t n);
      inline size_t ceilLog2(uint64_t n) {
	return OCPI_UTRUNCATE(size_t, n ? fls64(n - 1) : 0);
      }
      inline size_t floorLog2(uint64_t n) {
	//  ocpiInfo("Floor log2 of %u is %u", n, myfls(n)-1);
	return OCPI_UTRUNCATE(size_t, fls64(n) - 1);
      }
      inline size_t bitsForMax(uint64_t n) {
	return ceilLog2(n + 1);
      }
      OCPI::OS::Ether::Address &getSystemAddr();
      const std::string &getSystemId();
      // The posix.2 definition, meaning initial removal of trailing slashes.
      // empty strings and "/" result in an empty string.
      // The return value is a convenience - the c_str() of the output buffer.
      const char *baseName(const char *path, std::string &buf);
      // Do glob processing where the expectation is that any pattern results in a single
      // unique name.  Include tilde processing.  Return true on error
      bool globPath(const char *in, std::string &out);
      // find item in colon-separated path, returning complete path in "result"
      // return true on error
      bool searchPath(const char *path, const char *item, std::string &result,
		      const char *preferredSuffix = NULL, std::vector<std::string> *all = NULL);
      // set path to be the current project registry.  Return an error string or NULL.
      const char *getProjectRegistry(std::string &path);
      // return, via "path", a ":" separated list of all projects registered
      // or present in the project path.
      // return error-message string on error
      const char *getAllProjects(std::string &path);
      // Return the OpenCPI installation dir or throw;
      const std::string &getOpenCPI();
      // Return the CDK's dir or throw;
      const std::string &getCDK();
      // A convenience template for singletons possibly created at static construction
      // time (moved from OcpiDriverManager)
      template <class S> class Singleton {
        static S *getSingletonPtr(bool test) {
          static S *theSingleton;
	  if (!test) {
	    // FIXME: put this static mutex into OCPI:OS somehow
	    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	    // This is hyper-conservative since static constructors are run in a single thread.
	    // But C++ doesn't actually say that...
	    ocpiCheck(pthread_mutex_lock(&mutex) == 0);
	    if (!theSingleton)
	      theSingleton = new S;
	    ocpiCheck(pthread_mutex_unlock(&mutex) == 0);
	  }
          return theSingleton;
        }
      protected:
        Singleton() {};                   // Empty constructor
      private:
        // [Virtual] destructor to clean up memory?
        Singleton(Singleton const&);      // Don't implement
        void operator=(Singleton const&); // Don't implement
      public:
	static S &getSingleton() {
	  return *getSingletonPtr(false);
	}
	static S *singleton() {
	  return getSingletonPtr(true);
	}
      };
  }
}

#endif
