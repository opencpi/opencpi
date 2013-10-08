
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


int
main(int argc, char **argv) {
  const char *library = "work", *outDir = NULL, *wksFile = NULL, *package = NULL;
  bool
    doDefs = false, doImpl = false, doSkel = false, doAssy = false, doWrap = false,
    doBsv = false, doArt = false, doContainer = false;
  if (argc <= 1) {
    fprintf(stderr,
	    "Usage is: ocpigen [options] <owd>.xml\n"
	    " Code generation options that determine which files are created:\n"
	    " -d            Generate the definition/instantiation file: xyz_defs.[vh|vhd]\n"
	    " -i            Generate the implementation include file: xyz_impl.[vh|vhd]\n"
	    " -s            Generate the skeleton file: xyz_skel.[c|v|vhd]\n"
	    " -a            Generate the assembly (composition) file: xyz.[v|vhd]\n"
	    " -b            Generate the BSV interface file\n"
	    " -A            Generate the artifact descriptor xml file\n"
	    " -C            Generate an HDL container\n"
	    " -W <file>     Generate an assembly or container workers file: xyz.wks\n"
	    " Options for artifact XML and UUID source generation (-A):\n"
	    " -c <file>     The HDL container file to use for the artifact XML\n"
	    " -P <platform> The platform for the artifact (or cpu for rcc)\n"
	    " -O <os>       The OS for the artifact (rcc)\n"
	    " -V <version>  The OS version for the artifact (rcc)\n"
	    " -e <device>   The device for the artifact\n"
	    " -L <loadinfo> The load information for the device\n"
            " -p <package>  The package name for component specifications\n"
	    " Other options:\n"
	    " -l <lib>      The VHDL library name that <www>_defs.vhd will be placed in (-i)\n"
	    " -D <dir>      Specify the output directory for generated files\n"
	    " -I <dir>      Specify an include search directory for XML processing\n"
	    " -M <file>     Specify the file to write makefile dependencies to\n"
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
      case 'C':
	doContainer = true;
	break;
      case 'W':
	wksFile =*++ap;
	break;
      case 'M':
	depFile = *++ap;
	break;
      case 'I':
	if (ap[0][2])
	  addInclude(&ap[0][2]);
	else
	  addInclude(*++ap);
	break;
      case 'l':
	library = *++ap;
	break;
      case 'c':
	container = *++ap;
	break;
      case 'D':
	outDir = *++ap;
	break;
      case 'L':
	load = *++ap;
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
      default:
	fprintf(stderr, "Unknown flag: %s\n", *ap);
	return 1;
      }
    else {
      const char *root = outDir;
#if 0
      char *root = strdup(*ap);
      char *dot = strrchr(root, '.');
      if (!dot) {
	fprintf(stderr, "%s: No period to define filename extension\n", *ap);
	return 1;
      }
      *dot = 0;
      if (outDir) {
	char *slash = strrchr(root, '/');
	asprintf(&root, "%s/%s", outDir, slash ? slash + 1 : root);
      }
#endif
      Worker *w = new Worker();
      if ((err = w->parse(*ap, NULL, package)))
	fprintf(stderr, "For file %s: %s\n", *ap, err);
      else if (doDefs && (err = w->emitDefsHDL(root)))
	fprintf(stderr, "%s: Error generating definition/declaration file: %s\n", *ap, err);
      else if (doImpl && (err =
			  w->model == HdlModel ? w->emitImplHDL(root, library) :
			  (w->model == RccModel ? emitImplRCC : emitImplOCL)(w, root, library)))
	fprintf(stderr, "%s: Error generating implementation declaration file: %s\n", *ap, err);
    else if (doSkel && (err =
			w->model == HdlModel ? w->emitSkelHDL(root) :
			(w->model == RccModel ? emitSkelRCC : emitSkelOCL)(w, root)))
	fprintf(stderr, "%s: Error generating implementation skeleton file: %s\n", *ap, err);
      else if (doWrap && (err = w->emitDefsHDL(root, true)))
	fprintf(stderr, "%s: Error generating wrapper file: %s\n", *ap, err);
      else if (doAssy && (err = w->emitAssyHDL(root)))
	fprintf(stderr, "%s: Error generating assembly: %s\n", *ap, err);
      else if (wksFile && !container && (err = w->emitWorkersHDL(root, wksFile)))
	fprintf(stderr, "%s: Error generating assembly makefile: %s\n", *ap, err);
      else if (doBsv && (err = w->emitBsvHDL(root)))
	fprintf(stderr, "%s: Error generating BSV import file: %s\n", *ap, err);
      //      else if (doContainer && (err = emitContainerHDL(w, root)))
      //	fprintf(stderr, "%s: Error generating HDL container file: %s\n", *ap, err);
      else if (doArt)
	switch (w->model) {
	case HdlModel:
	  if (!container || !platform || !device) {
	    fprintf(stderr,
		    "%s: Missing container/platform/device options for HDL artifact descriptor", *ap);
	    return 1;
	  }
	  if ((err = w->emitArtHDL(root, wksFile)))
	    fprintf(stderr, "%s: Error generating bitstream artifact XML: %s\n",
		    *ap, err);
	  break;
	case RccModel:
	  if (!os || !os_version || !platform) {
	    fprintf(stderr,
		    "%s: Missing os/os_version/platform options for RCC artifact descriptor", *ap);
	    return 1;
	  }
	  if ((err = emitArtRCC(w, root)))
	    fprintf(stderr, "%s: Error generating shared library artifact XML: %s\n",
		    *ap, err);
    break;
	case OclModel:
	  if ((err = emitArtOCL(w, root)))
	    fprintf(stderr, "%s: Error generating shared library artifact XML: %s\n",
		    *ap, err);
    break;
	case NoModel:
	  ;
	}
      cleanWIP(w);
    }
  return err ? 1 : 0;
}
