#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_
#include <vector>
#include <string>
#include "OcpiUtilValue.h"
#include "OcpiUtilProperty.h"

// These structures capture what is in or will be put in
// the build configuration file.
// They are also used to dump the makefile fragment output
typedef std::vector<OCPI::Util::Value *> Values;

struct Param {
  OCPI::Util::Value *value;    // the specific value for the current configuration, perhaps the default
  OCPI::Util::Value *newValue;  // non-default value
  std::string uValue;  // unparsed value - the canonical value for comparison
  Values values;       // the values for all configurations
  const OCPI::Util::Property *param; // the property that is a parameter
  const char *parse(ezxml_t px, const OCPI::Util::Property *argParam);
  Param();
};

class Worker;
struct ParamConfig {
  std::vector<Param> params;
  std::string id;
  size_t nConfig; // ordinal
  bool used;  // Is this config in the current set?
  ParamConfig();
  ParamConfig(const ParamConfig &);
  const char * parse(Worker &w, ezxml_t cx);
  void write(Worker &w, FILE *xf, FILE *mf);
  // Is the given configuration the same as this one?
  bool equal(ParamConfig &other);
};

struct ParamConfig;
typedef std::vector<ParamConfig> ParamConfigs;

#endif
