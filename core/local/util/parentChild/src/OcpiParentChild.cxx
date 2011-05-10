#include <OcpiUtilMisc.h>
#include <OcpiParentChild.h>
namespace OCPI {
  namespace Util {
    const char *child = "child";  // template argument
    std::string childName(const char *name, const char *prefix) {
      static uint32_t childCount = 0;
      return name ? name : prefix + Misc::unsignedToString(childCount++);
    }
  }
}
