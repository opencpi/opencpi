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
#include "CpiPropertyTypes.h"
#include "ezxml.h"

namespace CPI {
  namespace Util {
    namespace Property {
    // This class is the runtime structure for property metadata
    class Property {
    public:
      // Describe a simple type that might be a structure member
      struct Member {
	ValueType type;
	const char *name;              // Member name if struct member
	uint32_t offset;	       // for structure members
	bool hasDefault;
	Value defaultValue; // union for scalar value.  non-scalars are malloc'ed
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
