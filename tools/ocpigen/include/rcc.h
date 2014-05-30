#ifndef RCC_H
#define RCC_H
#include "wip.h"

#define RCC_ASSEMBLY_ATTRS "platform", "config", "configuration"
#define RCC_ASSEMBLY_ELEMS "connection"
// These are for all implementaitons whether assembly or written
#define RCC_TOP_ATTRS "Pattern", "PortPattern", "DataWidth", "Language", "library"
// These are for implementaitons that you write (e.g. not generated assemblies)
#define RCC_IMPL_ATTRS GENERIC_IMPL_CONTROL_ATTRS, "RawProperties", "FirstRawProperty", "outer"
#define RCC_IMPL_ELEMS "timeinterface", "memoryinterface", "streaminterface", "messageinterface", "signal", "cpmaster", "time_service", "control", "metadata"

class RccAssembly : public Worker {
public:  
  static RccAssembly *
  create(ezxml_t xml, const char *xfile, const char *&err);
  RccAssembly(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~RccAssembly();
  const char *emitArtXML(const char *wksfile);
};

#endif
