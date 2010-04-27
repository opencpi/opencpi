#include "CpiApplication.h"
#include "CpiWorker.h"
#include "CpiProperty.h"
#include "CpiContainerPort.h"
#include "CpiContainerInterface.h"
#include "CpiContainerMisc.h"

namespace CPI {
  namespace Container {
    // This is user-visible, initialized from information in the metadata
    // It is intended to be constructed on the user's stack - a cache of
    // items needed
    Property::Property(Worker &w, const char *name) : worker(w) {
      // Get the metadata about this property from the worker's database.
      Metadata::Property &md = w.findProperty(name);
      type = md.types->type;
      ordinal = md.ordinal;
      mySequenceSize = md.is_sequence ? md.sequence_size : 0;
      myStringSize = md.types->size;
      myWriteSync = md.write_sync;
      myReadSync = md.read_sync;
      readVaddr = 0;
      writeVaddr = 0;
      // Now ask the underlying implementation to tell us what we can do
      w.prepareProperty(md, *this);
    }
    Worker::Worker(Application &a, ezxml_t impl, ezxml_t inst) :
      CPI::Util::Child<Application,Worker>(a), CPI::Metadata::Worker(impl),
      myXml(impl),
      myImplTag(ezxml_attr(impl, "name")),
      myInstTag(inst ? ezxml_attr(inst, "name") : 0) {
    }

    Worker::Worker( Application & a )
      :  CPI::Util::Child<Application,Worker>(a),
	 CPI::Metadata::Worker((const char *)0)
    {


    }




    // implementations of non-fast set methods
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		            \
    void Worker::set##pretty##Property(Worker::Ordinal p, const run) { assert(0); } \
CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE

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


  }
}
