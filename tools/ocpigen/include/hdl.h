#ifndef HDL_H
#define HDL_H
#include "wip.h"

#define HDL_ASSEMBLY_ATTRS "platform", "config", "configuration"
#define HDL_ASSEMBLY_ELEMS "connection"
class HdlAssembly : public Worker {
public:  
  static HdlAssembly *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlAssembly(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~HdlAssembly();
};



// Global to let worker know whether an assembly is being built or just a worker
extern bool hdlAssy;

#endif
