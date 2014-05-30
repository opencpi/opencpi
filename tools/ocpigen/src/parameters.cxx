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

Param::Param() : value(NULL), newValue(NULL) {}

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
ParamConfig() : nConfig(0), used(false) {}

const char *ParamConfig::
parse(Worker &w, ezxml_t cx) {
  const char *err;
  if ((err = OE::getRequiredString(cx, id, "id")))
    return err;
  params.resize(w.m_ctl.nParameters);
  for (ezxml_t px = ezxml_cchild(cx, "parameter"); px; px = ezxml_next(px)) {
    std::string name;
    size_t nParam;
    OU::Property *p;
    if ((err = OE::getRequiredString(px, name, "name")) ||
	(err = w.findParamProperty(name.c_str(), p, nParam)) ||
	(err = params[nParam].parse(px, p)))
      return err;
  }
  return NULL;
}

void ParamConfig::
write(FILE *xf, FILE *mf, size_t nConfig) {
  if (xf)
    fprintf(xf, "  <configuration id='%s'>\n", id.c_str());
  for (unsigned n = 0; n < params.size(); n++) {
    Param &p = params[n];
    if (used) {
      // Put out the Makefile value line
      fprintf(mf, "Param_%zu_%s:=", nConfig, p.param->m_name.c_str());
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
  if ((err = parseFile(fname.c_str(), NULL, "build", &x, NULL, true, false)))
    return err;
  unsigned nConfigs = OE::countChildren(x, "configuration");
  m_paramConfigs.resize(nConfigs);
  ParamConfig *pc = &m_paramConfigs[0];
  for (ezxml_t cx = ezxml_cchild(x, "configuration"); cx; cx = ezxml_next(cx), pc++)
    if ((err = pc->parse(*this, cx)))
      return err;
  return NULL;
}

// We have created a configuration based on current conditions.
// Add it if it is new
const char *Worker::
addConfig(ParamConfig &info) {
  ParamConfigs &pcs = m_paramConfigs;
  // Check that the configuration we have is not already in the existing file
  if (!pcs.empty()) {
    ParamConfig *pc = &pcs[0];
    for (size_t n = pcs.size(); n; n--, pc++)
      if (pc->equal(info)) {
	// Mark the old config as used so it will be written to the Makefile
	pc->used = true;
	return NULL;
      }
  }
  OU::format(info.id, "%zu", info.nConfig++);
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
  info.used = true;
  pcs.push_back(info);  // This basically copies/snapshots the conf.
  return NULL;
}

const char *Worker::
doParam(ParamConfig &info, PropertiesIter pi, unsigned nParam) {
  while (pi != m_ctl.properties.end() && !(*pi)->m_isParameter)
    pi++;
  if (pi == m_ctl.properties.end())
    addConfig(info);
  else {
    Param &p = info.params[nParam];
    pi++;
    for (unsigned n = 0; n < p.values.size(); n++) {
      p.value = p.values[n];
      p.value->unparse(p.uValue);
      const char *err;
      if ((err = doParam(info, pi, nParam + 1)))
	return err;
    }
  }
  return NULL;
}

 static const char *
addValue(OU::Property &p, const char *start, const char *end, Values &values) {
   OU::Value *v = new OU::Value();
   const char *err = p.parseValue(start, *v, end);
   if (err)
     delete v;
   else
     values.push_back(v);
   return err;
 }

static const char *
addValues(OU::Property &p, Values &values, bool hasValues, const char *content) {
  // The text content is now undone with XML-quoting, but if there are
  // multiple values, we have to look for slashes unprotected by backslash.
  const char *err = NULL;
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
	break;
      if (*ep && *ep != '\n')
	ep++;
    }
  } else
    err = addValue(p, content, end, values);
  return err;
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
  ParamConfig info;                      // Current config for generating them
  info.nConfig = m_paramConfigs.size();  // start counting after existing ones
  info.params.resize(m_ctl.nParameters);
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
    else if ((err = addValues(*p, info.params[nParam].values, hasValues, ezxml_txt(px))))
      return err;
  }
  // Fill in default values
  Param *par = &info.params[0];
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if ((*pi)->m_isParameter) {
      par->param = *pi;
      if (par->values.empty()) {
	assert((*pi)->m_default);
	par->values.push_back((*pi)->m_default);
      }
      par++;
    }
  // Generate all configurations, merging with existing, keeping existing ones
  // in the same position.
  doParam(info, m_ctl.properties.begin(), 0);
  // Write the makefile as well as the xml file if anything was added
  fprintf(m_mkFile, "ParamConfigurations:=");
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    if (m_paramConfigs[n].used)
      fprintf(m_mkFile, "%s%zu", n ? " " : "", n);
  fprintf(m_mkFile, "\n");
  for (size_t n = 0; n < m_paramConfigs.size(); n++)
    m_paramConfigs[n].write(m_xmlFile, m_mkFile, n);
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
// Given assembly properties or a parameter configuration,
// establish the paramer values for this worker.
// Note this worker will then be parameter-value-specific.
const char *Worker::
setParamConfig(OU::Assembly::Properties *instancePVs, size_t paramConfig) {
  if (instancePVs && instancePVs->size() && paramConfig)
    return "instance property values cannot be specified with a parameter configuration";
  const char *err;
  // FIXME: we could cache this in one place, but workers can still be
  // parameterized by xml attribute values, so it can't simply be cached in a Worker object.
  const char *slash = strrchr(m_file.c_str(), '/');
  std::string dir;
  if (slash)
    dir.assign(m_file.c_str(), slash - m_file.c_str());
  else
    dir = "gen"; // FIXME: this needs to be in a search path or something?
  if ((err = parseConfigFile(dir.c_str())))
    return err;
  if (paramConfig) {
    if (paramConfig >= m_paramConfigs.size())
      return OU::esprintf("Parameter configuration %zu exceeds available configurations (%zu)",
			  paramConfig, m_paramConfigs.size());
    m_paramConfig = &m_paramConfigs[paramConfig];
  } else {
    // We need to find a parameter configuration matching the specified parameter values.
    // The search is first-that-fits.
  }
  return NULL;
}
