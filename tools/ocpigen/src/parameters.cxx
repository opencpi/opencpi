// Parameter processing
#include <assert.h>
#include <strings.h>
#include "wip.h"

const char *Worker::
findParamProperty(const char *name, OU::Property *&prop, size_t &nParam) {
  size_t n = 0;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if (!strcasecmp((*pi)->m_name.c_str(), name)) {
      prop = *pi;
      if (prop->m_isParameter) {
	nParam = n;
	return NULL;
      } else
	return OU::esprintf("The '%s' property is not a parameter", name);
    } else if ((*pi)->m_isParameter)
      n++;
  return OU::esprintf("Parameter property not found: '%s'", name);
}

Param::Param() : value(NULL), newValue(NULL), param(NULL) {}

const char *Param::
parse(ezxml_t px, const OU::Property *argParam) {
  std::string xValue;
  const char *err;
  newValue = new OU::Value(*argParam);
  if ((err = OE::getRequiredString(px, xValue, "value")) ||
      (err = argParam->parseValue(xValue.c_str(), *newValue)))
    return err;
  newValue->unparse(uValue);
  value = newValue;
  param = argParam;
  return NULL;
}

ParamConfig::
ParamConfig(Worker &w) : m_worker(w), nConfig(0), used(false) {}

ParamConfig::
ParamConfig(const ParamConfig &other)
  : m_worker(other.m_worker), params(other.params), id(other.id), nConfig(other.nConfig),
    used(other.used) {
}

const char *ParamConfig::
parse(ezxml_t cx) {
  const char *err;
  if ((err = OE::getNumber(cx, "id", &nConfig, NULL, 0, false, true)))
    return err;
  OU::format(id, "%zu", nConfig);
  params.resize(m_worker.m_ctl.nParameters);
  for (ezxml_t px = ezxml_cchild(cx, "parameter"); px; px = ezxml_next(px)) {
    std::string name;
    size_t nParam;
    OU::Property *p;
    if ((err = OE::getRequiredString(px, name, "name")) ||
	(err = m_worker.findParamProperty(name.c_str(), p, nParam)) ||
	(err = p->finalize(*this, false)) ||
	(err = params[nParam].parse(px, p)))
      return err;
  }
  return NULL;
}

const char *ParamConfig::
getValue(const char *sym, OU::ExprValue &val) const {
  OU::Property *prop;
  size_t nParam; // FIXME not needed since properties have m_paramOrdinal?
  const char *err;
  if ((err = m_worker.findParamProperty(sym, prop, nParam)) ||
      (err = extractExprValue(*prop, *params[nParam].value, val)))
    return err;
  return NULL;
}

const char *Worker::
paramValue(const OU::Member &param, OU::Value &v, std::string &value) {
  switch (m_model) {
  case HdlModel:
    return hdlValue(param.m_name.c_str(), v, value, true);
  case RccModel:
    return rccValue(v, value, &param);
  case OclModel:
    return rccValue(v, value, &param);
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
    if (p.param == NULL) {
      // This is a new parameter that was not in this (existing) param config.
      continue;
    }
    if (used) {
      // Put out the Makefile value lines
      std::string val;
      if (m_worker.m_model == HdlModel) {
	std::string typeDecl, type;
	vhdlType(*p.value->m_vt, typeDecl, type);
	//	if (typeDecl.length())
	//	  fprintf(mf, "ParamVHDLtype_%zu_%s:=type %s_t is %s;\n",
	//		  nConfig, p.param->m_name.c_str(), p.param->m_name.c_str(), typeDecl.c_str());
	fprintf(mf, "ParamVHDL_%zu_%s:=constant %s : ",
		nConfig, p.param->m_name.c_str(), p.param->m_name.c_str());
	if (typeDecl.length())
	  fprintf(mf, "%s_t", p.param->m_name.c_str());
	else
	  fprintf(mf, "%s", type.c_str());
	fprintf(mf, " := ");
	for (const char *cp = m_worker.hdlValue(p.param->m_name, *p.value, val, false, VHDL);
	     *cp; cp++) {
	  if (*cp == '#' || *cp == '\\' && !cp[1])
	    fputc('\\', mf);
	  fputc(*cp, mf);
	}
	fputs("\n", mf);
	fprintf(mf, "ParamVerilog_%zu_%s:=parameter [%zu:0] %s = ",
		nConfig, p.param->m_name.c_str(), rawBitWidth(*p.param) - 1,
		p.param->m_name.c_str());
	for (const char *cp = m_worker.hdlValue(p.param->m_name, *p.value, val, true, Verilog);
	     *cp; cp++) {
	  if (*cp == '#' || *cp == '\\' && !cp[1])
	    fputc('\\', mf);
	  fputc(*cp, mf);
	}
	fputs("\n", mf);
      } else {
	fprintf(mf, "Param_%zu_%s:=", nConfig, p.param->m_name.c_str());
	for (const char *cp = m_worker.paramValue(*p.param, *p.value, val); *cp; cp++) {
	  if (*cp == '#' || *cp == '\\' && !cp[1])
	    fputc('\\', mf);
	  fputc(*cp, mf);
	}
	fputs("\n", mf);
      }
      fprintf(mf, "ParamMsg_%zu_%s:=", nConfig, p.param->m_name.c_str());
      for (const char *cp = p.uValue.c_str(); *cp; cp++) {
	if (*cp == '#' || *cp == '\\' && !cp[1])
	  fputc('\\', mf);
	fputc(*cp, mf);
      }
      fputs("\n", mf);
    }
    if (xf) {
      fprintf(xf, "    <parameter name='%s' value='",
	      p.param->m_name.c_str());
      for (const char *cp = p.uValue.c_str(); *cp; cp++)
	if (*cp == '\'')
	  fputs("&apos;", xf);
	else
	  fputc(*cp, xf);
      fputs("'/>\n", xf);
    }
  }
  if (xf)
    fputs("  </configuration>\n", xf);
}

// Is the given configuration the same as this one?
bool ParamConfig::
equal(ParamConfig &other) {
  if (params.size() != other.params.size())
    return false;
  for (unsigned n = 0; n < params.size(); n++)
    if (params[n].uValue != other.params[n].uValue)
      return false;
  return true;
}

const char *Worker::
parseConfigFile(const char *dir) {
  const char *err;
  std::string fname;
  OU::format(fname, "%s/%s-params.xml", dir, m_implName);
  ezxml_t x;
  std::string empty;
  if ((err = parseFile(fname.c_str(), empty, "build", &x, empty, true, false)))
    return err;
  for (ezxml_t cx = ezxml_cchild(x, "configuration"); cx; cx = ezxml_next(cx)) {
    ParamConfig *pc = new ParamConfig(*this);
    if ((err = pc->parse(cx)))
      return err;
    m_paramConfigs.push_back(pc); // FIXME...emplace_back for C++11
  }
  return NULL;
}

// We have created a configuration based on current conditions.
// Add it if it is new
const char *Worker::
addConfig(ParamConfig &info, size_t &nConfig) {
  // Check that the configuration we have is not already in the existing file
  for (unsigned n = 0; n < m_paramConfigs.size(); n++)
    if (m_paramConfigs[n]->equal(info)) {
      // Mark the old config as used so it will be written to the Makefile
      m_paramConfigs[n]->used = true;
      return NULL;
    }
  info.nConfig = nConfig++;
  OU::format(info.id, "%zu", info.nConfig);
  // We have a new one, so we know we will be writing out the XML file
  // The XML will contain old and unused, old and used, and new and used.
  // The Makefile will contain old-that-were-used and new
  // By opening the xf file we are indicating there is something new
  if (!m_xmlFile) {
    // First time for a new one.  Write out all the old ones
    const char *err;
    if ((err = openOutput(m_fileName.c_str(), m_outDir, "", "-params", ".xml", NULL,
			  m_xmlFile)))
      return err;
    fprintf(m_xmlFile, "<build>\n");
  }
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
    for (unsigned n = 0; n < p.values.size(); n++) {
      const char *err;
      p.value = new OU::Value(prop);
      if ((err = prop.finalize(info, false)) ||
	  (err = prop.parseValue(p.values[n].c_str(), *p.value)))
	return err;
      p.value->unparse(p.uValue); // make the canonical value
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
  if ((err = parseConfigFile(m_outDir)) ||
      (err = parseRawParams(x)) ||
      (err = openOutput(m_fileName.c_str(), m_outDir, "", "-params", ".mk", NULL, m_mkFile)))
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
	      "Warning: parameter '%s' ignored due to: %s", name.c_str(), err);
    addValues(*p, info.params[nParam].values, hasValues, ezxml_txt(px));
  }
  // Fill in default values when there are no values specified for a given parameter.
  // i.e. for this parameter, make the single value for all configs the default value
  Param *par = &info.params[0];
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if ((*pi)->m_isParameter) {
      par->param = *pi;
      if (par->values.empty()) {
	if (!(*pi)->m_default)
	  return OU::esprintf("The parameter property '%s' has no value and no default value",
			      (*pi)->m_name.c_str());
	//	assert((*pi)->m_default);
	std::string sval;
	(*pi)->m_default->unparse(sval);
	par->values.push_back(sval);
      }
      par++;
    }
  // Generate all configurations, merging with existing, keeping existing ones
  // in the same position.
  if ((err = doParam(info, m_ctl.properties.begin(), 0, nConfig)))
    return err;
  // Write the makefile as well as the xml file if anything was added
  fprintf(m_mkFile, "ParamConfigurations:=");
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    if (m_paramConfigs[n]->used)
      fprintf(m_mkFile, "%s%zu", n ? " " : "", n);
  fprintf(m_mkFile, "\n");
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    m_paramConfigs[n]->write(m_xmlFile, m_mkFile);
  if (m_xmlFile)
    fprintf(m_xmlFile, "</build>\n");
  ocpiDebug("m_xmlFile closing %p m_mkFile closing %p", m_xmlFile, m_mkFile);
  if (m_xmlFile && fclose(m_xmlFile) || m_mkFile && fclose(m_mkFile))
    return OU::esprintf("File close of parameter files failed.  Disk full?");
  m_xmlFile = m_mkFile = NULL;
  return NULL;
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
  const char *slash = strrchr(m_file.c_str(), '/');
  std::string dir;
  if (slash)
    dir.assign(m_file.c_str(), slash - m_file.c_str());
  else
    dir = "gen"; // FIXME: this needs to be in a search path or something?
  if ((err = parseConfigFile(dir.c_str())))
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
	      return OU::esprintf("Bad value \"%s\" for parameter \"%s\" for worker \"%s\": %s",
				  ap->m_value.c_str(), p->m_name.c_str(), m_implName, err);
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
	for (unsigned nn = 0; nn < pc->params.size(); nn++, p++)
	  if (!strcasecmp(ap->m_name.c_str(), p->param->m_name.c_str())) {
	    OU::Value apValue;
	    if ((err = p->param->parseValue(ap->m_value.c_str(), apValue)))
	      return err;
	    std::string apString;
	    apValue.unparse(apString); // to get canonicalized value of APV
	    if (apString != p->uValue)
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
      OU::esprintf("No parameter configuration for worker matches requested parameter values");
  return NULL;
}
