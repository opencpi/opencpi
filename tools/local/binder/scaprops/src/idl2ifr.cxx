
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "idl2ifr.h"

// Python program to run as the back end of omniidl
// It outputs our "repository" format, which is, for each interface found in the idl:
// \n<Repository-Id>\n<InterfaceName>\n<openOCPI Protocol Spec>
// So we can just strstr for repoid, skip 2 newlines, and we have the protocol spec
// ending in a line ending with </Protocol>

static const char omniidl_be[] = {
"from omniidl import idlast, idltype, idlutil, idlvisitor, output\n"
"import sys, string\n"
"\n"
"def ocpiType(k) :\n"
"    if k == idltype.tk_void:         return 'void'\n"
"    elif k == idltype.tk_short:      return 'short'\n"
"    elif k == idltype.tk_long:       return 'long'\n"
"    elif k == idltype.tk_ushort:     return 'unsigned short'\n"
"    elif k == idltype.tk_ulong:      return 'unsigned long'\n"
"    elif k == idltype.tk_float:      return 'float'\n"
"    elif k == idltype.tk_double:     return 'double'\n"
"    elif k == idltype.tk_boolean:    return 'boolean'\n"
"    elif k == idltype.tk_char:       return 'char'\n"
"    elif k == idltype.tk_octet:      return 'octet'\n"
"    elif k == idltype.tk_any:        return 'any'\n"
"    elif k == idltype.tk_TypeCode:   return 'CORBA::TypeCode'\n"
"    elif k == idltype.tk_Principal:  return 'CORBA::Principal'\n"
"    elif k == idltype.tk_longlong:   return 'long long'\n"
"    elif k == idltype.tk_ulonglong:  return 'unsigned long long'\n"
"    elif k == idltype.tk_longdouble: return 'long double'\n"
"    elif k == idltype.tk_wchar:      return 'wchar'\n"
"    elif k == idltype.tk_enum:       return 'ULong'\n"
"    return 'fried'\n"
"\n"
"def doInterface(i, f):\n"
"    f.write('\\n{0}\\n{1}\\n    <Protocol>\\n'.format(i.repoId(), i.identifier()))\n"
"    for o in i.callables() :\n"
"        if isinstance(o,idlast.Operation):\n"
"            f.write('      <Operation Name=\"{0}\"'.format(o.identifier()))\n"
"            if len(o.parameters()) == 0 :\n"
"                f.write('/>')\n"
"            else :\n"
"                f.write('>\\n')\n"
"                for a in o.parameters() :\n"
"                    if isinstance(a,idlast.Parameter) :\n"
"                        f.write('        <Argument Name=\"{0}\" Type=\"{1}\"/>\\n'.\n"
"                                format(a.identifier(), ocpiType(a.paramType().unalias().kind())))\n"
"                f.write('      </Operation>')\n"
"            f.write('\\n')\n"
"    f.write('    </Protocol>\\n')\n"
"\n"
"def doModule(m, f):\n"
"    for i in m.definitions() :\n"
"        if isinstance(i,idlast.Interface):\n"
"            doInterface(i, f)\n"
"        elif isinstance(i,idlast.Module):\n"
"            doModule(i, f)\n"
"            \n"
"def run(tree, args):\n"
"    for m in tree.declarations():\n"
"        if isinstance(m, idlast.Module):\n"
"           doModule(m,sys.stdout)\n"
};

// Like strcat but surrounding with double-quotes, and escaping them too.
static void appendQuoteEscape(char *base, char *append) {
  char *cp = base;
  while (*cp)
    cp++;
  *cp++ = ' ';
  *cp++ = '"';
  while (*append) {
    if (*append == '"')
      *cp++ = '\\';
    *cp++ = *append++;
  }
  *cp++ = '"';
  *cp = 0;
}

// Run the command returning the stderr output as a string or
// setting the out argument to a string olding the stdout (repo).
static const char *
run(const char *command, char *&out)
{
  char
    *myCommand,
    outFile[] = "/tmp/ocpisoXXXXXXX",
    errFile[] = "/tmp/ocpiseXXXXXXX";
    int ofd, efd;
  if ((ofd = mkstemp(outFile)) < 0)
    return "internal error running idl parser 1";
  const char *err = 0;
  do {
    if ((efd = mkstemp(errFile)) < 0) {
      err = "internal error running idl parser 2";
      break;
    }
    asprintf(&myCommand, "%s 1> %s 2> %s", command, outFile, errFile);
    int systemResult = system(myCommand);
    switch (systemResult) {
    case -1:
      err = "internal error running idl parser 3";
      break;
    case 127:
      err = "internal error running idl parser 4";
      break;
    default:
      off_t eSize = lseek(efd, 0, SEEK_END);
      if (eSize == 0) {
	if (systemResult)
	  return "internal error running idl parser 5";
	off_t oSize;
	switch ((oSize = lseek(ofd, 0, SEEK_END))) {
	case 0:
	  err = "No interfaces found in IDL file(s)";
	  break;
	case -1:
	  err = "internal error running idl parser 6";
	  break;
	default:
	  if (lseek(ofd, 0, SEEK_SET) != 0 ||
	      !(out = (char *)malloc(oSize + 1)) ||
	      read(ofd, out, oSize) != oSize)
	    err = "internal error running idl parser 7";
	  else
	    out[oSize] = 0;
	}
      }	else {
	switch (eSize) {
	case 0:
	  err = "internal error running idl parser 8";
	  break;
	case -1:
	  err = "internal error running idl parser 9";
	  break;
	default:
	  char *myErr;
	  if (lseek(efd, 0, SEEK_SET) != 0 ||
	      !(myErr = (char *)malloc(eSize + 1)) ||
	      read(efd, (void *)myErr, eSize) != eSize)
	    err = "internal error running idl parser 10";
	  else {
	    while (eSize && myErr[eSize-1] == '\n')
	      eSize--;
	    myErr[eSize] = 0;
	    if (myErr && !strncmp("omniidl: ", myErr, 9))
	      strcpy(myErr, myErr + 9);
	    err = myErr;
	  }
	  
	}
      }
    }
    free(myCommand);
    close(efd);
    unlink(errFile);
  } while (0);
  close(ofd);
  unlink(outFile);
  return err;
}

// Process the IDL files with -D/U/I options, return our "repository" format
 const char *
idl2ifr(char **argv, char *&repo)
{
  // To avoid all manner of environment and installation dependencies,
  // We put the python backend in a temp file, point omniidl to it, and then remove it.
  char beTemp[] = "/tmp/ocpiobeXXXXXXX";
  int fd;
  if ((fd = mkstemp(beTemp)) < 0 ||
      write(fd, omniidl_be, sizeof(omniidl_be) - 1) != sizeof(omniidl_be) - 1 ||
      close(fd) != 0)
    return "internal error creating idl back end file";
  // Create the name we want the file to be
  char *dup;
  asprintf(&dup, "%s.py", beTemp);
  const char *err = 0;
  repo = 0;
  do { //break on error to cleanup
    if (rename(beTemp, dup) != 0) {
      err = "internal error naming idl back end file";
      break;
    }
    size_t size = 0, files = 0;
    for (char **ap = argv; *ap; ap++) {
      size += strlen(*ap) * 2 + 3; // all escaped, quoted, spaced
      if (ap[0][0] == '-') {
	if (strchr("DUI", ap[0][1])) {
	  if (ap[0][2] == '\0')
	    if (!*++ap) {
	      err = "expected one more argumnent after -D, -I, or -U";
	      break;
	    } else
	      size += strlen(*ap) * 2 + 3;
	} else {
	  err = "invalid flag argument - not D, U, or I";
	  break;
	}
      } else
	files++;
    }    
    if (err)
      break;
    if (!files) {
      err = "no IDL files to process";
      break;
    }
    char *omni;
    asprintf(&omni, "omniidl -p/tmp -b%s", strrchr(beTemp, '/') + 1);
    char *cp = (char *)malloc(strlen(omni) + size + 1);
    strcpy(cp, omni);
    free(omni);
    // Fill the command buffer with options and file names
    for (char **ap = argv; *ap; ap++) {
      appendQuoteEscape(cp, *ap);
      if (ap[0][0] == '-' && ap[0][2] == '\0')
	appendQuoteEscape(cp, *++ap);
    }
    err = run(cp, repo);
    free(cp);
  } while(0);
  unlink(dup); // remove the .py file we created and wrote
  char *dupc;
  asprintf(&dupc, "%sc", dup);
  free(dup);
  unlink(dupc); // remove the .pyc file that might have been created
  free(dupc);
  if (err) {
    free(repo);
    repo = 0;
  }
#if 0
 else
    write(1, repo, strlen(repo));
#endif
  return err;
}
