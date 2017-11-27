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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include "OcpiDriverManager.h"
#include "OcpiLibraryManager.h"
#include "wip.h"
#include "hdl-container.h"
/*
 * Notes:  For verilog, for consistency, we generate a module definition that is "included"
 * in the skeleton file so it is readonly, and can be regenerated after there is code added
 * to the skeleton.  But we have an option to insert the redundant definition directly for
 * those that do not like "includes" in verilog.
 *
 * VHDL wrapping verilog (XST): declare a VHDL component, preserving case, when no VHDL is
 * found, the verilog will be found.  We will use this for our assemblies where workers are
 * instantiated inside the assemblies and the worker is verilog.
 *
 * Verilog wrapping VHDL: VHDL must be stored with an extended identifier. You don't need a
 * component declaration.  We will use this for instantiating assemblies into infrastructure
 * since our assemblies will be VHDL.
 *
 * case rules for module and port names may be funky.  isim says port names are case
 * insensitive..
 * Thus we must define case rules to enable this to work properly...

todo:
what to do with impl parameters/generics?
special treatment of WIP attribute parameters/generics?
XML might say which attributes can be mapped into parameters/generics
impl property aspecs
verilog
tieoffs
assembly
isim
merge code with runtime
add to tree.
 */
  //          name      abbrev  type    value description
#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpigen [options] <xml-worker-descriptor-file>\n" \
  "After options, the single argument is the name of the XML file to parse as input.\n" \
  "This argument can exclude the .xml suffix, which will be assumed.\n" \
  "XML file can be a worker, an assembly, a platform configuration, or a container.\n"
#define OCPI_OPTIONS \
  CMD_OPTION  (verbose,   v,    Bool,   NULL, "Be verbose") \
  CMD_OPTION  (defs,      d,    Bool,   NULL, "Generate the definition file (used for instantiation)") \
  CMD_OPTION  (impl,      i,    Bool,   NULL, "Generate the implementation header file (readonly)") \
  CMD_OPTION  (skel,      s,    Bool,   NULL, "Generate the implementation skeleton file (modified part)") \
  CMD_OPTION  (assy,      a,    Bool,   NULL, "Generate the assembly implementation file (readonly)") \
  CMD_OPTION  (parameters,r,    Bool,   NULL, "Process raw parameters on stdin") \
  CMD_OPTION  (build,     b,    Bool,   NULL, "Generate gen/Makefile from <worker.build>") \
  CMD_OPTION  (xml,       A,    Bool,   NULL, "Generate the artifact XML file for embedding") \
  CMD_OPTION  (workers,   W,    Bool,   NULL, "Generate the makefile fragment for workers in the assembly") \
  CMD_OPTION  (generics,  g,    Short,  "-1", "Generate the generics file a worker configuration") \
  CMD_OPTION  (attribute, x,    String, NULL, "Emit to standard output the value of an XML attribute") \
  CMD_OPTION  (alternate, w,    Bool,   NULL, "Use the alternate language (VHDL vs. Verilog) when generating defs and impl") \
  CMD_OPTION  (dependency,M,    String, NULL, "Specify the name of the dependency file when processing XML") \
  CMD_OPTION_S(library,   l,    String, NULL, "Add a library dependency for the worker") \
  CMD_OPTION_S(include,   I,    String, NULL, "Add an XML include directory to search") \
  CMD_OPTION  (directory, D,    String, NULL, "Specify the directory in which to put output generated files") \
  CMD_OPTION  (assembly,  S,    String, NULL, "Specify the the default assembly for containers") \
  CMD_OPTION_S(map,       L,    String, NULL, "Add a mapping from lib-name to its pathname <libname>:<path>") \
  CMD_OPTION  (dev,       e,    String, NULL, "Specify the device target for the artifact XML") \
  CMD_OPTION  (platform,  P,    String, NULL, "Specify the platform target for the artifact XML") \
  CMD_OPTION  (os,        O,    String, NULL, "Specify the operating system target for the artifact XML") \
  CMD_OPTION  (os_version,V,    String, NULL, "Specify the operating system version for the artifact XML") \
  CMD_OPTION  (arch,      H,    String, NULL, "Specify the architecture for the artifact") \
  CMD_OPTION  (package,   p,    String, NULL, "Specify the HDL package for the worker") \
  CMD_OPTION  (pfconfig,  X,    String, NULL, "Parse top level platform/configuration attribute") \
  CMD_OPTION  (pfdir,     F,    String, NULL, "The directory where the current platform lives") \
  CMD_OPTION  (gentest,   T,    Bool,   NULL, "Generate unit testing files, assemblies, apps")  \
  CMD_OPTION  (gencases,  C,    Bool,   NULL, "Figure out which test cases to run on which platforms") \

#define OCPI_OPTION
#define OCPI_OPTIONS_NO_MAIN
#include "CmdOption.h"

int
main(int argc, const char **argv) {
  OCPI::Driver::ManagerManager::suppressDiscovery();
  if (options.setArgv(argv))
    return 1;
  const char *outDir = NULL, *wksFile = NULL, *package = NULL;
  bool
    doDefs = false, doImpl = false, doSkel = false, doAssy = false, doWrap = false,
    doArt = false, doTopContainer = false, doTest = false, doCases = false, verbose = false,
    doTopConfig = false;
  int doGenerics = -1;
  if (argc <= 1) {
    fprintf(stderr,
	    "Usage is: ocpigen [options] <owd>.xml\n"
	    " Code generation options that determine which files are created:\n"
	    " -d            Generate the definition/instantiation file: xyz_defs.[vh|vhd]\n"
	    " -i            Generate the implementation include file: xyz_impl.[vh|vhd]\n"
	    " -s            Generate the skeleton file: xyz_skel.[c|v|vhd]\n"
	    " -a            Generate the assembly (composition) file: xyz.[v|vhd]\n"
	    " -b            Generate the BSV interface file\n"
	    " -A            Generate the UUID and artifact descriptor xml file for a container\n"
	    " -W <file>     Generate an assembly or container workers file: xyz.wks\n"
	    " Options for artifact XML and UUID source generation (-A):\n"
	    " -c <file>     The HDL container file to use for the artifact XML\n"
	    " -P <platform> The platform for the artifact (or cpu for rcc)\n"
	    " -O <os>       The OS for the artifact (rcc)\n"
	    " -V <version>  The OS version for the artifact (rcc)\n"
	    " -e <device>   The device for the artifact\n"
            " -p <package>  The package name for component specifications\n"
	    " Other options:\n"
	    " -l <lib>      The VHDL library name that <www>_defs.vhd will be placed in (-i)\n"
	    " -L <complib>:<xml-dir>\n"
	    "               Associate a component library name with an xml search dir\n"
	    " -D <dir>      Specify the output directory for generated files\n"
	    " -I <dir>      Specify an include search directory for XML processing\n"
	    " -M <file>     Specify the file to write makefile dependencies to\n"
	    " -S <assembly> Specify the name of the assembly for a container\n"
	    " -T            Generate test artifacts\n"
	    );
    return 1;
  }
#if !defined(NDEBUG)
  std::string params;
  for (int i = 0; i < argc; ++i)
    params.append(argv[i]).append(" ");
  ocpiDebug("ocpigen command line: '%s'\n", params.c_str());
#endif
  const char *err = 0;
  for (const char **ap = argv+1; !err && *ap; ap++) {
    if (ap[0][0] == '-')
      switch (ap[0][1]) {
      case 'v':
	verbose = true;
	break;
      case 'd':
	doDefs = true;
	break;
      case 'i':
	doImpl = true;
	break;
      case 's':
	doSkel = true;
	break;
      case 'a':
	doAssy = true;
	break;
      case 'A':
	doArt = true;
	break;
      case 'w':
	doWrap = true;
	break;
      case 'X':
	doTopContainer = true;
	break;
      case 'Y':
	doTopConfig = true;
	break;
      case 'T':
	doTest = true;
	break;
      case 'C':
	doCases = true;
	break;
      case 'g':
	doGenerics = atoi(&ap[0][2]);
	break;
      case 'W':
	wksFile =*++ap;
	break;
      case 'M':
	setDep(*++ap);
	break;
      case 'l':
	err = addLibrary(ap[0][2] ? &ap[0][2] : *++ap);
	break;
      case 'I':
	if (ap[0][2])
	  addInclude(&ap[0][2]);
	else
	  addInclude(*++ap);
	break;
      case 'D':
	outDir = *++ap;
	break;
      case 'S':
	assembly = *++ap;
	break;
      case 'L':
	if ((err = addLibMap(ap[0][2] ? &ap[0][2] : *++ap)))
	  err = OU::esprintf("Error processing -L (library map) option: %s", err);
	break;
      case 'e':
	g_device = *++ap;
	break;
      case 'P':
	g_platform = *++ap;
	break;
      case 'O':
	g_os = *++ap;
	break;
      case 'V':
	g_os_version = *++ap;
	break;
      case 'H':
	g_arch = *++ap;
	break;
      case 'p':
	package = *++ap;
	break;
      case 'x':
	attribute = *++ap;
	break;
      case 'r':
	break;
      case 'b':
	break;
      case 'F':
	platformDir = *++ap;
	break;
      default:
	err = OU::esprintf("Unknown flag: %s\n", *ap);
      }
    else
      try {
	setenv("OCPI_SYSTEM_CONFIG", "", 1);
	OCPI::Driver::ManagerManager::suppressDiscovery();
	if (doCases)
	  OCPI::Library::getManager().enableDiscovery();
	std::string parent;
	ezxml_t xml;
	std::string file;
	if (doTopContainer) {
	  std::string config, constraints;
	  OrderedStringSet platforms;
	  if ((err = parseFile(*ap, parent, "HdlContainer", &xml, file, false, false)) ||
	      (err = HdlContainer::parsePlatform(xml, config, constraints, platforms))) {
	    err = OU::esprintf("for container file %s:  %s\n", *ap, err);
	    break;
	  }
	  printf("%s %s", config.c_str(), constraints.c_str());
	  for (auto pi = platforms.begin(); pi != platforms.end(); ++pi)
	    printf(" %s", (*pi).c_str());
	  fputs("\n", stdout); 
	  return 0;
	}
	if (doTopConfig) {
	  if ((err = parseFile(*ap, parent, "HdlConfig", &xml, file, false, false))) {
	    err = OU::esprintf("for platform configuration file %s:  %s\n", *ap, err);
	    break;
	  }
	  const char *csf = ezxml_cattr(xml, "constraints");
	  if (csf)
	    printf("%s\n", csf);
	  return 0;
	}
	if (doTest) {
	  if ((err = createTests(*ap, package, outDir, verbose)))
	    break;
	  return 0;
	}
	if (doCases) {
	  if ((err = createCases(ap, package, outDir, verbose)))
	    break;
	  return 0;
	}
	Worker *w = Worker::create(*ap, parent, package, outDir, NULL, NULL,
				   doGenerics >= 0 ? doGenerics : 0, err);
	if (err)
	  err = OU::esprintf("For file %s: %s", *ap, err);
	else if (attribute && (err = w->emitAttribute(attribute)))
	  err = OU::esprintf("%s: Error retrieving attribute %s from file: %s", attribute, *ap, err);
	else if (doDefs && (err = w->emitDefsHDL(doWrap)))
	  err = OU::esprintf("%s: Error generating definition/declaration file: %s", *ap, err);
	else if (doImpl && (err =
			    w->m_model == HdlModel ? w->emitImplHDL(doWrap) :
			    (w->m_model == RccModel ? w->emitImplRCC() : w->emitImplOCL())))
	  err = OU::esprintf("%s: Error generating implementation declaration file: %s", *ap, err);
	else if (doSkel && (err =
			    w->m_model == HdlModel ? w->emitSkelHDL() :
			    w->m_model == RccModel ? w->emitSkelRCC() : w->emitSkelOCL()))
	  err = OU::esprintf("%s: Error generating implementation skeleton file: %s", *ap, err);
	else if (doAssy && (err = w->emitAssyHDL()))
	  err = OU::esprintf("%s: Error generating assembly: %s", *ap, err);
	else if (wksFile && (err = w->emitWorkersHDL(wksFile)))
	  err = OU::esprintf("%s: Error generating assembly makefile: %s", *ap, err);
	else if (doGenerics >= 0 && (err = w->emitHDLConstants((unsigned)doGenerics, doWrap)))
	  err = OU::esprintf("%s: Error generating constants for parameter configuration %u: %s",
			     *ap, doGenerics, err);
	else if (options.parameters() && (err = w->emitToolParameters()))
	  err = OU::esprintf("%s: Error generating parameter file for tools: %s", *ap, err);
	else if (options.build() && (err = w->emitMakefile()))
	  err = OU::esprintf("%s: Error generating gen/*.mk file for tools", err);
	else if (doArt)
	  switch (w->m_model) {
	  case HdlModel:
	    if (!g_platform || !g_device)
	      err = OU::esprintf("%s: Missing container/platform/device options for HDL "
				 "artifact descriptor", *ap);
	    else if ((err = w->emitArtXML(wksFile)))
	      err = OU::esprintf("%s: Error generating bitstream artifact XML: %s",
				 *ap, err);
	    break;
	  case RccModel:
	    if (!g_os || !g_os_version || !g_arch)
	      err = OU::esprintf("%s: Missing os/os_version/arch options for RCC artifact "
				 "descriptor", *ap);
	    else if ((err = w->emitArtXML(wksFile)))
	      err = OU::esprintf("%s: Error generating shared library artifact XML: %s",
				 *ap, err);
	    break;
	  case OclModel:
	    if ((err = w->emitArtXML(wksFile)))
	      err = OU::esprintf("%s: Error generating shared library artifact XML: %s",
		      *ap, err);
	    break;
	  case NoModel:
	    ;
	  }
	delete w;
      } catch (std::string &e) {
	fprintf(stderr, "Exception thrown: %s\n", e.c_str());
	return 1;
      } catch (...) {
	fprintf(stderr, "Unexpected/unknown exception thrown\n");
	return 1;
      }
  }
  if (!err)
    err = closeDep();
  if (err)
    fprintf(stderr, "%s\n", err);
 return err ? 1 : 0;
}
