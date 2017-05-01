/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CDKUTILS_H
#define CDKUTILS_H
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include "ezxml.h"

typedef std::set<std::string> StringSet;
typedef StringSet::const_iterator StringSetIter;

extern void
  setDep(const char *file),
  addInclude(const char *inc),
  addDep(const char *dep, bool child),
  printgen(FILE *f, const char *comment, const char *file, bool orig = false,
	   const char *endComment = "");  

const StringSet &getAllPlatforms();

extern const char
  *getPlatforms(const char *attr, StringSet &platforms),
  *getRccPlatforms(StringSet &platforms),
  *getHdlPlatforms(StringSet &platforms),
  *closeDep(),
  // Optional allows the element type might not match
  // NonExistentOK allows the file to not exist at all.
  *parseFile(const char *file, const std::string &parent, const char *element,
	     ezxml_t *xp, std::string &xfile, bool optional = false, bool search = true,
	     bool nonExistentOK = false),
  *openOutput(const char *name, const char *outDir, const char *prefix, const char *suffix,
	      const char *ext, const char *other, FILE *&f, std::string *path = NULL);
#endif
