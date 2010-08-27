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
#include "CpiUtilPropertyType.h"
#include "ezxml.h"

namespace CPI {
  namespace Util {
    namespace Prop {
      // This class is the runtime structure for property metadata
      class Property;
      class Member {
      public:
	ValueType type;
	const char *name;   // Member name if struct member
	uint32_t offset;    // for structure members
	unsigned bits, align, nBytes;
	bool hasDefault;
	Scalar::Value defaultValue; // union for member values
	const char *parse(ezxml_t xp, unsigned &maxAlign,
			  unsigned &argOffset, bool &sub32);
      };
      class Property {
	const char *parseImplAlso(ezxml_t x);
      public:
	// Describe structure member that might be the whole property
	const char *name;  // Name of the overall property independent of members
	Member *members;   // More than one when type is struct.
	unsigned maxAlign; // Largest alignment reqmnt based on type (up to 64b)
	unsigned nBytes;   // maximum size in bytes
	unsigned nMembers; // How many members
	unsigned offset;   // Offset within all properties
	// Caller needs these to decide to do beforeQuery/afterConfigure
	bool needReadSync, needWriteSync, isWritable, isReadable;
	bool isParameter;  // For compile-time parameter
	// Attributes for struct types
	bool isStruct, isStructSequence;
	unsigned nStructs; // sequence or array of structs
	// Other property attributes
	bool readError, writeError, isTest;
	unsigned long sequenceLength, dataOffset;
	// Sizes in bits of the various types
	const char *parse(ezxml_t x,
			  unsigned &argOffset,
			  bool &readableConfigs,
			  bool &writableConfigs,
			  bool &sub32Configs,
			  bool includeImpl
			  );
	const char *parseImpl(ezxml_t x);
	const char *parse(ezxml_t x);
	// Check when accessing with scalar type and sequence length
#ifdef NDEBUG
	inline const char *checkType(Scalar::Type ctype, unsigned n, bool write)
	{ return 0;}
#else
	const char *checkType(Scalar::Type ctype, unsigned n, bool write);
#endif
      private:
      };
    }
  }
}
#endif
