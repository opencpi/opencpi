// Process the tests.xml file.
#include <strings.h>
#include <set>
#include "OcpiOsDebugApi.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "OcpiLibraryManager.h"
#include "wip.h"

#define TESTS "-tests.xml"

namespace OL = OCPI::Library;
namespace {
  // The package serves two purposes: the spec and the impl.
  // If the spec already has a package prefix, then it will only
  // be used as the package of the impl.
  // FIXME: share this with the one in parse.cxx
  const char *
  findPackage(ezxml_t spec, const char *package, const char *specName, 
	      const std::string &parent, const std::string &specFile, std::string &package_out ) {
    if (!package)
      package = ezxml_cattr(spec, "package");
    if (package)
      package_out = package;
    else {
      std::string packageFileDir;
      // If the spec name already has a package, we don't use the package file name
      // to determine the package.
      const char *base =
	!strchr(specName, '.') && !specFile.empty() ? specFile.c_str() : parent.c_str();
      const char *cp = strrchr(base, '/');
      const char *err;
      // If the specfile (first) or the implfile (second) has a dir,
      // look there for package name file.  If not, look in the CWD (the worker dir).
      if (cp)
	packageFileDir.assign(base, cp + 1 - base);

      // FIXME: Fix this using the include path maybe?
      std::string packageFileName = packageFileDir + "package-name";
      if ((err = OU::file2String(package_out, packageFileName.c_str()))) {
	// If that fails, try going up a level (e.g. the top level of a library)
	packageFileName = packageFileDir + "../package-name";
	if ((err = OU::file2String(package_out, packageFileName.c_str()))) {
	  // If that fails, try going up a level and into "lib" where it may be generated
	  packageFileName = packageFileDir + "../lib/package-name";
	  if ((err = OU::file2String(package_out, packageFileName.c_str())))
	    return OU::esprintf("Missing package-name file: %s", err);
	}
      }
      for (cp = package_out.c_str(); *cp && isspace(*cp); cp++)
	;
      package_out.erase(0, cp - package_out.c_str());
      for (cp = package_out.c_str(); *cp && !isspace(*cp); cp++)
	;
      package_out.resize(cp - package_out.c_str());
    }
    return NULL;
  }

  Workers workers;
  WorkersIter findWorker(const char *name, Workers &ws) {
    for (WorkersIter wi = ws.begin(); wi != ws.end(); ++wi)
      if (!strcasecmp(name, (*wi)->cname()))
	return wi;
    return ws.end();
  }
  const char *argPackage;
  std::string specName, specPackage;
  bool verbose;
  typedef std::set<std::string> Strings;
  typedef Strings::const_iterator StringsIter;
  Strings excludeWorkers, excludeWorkersTmp;
  // If the spec in/out arg may be set in advance if it is inline or xi:included
  // FIXME: share this with the one in parse.cxx
  const char *
  getSpec(ezxml_t xml, const std::string &parent, const char *a_package, ezxml_t &spec,
	  std::string &specFile, std::string &a_specName) {
    // xi:includes at this level are component specs, nothing else can be included
    spec = NULL;
    std::string name, file;
    const char *err;
    if ((err = tryOneChildInclude(xml, parent, "ComponentSpec", &spec, specFile, true)))
      return err;
    const char *specAttr = ezxml_cattr(xml, "spec");
    if (specAttr) {
      if (spec)
	return "Can't have both ComponentSpec element (maybe xi:included) and a 'spec' attribute";
      size_t len = strlen(specAttr);
      file = specAttr;
      // If the file is suffixed, try it as is.
      if (!strcasecmp(specAttr + len - 4, ".xml") ||
	  !strcasecmp(specAttr + len - 5, "-spec") ||
	  !strcasecmp(specAttr + len - 5, "_spec"))
	err = parseFile(specAttr, parent, "ComponentSpec", &spec, specFile, false);
      else {
	// If not suffixed, try it, and then with suffixes.
	if ((err = parseFile(specAttr, parent, "ComponentSpec", &spec, specFile, false))) {
	  file = specAttr;
	  // Try the two suffixes
	  file += "-spec";
	  if ((err = parseFile(file.c_str(), parent, "ComponentSpec", &spec, specFile, false))) {
	    file = specAttr;
	    file += "_spec";
	    if ((err = parseFile(file.c_str(), parent, "ComponentSpec", &spec, specFile, false)))
	      return OU::esprintf("After trying \"-spec\" and \"_spec\" suffixes: %s", err);
	  }
	}
      }
    } else {
      if (parent.size()) {
	// No spec mentioned at all, try using the name of the parent file with suffixes
	OU::baseName(parent.c_str(), name);
	const char *dash = strrchr(name.c_str(), '-');
	if (dash)
	  name.resize(dash - name.c_str());
      } else {
	OU::baseName(OS::FileSystem::cwd().c_str(), name);
	const char *dot = strrchr(name.c_str(), '.');
	if (dot)
	  name.resize(dot - name.c_str());
      }
      // Try the two suffixes
      file = name + "-spec";
      if ((err = parseFile(file.c_str(), parent, "ComponentSpec", &spec, specFile, false))) {
	file =  name + "_spec";
	const char *err1 = parseFile(file.c_str(), parent, "ComponentSpec", &spec, specFile, false);
	if (err1)
	  return OU::esprintf("After trying \"-spec\" and \"_spec\" suffixes: %s", err);
      }
    }
    std::string fileName;
    if ((err = getNames(spec, specFile.c_str(), "ComponentSpec", name, fileName)))
      return err;
    // If name is file name, strip suffixes for name.
    if (name == fileName) {
      size_t len = name.length();
      if (len > 5 && (!strcasecmp(name.c_str() + len - 5, "-spec") ||
		      !strcasecmp(name.c_str() + len - 5, "_spec")))
	name.resize(len - 5);
    }
    // Find the package even though the spec package might be specified already
    argPackage = a_package;
    if (strchr(name.c_str(), '.'))
      a_specName = name;
    else {
      if ((err = findPackage(spec, a_package, a_specName.c_str(), parent, specFile, specPackage)))
	return err;
      a_specName = specPackage + "." + name;
    }
    if (verbose)
      fprintf(stderr, "Spec is \"%s\" in file \"%s\"\n", 
	      a_specName.c_str(), specFile.c_str());
      return NULL;
  }

  typedef std::pair<ParamConfig*,Worker*> WorkerConfig;
  struct comp {
    inline bool operator() (const WorkerConfig &lhs, const WorkerConfig &rhs) const {
      // Are all the non-impl parameter values the same?
      // Since they are all from the same spec the order will be the same
      if (lhs.second < rhs.second)
	return false;
      if (lhs.second > rhs.second)
	return true;
      for (unsigned p = 0; p < lhs.first->params.size(); ++p) {
	//	if (lhs.first->params[p].m_param->m_isImpl)
	//	  break;
	int c = lhs.first->params[p].m_uValue.compare(rhs.first->params[p].m_uValue);
	if (c < 0)
	  return true;
	if (c > 0)
	  break;
      }
      return false;
    }
  };
  typedef std::set<WorkerConfig, comp> WorkerConfigs;
  typedef WorkerConfigs::const_iterator WorkerConfigsIter;
  WorkerConfigs configs;
  bool specific;
  const char *
  tryWorker(const char *wname, void * = NULL) {
    ocpiInfo("Considering worker \"%s\"", wname);
    const char *dot = strrchr(wname, '.');
    std::string 
      name(wname, dot - wname),
      file,
      empty;
    if (excludeWorkers.find(wname) != excludeWorkers.end()) {
      if (verbose)
	fprintf(stderr, "Skipping worker \"%s\" since it was specifically excluded.\n", wname);
      ocpiCheck(excludeWorkersTmp.erase(wname) == 1);
      return NULL;
    }
    OU::format(file, "../%s/%s.xml", wname, name.c_str());
    if (!OS::FileSystem::exists(file)) {
      if (specific)
	return OU::esprintf("For worker \"%s\", cannot open file \"%s\"",
			    wname, file.c_str());
      if (verbose)
	fprintf(stderr, "Skipping worker \"%s\" since \"%s\" not found.\n", wname, file.c_str());
      return NULL;
    }
    const char *err;
    Worker *w = Worker::create(file.c_str(), empty, argPackage, NULL, NULL, NULL, 0, err);
    if (!err && w->m_specName == specName && !(err = w->parseBuildFile(false))) {
      if (verbose)
	fprintf(stderr, "Found worker for this spec: %s\n", wname);
      workers.push_back(w);
      return NULL;
    }
    delete w;
    return err;
  }
  Worker *wFirst;
  ParamConfigs defaultCases;
  
  void
  doProp(ParamConfig &globals, FILE *out, unsigned n, size_t len, bool &first) {
    for (;n < globals.params.size(); n++) {
      Param &p = globals.params[n];
      if (p.m_param && p.m_uValues.size() > 1)
	break;
    }
    if (n >= globals.params.size()) {
      fprintf(out, "\n");
      first = true;
      return;
    }
    Param &p = globals.params[n++];
    for (unsigned nn = 0; nn < p.m_uValues.size(); ++nn) {
      if (first) {
	fprintf(out, "%3zu:", defaultCases.size());
	defaultCases.push_back(new ParamConfig(*wFirst));
	first = false;
      }
      ParamConfig &c = *defaultCases.back();
      c.params.resize(c.params.size()+1);
      Param &param = c.params.back();
      param.m_param = p.m_param;
      param.m_uValue = p.m_uValues[nn];
      fprintf(out, "%*s  %*s", nn ? (int)len : 0, "", (int)p.m_param->m_name.length(),
	      p.m_uValues[nn].c_str());
      doProp(globals, out, n, len + p.m_param->m_name.length() + 2, first);
    }
  }

  struct InputOutput {
    std::string m_name, m_file, m_script;
    DataPort *m_port;
    InputOutput() : m_port(NULL) {}
    const char *parse(ezxml_t x, std::vector<InputOutput> *inouts) {
      const char 
	*name = ezxml_cattr(x, "name"),
	*port = ezxml_cattr(x, "port"),
	*file = ezxml_cattr(x, "file"),
	*script = ezxml_cattr(x, "script");
      bool isDir;
      if (file) {
	if (script)
	  return OU::esprintf("specifying both \"file\" and \"script\" attribute is invalid");
	if (!OS::FileSystem::exists(file, &isDir) || isDir)
	  return OU::esprintf("%s file \"%s\" doesn't exist or it a directory", OE::ezxml_tag(x),
			      file);
	m_file = file;
      } else if (script)
	m_script = script;
      if (port) {
	Port *p;
	if (!(p = wFirst->findPort(port)))
	  return OU::esprintf("%s port \"%s\" doesn't exist", OE::ezxml_tag(x), port);
	if (!p->isData())
	  return OU::esprintf("%s port \"%s\" exists, but is not a data port", 
			      OE::ezxml_tag(x), port);
	m_port = static_cast<DataPort *>(p);
      }
      if (name) {
	if (inouts)
	  for (unsigned n = 0; n < inouts->size()-1; n++)
	    if (!strcasecmp(name, (*inouts)[n].m_name.c_str()))
	      return OU::esprintf("name \"%s\" is a duplicate %s name", name, OE::ezxml_tag(x));
	m_name = name;
      }
      return NULL;
    }
  };
  typedef std::vector<InputOutput> InputOutputs;
  InputOutputs inputs, outputs; // global ones that may be applied to any case
  static InputOutput *findIO(Port &p, InputOutputs &ios) {
    for (unsigned n = 0; n < ios.size(); n++)
      if (ios[n].m_port == &p)
	return &ios[n];
    return NULL;
  }
  static InputOutput *findIO(const char *name, InputOutputs &ios) {
    for (unsigned n = 0; n < ios.size(); n++)
      if (!strcasecmp(ios[n].m_name.c_str(), name))
	return &ios[n];
    return NULL;
  }
  const char *doInputOutput(ezxml_t x, void *) {
    std::vector<InputOutput> &inouts = !strcasecmp(OE::ezxml_tag(x), "input") ? inputs : outputs;
    inouts.resize(inouts.size() + 1);
    return inouts.back().parse(x, &inouts);
  }
  Strings onlyPlatforms, excludePlatforms;
  const char *doPlatform(const char *platform, void *arg) {
    Strings &set = *(Strings *)arg;
    return set.insert(platform).second ? NULL :
      OU::esprintf("platform \"%s\" is already in the list", platform);
  }
  const char *doWorker(Worker *w, void *arg) {
    Workers &set = *(Workers *)arg;
    for (WorkersIter wi = set.begin(); wi != set.end(); ++wi)
      if (w == *wi)
	OU::esprintf("worker \"%s\" is already in the list", w->cname());
    set.push_back(w);
    return NULL;
  }
  // A test case, which may apply against multiple configurations
  // Default is:
  //   cases generated from configurations and defined property values
  //   inputs and outputs are global
  //   no property results (why not global results?  why not expressions?)
  //   globally only/exclude platforms
  //   globally only/exluded workers
  // Specific cases
  // Top level attributes of <tests>
  struct Case;
  typedef std::vector<Case *> Cases;
  Cases cases;
  struct Case {
    std::string m_name, m_done;
    Strings m_onlyPlatforms, m_excludePlatforms; // can only apply these at runtime
    Workers m_workers; // the inclusion/exclusion happens at parse time
    WorkerConfigs m_configs;
    ParamConfig m_settings;
    ParamConfig m_results; // what the resulting properties should be
    ParamConfigs m_subCases;
    InputOutputs m_ports;  // the actual inputs and outputs to use
    Case(ParamConfig &globals) : m_settings(globals), m_results(*wFirst)  {}
    static const char *doExcludePlatform(const char *platform, void *arg) {
      Case &c = *(Case *)arg;
      if (excludePlatforms.find(platform) != excludePlatforms.end())
	return
	  OU::esprintf("For case \"%s\", excluded platform \"%s\" is already globally excluded",
		       c.m_name.c_str(), platform);
      if (onlyPlatforms.size() && onlyPlatforms.find(platform) == onlyPlatforms.end())
	  OU::esprintf("For case \"%s\", excluded platform \"%s\" is not in the global only platforms",
		       c.m_name.c_str(), platform);
      return doPlatform(platform, &c.m_excludePlatforms);
    }
    static const char *doOnlyPlatform(const char *platform, void *arg) {
      Case &c = *(Case *)arg;
      if (onlyPlatforms.size() && onlyPlatforms.find(platform) == onlyPlatforms.end())
	OU::esprintf("For case \"%s\", only platform \"%s\" is not in the global only platforms",
		     c.m_name.c_str(), platform);
      if (excludePlatforms.find(platform) != excludePlatforms.end())
	return
	  OU::esprintf("For case \"%s\", only platform \"%s\" is globally excluded",
		       c.m_name.c_str(), platform);
      return doPlatform(platform, &c.m_onlyPlatforms);
    }
    static const char *doOnlyWorker(const char *worker, void *arg) {
      Case &c = *(Case *)arg;
      if (excludeWorkers.find(worker) != excludeWorkers.end())
	return
	  OU::esprintf("For case \"%s\", only worker \"%s\" is globally excluded",
		       c.m_name.c_str(), worker);
      WorkersIter wi;
      if ((wi = findWorker(worker, c.m_workers)) == c.m_workers.end())
	OU::esprintf("For case \"%s\", only worker \"%s\" is not a known worker", 
		     c.m_name.c_str(), worker);      
      return doWorker(*wi, &c.m_workers);
    }
    static const char *doExcludeWorker(const char *worker, void *arg) {
      Case &c = *(Case *)arg;
      if (excludeWorkers.find(worker) != excludeWorkers.end())
	return
	  OU::esprintf("excluded worker \"%s\" is already globally excluded", worker);
      WorkersIter wi;
      if ((wi = findWorker(worker, c.m_workers)) == c.m_workers.end())
	OU::esprintf("For case \"%s\", excluded worker \"%s\" is not a potential worker", 
		     c.m_name.c_str(), worker);      
      c.m_workers.erase(wi);
      return NULL;
    }
    static const char *doCase(ezxml_t cx, void *globals) {
      Case *c = new Case(*(ParamConfig *)globals);
      const char *err;
      if ((err = c->parse(cx, cases.size()))) {
	delete c;
	return err;
      }
      cases.push_back(c);
      return NULL;
    }
    const char *parse(ezxml_t x, size_t ordinal) {
      const char *err, *a;
      if ((a = ezxml_cattr(x, "name")))
	m_name = a;
      else
	OU::format(m_name, "case%02zu", ordinal);
      if (((a = ezxml_cattr(x, "onlyplatforms")) && 
	   (err = OU::parseList(a, doOnlyPlatform, this))) ||
	  ((a = ezxml_cattr(x, "excludeplatforms")) && 
	   (err = OU::parseList(a, doExcludePlatform, this))))
	return err;
      if ((a = ezxml_cattr(x, "onlyworkers"))) {
	if ((err = OU::parseList(a, doOnlyWorker, this)))
	  return err;
      } else
	m_workers = workers;
      if ((a = ezxml_cattr(x, "excludeworkers")) && 
	  (err = OU::parseList(a, doExcludeWorker, this)))
	return err;
      // For each port, find the InputOutput for this case
      for (unsigned n = 0; n < wFirst->m_ports.size(); n++)
	if (wFirst->m_ports[n]->isData()) {
	  Port &p = *wFirst->m_ports[n];
	  DataPort &dp = *static_cast<DataPort *>(&p);
	  const char *tag = dp.isDataProducer() ? "output" : "input";
	  InputOutput *myIo = NULL;
	  for (ezxml_t iox = ezxml_cchild(x, tag); iox; iox = ezxml_cnext(iox))
	    if ((a = ezxml_cattr(iox, "port")) && !strcasecmp(p.cname(), a)) {
	      // explicit io for port - either ref to global or complete one here.
	      if ((a = ezxml_cattr(iox, "name"))) {
		InputOutput *ios = findIO(a, dp.isDataProducer() ? outputs : inputs);
		if (!ios)
		  return OU::esprintf("No global %s defined with name: \"%s\"", tag, a);
		m_ports.push_back(*ios);
		myIo = &m_ports.back();
	      } else {
		m_ports.resize(m_ports.size() + 1);
		myIo = &m_ports.back();
		if ((err = myIo->parse(iox, NULL)))
		  return err;
	      }
	      break;
	    }
	  if (!myIo) {
	    // no explicit reference to global input and no locally defined input
	    InputOutput *ios = findIO(p, dp.isDataProducer() ? outputs : inputs);
	    if (!ios)
	      return OU::esprintf("No global %s defined for port: \"%s\"", tag, p.cname());
	    m_ports.push_back(*ios);
	  }
	}
      // Parse explicit property values for this case, which will override
      for (ezxml_t px = ezxml_cchild(x, "property"); px; px = ezxml_cnext(px)) {
	std::string name;
	if ((err = OE::getRequiredString(px, name, "name")))
	  return err;
	bool found = false;
	for (unsigned n = 0; n < m_settings.params.size(); n++) {
	  Param &sp = m_settings.params[n];
	  if (sp.m_param && !strcasecmp(sp.m_param->cname(), name.c_str())) {
	    sp.parse(px, *sp.m_param);
	    found = true;
	    break;
	  }
	}
	if (!found)
	  return OU::esprintf("Property name \"%s\" not a worker or test property", name.c_str());
      }
      // We have all the port specs for this case.
      // What else about a case:
      // We can exclude values?
      // This means we need a "subcase" for each set.
      // But does that mean different IO?
      // FUNDAMENTALLY A "CASE" IS SOMETHING THAT SHARES I/O ACROSS A PARAM SPACE
      // I.E. parameterized generators and validators
      // property/parameter settings issues
      // property results
      // generate app.
      // how to generate cases over the parameter/property space?
      return NULL;
    }
    void
    doProp(unsigned n) {
      ParamConfig &c = *m_subCases.back();
      if (n >= c.params.size())
	return;
      while (!c.params[n].m_param)
	n++;
      Param &p = c.params[n];
      
      for (unsigned nn = 0; nn < p.m_uValues.size(); ++nn) {
	if (nn)
	  m_subCases.push_back(new ParamConfig(c));
	m_subCases.back()->params[n].m_uValue = p.m_uValues[nn];
	doProp(n + 1);
      }
    }
    void
    pruneSubCases() {
      for (unsigned s = 0; s < m_subCases.size(); s++) {
	ParamConfig &pc = *m_subCases[s];
	// For each worker configuration, decide whether it can support the subcase.
	// Note worker configs only have *parameters*, while subcases can have runtime properties
	unsigned viableConfigs = 0;
	for (WorkerConfigsIter wci = configs.begin(); wci != configs.end(); ++wci) {
	  ParamConfig &wcfg = *wci->first;
	  // For each property in the subcase, decide whether it conflicts with a *parameter*
	  // in this worker config
	  for (unsigned nn = 0; nn < pc.params.size(); nn++) {
	    Param &sp = pc.params[nn];
	    if (sp.m_param == NULL)
	      continue;
	    OU::Property *wprop = wci->second->findProperty(sp.m_param->cname());
	    for (unsigned n = 0; n < wcfg.params.size(); n++) {
	      Param &wparam = wcfg.params[n];
	      if (wparam.m_param  && !strcasecmp(sp.m_param->cname(), wparam.m_param->cname())) {
		if (sp.m_uValue == wparam.m_uValue)
		  goto next;     // match - this subcase property is ok for this worker config
		goto skip_worker_config; // mismatch - this worker config rejected from subcase
	      }
	    }
	    // The subcase property was not found as a parameter in the worker config
	    if (wprop) {
	      // But it is a runtime property in the worker config so it is ok
	      assert(!wprop->m_isParameter);
	      continue;
	    }
	    // The subcase property was not in this worker at all, so it must be
	    // implementation specific or a test property
	    if (sp.m_param->m_isImpl) {
	      if (sp.m_param->m_default) {
		std::string uValue;
		sp.m_param->m_default->unparse(uValue);
		if (sp.m_uValue == uValue)
		  // The subcase property is the default value so it is ok for the worker
		  // to not have it at all.
		  continue;
	      }
	      // The impl property is not the default so this worker config cannot be used
	      goto skip_worker_config;
	    }
	    // The property is a test property which is ok
	  next:;
	  }
	  viableConfigs++;
	skip_worker_config:;
	}
	if (viableConfigs == 0) {
	  m_subCases.erase(m_subCases.begin() + s);
	  s--;
	}
      }
    }
    void
    print(FILE *out) {
      fprintf(out, "Case %s:\n", m_name.c_str());
      for (unsigned s = 0; s < m_subCases.size(); s++) {
	fprintf(out, "  Subcase %02u:\n",s);
	ParamConfig &pc = *m_subCases[s];
	for (unsigned n = 0; n < pc.params.size(); n++) {
	  Param &p = pc.params[n];
	  if (p.m_param)
	    fprintf(out, "    %s = %s\n", p.m_param->cname(), p.m_uValue.c_str());
	}
      }
    }
    // Generate inputs: input files
    const char *
    generateInputs() {
      for (unsigned s = 0; s < m_subCases.size(); s++) {
	ParamConfig &pc = *m_subCases[s];
	std::string env;
	for (unsigned n = 0; n < pc.params.size(); n++) {
	  Param &p = pc.params[n];
	  if (p.m_param) {
	    assert(!strchr(p.m_uValue.c_str(), '\''));
	    OU::formatAdd(env, "OCPI_TEST_%s='%s' ", p.m_param->cname(), p.m_uValue.c_str());
	  }
	}
	for (unsigned n = 0; n < m_ports.size(); n++) {
	  InputOutput &io = m_ports[n];
	  if (!io.m_port->isDataProducer() && io.m_script.size()) {
	    // We have an input port that has a script to generate the file
	    std::string file("gen/inputs");
	    OS::FileSystem::mkdir(file, true);
	    OU::formatAdd(file, "/%s.%02u.%s", m_name.c_str(), s, io.m_port->cname());
	    std::string cmd;
	    OU::format(cmd, "%s %s%s %s", env.c_str(),
		       strchr(io.m_script.c_str(), '/') ? "" : "./", io.m_script.c_str(),
		       file.c_str());
	    ocpiInfo("For case %s, executing generator %s for port %s: %s", m_name.c_str(),
		     io.m_script.c_str(), io.m_port->cname(), cmd.c_str());
	    int r;
	    fprintf(stderr,
		    "  Generating for %s.%02u port \"%s\" file: \"%s\"\n", 
		    m_name.c_str(), s, io.m_port->cname(), file.c_str());
	    if ((r = system(cmd.c_str())))
	      return OU::esprintf("Error %d(0x%x) generating input file \"%s\" from command:  %s",
				  r, r, file.c_str(), cmd.c_str());
	    if (!OS::FileSystem::exists(file))
	      return OU::esprintf("No output from generating input file \"%s\" from "
				  "command:  %s", file.c_str(), cmd.c_str());
	      
	  }
	}
      }
      return NULL;
    }
    // Generate application xml files
    const char *
    generateApplications() {
      const char *err;
      const char *dut = strrchr(wFirst->m_specName, '.');
      if (dut)
	dut++;
      else
	dut = wFirst->m_specName;
      for (unsigned s = 0; s < m_subCases.size(); s++) {
	ParamConfig &pc = *m_subCases[s];
	std::string file("gen/applications");
	OS::FileSystem::mkdir(file, true);
	OU::formatAdd(file, "/%s.%02u.xml", m_name.c_str(), s);
	unsigned nOutputs = 0, nInputs = 0;
	for (unsigned n = 0; n < m_ports.size(); n++)
	  if (m_ports[n].m_port->isDataProducer())
	    nOutputs++;
	  else
	    nInputs++;
	std::string app("<application");
	if (m_done.size())
	  OU::formatAdd(app, " done='%s'", m_done.c_str());
	else if (nOutputs)
	  OU::formatAdd(app, " done='file_write'");
	app += ">\n";
	if (nInputs)
	  for (unsigned n = 0; n < m_ports.size(); n++)
	    if (!m_ports[n].m_port->isDataProducer()) {
	      OU::formatAdd(app, "  <instance component='ocpi.file_read' connect='%s'", dut);
	      if (nInputs > 1)
		OU::formatAdd(app, " to='%s'",  m_ports[n].m_port->cname());
	      app += ">\n";
	      std::string l_file;
	      OU::formatAdd(l_file, "../../gen/inputs/%s.%02u.%s", m_name.c_str(), s, 
			    m_ports[n].m_port->cname());
	      OU::formatAdd(app, "    <property name='filename' value='%s'/>\n", l_file.c_str());
	      app += "  </instance>\n";
	    }
	OU::formatAdd(app, "  <instance component='%s'", wFirst->m_specName);
	if (nOutputs == 1)
	  app += " connect='file_write'";
	app += ">\n";
	for (unsigned n = 0; n < pc.params.size(); n++) {
	  Param &p = pc.params[n];
	  if (p.m_param && !p.m_isTest) {
	    if (p.m_param->m_isImpl && p.m_param->m_default) {
	      std::string uValue;
	      p.m_param->m_default->unparse(uValue);
	      if (uValue == p.m_uValue)
		continue;
	    }
	    assert(!strchr(p.m_uValue.c_str(), '\''));
	    OU::formatAdd(app, "    <property name='%s' value='%s'/>\n",
			  p.m_param->cname(), p.m_uValue.c_str());
	  }
	}
	app += "  </instance>\n";
	if (nOutputs)
	  for (unsigned n = 0; n < m_ports.size(); n++) {
	    InputOutput &io = m_ports[n];
	    if (io.m_port->isDataProducer()) {
	      OU::formatAdd(app, "  <instance component='ocpi.file_write'");
	      if (nOutputs > 1)
		OU::formatAdd(app, " name='from_%s'", io.m_port->cname());
	      app += "/>\n";
#if 0
	      std::string file;
	      OU::formatAdd(file, "%s.%02u.%s.output", m_name.c_str(), s, m_ports[n].m_port->cname());
	      OU::formatAdd(app, "    <property name='filename' value='%s'/>\n", file.c_str());
	      app += "  </instance>\n";
#endif
	      if (nOutputs > 1)
		OU::formatAdd(app,
			      "  <connection>"
			      "    <port instance='%s' port='%s'/>\n"
			      "    <port instance='from_%s' port='in'/>\n"
			      "  </connection>\n",
			      dut, io.m_port->cname(),
			      io.m_port->cname());
	    }
	  }
	app += "</application>\n";
	ocpiDebug("Creating application file in %s containing:\n%s",
		  file.c_str(), app.c_str());
	if ((err = OU::string2File(app.c_str(), file.c_str(), false)))
	  return err;
      }
      return NULL;
    }
    // Generate application xml files
    const char *
    generateVerification() {
      std::string file;
      OU::format(file, "gen/applications/verify_%s.sh", m_name.c_str());
      std::string verify;
      OU::format(verify,
		 "#!/bin/sh --noprofile\n"
		 "# Verification script script for %s\n"
		 "subcase=$1\n"
		 "shift\n"
		 "case $subcase in\n",
		 m_name.c_str());
      for (unsigned s = 0; s < m_subCases.size(); s++) {
	OU::formatAdd(verify, "  (%02u)\n", s);
	ParamConfig &pc = *m_subCases[s];
	for (unsigned n = 0; n < pc.params.size(); n++) {
	  Param &p = pc.params[n];
	  if (p.m_param) {
	    assert(!strchr(p.m_uValue.c_str(), '\''));
	    OU::formatAdd(verify, "    export OCPI_TEST_%s='%s'\n",
			  p.m_param->cname(), p.m_uValue.c_str());
	  }
	}
	OU::formatAdd(verify, "    ;;\n");
      }	
      OU::formatAdd(verify, 
		    "esac\n");
      for (unsigned n = 0; n < m_ports.size(); n++) {
	InputOutput &io = m_ports[n];
	if (io.m_port->isDataProducer())
	  if (io.m_script.size()) {
	    OU::formatAdd(verify,
			  "echo '  Verifying using output file(s): ' $*\n"
			  "%s%s $*",
			  io.m_script[0] == '/' ? "" : "../../",
			  io.m_script.c_str());
	    for (unsigned nn = 0; nn < m_ports.size(); nn++) {
	      InputOutput &in = m_ports[nn];
	      if (!in.m_port->isDataProducer())
		OU::formatAdd(verify, " ../../gen/inputs/%s.$subcase.%s",
			      m_name.c_str(), in.m_port->cname());
	    }
	    verify += "\n";
	  }
      }
      OU::formatAdd(verify,
		    "if [ $? = 0 ] ; then \n"
		    "  echo '  Verification PASSED'\n"
		    "else\n"
		    "  echo '  Verification FAILED'\n"
		    "fi\n");
      return OU::string2File(verify.c_str(), file.c_str(), false);
    }
    const char *
    generateCaseXml(FILE *out) {
      fprintf(out, "  <case name='%s'>\n", m_name.c_str());
      for (unsigned s = 0; s < m_subCases.size(); s++) {
	ParamConfig &pc = *m_subCases[s];
	fprintf(out, "    <subcase id='%u'>\n", s);
	// For each worker configuration, decide whether it can support the subcase.
	// Note worker configs only have *parameters*, while subcases can have runtime properties
	for (WorkerConfigsIter wci = configs.begin(); wci != configs.end(); ++wci) {
	  ParamConfig &wcfg = *wci->first;
	  // For each property in the subcase, decide whether it conflicts with a *parameter*
	  // in this worker config
	  for (unsigned nn = 0; nn < pc.params.size(); nn++) {
	    Param &sp = pc.params[nn];
	    if (sp.m_param == NULL)
	      continue;
	    OU::Property *wprop = wci->second->findProperty(sp.m_param->cname());
	    for (unsigned n = 0; n < wcfg.params.size(); n++) {
	      Param &wparam = wcfg.params[n];
	      if (wparam.m_param  && !strcasecmp(sp.m_param->cname(), wparam.m_param->cname())) {
		if (sp.m_uValue == wparam.m_uValue)
		  goto next;     // match - this subcase property is ok for this worker config
		goto skip_worker_config; // mismatch - this worker config rejected from subcase
	      }
	    }
	    // The subcase property was not found as a parameter in the worker config
	    if (wprop) {
	      // But it is a runtime property in the worker config so it is ok
	      assert(!wprop->m_isParameter);
	      continue;
	    }
	    // The subcase property was not in this worker at all, so it must be
	    // implementation specific or a test property
	    if (sp.m_param->m_isImpl) {
	      if (sp.m_param->m_default) {
		std::string uValue;
		sp.m_param->m_default->unparse(uValue);
		if (sp.m_uValue == uValue)
		  // The subcase property is the default value so it is ok for the worker
		  // to not have it at all.
		  continue;
	      }
	      // The impl property is not the default so this worker config cannot be used
	      goto skip_worker_config;
	    }
	    // The property is a test property which is ok
	  next:;
	  }
	  {
	    std::string name = wci->second->m_implName;
	    if (wcfg.nConfig)
	      OU::formatAdd(name, "-%zu", wcfg.nConfig);
	    std::string ports;
	    bool first = true;
	    for (unsigned n = 0; n < m_ports.size(); n++)
	      if (m_ports[n].m_port->isDataProducer()) {
		OU::formatAdd(ports, "%s%s", first ? "" : ",", m_ports[n].m_port->cname());
		first = false;
	      }
	    fprintf(out, "      <worker name='%s' model='%s' outputs='%s'/>\n",
		    name.c_str(), wci->second->m_modelString, ports.c_str());
	  }
	skip_worker_config:;
	}
	fprintf(out, "    </subcase>\n");
      }
      // Output the workers and configurations that are ok to run on this case.
      fprintf(out, "  </case>\n");
      return NULL;
    }
  };

  // Explicitly included workers
  const char *addWorker(const char *name, void *) {
    const char *dot = strrchr(name, '.'); // checked earlier
    // FIXME: support finding other workers in the project path
    std::string wdir;
    const char *slash = strrchr(name, '/');
    if (slash)
      wdir = name;
    else {
      wdir = "../";
      wdir += name;
    }
    bool isDir;
    if (!OS::FileSystem::exists(wdir, &isDir) || !isDir)
      return OU::esprintf("Worker \"%s\" doesn't exist or is not a directory", name);
    const char *wname = slash ? slash + 1 : name;
    std::string
      wkrName(wname, dot - wname),
      wOWD = wdir + "/" + wkrName + ".xml";
    if (!OS::FileSystem::exists(wOWD, &isDir) || isDir)
      return OU::esprintf("For worker \"%s\", \"%s\" doesn't exist or is a directory", 
			  name, wOWD.c_str());
    return tryWorker(name);
  }
  // Explicitly included workers
  const char *excludeWorker(const char *name, void *) {
    const char *dot = strrchr(name, '.');
    if (!dot)
      return OU::esprintf("For worker name \"%s\": missing model suffix (e.g. \".rcc\")", name);
    std::string file("../");
    file += name;
    bool isDir;
    if (!OS::FileSystem::exists(file, &isDir) || !isDir)
      return OU::esprintf("Excluded worker \"%s\" does not exist in this library.", name);
    if (!excludeWorkers.insert(name).second)
      return OU::esprintf("Duplicate worker \"%s\" in excludeWorkers attribute.", name);
    excludeWorkersTmp.insert(name);
    return NULL;
  }
  const char *findWorkers() {
    if (verbose) {
      fprintf(stderr, "Looking for workers with the same spec: \"%s\"\n", specName.c_str());
      if (excludeWorkers.size())
	fprintf(stderr, "Skipping workers specifically mentioned for exclusion\n");
    }
    specific = false;
    std::string workerNames;
    const char *err;
    if ((err = OU::file2String(workerNames, "../lib/workers", ' ')) ||
	(err = OU::parseList(workerNames.c_str(), tryWorker, (void*)&workers)))
      return err;
    return NULL;
  }
} // end of anonymous namespace

const char *
createTests(const char *file, const char *package, const char */*outDir*/, bool a_verbose) {
  verbose = a_verbose;
  const char *err;
  std::string parent, xfile, specFile;
  ezxml_t xml, spec;
  if (!file || !file[0]) {
    static char x[] = "<tests/>";
    xml = ezxml_parse_str(x, strlen(x));
  } else if ((err = parseFile(file, parent, "tests", &xml, xfile, false, false, false)))
    return err;
  // ================= 1. Get the spec
  if ((err = getSpec(xml, xfile, package, spec, specFile, specName)))
    return err;
  // ================= 2. Get/find/include/exclude the global workers
  // Parse global workers
  const char 
    *excludeWorkersAttr = ezxml_cattr(xml, "excludeWorkers"),
    *onlyWorkersAttr = ezxml_cattr(xml, "onlyWorkers");
  if (onlyWorkersAttr) {
    // We will only look at workers mentioned here
    if (excludeWorkersAttr)
      return OU::esprintf("the onlyWorkers and excludeWorkers attributes cannot both occur");
    if ((err = OU::parseList(onlyWorkersAttr, addWorker)))
      return err;
  } else  if ((excludeWorkersAttr && (err = OU::parseList(excludeWorkersAttr, excludeWorker))) ||
	      (err = findWorkers()))
    return err;
  if (excludeWorkersTmp.size() && verbose)
    for (StringsIter si = excludeWorkersTmp.begin(); si != excludeWorkersTmp.end(); ++si)
      fprintf(stderr, "Excluded worker \"%s\" never found.\n", si->c_str());
  // ================= 3. Get/collect the worker parameter configurations
  // Now "workers" has workers with parsed build files.
  // So next we globally enumerate PCs independent of them, that might be dependent on them.
  // But first!... we create the first one from the defaults.
  wFirst = *workers.begin();
#if 0
  {
    static char x[] = "<configuration id='0'/>";
    ezxml_t cxml = ezxml_parse_str(x, strlen(x));
    ParamConfig &pc = *new ParamConfig(*wFirst); // leak until we put it into an object
    ParamConfigs dummy;
    pc.parse(cxml, dummy, true);
    ocpiCheck(configs.insert(std::make_pair(&pc, wFirst)).second);
    // We now have an initial squeaky-clean, defaults-only, param config at id 0
    ocpiDebug("Inserting worker %s.%s/%p with new config %p/%zu",
	      wFirst->m_implName, wFirst->m_modelString, wFirst, &pc, pc.nConfig);
  }  
#endif
  for (WorkersIter wi = workers.begin(); wi != workers.end(); ++wi) {
    Worker &w = **wi;
    for (unsigned c = 0; c < w.m_paramConfigs.size(); ++c) {
      ocpiDebug("Inserting worker %s.%s/%p with new config %p/%zu", w.m_implName,
		w.m_modelString, &w, w.m_paramConfigs[c], w.m_paramConfigs[c]->nConfig);
      ocpiCheck(configs.insert(std::make_pair(w.m_paramConfigs[c], &w)).second);
    }
  }      
  // ================= 4. Derive the union set of values from all configurations
  // Next, parse all global values into a special set for all tests that don't specify values.
  // This is different from the defaults and may have multiple values.
  ParamConfig globals(*wFirst);
  for (WorkerConfigsIter wci = configs.begin(); wci != configs.end(); ++wci) {
    ParamConfig &pc = *wci->first;
    for (unsigned n = 0; n < pc.params.size(); n++) {
      Param &p = pc.params[n];
      if (p.m_param == NULL)
	continue;
      assert(p.m_param->m_isParameter);
      OU::Property *found;
      size_t pn;
      // It might be a non-param in wFirst, even though it is a parameter in another worker
      if ((err = wFirst->findParamProperty(p.m_param->cname(), found, pn, true))) {
	// Not in wFirst, must be impl-specific
	assert(p.m_param->m_isImpl);
	// See if it is here already
	unsigned nn;
	for (nn = 0; nn < globals.params.size(); nn++)
	  if (globals.params[nn].m_param &&
	      !strcasecmp(p.m_param->cname(), globals.params[nn].m_param->cname())) {
	    assert(globals.params[nn].m_worker == wci->second);
	    pn = nn;
	    break;
	  }
	if (nn >= globals.params.size()) {
	  pn = globals.params.size();
	  globals.params.push_back(p); // add to end
	  globals.params.back().m_worker = wci->second;// remember which worker it came from
	}
      } else if (wFirst == wci->second) {
	if (found->m_isImpl && strncasecmp("ocpi_", found->cname(), 5))
	  globals.params[pn].m_worker = wFirst;
      } else {
	assert(!strncasecmp("ocpi_", found->cname(), 5) ||
	       (!globals.params[pn].m_param->m_isImpl && !found->m_isImpl));
      }
      Param &gp = globals.params[pn];
      if (!gp.m_param)
	gp.m_param = p.m_param;
      ocpiDebug("existing value for %s(%u) is %s(%zu)",
	     p.m_param->cname(), n, p.m_uValue.c_str(), pn);
      for (unsigned nn = 0; nn < gp.m_uValues.size(); nn++)
	if (p.m_uValue == gp.m_uValues[nn])
	  goto next;
      if (!gp.m_valuesType) {
	gp.m_valuesType = &p.m_param->sequenceType();
	gp.m_valuesType->m_default = new OU::Value(*gp.m_valuesType);
      }
      gp.m_valuesType->m_default->parse(p.m_uValue.c_str(), NULL, gp.m_uValues.size() != 0);
      gp.m_uValues.push_back(p.m_uValue);
    next:;
    }
  }
  // ================= 4a. Parse and collect global non-parameter property values from workers
  for (WorkersIter wi = workers.begin(); wi != workers.end(); ++wi) {
    Worker &w = **wi;
    for (PropertiesIter pi = w.m_ctl.properties.begin(); pi != w.m_ctl.properties.end(); ++pi) {
      OU::Property &p = **pi;
      if (p.m_isParameter || !p.m_isWritable)
	continue;
      Param *found = NULL;
      for (unsigned n = 0; n < globals.params.size(); n++) {
	Param &param = globals.params[n];
	if (param.m_param && !strcasecmp(param.m_param->cname(), p.cname())) {
	  found = &param;
	  break;
	}
      }
      if (found) {
	// Found in more than one worker
	assert((p.m_isParameter && found->m_param->m_isParameter) || !found->m_param->m_isImpl ||
	       !strncasecmp("ocpi_", p.cname(), 5));
      } else {
	globals.params.resize(globals.params.size()+1);
	Param &param = globals.params.back();
	param.m_param = &p;
	if (p.m_isImpl)
	  param.m_worker = &w;
	if (p.m_default) {
	  p.m_default->unparse(param.m_uValue);
	  param.m_uValues.resize(1);
	  param.m_uValues[0] = param.m_uValue;
	}
      }
    }
  }
  // ================= 5. Parse and collect global property values specified for all cases
  // Parse explicit/default property values to apply to all cases
  for (ezxml_t px = ezxml_cchild(xml, "property"); px; px = ezxml_cnext(px)) {
    std::string name;
    bool isTest;
    if ((err = OE::getRequiredString(px, name, "name")) ||
	(err = OE::getBoolean(px, "test", &isTest)))
      return err;
    Param *param;
    for (unsigned n = 0; n < globals.params.size(); n++) {
      param = &globals.params[n];
      if (param->m_param && !strcasecmp(name.c_str(), param->m_param->cname())) {
	if (isTest)
	  return OU::esprintf("The test property \"%s\" is already a worker property",
			      name.c_str());
	goto next2;
      }
    }
    if (isTest) {
      globals.params.resize(globals.params.size()+1);
      param = &globals.params.back();
      OU::Property *newp = new OU::Property();
      param->m_param = newp;
      param->m_isTest = true;
      char *copy = ezxml_toxml(px);
      // Make legel property definition XML out of this xml
      ezxml_t propx;
      if ((err = OE::ezxml_parse_str(copy, strlen(copy), propx)))
	return err;
      ezxml_set_attr(propx, "test", NULL);
      ezxml_set_attr(propx, "value", NULL);
      ezxml_set_attr(propx, "values", NULL);
      ezxml_set_attr(propx, "valuefile", NULL);
      ezxml_set_attr(propx, "valuesfile", NULL);
      ezxml_set_attr(propx, "initial", "1");
      if ((err = newp->Member::parse(propx, false, true, NULL, "property", 0)))
	return err;
    } else
      return OU::esprintf("There is no property named \"%s\" for any worker", name.c_str());
  next2:;
    param->parse(px, *param->m_param);
  }
  // ================= 6. Parse and collect global platform values
  // Parse global platforms
  const char 
    *excludes = ezxml_cattr(xml, "excludePlatforms"),
    *onlys = ezxml_cattr(xml, "onlyPlatforms");
  if (onlys) {
    if (excludes)
      return OU::esprintf("the onlyPlatforms and excludePlatforms attributes cannot both occur");
    if ((err = OU::parseList(onlys, doPlatform, &onlyPlatforms)))
      return err;
  } else if (excludes && (err = OU::parseList(onlys, doPlatform, &excludePlatforms)))
    return err;
  // ================= 6. Parse and collect global input/output specs
  // Parse global inputs and outputs
  if ((err = OE::ezxml_children(xml, "input", doInputOutput)) ||
      (err = OE::ezxml_children(xml, "output", doInputOutput)))
    return err;
  // ================= 7. Parse the specified cases (if any)
  if (!ezxml_cchild(xml, "case")) {
    static char c[] = "<case/>";
    ezxml_t x = ezxml_parse_str(c, strlen(c));
    if ((err = Case::doCase(x, &globals)))
      return err;
  } else if ((err = OE::ezxml_children(xml, "case", Case::doCase, &globals)))
    return err;
  // ================= 8. Report what we found globally (not specific to any case)
  // So now we can generate cases based on existing configs and globals.
  if (OS::logGetLevel() >= OCPI_LOG_INFO) {
    fprintf(stderr,
	    "Spec is %s, in file %s, %zu workers, %zu configs\n"
	    "Configurations are:\n",
	    specName.c_str(), specFile.c_str(), workers.size(), configs.size());
    unsigned c = 0;
    for (WorkerConfigsIter wci = configs.begin(); wci != configs.end(); ++wci, ++c) {
      ParamConfig &pc = *wci->first;
      fprintf(stderr, "  %2u: (from %s.%s)\n", 
	      c, wci->second->m_implName, wci->second->m_modelString);
      for (unsigned n = 0; n < pc.params.size(); n++) {
	Param &p = pc.params[n];
	if (p.m_param == NULL)
	  continue;
	fprintf(stderr, "      %s = %s%s\n", p.m_param->cname(), p.m_uValue.c_str(), 
		p.m_isDefault ? " (default)" : "");
      }
    }
  }

  // ================= 9. Generate report in gen/cases.txt on all combinations of propert values
  OS::FileSystem::mkdir("gen", true);
  std::string summary;
  OU::format(summary, "gen/cases.txt");
  if (verbose)
    fprintf(stderr, "Writing discovered parameter combinations in \"%s\"\n", summary.c_str());
  FILE *out = fopen(summary.c_str(), "w");
  fprintf(out, 
	  "Values common to all property combinations:\n"
	  "===========================================\n");
  for (unsigned n = 0; n < globals.params.size(); n++) {
    Param &p = globals.params[n];
    if (p.m_param == NULL || p.m_uValues.size() > 1)
      continue;
    p.m_uValue = p.m_uValues[0];
    fprintf(out, "      %s = %s", p.m_param->cname(), p.m_uValues[0].c_str());
    if (p.m_param->m_isImpl && strncasecmp("ocpi_", p.m_param->cname(), 5)) {
      assert(p.m_worker);
      fprintf(out, " (specific to worker %s.%s)", 
	      p.m_worker->m_implName, p.m_worker->m_modelString);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n"
	  "Property combinations/subcases that are default for all cases:\n"
	  "==============================================================\n"
	  "    ");
  for (unsigned n = 0; n < globals.params.size(); n++) {
    Param &p = globals.params[n];
    if (p.m_param && p.m_uValues.size() > 1)
      fprintf(out, "  %s", p.m_param->cname());
  }
  fprintf(out, "\n");
  bool first = true;
  doProp(globals, out, 0, 0, first);
  // ================= 10. Generate HDL assemblies in gen/assemblies
  if (verbose)
    fprintf(stderr, "Generating required HDL assemblies in gen/assemblies\n");
  bool hdlFileIO;
  if ((err = OE::getBoolean(xml, "UseHdlFileIO", &hdlFileIO)))
    return err;
  bool seenHDL = false;
  for (WorkersIter wi = workers.begin(); wi != workers.end(); ++wi) {
    Worker &w = **wi;
    if (w.m_model == HdlModel) {
      assert(w.m_paramConfigs.size());
      for (unsigned c = 0; c < w.m_paramConfigs.size(); ++c) {
	ParamConfig &pc = *w.m_paramConfigs[c];
	// Make sure the configuration is in the test matrix (e.g. globals)
	bool allOk = true;
	for (unsigned n = 0; allOk && n < pc.params.size(); n++) {
	  Param &p = pc.params[n];
	  bool isOk = false;
	  if (p.m_param)
	    for (unsigned nn = 0; nn < globals.params.size(); nn++) {
	      Param &gp = globals.params[nn];
	      if (gp.m_param && !strcasecmp(p.m_param->cname(), gp.m_param->cname()))
		for (unsigned v = 0; v < gp.m_uValues.size(); v++)
		  if (p.m_uValue == gp.m_uValues[v]) {
		    isOk = true;
		    break;
		  }
	    }
	  if (!isOk)
	    allOk = false;
	}
	if (!allOk)
	  continue; // skip this config - it is not in the test matrix
	if (!seenHDL) {
	  OS::FileSystem::mkdir("gen/assemblies", true);
	  OU::string2File("include $(OCPI_CDK_DIR)/include/hdl/hdl-assemblies.mk\n",
			  "gen/assemblies/Makefile", true);
	  seenHDL = true;
	}
	std::string name(w.m_implName);
	//	if (c != 0)
	OU::formatAdd(name, "_%u", c);
	std::string dir("gen/assemblies/" + name);
	OS::FileSystem::mkdir(dir, true);
	OU::string2File(hdlFileIO ?
			"override HdlPlatform:=$(filter-out %sim,$(HdlPlatform))\n"
			"override HdlPlatforms:=$(filter-out %sim,$(HdlPlatforms))\n"
			"include $(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk\n" :
			"include $(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk\n",
			dir + "/Makefile", true);
	std::string assy;
	OU::format(assy,
		   "<HdlAssembly>\n"
		   "  <Instance Worker='%s' ParamConfig='%u' externals='true'/>\n"
		   "</HdlAssembly>\n",
		   w.m_implName, c);
	OU::string2File(assy, dir + "/" + name + ".xml", true);
	if (hdlFileIO) {
	  name += "_frw";
	  dir += "_frw";
	  OS::FileSystem::mkdir(dir, true);
	  OU::string2File("override HdlPlatform:=$(filter %sim,$(HdlPlatform))\n"
			  "override HdlPlatforms:=$(filter %sim,$(HdlPlatforms))\n"
			  "include $(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk\n",
			  dir + "/Makefile", true);
	  OU::format(assy,
		     "<HdlAssembly>\n"
		     "  <Instance Worker='%s' ParamConfig='%u'/>\n",
		     w.m_implName, c);
	  for (PortsIter pi = w.m_ports.begin(); pi != w.m_ports.end(); ++pi) {
	    Port &p = **pi;
	    if (p.isData())
	      OU::formatAdd(assy,
			    "  <Instance name='%s_%s' Worker='file_%s'/>\n"
			    "  <Connection>\n"
			    "    <port instance='%s_%s' %s='%s'/>\n"
			    "    <port instance='%s' %s='%s'/>\n"
			    "  </Connection>\n",
			    w.m_implName, p.cname(), p.isDataProducer() ? "write" : "read",
			    w.m_implName, p.cname(), p.isDataProducer() ? "to" : "from",
			    p.isDataProducer() ? "in" : "out",
			    w.m_implName, p.isDataProducer() ? "from" : "to", p.cname());
	  }
	  assy += "</HdlAssembly>\n";
	  OU::string2File(assy, dir + "/" + name + ".xml", true);
	}
      }
    }
  }  
  // ================= 10. Generate subcases for each case, and generate outputs per subcase
  fprintf(out,
	  "\n"
	  "Descriptions of the %zu case%s\n"
	  "=============================\n", 
	  cases.size(), cases.size() > 1 ? "s" : "");
  for (unsigned n = 0; n < cases.size(); n++) {
    cases[n]->m_subCases.push_back(new ParamConfig(cases[n]->m_settings));
    cases[n]->doProp(0);
    cases[n]->pruneSubCases();
    cases[n]->print(out);
  }
  fclose(out);
  if (verbose)
    fprintf(stderr, "Generating required input files in gen/inputs/\n");
  for (unsigned n = 0; n < cases.size(); n++)
    if ((err = cases[n]->generateInputs()))
      return err;
  if (verbose)
    fprintf(stderr, "Generating required application xml files in gen/applications/\n");
  for (unsigned n = 0; n < cases.size(); n++)
    if ((err = cases[n]->generateApplications()) ||
	(err = cases[n]->generateVerification()))
      return err;
  out = fopen("gen/cases.xml", "w");
  if (!out)
    return OU::esprintf("Failed to open summary XML file gen/cases.xml");
  if (verbose)
    fprintf(stderr, "Generating summary gen/cases.xml file\n");
  fprintf(out, "<cases spec='%s'>\n", wFirst->m_specName);
  for (unsigned n = 0; n < cases.size(); n++)
    if ((err = cases[n]->generateCaseXml(out)))
      return err;
  fprintf(out, "</cases>\n");
  if (fclose(out))
    return OU::esprintf("Failed to write open summary XML file gen/cases.xml");
  return NULL;
}

// The arguments are platforms determined in the runtime environment
const char *
createCases(const char **platforms, const char */*package*/, const char */*outDir*/, bool a_verbose) {
  struct CallBack : public OL::ImplementationCallback {
    std::string m_spec, m_model, m_platform, m_component, m_dir, m_err;
    bool m_dynamic;
    ezxml_t m_xml;
    bool m_first;
    FILE *m_run, *m_verify;
    std::string m_outputArgs, m_verifyOutputs;
    const char *m_outputs;
    CallBack(const char *spec, const char *model, const char *platform, bool dynamic,
	     ezxml_t xml)
      : m_spec(spec), m_model(model), m_platform(platform), m_dynamic(dynamic), m_xml(xml),
	m_first(true), m_run(NULL), m_verify(NULL), m_outputs(NULL) {
      const char *cp = strrchr(spec, '.');
      m_component = cp ? cp + 1 : spec;
    }
    ~CallBack() {
      if (m_run)
	fclose(m_run);
      if (m_verify)
	fclose(m_verify);
    }
    void doOutput(const char *output) {
      bool multiple = strchr(m_outputs, ',') != NULL;
      OU::formatAdd(m_outputArgs, " -pfile_write%s%s=fileName=$5.$6.$4.%s.out",
		    multiple ? "_" : "", multiple ? output : "", output);
      OU::formatAdd(m_verifyOutputs, " $5.$6.$4.%s.out", output);
    }
    static const char *doOutput(const char *output, void *me) {
      ((CallBack*)me)->doOutput(output);
      return NULL;
    }
    bool foundImplementation(const OL::Implementation &i, bool &accepted) {
      ocpiInfo("For platform %s, considering implementation %s from %s",
	       m_platform.c_str(), i.m_metadataImpl.name().c_str(), i.m_artifact.name().c_str());
      if (i.m_artifact.platform() == m_platform && i.m_metadataImpl.model() == m_model &&
	  i.m_artifact.m_dynamic == m_dynamic) {
	for (ezxml_t cx = ezxml_cchild(m_xml, "case"); cx; cx = ezxml_cnext(cx)) {
	  unsigned n = 0;
	  const char *name = ezxml_cattr(cx, "name");
	  for (ezxml_t sx = ezxml_cchild(cx, "subcase"); sx; sx = ezxml_cnext(sx), n++)
	    for (ezxml_t wx = ezxml_cchild(sx, "worker"); wx; wx = ezxml_cnext(wx))
	      if (i.m_metadataImpl.name() == ezxml_cattr(wx, "name") &&
		  i.m_metadataImpl.model() == ezxml_cattr(wx, "model")) {
		ocpiInfo("Accepted for case %s subcase %u from file: %s", name, n, 
			 i.m_artifact.name().c_str());
		if (m_first) {
		  m_first = false;
		  std::string dir("run/" + m_platform);
		  OS::FileSystem::mkdir(dir, true);
		  std::string file(dir + "/run.sh");
		  if (verbose)
		    fprintf(stderr, "Generating run script for platform: %s\n",
			    m_platform.c_str());
		  if (!(m_run = fopen(file.c_str(), "w"))) {
		    OU::format(m_err, "Cannot open file \"%s\" for writing", file.c_str());
		    return true;
		  }
		  file = dir + "/verify.sh";
		  if (!(m_verify = fopen(file.c_str(), "w"))) {
		    OU::format(m_err, "Cannot open file \"%s\" for writing", file.c_str());
		    return true;
		  }
		  m_outputs = ezxml_cattr(wx, "outputs");
		  m_outputArgs.clear();
		  m_verifyOutputs.clear();
		  OU::parseList(m_outputs, doOutput, this);
		  fprintf(m_run,
			  "#!/bin/sh --noprofile\n"
			  "# Note that this file runs on remote/embedded systems and thus\n"
			  "# may not have access to the full development host environment\n"
			  "echo Performing test cases for %s on platform %s 1>&2\n"
			  "export OCPI_LIBRARY_PATH="
			  "../../../lib/rcc:../../gen/assemblies:$OCPI_CDK_DIR/lib/components/rcc\n"
			  "# docase <name> <platform> <model> <worker> <case> <subcase> <implprops>\n"
			  "function docase {\n"
			  "  echo Running $1 test case: \"$5\", subcase $6, on platform $2 using "
			  "worker $4.$3...\n"
			  "  ocpirun -v -m$1=$3 -w$1=$4 -P$1=$2  \\\n"
			  "   %s ../../gen/applications/$5.$6.xml \\\n"
			  "   > $5.$6.$4.$3.log 2>&1 \n"
			  "}\n"
			  "set -e\n",
			  m_spec.c_str(), m_platform.c_str(), m_outputArgs.c_str());
		  fprintf(m_verify, 
			  "#!/bin/sh --noprofile\n"
			  "echo Verifying test cases for %s on platform %s 1>&2\n"
			  "function verify {\n"
			  "  echo Verifying $1: \"$5\", subcase $6 using worker $4.$3...\n"
			  "  ../../gen/applications/verify_$5.sh $6 %s\n"
			  "}\n"
			  "set -e\n",
			  m_spec.c_str(), m_platform.c_str(), m_verifyOutputs.c_str());
		}		  
		fprintf(m_run, "docase %s %s %s %s %s %02u\n",
			m_component.c_str(), m_platform.c_str(), m_model.c_str(),
			ezxml_cattr(wx, "name"), name, n);
		fprintf(m_verify, "verify %s %s %s %s %s %02u\n",
			m_component.c_str(), m_platform.c_str(), m_model.c_str(),
			ezxml_cattr(wx, "name"), name, n);
#if 0
		for (ezxml_t px = ezxml_cchild(wx, "property"); px; px = ezxml_cnext(px))
		  fprintf(m_run, " -p%s=%s='%s'", m_component.c_str(), ezxml_cattr(px, "name"),
			  ezxml_cattr(px, "value"));
		fprintf(m_run, "\n");
#endif
	      }
	}
	accepted = true;
      }
      return false;
    }
  };
  std::string path;
  OU::format(path, "../lib/rcc:gen/assemblies:%s/lib/components/rcc",
	     getenv("OCPI_CDK_DIR"));
  setenv("OCPI_LIBRARY_PATH", path.c_str(), true);
  ocpiInfo("Initializing OCPI_LIBRARY_PATH to \"%s\"", path.c_str());
  verbose = a_verbose;
  const char *err;
  ezxml_t xml;
  if ((err = OE::ezxml_parse_file("gen/cases.xml", xml)))
    return err;
  const char *spec = ezxml_cattr(xml, "spec");
  assert(spec);
  if (verbose)
    fprintf(stderr, "Generating execution scripts for each platform that can run.\n");
  OS::FileSystem::mkdir("run", true);
  for (const char **p = platforms; *p; p++) {
    std::string model;
    const char *cp = strchr(*p, '-');
    assert(cp);
    model.assign(*p, cp - *p);
    CallBack cb(spec, model.c_str(), cp + 3, cp[1] == '1', xml);
    OL::Manager::findImplementations(cb, ezxml_cattr(xml, "spec"));
    if (cb.m_err.size())
      return OU::esprintf("While processing platform %s: %s", cp + 1, cb.m_err.c_str());
  }
  return NULL;
}
