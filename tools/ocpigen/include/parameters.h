#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_
#include <vector>
#include <string>
#include "OcpiUtilValue.h"
#include "OcpiUtilProperty.h"

// These structures capture what is in or will be put in
// the build configuration file.
// They are also used to dump the makefile fragment output

// This vector holds a sequence of alternative values for a given parameter
// It is held as a string because it can't be parsed until the other
// parameter values are known (e.g. the length of an array in another parameter).
typedef std::vector<std::string> Values;

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
class ParamConfig : public OCPI::Util::IdentResolver {
  Worker &m_worker;
 public:
  std::vector<Param> params;
  std::string id;
  size_t nConfig; // ordinal
  bool used;  // Is this config in the current set?
  ParamConfig(Worker &w);
  ParamConfig(const ParamConfig &);
  ParamConfig &operator=(const ParamConfig * p);
  const char * parse(ezxml_t cx);
  void write(FILE *xf, FILE *mf);
  void writeVhdlConstants(FILE *gf);
  // Is the given configuration the same as this one?
  bool equal(ParamConfig &other);
  // The callback when evaluating expressions for data types (e.g. array length).
  const char *getValue(const char *sym, OCPI::Util::ExprValue &val) const;
};

// This must be pointers since it has a reference member which can't be copied,
// and we're not using c++11 yet, with "emplace".
typedef std::vector<ParamConfig*> ParamConfigs;

#endif
