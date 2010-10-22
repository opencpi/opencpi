
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

#include "OcpiApplication.h"
#include "OcpiWorker.h"
#include "OcpiProperty.h"
#include "OcpiContainerPort.h"
#include "OcpiContainerInterface.h"
#include "OcpiContainerMisc.h"

namespace OCPI {
  namespace CP = Util::Prop;
  namespace CM = Metadata;
  namespace Container {
    // This is user-visible, initialized from information in the metadata
    // It is intended to be constructed on the user's stack - a cache of
    // just the items needed for fastest access
    Property::Property(Worker &w, const char *aname) :
      worker(w), myMeta(w.findProperty(aname)) {
      // Get the metadata about this property from the worker's database.
      if (myMeta.isStruct) {
	type.scalar = CP::Scalar::OCPI_none; // Make all scalar type checks fail
	isStruct = true;
      } else
	type = myMeta.members->type;
      myWriteSync = myMeta.needWriteSync;
      myReadSync = myMeta.needReadSync;
      readVaddr = 0;
      writeVaddr = 0;
      // Now ask the underlying implementation to tell us what we can do
      w.prepareProperty(myMeta, *this);
    }
    Controllable::Controllable(const char *ops)
      : myState(CM::Worker::EXISTS)  {
#define CONTROL_OP(x, c, t, s1, s2, s3) \
      if (ops && strstr(ops, #x)) \
	controlMask |= 1 << CM::Worker::Op##c;
      OCPI_CONTROL_OPS
#undef CONTROL_OP
    }

    Worker::Worker(Application &a, ezxml_t impl, ezxml_t inst) :
      OCPI::Util::Child<Application,Worker>(a), CM::Worker(impl),
      Controllable(ezxml_cattr(impl, "controlOperations")),
      myXml(impl),
      myImplTag(impl ? ezxml_cattr(impl, "name") : 0),
      myInstTag(inst ? ezxml_cattr(inst, "name") : 0) {
    }

    Worker::~Worker(){}
    bool Worker::hasImplTag(const char *tag) {
      return strcmp(tag, myImplTag) == 0;
    }
    bool Worker::hasInstTag(const char *tag) {
      return strcmp(tag, myInstTag) == 0;
    }
    Port &Worker::
    getPort(const char *name) {
      Port *p = findChild(&Port::hasName, name);
      if (p)
        return *p;
      OCPI::Metadata::Port *metaPort = findPort(name);
      if (!metaPort)
        throw ApiError("no port found with name \"", name, "\"", NULL);
      return createPort(*metaPort);
    }


    void Worker::setProperty(const char *name, const char *value) {
      Property prop(*this, name);
      CP::Scalar::Value v; // FIXME storage when not scalar
      if (prop.isStruct)
	throw ApiError("No support yet for setting struct properties", NULL);
      CP::ValueType &vt = prop.myMeta.members->type;
      if (vt.length > 1)
	throw ApiError("No support yet for setting sequence/array properties",
		       NULL);
      const char *err = v.parse(value, vt.scalar, vt.stringLength);
      if (err)
        throw ApiError("Error parsing property value", NULL);
      switch (vt.scalar) {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                  \
	case CP::Scalar::OCPI_##pretty: prop.set##pretty##Value(v.run); break; \
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      default:
	ocpiAssert(!"unknown data type");
      }
    }
  }
}
