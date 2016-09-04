//#include "string.h"
#include <string>
#include "OcpiUtilMisc.h"
#include "utils.h"

namespace OCPI {
  namespace SCA {
    namespace OU = OCPI::Util;
    ezxml_t
    addChild(ezxml_t x, const char *name, unsigned level, const char *txt, const char *attr1,
	     const char *value1, const char *attr2, const char *value2) {
      const char *otxt = ezxml_txt(x);
      const char *nl = strrchr(otxt, '\n');
      std::string text;
      text.assign(otxt, nl ? nl - otxt : strlen(otxt));
      OU::formatAdd(text, "\n%*s", level*2, "");
      ezxml_t cx = ezxml_add_child(x, name, text.length());
      if (txt)
	ezxml_set_txt_d(cx, txt);
      if (attr1)
	ezxml_set_attr_d(cx, attr1, value1);
      if (attr2)
	ezxml_set_attr_d(cx, attr2, value2);
      OU::formatAdd(text, "\n%*s", (level-1)*2, "");
      ezxml_set_txt_d(x, text.c_str());
      return cx;
    }
  }
}
