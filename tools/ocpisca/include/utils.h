#include "ezxml.h"

namespace OCPI {
  namespace SCA {
    ezxml_t addChild(ezxml_t x, const char *name, unsigned level, const char *txt = NULL,
		     const char *attr1 = NULL, const char *value1 = NULL,
		     const char *attr2 = NULL, const char *value2 = NULL);
  }
}

