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

// Utility functions relating to available platforms.

#include <fnmatch.h>
#include "ocpi-config.h"
#include "OcpiOsFileIterator.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilCppMacros.h"
#include "OcpiUtilMisc.h"
#include "cdkutils.h"

namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OF = OCPI::OS::FileSystem;

namespace {

typedef std::vector<std::string> StringArray;
StringArray hdlPrimitivePath;
StringArray componentPath;
StringSet oclPlatforms; bool oclPlatformsDone;
StringSet rccPlatforms; bool rccPlatformsDone;
StringSet hdlPlatforms; bool hdlPlatformsDone;
StringSet allPlatforms; bool allPlatformsDone;
StringSet oclTargets;
StringSet rccTargets;
StringSet hdlTargets;
StringSet allTargets; bool allTargetsDone;

const char *
addPlaces(const char *envname, const char *suffix, bool check, StringArray &list) {
  const char *env = getenv(envname);
  ocpiInfo("Path %s is: %s", envname, env ? env : "<not set>");
  for (OU::TokenIter ti(env, ":"); ti.token(); ti.next()) {
    bool isDir;
    if (OF::exists(ti.token(), &isDir) && isDir)
      list.push_back(std::string(ti.token()) + suffix);
    else if (check)
      return OU::esprintf("in %s, \"%s\" is not a directory", env, ti.token());
  }
  return NULL;
}
const char *
addPlatform(const char *name, StringSet &platforms) {
  platforms.insert(name);
  allPlatforms.insert(name);
  return NULL;
}
const char *
addTarget(const char *name, StringSet &targets) {
  targets.insert(name);
  allTargets.insert(name);
  return NULL;
}
const char *
doHdlPlatform(std::string &place) {
  const char *name = strrchr(place.c_str(), '/');
  assert(name);
  std::string p(++name), dir;
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
  return addPlatform(name, hdlPlatforms);
}
}

const char *
getCdkDir(std::string &cdk) {
  // This will actually throw an exception
  cdk = OU::getCdk();
  return NULL;
#if 0
  const char *env = getenv("OCPI_CDK_DIR");
  bool isDir;
  if (env && OF::exists(env, &isDir) && isDir) {
    cdk = env;
    ocpiInfo("OCPI_CDK_DIR: %s", env);
    return NULL;
  }
  return "OCPI_CDK_DIR not set, does not exist, or is not a directory";
#endif
}

const char *
getPrereqDir(std::string &dir) {
  const char *env = getenv("OCPI_PREREQUISITES_DIR");
  if (env)
    dir = env;
  else {
    std::string cdk;
    const char *err = getCdkDir(cdk);
    if (err)
      return err;
    dir = cdk + "/../prerequisites";
    if (!OF::exists(dir))
      dir = "/opt/opencpi/prerequisites";
  }
  bool isDir;
  if (OF::exists(dir, &isDir) && isDir) {
    ocpiDebug("OCPI_PREREQUISITES_DIR: %s", env);
    return NULL;
  }
  return
    OU::esprintf("OpenCPI prerequisites directory \"%s\" does not exist or is not a directory",
		 dir.c_str());
}

const char *
getRccPlatforms(const StringSet *&platforms) {
  platforms = &rccPlatforms;
  if (rccPlatformsDone)
    return NULL;
  const char *err;
  std::string dir;
  if ((err = getCdkDir(dir)))
    return err;
  // THIS IS BROKEN FIND OUT WHO NEEDS THIS AND FIX IT TO LOOK IN PROJECTS for rcc/platforms
  dir += "/platforms";
  for (OS::FileIterator it(dir, "*"); !it.end(); it.next())
    if (it.isDirectory()) {
      std::string abs, rel, target;
      OU::format(target, "%s/%s-target.mk", it.absoluteName(abs), it.relativeName(rel));
      if (OF::exists(target))
	addPlatform(rel.c_str(), rccPlatforms);
    }
  rccPlatformsDone = true;
  return NULL;
}

const char *
getComponentLibrary(const char *lib, OrderedStringSet &libs) {
  std::string path;
  const char *err;
  if ((err = getComponentLibrary(lib, path)))
    return err;
  libs.push_back(path);
  return NULL;
}
const char *
getComponentLibrary(const char *lib, std::string &path) {
  const char *err;
  bool isDir;
  if (strchr(lib, '/')) {
    if (!OS::FileSystem::exists(lib, &isDir) || !isDir)
      return OU::esprintf("The component library location: \"%s\" is not a directory", lib);
    path = lib;
    path += "/lib";
    if (!OS::FileSystem::exists(path, &isDir) || !isDir)
      path = lib;
    return NULL;
  }
  if (componentPath.empty()) {
    if ((err = getCdkDir(path)) ||
	(err = addPlaces("OCPI_COMPONENT_LIBRARY_PATH", "/lib", true, componentPath)) ||
	(err = addPlaces("OCPI_PROJECT_PATH", "/lib", true, componentPath)))
      return err;
    componentPath.push_back(path + "/lib");
  }
  for (unsigned n = 0; n < componentPath.size(); n++) {
    OU::format(path, "%s/%s", componentPath[n].c_str(), lib);
    if (OS::FileSystem::exists(path, &isDir) && isDir)
      return NULL;
  }
  path.clear();
  for (unsigned n = 0; n < componentPath.size(); n++)
    OU::formatAdd(path, "%s\"%s\"", n ? "" : ", ", componentPath[n].c_str());
  return OU::esprintf("Could not find component library \"%s\"; looked in:  %s",
		      lib, path.c_str());
}

const char *
getHdlPrimitive(const char *prim, const char *type, OrderedStringSet &prims) {
  const char *err;
  if (strchr(prim, '/')) {
    if (!OS::FileSystem::exists(prim))
      return OU::esprintf("The %s location: \"%s\" does not exist", type, prim);
    prims.push_back(prim);
    return NULL;
  }
  std::string cdk;
  if (hdlPrimitivePath.empty()) {
    if ((err = getCdkDir(cdk)) ||
	(err = addPlaces("OCPI_HDL_PRIMITIVE_PATH", "", true, hdlPrimitivePath)) ||
	(err = addPlaces("OCPI_PROJECT_PATH", "/lib/hdl", false, hdlPrimitivePath)))
      return err;
    hdlPrimitivePath.push_back(cdk + "/lib/hdl");
  }
  std::string path;
  for (unsigned n = 0; n < hdlPrimitivePath.size(); n++) {
    OU::format(path, "%s/%s", hdlPrimitivePath[n].c_str(), prim);
    if (OS::FileSystem::exists(path)) {
      prims.push_back(path);
      return NULL;
    }
  }
  path.clear();
  for (unsigned n = 0; n < hdlPrimitivePath.size(); n++)
    OU::formatAdd(path, "%s\"%s\"", n ? "" : ", ", hdlPrimitivePath[n].c_str());
  return OU::esprintf("Could not find primitive %s \"%s\"; looked in:  %s", 
		      type, prim, path.c_str());
}

const char *
getHdlPlatforms(const StringSet *&platforms) {
  const char *err;
  platforms = &hdlPlatforms;
  if (hdlPlatformsDone)
    return NULL;
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
	  addPlatform(p.c_str(), hdlPlatforms);
	}
      else
	for (OS::FileIterator it(places[n], "*"); !it.end(); it.next()) {
	  std::string p(it.relativeName()), s, abs;
	  if (OF::exists(OU::format(s, "%s/%s.xml", it.absoluteName(abs), p.c_str())) ||
	      OF::exists(OU::format(s, "%s/lib/hdl/%s.xml", abs.c_str(), p.c_str())) ||
	      OF::exists(OU::format(s, "%s/hdl/%s.xml", abs.c_str(), p.c_str())))
	    if ((err = doHdlPlatform(abs)))
		return err;
	}
    } else if ((err = doHdlPlatform(places[n])))
      return err;
  }
  hdlPlatformsDone = true;
  return NULL;
}

const char *
getOclPlatforms(const StringSet *&platforms) {
  const char *err;
  platforms = &oclPlatforms;
  if (oclPlatformsDone)
    return NULL;
  std::string ocpiocl;
  if ((err = getCdkDir(ocpiocl)))
    return err;
#if 0
  OU::formatAdd(ocpiocl, "/bin/%s-%s-%s/ocpiocl",
		OCPI_CPP_STRINGIFY(OCPI_OS) + strlen("OCPI"),
		OCPI_CPP_STRINGIFY(OCPI_OS_VERSION), OCPI_CPP_STRINGIFY(OCPI_ARCH));
#else
  OU::formatAdd(ocpiocl, "/%s%s%s%s/bin/ocpiocl",
		OCPI_CPP_STRINGIFY(OCPI_PLATFORM),
		!OCPI_DEBUG || OCPI_DYNAMIC ? "-" : "",
		OCPI_DYNAMIC ? "d" : "",
		OCPI_DEBUG ? "" : "o");
#endif
  std::string cmd;
  OU::format(cmd, "%stest test && %s targets", ocpiocl.c_str(), ocpiocl.c_str());
  FILE *out;
  if ((out = popen(cmd.c_str(), "r")) == NULL)
    return OU::esprintf("Could not execute the \"ocpiocl targets\" command");
  std::string targets;
  for (int c; (c = fgetc(out)) != EOF; targets += (char)c)
    ;
  for (OU::TokenIter ti(targets.c_str()); ti.token(); ti.next()) {
    const char *eq = strchr(ti.token(), '=');
    if (!eq)
      return OU::esprintf("Invalid output from the \"ocpiocl targets\" command:  \"%s\"",
			  targets.c_str());
    std::string platform(ti.token(), eq - ti.token());
    oclPlatforms.insert(platform);
  }
  oclPlatformsDone = true;
  return NULL;
}

// Get it two ways.  If OCPI_ALL_PLATFORMS is provided we use it.  Otherwise we look around.
const char *
getAllPlatforms(const StringSet *&platforms, Model m) {
  if (!allPlatformsDone) {
    const char *env = getenv("OCPI_ALL_PLATFORMS");
    if (env)
      for (const char *ep; *env; env = ep) {
	while (isspace(*env)) env++;
	for (ep = env; *ep && !isspace(*ep); ep++)
	  ;
	if ((ep - env) < 5)
	  return OU::esprintf("the environment variable OCPI_ALL_PLATFORMS (\"%s\") is invalid",
			      env);
	std::string p;
	p.assign(env, (ep - 4) - env);
	if (!strncmp(ep - 4, ".rcc", 4))
	  addPlatform(p.c_str(), rccPlatforms);
	else if (!strncmp(ep - 4, ".hdl", 4))
	  addPlatform(p.c_str(), hdlPlatforms);
	else if (!strncmp(ep - 4, ".ocl", 4))
	  addPlatform(p.c_str(), oclPlatforms);
	else
	  return OU::esprintf("the environment variable OCPI_ALL_PLATFORMS (\"%s\") is invalid",
			      env);
      }
    else {
      const char *err;
      const StringSet *dummy;
      if ((err = getRccPlatforms(dummy)) ||
	  (err = getHdlPlatforms(dummy)) ||
	  (err = getOclPlatforms(dummy)))
	return err;
    }
    allPlatformsDone = true;
  }
  switch (m) {
  case NoModel: platforms = &allPlatforms; break;
  case RccModel: platforms = &rccPlatforms; break;
  case OclModel: platforms = &oclPlatforms; break;
  case HdlModel: platforms = &hdlPlatforms; break;
  default:
    return "unsupported model for platforms";
  }
  return NULL;
}

// Parse the attribute value, validating it, and expanding it if wildcard
// Do not bother to determine whether each platform is valid unless
// 'onlyValidPlatforms' is set to true
const char *
getPlatforms(const char *attr, OrderedStringSet &platforms, Model m, bool onlyValidPlatforms) {
  if (!attr)
    return NULL;
  const char *err;
  const StringSet *universe;
  if ((err = getAllPlatforms(universe, m)))
    return err;
  for (OU::TokenIter ti(attr); ti.token(); ti.next()) {
    bool found;
    if (onlyValidPlatforms) {
      for (StringSetIter si = universe->begin(); si != universe->end(); ++si)
        if (fnmatch(ti.token(), (*si).c_str(), FNM_CASEFOLD) == 0) {
          found = true;
          platforms.push_back(*si);
	}
      if (!found)
	return OU::esprintf("the string \"%s\" does not indicate or match any platforms",
			    ti.token());
    } else
      platforms.push_back(ti.token());
  }
  return NULL;
}

// Get it two ways.  If OCPI_ALL_TARGETS is provided we use it.  Otherwise we look around.
const char *
getAllTargets(const StringSet *&targets, Model m) {
  if (!allTargetsDone) {
    const char *env;
    if ((env = getenv("OCPI_ALL_HDL_TARGETS")))
      for (OU::TokenIter ti(env); ti.token(); ti.next())
	addTarget(ti.token(), hdlTargets);
    else
      ocpiInfo("the environment variable OCPI_ALL_HDL_TARGETS is not set");
    if ((env = getenv("OCPI_ALL_RCC_TARGETS")))
      for (OU::TokenIter ti(env); ti.token(); ti.next())
	addTarget(ti.token(), rccTargets);
    else
      ocpiInfo("the environment variable OCPI_ALL_RCC_TARGETS is not set");
    allTargetsDone = true;
  }
  switch (m) {
  case NoModel: targets = &allTargets; break;
  case RccModel: targets = &rccTargets; break;
  case HdlModel: targets = &hdlTargets; break;
  default:
    return "unsupported model for targets";
  }
  return NULL;
}

// Parse the attribute value, validating it, and expanding it if wildcard
const char *
getTargets(const char *attr, OrderedStringSet &targets, Model m) {
  if (!attr)
    return NULL;
  const char *err;
  const StringSet *universe;
  if ((err = getAllTargets(universe, m)))
    return err;
  for (OU::TokenIter ti(attr); ti.token(); ti.next()) {
    bool found;
    for (StringSetIter si = universe->begin(); si != universe->end(); ++si)
      if (fnmatch(ti.token(), (*si).c_str(), FNM_CASEFOLD) == 0) {
	found = true;
	targets.push_back(*si);
      }
    if (!found)
      return OU::esprintf("the string \"%s\" does not indicate or match any targets", ti.token());
  }
  return NULL;
}
std::list<std::string>::const_iterator OrderedStringSet::
find(const std::string &s) {
  for (auto si = begin(); si != end(); ++si)
    if (s == *si)
      return si;
  return end();
}
void OrderedStringSet::push_back(const std::string &s) {
  if (find(s) == end())
    std::list<std::string>::push_back(s);
}
