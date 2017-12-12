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

#ifndef CDKUTILS_H
#define CDKUTILS_H
#include <cstdio>
#include <string>
#include <set>
#include <list>
#include "ezxml.h"

// Note that some callers may depend on this being ORDERED
typedef std::set<std::string> StringSet;
typedef StringSet::const_iterator StringSetIter;
// Ordered list with no duplicates, i.e. a set ordered by order of insertion
struct OrderedStringSet : public std::list<std::string> {
  void push_back(const std::string &);
  std::list<std::string>::const_iterator find(const std::string &);
};

enum Model {
  NoModel,
  HdlModel,
  RccModel,
  OclModel
};

extern void
  setDep(const char *file),
  addInclude(const char *inc),
  addInclude(const std::string &inc),
  addDep(const char *dep, bool child),
  printgen(FILE *f, const char *comment, const char *file, bool orig = false,
	   const char *endComment = "");  

extern const char
  *expandEnv(const char *in, std::string &out),
  *getCdkDir(std::string &cdk), // FIXME: put in runtime?
  *getPrereqDir(std::string &dir),
  *getPlatforms(const char *attr, OrderedStringSet &platforms, Model m = NoModel, bool onlyValidPlatforms = true),
  *getHdlPrimitive(const char *prim, const char *type, OrderedStringSet &prims),
  *getComponentLibrary(const char *lib, OrderedStringSet &libs),
  *getComponentLibrary(const char *lib, std::string &path),
  *getRccPlatforms(const StringSet *&platforms),
  *getHdlPlatforms(const StringSet *&platforms),
  *getOclPlatforms(const StringSet *&platforms),
  *getAllPlatforms(const StringSet *&platforms, Model m = NoModel),
  *getAllTargets(const StringSet *&targets, Model m = NoModel),
  *getPlatforms(const char *attr, StringSet &targets, Model m, bool onlyValidPlatformsPlatforms = true),
  *getTargets(const char *attr, OrderedStringSet &targets, Model m),
  *closeDep(),
  // Optional allows the element type might not match
  // NonExistentOK allows the file to not exist at all.
  *parseFile(const char *file, const std::string &parent, const char *element,
	     ezxml_t *xp, std::string &xfile, bool optional = false, bool search = true,
	     bool nonExistentOK = false),
  *openOutput(const char *name, const char *outDir, const char *prefix, const char *suffix,
	      const char *ext, const char *other, FILE *&f, std::string *path = NULL);
#endif
