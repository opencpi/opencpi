#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_
#include <vector>
#include <set>
#include <string>
#include "OcpiUtilValue.h"
#include "OcpiUtilProperty.h"
#include "cdkutils.h"
#include "ocpigen.h"

// These structures capture what is in or will be put in
// the build configuration file.
// They are also used to dump the makefile fragment output

// This vector holds a sequence of alternative values for a given parameter
// It is held as a string because it can't be parsed until the other
// parameter values are known (e.g. the length of an array in another parameter).
typedef std::vector<std::string> Values;
typedef std::set<std::string> Strings;
typedef Strings::const_iterator StringsIter;

class Worker;
#define PARAM_ATTRS "name", "value", "values", "valueFile", "valueFiles"
struct Param {
  OCPI::Util::Value           m_value;      // value for the current config, perhaps the default
  std::string                 m_uValue;     // unparsed value: the canonical value for comparison
  OCPI::Util::Member         *m_valuesType; // the type (a sequence of these values).
  Values                      m_uValues;    // *Either* parsed from XML or captured from raw
  const OCPI::Util::Property *m_param;      // the property that is a parameter
  bool                        m_isDefault;  // is m_value from property default?
  Worker                     *m_worker;     // the worker of this param
                                            // when the paramconfig spans impls
  bool                        m_isTest;
  std::string                 m_generate;   // how to generate a value
  Strings                     m_explicitPlatforms; // platforms w/ all platform-specified values
  struct Attributes {            // these attributes are PER VALUE
    bool         m_onlyExcluded; // the value is only excluded, for platforms in m_excluded
                                 // which is only when exclusion happens before inclusion
    Strings      m_excluded;     // platforms this value is excluded for 
                                 // i.e. if platform is in set, don't use value
    Strings      m_included;     // the platforms this value is used for
                                 // i.e. if platform is not in set, don't use value
    Strings      m_only;         // the platforms it is explicitly set for using "only".
                                 // i.e. if the platform is explicit, val is used if in this set
    Attributes() : m_onlyExcluded(false) {}
  };
  std::vector<Attributes>     m_attributes;
  //  static const char *getPlatforms(const char *platform, Strings &platforms);
  Param();
  const char 
    *parse(ezxml_t px, const OCPI::Util::Property &prop, bool global = false),
    *excludeValue(std::string &uValue, Attributes *&attrs, const char *platform),
    *addValue(std::string &uValue, Attributes *&attrs, const char *platform),
    *onlyValue(std::string &uValue, Attributes *&attrs, const char *platform);
};

class Worker;
class ParamConfig;
// This must be pointers since it has a reference member which can't be copied,
// and we're not using c++11 yet, with "emplace".
typedef std::vector<ParamConfig*> ParamConfigs;
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
  const char * parse(ezxml_t cx, const ParamConfigs &configs); //, bool includeInitial = false);
  void doDefaults(); //bool includeInitial = false);
  void write(FILE *xf, FILE *mf);
  void writeConstants(FILE *gf, Language lang);
  // Is the given configuration the same as this one?
  bool equal(ParamConfig &other);
  // The callback when evaluating expressions for data types (e.g. array length).
  const char *getValue(const char *sym, OCPI::Util::ExprValue &val) const;
};

// The build information that is not necessary for code generation.
// (except the actual explicit configs, which are in ParamConfigs).
// Many of the lists need to preserve the original XML ordering
struct Build {
  Worker             &m_worker;
  ParamConfig         m_globalParams;  // parameters set for all non-id'd build configurations
  OrderedStringSet    m_onlyPlatforms, m_excludePlatforms;
  OrderedStringSet    m_onlyTargets, m_excludeTargets;
  OrderedStringSet    m_sourceFiles;    // absolute or relative to the worker dir ORDERED
  OrderedStringSet    m_libraries;      // primitive libraries, slashes imply no search
  OrderedStringSet    m_xmlIncludeDirs; // include paths for XML files
  OrderedStringSet    m_includeDirs;    // include paths for source files
  OrderedStringSet    m_componentLibraries;
  // HDL-specific
  OrderedStringSet    m_cores;
  // RCC-specific
  OrderedStringSet    m_staticPrereqLibs;
  OrderedStringSet    m_dynamicPrereqLibs;
  Build(Worker &w);
  const char *parse(ezxml_t x);
  void writeMakeVars(FILE *mkFile);
};
#endif
