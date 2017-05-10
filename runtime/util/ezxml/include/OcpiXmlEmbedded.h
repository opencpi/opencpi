#ifndef OCPI_XML_EMBEDDED_H
#define OCPI_XML_EMBEDDED_H

#include <string>

namespace OCPI {
  namespace Util {
    namespace EzXml {
      // Adds XML metadata string to end of artifact file:
      void artifact_addXML(const std::string &fname, const std::string &xml);
      // Will query XML metadata string from end of artifact file:
      void artifact_getXML(const std::string &fname, std::string &xml);
      // Will remove XML metadata from end of artifact file:
      bool artifact_stripXML(const std::string &fname);
    }
  }
}

#endif
