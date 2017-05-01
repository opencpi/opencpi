// Utility functions relating to available platforms.

#include "fnmatch.h"
#include "OcpiOsFileIterator.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilMisc.h"
#include "cdkutils.h"

namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OF = OCPI::OS::FileSystem;

const char *
getCdkDir(std::string &cdk) {
  const char *env = getenv("OCPI_CDK_DIR");
  bool isDir;
  if (env && OF::exists(env, &isDir) && isDir) {
    cdk = env;
    return NULL;
  }
  return "OCPI_CDK_DIR not set, does not exist, or is not a directory";
}

const char *
getRccPlatforms(StringSet &platforms) {
  const char *err;
  std::string dir;
  if ((err = getCdkDir(dir)))
    return err;
  dir += "/platforms";
  for (OS::FileIterator it(dir, "*"); !it.end(); it.next())
    if (it.isDirectory()) {
      std::string abs, rel, target;
      OU::format(target, "%s/%s-target.mk", it.absoluteName(abs), it.relativeName(rel));
      if (OF::exists(target))
	platforms.insert(rel);
    }
  return NULL;
}

namespace {

typedef std::vector<std::string> StringArray;
const char *
addPlaces(const char *env, const char *suffix, bool check, StringArray &list) {
  std::string s;
  for (const char *cp = getenv(env); cp && *cp; cp++) {
    const char *ep = strchr(cp, ':');
    if (!ep)
      s = cp;
    else
      s.assign(cp, ep - cp);
    bool isDir;
    if (OF::exists(s, &isDir) && isDir)
      list.push_back(s + suffix);
    else if (check)
      return OU::esprintf("in %s, \"%s\" is not a directory", env, s.c_str());
    cp = ep;
  }
  return NULL;
}
const char *
addPlatform(const char *name, StringSet &platforms) {
  platforms.insert(name);
  return NULL;
}
const char *
doPlatform(std::string &place, StringSet &platforms) {
  const char *name = strrchr(place.c_str(), '/');
  assert(name);
  std::string p(++name + 1), dir;
  OU::format(dir, "%s/lib/%s.mk", place.c_str(), name);
  if (OF::exists(dir))
    dir = place + "/lib";
  else
    dir = place;
  if (!OF::exists(dir + "/hdl/" + p + ".xml") && !OF::exists(dir + "/" + p + ".xml"))
    return OU::esprintf("no %s.xml file found under %s for HDL platform", name, place.c_str());
  if (!OF::exists(dir + "/" + p + ".mk"))
    return OU::esprintf("no %s.mk file found under %s for HDL platform; not built?",
			name, place.c_str());
  return addPlatform(name, platforms);
}

} // anonymous

const char *
getHdlPlatforms(StringSet &platforms) {
  const char *err;
  std::string cdk;
  StringArray places;
  if ((err = getCdkDir(cdk)) ||
      (err = addPlaces("OCPI_HDL_PLATFORM_PATH", "", true, places)) ||
      (err = addPlaces("OCPI_PROJECT_PATH", "/lib/platforms", false, places)))
    return err;
  places.push_back(cdk + "/lib/platforms"); // this must exist
  for (unsigned n = 0; n < places.size(); n++) {
    const char *slash = strrchr(places[n].c_str(), '/');
    assert(slash);
    if (!strcmp(++slash, "platforms")) {
      std::string dir(slash);
      dir = places[n] + "/mk";
      if (OF::exists(dir))
	for (OS::FileIterator it(dir, "*.mk"); !it.end(); it.next()) {
	  std::string p(it.relativeName());
	  p.resize(p.size() - 3);
	  addPlatform(p.c_str(), platforms);
	}
      else
	for (OS::FileIterator it(places[n], "*"); !it.end(); it.next()) {
	  std::string p(it.relativeName()), s, abs;
	  if (OF::exists(OU::format(s, "%s/%s.xml", it.absoluteName(abs), p.c_str())) ||
	      OF::exists(OU::format(s, "%s/lib/hdl/%s.xml", abs.c_str(), p.c_str())) ||
	      OF::exists(OU::format(s, "%s/hdl/%s.xml", abs.c_str(), p.c_str())))
	    if ((err = doPlatform(abs, platforms)))
		return err;
	}
    } else if ((err = doPlatform(places[n], platforms)))
      return err;
  }
  return NULL;
}

const StringSet &
getAllPlatforms() {
  static StringSet allPlatforms;
  if (allPlatforms.empty()) {
    const char *err;
    if ((err = getRccPlatforms(allPlatforms)) ||
	(err = getHdlPlatforms(allPlatforms)))
      ocpiBad("error getting all platforms: %s", err);
  }
  return allPlatforms;
}

// Parse the attribute value, validating it, and expanding it if wildcard
// A static method
const char *
getPlatforms(const char *attr, StringSet &platforms) {
  static StringSet allPlatforms;
  if (!attr)
    return NULL;
  if (allPlatforms.empty()) {
    const char *env = getenv("OCPI_ALL_PLATFORMS");
    if (!env)
      return "the environment variable OCPI_ALL_PLATFORMS is not set";
    for (const char *ep; *env; env = ep) {
      while (isspace(*env)) env++;
      for (ep = env; *ep && !isspace(*ep); ep++)
	;
      std::string p;
      p.assign(env, ep - env);
      allPlatforms.insert(p);
    }
    if (allPlatforms.empty())
      return "the environment variable OCPI_ALL_PLATFORMS has no platforms";
  }
  while (*attr) {
    while (*attr && (isspace(*attr) || *attr == ','))
      attr++;
    if (!*attr)
      break;
    const char *cp;
    for (cp = attr; *cp && !isspace(*cp) && *cp != ','; cp++)
      ;
    std::string plat(attr, cp - attr);
    bool found;
    for (StringSetIter si = allPlatforms.begin(); si != allPlatforms.end(); ++si)
      if (fnmatch(plat.c_str(), (*si).c_str(), FNM_CASEFOLD) == 0) {
	found = true;
	platforms.insert(*si);
      }
    if (!found)
      return OU::esprintf("the string \"%s\" does not indicate or match any platforms",
			  plat.c_str());
    attr = cp;
  }    
  return NULL;
}
