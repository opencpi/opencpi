// Parameter processing
#include <assert.h>
#include <strings.h>
#include "OcpiOsFileSystem.h"
#include "parameters.h"
#include "wip.h"
#include "hdl.h"
namespace OU=OCPI::Util;

const char *Worker::
addParamConfigSuffix(std::string &s) {
  if (m_paramConfig && m_paramConfig->nConfig)
    OU::formatAdd(s, "_c%zu", m_paramConfig->nConfig);
  return s.c_str();
}

const char *Worker::
findParamProperty(const char *name, OU::Property *&prop, size_t &nParam, bool includeInitial) {
  size_t n = 0;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if (!strcasecmp((*pi)->m_name.c_str(), name)) {
      prop = *pi;
      if (prop->m_isParameter || (includeInitial && prop->m_isWritable)) {
	nParam = n;
	return NULL;
      } else
	return OU::esprintf("The '%s' property is not a parameter", name);
    } else if ((*pi)->m_isParameter || (includeInitial && (*pi)->m_isWritable))
      n++;
  return OU::esprintf("Parameter property not found: '%s'", name);
}

Param::Param() : m_valuesType(NULL), m_param(NULL), m_isDefault(false), m_worker(NULL), 
		 m_isTest(false) {}

// the "global" argument is true when this parameter has a global setting across many other
// configurations and so it cannot be given a value if it is dependent on other parameters for
// its type (e.g. array dimensions)
const char *Param::
parse(ezxml_t px, const OU::Property &p, bool global) {
  std::string xValue;
  const char *err;
  const char
    *generate = ezxml_cattr(px, "generate"),
    *value = ezxml_cattr(px, "value"),
    *values = ezxml_cattr(px, "values"),
    *valueFile = ezxml_cattr(px, "valueFile"),
    *valuesFile = ezxml_cattr(px, "valuesFile");
  unsigned n = (value ? 1 : 0) + (values ? 1 : 0) + (valueFile ? 1 : 0) + (valuesFile ? 1 : 0) +
    (generate ? 1 : 0);
  if (n != 1)
    return OU::esprintf("Exactly one attribute must be specified among: "
			"value, values, valuefile, valuesFile, or (for tests) generate");
  if (generate) {
    m_generate = generate;
    return NULL;
  } else if (p.m_usesParameters && global)
    return OU::esprintf("Property \"%s\" must be generated since its type depends on parameters",
			p.cname());
  std::string fileValue;
  if (valueFile) {
    if ((err = (p.needsNewLineBraces() ?
		OU::file2String(fileValue, valueFile, "{", "},{", "}") :
		OU::file2String(fileValue, valueFile, ','))))
      return err;
    value = fileValue.c_str();
  } else if (valuesFile) {
    if ((err = (p.needsComma() ? 
		OU::file2String(fileValue, valuesFile, "{", "},{", "}") :
		OU::file2String(fileValue, valuesFile, ','))))
      return err;
    values = fileValue.c_str();
  }
  m_param = &p;
  m_isDefault = false;
  m_uValues.clear(); // forget previous values since we might be overriding them
  if (values) {
    if (!m_valuesType) {
      m_valuesType = &p.sequenceType();
      m_valuesType->m_default = new OU::Value(*m_valuesType);
    }
    if ((err = m_valuesType->m_default->parse(values, NULL, false, NULL)))
      return err;
    m_valuesType->m_default->unparse(m_uValue);
    m_uValues.resize(m_valuesType->m_default->m_nElements);
    for (unsigned n = 0; n < m_valuesType->m_default->m_nElements; n++)
      m_valuesType->m_default->elementUnparse(*m_valuesType->m_default, m_uValues[n], n, false,
					      '\0', false, *m_valuesType->m_default);
  } else {
    OU::Value newValue;
    if ((err = p.parseValue(value, newValue)))
      return err;
    newValue.unparse(m_uValue);
    if (p.m_default) {
      std::string defValue;
      p.m_default->unparse(defValue);
      if (defValue == m_uValue) {
	m_isDefault = true;
	m_value = *p.m_default;
	return NULL;
      }
    }
    m_uValues.resize(1);
    m_uValues[0] = m_uValue;
    m_value = newValue;
  }
  return NULL;
}

ParamConfig::
ParamConfig(Worker &w) : m_worker(w), nConfig(0), used(false) {
  params.resize(w.m_ctl.properties.size());
}

ParamConfig::
ParamConfig(const ParamConfig &other)
  : m_worker(other.m_worker), params(other.params), id(other.id), nConfig(other.nConfig),
    used(other.used) {
}

const char *ParamConfig::
parse(ezxml_t cx, const ParamConfigs &configs, bool includeInitial) {
  const char *err;
  if ((err = OE::getNumber(cx, "id", &nConfig, NULL, 0, false, true)))
    return err;
  for (unsigned n = 0; n < configs.size(); n++)
    if (configs[n]->nConfig == nConfig)
      return OU::esprintf("Duplicate configuration id %zu in build file", nConfig);
  OU::format(id, "%zu", nConfig);
  // Note that this resize when including initial props, will overallocate, e.g. volatiles.
  params.resize(includeInitial ? m_worker.m_ctl.properties.size() : 
		m_worker.m_ctl.nParameters);
  for (ezxml_t px = ezxml_cchild(cx, "parameter"); px; px = ezxml_next(px)) {
    std::string name;
    size_t nParam;
    OU::Property *p;
    if ((err = OE::getRequiredString(px, name, "name")) ||
	(err = m_worker.findParamProperty(name.c_str(), p, nParam)) ||
	(err = p->finalize(*this, "property", false)) ||
	(err = params[nParam].parse(px, *p)))
      return err;
  }
  // Fill in the default values that were not mentioned
  size_t n = 0;
  for (PropertiesIter pi = m_worker.m_ctl.properties.begin(); pi != m_worker.m_ctl.properties.end(); pi++)
    if ((*pi)->m_isParameter || (includeInitial && (*pi)->m_isWritable)) {
      if (!params[n].m_param) {
	params[n].m_param = *pi;
	if ((*pi)->m_default)
	  params[n].m_value = *(*pi)->m_default; // assignment operator
	else
	  params[n].m_value.setType(**pi);        // blank default value
	params[n].m_value.unparse(params[n].m_uValue);
	params[n].m_isDefault = true;
      }
      n++;
    }
  return NULL;
}

const char *ParamConfig::
getValue(const char *sym, OU::ExprValue &val) const {
  OU::Property *prop;
  size_t nParam; // FIXME not needed since properties have m_paramOrdinal?
  const char *err;
  if ((err = m_worker.findParamProperty(sym, prop, nParam)) ||
      (err = extractExprValue(*prop, params[nParam].m_value, val)))
    return err;
  return NULL;
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
  if (xf)
    fprintf(xf, "  <configuration id='%s'>\n", id.c_str());
  for (unsigned n = 0; n < params.size(); n++) {
    Param &p = params[n];
    if (p.m_param == NULL) {
      // This is a new parameter that was not in this (existing) param config.
      continue;
    }
    if (used) {
      // Put out the Makefile value lines
      std::string val;
      if (m_worker.m_model == HdlModel) {
	std::string typeDecl, type;
	vhdlType(*p.m_param, typeDecl, type);
	//	if (typeDecl.length())
	//	  fprintf(mf, "ParamVHDLtype_%zu_%s:=type %s_t is %s;\n",
	//		  nConfig, p.m_param->m_name.c_str(), p.m_param->m_name.c_str(), typeDecl.c_str());
	fprintf(mf, "ParamVHDL_%zu_%s:=constant %s : ",
		nConfig, p.m_param->m_name.c_str(), p.m_param->m_name.c_str());
	//	if (typeDecl.length())
	//	  fprintf(mf, "%s_t", p.m_param->m_name.c_str());
	//	else
	fprintf(mf, "%s", type.c_str());
	fprintf(mf, " := ");
	for (const char *cp = m_worker.hdlValue(p.m_param->m_name, p.m_value, val, false, VHDL);
	     *cp; cp++) {
	  if (*cp == '#' || (*cp == '\\' && !cp[1]))
	    fputc('\\', mf);
	  fputc(*cp, mf);
	}
	fputs("\n", mf);
	fprintf(mf, "ParamVerilog_%zu_%s:=parameter [%zu:0] %s = ",
		nConfig, p.m_param->m_name.c_str(), rawBitWidth(*p.m_param) - 1,
		p.m_param->m_name.c_str());
	for (const char *cp = m_worker.hdlValue(p.m_param->m_name, p.m_value, val, true, Verilog);
	     *cp; cp++) {
	  if (*cp == '#' || (*cp == '\\' && !cp[1]))
	    fputc('\\', mf);
	  fputc(*cp, mf);
	}
	fputs("\n", mf);
      } else {
	fprintf(mf, "Param_%zu_%s:=", nConfig, p.m_param->m_name.c_str());
	for (const char *cp = m_worker.paramValue(*p.m_param, p.m_value, val); *cp; cp++) {
	  if (*cp == '#' || (*cp == '\\' && !cp[1]))
	    fputc('\\', mf);
	  fputc(*cp, mf);
	}
	fputs("\n", mf);
      }
      fprintf(mf, "ParamMsg_%zu_%s:=", nConfig, p.m_param->m_name.c_str());
      for (const char *cp = p.m_uValue.c_str(); *cp; cp++) {
	if (*cp == '#' || (*cp == '\\' && !cp[1]))
	  fputc('\\', mf);
	fputc(*cp, mf);
      }
      fputs("\n", mf);
    }
    if (xf) {
      fprintf(xf, "    <parameter name='%s' value='", p.m_param->m_name.c_str());
      std::string xml;
      OU::encodeXmlAttrSingle(p.m_uValue, xml);
      fputs(xml.c_str(), xf);
      fputs("'/>\n", xf);
    }
  }
  if (xf)
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
      std::string typeDecl, type;
      vhdlType(pr, typeDecl, type);
      m_worker.hdlValue(pr.m_name, p.m_value, value, false, VHDL);
      fprintf(gf, "  constant %s : %s := %s;\n",
	      p.m_param->m_name.c_str(), type.c_str(), value.c_str());
    } else
      fprintf(gf, "  parameter [%zu:0] %s  = %s;\n", rawBitWidth(pr)-1, pr.m_name.c_str(),
	      verilogValue(p.m_value, value));
  }
  // This is static (not a port method) since it is needed when there are parameters with
  // no control interface.
  m_worker.emitPropertyAttributeConstants(gf, lang);
  for (unsigned i = 0; i < m_worker.m_ports.size(); i++)
    m_worker.m_ports[i]->emitInterfaceConstants(gf, lang);
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
parseBuildFile(bool optional) {
  const char *err;
  std::string fname;
  if (m_paramConfigs.size())
    return NULL;
  // We are only looking next to the OWD or "gen" below it
  // And it may be optional in any case.
  std::string dir;
  const char *slash = strrchr(m_file.c_str(), '/');
  if (slash)
    dir.assign(m_file.c_str(), (slash + 1) - m_file.c_str());
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
	if (!OS::FileSystem::exists(fname))
	  return optional ? NULL :
	    OU::esprintf("Cannot find %s.build or %s-build.xml in worker directory or \"gen\" "
			 "subdirectory", m_implName, m_implName);
      }
    }
  }
  ezxml_t x;
  std::string empty;
  if ((err = parseFile(fname.c_str(), empty, "build", &x, empty, false, true, optional)))
    return err;
  for (ezxml_t cx = ezxml_cchild(x, "configuration"); cx; cx = ezxml_next(cx)) {
    ParamConfig *pc = new ParamConfig(*this);
    if ((err = pc->parse(cx, m_paramConfigs)))
      return err;
    m_paramConfigs.push_back(pc); // FIXME...emplace_back for C++11
  }
  return NULL;
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

// We have created a configuration based on current conditions.
// Add it if it is new
const char *Worker::
addConfig(ParamConfig &info, size_t &nConfig) {
  // Check that the configuration we have is not already in the existing file
  ocpiDebug("Possibly adding parameter config. n=%zu size %zu", nConfig, m_paramConfigs.size());
  for (unsigned n = 0; n < m_paramConfigs.size(); n++)
    if (m_paramConfigs[n]->equal(info)) {
      // Mark the old config as used so it will be written to the Makefile
      m_paramConfigs[n]->used = true;
      return NULL;
    }
  info.nConfig = nConfig++;
  ocpiDebug("Actually adding parameter config %zu", nConfig);
  OU::format(info.id, "%zu", info.nConfig);
  // We have a new one, so we know we will be writing out the XML file
  // The XML will contain old and unused, old and used, and new and used.
  // The Makefile will contain old-that-were-used and new
  // By opening the xf file we are indicating there is something new
  const char *err;
  if (!m_xmlFile && (err = startBuildXml(m_xmlFile)))
      return err;
  ParamConfig *newpc = new ParamConfig(info);
  newpc->used = true;
  m_paramConfigs.push_back(newpc);
  return NULL;
}

const char *Worker::
doParam(ParamConfig &info, PropertiesIter pi, unsigned nParam, size_t &nConfig) {
  while (pi != m_ctl.properties.end() && !(*pi)->m_isParameter)
    pi++;
  if (pi == m_ctl.properties.end())
    addConfig(info, nConfig);
  else {
    Param &p = info.params[nParam];
    OU::Property &prop = **pi;
    pi++;
    nParam++;
    for (unsigned n = 0; n < p.m_values.size(); n++) {
      const char *err;
      if ((err = prop.finalize(info, "property", false)) ||
	  (err = prop.parseValue(p.m_values[n].c_str(), p.m_value, NULL, &info)))
	return err;
      p.m_value.unparse(p.m_uValue); // make the canonical value
      if ((err = doParam(info, pi, nParam, nConfig)))
	return err;
    }
  }
  return NULL;
}

static void // const char *
addValue(OU::Property &p, const char *start, const char *end, Values &values) {
#if 0
   OU::Value *v = new OU::Value();
   const char *err = p.parseValue(start, *v, end);
   if (err)
     delete v;
   else {
#endif
     std::string sval(start, end - start);
     values.push_back(sval);
     ocpiDebug("Adding a value for the %s parameter: \"%.*s\"",
	       p.m_name.c_str(), (unsigned)(end - start), start);
#if 0
   }
   return err;
#endif
 }

static void
addValues(OU::Property &p, Values &values, bool hasValues, const char *content) {
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
      addValue(p, cp, ep, values);
      if (*ep && *ep != '\n')
	ep++;
    }
  } else
    addValue(p, content, end, values);
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
  fprintf(mkFile, "ParamConfigurations:=");
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    if (m_paramConfigs[n]->used)
      fprintf(mkFile, "%s%zu", n ? " " : "", n);
  fprintf(mkFile, "\n");
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
  if ((m_paramConfigs.size() == 0 && (err = parseBuildFile(true))) ||
      (err = parseRawParams(x)) ||
      (err = openOutput(m_fileName.c_str(), m_outDir, "", "-params", ".mk", NULL, mkFile)))
    return err;
  ParamConfig info(*this);                          // Current config for generating them
  size_t nConfig = m_paramConfigs.size();
  info.params.resize(m_ctl.nParameters);
  // This loop has parameters as the outer loop, and possible values for each parameter
  // as the inner loop.
  for (ezxml_t px = ezxml_cchild(x, "parameter"); px; px = ezxml_next(px)) {
    std::string name;
    bool hasValues;
    OU::Property *p;
    size_t nParam;

    if ((err = OE::getRequiredString(px, name, "name")) ||
	(err = OE::getBoolean(px, "values", &hasValues)))
      return err;
    if ((err = findParamProperty(name.c_str(), p, nParam)))
      fprintf(stderr,
	      "Warning: parameter '%s' ignored due to: %s\n", name.c_str(), err);
    else
      addValues(*p, info.params[nParam].m_values, hasValues, ezxml_txt(px));
  }
  // Fill in default values when there are no values specified for a given parameter.
  // i.e. for this parameter, make the single value for all configs the default value
  Param *par = &info.params[0];
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if ((*pi)->m_isParameter) {
      par->m_param = *pi;
      if (par->m_values.empty()) {
	if (!(*pi)->m_default)
	  return OU::esprintf("The parameter property '%s' has no value and no default value",
			      (*pi)->m_name.c_str());
	//	assert((*pi)->m_default);
	std::string sval;
	(*pi)->m_default->unparse(sval);
	par->m_values.push_back(sval);
      }
      par++;
    }
  // Generate all configurations, merging with existing, keeping existing ones
  // in the same position.
  if ((err = doParam(info, m_ctl.properties.begin(), 0, nConfig)))
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
emitMakefile() {
  const char *err;
  FILE *mkFile;
  if ((err = parseBuildFile(false)) ||
      (err = openOutput(m_fileName.c_str(), m_outDir, "", "-params", ".mk", NULL, mkFile)))
    return err;
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    m_paramConfigs[n]->used = true;
  return writeParamFiles(mkFile, NULL);  
}
// Based on worker xml, read the <worker>.build, and emit the generics file
const char *Worker::
emitHDLConstants(size_t config, bool other) {
  const char *err;
  FILE *f;
  Language lang = other ? (m_language == VHDL ? Verilog : VHDL) : m_language;
  if ((err = parseBuildFile(false)) ||
      (err = openOutput("generics", m_outDir, "", "", lang == VHDL ? VHD : ".vh", NULL, f)))
    return err;
  if (config >= m_paramConfigs.size() || m_paramConfigs[config] == NULL)
    return OU::esprintf("Invalid parameter configuration for VHDL generics: %zu", config);
  m_paramConfigs[config]->writeConstants(f, lang);
  return fclose(f) ?
    OU::esprintf("File close of VHDL generics file failed.  Disk full?") : NULL;
}

#if 0
const char *Worker::
getParamConfig(const char *id, const ParamConfig *&config) {
  const char *err = parseConfigFile();
  if (!err) {
    const ParamConfig *pc = &m_paramConfigs[0];
    for (size_t n = m_paramConfigs.size(); n; n--, pc++)
      if (!strcasecmp(id, pc->id.c_str())) {
	config = pc;
	return NULL;
      }
    err = OU::esprintf("No parameter configuration found with id: '%s'", id);
  }
  return err;
}
#endif
// Given assembly instance properties or a parameter configuration,
// establish the parameter values for this worker.
// Note this worker will then be parameter-value-specific.
// If instancePVs is NULL, use paramconfig
const char *Worker::
setParamConfig(OU::Assembly::Properties *instancePVs, size_t paramConfig) {
  const char *err;
  // So we have parameter configurations
  // FIXME: we could cache this parsing in one place, but workers can still be
  // parameterized by xml attribute values, so it can't simply be cached in a Worker object.
  if ((err = parseBuildFile(paramConfig == 0)))
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
