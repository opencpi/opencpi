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
  }
}
#endif
