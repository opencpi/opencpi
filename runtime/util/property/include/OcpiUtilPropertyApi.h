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

/*
 Definitions exposed to the API for property metadata
*/

#ifndef OCPI_UTIL_PROPERTY_API_H
#define OCPI_UTIL_PROPERTY_API_H

#include "OcpiUtilDataTypesApi.h"

namespace OCPI {
  namespace API {
    // This structure holds attributes that are exposed to the ACI.
    // Normally that means what is somehow documented in any case.
    // It allows property introspection
    // It is not within the Property class (namespace) for reasons of circular dependencies
    struct PropertyAttributes {
      // How is it set from outside the worker?
      bool isParameter; // is a compiler/build-time constant, cannot be set at runtime, reads as constant
      bool isInitial;   // value may be set before start, in XML, but not at runtime, cached if not volatile
      bool isWritable;  // is dynamically writable at runtime, including after start, cached if not volatile
      // How is the value read?  If neither of these you can only read back what was set (above)
      bool isVolatile;  // worker may change value any time, reading is always uncached
      bool isConst;     // property is readable, stored in worker, and cachable.
      // Other
      bool isDebug;     // is only available if the worker was built with ocpi_debug = true
      bool isHidden;    // is not dumped by default
      bool isWorker;    // is a worker property that is not in its spec
      bool isBuiltin;   // property is part of infrastructure, not declared by spec or worker
      bool isPadding;   // is a worker property used only for padding property offsets
      bool isRaw;       // is a raw property where the worker itself is responsible for access+storage
      std::string name; // property name (copy)
      // Dynamic conditions of a particular access
      bool isCached;    // property value is currently cached in the ACI process
      bool isUnreadable;// never written, not readable, not volatile etc., or padding
      PropertyAttributes() :
	isParameter(false), isInitial(false), isWritable(false), isVolatile(false), isConst(false),
	isDebug(false), isHidden(false), isWorker(false), isBuiltin(false), isPadding(false),
	isRaw(false), isCached(false), isUnreadable(false) {
      }
    };
    enum PropertyOption {
      UNCACHED,      // force uncached access when possible
      HEX,           // format integers in hex
      APPEND,        // append value to output string, instead of setting it
      UNREADABLE_OK, // allow an unreadable property to return an empty string, and not throw
      NONE           // convenience for conditional elements
    };
    typedef const std::initializer_list<PropertyOption> PropertyOptionList;
    const PropertyOptionList noPropertyOptions; // because GCC 4.4 doesn't completely support init lists

#if 0
  struct ReadableSequenceProperty {
    size_t length() const;
    size_t maxLength() const;
  };
  struct ReadableArrayProperty {
    size_t length() const;
  };
  struct WritableSequenceProperty {
    size_t maxLength() const;
  };
  struct WritableArrayProperty {
    size_t length() const;
  };
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		        \
  struct Readable##pretty##Property {                                           \
    run get() const;		                                                \
  };                                                                            \
  struct Readable##pretty##SequenceProperty : public ReadableSequenceProperty {	\
    size_t get(run *, size_t length) const;                                     \
  };                                                                            \
  struct Readable##pretty##ArrayProperty : public ReadableArrayProperty {	\
    unsigned get(run *, size_t length) const;                                   \
  };
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)	        \
  struct Readable##pretty##Property {                                           \
    char *get(char *, size_t length) const;	                                \
  };                                                                            \
  struct Readable##pretty##SequenceProperty : public ReadableSequenceProperty { \
    unsigned get(char **, size_t length, char *buf, size_t space) const;        \
    size_t stringLength() const;                                                \
    size_t space() const;                                                       \
  };                                                                            \
  struct Readable##pretty##ArrayProperty : public ReadableArrayProperty {       \
    unsigned get(char **, size_t length, char *buf, size_t space) const;        \
    size_t stringLength() const;                                                \
    size_t space() const { return length() * (stringLength() + 1); };	        \
  };

OCPI_PROPERTY_DATA_TYPES

#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#if 0 // avoid SWIG bugs
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#else
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)
#endif
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		                   \
  struct Writable##pretty##Property {                                                      \
    void set(const run);                                                                   \
  };                                                                                       \
  struct Writable##pretty##SequenceProperty : public WritableSequenceProperty {            \
    void set(const run *, size_t nElements);                                               \
  };                                                                                       \
  struct Writable##pretty##ArrayProperty : public WritableArrayProperty {                  \
    void set(const run *, size_t nElements);                                               \
  };                                                                                       \
  struct ReadWrite##pretty##Property                                                       \
  : public Readable##pretty##Property, public Writable##pretty##Property {                 \
  };                                                                                       \
  struct ReadWrite##pretty##SequenceProperty                                               \
  : public Readable##pretty##SequenceProperty, public Writable##pretty##SequenceProperty { \
  };                                                                                       \
  struct ReadWrite##pretty##ArrayProperty                                                  \
  : public Readable##pretty##ArrayProperty, public Writable##pretty##ArrayProperty {       \
  };

OCPI_PROPERTY_DATA_TYPES

#undef OCPI_DATA_TYPE
#endif
  }
}
#endif
