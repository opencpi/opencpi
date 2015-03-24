#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include "OcpiOsLoadableModule.h"
#include "OclContainer.h"

namespace OU =  OCPI::Util;

#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiocl [options] <command>\n" \
  "  Commands are:\n" \
  "    compile   - act like a compiler, building worker binaries\n" \
  "    search    - find available OpenCL devices\n" \
  "    probe     - test whether an OpenCL device exists\n" \
  "    compile   - act like a compiler, converting source (.cl) to object (.clo)\n" \
  "    test      - test for the existence of an OpenCL environment\n"

//           name      abbrev  type    value description
#define OCPI_OPTIONS \
  CMD_OPTION_S(define,     D,    String, NULL,    "Set preprocessor definition") \
  CMD_OPTION_S(include,    I,    String, NULL,    "Set include directory") \
  CMD_OPTION(  output,     o,    String, NULL,    "Set name of output file") \
  CMD_OPTION(  device,     d,    String, NULL,    "Specific OCL device to use") \
  CMD_OPTION(  target,     t,    String, NULL,    "Target device type to compile for") \
  CMD_OPTION(  verbose,    v,    Bool,   "false", "Provide verbose output during operation") \
  CMD_OPTION(  loglevel,   l,    UChar,  "0",     "Set logging level used during operation") \

#include "CmdOption.h"

namespace OC = OCPI::Container;
namespace OA = OCPI::API;

static int mymain(const char **ap) {
  OCPI::OS::logSetLevel(options.loglevel());
  OCPI::Driver::ManagerManager::suppressDiscovery();
  const char *env = getenv("OCPI_OPENCL_OBJS");
  OCPI::OS::LoadableModule lm(env ? env : "libOpenCL.so", true);

  if (!strcasecmp(*ap, "test"))
    return 0;
  if (!strcasecmp(*ap, "search")) {
    OA::PVarray vals(5);
    unsigned n = 0;
    vals[n++] = OA::PVBool("printOnly", true);
    if (options.verbose())
      vals[n++] = OA::PVBool("verbose", true);
    vals[n++] = OA::PVEnd;
    OCPI::OCL::Driver::getSingleton().search(vals, NULL, true);
  } else if(!strcasecmp(*ap, "probe")) {
    if (!*++ap)
      options.bad("Missing device name argument to probe");
    std::string err;
    if (!OCPI::OCL::Driver::getSingleton().open(*ap, options.verbose(), true, err))
      options.bad("Error during probe for \"%s\": %s", *ap, err.c_str());
  } else if(!strcasecmp(*ap, "compile")) {
    if (!options.target())
      options.bad("The target (-t) option must be specified for the compile command");
    size_t numIncludes, numDefines, numSources;
    const char
      **includes = options.include(numIncludes),
      **defines = options.define(numDefines),
      **sources = ++ap;
    for (numSources = 0; *ap++; numSources++)
      ;
    if (!numSources)
      options.bad("No source files supplied after \"compile\" command.");
    std::vector<int> fds(numSources, -1);
    std::vector<off_t> sizes(numSources*2, 0);
    std::vector<void *> mapped(numSources*2, NULL);
    for (unsigned n = 0; n < numSources; n++) {
      if ((fds[n] = open(sources[n], O_RDONLY)) < 0 ||
	  (sizes[n*2+1] = lseek(fds[n], 0, SEEK_END)) == -1 ||
	  (mapped[n*2+1] = mmap(NULL, sizes[n*2+1], PROT_READ, MAP_SHARED, fds[n], 0)) == MAP_FAILED)
	options.bad("Can't open source file: %s", sources[n]);
      std::string line;
      OU::format(line, "# 1 \"%s\"\n", sources[n]);
      mapped[n*2] = new char[line.size()];
      memcpy(mapped[n*2], line.c_str(), line.size());
      sizes[n*2] = line.size();
    }
    OCPI::OCL::compile(numSources*2, &mapped[0], &sizes[0], includes, defines,
		       options.output(), options.target(), options.verbose());
  }
  return 0;
}

int
main(int /*argc*/, const char **argv) {
  return options.main(argv, mymain);
}
