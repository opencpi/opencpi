#include <string>
#include <cstdio>
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "utils.h"

namespace OU = OCPI::Util;
namespace OCPI {
  namespace SCA {
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
