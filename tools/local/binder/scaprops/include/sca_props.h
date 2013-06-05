
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

 Definitions for sca property encoding and decoding.
 Offline, in tools, property information is encoded into a string format 
 suitable for program level arguments (argv).  All properties are encoded into
 a single string.  This relieves runtime of any parsing overhead (time or space)
 or dependencies on external parsing libraries.

 The "scope" of this property support is configuration properties for CP289
 components.  Thus it is not (yet) intended to support SCA GPP components.

 This file defines the binary (non-string) format of SCA component properties,
 as well as the functions to encode (binary to string) and decode 
 (string to binary).

 Yes, this code is "C-ish C++".
*/
#ifndef OCPI_SCA_PROPS_H
#define OCPI_SCA_PROPS_H

namespace OCPI {
namespace SCA {

#define SCA_DATA_TYPES \
  SCA_DATA_TYPE_H(boolean, Boolean, u, 8)                        \
    SCA_DATA_TYPE_H(char, Char, u, 8)                                \
    SCA_DATA_TYPE(double, Double, f, 64)                        \
    SCA_DATA_TYPE(float, Float, f, 32)                                \
    SCA_DATA_TYPE(short, Short, u, 16)                                \
    SCA_DATA_TYPE(long, Long, u, 32)                                \
    SCA_DATA_TYPE_H(octet, Octet, u, 8)                                \
    SCA_DATA_TYPE(ulong, ULong, u, 32)                                \
    SCA_DATA_TYPE(ushort, UShort, u, 16)                        \
    SCA_DATA_TYPE_S(string, String)

#define SCA_DATA_TYPE_H SCA_DATA_TYPE
#define SCA_DATA_TYPE_S(s,c) SCA_DATA_TYPE(s,c,x,0)

typedef enum {
  SCA_none,
#define SCA_DATA_TYPE(l,c,t,n) SCA_##l,
SCA_DATA_TYPES
#undef SCA_DATA_TYPE
SCA_data_type_limit
} DataType;

// Describe a simple type, along with its max size(for strings)
typedef struct {
  size_t size;
  DataType data_type;
  const char *name; // for struct members
} SimpleType;

// Describe a property
 typedef struct {
  const char *name;
  bool is_sequence, is_struct, is_readable, is_writable,
    read_error, write_error, read_sync, write_sync, is_test,
    is_impl;
  size_t sequence_size, num_members, offset, data_offset;
  const volatile unsigned char *read_vaddr;
  volatile unsigned char *write_vaddr;
  SimpleType *types; // More than one when type is struct.
 } Property;

 // Describe a port
 typedef struct {
   const char *name;
   const char *repid;
   bool provider, twoway;
 } Port;

 typedef struct {
   uint32_t testId;
   unsigned int numInputs, numResults;
   unsigned int * inputValues;  // reference to property[n]
   unsigned int * resultValues;
 } Test;

// Return a single string, to be freed by caller, or NULL on error.
extern char *encode_props(Property *properties, size_t nprops, size_t size,
                          Port *ports, size_t nports,
                          Test *tests, size_t ntests);
// Return an array of structs, to be freed by caller (in one "free");
extern bool decode_props(const char *props,
                         Property **propsp, size_t *nprops, size_t *size,
                         Port **ports, size_t *nports,
                         Test **tests, size_t *ntests);
extern const char *emit_ocpi_xml(const char *specFile, const char *implFile,
				 const char *specName, const char *implName,
				 const char *parentFile, const char *model,
				 char *idlFiles[], bool debug,
				 Property *properties, size_t nprops,
				 Port *ports, size_t nports,
				 Test *tests, size_t ntests);
}}
#endif
