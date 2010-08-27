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
#include "CpiUtilProperty.h"
#include "CpiContainerMisc.h"

namespace CPI {
  namespace Container {
    // User interface for runtime property support for a worker, generally copied
    // Optimized for scalar and/or memory mapped access
    // Note that the API for this has the user typically constructing this structure
    // on their stack so that access to members (in inline methods) has no indirection.
    class Property {
      friend class Worker;
      Worker &worker;                     // which worker do I belong to
      CPI::Util::Prop::Property &myMeta;  // reference to info about me
      CPI::Util::Prop::ValueType type;    //cached info when not a struct
      bool isStruct;
      bool myWriteSync, myReadSync;
    public:
      inline const char *getName() { return myMeta.name; }
      inline bool isWritable() { return myMeta.isWritable; }
      inline bool isReadable() { return myMeta.isReadable; }
      inline CPI::Util::Prop::Scalar::Type getType() { return type.scalar; }
      inline bool needWriteSync() { return myWriteSync; }
      inline bool needReadSync() { return myReadSync; }
      inline unsigned sequenceSize() { return type.length; }
      inline unsigned stringSize() { return type.stringLength; }
      inline unsigned isSequence() { return type.length != 0; }
      volatile void *writeVaddr, *readVaddr;  // Pointers non-null if directly usable.
      Property(Worker &, const char *);
      // We don't use templates (sigh) so we can control which types are supported explicitly
      // The "writeVaddr" member is only non-zero if the implementation does not produce errors and 
      // it is atomic at this data size
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                  \
      inline void set##pretty##Value(run val) {                                \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, 0, true);      \
        if (writeVaddr)                                                        \
          *(store *)writeVaddr= *(store*)((void*)&(val));                      \
        else                                                                   \
          worker.set##pretty##Property(myMeta, val);                           \
      }                                                                        \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) {    \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, n, true);      \
        worker.set##pretty##SequenceProperty(myMeta, vals, n);                 \
      }                                                                        \
      inline run get##pretty##Value() {                                        \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, 0, false);     \
        if (readVaddr) {                                                       \
          union { store s; run r; }u;                                          \
          u.s = *(store *)readVaddr;                                           \
          return u.r;                                                          \
        } else                                                                 \
          return worker.get##pretty##Property(myMeta);                         \
      }                                                                        \
      inline unsigned get##pretty##SequenceValue(run *vals, unsigned n) {      \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, n, false);     \
        return worker.get##pretty##SequenceProperty(myMeta, vals, n);          \
      }
#undef CPI_DATA_TYPE_S
      // for a string we will take a function call
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)             \
      inline void set##pretty##Value(const run val) {                       \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, 0, true);   \
        worker.set##pretty##Property(myMeta, val);                          \
      }                                                                     \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) { \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, n, true);   \
        worker.set##pretty##SequenceProperty(myMeta, vals, n);              \
      }                                                                     \
      inline void get##pretty##Value(char *val, unsigned length) {          \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, 0, false);  \
        worker.get##pretty##Property(myMeta, val, length);                  \
      }                                                                     \
      inline unsigned get##pretty##SequenceValue                            \
        (run *vals, unsigned n, char *buf, unsigned space) {                \
        myMeta.checkType(CPI::Util::Prop::Scalar::CPI_##pretty, n, false);  \
        return worker.get##pretty##SequenceProperty                         \
          (myMeta, vals, n, buf, space);                                    \
      }
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#define CPI_DATA_TYPE_S CPI_DATA_TYPE
#undef CPI_DATA_TYPE
    };
  }
}
#endif



