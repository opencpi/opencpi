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

// This file exposes the CPI user interface for workers and the ports they own.
#ifndef CPI_PROPERTY_H
#define CPI_PROPERTY_H
#include "CpiMetadataProperty.h"

namespace CPI {
  namespace Container {
    // User interface for runtime property support for a worker, generally copied
    // Optimized for scalar and/or emory mapped access
    // Note that the API for this has the user typically constructing this structure
    // on their stack so that access to members (in inline methods) has no indirection.
    class Property {
      friend class Worker;
      Worker &worker;                     // Use worker if pointers can't work
      Metadata::Property &myMeta;
      Metadata::Property::Type type; // For type-checking user's data type assertion for a property
      Worker::Ordinal ordinal;             // Index into properties of the worker
      unsigned mySequenceSize, myStringSize;
      bool myWriteSync, myReadSync;
      inline void checkType(Metadata::Property::Type ctype, unsigned n) {
#ifndef NDEBUG
        if (ctype != type)
          throw ApiError("incorrect type for this property", 0);
        if (n > mySequenceSize)
          throw ApiError("sequence to too long for this property", 0);
#else 
        ( void ) ctype;
        ( void ) n;
#endif
      }
    public:
      inline const char *name() { return myMeta.name; }
      inline bool is_writable() { return myMeta.is_writable; }
      inline bool is_readable() { return myMeta.is_readable; }
      inline Metadata::Property::Type getType() { return type; }
      inline bool needWriteSync() { return myWriteSync; }
      inline bool needReadSync() { return myReadSync; }
      inline unsigned sequenceSize() { return mySequenceSize; }
      inline unsigned stringSize() { return myStringSize; }
      inline unsigned isSequence() { return mySequenceSize != 0; }
      volatile void *writeVaddr, *readVaddr;  // Pointers non-null if directly usable.
      Property(Worker &, const char *);
      // We don't use templates (sigh) so we can control which types are supported explicitly
      // The "writeVaddr" member is only non-zero if the implementation does not produce errors and 
      // it is atomic at this data size
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      inline void set##pretty##Value(run val) {                                \
        checkType(Metadata::Property::CPI_##pretty, 0);                        \
        if (writeVaddr)                                                        \
          *(store *)writeVaddr= *(store*)((void*)&(val));                        \
        else                                                                \
          worker.set##pretty##Property(ordinal, val);                        \
      }                                                                        \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) {        \
        checkType(Metadata::Property::CPI_##pretty, n);                        \
        worker.set##pretty##SequenceProperty(ordinal, vals, n);                \
      }                                                                        \
      inline run get##pretty##Value() {                                        \
        checkType(Metadata::Property::CPI_##pretty, 0);                        \
        if (readVaddr) {                                                \
          union { store s; run r; }u;                                        \
          u.s = *(store *)readVaddr;                                        \
          return u.r;                                                        \
        } else                                                                \
          return worker.get##pretty##Property(ordinal);                        \
      }                                                                        \
      inline unsigned get##pretty##SequenceValue(run *vals, unsigned length) { \
        return worker.get##pretty##SequenceProperty(ordinal, vals, length);        \
      }
#undef CPI_DATA_TYPE_S
      // for a string we will take a function call
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)              \
      inline void set##pretty##Value(const run val) {                      \
        checkType(Metadata::Property::CPI_##pretty, 0);                      \
        worker.set##pretty##Property(ordinal, val);                      \
      }                                                                      \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) { \
        checkType(Metadata::Property::CPI_##pretty, n);                      \
        worker.set##pretty##SequenceProperty(ordinal, vals, n);              \
      }                                                                      \
      inline void get##pretty##Value(char *val, unsigned length) {    \
        checkType(Metadata::Property::CPI_##pretty, 0);                      \
        worker.get##pretty##Property(ordinal, val, length);              \
      }                                                                      \
      inline unsigned get##pretty##SequenceValue                      \
        (run *vals, unsigned length, char *buf, unsigned space) {     \
        return worker.get##pretty##SequenceProperty                      \
          (ordinal, vals, length, buf, space);                              \
      } 
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#define CPI_DATA_TYPE_S CPI_DATA_TYPE
#undef CPI_DATA_TYPE
    };
  }
}
#endif



