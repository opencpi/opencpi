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

// This file defines runtime data types for properties, property value lists,
// parsing, printing etc.



#ifndef CPI_PROPERTY_TYPES_H
#define CPI_PROPERTY_TYPES_H
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS // wierd standards goof up
#endif
#include <stdint.h>
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
    CPI_DATA_TYPE_S(string,    String,    @, 32,  char*,    String,    %^&)      \

// NOTE above that strings are aligned at 32 bits

#define CPI_DATA_TYPE_H CPI_DATA_TYPE
#define CPI_DATA_TYPE_X CPI_DATA_TYPE
#define CPI_DATA_TYPE_S CPI_DATA_TYPE

namespace CPI {
  namespace Util {
    namespace Property {
      enum ScalarType {
        CPI_none, // 0 isn't a valid type
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) CPI_##pretty,
        CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
        CPI_data_type_limit
      };
      union ScalarValue {
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) run v##pretty;
	CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
      };
      struct ValueType {
	ScalarType scalarType;
	bool isSequence, isArray;
	uint32_t
	  stringLength, // maximum strlen (terminating null not included)
	  length;       // maximum for sequences, specific length for arrays
      };
    }
  }
}
