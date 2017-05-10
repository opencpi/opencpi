// Replaces the original ocpixml script with C++ version (AV-2501)

#include <cstring> // strcmp
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <stdexcept>
#include "OcpiXmlEmbedded.h"
#include "OcpiOsFileSystem.h" // This is NOT fully OCPI::OS-enabled, e.g. path names, etc.

namespace OX = OCPI::Util::EzXml;
namespace OS = OCPI::OS;

#define OCPI_OPTIONS_HELP \
"This program performs functions relating to the XML inside artifact files.\n"\
"Usage is: ocpixml [<options>] <command> <file> [<file2>]\n"\
"Commands are: \n"\
"  get   - extract the XML from the file to standard output.\n"\
"  strip - remove the XML from the file while copying artifact to <file2>.\n"\
"  add   - add the XML file <file2> to the file, in place. file2 can be '-' for standard input.\n"

#define OCPI_OPTIONS \
    CMD_OPTION(log_level,  l, ULong,  0, "<log-level>\n" \
	                               "set log level during execution, overriding OCPI_LOG_LEVEL")

#include "CmdOption.h"


static int mymain(const char **argv) {
  // C++11: const std::map<std::string, int> valid_verbs = {{"get",3}, {"strip",4}, {"add",4}};
  std::map<std::string, unsigned> valid_verbs;
  valid_verbs["get"]=1;
  valid_verbs["strip"]=2;
  valid_verbs["add"]=2;

  if (not valid_verbs.count(argv[0])) {
    std::string e = "Invalid verb '";
    e.append(argv[0]);
    e.append("'");
    options.usage();
    throw std::invalid_argument(e);
  }
  if (options.argvCount()-1 != valid_verbs[argv[0]]) {
    std::string e = "Incorrect number of parameters for '";
    e.append(argv[0]);
    e.append("'");
    options.usage();
    throw std::invalid_argument(e);
  }

  // Start "real" work
  std::string xml;

  // add
  if (0 == strcmp("add", argv[0])) {
    std::stringstream buffer;
    if (0 == strcmp("-", argv[2])) { // Special case of stdin
      buffer << std::cin.rdbuf();
    } else {
      std::ifstream src(argv[2], std::ios::binary);
      buffer << src.rdbuf();
    }
    xml = buffer.str();
    OX::artifact_addXML(argv[1],buffer.str()); // Throws if invalid XML
  }

  // get
  if (0 == strcmp("get", argv[0])) {
    OX::artifact_getXML(argv[1], xml);
    if (xml.empty()) {
      std::cerr << "This file \"" << argv[1] << "\" is not an artifact file.\n";
      return 1;
    }
    std::cout << xml;
  }

  // strip
  if (0 == strcmp("strip", argv[0])) {
    OS::FileSystem::copy(argv[1], argv[2]);
    if (!OX::artifact_stripXML(argv[2])) {
      std::cerr << "This file \"" << argv[1] << "\" is not an artifact file.\n";
      OS::FileSystem::remove(argv[2]);
      return 1;
    }
  }
  return 0;
}
