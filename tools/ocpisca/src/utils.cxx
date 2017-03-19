#include <string>
#include <cstdio>
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "utils.h"

namespace OU = OCPI::Util;
namespace OCPI {
  namespace SCA {
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
    void
    writeXml(ezxml_t root, FILE *f, const char *type, const char *dtd, const std::string &path) {
      const char *xml = ezxml_toxml(root);
      if (fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		  "<!DOCTYPE %s PUBLIC \"-//JTRS//DTD SCA V2.2.2 %s//EN\" "
		  "\"%s.dtd\">\n", dtd, type, dtd) == EOF ||
	  fputs(xml, f) == EOF || fputc('\n', f) == EOF || fclose(f))
	throw OU::Error("Error closing %s output file: \"%s\"  No space?", type, path.c_str());
    }
  }
}
