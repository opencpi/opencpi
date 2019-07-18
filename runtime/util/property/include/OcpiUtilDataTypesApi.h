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

// This file defines runtime data types for properties, property value lists,
// parsing, printing etc.



#ifndef OCPI_UTIL_DATA_TYPES_API_H
#define OCPI_UTIL_DATA_TYPES_API_H
#include <stdint.h>
#include <cassert>
#include <limits>
/*
  These are the "simple" scalar property data types we support
  The list is built from several sources, the SCA, and CORBA IDL,
  although it does not precisely depend on either one.

  The different macro names mean:
  OCPI_DATA_TYPE_H - very CORBA-specific, the "H" indicates that a CORBA "helper"
                     is required in "type any" processing
  OCPI_DATA_TYPE_S - the special case of "string"
  OCPI_DATA_TYPE_X - names that are not in SCA
  The different arguments to the macro are:
  1. SCA simple name (from the property spec), or made up similar name for OCPI_DATA_TYPE_X
  2. CORBA C++ name from C++ language mapping
  3. the prefix of the fixed size type: u for integers, f floats, x string
  4. the size in bits
  5. the runtime C++ type we have chosen, consistent with CDR
  6. the uppercased typename used for OCPI user APIs specific to types
  7. the mapped, sized, C++ (including stdint.h) C++ typename

  It is unfortunate that "string" falls into this "basic" category, but
  CORBA and SCA are both strange about it so we are too.

  CORBA "char" can map to "char" or "unsigned char" in different ORBs.
  Blind casts/conversions are done from the corba types to the c++ types
*/

#define OCPI_PROPERTY_DATA_TYPES OCPI_DATA_TYPES
#define OCPI_DATA_TYPES \
  /*                 sca        CORBA/IDL  ? bits c++/run   Pretty     Storage */\
    OCPI_DATA_TYPE_H(boolean,   Boolean,   u,  8, bool,     Bool,      uint8_t)  \
    OCPI_DATA_TYPE_H(char,      Char,      u,  8, char,     Char,      uint8_t)  \
    OCPI_DATA_TYPE(  double,    Double,    f, 64, double,   Double,    uint64_t) \
    OCPI_DATA_TYPE(  float,     Float,     f, 32, float,    Float,     uint32_t) \
    OCPI_DATA_TYPE(  short,     Short,     u, 16, int16_t,  Short,     uint16_t) \
    OCPI_DATA_TYPE(  long,      Long,      u, 32, int32_t,  Long,      uint32_t) \
    OCPI_DATA_TYPE_H(octet,     Octet,     u,  8, uint8_t,  UChar,     uint8_t)  \
    OCPI_DATA_TYPE(  ulong,     ULong,     u, 32, uint32_t, ULong,     uint32_t) \
    OCPI_DATA_TYPE(  ushort,    UShort,    u, 16, uint16_t, UShort,    uint16_t) \
    OCPI_DATA_TYPE_X(longlong,  LongLong,  u, 64, int64_t,  LongLong,  uint64_t) \
    OCPI_DATA_TYPE_X(ulonglong, ULongLong, u, 64, uint64_t, ULongLong, uint64_t) \
    OCPI_DATA_TYPE_S(string,    String,    @, 32, OCPI::API::CharP,    String,    %^&) \
    /**/
// NOTE above that strings are aligned at 32 bits

#if 0 // These don't work with SWIG because SWIG has bugs in its ability to do C/C++ preprocessing
#define OCPI_DATA_TYPE_H OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_X OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#else
#define OCPI_DATA_TYPE_H(sca,corba,letter,bits,run,pretty,store) OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE_X(sca,corba,letter,bits,run,pretty,store) OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)
#endif
namespace OCPI {
  namespace API {
    // Enumerated type for base builtin types
    typedef const char *CharP;
    enum BaseType {
      OCPI_none, // 0 isn't a valid type
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) OCPI_##pretty,
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      OCPI_Struct,  // type that is a struct
      OCPI_Enum,    // type that is an enum
      OCPI_Type,    // recursive type that is not a struct
      OCPI_scalar_type_limit
    };
// Missing types that ARE in minimum IDL: longdouble, wchar, wstring, fixed, union, any

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) typedef run pretty;
    OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
  const run pretty##_MAX = std::numeric_limits<run>::max(); \
  const run pretty##_MIN = std::numeric_limits<run>::min();
    OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#if 0 // avoid SWIG bugs
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#else
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)
#endif
    // Structure to capture indexing arrays and sequences and navigating to struct members
    // The only intended usage is in the initializer_list below
    struct Access {
      union {
	size_t m_index;
	const char *m_member;
      };
      bool m_number;
      Access(size_t subscript)   : m_index(subscript), m_number(true) {} // get element
      // Allow (signed) ints for convenience, including 0, which should not end up being NULL for const char*
      Access(int subscript)      : m_index((assert(subscript >= 0), (size_t)subscript)), m_number(true) {}
      Access(const char *member) : m_member(member), m_number(false) {}; // get member
    };
    typedef const std::initializer_list<Access> AccessList;
    const AccessList emptyList; // because GCC 4.4 doesn't completely support init lists
  } // API
} // OCPI
#endif
