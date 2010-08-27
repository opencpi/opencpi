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
#ifndef CPI_SCA_PROPS_H
#define CPI_SCA_PROPS_H

namespace CPI {
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
  unsigned long size;
  DataType data_type;
} SimpleType;

// Describe a property
 typedef struct {
  const char *name;
  bool is_sequence, is_struct, is_readable, is_writable,
    read_error, write_error, read_sync, write_sync, is_test,
    is_impl;
  unsigned long sequence_size, num_members, offset, data_offset;
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
   unsigned int testId;
   unsigned int numInputs, numResults;
   unsigned int * inputValues;  // reference to property[n]
   unsigned int * resultValues;
 } Test;

// Return a single string, to be freed by caller, or NULL on error.
extern char *encode_props(Property *properties, unsigned nprops, unsigned size,
                          Port *ports, unsigned nports,
                          Test *tests, unsigned ntests);
// Return an array of structs, to be freed by caller (in one "free");
extern bool decode_props(const char *props,
                         Property **propsp, unsigned *nprops, unsigned *size,
                         Port **ports, unsigned *nports,
                         Test **tests, unsigned *ntests);
extern const char *emit_ocpi_xml(const char *specFile, const char *implFile,
				 const char *specName, const char *implName,
				 const char *parentFile, const char *model,
				 char *idlFiles[], bool debug,
				 Property *properties, unsigned nprops,
				 Port *ports, unsigned nports,
				 Test *tests, unsigned ntests);
}}
#endif
