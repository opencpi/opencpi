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

/*
  todo: 
  maybe rename ocpiidl to combine with SCA aspects.
  allow multiple structs for multiple topics on a port?
  dds integ: only need type when no qualified name (didn't come from idl)
  or "dds-name"?
  options for output filename, output protocol name, etc. - default topic name?
  unions
  other pragma tag values and comments for compatibility with opendds and rti...
  (DDS_KEY, DCPS_DATA_KEY, DCPS_DATA_TYPE, //@key)
  multiple output files?
  do subtype names matter for dds layer?
  keys in substructures?
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <vector>
#include <list>

#include "ocpi-config.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilDataTypes.h"
#include "OcpiUtilProtocol.h"
#include "OcpiUtilMisc.h"
#include "cdkutils.h"
#include "ocpidds.h"

namespace OS = OCPI::OS;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
const char *
emitIDL(const char *outDir, const char *protoFile) {
  (void)outDir;
  OU::Protocol p;
  const char *err = 0;
  ezxml_t x;
  std::string dummy;
  if ((err = parseFile(protoFile, NULL, "protocol", &x, dummy, false)) ||
      (err = p.parse(x, NULL, NULL, NULL, NULL)))
    return err;
  std::string out;
  p.printXML(out, 0);
  fputs(out.c_str(), stdout);
  return NULL;
}

// Python program to run as the back end of omniidl
// It outputs our "repository" format, which is, for each interface found in the idl:
// \n<Repository-Id>\n<InterfaceName>\n<openOCPI Protocol Spec>
// So we can just strstr for repoid, skip 2 newlines, and we have the protocol spec
// ending in a line ending with </Protocol>

static const char omniidl_be[] = {
"from omniidl import idlast, idltype, idlutil, idlvisitor, output\n"
"import os.path, sys, string, pprint\n"
"\n"
"def enumType(t) :\n"
"    return ('enum {0} '.format(len(t.decl().enumerators())) + ' '.join(map(idlast.DeclRepoId.identifier, t.decl().enumerators())) + ' .')\n"
"    \n"
"def member(t) :\n"
"    return t.identifier() + ocpiType(t.memberType())\n"
"def array(x) :\n"
"    return str(x)\n"
//"    return '['+str(x)+']'\n"
"def ocpiSizes(s) :\n"
"    #sys.stderr.write(repr(s.sizes()))\n"
"    if len(s.sizes()) != 0:\n"
"       return 'array ' + ' '.join(map(array,s.sizes())) + ' '\n"
"    else:\n"
"       return ''\n"
"def structType(t) :\n"
"    #sys.stderr.write(repr(t))\n"
"    #sys.stderr.write(repr(t))\n"
"    ret = 'struct {0} '.format(len(t.members()))\n"
"    for m in t.members() :\n"
"        for d in m.declarators() :\n"
"            ret = (ret + d.identifier() + ' ' + \n"
"                   ocpiSizes(d) + \n"
"                   ocpiType(m.memberType()) + ';')\n"
"    return ret + '.'\n"
"    \n"
"def sequenceType(t) :\n"
"    #sys.stdout.write(repr(t))\n"
"    #sys.stdout.write(str(t.seqType()))\n"
"    #sys.stdout.write(pprint.pformat(t.seqType().kind()))\n"
"    #sys.stdout.write(ocpiType(t.seqType()))\n"
"    #sys.stdout.write(repr(inspect.getmembers(tt)))\n"
"    return 'sequence {0} {1}'.format(t.bound(), ocpiType(t.seqType()))\n"
"    \n"
"def arrayType(t) :\n"
"    #sys.stdout.write(repr(t))\n"
"    #sys.stdout.write(str(t.seqType()))\n"
"    #sys.stdout.write(pprint.pformat(t.seqType().kind()))\n"
"    #sys.stdout.write(ocpiType(t.seqType()))\n"
"    #tt = t.seqType()\n"
"    #sys.stdout.write(repr(inspect.getmembers(tt)))\n"
"    return 'array {0} {1}'.format(t.bound(), ocpiType(t.seqType()))\n"
"    \n"
"def ocpiTypeDesc(t) :\n"
"    k = t.kind()\n"
"    if k == idltype.tk_void:         return 'void'\n"
"    elif k == idltype.tk_short:      return 'short'\n"
"    elif k == idltype.tk_long:       return 'long'\n"
"    elif k == idltype.tk_ushort:     return 'ushort'\n"
"    elif k == idltype.tk_ulong:      return 'ulong'\n"
"    elif k == idltype.tk_float:      return 'float'\n"
"    elif k == idltype.tk_double:     return 'double'\n"
"    elif k == idltype.tk_boolean:    return 'boolean'\n"
"    elif k == idltype.tk_char:       return 'char'\n"
"    elif k == idltype.tk_octet:      return 'octet'\n"
"    elif k == idltype.tk_any:        return 'any'\n"
"    elif k == idltype.tk_TypeCode:   return 'CORBA::TypeCode'\n"
"    elif k == idltype.tk_Principal:  return 'CORBA::Principal'\n"
"    elif k == idltype.tk_objref:     return 'objref '+t.decl().repoId()\n"
"    elif k == idltype.tk_struct:     return structType(t.decl())\n"
//union
"    elif k == idltype.tk_enum:       return enumType(t)\n"
"    elif k == idltype.tk_string:     return 'string ' + str(t.bound())\n"
"    elif k == idltype.tk_sequence:   return sequenceType(t)\n"
//"    elif k == idltype.tk_array:      return arrayType(t)\n"
//array: this never happens
//alias: should not get here.
//except
"    elif k == idltype.tk_longlong:   return 'longlong'\n"
"    elif k == idltype.tk_ulonglong:  return 'ulonglong'\n"
"    elif k == idltype.tk_longdouble: return 'longdouble'\n"
"    elif k == idltype.tk_wchar:      return 'wchar'\n"
//wstring
//fixed
//value
//value_box
//native
//abstract
//local
"    return k #'fried'\n"
"\n"
"def ocpiType(t) :\n"
"    tu = t.unalias()\n"
"    sys.stderr.write(repr(t))\n"
"    sys.stderr.write(repr(tu))\n"
"    k = tu.kind()\n"
"    #sys.stderr.write(repr(k))\n"
// If the unaliased type is an alias it means complex/array declarator
"    if k == idltype.tk_alias:\n"
"       return ocpiSizes(tu.decl()) + ocpiTypeDesc(tu.decl().alias().aliasType())\n"
"    else:\n"
"       return ocpiTypeDesc(tu)\n"
"\n"
// arrays: struct member, exception member, typedef
"def doInterface(i, f):\n"
"    f.write('{0}\\n{1} {2} {3}\\n'.format(i.repoId(), i.identifier(), i.line(), i.file()))\n"
"    nOps = 0\n"
"    for o in i.callables() :\n"
"        if isinstance(o,idlast.Operation):\n"
"            nOps = nOps + 1\n"
"    f.write('i{0}\\n'.format(nOps))\n"
"    for o in i.callables() :\n"
"        if isinstance(o,idlast.Operation):\n"
"            f.write('{0}{1} {2} {3} {4}@{5} {6}\\n'.format(o.oneway(), o.identifier(), len(o.parameters()),\n"
"                                           len(o.raises()), ocpiType(o.returnType()), o.line(), o.file()))\n"
"            for a in o.parameters() :\n"
"                if isinstance(a,idlast.Parameter) :\n"
"                   f.write('{0}{1} {2}\\n'.\n"
"                           format(a.direction(), a.identifier(), \n"
"                                  ocpiType(a.paramType())))\n"
"            f.write('\\n')\n"
"            for e in o.raises() :\n"
"                if isinstance(e,idlast.Exception) :\n"
"                   f.write('{0} {1} {2} {3}\\n'.\n"
"                           format(e.identifier(), len(e.members()), e.line(), e.file()))\n"
"                for m in e.members() :\n"
"                    if isinstance(m,idlast.Member) :\n"
"                        f.write('{0} {1}\\n'.\n"
"                            format(m.declarators()[0].identifier(), \n"
"                                   ocpiType(m.memberType())))\n"
"                f.write('\\n')\n"
"            f.write('\\n')\n"
"    f.write('\\n')\n"
"\n"
"def doStruct(s, f):\n"
"    f.write('{0}\\n{1} {2} {3}\\n'.format(s.repoId(), s.identifier(), s.line(), s.file()))\n"
"    f.write('s{0}\\n'.format(structType(s)))\n"
"    f.write('\\n'.join(map(idlast.Pragma.text,s.pragmas()))+'\\n')\n"
"    f.write('\\n')\n"
"\n"
"def doDecls(decls, f):\n"
"    for i in decls :\n"
"        if isinstance(i,idlast.Interface):\n"
"            doInterface(i, f)\n"
"        elif isinstance(i,idlast.Module):\n"
"            doModule(i, f)\n"
"        elif isinstance(i,idlast.Struct):\n"
"            doStruct(i, f)\n"
"\n"
"def doModule(m, f):\n"
"    doDecls(m.definitions(),f)\n"
"\n"
"def run(tree, args):\n"
"    sys.stdout.write(sys.version)\n"
"    sys.stdout.write(repr(sys.path)+'\\n')\n"
"    doDecls(tree.declarations(),sys.stdout)\n"
"\n"
};

// This is from the gnu library and BSD, and is in darwin
//#if defined(OCPI_OS_linux) || defined(OCPI_OS_win32)
//#if defined(OCPI_OS_win32)
// Its too much trouble to keep track of when we need this and when we don't
// so its just redundantly present...
// Its in darwin and ubuntu, but not RHEL5 or linux
static int OURmkstemps(char *temp, size_t suffixlen) {
  char *cp = temp + strlen(temp) - suffixlen;
  char save = *cp;
  *cp = 0;
  int fd;
#if defined(OCPI_OS_win32)
  if (!mktemp(temp))
    return -1;
  *cp = save;
  fd = _open(temp, _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, _S_IREAD|_S_IWRITE);
#else
  if ((fd = mkstemp(temp)) >= 0) {
    char *old = strdup(temp);
    *cp = save;
    if (rename(old, temp)) {
      close(fd);
      fd = -1;
    }
    free(old);
  }
#endif
  return fd;
}
//#endif

// Like strcat but surrounding with double-quotes, and escaping them too.
static void appendQuoteEscape(char *base, const char *append) {
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

// to make the tmp directory portable
// Sort of like asprintf.  Outputs must be freed
// If return is < 0, then outputs are not set.
static int
mymkstemp(char **result, char **dir, const char *prefix, const char *suffix) {
  char *tmp = getenv("TMPDIR");
  int fd = -1;
  if (!tmp)
    tmp = getenv("TMP");
  if (!tmp)
    tmp = getenv("TEMP");
  if (!tmp)
    tmp = strdup(P_tmpdir);
  else
    tmp = strdup(tmp);
  char *cp = tmp + strlen(tmp) - 1;
  if (cp != tmp && strchr(OS::FileSystem::slashes, *cp))
    *cp = '\0';
  ocpiCheck(asprintf(&cp, "%s%c%sXXXXXX%s%s", tmp, OS::FileSystem::slashes[0], prefix,
		     suffix ? "." : "", suffix ? suffix : "") > 0);
  fd = OURmkstemps(cp, suffix ? strlen(suffix)+1 : 0);
  if (fd >= 0) {
    if (dir)
      *dir = tmp;
    else
      free(tmp);
    *result = cp;
  } else {
    free(tmp);
    free(cp);
  }
  return fd; 
}

// Run the command returning the stderr output as a string or
// setting the out argument to a string olding the stdout (repo).
static const char *
run(const char *command, char *&out)
{
  char *myCommand, *outFile, *errFile;
  int ofd, efd;
  if ((ofd = mymkstemp(&outFile, NULL, "ocpiso", NULL)) < 0)
    return "internal error running idl parser 1";
  const char *err = 0;
  do {
    if ((efd = mymkstemp(&errFile, NULL, "ocpise", NULL)) < 0) {
      err = "internal error running idl parser 2";
      break;
    }
    ocpiCheck(asprintf(&myCommand, "%s 1> %s 2> %s", command, outFile, errFile) > 0);
    //printf("IDLFront:%s\n", myCommand);
    // Windows requires the files be closed and then reopened...
    close(ofd); ofd = -1;
    close(efd); efd = -1;
    int systemResult = system(myCommand);
    switch (systemResult) {
    case -1:
      err = "internal error running idl parser 3";
      break;
    case 127:
      err = "internal error running idl parser 4";
      break;
    default:
      if ((efd = open(errFile, O_RDONLY)) < 0) {
	err = "internal error running idl parsed 4a";
	break;
      }
      off_t eSize = lseek(efd, 0, SEEK_END);
      if (eSize == 0) {
	off_t oSize;
	if ((ofd = open(outFile, O_RDONLY)) < 0) {
	  err = "internal error running idl parsed 6a";
	  break;
	}
	switch ((oSize = lseek(ofd, 0, SEEK_END))) {
	case 0:
	  if (systemResult)
	    ocpiCheck(asprintf((char **)&err, "No error output when IDL parser failed on: %s",
			       myCommand) > 0);
	  else
	    err = "No interfaces found in IDL file(s)";
	  break;
	case -1:
	  err = "internal error running idl parser 6";
	  break;
	default:
	  // since windows translates CRLF sequences, the size of the
	  // file is NOT what will be read.  Any CRLFs read as LFs.
	  if (lseek(ofd, 0, SEEK_SET) != 0 ||
	      !(out = (char *)malloc(oSize + 1)) ||
	      read(ofd, (char *)out, oSize) < 0)
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
	  ssize_t n;
	  // since windows translates CRLF sequences, the size of the
	  // file is NOT what will be read.  Any CRLFs read as LFs.
	  if (lseek(efd, 0, SEEK_SET) != 0 ||
	      !(myErr = (char *)malloc(eSize + 1)) ||
	      (n = read(efd, (void *)myErr, eSize)) < 0)
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
    if (efd >= 0)
      close(efd);
    unlink(errFile);
  } while (0);
  if (ofd >= 0)
    close(ofd);
  unlink(outFile);
  free(outFile);
  free(errFile);
  return err;
}

// Process the IDL files with -D/U/I options, return our "repository" format
 static const char *
idl2ifr(const char *const *argv, char *&repo)
{
  // To avoid all manner of environment and installation dependencies,
  // We put the python backend in a temp file, point omniidl to it, and then remove it.
  char *beTemp, *beDir;
  int fd;
  if ((fd = mymkstemp(&beTemp, &beDir, "be", "py")) < 0 ||
      write(fd, omniidl_be, sizeof(omniidl_be) - 1) != sizeof(omniidl_be) - 1 ||
      close(fd) != 0)
    return "internal error creating idl back end file";
  // Create the name we want the file to be
  const char *err = 0;
  repo = 0;
  do { //break on error to cleanup
    size_t size = 0, files = 0;
    for (const char * const *ap = argv; *ap; ap++) {
      size += strlen(*ap) * 2 + 3; // all escaped, quoted, spaced
      if (ap[0][0] == '-') {
	if (strchr("DUI", ap[0][1])) {
	  if (ap[0][2] == '\0') {
	    if (!*++ap) {
	      err = "expected one more argumnent after -D, -I, or -U";
	      break;
	    } else
	      size += strlen(*ap) * 2 + 3;
	  }
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
    char *beCopy = strdup(beTemp), *beName;
    for (beName = beCopy + strlen(beCopy) - 1; beName > beCopy; beName--) {
      if (*beName == '.')
	*beName = 0;
      if (strchr(OS::FileSystem::slashes, beName[-1]))
	break;
    }
    char *omni;
    ocpiCheck(asprintf(&omni, "/usr/local/bin/omniidl -p%s -b%s", beDir, beName) > 0);
    free(beCopy);
    char *cp = (char *)malloc(strlen(omni) + size + 1);
    strcpy(cp, omni);
    free(omni);
    // Fill the command buffer with options and file names
    for (const char * const *ap = argv; *ap; ap++) {
      appendQuoteEscape(cp, *ap);
      if (ap[0][0] == '-' && ap[0][2] == '\0')
	appendQuoteEscape(cp, *++ap);
    }
    err = run(cp, repo);
    free(cp);
  } while(0);
  unlink(beTemp); // remove the .py file we created and wrote
  char *dupc;
  ocpiCheck(asprintf(&dupc, "%sc", beTemp) > 0);
  free(beTemp);
  unlink(dupc); // remove the .pyc file that might have been created
  free(dupc);
  free(beDir);
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
// make a copy of the chars up to the terminating character and update the pointer
static void getString(std::string &s, const char *&p, const char *sep = " ") {
  const char *np = p;
  while (*np && !strchr(sep, *np))
    np++;
  size_t length = np - p;
  s.assign(p, length);
  p += length;
  if (*p)
    p++;
}
  // parse a positive number up to a space, and update the pointer
unsigned long getNum(const char *&cp, const char *term = " ") {
  unsigned n = atoi(cp);
  while (*cp && !strchr(term, *cp))
    cp++;
  if (*cp)
    cp++;
  return n;
}
  // look up the type kind
OA::BaseType getKind(std::string &kind) {
  unsigned k = 0;
  for (const char **ap = OU::idlTypeNames; *ap; ap++, k++)
    if (!strcasecmp(*ap, kind.c_str()))
      return (OA::BaseType)k;
  return OA::OCPI_none;
}

static void getMember(OU::Member &m, const char *&cp, const char *term);

// cp points to the type description, parse a type and return it
void getType(OU::ValueType &t, const char *&cp, const char *term) {
  const char *save = cp;
  std::string kind;
  getString(kind, cp, term);
  // Is this type a sequence of something
  if (kind == "sequence") {
    t.m_isSequence = true;
    t.m_sequenceLength = getNum(cp);
    save = cp;
    getString(kind, cp, term);
  }
  // Is this type a single or sequence of an array?
  if (kind == "array") {
    // An array of what comes after
    std::vector<size_t> dimensions;
    while (isdigit(*cp)) {
      dimensions.push_back(getNum(cp));
      t.m_arrayRank++;
    }
    t.m_arrayDimensions = new size_t[t.m_arrayRank];
    for (unsigned i = 0; i < t.m_arrayRank; i++)
      t.m_arrayDimensions[i] = dimensions[i];
    save = cp;
    getString(kind, cp, term);
  }
  // Is this a recursive simple type?
  if (kind == "array" || kind == "sequence") {
    t.m_baseType = OA::OCPI_Type;
    cp = save;
    t.m_type = new OU::Member;
    getType(*t.m_type, cp, term);
  } else {
    t.m_baseType = getKind(kind);
    switch (t.m_baseType) {
    case OA::OCPI_Enum:
      t.m_nEnums = getNum(cp);
      if (t.m_nEnums)
	t.m_enums = new const char *[t.m_nEnums];
      for (const char **e = t.m_enums; *cp != '.'; e++) {
	std::string en;
	getString(en, cp);
	*e = new char[en.length() + 1];
	strcpy((char *)*e, en.c_str());
      }
      cp += 2; // skip period and type terminator
      break;
    case OA::OCPI_Struct:
      t.m_nMembers = getNum(cp);
      if (t.m_nMembers)
	t.m_members = new OU::Member[t.m_nMembers];
      for (OU::Member *m = t.m_members; *cp != '.'; m++)
	getMember(*m, cp, " ;");
      cp += 2; // skip period and type terminator
      break;
    case OA::OCPI_String:
      t.m_stringLength = getNum(cp, term);
      break;
#if 0      
    case Type::OBJREF:
      // FIXME: this could be recursive
      {
	char *repid = getString(tp);
	t.m_interface = new InterfaceI(repid, this, file, pos, "", "");
	free(repid);
      }
      break;
#endif
    default:
      ;
    }
  }
}

static void
getMember(OU::Member &m, const char *&cp, const char *term) {
  getString(m.m_name, cp);
  getType(m, cp, term);
}

  // Parameters can be operation parameters, or exception members
static void
getArg(OU::Member &m, const char *&cp, const char *term) {
  char inout = *cp++;
  getString(m.m_name, cp);
  getType(m, cp, term);
  m.m_isIn = inout == '0' || inout == '2';
  m.m_isOut = inout == '1' || inout == '2';
}

// Initialize a protocol from an interface in the repo
static void 
doInterface(OU::Protocol &p, const char *&cp) {
  p.m_nOperations = getNum(cp, "\n");
  if (p.m_nOperations)
    p.m_operations = new OU::Operation[p.m_nOperations];
  // Loop over operations
  for (OU::Operation *op = p.m_operations; *cp != '\n'; op++) {
    op->m_isTwoWay = *cp++ != '1';
    getString(op->m_name, cp);
    op->m_nArgs = getNum(cp);
    op->m_nExceptions = getNum(cp);
    if (op->m_isTwoWay)
      op->m_nArgs++;
    if (op->m_nArgs)
      op->m_args = new OU::Member[op->m_nArgs];
    OU::Member *m = op->m_args;
    if (!op->m_isTwoWay) {
      OU::Member dummy;
      getType(dummy, cp, "@");
    } else
      getType(*m++, cp, "@");
    unsigned line, len;
    int n = sscanf(cp, "%u %n", &line, &len);
    assert(n == 1);
    cp += len;
    std::string tmp;
    getString(tmp, cp, "\n"); // skip redundant filename
    // Loop over parameters
    for (; *cp != '\n'; m++)
      getArg(*m, cp, "\n");
    cp++;
    // Loop over exceptions - which we use "operations" for.
    if (op->m_nExceptions)
      op->m_exceptions = new OU::Operation[op->m_nExceptions];
    for (OU::Operation *ex = op->m_exceptions; *cp != '\n'; ex++) {
      getString(ex->m_name, cp);
      ex->m_nArgs = getNum(cp);
      n = sscanf(cp, "%u %n", &line, &len);
      assert(n == 1);
      cp+= len;
      getString(tmp, cp, "\n");
      addDep(tmp.c_str(), false);
      if (ex->m_nArgs)
	ex->m_args = new OU::Member[ex->m_nArgs];
      // Loop over parameters to exception
      for (OU::Member *em = ex->m_args; *cp != '\n'; em++)
	getMember(*em, cp, " \n");
      cp++;
    }
  }
}

static const char *
doStruct(OU::Protocol &p, const char *&cp) {
  std::string tmp;
  getString(tmp, cp);
  assert(tmp == "struct");

  p.m_nOperations = 1;
  p.m_operations = new OU::Operation[1];
  OU::Operation &op = p.m_operations[0];
  op.m_nArgs = getNum(cp);
  op.m_name = p.m_name;
  op.m_qualifiedName = p.m_qualifiedName;
  op.m_isTwoWay = false;
  if (op.m_nArgs)
    op.m_args = new OU::Member[op.m_nArgs];  
  for (OU::Member *m = op.m_args; *cp != '.'; m++)
    getMember(*m, cp, " ;");
  cp += 2; // skip period and nl
  while (*cp != '\n') {
    getString(tmp, cp, "\n");
    const char *prag = tmp.c_str();
    std::string tmp1;
    getString(tmp1, prag, " \n");
    if (tmp1 == "keylist" && *prag != '\n') {
      while (isspace(*prag)) // don't trust pragma spacing
	prag++;
      getString(tmp1, prag, " \n");
      if (tmp1 == p.m_name) {
	while (*prag) {
	  getString(tmp1, prag, " \n");
	  OU::Member *a = op.m_args;
	  while (isspace(*prag)) // don't trust pragma spacing
	    prag++;
	  unsigned n;
	  for (n = 0; n < op.m_nArgs; n++, a++)
	    if (tmp1 == a->m_name) {
	      a->m_isKey = true;
	      break;
	    }
	  if (n >= op.m_nArgs)
	    return OU::esprintf("Pragma refers to non-existent field \"%s\" for data type \"%s\"",
			    tmp1.c_str(), op.m_name.c_str());
	}
      } else
	return OU::esprintf("Invalid Pragma doesn't match struct name: %s", tmp.c_str());
    } else
      return OU::esprintf("Unknown Pragma: %s", tmp.c_str());
  }
  cp++;
  return NULL;
}
const char *
emitProtocol(const char *const *argv, const char *outDir, const char *file,
	     const char *structName) {
  const char *err = 0;
  std::string s;
  if (!structName) {
    const char
      *dot = strrchr(file, '.'),
      *slash = strrchr(file, '/');
    if (slash) {
      slash++;
      if (dot && dot < slash)
	dot = 0;
    } else
      slash = file;
    if (!dot)
      dot = file + strlen(file);
    s.assign(slash, dot - slash);
    structName = s.c_str();
  }
  char *repo;
  addInclude(file);
  if ((err = idl2ifr(argv, repo)))
    return err;
  const char *cp = repo;
  std::string l1, l2;
  getString(l1, cp, "\n");
  getString(l2, cp, "\n");
  //  printf("Repo l1: %s\n", l1.c_str());
  //  printf("Repo l2: %s\n", l2.c_str());
  // Iterate through repository
  bool found = false;
  for (; !found && *cp; cp++) {
    OU::Protocol p;
    getString(l1, cp, "\n");
    const char *qEnd = strrchr(l1.c_str(), ':');
    for (const char *q = strchr(l1.c_str(), ':') + 1; q < qEnd; q++)
      if (*q == '/')
	p.m_qualifiedName += "::";
      else
	p.m_qualifiedName += *q;
    getString(p.m_name, cp);
    unsigned line, len;
    int n = sscanf(cp, "%u %n", &line, &len);
    assert(n == 1);
    cp += len;
    std::string tmp;
    getString(tmp, cp, "\n");
    addDep(tmp.c_str(), false); // let's assume they are all absolute?
    //    printf("RepId %s, name %s, line %u, file %s\n", 
    //	   p.m_qualifiedName.c_str(), p.m_name.c_str(), line, tmp.c_str());
    switch (*cp++) {
    case 'i':
      //      printf("Found Interface\n");
      doInterface(p, cp);
      break;
    case 's':
      //      printf("Found Struct\n");
      if ((err = doStruct(p, cp)))
	break;
      if (!strcasecmp(p.m_name.c_str(), structName)) {
	FILE *f;
	if (!(err = openOutput(structName, outDir, "", "_protocol", ".xml", NULL, f))) {
	  printgen(f, "<!--", file, false, " -->");
	  std::string out;
	  p.printXML(out, 0);
	  fputs(out.c_str(), f);
	  fclose(f);
	}
	found = true;
      }
    default:
      ;
    }
  }
  if (!found)
    err = OU::esprintf("Could not find struct named \"%s\" in IDL file \"%s\"",
		   structName, file);
  free(repo);
  return err;
}
