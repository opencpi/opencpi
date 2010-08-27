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
    const char *library = "work", *hdlDep = 0, *outDir = 0;
  bool
    doDefs = false, doImpl = false, doSkel = false, doAssy = false, doWrap = false,
    doBsv = false, doArt = false;
  if (argc <= 1) {
    fprintf(stderr,
	    "Usage is: wipgen [-sdf] <owd>.xml\n"
	    " This tool can produce any combination of 4 types of output files:\n"
	    " 1. <www>_defs.{v|vhd}:  declarations used only to instantiate (read only).\n"
	    "                         VHDL: the component declaration\n"
	    "                         Verilog: the empty module declaration\n"
	    "                         RCC: nothing to generate for instantiation\n"
	    " 2. <www>_impl.{v|vhd}:  declarations used to implement (read only).\n"
	    "                         VHDL: the entity declaration, etc.\n"
	    "                         Verilog: the module declaration.\n"
	    "                         RCC: the generated header file.\n"
	    " 3. <www>.{v|vhd}:       a skeleton of an implementation file (read/write0.\n"
	    "                         VHDL: the architecture body, etc.\n"
	    "                         Verilog: the module body\n"
	    "                         RCC: the actual skeleton\n"
	    " 4. <www>_wrap.{v|vhd}:  a cross-language wrapper file\n"
	    " Options are:\n"
	    " -l <lib>     The VHDL library name that <www>_defs.vhd will be placed in\n"
	    " -d           Generate the definition/instantiation file\n"
	    " -i           Generate the implementation include file\n"
	    " -s           Generate the skeleton file\n"
	    " -a           Generate the assembly (composition) file\n"
	    " -h <file>    Read an HDL container file for the assembly\n"
	    " -A           Generate the artifact descriptor xml file\n"
	    " -D <dir>     Specify the output directory\n"
	    " -I <dir>     Specify an include directory\n"
	    );
    return 1;
  }
  const char *err;
  for (char **ap = argv+1; *ap; ap++)
    if (ap[0][0] == '-')
      switch (ap[0][1]) {
      case 'I':
	if (ap[0][2])
	  addInclude(&ap[0][2]);
	else
	  addInclude(*++ap);
	break;
      case 'l':
	library = *++ap;
	break;
      case 'h':
	hdlDep = *++ap;
	break;
      case 'D':
	outDir = *++ap;
	break;
      case 'A':
	doArt = true;
	break;
      case 'b':
	doBsv = true;
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
      case 'w':
	doWrap = true;
	break;
      case 'M':
	depFile = *++ap;
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
      Worker *w = myCalloc(Worker, 1);
      if ((err = parseWorker(*ap, NULL, w)))
	fprintf(stderr, "For file %s: %s\n", *ap, err);
      else if (doDefs && (err = emitDefsHDL(w, root)))
	fprintf(stderr, "%s: Error generating definition/declaration file: %s\n", *ap, err);
      else if (doImpl && (err = (w->model == HdlModel ? emitImplHDL : emitImplRCC)(w, root, library)))
	fprintf(stderr, "%s: Error generating implementation declaration file: %s\n", *ap, err);
      else if (doSkel && (err = (w->model == HdlModel ? emitSkelHDL : emitSkelRCC)(w, root)))
	fprintf(stderr, "%s: Error generating implementation skeleton file: %s\n", *ap, err);
      else if (doWrap && (err = emitDefsHDL(w, root, true)))
	fprintf(stderr, "%s: Error generating wrapper file: %s\n", *ap, err);
      else if (doAssy && (err = emitAssyHDL(w, root)))
	fprintf(stderr, "%s: Error generating assembly: %s\n", *ap, err);
      else if (doBsv && (err = emitBsvHDL(w, root)))
	fprintf(stderr, "%s: Error generating BSV import file: %s\n", *ap, err);
      else if (doArt)
	switch (w->model) {
	case HdlModel:
	  if (!hdlDep) {
	    fprintf(stderr,
		    "%s: No -h file specified when requesting an artifact descriptor", *ap);
	    return 1;
	  } else if ((err = emitArtHDL(w, root, hdlDep)))
	    fprintf(stderr, "%s: Error generating bitstream artifact XML: %s\n",
		    *ap, err);
	  break;
	case RccModel:
	  if ((err = emitArtRCC(w, root)))
	    fprintf(stderr, "%s: Error generating shared library artifact XML: %s\n",
		    *ap, err);
	}
      cleanWIP(w);
    }
  return err ? 1 : 0;
}
