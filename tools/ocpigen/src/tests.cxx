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

// Process the tests.xml file.
#include <strings.h>
#include <sstream>
#include <set>
#include <limits>
#include <algorithm>
#include "OcpiOsDebugApi.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "OcpiLibraryManager.h"
#include "wip.h"
#include "data.h"
#include "hdl-device.h"

#define TESTS "-tests.xml"
#define MS_CONFIG "bypass", "metadata", "throttle", "full"

namespace OL = OCPI::Library;
namespace {
  // The package serves two purposes: the spec and the impl.
  // If the spec already has a package prefix, then it will only
  // be used as the package of the impl.
  // FIXME: share this with the one in parse.cxx
  const char *
  findPackage(ezxml_t spec, const char *package, const char *specName,
              const std::string &parent, const std::string &specFile, std::string &package_out) {
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
        packageFileDir.assign(base, OCPI_SIZE_T_DIFF(cp + 1, base));

      // FIXME: Fix this using the include path maybe?
      std::string packageFileName = packageFileDir + "package-id";
      if ((err = OU::file2String(package_out, packageFileName.c_str()))) {
        // If that fails, try going up a level (e.g. the top level of a library)
        packageFileName = packageFileDir + "../package-id";
        if ((err = OU::file2String(package_out, packageFileName.c_str()))) {
          // If that fails, try going up a level and into "lib" where it may be generated
          packageFileName = packageFileDir + "../lib/package-id";
          if ((err = OU::file2String(package_out, packageFileName.c_str())))
            return OU::esprintf("Missing package-id file: %s", err);
        }
      }
      for (cp = package_out.c_str(); *cp && isspace(*cp); cp++)
        ;
      package_out.erase(0, OCPI_SIZE_T_DIFF(cp, package_out.c_str()));
      for (cp = package_out.c_str(); *cp && !isspace(*cp); cp++)
        ;
      package_out.resize(OCPI_SIZE_T_DIFF(cp, package_out.c_str()));
    }
    return NULL;
  }
  const char *
  remove(const std::string &name) {
    ocpiInfo("Trying to remove %s", name.c_str());
    int rv = ::system(std::string("rm -r -f " + name).c_str());
    return rv ?
      OU::esprintf("Error removing \"%s\" directory: %d", name.c_str(), rv) :
      NULL;
  }
  Workers workers;
  unsigned matchedWorkers = 0; // count them even if they are not built or usable
  std::string testFile;
  WorkersIter findWorker(const char *name, Workers &ws) {
    for (auto wi = ws.begin(); wi != ws.end(); ++wi) {
        std::string workername; //need the worker name and the model in order to make comparison
        OU::formatAdd(workername, "%s.%s",(*wi)->cname(), (*wi)->m_modelString);
	if (!strcasecmp(name, workername.c_str()))
          return wi;
    }
    return ws.end();
  }
  size_t timeout, duration;
  const char *finishPort;
  const char *argPackage;
  std::string specName, specPackage;
  bool verbose;
  Strings excludeWorkers, excludeWorkersTmp;
  bool testingOptionalPorts;
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
          name.resize(OCPI_SIZE_T_DIFF(dash, name.c_str()));
      } else {
        OU::baseName(OS::FileSystem::cwd().c_str(), name);
        const char *dot = strrchr(name.c_str(), '.');
        if (dot)
          name.resize(OCPI_SIZE_T_DIFF(dot, name.c_str()));
      }
      // Try the two suffixes
      file = name + "-spec";
      if ((err = parseFile(file.c_str(), parent, "ComponentSpec", &spec, specFile, false))) {
        file =  name + "_spec";
        const char *err1 = parseFile(file.c_str(), parent, "ComponentSpec", &spec, specFile, false);
        if (err1) {
          // No spec files are found, how about a worker with the same name?
          // (if no spec, must be a single worker with embedded spec?)
          bool found = false;
          for (OS::FileIterator iter("../", name + ".*"); !iter.end(); iter.next()) {
            std::string wname;
            const char *suffix = strrchr(iter.relativeName(wname), '.') + 1;
            if (strcmp(suffix, "test")) {
              OU::format(file, "../%s/%s.xml", wname.c_str(), name.c_str());
              if ((err1 =
                   parseFile(file.c_str(), parent, NULL, &spec, specFile, false, false, false))) {
                ocpiInfo("When trying to open and parse \"%s\":  %s", file.c_str(), err1);
                continue;
              }
              if (!ezxml_cchild(spec, "componentspec")) {
                ocpiInfo("When trying to parse \"%s\":  no embedded ComponentSpec element found",
                         file.c_str());
                continue;
              }
              if (found)
                return OU::esprintf("When looking for workers matching \"%s\" with embedded "
                                    "specs, found more than one", name.c_str());
              found = true;
              specName = name; // package?
            }
          }
          if (!found)
            return OU::esprintf("After trying \"-spec\" and \"_spec\" suffixes, no spec found");
        }
      }
    }
    std::string fileName;
    // This is a component spec file or a worker OWD that contains a componentspec element
    if ((err = getNames(spec, specFile.c_str(), NULL, name, fileName)))
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
        //      if (lhs.first->params[p].m_param->m_isImpl)
        //        break;
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
  Worker *emulator = NULL; // this is a singleton in this context
  // Given a worker, see if it is what we want, either matching a spec, or emulating a device
  const char *
  tryWorker(const char *wname, const std::string &matchName, bool matchSpec, bool specific) {
    ocpiInfo("Considering worker \"%s\"", wname);
    const char *dot = strrchr(wname, '.');
    std::string
      name(wname, OCPI_SIZE_T_DIFF(dot, wname)),
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
      if (matchSpec && specific)
        return OU::esprintf("For worker \"%s\", cannot open file \"%s\"", wname, file.c_str());
      if (verbose)
        fprintf(stderr, "Skipping worker \"%s\" since \"%s\" not found.\n", wname, file.c_str());
      return NULL;
    }
    const char *err;
    Worker *w = Worker::create(file.c_str(), testFile, argPackage, NULL, NULL, NULL, 0, err);
    bool missing;
    if (!err && (matchSpec ? w->m_specName == matchName :
                 w->m_emulate && w->m_emulate->m_implName == matchName) &&
        !(err = w->parseBuildFile(true, &missing, &testFile))) {
      if (verbose)
        fprintf(stderr,
                "Found worker for %s:  %s\n", matchSpec ? "this spec" : "emulating this worker",
                wname);
      matchedWorkers++;
      if (missing) {
        if (verbose)
          fprintf(stderr, "Skipping worker \"%s\" since it isn't built for any target\n", wname);
        return NULL;
      }
      if (matchSpec) {
        if (w->m_signals.size()) {
          if (verbose)
            fprintf(stderr, "Worker has device signals.  Looking for emulator worker.\n");
          std::string workerNames;
          if ((err = OU::file2String(workerNames, "../lib/workers", ' ')))
            return err;
	  addDep("../lib/workers", false); // if we add or remove a worker from the library...
          for (OU::TokenIter ti(workerNames.c_str()); ti.token(); ti.next()) {
            if ((err = tryWorker(ti.token(), w->m_implName, false, false)))
              return err;
          }
        }
        workers.push_back(w);
      } else {
        // Found an emulator
        if (emulator)
          return OU::esprintf("Multiple emulators found for %s", matchName.c_str());
        emulator = w;
      }
      return NULL;
    }
    delete w;
    return err;
  }
  typedef std::vector<DataPort *> DataPorts;
  DataPorts optionals;
  Worker *wFirst;
  enum MsConfig {bypass, metadata, throttle, full};
  static const char *s_stressorMode[] = { MS_CONFIG, NULL };
  struct InputOutput {
    std::string m_name, m_file, m_script, m_view;
    const DataPort *m_port;
    size_t m_messageSize;
    bool m_messagesInFile, m_suppressEOF, m_disableBackpressure, m_stopOnEOF, m_testOptional;
    MsConfig m_msMode;
    InputOutput()
      : m_port(NULL), m_messageSize(0), m_messagesInFile(false), m_suppressEOF(false),
        m_disableBackpressure(false), m_stopOnEOF(false), m_testOptional(false), m_msMode(bypass) {}
    const char *parse(ezxml_t x, std::vector<InputOutput> *inouts) {
      const char
        *name = ezxml_cattr(x, "name"),
        *port = ezxml_cattr(x, "port"),
        *file = ezxml_cattr(x, "file"),
        *script = ezxml_cattr(x, "script"),
        *view = ezxml_cattr(x, "view"),
        *err;
      if ((err = OE::checkAttrs(x, "name", "port", "file", "script", "view", "messageSize",
                                "messagesInFile", "suppressEOF", "stopOnEOF", "disableBackpressure", "testOptional",
                                "stressorMode", (void*)0)))
        return err;
      size_t nn;
      bool suppress, stop, backpressure, testOptional;
      if ((err = OE::getNumber(x, "messageSize", &m_messageSize, 0, true, false)) ||
          (err = OE::getBoolean(x, "messagesInFile", &m_messagesInFile)) ||
          (err = OE::getBoolean(x, "suppressEOF", &m_suppressEOF, false, true, &suppress)) ||
          (err = OE::getBoolean(x, "stopOnEOF", &m_stopOnEOF, false, true, &stop)) ||
          (err = OE::getBoolean(x, "disableBackpressure", &m_disableBackpressure, false, false,
                                &backpressure)) ||
          (err = OE::getBoolean(x, "testOptional", &m_testOptional, false, false,
                                &testOptional)) ||
          (err = OE::getEnum(x, "stressorMode", s_stressorMode, "input stress mode", nn, m_msMode)))
        return err;
      if (!ezxml_cattr(x, "stopOnEOF"))
        m_stopOnEOF = true; // legacy exception to the default-is-always-false rule
      m_msMode = (MsConfig)nn;
      bool isDir;
      if (file) {
        if (script)
          return OU::esprintf("specifying both \"file\" and \"script\" attribute is invalid");
        if (!OS::FileSystem::exists(file, &isDir) || isDir)
          return OU::esprintf("%s file \"%s\" doesn't exist or is a directory", OE::ezxml_tag(x),
                              file);
        m_file = file;
      } else if (script)
        m_script = script;
      if (view)
        m_view = view;
      if (port) {
        Port *p;
        if (!(p = wFirst->findPort(port)) && (!emulator || !(p = emulator->findPort(port))))
          return OU::esprintf("%s port \"%s\" doesn't exist", OE::ezxml_tag(x), port);
        if (!p->isData())
          return OU::esprintf("%s port \"%s\" exists, but is not a data port",
                              OE::ezxml_tag(x), port);
        if (p->isDataProducer()) {
          if (suppress)
            return
              OU::esprintf("the \"suppressEOF\" attribute is invalid for an output port:  \"%s\"",
                           port);
          if (!stop)
            m_stopOnEOF = true;
          if (m_msMode != bypass)
            return
              OU::esprintf("the \"stressorMode\" attribute is invalid for an output port:  \"%s\"",
                           port);
        } else {
          if (stop)
            return
              OU::esprintf("the \"stopOnEOF\" attribute is invalid for an input port:  \"%s\"",
                           port);
          if (backpressure)
            return
              OU::esprintf("the \"disableBackpressure\" attribute is invalid for an input port:  \"%s\"",
                           port);
        }
        m_port = static_cast<DataPort *>(p);
        if (testOptional) {
          testingOptionalPorts = true;
          optionals.resize(optionals.size() + 1);
          optionals.push_back(static_cast<DataPort *>(p));
        }
      }
      if (name) {
        if (inouts) {
          for (unsigned n = 0; n < inouts->size()-1; n++) {
            if (!strcasecmp(name, (*inouts)[n].m_name.c_str())) {
              return OU::esprintf("name \"%s\" is a duplicate %s name", name, OE::ezxml_tag(x));
            }
          }
        }
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
  OrderedStringSet onlyPlatforms, excludePlatforms;
  const char *doPlatform(const char *platform, Strings &platforms) {
    platforms.insert(platform); // allow duplicates
    return NULL;
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
  OrderedStringSet allPlatforms;
  struct Case {
    std::string m_name;
    Strings m_onlyPlatforms, m_excludePlatforms; // can only apply these at runtime
    Workers m_workers; // the inclusion/exclusion happens at parse time
    WorkerConfigs m_configs;
    ParamConfig m_settings;
    ParamConfig m_results; // what the resulting properties should be
    ParamConfigs m_subCases;
    InputOutputs m_ports;  // the actual inputs and outputs to use
    size_t m_timeout, m_duration;
    std::string m_delays;

    Case(ParamConfig &globals)
      : m_settings(globals), m_results(*wFirst), m_timeout(timeout), m_duration(duration)
    {}
    static const char *doExcludePlatform(const char *a_platform, void *arg) {
      Case &c = *(Case *)arg;
      OrderedStringSet platforms;
      const char *err;
      if ((err = getPlatforms(a_platform, platforms)))
        return err;
      for (auto pi = platforms.begin(); pi != platforms.end(); ++pi) {
        const char *platform = pi->c_str();
        if (excludePlatforms.find(platform) != excludePlatforms.end()) {
      	  fprintf(stderr, "Warning:  for case \"%s\", excluded platform \"%s\" is already "
      		  "globally excluded\n", c.m_name.c_str(), platform);
          return NULL;
        }
      	if (onlyPlatforms.size() && onlyPlatforms.find(platform) == onlyPlatforms.end()) {
          //If there is a global onlyPlatforms list, only exclude things from that list
      	  fprintf(stderr, "Warning:  for case \"%s\", excluded platform \"%s\" is not in the "
      		  "global only platforms\n", c.m_name.c_str(), platform);
          return NULL;
        }
        if ((err = doPlatform(platform, c.m_excludePlatforms)))
          return err;
      }
      return NULL;
    }
    static const char *doOnlyPlatform(const char *a_platform, void *arg) {
      Case &c = *(Case *)arg;
      OrderedStringSet platforms;
      const char *err;
      if ((err = getPlatforms(a_platform, platforms)))
        return err;
      for (auto pi = platforms.begin(); pi != platforms.end(); ++pi) {
        const char *platform = pi->c_str();
        if (excludePlatforms.find(platform) != excludePlatforms.end())
          return OU::esprintf("For case \"%s\", only platform \"%s\" is globally excluded",
			      c.m_name.c_str(), platform);
        if (onlyPlatforms.size() && onlyPlatforms.find(platform) == onlyPlatforms.end())
           return OU::esprintf("For case \"%s\", only platform \"%s\" is not in global list",
			       c.m_name.c_str(), platform);
        if ((err = doPlatform(platform, c.m_onlyPlatforms)))
          return err;
      }
      return NULL;
    }
    static const char *doOnlyWorker(const char *worker, void *arg) {
      Case &c = *(Case *)arg;
      if (excludeWorkers.find(worker) != excludeWorkers.end())
        return
          OU::esprintf("For case \"%s\", only worker \"%s\" is globally excluded",
                       c.m_name.c_str(), worker);
      WorkersIter wi;
      if ((wi = findWorker(worker, workers)) == workers.end()) //checking against global worker list
        return OU::esprintf("For case \"%s\", only worker \"%s\" is not a known worker",
                            c.m_name.c_str(), worker);
      return doWorker(*wi, &c.m_workers);
    }
    static const char *doExcludeWorker(const char *worker, void *arg) {
      Case &c = *(Case *)arg;
      if (excludeWorkers.find(worker) != excludeWorkers.end())
        return OU::esprintf("excluded worker \"%s\" is already globally excluded", worker);
      WorkersIter wi;
      if ((wi = findWorker(worker, c.m_workers)) == c.m_workers.end()) {
        fprintf(stderr, "Warning:  for case \"%s\", excluded worker \"%s\" is not a potential worker",
                     c.m_name.c_str(), worker);
        return NULL;
      }
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
    const char *doPorts(Worker &w, ezxml_t x) {
      for (unsigned n = 0; n < w.m_ports.size(); n++)
        if (w.m_ports[n]->isData()) {
          Port &p = *w.m_ports[n];
          DataPort &dp = *static_cast<DataPort *>(&p);
          const char *a, *err, *tag = dp.isDataProducer() ? "output" : "input";
          InputOutput *myIo = NULL;
          for (ezxml_t iox = ezxml_cchild(x, tag); iox; iox = ezxml_cnext(iox))
            if ((a = ezxml_cattr(iox, "port")) && !strcasecmp(p.pname(), a)) {
              // explicit io for port - either ref to global or complete one here.
              if ((a = ezxml_cattr(iox, "name"))) {
                InputOutput *ios = findIO(a, dp.isDataProducer() ? outputs : inputs);
                if (!ios)
                  return OU::esprintf("No global %s defined with name: \"%s\"", tag, a);
                ios->m_port = &dp;
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
            if (!ios) {
              if (dp.isDataProducer()) {
                ios = new InputOutput;
                ios->m_port = &dp;
                fprintf(stderr, "Warning:  no output file or script defined for port \"%s\"\n",
                        dp.pname());
              } else
                return OU::esprintf("No global %s defined for port: \"%s\"", tag, p.pname());
            }
            m_ports.push_back(*ios);
          }
        }
      return NULL;
    }
    // FIXME: this code is redundant with the OcpiUtilAssembly.cxx
    const char *parseDelay(ezxml_t sx, const OU::Property &p) {
      const char *err;
      if ((err = OE::checkAttrs(sx, "delay", "value", NULL)) ||
          (err = OE::checkElements(sx, NULL)))
        return err;
      if (p.m_isTest)
        return "delayed property settings are not allowed for test properties";
      // We are preparsing these delays to produce earlier errors, otherwise we could just
      // save the XML and attach it to the generated apps
      OU::ValueType vt(OA::OCPI_Double);
      OU::Value v(vt);
      const char
        *delay = ezxml_cattr(sx, "delay"),
        *value = ezxml_cattr(sx, "value");
        if (!delay || !value)
          return "<set> elements must contain both \"delay\" and \"value\" attributes";
      if ((err = v.parse(delay)))
        return err;
      v.m_Double *= 1e6;
      if (v.m_Double < 0 || v.m_Double >= std::numeric_limits<uint32_t>::max())
        return OU::esprintf("delay value \"%s\"(%g) out of range, 0 to %g", delay, v.m_Double/1e6,
                        (double)std::numeric_limits<uint32_t>::max()/1e6);
      v.setType(p);
      if ((err = v.parse(value)))
        return err;
      ocpiDebug("Adding delay for:  <property name='%s' delay='%s' value='%s'/>",
                p.cname(), delay, value);
      OU::formatAdd(m_delays, "    <property name='%s' delay='%s' value='%s'/>\n",
                    p.cname(), delay, value);
      // add to some list
      return NULL;
    }

    // Parse a case
    const char *parse(ezxml_t x, size_t ordinal) {
      const char *err, *a;
      if ((a = ezxml_cattr(x, "name")))
        m_name = a;
      else
        OU::format(m_name, "case%02zu", ordinal);
      if ((err = OE::checkAttrs(x, "duration", "timeout", "onlyplatforms", "excludeplatforms",
                                "onlyworkers", "excludeworkers", NULL)) ||
          (err = OE::checkElements(x, "property", "input", "output", NULL)) ||
          (err = OE::getNumber(x, "duration", &m_duration, NULL, duration)) ||
          (err = OE::getNumber(x, "timeout", &m_timeout, NULL, timeout)))
        return err;
      if (m_duration && m_timeout)
        return OU::esprintf("Specifying both duration and timeout is not supported");
      if ((err = doPorts(*wFirst, x)) || (emulator && (err = doPorts(*emulator, x))))
          return err;
      if ((a = ezxml_cattr(x, "onlyworkers"))) {
        if (ezxml_cattr(x, "excludeworkers"))
          return OU::esprintf("the onlyWorkers and excludeWorkers attributes cannot both occur");
        if ((err = OU::parseList(a, doOnlyWorker, this)))
          return err;
      } else {
        m_workers = workers;
        if ((a = ezxml_cattr(x, "excludeworkers")) && (err = OU::parseList(a, doExcludeWorker, this)))
          return err;
      }
      // The only time the onlyplatforms= or excludeplatforms= attributes have an effect in the gen/case.xml
      // is at the subcase level, not case or cases, so the global lists of only/excludedPlatforms
      // must be added to each case's individual lists (m_excludePlatforms and m_onlyPlatforms)
      // there's a check earlier to make sure both onlyPlatforms and excludePlatforms
      // aren't set at the same time
      // global level excludePlatforms is added to case level excludePlatforms here
      // onlyPlatforms is handled later
      if (excludePlatforms.size())
         m_excludePlatforms.insert(excludePlatforms.begin(), excludePlatforms.end());

      if ((a = ezxml_cattr(x, "onlyplatforms"))) {
        if (ezxml_cattr(x, "excludeplatforms"))
          return OU::esprintf("the onlyplatforms and excludeplatforms attributes cannot both occur");
        if ((err = OU::parseList(a, doOnlyPlatform, this)))
          return err;
      } else if ((a = ezxml_cattr(x, "excludeplatforms")) &&
                 (err = OU::parseList(a, doExcludePlatform, this)))
        return err;
      // Parse explicit property values for this case, which will override
      for (ezxml_t px = ezxml_cchild(x, "property"); px; px = ezxml_cnext(px)) {
        if ((err = OE::checkAttrs(px, PARAM_ATTRS, "generate", "add", "only", "exclude", NULL)) ||
            (err = OE::checkElements(px, "set", NULL)))
          return err;
        std::string name;
        if ((err = OE::getRequiredString(px, name, "name")))
          return err;
	// The input name can be worker-qualified or not.
        Param *found = NULL, *wfound = NULL;
	bool qname = strchr(name.c_str(), '.') != NULL;
        for (unsigned n = 0; n < m_settings.params.size(); n++) {
          Param &sp = m_settings.params[n];
          if (sp.m_param) {
	    const char *dot = strrchr(sp.m_name.c_str(), '.');
	    // Note we are looking at the param name, not the property name, which allows us to
	    // specifically name worker-specific properties (name.model.property)
	    if (!strcasecmp(sp.m_name.c_str(), name.c_str())) { // whole (maybe qualified) name match
	      if (qname) {
		assert(!wfound && !found); // can't happen
		wfound = &sp;
	      } else { // found an unqualified name - we should only find one
		assert(!wfound && !found && (sp.m_isTest || !sp.m_param->m_isImpl));
		found = &sp;
	      }
	    } else if (!qname && dot && !strcasecmp(dot + 1, name.c_str())) { // matched the last part
	      assert(!wfound);
	      if (found && !sp.m_worker->m_emulate)
		return OU::esprintf("Error:  Property name \"%s\" matches more than one worker-specific "
				    "property and is ambiguous.  Worker-specific properties "
				    "can be prefixed by worker name and model,"
				    " e.g. wkr1.rcc.prop1", name.c_str());

	      found = &sp;
	    }
	  }
	}
	if (!found)
	  found = wfound;
        if (!found)
          return OU::esprintf("Property name \"%s\" not a spec or test property (worker-specific "
			      "properties must be prefixed by worker name and model, e.g. wkr1.rcc.prop1)",
			      name.c_str());
	// poor man's: any xml attributes? e.g. if not only "set" element?
	if (px->attr && px->attr[0] &&
	    (strcasecmp(px->attr[0], "name") || (px->attr[2] && strcasecmp(px->attr[2], "name"))) &&
	    (err = found->parse(px, NULL, NULL, true)))
	  return err;
        for (ezxml_t sx = ezxml_cchild(px, "set"); sx; sx = ezxml_cnext(sx))
          if ((err = parseDelay(sx, *found->m_param)))
            return err;
      }
      return NULL;
    }
    void
    doProp(unsigned n) {
      ParamConfig &c = *m_subCases.back();
      while (n < c.params.size() && (!c.params[n].m_param || !c.params[n].m_generate.empty() ||
                                     c.params[n].m_uValues.empty()))
        n++;
      if (n >= c.params.size())
        return;
      Param &p = c.params[n];
      for (unsigned nn = 0; nn < p.m_uValues.size(); ++nn) {
        if (nn)
          m_subCases.push_back(new ParamConfig(c));
        m_subCases.back()->params[n].m_uValue = p.m_uValues[nn];
        doProp(n + 1);
      }
    }
    const char *
    pruneSubCases() {
      ocpiDebug("Pruning subcases for case %s starting with %zu subcases",
                m_name.c_str(), m_subCases.size());
      for (unsigned s = 0; s < m_subCases.size(); s++) {
        ocpiDebug("Considering pruning subcase %s.%u",  m_name.c_str(), s);
        ParamConfig &pc = *m_subCases[s];
        // For each worker configuration, decide whether it can support the subcase.
        // Note worker configs only have *parameters*, while subcases can have runtime properties
        unsigned viableConfigs = 0;
        for (WorkerConfigsIter wci = configs.begin(); wci != configs.end(); ++wci) {
          ParamConfig &wcfg = *wci->first;
          ocpiDebug("--Considering worker %s.%s(%zu)", wci->second->cname(),
                    wci->second->m_modelString, wci->first->nConfig);
          // For each property in the subcase, decide whether it conflicts with a *parameter*
          // in this worker config
          for (unsigned nn = 0; nn < pc.params.size(); nn++) {
            Param &sp = pc.params[nn];
            if (sp.m_param == NULL)
              continue;
            OU::Property *wprop = wci->second->findProperty(sp.m_param->cname());
            for (unsigned n = 0; n < wcfg.params.size(); n++) {
              Param &wparam = wcfg.params[n];
              if (wparam.m_param && !strcasecmp(sp.m_param->cname(), wparam.m_param->cname())) {
                if (sp.m_uValue == wparam.m_uValue)
                  goto next;     // match - this subcase property is ok for this worker config
                ocpiDebug("--Skipping worker %s.%s(%zu) because its param %s is different",
                          wci->second->cname(), wci->second->m_modelString,
                          wci->first->nConfig, sp.m_param->cname());
                goto skip_worker_config; // mismatch - this worker config rejected from subcase
              }
            }
            // The subcase property was not found as a parameter in the worker config
            if (wprop) {
              // But it is a runtime property in the worker config so it is ok
              assert(!wprop->m_isParameter);
              continue; // do next subcase parameter/property
            }
            // The subcase property is for the emulator, which is ok
            if (sp.m_worker && sp.m_worker->m_emulate)
              continue;
            // The subcase property was not in this worker at all, so it must be
            // implementation specific or a test property or an emulator property
            if (sp.m_param->m_isImpl && sp.m_uValue.size()) {
              if (sp.m_param->m_default) {
                std::string uValue;
                sp.m_param->m_default->unparse(uValue);
                if (sp.m_uValue == uValue)
                  // The subcase property is the default value so it is ok for the worker
                  // to not have it at all.
                  continue;
              }
              // The impl property is not the default so this worker config cannot be used
              ocpiDebug("Skipping worker %s.%s(%zu) because param %s(%s) is impl-specific and %s",
                        wci->second->cname(), wci->second->m_modelString, wci->first->nConfig,
                        sp.m_param->cname(), sp.m_uValue.c_str(),
                        sp.m_param->m_default ? " the value does not match the default" :
                        "there is no default to check");
              goto skip_worker_config;
            }
            // The property is a test property which is ok
          next:;
          }
          viableConfigs++;
        skip_worker_config:;
        }
        if (viableConfigs == 0) {
          ocpiDebug("Removing subcase %u since no workers implement it", s);
          m_subCases.erase(m_subCases.begin() + s);
          s--;
        }
      }
      return m_subCases.size() == 0 ?
        OU::esprintf("For case %s, there are no valid parameter combinations for any worker",
                     m_name.c_str()) : NULL;
    }
    void
    print(FILE *out) {
      fprintf(out, "Case %s:\n", m_name.c_str());
      table(out);
      for (unsigned s = 0; s < m_subCases.size(); s++) {
        fprintf(out, "  Subcase %02u:\n",s);
        ParamConfig &pc = *m_subCases[s];
        for (unsigned n = 0; n < pc.params.size(); n++) {
          Param &p = pc.params[n];
          if (p.m_param) {
            if (p.m_generate.empty())
              fprintf(out, "    %s = %s\n", p.m_param->cname(), p.m_uValue.c_str());
            else
              fprintf(out, "    %s = generated in file: gen/properties/%s.%02u.%s\n",
                      p.m_param->cname(), m_name.c_str(), s, p.m_param->cname());
          }
        }
      }
    }
    void
    table(FILE *out) {
      std::vector<size_t> sizes(m_settings.params.size(), 0);
      std::vector<const char *> last(m_settings.params.size(), NULL);
      bool first = true;
      for (unsigned n = 0; n < m_settings.params.size(); n++) {
        Param &p = m_settings.params[n];
        if (p.m_param && p.m_uValues.size() > 1) {
          sizes[n] = p.m_param->m_name.length() + 2;
          for (unsigned u = 0; u < p.m_uValues.size(); u++)
            sizes[n] = std::max(sizes[n], p.m_uValues[u].length());
          if (first) {
            fprintf(out, "  Summary of subcases\n");
            fprintf(out, "  Subcase # ");
            first = false;
          }
          fprintf(out, "  %-*s", (int)sizes[n], p.m_param->cname());
        }
      }
      if (first)
        return;
      fprintf(out, "\n  ---------");
      for (unsigned n = 0; n < m_settings.params.size(); n++) {
        Param &p = m_settings.params[n];
        if (p.m_param && p.m_uValues.size() > 1) {
          std::string dashes;
          dashes.assign(sizes[n], '-');
          fprintf(out, "  %s", dashes.c_str());
        }
      }
      fprintf(out, "\n");
      for (unsigned s = 0; s < m_subCases.size(); s++) {
        fprintf(out, "%6u:   ", s);
        ParamConfig &pc = *m_subCases[s];
        for (unsigned n = 0; n < pc.params.size(); n++) {
          Param &p = pc.params[n];
          if (p.m_param && p.m_uValues.size() > 1) {
            fprintf(out, "  %*s", (int)sizes[n],
                    last[n] && !strcmp(last[n], p.m_uValue.c_str()) ? "" :
                    p.m_uValue.empty() ? "-" : p.m_uValue.c_str());
            last[n] = p.m_uValue.c_str();
          }
        }
        fprintf(out, "\n");
      }
      fprintf(out, "\n");
    }
    const char *
    generateFile(bool &first, const char *dir, const char *type, unsigned s,
                 const std::string &name, const std::string &generate, const std::string &env,
                 std::string &file) {
      if (verbose && first) {
        fprintf(stderr, "Generating for %s.%02u:\n", m_name.c_str(), s);
        first = false;
      }
      // We have an input port that has a script to generate the file
      file = "gen/";
      file += dir;
      OS::FileSystem::mkdir(file, true);
      OU::formatAdd(file, "/%s.%02u.%s", m_name.c_str(), s, name.c_str());
      // Allow executing from the target dir in case we created some C++ programs
      std::string cmd("PATH=.:$OCPI_TOOL_DIR:$OCPI_PROJECT_DIR/scripts:$PATH ");
      cmd += "PYTHONPATH=$OCPI_PROJECT_DIR/scripts:$PYTHONPATH ";
      cmd += env;
      size_t prefix = cmd.length();
      OU::formatAdd(cmd, " %s %s", generate.c_str(), file.c_str());
      ocpiInfo("For case %s.%02u, executing generator \"%s\" for %s %s: %s", m_name.c_str(), s,
               generate.c_str(), type, name.c_str(), cmd.c_str());
      if (verbose)
	fprintf(stderr,
		"  Generating %s \"%s\" file: \"%s\"\n"
		"    Using command: %s\n",
		type, name.c_str(), file.c_str(), cmd.c_str() + prefix);
      int r;
      if ((r = system(cmd.c_str())))
        return OU::esprintf("Error %d(0x%x) generating %s file \"%s\" from command:  %s",
                            r, r, type, file.c_str(), cmd.c_str());
      if (!OS::FileSystem::exists(file))
        return OU::esprintf("No output from generating %s file \"%s\" from command:  %s",
                            type, file.c_str(), cmd.c_str());
      const char *space = strchr(generate.c_str(), ' ');
      std::string path;
      path.assign(generate.c_str(),
		  space ? OCPI_SIZE_T_DIFF(space, generate.c_str()) : generate.length());
      if (OS::FileSystem::exists(path))
        addDep(path.c_str(), true);
      return NULL;
    }
    // Generate inputs: input files
    const char *
    generateInputs() {
      const char *err;
      for (unsigned s = 0; s < m_subCases.size(); s++) {
        bool first = true;
        ParamConfig &pc = *m_subCases[s];
        std::string env;
        for (unsigned n = 0; n < pc.params.size(); n++) {
          Param &p = pc.params[n];
          if (p.m_param && p.m_generate.empty()) {
            assert(!strchr(p.m_uValue.c_str(), '\''));
            OU::formatAdd(env, "OCPI_TEST_%s='%s' ", p.m_param->cname(), p.m_uValue.c_str());
          }
        }
        std::string file;
        for (unsigned n = 0; n < pc.params.size(); n++) {
          Param &p = pc.params[n];
          if (p.m_param && !p.m_generate.empty()) {
            if ((err = generateFile(first, "properties", "property value", s, p.m_param->m_name,
                                    p.m_generate, env, file)) ||
                (err = (p.m_param->needsNewLineBraces() ?
                        OU::file2String(p.m_uValue, file.c_str(), "{", "},{", "}") :
                        OU::file2String(p.m_uValue, file.c_str(), ','))))
              return err;
            OU::formatAdd(env, "OCPI_TEST_%s='%s' ", p.m_param->cname(), p.m_uValue.c_str());
            OU::formatAdd(env, "OCPI_TESTFILE_%s='%s' ", p.m_param->cname(), file.c_str());
          }
        }

        for (unsigned n = 0; n < m_ports.size(); n++) {
          InputOutput &io = m_ports[n];
          if (io.m_port) {
            if (!io.m_port->isDataProducer() && io.m_script.size()) {
              if ((err = generateFile(first, "inputs", "input port", s,
                                      io.m_port->OU::Port::m_name, io.m_script, env, file)))
                return err;
            }
          }
        }
      }
      return NULL;
    }
    void
    generateAppInstance(Worker &w, ParamConfig &pc, unsigned nOut, unsigned nOutputs, unsigned s,
                        const DataPort *first, bool a_emulator, std::string &app, const char *dut, bool testingOptional) {
      OU::formatAdd(app, "  <instance component='%s' name='%s'", w.m_specName, dut);
      if (nOut == 1 && !testingOptional) {
        if (nOutputs == 1)
          OU::formatAdd(app, " connect='bp'");
        else
          OU::formatAdd(app, " connect='bp_%s_%s'", dut, first->pname());
      }
      bool any = false;
      for (unsigned n = 0; n < pc.params.size(); n++) {
        Param &p = pc.params[n];
        // FIXME: Should m_uValues hold the results of the generate? (AV-3114)
        if (p.m_param && !p.m_isTest &&
            ((p.m_uValues.size() && p.m_uValue.size()) || p.m_generate.size())) {
	  // If the property is specific to the emulator, don't apply it to the DUT
          if (p.m_worker && p.m_worker->m_emulate && !w.m_emulate)
            continue;
	  // The emulator should not get the dut's hidden properties
	  // FIXME: this is overly broad since users may use "hidden" and
	  // this might present things like "debug" applying to the emulator
          if (w.m_emulate && p.m_param->m_isHidden)
            continue;
          if (p.m_param->m_isImpl && p.m_param->m_default) {
            std::string uValue;
            p.m_param->m_default->unparse(uValue);
            if (uValue == p.m_uValue)
              continue;
          }
          assert(!strchr(p.m_uValue.c_str(), '\''));
          if (!any)
            app += ">\n";
          any = true;
          OU::formatAdd(app, "    <property name='%s' ", p.m_param->cname());
          if (p.m_generate.empty())
            OU::formatAdd(app, "value='%s'", p.m_uValue.c_str());
          else
            OU::formatAdd(app, "valueFile='../../gen/properties/%s.%02u.%s'",
                          m_name.c_str(), s, p.m_param->cname());
          app += "/>\n";
        }
      }
      if (!a_emulator)
        app += m_delays;
      app += any ? "  </instance>\n" : "/>\n";
    }

    // Generate application xml files, being careful not to write files that are
    // not changing
    const char *
    generateApplications(const std::string &dir, Strings &files) {
      const char *err;
      const char *dut = strrchr(wFirst->m_specName, '.');
      bool isOptional;
      if (dut)
        dut++;
      else
        dut = wFirst->m_specName;
      const char *em =  emulator ? strrchr(emulator->m_specName, '.') : NULL;
      if (em)
        em++;
      else if (emulator)
        em = emulator->m_specName;
      for (unsigned s = 0; s < m_subCases.size(); s++) {
        ParamConfig &pc = *m_subCases[s];
        OS::FileSystem::mkdir(dir, true);
        std::string name;
        OU::format(name, "%s.%02u.xml", m_name.c_str(), s);
        files.insert(name);
        std::string file(dir + "/" + name);;
        unsigned nOutputs = 0, nInputs = 0, nEmIn = 0, nEmOut = 0, nWIn = 0, nWOut = 0;
        const DataPort *first = NULL, *firstEm = NULL;
        for (unsigned n = 0; n < m_ports.size(); n++)
          if (m_ports[n].m_port) {
	    const DataPort &p = *m_ports[n].m_port;
            if (p.isDataProducer()) {
              if (!first)
                first = &p;
              nOutputs++;
              if (&p.worker() == wFirst)
                nWOut++;
              else if (&p.worker() == emulator) {
                if (!firstEm)
                  firstEm = &p;
                nEmOut++;
              } else
                assert("port is neither worker or emulator?" == 0);
            } else {
              nInputs++;
              if (&p.worker() == wFirst)
                nWIn++;
              else if (&p.worker() == emulator)
                nEmIn++;
              else
                assert("port is neither worker or emulator?" == 0);
            }
            isOptional = m_ports[n].m_testOptional;
          }
        std::string app("<application");
        // the testrun.sh script has the name "file_write_from..." or "file_write" hardcoded, so
        // the name of the file_write is limited to those options

// if (w.findPort(m_name.c_str())) {


        if ((optionals.size() >= nOutputs) && isOptional && !finishPort)
          OU::formatAdd(app, " done='%s'", dut);
        else if (nOutputs == 1)
          OU::formatAdd(app, " done='file_write'");
        else if (nOutputs > 1) {
          if (finishPort && wFirst->findPort(finishPort)) {
            OU::formatAdd(app, " done='file_write_from_%s'", finishPort);
          } else if (finishPort && emulator->findPort(finishPort)) {
            OU::formatAdd(app, " done='file_write_from_%s'", finishPort);
          } else {
            OU::formatAdd(app, " done='file_write_from_%s'", (firstEm ? firstEm : first)->pname());
          }
        }
        app += ">\n";
        if (nInputs)
          for (unsigned n = 0; n < m_ports.size(); n++)
            if (!m_ports[n].m_port->isDataProducer()) {
              InputOutput &io = m_ports[n];
              if (!io.m_testOptional) {
                if (&io.m_port->worker() == emulator ) {
                  OU::formatAdd(app, "  <instance component='ocpi.core.file_read' connect='%s_ms_%s'", em, io.m_port->pname());
                } else {
                  OU::formatAdd(app, "  <instance component='ocpi.core.file_read' connect='%s_ms_%s'", dut, io.m_port->pname());
                }
                if (io.m_messageSize)
                  OU::formatAdd(app, " buffersize='%zu'", io.m_messageSize);
        	      app += ">\n";
                std::string l_file;
                if (io.m_file.size())
                  OU::formatAdd(l_file, "%s%s", io.m_file[0] == '/' ? "" : "../../", io.m_file.c_str());
                else
                  OU::formatAdd(l_file, "../../gen/inputs/%s.%02u.%s", m_name.c_str(), s, io.m_port->pname());
                OU::formatAdd(app, "    <property name='filename' value='%s'/>\n", l_file.c_str());
                if (io.m_messageSize)
                  OU::formatAdd(app, "    <property name='messageSize' value='%zu'/>\n", io.m_messageSize);
                if (io.m_messagesInFile)
                  OU::formatAdd(app, "    <property name='messagesInFile' value='true'/>\n");
                if (io.m_suppressEOF)
                  OU::formatAdd(app, "    <property name='suppressEOF' value='true'/>\n");
                app += "  </instance>\n";
                if (&io.m_port->worker() == emulator ) {
                  OU::formatAdd(app, "  <instance component='ocpi.core.metadata_stressor' name='%s_ms_%s' connect='%s'", em, io.m_port->pname(), em);
                } else {
                  OU::formatAdd(app, "  <instance component='ocpi.core.metadata_stressor' name='%s_ms_%s' connect='%s'", dut, io.m_port->pname(), dut);
                }
                if (nInputs > 1)
                  OU::formatAdd(app, " to='%s'",  io.m_port->pname());
                app += ">\n";
                if (io.m_msMode == full)
                  app += "    <property name='mode' value='full'/>\n"
                         "    <property name='enable_give_lsfr' value='true'/>\n"
                         "    <property name='enable_take_lsfr' value='true'/>\n"
                         "    <property name='insert_nop' value='true'/>\n";
                else if (io.m_msMode == throttle)
                  app += "    <property name='mode' value='data'/>\n"
                         "    <property name='enable_give_lsfr' value='true'/>\n"
                         "    <property name='enable_take_lsfr' value='true'/>\n";
                else if (io.m_msMode == metadata)
                  app += "    <property name='mode' value='metadata'/>\n";
                app += "  </instance>\n";
              }
            }
        generateAppInstance(*wFirst, pc, nWOut, nOutputs, s, first, false, app, dut, isOptional);
        if (emulator)
          generateAppInstance(*emulator, pc, nEmOut, nOutputs, s, firstEm, true, app, em, false);
        if (nOutputs)
          for (unsigned n = 0; n < m_ports.size(); n++) {
            InputOutput &io = m_ports[n];
            const DataPort &p = *m_ports[n].m_port;
            if (io.m_port->isDataProducer()) {
              if (!io.m_testOptional) {
              OU::formatAdd(app, "  <instance component='ocpi.core.backpressure'");
              if (nOutputs > 1) {
                if (&p.worker() == emulator)
                  OU::formatAdd(app, " name='bp_%s_%s' connect='file_write_from_%s'", em,
                                io.m_port->pname(),  io.m_port->pname());
                else
                  OU::formatAdd(app, " name='bp_%s_%s' connect='file_write_from_%s'", dut,
                                io.m_port->pname(), io.m_port->pname());
              } else {
                OU::formatAdd(app, " name='bp' connect='file_write'");
              }
              app += ">\n";
              if (!io.m_disableBackpressure)
                OU::formatAdd(app, "    <property name='enable_select' value='true'/>\n");
              app += "  </instance>\n";
              OU::formatAdd(app, "  <instance component='ocpi.core.file_write'");
              // the testrun.sh script has the name "file_write_from..." or "file_write" hardcoded, so
              // the name of the file_write is limited to those options
              if (nOutputs > 1) {
                if (&p.worker() == emulator)
                  OU::formatAdd(app, " name='file_write_from_%s'", io.m_port->pname());
                else
                  OU::formatAdd(app, " name='file_write_from_%s'", io.m_port->pname());
              }
              if (!io.m_messagesInFile && io.m_stopOnEOF)
                app += "/>\n";
              else {
                app += ">\n";
                if (io.m_messagesInFile)
                  OU::formatAdd(app, "    <property name='messagesInFile' value='true'/>\n");
                if (!io.m_stopOnEOF)
                  OU::formatAdd(app, "    <property name='stopOnEOF' value='false'/>\n");
                app += "  </instance>\n";
              }
              if (&io.m_port->worker() == wFirst && nWOut > 1)
                OU::formatAdd(app,
                              "  <connection>\n"
                              "    <port instance='%s' name='%s'/>\n"
                              "    <port instance='bp_%s_%s' name='in'/>\n"
                              "  </connection>\n",
                              dut, io.m_port->pname(),
                              dut, io.m_port->pname());
              if (&io.m_port->worker() == emulator && nEmOut > 1)
                OU::formatAdd(app,
                              "  <connection>\n"
                              "    <port instance='%s' name='%s'/>\n"
                              "    <port instance='bp_%s_%s' name='in'/>\n"
                              "  </connection>\n",
                              em, io.m_port->pname(),
                              em, io.m_port->pname());
              }
            }
          }
        app += "</application>\n";
        ocpiDebug("Creating application file in %s containing:\n%s",
                  file.c_str(), app.c_str());
        if ((err = OU::string2File(app.c_str(), file.c_str(), false, true)))
          return err;
      }
      return NULL;
    }
    
    // Generate the verification script for this case
    const char *
    generateVerification(const std::string &dir, Strings &files) {
      std::string name;
      OU::format(name, "verify_%s.sh", m_name.c_str());
      files.insert(name);
      std::string file(dir + "/" + name);
      std::string verify;
      OU::format(verify,
                 "#!/bin/sh --noprofile\n"
                 "# Verification and/or viewing script for case: %s\n"
                 "# Args are: <worker> <subcase> <verify> <view>\n"
                 "# Act like a normal process if get this signal\n"
                 "trap 'exit 130' SIGINT\n"
                 "function isPresent {\n"
                 "  local key=$1\n"
                 "  shift\n"
                 "  local vals=($*)\n"
                 "  for i in $*; do if [ \"$key\" = \"$i\" ]; then return 0; fi; done\n"
                 "  return 1\n"
                 "}\n"
                 "worker=$1; shift\n"
                 "subcase=$1; shift\n"
                 "! isPresent run $* || run=run\n"
                 "! isPresent view $* || view=view\n"
                 "! isPresent verify $* || verify=verify\n"
                 "if [ -n \"$verify\" ]; then\n"
                 "  if [ -n \"$view\" ]; then\n"
                 "    msg=\"Viewing and verifying\"\n"
                 "  else\n"
                 "    msg=Verifying\n"
                 "  fi\n"
                 "elif [ -n \"$view\" ]; then\n"
                 "  msg=Viewing\n"
                 "else\n"
                 "  exit 1\n"
                 "fi\n"
                 "eval export OCPI_TESTCASE=%s\n"
                 "eval export OCPI_TESTSUBCASE=$subcase\n",
                 m_name.c_str(), m_name.c_str());
      verify += "exitval=0\n";
      size_t len = verify.size();
      for (unsigned n = 0; n < m_ports.size(); n++) {
        InputOutput &io = m_ports[n];
        if (io.m_port->isDataProducer()) {
          if (io.m_script.size() || io.m_view.size() || io.m_file.size()) {
            OU::formatAdd(verify,
                          "echo '  '$msg case %s.$subcase for worker \"$worker\" using %s on"
                          " output file:  %s.$subcase.$worker.%s.out\n"
                          "while read comp name value; do\n"
                          "  [ $comp = \"%s\"%s%s%s ] && eval export OCPI_TEST_$name=\\\"$value\\\"\n"
                          "done < %s.$subcase.$worker.props\n",
                          m_name.c_str(), io.m_script.size() ? "script" : "file comparison",
                          m_name.c_str(), io.m_port->pname(),
                          strrchr(specName.c_str(), '.') + 1,
                          emulator ? " -o $comp = \"" : "",
                          emulator ? strrchr(emulator->m_specName, '.') + 1 : "",
                          emulator ? "\"" : "",
                          m_name.c_str());
            // Put the value of any test properties into the environment according to the subcase
            bool firstTest = true;
            for (unsigned s = 0; s < m_subCases.size(); s++) {
              bool firstSubCase = true;
              ParamConfig &pc = *m_subCases[s];
              for (unsigned nn = 0; nn < pc.params.size(); nn++) {
                Param &sp = pc.params[nn];
                if (sp.m_param && sp.m_isTest) {
                  if (firstTest) {
                    firstTest = false;
                    OU::formatAdd(verify, "case $subcase in\n");
                  }
                  if (firstSubCase) {
                    firstSubCase = false;
                    OU::formatAdd(verify, "  (%02u)", s);
                  } else
                    verify += ";";
                  OU::formatAdd(verify, " export OCPI_TEST_%s='%s'",
                                sp.m_param->cname(), sp.m_uValue.c_str());
                }
              }
              if (!firstSubCase)
                verify += ";;\n";
            }
            if (!firstTest)
              verify += "esac\n";
            std::string inArgs;
            for (unsigned nn = 0; nn < m_ports.size(); nn++) {
              InputOutput &in = m_ports[nn];
              if (!in.m_port->isDataProducer()) {
		if (in.m_file.size())
		  OU::formatAdd(inArgs, " %s%s",
				in.m_file[0] == '/' ? "" : "../../", in.m_file.c_str());
		else
		  OU::formatAdd(inArgs, " ../../gen/inputs/%s.$subcase.%s",
				m_name.c_str(), in.m_port->pname());
	      }
            }
            if (io.m_view.size())
              OU::formatAdd(verify, "[ -z \"$view\" ] || %s%s %s.$subcase.$worker.%s.out %s\n",
                            io.m_view[0] == '/' ? "" : "../../",
                            io.m_view.c_str(), m_name.c_str(), io.m_port->pname(),
                            inArgs.c_str());
            if (io.m_script.size() || io.m_file.size()) {
              OU::formatAdd(verify, "[ -z \"$verify\" ] || {\n");
              std::string out;
              OU::format(out, "%s.$subcase.$worker.%s.out", m_name.c_str(), io.m_port->pname());
              if (io.m_script.size())
                OU::formatAdd(verify,
                  "  PATH=../..:../../$OCPI_TOOL_DIR:$OCPI_PROJECT_DIR/scripts:$PATH %s %s %s %s\n",
                  "PYTHONPATH=$OCPI_PROJECT_DIR/scripts:$PYTHONPATH ",
                  io.m_script.c_str(), out.c_str(), inArgs.c_str());
              else
                OU::formatAdd(verify,
                              "  echo '    'Comparing output file to specified file: \"%s\"\n"
                              "  cmp %s %s%s\n",
                              io.m_file.c_str(), out.c_str(),
                              io.m_file[0] == '/' ? "" : "../../", io.m_file.c_str());
              OU::formatAdd(verify,
                            "  r=$?\n"
                            "  tput bold 2>/dev/null\n"
                            "  if [ $r = 0 ] ; then \n"
                            "    tput setaf 2 2>/dev/null\n"
                            "    echo '    Verification for port %s: PASSED'\n"
                            "  else\n"
                            "    tput setaf 1 2>/dev/null\n"
                            "    echo '    Verification for port %s: FAILED'\n"
                            "    failed=1\n"
                            "  fi\n"
                            "  tput sgr0 2>/dev/null\n"
                            "  [ $r = 0 ] || exitval=1\n"
                            "}\n", io.m_port->pname(), io.m_port->pname());
            } else
              OU::formatAdd(verify,
                            "echo  ***No actual verification is being done.  Output is: $*\n");
          }
        }
      }
      if (len == verify.size())
        verify += "echo '  Verification was not run since there are no output ports.'\n";
      verify += "exit $exitval\n";
      return OU::string2File(verify.c_str(), file.c_str(), false, true, true);
    }
    const char *
    generateCaseXml(FILE *out) {
      fprintf(out, "  <case name='%s'>\n", m_name.c_str());
      for (unsigned s = 0; s < m_subCases.size(); s++) {
        ParamConfig &pc = *m_subCases[s];
        // Figure out which platforms will not support this subcase
        assert(pc.params.size() == m_settings.params.size());
        Strings excludedPlatforms; // for the subcase
        Strings excludedPlatformsMerge; //need a temp variable when merging subcase and test platforms
        for (unsigned n = 0; n < pc.params.size(); n++) {
          Param
            &cp = m_settings.params[n], // case param
            &sp = pc.params[n];         // subcase param
          assert((cp.m_param && sp.m_param) || (!cp.m_param && !sp.m_param));
          // We now support unset values (e.g. raw config registers...)
          //      assert(!cp.m_param || cp.m_uValues.size() || cp.m_generate.size());
          // AV-3372: If cp is generated, there's likely an empty string sitting in cp.m_uValues.
          if (cp.m_generate.size() and (cp.m_uValues.size() == 1) and cp.m_uValues[0].empty() ) {
            ocpiDebug("Erasing false empty value for generated %s", cp.m_param->pretty());
            cp.m_uValues.clear();
          }
          if (!cp.m_param || cp.m_uValues.empty()) // empty is generated - no exclusions
            continue;
          Param::Attributes *attrs = NULL;
          for (unsigned i = 0; !attrs && i < cp.m_uValues.size(); i++) {
            if (sp.m_uValue == cp.m_uValues[i]) {
              attrs = &cp.m_attributes[i];
            }
          }
          assert(attrs);
          for (auto si = allPlatforms.begin(); si != allPlatforms.end(); ++si) {
            const char *p = si->c_str();
            // allowed platform for this test? if platform not in global onlyPlatforms
            // list it shouldn't be tested anyway
            if (onlyPlatforms.size() && onlyPlatforms.find(p) == onlyPlatforms.end())
              continue;
            // If all values for this platform are not explicit
            if (cp.m_explicitPlatforms.find(p) == cp.m_explicitPlatforms.end()) {
              if (attrs->m_excluded.find(p) != attrs->m_excluded.end() ||
                  (attrs->m_included.size() && attrs->m_included.find(p) == attrs->m_included.end())) {
                excludedPlatformsMerge.insert(p);
              }
            } else if (attrs->m_only.find(p) == attrs->m_only.end()) {
              // This value is not specifically set for this platform.  Exclude the platform.
              excludedPlatformsMerge.insert(p);
            }
          }
        }
        // add per-case excluded platforms from the test xml to the list
        if (excludedPlatformsMerge.size() > 0) {
          excludedPlatforms.insert(excludedPlatformsMerge.begin(), excludedPlatformsMerge.end());
        }
        if (m_excludePlatforms.size() > 0){
          excludedPlatforms.insert(m_excludePlatforms.begin(), m_excludePlatforms.end());
        }

        // Now that all platforms exclusions have been collected, generate list
        fprintf(out, "    <subcase id='%u'", s);
        if (excludedPlatforms.size() && !onlyPlatforms.size() && !m_onlyPlatforms.size()) {
          fprintf(out, " exclude='");
          for (auto si = excludedPlatforms.begin(); si != excludedPlatforms.end(); ++si) {
            fprintf(out, "%s%s", si == excludedPlatforms.begin() ? "" : " ", si->c_str());
          }
    	  fprintf(out, "'");
        }
        // Now we know which platforms should be included
        if (m_onlyPlatforms.size()) {
          fprintf(out, " only='");
          for (auto si = m_onlyPlatforms.begin(); si != m_onlyPlatforms.end(); ++si) {
            fprintf(out, "%s%s", si == m_onlyPlatforms.begin() ? "" : " ", si->c_str());
          }
          fprintf(out, "'");
        } else if (onlyPlatforms.size()) {
          fprintf(out, " only='");
          for (auto si = onlyPlatforms.begin(); si != onlyPlatforms.end(); ++si) {
            const char *p = si->c_str();
            if (m_excludePlatforms.size()  && m_excludePlatforms.find(p) != m_excludePlatforms.end())
              continue;
            fprintf(out, "%s%s", si == onlyPlatforms.begin() ? "" : " ", p);
          }
          fprintf(out, "'");
        }
        if (m_timeout)
          fprintf(out, " timeout='%zu'", m_timeout);
        if (m_duration)
          fprintf(out, " duration='%zu'", m_duration);
        fprintf(out, ">\n");
        // For each worker configuration, decide whether it can support the subcase.
        // Note worker configs only have *parameters*, while subcases can have runtime properties
        for (WorkerConfigsIter wci = configs.begin(); wci != configs.end(); ++wci) {
          ocpiDebug("  For case xml for %s.%u worker %s.%s(%zu)", m_name.c_str(), s,
                    wci->second->cname(), wci->second->m_modelString, wci->first->nConfig);
          ParamConfig &wcfg = *wci->first;
          // Only use worker configurations that make use of this case's workers
          if (std::find(m_workers.begin(), m_workers.end(), wci->second) == m_workers.end())
              goto skip_worker_config;
          // For each property in the subcase, decide whether it conflicts with a *parameter*
          // in this worker config
          for (unsigned nn = 0; nn < pc.params.size(); nn++) {
            Param &sp = pc.params[nn];
            if (sp.m_param == NULL)
              continue;
            OU::Property *wprop = wci->second->findProperty(sp.m_param->cname());
            for (unsigned n = 0; n < wcfg.params.size(); n++) {
              Param &wparam = wcfg.params[n];
              if (wparam.m_param && !strcasecmp(sp.m_param->cname(), wparam.m_param->cname())) {
                if (sp.m_uValue == wparam.m_uValue)
                  goto next;     // match - this subcase property is ok for this worker config
                ocpiDebug("--Skipping worker %s.%s(%zu) because its param %s is different",
                          wci->second->cname(), wci->second->m_modelString,
                          wci->first->nConfig, sp.m_param->cname());
                goto skip_worker_config; // mismatch - this worker config rejected from subcase
              }
            }
            // The subcase property was not found as a parameter in the worker config
            if (wprop) {
              // But it is a runtime property in the worker config so it is ok
              assert(!wprop->m_isParameter);
              continue;
            }
            // The subcase property is for the emulator, which is ok
            if (sp.m_worker && sp.m_worker->m_emulate)
              continue;
            // The subcase property was not in this worker at all, so it must be
            // implementation specific or a test property
            if (sp.m_param->m_isImpl && sp.m_uValue.size()) {
              if (sp.m_param->m_default) {
                std::string uValue;
                sp.m_param->m_default->unparse(uValue);
                if (sp.m_uValue == uValue)
                  // The subcase property is the default value so it is ok for the worker
                  // to not have it at all.
                  continue;
              }
              // The impl property is not the default so this worker config cannot be used
              ocpiDebug("--Skipping worker %s.%s(%zu) because param %s(%s) is impl-specific and %s",
                        wci->second->cname(), wci->second->m_modelString, wci->first->nConfig,
                        sp.m_param->cname(), sp.m_uValue.c_str(),
                        sp.m_param->m_default ? " the value does not match the default" :
                        "there is no default to check");
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
              if (m_ports[n].m_port->isDataProducer() & !m_ports[n].m_testOptional) { //have to delete optional ports being tested
                OU::formatAdd(ports, "%s%s", first ? "" : " ", m_ports[n].m_port->pname());
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
      wkrName(wname, OCPI_SIZE_T_DIFF(dot, wname)),
      wOWD = wdir + "/" + wkrName + ".xml";
    if (!OS::FileSystem::exists(wOWD, &isDir) || isDir)
      return OU::esprintf("For worker \"%s\", \"%s\" doesn't exist or is a directory",
                          name, wOWD.c_str());
    return tryWorker(name, specName, true, true);
  }
  // Explicitly excluded workers
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
    std::string workerNames;
    const char *err;
    if ((err = OU::file2String(workerNames, "../lib/workers", ' ')))
      return err;
    addDep("../lib/workers", false); // if we add or remove a worker from the library...
    for (OU::TokenIter ti(workerNames.c_str()); ti.token(); ti.next())
      if ((err = tryWorker(ti.token(), specName, true, false)))
        return err;
    return NULL;
  }

  void connectHdlFileIO(const Worker &w, std::string &assy, InputOutputs &ports) {
    for (PortsIter pi = w.m_ports.begin(); pi != w.m_ports.end(); ++pi) {
      Port &p = **pi;
      bool optional = false;
      InputOutput *ios = findIO(p, ports);
      if (ios)
        optional = ios->m_testOptional;
      if (p.isData() && !optional) {
          OU::formatAdd(assy,
                        "  <Instance name='%s_%s' Worker='file_%s'/>\n"
                        "  <Connection>\n"
                        "    <port instance='%s_%s' %s='%s'/>\n"
                        "    <port instance='%s%s%s' %s='%s'/>\n"
                        "  </Connection>\n",
                        w.m_implName, p.pname(), p.isDataProducer() ? "write" : "read",
                        w.m_implName, p.pname(), p.isDataProducer() ? "to" : "from",
                        p.isDataProducer() ? "in" : "out", w.m_implName,
                        p.isDataProducer() ? "_backpressure_" : "_ms_", p.pname(),
  		      p.isDataProducer() ? "from" : "to", p.isDataProducer() ? "out" : "in");
      }
    }
  }

  void connectHdlStressWorkers(const Worker &w, std::string &assy, bool hdlFileIO, InputOutputs &ports) {
    for (PortsIter pi = w.m_ports.begin(); pi != w.m_ports.end(); ++pi) {
      Port &p = **pi;
      bool optional = false;
      InputOutput *ios = findIO(p, ports);
      if (ios)
        optional = ios->m_testOptional;
      if (p.isData() && !optional) {
        if (p.isDataProducer()) {
          OU::formatAdd(assy,
                        "  <Instance Name='%s_backpressure_%s' Worker='backpressure'/>\n",
                        w.m_implName, p.pname());
          OU::formatAdd(assy,
                        "  <Connection>\n"
                        "    <port instance='uut_%s' name='%s'/>\n"
                        "    <port instance='%s_backpressure_%s' name='in'/>\n"
                        "  </Connection>\n", w.m_implName, p.pname(), w.m_implName, p.pname());
          if (!hdlFileIO) {
            OU::formatAdd(assy,
                          "  <Connection Name='%s_backpressure_%s' External='producer'>\n"
                          "    <port Instance='%s_backpressure_%s' Name='out'/>\n"
                          "  </Connection>\n", p.pname(), w.m_implName, w.m_implName, p.pname());
          }
        } else {
          OU::formatAdd(assy,
                        "  <Instance Name='%s_ms_%s' Worker='metadata_stressor'/>\n",
                        w.m_implName, p.pname());
          OU::formatAdd(assy,
                        "  <Connection>\n"
                        "    <port instance='%s_ms_%s' name='out'/>\n"
                        "    <port instance='uut_%s' name='%s'/>\n"
                        "  </Connection>\n", w.m_implName, p.pname(), w.m_implName, p.pname());
          if (!hdlFileIO) {
            OU::formatAdd(assy,
                          "  <Connection Name='%s_ms_%s' External='consumer'>\n"
                          "    <port Instance='%s_ms_%s' Name='in'/>\n"
                          "  </Connection>\n",  p.pname(),  w.m_implName, w.m_implName, p.pname());
          }
        }
      }
    }
  }

  const char *generateHdlAssembly(const Worker &w, unsigned c, const std::string &dir, const
                                    std::string &name, bool hdlFileIO, Strings &assyDirs, InputOutputs &ports) {
    OS::FileSystem::mkdir(dir, true);
    assyDirs.insert(name);
    const char *err;
    if ((err = OU::string2File(hdlFileIO ?
                     "override HdlPlatform:=$(filter %sim,$(HdlPlatform))\n"
                     "override HdlPlatforms:=$(filter %sim,$(HdlPlatforms))\n"
                     "include $(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk\n" :
                     "include $(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk\n",
                     dir + "/Makefile", false, true)))
        return err;
    std::string assy;
    OU::format(assy,
               "<HdlAssembly%s>\n"
               "  <Instance Worker='%s' Name='uut_%s' ParamConfig='%u'/>\n",
               emulator ? " language='vhdl'" : "", w.m_implName, w.m_implName, c);
      if (emulator) {
        OU::formatAdd(assy, "  <Instance Worker='%s' Name='uut_%s' ParamConfig='%u'/>\n",
                    emulator->m_implName, emulator->m_implName, c);
        for (unsigned n = 0; n < w.m_ports.size(); n++) {
          Port &p = *w.m_ports[n];
          if (p.m_type == DevSigPort || p.m_type == PropPort)
            OU::formatAdd(assy,
                        "  <Connection>\n"
                        "    <port instance='uut_%s' name='%s'/>\n"
                        "    <port instance='uut_%s' name='%s'/>\n"
                        "  </Connection>\n",
                        w.m_implName, p.pname(), emulator->m_implName, p.pname());
        }
      }
      connectHdlStressWorkers(w, assy, hdlFileIO, ports);
      if(emulator)
        connectHdlStressWorkers(*emulator, assy, hdlFileIO, ports);
      if (hdlFileIO) {
        connectHdlFileIO(w, assy, ports);
        if (emulator)
          connectHdlFileIO(*emulator, assy, ports);
      }
      assy += "</HdlAssembly>\n";
      return OU::string2File(assy, dir + "/" + name + ".xml", false, true);
    }
} // end of anonymous namespace

// Called for all workers globally acceptable workers and the one emulator if present
static void
addNonParameterProperties(Worker &w, ParamConfig &globals) {
  for (PropertiesIter pi = w.m_ctl.properties.begin(); pi != w.m_ctl.properties.end(); ++pi) {
    OU::Property &p = **pi;
    if (p.m_isParameter || !p.m_isWritable)
      continue;
    std::string name;
    Param::fullName(p, &w, name);
    bool found = false;
    for (unsigned n = 0; n < globals.params.size(); n++) {
      Param &param = globals.params[n];
      if (param.m_param && !strcasecmp(param.m_name.c_str(), name.c_str())) {
        found = true;
        break;
      }
    }
    if (found)
      continue;
    // New, non-parameter property never seen - could be worker-specific
    globals.params.resize(globals.params.size()+1);
    Param &param = globals.params.back();
    param.setProperty(&p, p.m_isImpl || w.m_emulate ? &w : NULL);
    if (p.m_default) {
      p.m_default->unparse(param.m_uValue);
      param.m_uValues.resize(1);
      param.m_attributes.resize(1);
      param.m_uValues[0] = param.m_uValue;
    }
  }
}
const char *
createTests(const char *file, const char *package, const char */*outDir*/, bool a_verbose) {
  verbose = a_verbose;
  const char *err;
  std::string parent, specFile;
  ezxml_t xml, xspec;
  if (!file || !file[0]) {
    static char x[] = "<tests/>";
    xml = ezxml_parse_str(x, strlen(x));
  } else if ((err = parseFile(file, parent, "tests", &xml, testFile, false, false, false)) ||
             (err = OE::checkAttrs(xml, "spec", "timeout", "duration", "onlyWorkers",
                                   "excludeWorkers", "useHDLFileIo", "mode", "onlyPlatforms",
                                   "excludePlatforms", "finishPort", NULL)) ||
             (err = OE::checkElements(xml, "property", "case", "input", "output", NULL)))
    return err;
  // ================= 1. Get the spec
  if ((err = getSpec(xml, testFile, package, xspec, specFile, specName)) ||
      (err = OE::getNumber(xml, "duration", &duration)) ||
      (err = OE::getNumber(xml, "timeout", &timeout)))
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
      fprintf(stderr, "Warning:  excluded worker \"%s\" never found.\n", si->c_str());
  // ================= 3. Get/collect the worker parameter configurations
  // Now "workers" has workers with parsed build files.
  // So next we globally enumerate PCs independent of them, that might be dependent on them.
  if (workers.empty()) { // AV-3369 (and possibly others)
    if (matchedWorkers) {
      const char *e =
        "Workers were found that matched the spec, but none were built, so no tests generated";
      if (verbose)
        fprintf(stderr, "%s\n", e);
      ocpiInfo("%s", e);
      return NULL;
    }
    return OU::esprintf("There are currently no valid workers implementing %s", specName.c_str());
  }
  // But first!... we create the first one from the defaults.
  wFirst = *workers.begin();
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
    ocpiDebug("Processing config %zu of worker %s.%s",
              pc.nConfig, pc.worker().cname(), pc.worker().m_modelString);
    for (unsigned n = 0; n < pc.params.size(); n++) {
      Param &p = pc.params[n];
      if (p.m_param == NULL)
        continue;
      assert(p.m_param->m_isParameter);
      size_t pn;
      assert(p.m_name.length());
      // See if the (implementation-qualified) name is in the globals yet
      unsigned nn;
      for (nn = 0; nn < globals.params.size(); nn++) {
        Param &gp = globals.params[nn];
        if (!gp.m_param)
          continue;
        assert(gp.m_name.length());
        if (!strcasecmp(p.m_name.c_str(), globals.params[nn].m_name.c_str())) {
          pn = nn;
          break;
        }
      }
      if (nn >= globals.params.size()) {
        pn = globals.params.size();
        globals.params.push_back(p); // add to end
        globals.params.back().m_worker = &pc.worker();// remember which worker it came from
      }
      Param &gp = globals.params[pn];
      if (!gp.m_param)
        gp.setProperty(p.m_param, p.m_worker);
      ocpiDebug("existing value for %s(%u) is %s(%zu)",
                p.m_param->cname(), n, p.m_uValue.c_str(), pn);
      for (nn = 0; nn < gp.m_uValues.size(); nn++)
        if (p.m_uValue == gp.m_uValues[nn])
          goto next;
      if (!gp.m_valuesType) {
        gp.m_valuesType = &p.m_param->sequenceType();
        gp.m_valuesType->m_default = new OU::Value(*gp.m_valuesType);
      }
      gp.m_valuesType->m_default->parse(p.m_uValue.c_str(), NULL, gp.m_uValues.size() != 0);
      gp.m_uValues.push_back(p.m_uValue);
      gp.m_attributes.push_back(Param::Attributes());
    next:;
    }
  }
  // ================= 4a. Parse and collect global non-parameter property values from workers
  for (WorkersIter wi = workers.begin(); wi != workers.end(); ++wi)
    addNonParameterProperties(**wi, globals);
  if (emulator)
    addNonParameterProperties(*emulator, globals);
  // ================= 4b. Add "empty" values for parameters that are not in all workers
  for (WorkerConfigsIter wci = configs.begin(); wci != configs.end(); ++wci) {
    ParamConfig &pc = *wci->first;
    // Check if any properties have no values.
    for (unsigned n = 0; n < globals.params.size(); n++) {
      Param &gparam = globals.params[n];
      if (!gparam.m_param)
        continue;
      for (unsigned nn = 0; nn < pc.params.size(); nn++) {
        Param &p = pc.params[nn];
        if (!p.m_param)
          continue;
        if (!strcasecmp(gparam.m_param->cname(), p.m_param->cname()))
          goto found;
      }
      // A globally defined property was not found in this worker config, so we must add
      // an empty value if there is not one already and there is no default
      {
        std::string defValue;
        if (gparam.m_param->m_default)
          gparam.m_param->m_default->unparse(defValue);
        for (auto it = gparam.m_uValues.begin(); it != gparam.m_uValues.end(); ++it)
          if (it->empty() || (*it) == defValue)
            goto found;
      }
      // AV-3372: We shouldn't do this if the property is generated, but we cannot tell at this point if it is.
      // gparam.m_generate is empty because it's from the worker XML. We will remove this later if needed.
      ocpiDebug("Adding empty value for property %s because it is not in worker %s.%s(%zu)",
                gparam.m_param->cname(), pc.worker().cname(), pc.worker().m_modelString,
                pc.nConfig);
      gparam.m_uValues.push_back("");
      gparam.m_attributes.push_back(Param::Attributes());
    found:;
    }
  }
  // ================= 5. Parse and collect global property values specified for all cases
  // Parse explicit/default property values to apply to all cases
#define TEST_ATTRS "generate", "add", "only", "exclude", "test"
  for (ezxml_t px = ezxml_cchild(xml, "property"); px; px = ezxml_cnext(px)) {
    std::string name;
    bool isTest;
    if ((err = OE::getRequiredString(px, name, "name")) ||
        (err = OE::getBoolean(px, "test", &isTest)) ||
        (err = (isTest ?
                OE::checkAttrs(px, PARAM_ATTRS, TEST_ATTRS, OCPI_UTIL_MEMBER_ATTRS, NULL) :
                OE::checkAttrs(px, PARAM_ATTRS, TEST_ATTRS, NULL))))
      return err;
    Param *found = NULL;
    // First pass, look for correctly scoped names in the Param (including worker.model.prop).
    for (unsigned n = 0; n < globals.params.size(); n++) {
      Param &p = globals.params[n];
      if (p.m_param && !strcasecmp(name.c_str(), p.m_name.c_str())) {
        if (isTest)
          return OU::esprintf("The test property \"%s\" is already a worker property",
                              name.c_str());
        found = &p;
        break;
      }
    }
    if (!found) {
      // Second pass for backward compatibility, with warnings, when a property name is not qualified
      for (unsigned n = 0; n < globals.params.size(); n++) {
        Param &p = globals.params[n];
        if (p.m_param && !strcasecmp(name.c_str(), p.m_param->cname())) {
          if (isTest)
            return OU::esprintf("The test property \"%s\" is already a worker property",
                                name.c_str());
          if (found)
            return OU::esprintf("Ambiguous property \"%s\" is worker-specific in multiple workers (%s and %s)",
                                p.m_param->cname(), p.m_worker->cname(), found->m_worker->cname());
          found = &p;
          break;
        }
      }
      if (found) {
        assert(found->m_param && found->m_param->m_isImpl);
        fprintf(stderr, "Warning:  property \"%s\" is worker-specific and should be identified as %s.%s.%s\n",
                found->m_param->cname(), found->m_worker->cname(), found->m_worker->m_modelString,
                found->m_param->cname());
      } else if (!isTest)
        return OU::esprintf("There is no property named \"%s\" for any worker", name.c_str());
    }
    if (isTest) {
      assert(!found);
      globals.params.resize(globals.params.size()+1);
      found = &globals.params.back();
      OU::Property &newp = *new OU::Property();
      found->m_isTest = true;
      char *copy = ezxml_toxml(px);
      // Make legal property definition XML out of this xml
      ezxml_t propx;
      if ((err = OE::ezxml_parse_str(copy, strlen(copy), propx)))
        return err;
      ezxml_set_attr(propx, "test", NULL);
      ezxml_set_attr(propx, "value", NULL);
      ezxml_set_attr(propx, "values", NULL);
      ezxml_set_attr(propx, "valuefile", NULL);
      ezxml_set_attr(propx, "valuesfile", NULL);
      ezxml_set_attr(propx, "initial", "1");
      if ((err = newp.Member::parse(propx, false, true, NULL, "property", 0)))
        return err;
      found->setProperty(&newp, NULL);
      // We allow a test property to be specified with no values (values only in cases)
      if (!ezxml_cattr(px, "value") &&
          !ezxml_cattr(px, "values") &&
          !ezxml_cattr(px, "valuefile") &&
          !ezxml_cattr(px, "valuesfile") &&
          !ezxml_cattr(px, "generate"))
        continue;
    }
    if ((err = found->parse(px, NULL, NULL, true)))
      return err;
  }
  // Check if any properties have no values.
  for (unsigned n = 0; n < globals.params.size(); n++) {
    Param &param = globals.params[n];
    if (!param.m_param)
      continue;
    const OU::Property &p = *param.m_param;
    if (!p.m_isParameter && p.m_isWritable && param.m_uValues.empty())
      fprintf(stderr,
              "Warning:  no values for writable property with no default: \"%s\" %zu %zu\n",
              p.cname(), param.m_uValues.size(), param.m_uValue.size());
  }
  // ================= 6. Parse and collect global platform values
  finishPort = ezxml_cattr(xml, "finishPort");
  // Parse global platforms
  const char
    *excludes = ezxml_cattr(xml, "excludePlatforms"),
    *onlys = ezxml_cattr(xml, "onlyPlatforms");
  if (!excludes)
    excludes = ezxml_cattr(xml, "exclude");
  if (!onlys)
    onlys = ezxml_cattr(xml, "only");
  if (onlys) {
    if (excludes)
      return OU::esprintf("the only and exclude attributes cannot both occur");
    if ((err = getPlatforms(onlys, onlyPlatforms)))
      return err;
  } else if (excludes && (err = getPlatforms(excludes, excludePlatforms)))
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
              c, pc.worker().cname(), pc.worker().m_modelString);
      for (unsigned n = 0; n < pc.params.size(); n++) {
        Param &p = pc.params[n];
        if (p.m_param == NULL)
          continue;
        fprintf(stderr, "      %s = %s%s\n", p.m_param->cname(), p.m_uValue.c_str(),
                p.m_isDefault ? " (default)" : "");
      }
    }
  }

  // ================= 9. Generate report in gen/cases.txt on all combinations of property values
  OS::FileSystem::mkdir("gen", true);
  std::string summary;
  OU::format(summary, "gen/cases.txt");
  if (verbose)
    fprintf(stderr, "Writing cases/subcases report in \"%s\"\n", summary.c_str());
  FILE *out = fopen(summary.c_str(), "w");
  fprintf(out,
          "Values common to all property combinations:\n"
          "===========================================\n");
  for (unsigned n = 0; n < globals.params.size(); n++) {
    Param &p = globals.params[n];
    if (p.m_param == NULL || p.m_uValues.size() != 1 || !p.m_generate.empty())
      continue;
    p.m_uValue = p.m_uValues[0];
    fprintf(out, "      %s = %s", p.m_param->cname(),
            p.m_uValues[0].empty() ? "<no value>" : p.m_uValues[0].c_str());
    if (p.m_param->m_isImpl && strncasecmp("ocpi_", p.m_param->cname(), 5)) {
      assert(p.m_worker);
      fprintf(out, " (specific to worker %s.%s)",
              p.m_worker->m_implName, p.m_worker->m_modelString);
    }
    fprintf(out, "\n");
  }
#if 0
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
#endif
  // ================= 10. Generate HDL assemblies in gen/assemblies
  if (verbose)
    fprintf(stderr, "Generating required HDL assemblies in gen/assemblies\n");
  bool hdlFileIO;
  if ((err = OE::getBoolean(xml, "UseHdlFileIO", &hdlFileIO)))
    return err;
  const char *env = getenv("OCPI_FORCE_HDL_FILE_IO");
  if (env)
    hdlFileIO = env[0] == '1';
  bool seenHDL = false;
  Strings assyDirs;
  std::string assemblies("gen/assemblies");
  for (WorkersIter wi = workers.begin(); wi != workers.end(); ++wi) {
    Worker &w = **wi;
    if (w.m_model == HdlModel) {
      ocpiInfo("Generating assemblies for worker: %s", w.m_implName);
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
          OS::FileSystem::mkdir(assemblies, true);
          OU::string2File("include $(OCPI_CDK_DIR)/include/hdl/hdl-assemblies.mk\n",
                          (assemblies + "/Makefile").c_str(), true);
          seenHDL = true;
        }
        std::string name(w.m_implName);
        OU::formatAdd(name, "_%u", c);
        std::string dir(assemblies + "/" + name);
        ocpiInfo("Generating assembly: %s", dir.c_str());
        //there's always at least one case if there's a -test.xml
        if ((err = generateHdlAssembly(w, c, dir, name, false, assyDirs, cases[0]->m_ports)))
          return err;
        if (testingOptionalPorts) {
          for (unsigned n = 0; n < cases.size(); n++) {
            std::ostringstream temp; //can't use to_string
            temp << n;
            if ((err = generateHdlAssembly(w, c, dir + "_op_" + temp.str(), name + "_op_" + temp.str(), false, assyDirs, cases[n]->m_ports)))
              return err;
          }
        }
        if (hdlFileIO) {
          name += "_frw";
          dir += "_frw";
          if ((err = generateHdlAssembly(w, c, dir, name, true, assyDirs, cases[0]->m_ports)))
            return err;
          if (testingOptionalPorts) {
            for (unsigned n = 0; n < cases.size(); n++) {
              std::ostringstream temp; //can't use to_string
              temp << n;
              if ((err = generateHdlAssembly(w, c, dir + "_op_" + temp.str(), name + "_op_" + temp.str(), true, assyDirs, cases[n]->m_ports)))
                return err;
            }
          }
        }
      }
    }
  }
  // Cleanup any assemblies that were not just generated
  assyDirs.insert("Makefile");
  for (OS::FileIterator iter(assemblies, "*"); !iter.end(); iter.next())
    if (assyDirs.find(iter.relativeName()) == assyDirs.end() &&
        (err = remove(assemblies + "/" + iter.relativeName())))
      return err;

  // ================= 10. Generate subcases for each case, and generate outputs per subcase
  fprintf(out,
          "\n"
          "Descriptions of the %zu case%s and %s subcases:\n"
          "==============================================\n",
          cases.size(), cases.size() > 1 ? "s" : "", cases.size() > 1 ? "their" : "its");
  for (unsigned n = 0; n < cases.size(); n++) {
    cases[n]->m_subCases.push_back(new ParamConfig(cases[n]->m_settings));
    cases[n]->doProp(0);
    if ((err = cases[n]->pruneSubCases()))
      return err;
    cases[n]->print(out);
  }
  fclose(out);
  if (verbose)
    fprintf(stderr, "Generating required input and property value files in gen/inputs/ and "
            "gen/properties/\n");
  for (unsigned n = 0; n < cases.size(); n++)
    if ((err = cases[n]->generateInputs()))
      return err;
  std::string dir("gen/applications");
  if (verbose)
    fprintf(stderr, "Generating required application xml files in %s/\n", dir.c_str());
  Strings appFiles;
  for (unsigned n = 0; n < cases.size(); n++)
    if ((err = cases[n]->generateApplications(dir, appFiles)) ||
        (err = cases[n]->generateVerification(dir, appFiles)))
      return err;
  for (OS::FileIterator iter(dir, "*"); !iter.end(); iter.next())
    if (appFiles.find(iter.relativeName()) == appFiles.end() &&
        (err = remove(dir + "/" + iter.relativeName())))
      return err;
#if 1
  if ((err = openOutput("cases", "gen", "", "", ".xml", NULL, out)))
    return err;
#else
  out = fopen("gen/cases.xml", "w");
  if (!out)
    return OU::esprintf("Failed to open summary XML file gen/cases.xml");
#endif
  if (verbose)
    fprintf(stderr, "Generating summary gen/cases.xml file\n");
  fprintf(out, "<cases spec='%s'>\n", wFirst->m_specName);
  if ((err = getPlatforms("*", allPlatforms)))
    return err;
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
    typedef std::map<std::string, std::string> Runs;
    typedef Runs::const_iterator RunsIter;
    typedef std::pair<std::string, std::string> RunsPair;
    Runs m_runs; // for this platform, what runs are done
    CallBack(const char *spec, const char *model, const char *platform, bool dynamic,
             ezxml_t xml)
      : m_spec(spec), m_model(model), m_platform(platform), m_dynamic(dynamic), m_xml(xml),
        m_first(true), m_run(NULL), m_verify(NULL), m_outputs(NULL) {
      const char *cp = strrchr(spec, '.');
      m_component = cp ? cp + 1 : spec;
    }
    ~CallBack() {
      if (m_run) {
        for (RunsIter ri = m_runs.begin(); ri != m_runs.end(); ++ri)
          fprintf(m_run, "%s", ri->second.c_str());
        fprintf(m_run, "exit $failed\n");
        fclose(m_run);
      }
      if (m_verify)
        fclose(m_verify);
    }
#if 0
    void doOutput(const char *output) {
      bool multiple = strchr(m_outputs, ',') != NULL;
      OU::formatAdd(m_outputArgs, " -pfile_write%s%s=fileName=$5.$6.$4.$3.%s.out",
                    multiple ? "_" : "", multiple ? output : "", output);
      OU::formatAdd(m_verifyOutputs, " %s", output);
    }
    static const char *doOutput(const char *output, void *me) {
      ((CallBack*)me)->doOutput(output);
      return NULL;
    }
#endif
    static bool found(const char *platform, const char *list) {
      size_t len = strlen(platform);
      const char *p = list ? strstr(list, platform) : NULL;
      return p && (!p[len] || isspace(p[len])) && (p == list || isspace(p[-1]));
    }
    static bool included(const char *platform, ezxml_t x) {
      const char
        *exclude = ezxml_cattr(x, "exclude"),
        *only = ezxml_cattr(x, "only");
      if (!exclude)
	exclude = ezxml_cattr(x, "excludeplatforms");
      if (!only)
	only = ezxml_cattr(x, "onlyplatforms");
      return
        (!exclude || !found(platform, exclude)) &&
        (!only || found(platform, only));
    }
    bool foundImplementation(const OL::Implementation &i, bool &accepted) {
      ocpiInfo("For platform %s, considering worker %s.%s from %s platform %s dynamic %u",
               m_platform.c_str(), i.m_metadataImpl.cname(), i.m_metadataImpl.model().c_str(),
               i.m_artifact.name().c_str(), i.m_artifact.platform().c_str(),
               i.m_artifact.dynamic());
      if (i.m_artifact.platform() == m_platform && i.m_metadataImpl.model() == m_model &&
          i.m_artifact.dynamic() == m_dynamic) {
        unsigned sn = 0;
        for (ezxml_t cx = ezxml_cchild(m_xml, "case"); cx; cx = ezxml_cnext(cx)) {
          unsigned n = 0;
          const char *name = ezxml_cattr(cx, "name");
          for (ezxml_t sx = ezxml_cchild(cx, "subcase"); sx; sx = ezxml_cnext(sx), n++, sn++)
            if (included(m_platform.c_str(), sx))
              for (ezxml_t wx = ezxml_cchild(sx, "worker"); wx; wx = ezxml_cnext(wx))
                if (!strcmp(i.m_metadataImpl.cname(), ezxml_cattr(wx, "name")) &&
                    i.m_metadataImpl.model() == ezxml_cattr(wx, "model")) {
                  ocpiInfo("Accepted for case %s subcase %u from file: %s", name, n,
                           i.m_artifact.name().c_str());
                  if (m_first) {
                    m_first = false;
                    std::string dir("run/" + m_platform);
                    OS::FileSystem::mkdir(dir, true);
                    std::string file(dir + "/run.sh");
                    if (verbose)
                      fprintf(stderr, "  Generating run script for platform: %s\n",
                              m_platform.c_str());
                    if (!(m_run = fopen(file.c_str(), "w"))) {
                      OU::format(m_err, "Cannot open file \"%s\" for writing", file.c_str());
                      return true;
                    }
                    fprintf(m_run,
                            "#!/bin/sh --noprofile\n"
                            "# Note that this file runs on remote/embedded systems and thus\n"
                            "# may not have access to the full development host environment\n"
                            "failed=0\n"
                            "source $OCPI_CDK_DIR/scripts/testrun.sh %s %s $* - %s\n",
                            m_spec.c_str(), m_platform.c_str(), ezxml_cattr(wx, "outputs"));
                  }
                  const char
                    *to = ezxml_cattr(sx, "timeout"),
                    *du = ezxml_cattr(sx, "duration"),
                    *w = ezxml_cattr(wx, "name"),
                    *o = ezxml_cattr(wx, "outputs");
                  std::string doit;
                  OU::format(doit, "docase %s %s %s %02u %s %s %s\n", m_model.c_str(), w, name, n,
                             to ? to : "0", du ? du : "0", o);
                  std::string key;
                  OU::format(key, "%08u %s", sn, w);
                  m_runs.insert(RunsPair(key, doit));
                }
        }
        accepted = true;
      }
      return false;
    }
  };
  const char *err;
  std::string registry, path;
  if ((err = OU::getProjectRegistry(registry)))
    return err;
  OU::format(path, "../lib/rcc:../lib/ocl:gen/assemblies:%s/ocpi.core/artifacts",
             registry.c_str());
  setenv("OCPI_LIBRARY_PATH", path.c_str(), true);
  ocpiInfo("Initializing OCPI_LIBRARY_PATH to \"%s\"", path.c_str());
  verbose = a_verbose;
  ezxml_t xml;
  if ((err = OE::ezxml_parse_file("gen/cases.xml", xml)))
    return err;
  const char *spec = ezxml_cattr(xml, "spec");
  assert(spec);
  if (verbose)
    fprintf(stderr, "Generating execution scripts for each platform that can run.\n");
  OS::FileSystem::mkdir("run", true);
  for (const char **p = platforms; *p; p++) {
    ocpiDebug("Trying platform %s", *p);
    std::string model;
    const char *cp = strchr(*p, '-');
    assert(cp);
    model.assign(*p, OCPI_SIZE_T_DIFF(cp, *p));
    CallBack cb(spec, model.c_str(), cp + 3, cp[1] == '1', xml);
    OL::Manager::findImplementations(cb, ezxml_cattr(xml, "spec"));
    if (cb.m_err.size())
      return OU::esprintf("While processing platform %s: %s", cp + 1, cb.m_err.c_str());
  }
  return NULL;
}
