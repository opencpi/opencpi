#ifndef OCPI_UTIL_EXCEPTION_API_H
#define OCPI_UTIL_EXCEPTION_API_H
#include <string>
namespace OCPI {
  namespace API {
    // simplest error that API caller's see
    class Error {
    protected:
      virtual ~Error();
    public:
      virtual const std::string &error() const = 0;
    };
  }
}
#endif
