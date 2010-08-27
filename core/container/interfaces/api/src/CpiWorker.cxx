#include "CpiApplication.h"
#include "CpiWorker.h"
#include "CpiProperty.h"
#include "CpiContainerPort.h"
#include "CpiContainerInterface.h"
#include "CpiContainerMisc.h"

namespace CPI {
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
	type.scalar = CP::Scalar::CPI_none; // Make all scalar type checks fail
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
      CPI_CONTROL_OPS
#undef CONTROL_OP
    }

    Worker::Worker(Application &a, ezxml_t impl, ezxml_t inst) :
      CPI::Util::Child<Application,Worker>(a), CM::Worker(impl),
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
      CPI::Metadata::Port *metaPort = findPort(name);
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
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                  \
	case CP::Scalar::CPI_##pretty: prop.set##pretty##Value(v.run); break; \
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
      default:
	cpiAssert(!"unknown data type");
      }
    }
  }
}
