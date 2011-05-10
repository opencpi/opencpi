
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


// This file exposes the OCPI user interface for workers and the ports they own.
#ifndef OCPI_PROPERTY_H
#define OCPI_PROPERTY_H
#include "OcpiUtilProperty.h"
#include "OcpiContainerMisc.h"
#include "OcpiContainerApi.h"

namespace OCPI {
  namespace Container {
    // User interface for runtime property support for a worker, generally copied
    // Optimized for scalar and/or memory mapped access
    // Note that the API for this has the user typically constructing this structure
    // on their stack so that access to members (in inline methods) has no indirection.
    class Property : OCPI::API::Property {
      friend class Worker;
      Worker &worker;                     // which worker do I belong to
      OCPI::Util::Prop::Property &myMeta;  // reference to info about me
      OCPI::Util::Prop::ValueType type;    //cached info when not a struct
      bool isStruct;
      bool myWriteSync, myReadSync;
    public:
      inline const char *getName() { return myMeta.name; }
      inline bool isWritable() { return myMeta.isWritable; }
      inline bool isReadable() { return myMeta.isReadable; }
      inline OCPI::Util::Prop::Scalar::Type getType() { return type.scalar; }
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
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                  \
      inline void set##pretty##Value(run val) {                                \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, 0, true);      \
        if (writeVaddr)                                                        \
          *(store *)writeVaddr= *(store*)((void*)&(val));                      \
        else                                                                   \
          worker.set##pretty##Property(myMeta, val);                           \
      }                                                                        \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) {    \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, n, true);      \
        worker.set##pretty##SequenceProperty(myMeta, vals, n);                 \
      }                                                                        \
      inline run get##pretty##Value() {                                        \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, 0, false);     \
        if (readVaddr) {                                                       \
          union { store s; run r; }u;                                          \
          u.s = *(store *)readVaddr;                                           \
          return u.r;                                                          \
        } else                                                                 \
          return worker.get##pretty##Property(myMeta);                         \
      }                                                                        \
      inline unsigned get##pretty##SequenceValue(run *vals, unsigned n) {      \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, n, false);     \
        return worker.get##pretty##SequenceProperty(myMeta, vals, n);          \
      }
#undef OCPI_DATA_TYPE_S
      // for a string we will take a function call
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)             \
      inline void set##pretty##Value(const run val) {                       \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, 0, true);   \
        worker.set##pretty##Property(myMeta, val);                          \
      }                                                                     \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) { \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, n, true);   \
        worker.set##pretty##SequenceProperty(myMeta, vals, n);              \
      }                                                                     \
      inline void get##pretty##Value(char *val, unsigned length) {          \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, 0, false);  \
        worker.get##pretty##Property(myMeta, val, length);                  \
      }                                                                     \
      inline unsigned get##pretty##SequenceValue                            \
        (run *vals, unsigned n, char *buf, unsigned space) {                \
        myMeta.checkType(OCPI::Util::Prop::Scalar::OCPI_##pretty, n, false);  \
        return worker.get##pretty##SequenceProperty                         \
          (myMeta, vals, n, buf, space);                                    \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE
    };
  }
}
#endif



