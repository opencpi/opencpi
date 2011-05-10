#ifndef OcpiDriverManager_h
#define OcpiDriverManager_h
namespace OCPI {
  namespace API {
    class DriverManager {
      static void configure(const char *file = NULL);
    };
  }
}
#endif
