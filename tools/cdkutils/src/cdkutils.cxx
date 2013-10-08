
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

#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include "OcpiUtilMisc.h"
#include "cdkutils.h"

namespace OU = OCPI::Util;
const char **includes;
static unsigned nIncludes;
void
addInclude(const char *inc) {
  includes = (const char **)realloc(includes, (nIncludes + 2) * sizeof(char *));
  includes[nIncludes++] = inc;
  includes[nIncludes] = 0;
}

const char *
parseFile(const char *file, const char *parent, const char *element,
          ezxml_t *xp, const char **xfile, bool optional) {
  const char *err = NULL;
  char *myFile;
  const char *slash = strrchr(file, '/');
  const char *dot = strrchr(file, '.');
  if (!dot || slash && slash > dot)
    asprintf(&myFile, "%s.xml", file);
  else
    myFile = strdup(file);
  const char *cp = parent ? strrchr(parent, '/') : 0;
  if (myFile[0] != '/' && cp)
    asprintf((char**)&cp, "%.*s%s", (int)(cp - parent + 1), parent, myFile);
  else
    cp = strdup(myFile);
  std::list<const char *> tries;
  tries.push_back(cp);
  int fd = open(cp, O_RDONLY);
  do { // break on error
    if (fd < 0) {
      // file was not where parent file was, and not local.
      // Try the include paths
      if (myFile[0] != '/' && includes) {
	for (const char **ap = includes; *ap; ap++) {
	  if (!(*ap)[0] || !strcmp(*ap, "."))
	    cp = strdup(myFile);
	  else
	    asprintf((char **)&cp, "%s/%s", *ap, myFile);
	  tries.push_back(cp);
	  if ((fd = open(cp, O_RDONLY)) >= 0)
	    break;
	}
      }
      if (fd < 0) {
	std::string files;
	bool first = true;
	for (std::list<const char *>::const_iterator i = tries.begin(); i != tries.end(); i++) {
	  OU::formatAdd(files, "%s %s", first ? "" : ",", *i);
	  first = false;
	}
	err =
	  OU::esprintf("File \"%s\" could not be opened for reading/parsing.  Files tried: %s",
		       file, files.c_str());
	break;
      }
    }
    ezxml_t x = ezxml_parse_fd(fd);
    if (x && ezxml_error(x)[0]) {
      err = OU::esprintf("XML Parsing error: %s", ezxml_error(x));
      break;
    }
    if (!x || !x->name) {
      err = OU::esprintf("File \"%s\" (when looking for \"%s\") could not be parsed as XML",
			 cp, file);
      break;
    }
    if (element && strcasecmp(x->name, element)) {
      if (optional)
	*xp = 0;
      else {
	err = OU::esprintf("File \"%s\" does not contain a %s element", cp, element);
	break;
      }
    } else
      *xp = x;
    if (xfile)
      *xfile = strdup(cp);
    addDep(cp, parent != 0);
  } while (0);
  while (!tries.empty()) {
    cp = tries.front();
    tries.pop_front();
    free((void*)cp);
  }
  return err;
}

static const char **deps;
static bool *depChild;
static unsigned nDeps;
const char *depFile;

void
addDep(const char *dep, bool child) {
  for (unsigned n = 0; n < nDeps; n++)
    if (!strcmp(dep, deps[n])) {
      if (child)
        depChild[n] = child;
      return;
    }
  deps = (const char **)realloc(deps, (nDeps + 2) * sizeof(char *));
  depChild = (bool *)realloc(depChild, (nDeps + 2) * sizeof(bool));
  depChild[nDeps] = child;
  deps[nDeps++] = strdup(dep);
  depChild[nDeps] = 0;
  deps[nDeps] = 0;
}

static const char *
dumpDeps(const char *top) {
  if (!depFile)
    return 0;
  static FILE *out = NULL;
  if (out == NULL) {
    out = fopen(depFile, "w");
    if (out == NULL)
      return OU::esprintf("Cannot open dependency file \"%s\" for writing", top);
  }
  fprintf(out, "%s:", top);
  for (unsigned n = 0; n < nDeps; n++)
    fprintf(out, " %s", deps[n]);
  fprintf(out, "\n");
  for (unsigned n = 0; n < nDeps; n++)
    if (depChild[n])
      fprintf(out, "\n%s:\n", deps[n]);
  fflush(out);
  //  fclose(out);
  //  depFile = 0;
  return 0;
}

const char *
openOutput(const char *name, const char *outDir, const char *prefix, const char *suffix,
	   const char *ext, const char *other, FILE *&f) {
  char *file;
  asprintf(&file, "%s%s%s%s%s%s", outDir ? outDir : "", outDir ? "/" : "",
	  prefix, name, suffix, ext);
  if ((f = fopen(file, "w")) == NULL)
    return OU::esprintf("Can't not open file %s for writing (%s)\n",
			file, strerror(errno));
  dumpDeps(file);
  if (other && strcmp(other, name)) {
    char *otherFile;
    asprintf(&otherFile, "%s%s%s%s%s%s", outDir ? outDir : "", outDir ? "/" : "",
	    prefix, other, suffix, ext);
    // Put all this junk in OcpiOs
    char dummy;
    ssize_t length = readlink(otherFile, &dummy, 1);
    if (length != -1) {
      char *buf = (char*)malloc(length + 1);
      if (readlink(otherFile, buf, length) != length)
	return "Unexpected system error reading symlink";
      buf[length] = '\0';
      if (!strcmp(otherFile, buf))
	return 0;
      if (unlink(otherFile))
	return "Cannot remove symlink to replace it";
    } else if (errno != ENOENT)
      return "Unexpected error reading symlink";
    char *contents = strrchr(file, '/');
    contents = contents ? contents + 1 : file;
    if (symlink(contents, otherFile))
      return "Cannot create symlink";
  }
  return 0;
}

void
printgen(FILE *f, const char *comment, const char *file, bool orig, const char *endComment) {
  time_t now = time(0);
  char *ct = ctime(&now);
  ct[strlen(ct) - 1] = '\0';
  struct tm *local = localtime(&now);
  fprintf(f,
	  "%s THIS FILE WAS %sGENERATED ON %s %s%s\n"
	  "%s BASED ON THE FILE: %s%s\n"
	  "%s YOU %s EDIT IT%s\n",
	  comment, orig ? "ORIGINALLY " : "", ct, local->tm_zone, endComment,
	  comment, file, endComment,
	  comment, orig ? "*ARE* EXPECTED TO" : "PROBABLY SHOULD NOT", endComment);
}

