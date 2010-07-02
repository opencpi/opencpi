// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*

 Definitions for worker metadata encoding,decoding.
 Offline, in tools, this information is encoded into a string format 
 suitable for program level arguments (argv).  All properties are encoded into
 a single string.  This relieves runtime of any parsing overhead (time or space)
 or dependencies on external parsing libraries.

 The "scope" of this property support is configuration properties for CP289
 components.  Thus it is not (yet) intended to support SCA GPP components.

 This file defines the binary (non-string) format of SCA component properties,
 as well as the functions to encode (binary to string) and decode 
 (string to binary).

 
*/
#ifndef CPI_METADATA_PROPERTY_H
#define CPI_METADATA_PROPERTY_H

#include "ezxml.h"
/*
  These are the "simple" property data types
  The list is built from two sources, the SCA, and CORBA IDL,
  although it does not precisely depend on either one.

  The different macro names mean:
  CPI_DATA_TYPE_H - // very CORBA-specific, the "H" indicates that a CORBA "helper" is required in "type any" processing
  CPI_DATA_TYPE_S - // the special case of "string"
  CPI_DATA_TYPE_X - // names that are not in SCA
  The different arguments to the macro are:
  1. SCA simple name, or made up similar name for CPI_DATA_TYPE_X
  2. CORBA C++ name from C++ language mapping
  3. the prefix of the fixed size type: u for integers, f floats, x string
  4. the size in bits
  5. the runtime C++ type
  6. the uppercased typename used for CPI user APIs specific to types
  7. the mapped, sized, C++ (including stdint.h) C++ typename
  
  It is unfortunate that "string" falls into this "basic" category, but
  CORBA and SCA are both strange about it so we are too.

  CORBA "char" can map to "char" or "unsigned char" in different ORBs.
  Blind casts/conversions are done from the corba types to the c++ types
*/

#define CPI_PROPERTY_DATA_TYPES \
  /*                sca        CORBA      ? bits c++/run   Pretty     Storage */\
    CPI_DATA_TYPE_H(boolean,   Boolean,   u,  8, bool,     Bool,      uint8_t)  \
    CPI_DATA_TYPE_H(char,      Char,      u,  8, char,     Char,      uint8_t)  \
    CPI_DATA_TYPE(  double,    Double,    f, 64, double,   Double,    uint64_t) \
    CPI_DATA_TYPE(  float,     Float,     f, 32, float,    Float,     uint32_t) \
    CPI_DATA_TYPE(  short,     Short,     u, 16, int16_t,  Short,     uint16_t) \
    CPI_DATA_TYPE(  long,      Long,      u, 32, int32_t,  Long,      uint32_t) \
    CPI_DATA_TYPE_H(octet,     Octet,     u,  8, uint8_t,  UChar,     uint8_t)  \
    CPI_DATA_TYPE(  ulong,     ULong,     u, 32, uint32_t, ULong,     uint32_t) \
    CPI_DATA_TYPE(  ushort,    UShort,    u, 16, uint16_t, UShort,    uint16_t) \
    CPI_DATA_TYPE_X(longlong,  LongLong,  u, 64, int64_t,  LongLong,  uint64_t) \
    CPI_DATA_TYPE_X(ulonglong, ULongLong, u, 64, uint64_t, ULongLong, uint64_t) \
    CPI_DATA_TYPE_S(string,    String,    @, 32, char*,    String,    %^&)      \

#define CPI_DATA_TYPE_H CPI_DATA_TYPE
#define CPI_DATA_TYPE_X CPI_DATA_TYPE
#define CPI_DATA_TYPE_S CPI_DATA_TYPE

#define CPI_CONTROL_OPS                                                        \
  CONTROL_OP(initialize,     Initialize,     INITIALIZED, EXISTS,      NONE,        NONE) \
  CONTROL_OP(start,          Start,          OPERATING,   SUSPENDED,   INITIALIZED, NONE) \
  CONTROL_OP(stop,           Stop,           SUSPENDED,   OPERATING,   NONE,        NONE) \
  CONTROL_OP(release,        Release,        EXISTS,      INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(beforeQuery,    BeforeQuery,    NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(afterConfigure, AfterConfigure, NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(test,           Test,           NONE,        INITIALIZED, NONE,        NONE) \
  /**/

namespace CPI {
  namespace Metadata {
    // This class is the runtime structure for property metadata
    class Worker;
    class Property {
      friend class Worker;
    public:
      enum Type {
        CPI_none, // 0 isn't a valid type
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) CPI_##pretty,
        CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
        CPI_data_type_limit
      };
      // Describe a simple type, along with its max size(for strings)
      struct SimpleType {
        unsigned long size;
        Type type;
      };
      SimpleType *types; // More than one when type is struct.
      const char *name;
      unsigned maxAlign; // Largest alignment requirement based on type (up to 64)
      unsigned ordinal, size;
      // Caller needs these to decide to do beforeQuery/afterConfigure
      bool read_sync, write_sync, is_writable, is_readable;
      // Property description, metadata
      bool is_sequence, is_struct, read_error, write_error, is_test;
      unsigned long sequence_size, numMembers, offset, data_offset;
      // Sizes in bits of the various types
      static uint8_t tsize[CPI_data_type_limit];
      bool decode(ezxml_t x, SimpleType *&s);
      void align(unsigned theOrdinal, unsigned &theOffset);
    private:
    };
  }
}
#endif
