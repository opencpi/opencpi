// This file is for corba-related utility functions exposed in the API
#ifndef _OCPICORBAAPI_H__
#define _OCPICORBAAPI_H__

namespace OCPI {
  namespace API {
    // Convert the stringified object reference to the corbaloc: format.
    // If already corbaloc, then return that
    void ior2corbaloc(const char *ior, std::string &corbaloc) throw (std::string);
    inline void ior2corbaloc(const std::string &ior, std::string &corbaloc) {
      ior2corbaloc(ior.c_str(), corbaloc);
    }
  }
}

#endif
