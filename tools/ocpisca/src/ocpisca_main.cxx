#include "OcpiOsFileSystem.h"
#include "OcpiDriverManager.h"
#include "OcpiUtilEzxml.h"
#include "OcpiApplication.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OX = OCPI::Util::EzXml;

#define OCPI_OPTIONS_HELP \
  "Usage is: ocpisca <options>... [<opencpi xml file> | <sca xml file>]\n" \
  "  This command translates/generates between SCA and OpenCPI XML files.\n" \
  "  Which XML files are expected depends on which options are present.\n"

#define OCPI_OPTIONS \
  CMD_OPTION(verbose,    v, Bool,   0, "be verbose in describing what is happening")\
  CMD_OPTION(xml,        x, Bool,   0, "create SCA/REDHAWK XML files for an OpenCPI application\n" \
                                       "  (input file is application or deployment file)") \
  CMD_OPTION(deployment, d, String, 0, "separate XML file to read deployment from for the app") \
  CMD_OPTION(package,    P, Bool,   0, "copy application and deployment XML, and artifacts too") \
  CMD_OPTION(directory,  D, String, 0, "the directory where output files are placed") \

#include "CmdOption.h"

static int mymain(const char **ap) {

  OCPI::Driver::ManagerManager::suppressDiscovery();
  std::string file;  // the file that the application XML came from
  ezxml_t xml = NULL;
  if (*ap) {
    std::string depFile;
    file = *ap;
    if (!OS::FileSystem::exists(file)) {
      file += ".xml";
      if (!OS::FileSystem::exists(file))
	options.bad("file %s (or %s.xml) does not exist", *ap, *ap);
    }
    const char *err, *deployment;
    if ((err = OX::ezxml_parse_file(file.c_str(), xml)))
      options.bad("parsing XML file %s: %s", file.c_str(), err);
    if (!strcasecmp(ezxml_name(xml), "deployment")) {
      depFile = file;
      file.clear();
      OX::getOptionalString(xml, file, "application");
      ezxml_free(xml); // we only used it to grab the app attribute
      if (file.empty())
	options.bad("Input file, \"%s\" is a deployment file with no application attribute", *ap);
      if (!OS::FileSystem::exists(file)) {
	file += ".xml";
	if (!OS::FileSystem::exists(file))
	  options.bad("application file %s (or %s) does not exist", file.c_str(),
		      file.c_str());
      }
      if ((err = OX::ezxml_parse_file(file.c_str(), xml)))
	options.bad("error parsing XML file %s: %s", file.c_str(), err);
      
    } else if (strcasecmp(ezxml_name(xml), "application"))
      options.bad("file \"%s\" is a \"%s\" XML file.", file.c_str(), ezxml_name(xml));
    else if ((deployment = ezxml_cattr(xml, "deployment"))) {
      if (options.deployment())
	options.bad("Application XML has deployment attribute \"%s\", and a separate "
		    "deployment file \"%s\" was supplied.", deployment, options.deployment());
      depFile = deployment;
    } else if (!options.deployment())
      options.bad("Neither a deployment XML attribute, nor the deployment file was supplied");
    else
      depFile = options.deployment();
    std::vector<OA::PValue> params;
    params.push_back(OA::PVString("deployment", depFile.c_str()));
    params.push_back(OA::PVBool("execution", false));
    params.push_back(OA::PVEnd);
    std::string name;
    OU::baseName(file.c_str(), name);
    OA::ApplicationX app(xml, name.c_str(), &params[0]);
    app.genScaPrf(options.directory());
    app.genScaScd(options.directory());
    app.genScaSpd(options.directory());
  } else {
    options.bad("Missing filename for command");
  }
  return 0;
}

int
main(int /*argc*/, const char **argv) {
  return options.main(argv, mymain);
}
