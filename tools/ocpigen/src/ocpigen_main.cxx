
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include "wip.h"
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
  CMD_OPTION  (defs,      d,    Bool,   NULL, "Generate the definition file (used for instantiation)") \
  CMD_OPTION  (impl,      i,    Bool,   NULL, "Generate the implementation header file (readonly)") \
  CMD_OPTION  (skel,      s,    Bool,   NULL, "Generate the implementation skeleton file (modified part)") \
  CMD_OPTION  (assy,      a,    Bool,   NULL, "Generate the assembly implementation file (readonly)") \
  CMD_OPTION  (parameters,r,    Bool,   NULL, "Process raw parameters on stdin") \
  CMD_OPTION  (xml,       A,    Bool,   NULL, "Generate the artifact XML file for embedding") \
  CMD_OPTION  (bsv,       b,    Bool,   NULL, "Generate the BlueSpec interface file (broken)") \
  CMD_OPTION  (workers,   W,    Bool,   NULL, "Generate the makefile fragment for workers in the assembly") \
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
  CMD_OPTION  (package,   p,    String, NULL, "Specify the HDL package for the worker") \
  CMD_OPTION  (config,    c,    String, NULL, "Specify the configuration for the artifact XML") \

#include "CmdOption.h"

int
main(int argc, char **argv) {
  if (options.setArgv(argv))
    return 1;
  const char *outDir = NULL, *wksFile = NULL, *package = NULL;
  bool
    doDefs = false, doImpl = false, doSkel = false, doAssy = false, doWrap = false,
    doBsv = false, doArt = false;
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
	    );
    return 1;
  }
  const char *err = 0;
  for (char **ap = argv+1; *ap; ap++)
    if (ap[0][0] == '-')
      switch (ap[0][1]) {
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
      case 'b':
	doBsv = true;
	break;
      case 'w':
	doWrap = true;
	break;
      case 'W':
	wksFile =*++ap;
	break;
      case 'M':
	depFile = *++ap;
	break;
      case 'l':
	if (ap[0][2])
	  addLibrary(&ap[0][2]);
	else
	  addLibrary(*++ap);
	break;
      case 'I':
	if (ap[0][2])
	  addInclude(&ap[0][2]);
	else
	  addInclude(*++ap);
	break;
      case 'c':
	container = *++ap;
	break;
      case 'D':
	outDir = *++ap;
	break;
      case 'S':
	assembly = *++ap;
	break;
      case 'L':
#if 0
	load = *++ap;
#endif
	if (ap[0][2])
	  err = addLibMap(&ap[0][2]);
	else
	  err = addLibMap(*++ap);
	if (err) {
	  fprintf(stderr, "Error processing -L (library map) option: %s\n", err);
	  return 1;
	}
	break;
      case 'e':
	device = *++ap;
	break;
      case 'P':
	platform = *++ap;
	break;
      case 'O':
	os = *++ap;
	break;
      case 'V':
	os_version = *++ap;
	break;
      case 'p':
	package = *++ap;
	break;
      case 'x':
	attribute = *++ap;
	break;
      case 'r':
	break;
      default:
	fprintf(stderr, "Unknown flag: %s\n", *ap);
	return 1;
      }
    else
      try {
	Worker *w = Worker::create(*ap, NULL, package, outDir, NULL, 0, err);
	if (err)
	  fprintf(stderr, "For file %s: %s\n", *ap, err);
	else if (attribute && (err = w->emitAttribute(attribute)))
	  fprintf(stderr, "%s: Error retrieving attribute %s from file: %s\n", attribute, *ap, err);
	else if (doDefs && (err = w->emitDefsHDL(doWrap)))
	  fprintf(stderr, "%s: Error generating definition/declaration file: %s\n", *ap, err);
	else if (doImpl && (err =
			    w->m_model == HdlModel ? w->emitImplHDL(doWrap) :
			    (w->m_model == RccModel ? w->emitImplRCC() : emitImplOCL(w))))
	  fprintf(stderr, "%s: Error generating implementation declaration file: %s\n", *ap, err);
	else if (doSkel && (err =
			    w->m_model == HdlModel ? w->emitSkelHDL() :
			    w->m_model == RccModel ? w->emitSkelRCC() : emitSkelOCL(w)))
	  fprintf(stderr, "%s: Error generating implementation skeleton file: %s\n", *ap, err);
	else if (doAssy && (err = w->emitAssyHDL()))
	  fprintf(stderr, "%s: Error generating assembly: %s\n", *ap, err);
	else if (wksFile && (err = w->emitWorkersHDL(wksFile)))
	  fprintf(stderr, "%s: Error generating assembly makefile: %s\n", *ap, err);
	else if (doBsv && (err = w->emitBsvHDL()))
	  fprintf(stderr, "%s: Error generating BSV import file: %s\n", *ap, err);
	else if (options.parameters() && (err = w->emitToolParameters()))
	  fprintf(stderr, "%s: Error generating parameter file for tools: %s\n", *ap, err);
	else if (doArt)
	  switch (w->m_model) {
	  case HdlModel:
	    if (!platform || !device) {
	      fprintf(stderr,
		      "%s: Missing container/platform/device options for HDL artifact descriptor", *ap);
	      return 1;
	    }
	    if ((err = w->emitArtXML(wksFile)))
	      fprintf(stderr, "%s: Error generating bitstream artifact XML: %s\n",
		      *ap, err);
	    break;
	  case RccModel:
	    if (!os || !os_version || !platform) {
	      fprintf(stderr,
		      "%s: Missing os/os_version/platform options for RCC artifact descriptor", *ap);
	      return 1;
	    }
	    if ((err = w->emitArtXML(wksFile)))
	      fprintf(stderr, "%s: Error generating shared library artifact XML: %s\n",
		      *ap, err);
	    break;
	  case OclModel:
	    if ((err = emitArtOCL(w)))
	      fprintf(stderr, "%s: Error generating shared library artifact XML: %s\n",
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
  return err ? 1 : 0;
}
