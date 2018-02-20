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

#include <cerrno>
#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <list>
#include <memory>
#include <fstream>

#include "OcpiUtilMisc.h"
#include "cdkutils.h"

namespace OU = OCPI::Util;
static std::vector<std::string> includes; // our included files
typedef std::vector<std::string>::const_iterator IncludesIter;
// For anybody who used the old "includes" (only ocpidds that I am aware of)
// provide a close equivalent...
std::vector<const char *> compat_includes() {
  std::vector<const char *> vec(includes.size());
  for (IncludesIter iter = includes.begin(); iter != includes.end(); ++iter)
    vec.push_back(iter->c_str());
  return vec;
}
void
addInclude(const std::string &inc) {
  includes.push_back(inc);
}
void
addInclude(const char *inc) {
  includes.push_back(inc);
}

// The "optional" argument says the file may not exist at all, or it
// may have the wrong top level element.  If the filename is "-",
// stdin is assumed.
const char *
parseFile(const char *file, const std::string &parent, const char *element,
          ezxml_t *xp, std::string &xfile, bool optional, bool search, bool nonExistentOK) {
  const char *err = NULL;
  char *myFile;
  const char *slash = strrchr(file, '/');
  const char *dot = strrchr(file, '.');
  if (!dot || (slash && slash > dot))
    ocpiCheck(asprintf(&myFile, "%s.xml", file) > 0);
  else
    myFile = strdup(file);
  const char *cp = strrchr(parent.c_str(), '/');
  if (myFile[0] != '/' && cp)
    ocpiCheck(asprintf((char**)&cp, "%.*s%s", (int)(cp - parent.c_str() + 1), parent.c_str(),
		       myFile) > 0);
  else
    cp = strdup(myFile);
  std::list<const char *> tries;
  bool relative = false;
  if (*cp != '/')
    relative = true;
  tries.push_back(cp);
  int fd = open(cp, O_RDONLY);
  ocpiDebug("Trying to open '%s', and %s", cp, fd < 0 ? "failed" : "succeeded");
  do { // break on error
    if (fd < 0) {
      // file was not where parent file was, and not local.
      // Try the include paths
      if (myFile[0] != '/' && !includes.empty() && search) {
        for (IncludesIter iter = includes.begin(); iter != includes.end(); ++iter) {
          const char *ptr = iter->c_str();
	  if (!ptr[0] || !strcmp(ptr, "."))
	    cp = strdup(myFile);
	  else
	    ocpiCheck(asprintf((char **)&cp, "%s/%s", /**ap*/ptr, myFile) > 0);
	  if (*cp != '/')
	    relative = true;
	  tries.push_back(cp);
	  fd = open(cp, O_RDONLY);
	  ocpiDebug("Trying to open '%s', and %s", cp, fd < 0 ? "failed" : "succeeded");
          if (fd >= 0)
	    break;
	}
      }

      if (fd < 0) {
	std::string files;
	bool first = true;
	for (std::list<const char *>::const_iterator i = tries.begin(); i != tries.end(); ++i) {
	  OU::formatAdd(files, "%s%s", first ? "" : ", ", *i);
	  first = false;
	}
	if (nonExistentOK) {
	  *xp = 0;
	  ocpiInfo("Optional file \"%s\" could not be opened for reading/parsing."
		   "  Files tried: %s", file, files.c_str());
	} else {
	  if (relative) {
	    char *cwd = getcwd(NULL, 0); // NOTE: Linux extension, NOT POSIX. OSX supports as well.
	    if (cwd) {
	      OU::formatAdd(files, ". CWD is %s", cwd);
	      free(cwd);
            }
	  }
	  err =
	    OU::esprintf("File \"%s\" could not be opened for reading/parsing."
			 "  Files tried: %s", file, files.c_str());
	}
	break;
      }
    }
    ezxml_t x = ezxml_parse_fd(fd);
    if (x && ezxml_error(x)[0]) {
      err = OU::esprintf("XML Parsing error in file \"%s\": %s", cp, ezxml_error(x));
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
    xfile = cp;
    addDep(cp, !parent.empty());
  } while (0);
  close(fd);
  free(myFile);
  while (!tries.empty()) {
    cp = tries.front();
    tries.pop_front();
    free((void*)cp);
  }
  return err;
}
                           // pair = dep,child
static std::vector<std::pair<std::string,bool> > depList;
typedef std::vector<std::pair<std::string,bool> >::iterator DepListIter;
static std::ofstream depOut; // our dependency output file stream
static const char *depFile;

void
setDep(const char *dep) {
  assert(dep);
  depFile = dep;
}

void
addDep(const char *dep, bool child) {
  // Update "child" flag if we already have
  for (DepListIter it = depList.begin(); it != depList.end(); ++it)
    if (!strcmp(dep, it->first.c_str())) {
      if (child)
	it->second = child;
      return;
    }
  depList.push_back(std::make_pair(dep, child));
}
// Called to close the file and determine any errors
const char *
closeDep() {
  if (depOut.is_open()) {
    depOut.close();
    if (!depOut.good())
      return OU::esprintf("Closing and flushing dependency file \"%s\" failed", depFile);
  }
  return NULL;
}

static const char *
dumpDeps(const char *top) {
  if (!depFile)
    return NULL;
  if (!depOut.is_open()) {
    depOut.open(depFile);
    if (!depOut.good())
      return OU::esprintf("Cannot open dependency file \"%s\" for writing", depFile);
  }
  depOut << top << ":";
  for (DepListIter it = depList.begin(); it != depList.end(); ++it)
    if (strcmp(top, it->first.c_str()))
      depOut << " " << it->first;
  depOut << std::endl;
  for (DepListIter it = depList.begin(); it != depList.end(); ++it)
     if (it->second && strcmp(top, it->first.c_str()))
       depOut << "\n" << it->first << ":\n";
  if (!depOut.flush().good())
    return OU::esprintf("Error writing to dependency file \"%s\".", depFile);
  return NULL;
}

// TODO: Make an iostream version of this call to move away from C-style FILE handles
const char *
openOutput(const char *name, const char *outDir, const char *prefix, const char *suffix,
	   const char *ext, const char *other, FILE *&f, std::string *path) {
  std::string file;
  OU::format(file, "%s%s%s%s%s%s", outDir ? outDir : "", outDir ? "/" : "", prefix, name, suffix,
	     ext);
  if (path)
    *path = file;
  ocpiDebug("openOutput attempting to write to %s", file.c_str());
  const char *err = NULL;
  f = NULL;
  do { // break on error
    if ((f = fopen(file.c_str(), "w")) == NULL) {
      err = OU::esprintf("Can't not open file %s for writing (%s)\n", file.c_str(),
			 strerror(errno));
      break;
    }
    if ((err = dumpDeps(file.c_str())))
      break;
    if (other && strcmp(other, name)) {
      std::string otherFile;
      OU::format(otherFile, "%s%s%s%s%s%s", outDir ? outDir : "", outDir ? "/" : "", prefix,
		 other,
		 suffix, ext);
      // FIXME: Put all this into OcpiOs
      // FIXME: "man 2 readlink" implies this will set length to "1" no matter the real length of the answer (AV-959)
      char dummy;
      ssize_t length = readlink(otherFile.c_str(), &dummy, 1);
      if (length != -1) {
	char *buf = (char*)malloc(length + 1);
	if (readlink(otherFile.c_str(), buf, length) != length) {
	  free(buf);
	  err = "Unexpected system error reading symlink";
	  break;
	}
	buf[length] = '\0';
	bool same = strcmp(otherFile.c_str(), buf) == 0;
	free(buf);
	if (same)
	  return 0;
	if (unlink(otherFile.c_str())) {
	  err = "Cannot remove symlink to replace it";
	  break;
	}
      } else if (errno != ENOENT) {
	err = "Unexpected error reading symlink";
	break;
      }
      const char *contents = strrchr(file.c_str(), '/');
      if (symlink(contents ? contents + 1 : file.c_str(), otherFile.c_str()))
	err = "Cannot create symlink";
    }
  } while (0);
  if (err && f)
    fclose(f);
  return err;
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

const char *
expandEnv(const char *s, std::string &out) {
  for (const char *in = s; *in;) {
    if (*in == '$') {
      bool paren = in[1] == '(';
      if (paren)
	in++;
      if (!isalpha(*++in))
	return OU::esprintf("invalid environment reference in: %s", in);
      const char *name = in;
      while (*++in && (isalnum(*in) || *in == '_'))
	;
      std::string ename(name, in - name);
      if (paren && *in++ != ')')
	return OU::esprintf("invalid environment reference in: %s", in);
      const char *env = getenv(ename.c_str());
      if (!env)
	return OU::esprintf("in environment reference to \"%s\" in \"%s\": variable is not set",
			    ename.c_str(), s);
      out += env;
    } else
      out += *in++;
  }
  return NULL;
}
