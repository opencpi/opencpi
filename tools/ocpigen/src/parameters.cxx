/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Parameter processing
#include <assert.h>
#include <strings.h>
#include <fnmatch.h>
#include "OcpiOsFileSystem.h"
#include "parameters.h"
#include "wip.h"
#include "hdl.h"
#include "hdl-device.h"
namespace OU=OCPI::Util;
namespace OF=OCPI::OS::FileSystem;

const char *Worker::
addParamConfigSuffix(std::string &s) {
  if (m_paramConfig && m_paramConfig->nConfig)
    OU::formatAdd(s, "_c%zu", m_paramConfig->nConfig);
  return s.c_str();
}

const char *Worker::
findParamProperty(const char *a_name, OU::Property *&prop, size_t &nParam, bool includeInitial) {
  size_t n = 0;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if (!strcasecmp((*pi)->m_name.c_str(), a_name)) {
      prop = *pi;
      if (prop->m_isParameter || (includeInitial && prop->m_isWritable)) {
	nParam = n;
	return NULL;
      } else
	return OU::esprintf("The '%s' property is not a parameter", a_name);
    } else if ((*pi)->m_isParameter || (includeInitial && (*pi)->m_isWritable))
      n++;
  return OU::esprintf("Parameter property not found: '%s'", a_name);
}

Param::Param() : m_valuesType(NULL), m_param(NULL), m_isDefault(false), m_worker(NULL),
		 m_isTest(false) {}

void Param::
fullName(const OCPI::Util::Property &prop, const Worker *w, std::string &name) {
  if (prop.m_isImpl) {
    assert(w);
    OU::format(name, "%s.%s.%s", w->cname(), w->m_modelString, prop.cname());
  } else
    name = prop.cname(); // spec prop, common name for all workers
}

void Param::
setProperty(const OCPI::Util::Property *prop, const Worker *w) {
  assert(prop || m_param);
  //  assert(w || m_worker); no worker means a test property in unit test
  if (prop) {
    if (m_param)
      assert(m_param == prop);
    m_param = prop;
  }
  if (w) {
    if (m_worker)
      assert(m_worker == w);
    m_worker = w;
  }
  std::string name;
  fullName(*m_param, m_worker, name);
  if (m_name.length())
    assert(m_name == name);
  m_name = name;
}

// Exclude these values from the set
// One use case, when not global, is to subtract from the default set
// Other use case, add a line that can be deleted while leaving the normal set alone
// If a platform is supplied, ensure that these values are not used for that platform
const char *Param::
excludeValue(std::string &uValue, Attributes *&attrs, const char *platform) {
  if (platform && m_explicitPlatforms.find(platform) != m_explicitPlatforms.end())
    return OU::esprintf("Can't exclude values for platforms using \"only\" attribute");
  if (m_uValues.empty())
    ocpiBad("excluding property value \"%s\" when there are no existing values?", uValue.c_str());
  else if (attrs) {
    if (platform)
      attrs->m_excluded.insert(platform);
    else if (attrs->m_excluded.empty()) {
      ssize_t n = (attrs - &m_attributes[0]);
      m_uValues.erase(m_uValues.begin() + n);
      m_attributes.erase(m_attributes.begin() + n);
    } else
      attrs->m_onlyExcluded = true;
    return NULL;
  } else
    ocpiBad("excluded value \"%s\" does not match any existing value", uValue.c_str());
  m_uValues.push_back(uValue);
  m_attributes.resize(m_attributes.size() + 1);
  attrs = &m_attributes.back();
  attrs->m_onlyExcluded = true;
  if (platform)
    attrs->m_excluded.insert(platform);
  return NULL;
}
// Add to the set the values for this parameter
// If a platform is supplied, this adds the values just for the platform
const char *Param::
addValue(std::string &uValue, Attributes *&attrs, const char *platform) {
  if (platform && m_explicitPlatforms.find(platform) != m_explicitPlatforms.end())
    return "if any value for a platform is specified with \"only\", all must be";
  // Its already there, but under what conditions?
  if (attrs && platform && attrs->m_excluded.find(platform) != attrs->m_excluded.end())
    return "value is already excluded for platform";
  if (attrs && !attrs->m_onlyExcluded) {
    // Its already being used since attrs exists and not only for exclusion
    if (platform) {
      if (attrs->m_included.find(platform) != attrs->m_included.end())
	return "value is already included for platform";
      if (attrs->m_included.empty()) {
	ocpiBad("value \"%s\" is already specified for general use, "
		"ignoring it for platform \"%s\"", uValue.c_str(), platform);
	return NULL;
      }
      // So it is only included for some other platforms
    } else {
      ocpiBad("value \"%s\" is already specified", uValue.c_str());
      return NULL;
    }
  } else if (attrs && attrs->m_onlyExcluded)
    attrs->m_onlyExcluded = false;
  else {
    m_uValues.push_back(uValue);
    m_attributes.resize(m_attributes.size() + 1);
    attrs = &m_attributes.back();
  }
  if (platform)
    attrs->m_included.insert(platform);
  return NULL;
}
// Set the values for this parameter - making the provided values the only values.
// If a platform is supplied, this sets the exact set of values for that platform.
const char *Param::
onlyValue(std::string &uValue, Attributes *&attrs, const char *platform) {
  if (!platform)
    return OU::esprintf("The \"only\" attribute can only be true when a platform is specified");
  if (attrs && !attrs->m_onlyExcluded) {
    if (attrs->m_only.find(platform) != attrs->m_only.end()) {
      ocpiBad("value \%s\" is already specified explicitly for platform \"%s\"",
	      uValue.c_str(), platform);
      return NULL;
    }
  } else if (attrs && attrs->m_onlyExcluded)
    attrs->m_onlyExcluded = false;
  else {
    m_uValues.push_back(uValue);
    m_attributes.resize(m_attributes.size() + 1);
    attrs = &m_attributes.back();
  }
  m_explicitPlatforms.insert(platform);
  attrs->m_only.insert(platform);
  return NULL;
}

// the "global" argument is true when this parameter has a global setting across many other
// configurations and so it cannot be given a value if it is dependent on other parameters for
// its type (e.g. array dimensions)
const char *Param::
parse(ezxml_t px, const OU::Property *p, const Worker *worker, bool global) {
  std::string xValue;
  const char *err;
  const char
    *generate = ezxml_cattr(px, "generate"),
    *value = ezxml_cattr(px, "value"),
    *values = ezxml_cattr(px, "values"),
    *valueFile = ezxml_cattr(px, "valueFile"),
    *valuesFile = ezxml_cattr(px, "valuesFile");
  if ((!!value + !!values + !!valueFile + !!valuesFile + !!generate) != 1)
    return OU::esprintf("Exactly one attribute must be specified among: "
			"value, values, valuefile, valuesFile, or (for tests) generate");
  setProperty(p, worker);  // possibly overwriting
  const OU::Property &prop = *m_param;
  if (generate) {
    m_generate = generate;
    return NULL;
  } else if (prop.m_usesParameters && global)
    return OU::esprintf("Property \"%s\" must be generated since its type depends on parameters",
			prop.cname());
  std::string fileValue;
  if (valueFile) {
    if ((err = (prop.needsNewLineBraces() ?
		OU::file2String(fileValue, valueFile, "{", "},{", "}") :
		OU::file2String(fileValue, valueFile, ','))))
      return err;
    value = fileValue.c_str();
  } else if (valuesFile) {
    if ((err = (prop.needsComma() ?
		OU::file2String(fileValue, valuesFile, "{", "},{", "}") :
		OU::file2String(fileValue, valuesFile, ','))))
      return err;
    values = fileValue.c_str();
  }
  // =============================================================================
  // We might be overwriting a global parsed value here.
  // The type might be finalized differently (based on different parameter values)
  // =============================================================================
  m_value.setType(prop); // not necessarily setting a value here, but we know the type
  if (values) {
    // if (m_valuesType) This is now a memory leak: FIXME: write the copy-constructor
    //       delete m_valuesType;
    m_valuesType = &prop.sequenceType();
    m_valuesType->m_default = new OU::Value(*m_valuesType);
    if ((err = m_valuesType->m_default->parse(values, NULL, false, NULL)))
      return err;
    // The specified values are in: *m_valuesType->m_default, not unparsed
  } else {
    OU::Value newValue;
    if ((err = prop.parseValue(value, newValue)))
      return err;
    m_isDefault = false;
    newValue.unparse(m_uValue);
    if (prop.m_default) {
      std::string defValue;
      prop.m_default->unparse(defValue);
      if (defValue == m_uValue) {
	m_isDefault = true;
	m_value = *prop.m_default; // copy
      } else
	m_value = newValue; // copy
    } else
      m_value = newValue; // copy
    // The specified value is in: m_value, unparsed in m_uValue
  }
  const char // all these can be glob-wildcarded
    *add = ezxml_cattr(px, "add"),
    *exclude = ezxml_cattr(px, "exclude"),
    *only = ezxml_cattr(px, "only");
  if (exclude && only)
    return OU::esprintf("setting both \"only\" and \"exclude\" attributes is invalid");
  OrderedStringSet platforms;
  if ((err = getPlatforms(exclude, platforms)) ||
      (err = getPlatforms(only, platforms)) ||
      (err = getPlatforms(add, platforms)))
    return err;
  // We have captured the incoming values - now we just have to decide what to do with them
  // We are setting, adding, or excluding the values
  // We are dealing with a global parameter (applied to all cases/configs) or for one case/config
  // We can optionally specify a platform to which these values apply
  std::string uValue;
  if (!exclude && !only && !add) {
    m_uValues.clear(); // forget previous values
    m_attributes.clear();
  }
  for (unsigned n = 0; !err && n < (values ? m_valuesType->m_default->m_nElements : 1); n++) {
    if (values) {
      uValue.clear(); // needed?
      m_valuesType->m_default->elementUnparse(*m_valuesType->m_default, uValue, n, false,
					      '\0', false, *m_valuesType->m_default);
      //      if (n == 0)
      //	m_value.parse(uValue.c_str()); // make sure m_value reflects the first value
    } else
      uValue = m_uValue;
    Attributes *attrs = NULL;
    for (unsigned nn = 0; nn < m_uValues.size(); nn++)
      if (uValue == m_uValues[nn]) {
	attrs = &m_attributes[nn];
	break;
      }
    if (platforms.size())
      for (auto pi = platforms.begin(); pi != platforms.end(); ++pi) {
	const char *platform = pi->c_str();
	if (exclude)
	  err = excludeValue(uValue, attrs, platform);
	else if (only)
	  err = onlyValue(uValue, attrs, platform);
	else if (add)
	  err = addValue(uValue, attrs, platform);
      }
    else
      err = addValue(uValue, attrs, NULL);
  }
  return err;
}

ParamConfig::
ParamConfig(Worker &w) : m_worker(w), nConfig(0), used(false) {
  params.resize(w.m_ctl.properties.size());
}

void ParamConfig::
clone(const ParamConfig &other) {
  params = other.params;
  id = other.id;
  nConfig = other.nConfig;
  used = other.used;
}

ParamConfig::
ParamConfig(const ParamConfig &other)
  : m_worker(other.m_worker) {
  clone(other);
}

// Fill in unspecified parameters with their single default value
void ParamConfig::
doDefaults() {
  params.resize(m_worker.m_ctl.nParameters); // in case parameter properties were added recently
  size_t n = 0;
  for (PropertiesIter pi = m_worker.m_ctl.properties.begin();
       pi != m_worker.m_ctl.properties.end(); pi++) {
    OU::Property &p = **pi;
    if (p.m_isParameter) {
      assert(n == p.m_paramOrdinal);
      if (!params[n].m_param) { // If we didn't see it when parsing this config
	assert(params[n].m_uValues.size() == 0);
	params[n].setProperty(&p, &m_worker);
	params[n].m_isDefault = true;
	if (p.m_default) {
	  if (p.m_defaultExpr.length()) {
	    // If the default is an expression, reevaluate it with the current parameter values.
	    params[n].m_value.setType(p);    // blank default value
	    params[n].m_value.parse(p.m_defaultExpr.c_str(), NULL, false, this, NULL);
	    params[n].m_isDefault = false;
	  } else
	    params[n].m_value = *p.m_default; // assignment operator to copy the value
	} else
	  params[n].m_value.setType(p);       // blank default value
	params[n].m_value.unparse(params[n].m_uValue);
	params[n].m_uValues.push_back(params[n].m_uValue);
	params[n].m_attributes.resize(params[n].m_attributes.size() + 1);
      }
      n++;
    }
  }
}
const char *ParamConfig::
parse(ezxml_t cx, const ParamConfigs &configs) { // , bool includeInitial) {
  const char *err;
  bool haveId;
  if ((err = OE::getNumber(cx, "id", &nConfig, &haveId)))
    return err;
  if (haveId) {
    for (unsigned n = 0; n < configs.size(); n++)
      if (configs[n]->nConfig == nConfig)
	return OU::esprintf("Duplicate configuration id %zu in build file", nConfig);
    OU::format(id, "%zu", nConfig);
  }
  // Note that this resize when including initial props, will overallocate, e.g. volatiles.
  params.resize(//includeInitial ? m_worker.m_ctl.properties.size() :
		m_worker.m_ctl.nParameters);
  for (ezxml_t px = ezxml_cchild(cx, "parameter"); px; px = ezxml_cnext(px)) {
    if ((err = OE::checkAttrs(px, PARAM_ATTRS, NULL)))
      return err;
    std::string name;
    size_t nParam;
    OU::Property *p;
    if ((err = OE::getRequiredString(px, name, "name")) ||
	(err = m_worker.findParamProperty(name.c_str(), p, nParam)) ||
	(err = p->finalize(*this, "property", false)) || // FIXME: false?  and what about doing this in a 2nd pass?
	(err = params[nParam].parse(px, p, &m_worker)))
      return err;
  }
  return NULL;
}

const char *ParamConfig::
getValue(const char *sym, OU::ExprValue &val) const {
  OU::Property *prop;
  size_t nParam; // FIXME not needed since properties have m_paramOrdinal?
  const char *err;
  if ((err = m_worker.findParamProperty(sym, prop, nParam)))
    return err;
  const Param &param = params[nParam];
  const OU::Value *v;
  if (param.m_param) {
    if (param.m_uValues.size() > 1)
      return OU::esprintf("parameter property %s used in expression, but has multiple values",
			  prop->cname());
    v = &param.m_value;
  } else if (!(v = prop->m_default))
    return OU::esprintf("The parameter \"%s\" has no default or specified value", sym);
  return extractExprValue(*prop, *v, val);
}

const char *Worker::
paramValue(const OU::Member &param, OU::Value &v, std::string &value) {
  switch (m_model) {
  case HdlModel:
    return hdlValue(param.m_name.c_str(), v, value, true);
  case RccModel:
    return rccValue(v, value, param);
  case OclModel:
    return rccValue(v, value, param);
  default:
    assert("bad model" == 0);
  }
  return NULL;
}
void ParamConfig::
write(FILE *xf, FILE *mf) {
  bool nonDefault = false;
  for (PropertiesIter pi = m_worker.m_ctl.properties.begin(); pi != m_worker.m_ctl.properties.end(); pi++) {
    OU::Property &pr = **pi;
    if (!pr.m_isParameter)
      continue;
    Param *p = NULL;
    for (unsigned n = 0; n < params.size(); n++)
      if (params[n].m_param && !strcasecmp(params[n].m_param->cname(), pr.cname())) {
	p = &params[n];
	break;
      }
    if (used) {
      if (!p)
	assert(pr.m_default);
      // Put out the Makefile value lines
      std::string val;
      // FIXME: this should be an RCC method
      fprintf(mf, "Param_%zu_%s_%s:=", nConfig, m_worker.m_implName, pr.cname());
      for (const char *cp = m_worker.rccValue(p ? p->m_value : *pr.m_default, val, pr); *cp; cp++) {
	if (*cp == '#' || (*cp == '\\' && !cp[1]))
	  fputc('\\', mf);
	fputc(*cp, mf);
      }
      fputs("\n", mf);
      fprintf(mf, "ParamMsg_%zu_%s_%s:=", nConfig, m_worker.m_implName, pr.cname());
      std::string uValue;
      if (p)
	uValue = p->m_uValue;
      else
	pr.m_default->unparse(uValue);
      for (const char *cp = uValue.c_str(); *cp; cp++) {
	if (*cp == '#' || (*cp == '\\' && !cp[1]))
	  fputc('\\', mf);
	fputc(*cp, mf);
      }
      fputs("\n", mf);
    }
    if (xf && p && !p->m_isDefault) {
      if (!nonDefault)
	fprintf(xf, "  <configuration id='%s'>\n", id.c_str());
      nonDefault = true;
      fprintf(xf, "    <parameter name='%s' value='", pr.cname());
      std::string xml;
      OU::encodeXmlAttrSingle(p->m_uValue, xml);
      fputs(xml.c_str(), xf);
      fputs("'/>\n", xf);
    }
  }
  if (xf && nonDefault)
    fputs("  </configuration>\n", xf);
}

void ParamConfig::
writeConstants(FILE *gf, Language lang) {
  fprintf(gf,
	  "%s This file sets values for constants that may be affected by parameter values\n",
	  hdlComment(lang));
  if (lang == VHDL) {
    fprintf(gf,
	    "library ocpi; use ocpi.all, ocpi.types.all;\n"
	    "package body %s_constants is\n"
	    "  -- Parameter property values as constants\n",
	    m_worker.m_implName);
  }
  for (unsigned n = 0; n < params.size(); n++) {
    Param &p = params[n];
    if (p.m_param == NULL)
      continue;
    const OU::Property &pr = *p.m_param;
    std::string value;
    if (lang == VHDL) {
      if (pr.m_baseType == OA::OCPI_String && pr.m_stringLength == 0) {
	// This string parameter has no string length since it will be set based on the
	// actual (possibly computed) value.
	assert(p.m_value.m_vt == &pr);
	pr.m_stringLength = p.m_value.maxStringLength();
      }
      std::string typeDecl, type;
      vhdlType(pr, typeDecl, type, false, true); // true for asserting all constants are known
      m_worker.hdlValue(pr.m_name, p.m_value, value, false, VHDL, true); // ditto
      fprintf(gf, "  constant %s : %s := %s;\n",
	      p.m_param->m_name.c_str(), type.c_str(), value.c_str());
    } else
      fprintf(gf, "  parameter [%zu:0] %s  = %s;\n", rawBitWidth(pr)-1, pr.m_name.c_str(),
	      verilogValue(p.m_value, value, true));
  }
  // This is static (not a port method) since it is needed when there are parameters with
  // no control interface.
  m_worker.emitPropertyAttributeConstants(gf, lang);
  for (unsigned i = 0; i < m_worker.m_ports.size(); i++)
    m_worker.m_ports[i]->emitInterfaceConstants(gf, lang);
  m_worker.emitSignalMacros(gf, lang);
  if (lang == VHDL)
    fprintf(gf, "end %s_constants;\n", m_worker.m_implName);
}

// Is the given configuration the same as this one?
bool ParamConfig::
equal(ParamConfig &other) {
  if (params.size() != other.params.size())
    return false;
  for (unsigned n = 0; n < params.size(); n++)
    if (params[n].m_uValue != other.params[n].m_uValue)
      return false;
  return true;
}

const char *Worker::
parseBuildFile(bool optional, bool *missing, const std::string *parent) {
  const char *err;
  std::string fname;
  if (missing)
    *missing = false;
  if (m_paramConfigs.size()) // file is not missing since it has already been parsed.
    return NULL;
  // We are only looking next to the OWD or "gen" below it
  // And it may be optional in any case.
  std::string dir;
  const char *slash = strrchr(m_file.c_str(), '/');
  if (slash)
    dir.assign(m_file.c_str(), (size_t)((slash + 1) - m_file.c_str()));
  // First look for the build file next to the OWD
  OU::format(fname, "%s%s.build", dir.c_str(), m_implName);
  if (!OS::FileSystem::exists(fname)) {
    OU::format(fname, "%s%s-build.xml", dir.c_str(), m_implName);
    if (!OS::FileSystem::exists(fname)) {
      // Next look for it in the gen/ below the OWD
      OU::format(fname, "%sgen/%s-build.xml", dir.c_str(), m_implName);
      if (!OS::FileSystem::exists(fname)) {
	// Finally look in the local gen subdir
	OU::format(fname, "gen/%s-build.xml", m_implName);
	if (!OS::FileSystem::exists(fname)) {
	  if (missing)
	    *missing = true;
	  return optional ? NULL :
	    OU::esprintf("Cannot find %s.build or %s-build.xml in worker directory or \"gen\" "
			 "subdirectory (%s)", m_implName, m_implName, dir.c_str());
	}
      }
    }
  }
  ezxml_t x;
  std::string xfile;
  std::string empty;
  if ((err = parseFile(fname.c_str(), parent ? *parent : empty, "build", &x, xfile, false, true, optional)))
    return err;
  if ((err = parseBuildXml(x, xfile)))
    return OU::esprintf("when processing build file \"%s\": %s", xfile.c_str(), err);
  return NULL;
}

const char *Worker::
parseBuildXml(ezxml_t x, const std::string &file) {
  const char *err;
  // This cannot be done during construction since the OWD isn't parsed then
  m_build.m_globalParams.params.resize(m_ctl.nParameters);
  if ((err = OE::checkAttrs(x, BUILD_ATTRS)) ||
      (err = OE::checkElements(x, "configuration", "parameter", NULL)))
    return err;
  // Establish default values (or multiple values) for parameters that apply to all
  // configurations that do not mention them
  for (ezxml_t px = ezxml_cchild(x, "parameter"); px; px = ezxml_cnext(px)) {
    if ((err = OE::checkAttrs(px, PARAM_ATTRS, NULL)))
      return err;
    std::string l_name;
    OU::Property *p;
    size_t nParam;
    if ((err = OE::getRequiredString(px, l_name, "name", "property")) ||
	(err = findParamProperty(l_name.c_str(), p, nParam, false)) ||
	(err = m_build.m_globalParams.params[nParam].parse(px, p, this, true)))
      return err;
    assert(nParam == p->m_paramOrdinal); // FIXME: get rid of nParam someday
  }
  m_build.m_globalParams.doDefaults(); // set unmentioned params to default values
  // There are three cases for configurations here:
  // 1. With IDs, which are defaulted against the property's default value when not specified.
  //    This was the original meaning and supports backward compatibility.
  //    If they are present, no configuration will be autogenerated
  // 2. Configurations without IDs, which are applied against the cross-product of supplied
  //    values in top-level "parameter" elements.
  //    An empty one will do cross-product against defaults
  //    An empty one may be required to generate defaults if configs with IDs are present.
  //    E.g. <configuration/>
  // 3. No configurations mean cross product of top level values.

  // First pass: is to populate the config array with explicitly id'd configs.
  for (ezxml_t cx = ezxml_cchild(x, "configuration"); cx; cx = ezxml_cnext(cx)) {
    bool haveId;
    size_t id;
    if ((err = OE::getNumber(cx, "id", &id, &haveId)))
      return err;
    if (!haveId)
      continue;
    ParamConfig *pc = new ParamConfig(*this);
    if ((err = pc->parse(cx, m_paramConfigs)))
      return err;
    pc->nConfig = id++;
    pc->doDefaults(); // configs with id have defaults filled in
    if (id > m_paramConfigs.size())
      m_paramConfigs.resize(id);
    m_paramConfigs[pc->nConfig] = pc; // this is a complete, defaulted configuration, as always
  }
  // Second pass:  generate cross-product configs based on unlabeled (partial) configs.
  for (ezxml_t cx = ezxml_cchild(x, "configuration"); cx; cx = ezxml_cnext(cx)) {
    bool haveId;
    size_t id;
    if ((err = OE::getNumber(cx, "id", &id, &haveId)))
      return err;
    if (haveId)
      continue; // handled it in previous pass
    ParamConfig *pc = new ParamConfig(m_build.m_globalParams);
    // Parse it sparsely (no defaulting), and then generate new ones recursively
    if ((err = pc->parse(cx, m_paramConfigs)) ||
	(err = doParam(*pc, m_ctl.properties.begin(), true, 0)))
      return err;
  }
  // Third pass: if we encountered no configurations at all, we generate default ones.
  if (!m_paramConfigs.size() &&
      (err = doParam(m_build.m_globalParams, m_ctl.properties.begin(), true, 0)))
    return err;
  return m_build.parse(x, file.c_str());
}

const char *Worker::
startBuildXml(FILE *&f) {
  const char *err;
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "", "-build", ".xml", NULL,
			f)))
    return err;
  fprintf(f, "<build>\n");
  return NULL;
}

// We have created an unnumbered configuration based on current conditions.
// Add it if it is new, as config number nConfig
const char *Worker::
addConfig(ParamConfig &info, bool fromXml) {
  // Check that the configuration we have is not already in the existing file
  ocpiDebug("Possibly adding parameter config. n=%zu", m_paramConfigs.size());
  for (unsigned n = 0; n < m_paramConfigs.size(); n++)
    if (m_paramConfigs[n]->equal(info)) {
      // Mark the old config as used so it will be written to the Makefile
      m_paramConfigs[n]->used = true;
      ocpiInfo("Parameter config %zu is skipped since it is identical to config %u",
	       m_paramConfigs.size(), n);
      return NULL;
    }
  info.nConfig = m_paramConfigs.size();
  ocpiDebug("Actually adding parameter config %zu", info.nConfig);
  OU::format(info.id, "%zu", info.nConfig);
  const char *err;
  // We have a new one, and if processing the raw XML from the Makefile,
  // we know we will be writing out the XML file
  // The XML will contain old and unused, old and used, and new and used.
  // The Makefile will contain old-that-were-used and new
  // By opening the xf file we are indicating there is something new to write
  // If were generating configs from the Makefile, prepare to write to XML
  if (!fromXml && !m_xmlFile && (err = startBuildXml(m_xmlFile)))
    return err;
  ParamConfig *newpc = new ParamConfig(info); // copy to new one
  newpc->used = true;
  assert(m_paramConfigs.size() <= info.nConfig);
  m_paramConfigs.resize(info.nConfig + 1);
  m_paramConfigs[info.nConfig] = newpc;
  return NULL;
}

const char *Worker::
doParam(ParamConfig &info, PropertiesIter pi, bool fromXml, unsigned nParam) {
  while (pi != m_ctl.properties.end() && !(*pi)->m_isParameter)
    pi++;
  if (pi == m_ctl.properties.end())
    addConfig(info, fromXml);
  else {
    OU::Property &prop = **pi;
    assert(nParam == prop.m_paramOrdinal);
    Param &p = info.params[nParam];
    pi++;
    nParam++;
    assert(p.m_uValues.size());
    for (unsigned n = 0; n < p.m_uValues.size(); n++) {
      const char *err;
      // This "finalize" might be changing the data type differently for each configuration
      if ((err = prop.finalize(info, "property", false)) ||
	  (err = prop.parseValue(p.m_uValues[n].c_str(), p.m_value, NULL, &info)))
	return err;
      p.m_value.unparse(p.m_uValue); // make the specific value for this configuration
      if ((err = doParam(info, pi, fromXml, nParam))) // recurse for later parameters
	return err;
    }
  }
  return NULL;
}

static const char *
addValue(Param &p, const char *start, const char *end, Values &values) {
  std::string sval(start, (size_t)(end - start));
  ocpiDebug("Adding a value for the %s parameter: \"%.*s\"",
	    p.m_param->m_name.c_str(), (unsigned)(end - start), start);
  OU::Value v;
  const char *err;
  if ((err = p.m_param->parseValue(sval.c_str(), v)))
    return err;
  v.unparse(sval);
  if (values.size() == 0)
    p.m_value = v; // set first value as THE value for now
  values.push_back(sval);
  return NULL;
 }

static const char *
addValues(Param &p, Values &values, bool hasValues, const char *content) {
  const char *err;
  // The text content is now undone with XML-quoting, but if there are
  // multiple values, we have to look for slashes unprotected by backslash.
  while (*content == '\n')
    content++;
  const char *end = content + strlen(content);
  while (end > content && end[-1] == '\n')
    end--;
  if (hasValues) {
    const char *ep;
    for (const char *cp = content; cp != end; cp = ep) {
      for (ep = cp; *ep != '\n' && *ep && *ep != '/'; ep++)
	if (*ep == '\\')
	  if (++ep == end)
	    break;
      if ((err = addValue(p, cp, ep, values)))
	return err;
      if (*ep && *ep != '\n')
	ep++;
    }
  } else if ((err = addValue(p, content, end, values)))
    return err;
  return NULL;
}

static const char *
parseRawParams(ezxml_t &x) {
  if ((x = ezxml_parse_fd(0)) && ezxml_error(x)[0])
    return OU::esprintf("XML Parsing error in raw parameters: %s", ezxml_error(x));
  if (!x || !x->name)
    return "Raw parameters could not be parsed as XML";
  if (strcasecmp(x->name, "parameters"))
    return OU::esprintf("Raw parameters input does not contain a 'parameters' element");
  return NULL;
}

const char *Worker::
writeParamFiles(FILE *mkFile, FILE *xmlFile) {
  // Write the makefile as well as the xml file if anything was added
  m_build.writeMakeVars(mkFile);
  fprintf(mkFile, "ParamConfigurations:=");
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    if (m_paramConfigs[n]->used)
      fprintf(mkFile, "%s%zu", n ? " " : "", n);
  fprintf(mkFile, "\nWorkerName_%s:=%s\n", m_fileName.c_str(), m_implName);
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    m_paramConfigs[n]->write(xmlFile, mkFile);
  if (xmlFile)
    fprintf(xmlFile, "</build>\n");
  ocpiDebug("xmlFile closing %p mkFile closing %p", xmlFile, mkFile);
  if ((xmlFile && fclose(xmlFile)) || (mkFile && fclose(mkFile)))
    return OU::esprintf("File close of parameter files failed.  Disk full?");
  return NULL;
}

// Take as input the list of parameters that are set in the Makefile or the
// environment (i.e. something a human wrote and might have errors).
// Check against the actual properties of the worker, checking the values
// using the type-aware parser.  Return in two output files sets of valid parameter
// settings that are ready/convenient/appropriate for tools to supply to the
// language processors.
// One output file is XML, which is updated in place to maintain the
// existing mappings between configuration IDs and parameter values,
// adding any new configurations under new IDs.
// The other is something that the Makefile can include
const char *Worker::
emitToolParameters() {
  ezxml_t x;
  const char *err;
  FILE *mkFile;
  // If we are an emulator, then we use the build configurations from the emulatee.
  if (m_emulate) {
    FILE *f;
    startBuildXml(f);
    return emitMakefile(f);
  }
  if ((m_paramConfigs.size() == 0 && (err = parseBuildFile(true))) ||
      (err = parseRawParams(x)) ||
      (err = openOutput(m_fileName.c_str(), m_outDir, "", "", ".mk", NULL, mkFile)))
    return err;
  ParamConfig info(*this);                          // Current config for generating them
  info.params.resize(m_ctl.nParameters);
  for (ezxml_t px = ezxml_cchild(x, "parameter"); px; px = ezxml_cnext(px)) {
    std::string l_name;
    bool hasValues;
    OU::Property *p;
    size_t nParam;

    if ((err = OE::getRequiredString(px, l_name, "name")) ||
	(err = OE::getBoolean(px, "values", &hasValues)))
      return err;
    if ((err = findParamProperty(l_name.c_str(), p, nParam))) {
      fprintf(stderr,
	      "Warning: parameter '%s' ignored due to: %s\n", l_name.c_str(), err);
      continue;
    }
    if ((err = p->finalize(info, "property", false)))
      return err;
    Param &param = info.params[nParam];
    param.setProperty(p, this);  // record that we have set a value
    param.m_value.setType(*p);
    if ((err = addValues(info.params[nParam], info.params[nParam].m_uValues, hasValues,
			 ezxml_txt(px))))
      return err;
  }
  // Fill in default values when there are no values specified for a given parameter.
  // i.e. for this parameter, make the single value for all configs the default value
  Param *par = &info.params[0];
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if ((*pi)->m_isParameter) {
      par->setProperty(*pi, this);
      if (par->m_uValues.empty()) {
	if (!(*pi)->m_default)
	  return OU::esprintf("The parameter property '%s' has no value and no default value",
			      (*pi)->m_name.c_str());
	std::string sval;
	(*pi)->m_default->unparse(sval);
	par->m_uValues.push_back(sval);
	par->m_attributes.resize(par->m_attributes.size() + 1);
	par->m_isDefault = true;
      }
      par++;
    }
  // Generate all configurations, merging with existing, keeping existing ones
  // in the same position.
  if ((err = doParam(info, m_ctl.properties.begin(), false, 0)))
    return err;
  // Force an empty build file
  //  if (!m_xmlFile && m_paramConfigs.size() == 0 && (err = startBuildXml(m_xmlFile)))
  //    return err;
  err = writeParamFiles(mkFile, m_xmlFile);
  m_xmlFile = NULL;
  return err;
}

// Based on worker xml, read the <worker>.build, and emit the
// gen/<worker>-params.mk
const char *Worker::
emitMakefile(FILE *xmlFile) {
  const char *err;
  FILE *mkFile;
  if ((!m_emulate && (err = parseBuildFile(false))) ||
      (err = openOutput(m_fileName.c_str(), m_outDir, "", "", ".mk", NULL, mkFile)))
    return err;
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    m_paramConfigs[n]->used = true;
  return writeParamFiles(mkFile, xmlFile);
}
// Based on worker xml, read the <worker>-build.xml, and emit the generics file
const char *Worker::
emitHDLConstants(size_t config, bool other) {
  const char *err;
  FILE *f;
  Language lang = other ? (m_language == VHDL ? Verilog : VHDL) : m_language;
  if (//(err = parseBuildFile(false)) ||
      (err = openOutput("generics", m_outDir, "", "", lang == VHDL ? VHD : ".vh", NULL, f)))
    return err;
  if (config >= m_paramConfigs.size() || m_paramConfigs[config] == NULL)
    return OU::esprintf("Invalid parameter configuration for VHDL generics: %zu", config);
  // NOTE: resolveExpressions is called earlier when the worker is constructed and then called again here
  // to apply this particular config
  //  if (!(err = resolveExpressions(*m_paramConfigs[config])) && !(err = finalizeProperties()))
    m_paramConfigs[config]->writeConstants(f, lang);
  return fclose(f) ?
    OU::esprintf("File close of VHDL generics file failed.  Disk full?") : NULL;
}

// Given assembly instance properties or a parameter configuration,
// establish the parameter values for this worker.
// Note this worker will then be parameter-value-specific.
// If instancePVs is NULL, use paramconfig
const char *Worker::
setParamConfig(OU::Assembly::Properties *instancePVs, size_t paramConfig,
	       const std::string *parent) {
  const char *err;
  // So we have parameter configurations
  // FIXME: we could cache this parsing in one place, but workers can still be
  // parameterized by xml attribute values, so it can't simply be cached in a Worker object.
  if ((err = parseBuildFile(paramConfig == 0, NULL, parent)))
    return err;
  if (m_paramConfigs.size() == 0) {
    // FIXME: check whether it is ever possible to have no paramconfigs any more...
    // No parameter configurations, so we just do error checking
    if (paramConfig != 0)
      return OU::esprintf("Worker '%s' has no parameter configurations, but config %zu specified",
			  m_implName, paramConfig);
    if (instancePVs && instancePVs->size()) {
      OU::Assembly::Property *ap = &(*instancePVs)[0];
      for (unsigned nn = 0; nn < instancePVs->size(); nn++, ap++)
	if (ap->m_hasValue) {
	  OU::Property *p = findProperty(ap->m_name.c_str());
	  if (!p || !p->m_isParameter)
	    return OU::esprintf("Worker \"%s\" has no parameter property named \"%s\"",
				m_implName, ap->m_name.c_str());
	  if (p->m_default) {
	    std::string defValue, newValue;
	    p->m_default->unparse(defValue);
	    OU::Value ipv;
	    ipv.setType(*p);
	    if (!(err = ipv.parse(ap->m_value.c_str()))) {
	      ipv.unparse(newValue);
	      if (defValue != newValue)
		err = "value doesn't match default, and no other choices exist";
	    }
	    if (err)
	      return OU::esprintf("Bad value \"%s\" (default is \"%s\", new is \"%s\") for parameter \"%s\" for worker \"%s\": %s",
				  ap->m_value.c_str(), defValue.c_str(), newValue.c_str(),
				  p->m_name.c_str(), m_implName, err);
	  }
	}
    }
    return NULL;
  }
  if (paramConfig >= m_paramConfigs.size())
    return OU::esprintf("Parameter configuration %zu exceeds available configurations (%zu)",
			paramConfig, m_paramConfigs.size());
  if (!instancePVs || instancePVs->size() == 0) {
    m_paramConfig = m_paramConfigs[paramConfig];
    return NULL;
  }
  size_t low, high;
  if (paramConfig)
    low = high = paramConfig;
  else
    low = 0, high = m_paramConfigs.size() - 1;
  // At this point we know we have configurations and values to match against them
  // Scan the configs until one matches. FIXME: use scoring via selection expression...
  for (size_t n = low; n <= high; n++) {
    ParamConfig *pc = m_paramConfigs[n];
    OU::Assembly::Property *ap = &(*instancePVs)[0];
    for (unsigned nn = 0; nn < instancePVs->size(); nn++, ap++)
      if (ap->m_hasValue) {
	Param *p = &pc->params[0];
	for (unsigned nnn = 0; nnn < pc->params.size(); nnn++, p++)
	  if (!strcasecmp(ap->m_name.c_str(), p->m_param->m_name.c_str())) {
	    OU::Value apValue;
	    if ((err = p->m_param->parseValue(ap->m_value.c_str(), apValue)))
	      return err;
	    std::string apString;
	    apValue.unparse(apString); // to get canonicalized value of APV
	    if (apString != p->m_uValue)
	      goto nextConfig; // a mismatch - this paramconfig can't used
	    break;
	  }
      }
    m_paramConfig = pc;
    break;
  nextConfig:;
  }
  if (!m_paramConfig)
    return
      OU::esprintf("No built parameter configuration for worker \"%s\" matches requested "
		   "parameter values", cname());
  return NULL;
}

// There is probably a more clever way to do this, but we need decent warnings.
static const char *
onlyExclude(const char *thing,
	    const char *baseFile, OrderedStringSet &baseOnly, OrderedStringSet &baseExclude,
	    const char *newFile, const OrderedStringSet &newOnly,
	    const OrderedStringSet &newExclude) {
  if (newOnly.size()) {
    if (baseOnly.size()) {
      for (auto it = newOnly.begin(); it != newOnly.end(); ++it)
	if (baseOnly.find(*it) == baseOnly.end())
	  return OU::esprintf("the %s \"%s\" in only%ss in \"%s\" is not found in the only%ss list in \"%s\"",
			      thing, (*it).c_str(), thing, baseFile, thing, newFile);
      for (auto it = newOnly.begin(); it != newOnly.end(); ++it) {
	auto baseIt = baseOnly.find(*it);
	if (baseIt != baseOnly.end())
	  baseOnly.erase(baseIt);
      }
    } else if (baseExclude.size()) {
      for (auto it = newOnly.begin(); it != newOnly.end(); ++it)
	if (baseExclude.find(*it) != baseOnly.end())
	  return OU::esprintf("the %s \"%s\" in only%ss in \"%s\" is excluded in \"%s\"",
			      thing, (*it).c_str(), thing, newFile, baseFile);
    } else
      baseOnly = newOnly;
  } else if (newExclude.size()) {
    if (baseOnly.size())
      for (auto it = newExclude.begin(); it != newExclude.end(); ++it) {
	auto baseIt = baseOnly.find(*it);
	if (baseIt != baseOnly.end())
	  baseOnly.erase(baseIt);
	else
	  fprintf(stderr, "the %s \"%s\" in exclude%ss in \"%s\" is not present in the only%ss list in \"%s\"",
		  thing, (*it).c_str(), thing, newFile, thing, baseFile);
      }
    else if (baseExclude.size())
      for (auto it = newExclude.begin(); it != newExclude.end(); ++it)
	baseExclude.push_back(*it);
  } else
    baseExclude = newExclude;
  return NULL;
}

Build::Build(Worker &w) : m_worker(w), m_globalParams(w) {
}
// Parse top-level attributes for the owd or build xml.  If buildFile arg not NULL, we are parsing
// the build file which is parsed after the owd
const char *Build::
parse(ezxml_t x, const char *buildFile) {
  Model m = m_worker.m_model;
  // Expand and make sure they are valid platform names
  const char
    *err,
    *aonly = ezxml_cattr(x, "onlyplatforms"),
    *aexclude = ezxml_cattr(x, "excludeplatforms");
  // For compatibility: "only" should be deprecated
  if (!aonly)
    aonly = ezxml_cattr(x, "only");
  if (aonly && aexclude)
    return "onlyPlatforms and excludePlatforms cannot both be used";
  if (buildFile) {
    OrderedStringSet only, exclude;
    if ((aonly && (err = getPlatforms(aonly, only, m))) ||
	(aexclude && (err = getPlatforms(aexclude, exclude, m))) ||
	(err = onlyExclude("platform", m_worker.m_file.c_str(), m_onlyPlatforms, m_excludePlatforms,
			   buildFile, only, exclude)))
      return err;
  } else if ((aonly && (err = getPlatforms(aonly, m_onlyPlatforms, m))) ||
	     (aexclude && (err = getPlatforms(aexclude, m_excludePlatforms, m))))
    return err;
  aonly = ezxml_cattr(x, "onlytargets");
  aexclude = ezxml_cattr(x, "excludetargets");
  if ((m_onlyPlatforms.size() || m_excludePlatforms.size()) &&
      (aonly || aexclude))
    return "targets and platforms cannot both be specified";
  if (buildFile) {
    OrderedStringSet only, exclude;
    if ((aonly && (err = getTargets(aonly, only, m))) ||
	(aexclude && (err = getTargets(aexclude, exclude, m))) ||
	(err = onlyExclude("target", m_worker.m_file.c_str(), m_onlyTargets, m_excludeTargets,
			   buildFile, only, exclude)))
      return err;
  } else if ((aonly && (err = getTargets(aonly, m_onlyTargets, m))) ||
      (aexclude && (err = getTargets(aexclude, m_excludeTargets, m))))
    return err;
  if (buildFile)
    return NULL;
  // Everything from here down is OWD-only
  for (OU::TokenIter ti(ezxml_cattr(x, "sourcefiles")); ti.token(); ti.next()) {
#if 0  // this is only sensible when reading the OWD in the worker's own directory
    // FIXME: make this conditional on that case some how.
    std::string file(OF::joinNames(OF::directoryName(m_worker.m_file), ti.token()));
    if (!OS::FileSystem::exists(file))
      fprintf(stderr, "Warning:  the source file: \"%s\" does not exist\n", file.c_str());
#endif
    m_sourceFiles.push_back(ti.token());
  }
  for (OU::TokenIter ti(ezxml_cattr(x, "libraries")); ti.token(); ti.next())
    if (m != HdlModel)
      return "Invalid \"libraries\" attribute:  worker model is not HDL";
    else if ((err = getHdlPrimitive(ti.token(), "library", m_libraries)))
      return err;
  for (OU::TokenIter ti(ezxml_cattr(x, "cores")); ti.token(); ti.next())
    if (m != HdlModel)
      return "Invalid \"cores\" attribute:  worker model is not HDL";
    else if ((err = getHdlPrimitive(ti.token(), "core", m_cores)))
      return err;
  bool isDir;
  // xmlincudedirs is handled specially in owd parsing for bootstrap reasons
  for (OU::TokenIter ti(ezxml_cattr(x, "includedirs")); ti.token(); ti.next()) {
    if (OS::FileSystem::exists(ti.token(), &isDir) && isDir)
      m_includeDirs.push_back(ti.token());
    else
      return OU::esprintf("The include directory: \"%s\" is not a directory", ti.token());
  }
  for (OU::TokenIter ti(ezxml_cattr(x, "componentlibraries")); ti.token(); ti.next())
#if 0 // note: componentlibraries can be used for other xml files like specs etc.
    if (!m_worker.m_isSlave && !m_worker.m_emulate)
      return OU::esprintf("componentlibraries only valid on workers with slaves or emulators");
#endif
    if ((err = getComponentLibrary(ti.token(), m_componentLibraries)))
      return err;
  std::string prereqs;
  if ((err = getPrereqDir(prereqs)))
    return err;
  std::string path;
  for (OU::TokenIter ti(ezxml_cattr(x, "staticprereqlibs")); ti.token(); ti.next()) {
    if (m != RccModel)
      return "Invalid \"StaticPrereqLibs\" attribute:  worker model is not RCC";
    path = prereqs + "/" + ti.token();
    if (OS::FileSystem::exists(path, &isDir) && isDir)
      m_staticPrereqLibs.push_back(ti.token());
    else
      return
	OU::esprintf("The prerequisite library \"%s\"  is not found/installed.", ti.token());
  }
  for (OU::TokenIter ti(ezxml_cattr(x, "dynamicprereqlibs")); ti.token(); ti.next()) {
    if (m != RccModel)
      return "Invalid \"DynamicPrereqLibs\" attribute:  worker model is not RCC";
    path = prereqs + "/" + ti.token();
    if (OS::FileSystem::exists(path, &isDir) && isDir)
      m_dynamicPrereqLibs.push_back(ti.token());
    else
      return
	OU::esprintf("The prerequisite library \"%s\"  is not found/installed.", ti.token());
  }
  return NULL;
}
static void writeVar(FILE *f, const char *var, OrderedStringSet &vals) {
  if (vals.empty())
    return;
  fprintf(f,"$(call OcpiCheckVars,%s)\n%s:=", var, var);
  for (auto i = vals.begin(); i != vals.end(); ++i)
    fprintf(f, "%s%s", i == vals.begin() ? "" : " ", (*i).c_str());
  fprintf(f, "\n");
}
void Build::
writeMakeVars(FILE *mkf) {
  writeVar(mkf, "OnlyPlatforms", m_onlyPlatforms);
  writeVar(mkf, "ExcludePlatforms", m_excludePlatforms);
  writeVar(mkf, "OnlyTargets", m_onlyTargets);
  writeVar(mkf, "ExcludeTargets", m_excludeTargets);
  writeVar(mkf, "SourceFiles", m_sourceFiles);
  writeVar(mkf, "Libraries", m_libraries);
  writeVar(mkf, "XmlIncludeDirs", m_xmlIncludeDirs);
  writeVar(mkf, "IncludeDirs", m_includeDirs);
  writeVar(mkf, "ComponentLibraries", m_componentLibraries);
  writeVar(mkf, "Cores", m_cores);
  writeVar(mkf, "RccStaticPrereqLibs", m_staticPrereqLibs);
  writeVar(mkf, "RccDynamicPrereqLibs", m_dynamicPrereqLibs);
}
