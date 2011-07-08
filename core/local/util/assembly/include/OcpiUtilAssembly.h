
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


/*
 Definitions for assembly metadata decoding, etc.
*/
#ifndef OCPI_ASSEMBLY_H
#define OCPI_ASSEMBLY_H
#include "ezxml.h"

namespace OCPI {
  namespace Util {
    namespace Assy {
      class Instance {
	
      };
      class Connection;
      class Assembly {
	std::string m_name;
	Instance *m_instances;
	Connection *m_connections;
      };
      class Property {
	const char *parseImplAlso(ezxml_t x);
      public:
	// Describe structure member that might be the whole property
	const char *name;     // Name of the overall property independent of members
	Member *members;      // More than one when type is struct.
	unsigned maxAlign;    // Largest alignment reqmnt based on type (up to 64b)
	unsigned nBytes;      // Maximum size in bytes
	unsigned nMembers;    // How many members
	unsigned offset;      // Offset within all properties
	unsigned smallest;    // Smallest unit of storage
	unsigned granularity; // Granularity of smallest unit
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
			  bool includeImpl = false
			  );
	const char *parseImpl(ezxml_t x);
	const char *parse(ezxml_t x);
	const char *parseValue(ezxml_t x, Scalar::Value &value);
	// Check when accessing with scalar type and sequence length
#ifdef NDEBUG
	inline const char *checkType(Scalar::Type ctype, unsigned n, bool write)
	{ ( void ) ctype; ( void ) n; ( void ) write; return 0;}
#else
	const char *checkType(Scalar::Type ctype, unsigned n, bool write);
#endif
      private:
      };
    }
  }
}
#endif