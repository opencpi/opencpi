
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
#include "ocpidds.h"
/*
 * Generate things related to DDS.
 * In particular, generate the OpenCPI protocol XML from DDS IDL
 */
int
main(int argc, char **argv) {
  const char *outDir = 0, *structName = 0;
  unsigned doTest = 0;
  bool
    doProto = false, doIDL = false;
  if (argc <= 1) {
    fprintf(stderr,
	    "Usage is: ocpidds [options] <input-files> \n"
	    " Code generation options that determine which files are created and used:\n"
	    " -p            Generate the protocol XML file from  DDS IDL files.\n"
	    " -d            Generate the DDS IDL file from an XML protocol file.\n"
	    " -s            IDL struct name for DDS topic for -p option. Default is IDL file name\n"
	    " -t <count>    Internal test\n"
	    " Other options:\n"
	    " -O <dir>      Specify the output directory for generated files\n"
	    " -DSYM         Specify preprocessor definition for IDL\n"
	    " -USYM         Specify preprocessor undefinition for IDL\n"
	    " -I <dir>      Specify an include search directory for IDL/XML processing\n"
	    " -M <file>     Specify the file to write makefile dependencies to\n"
	    );
    return 1;
  }
  const char *err = 0;
  for (char **ap = argv+1; *ap; ap++)
    if (ap[0][0] == '-')
      switch (ap[0][1]) {
      case 'd':
	doIDL = true;
	break;
      case 'p':
	doProto = true;
	break;
      case 's':
	structName = *++ap;
	break;
      case 't':
	doTest = atoi(*++ap);
	break;
      case 'M':
	depFile = *++ap;
	break;
      case 'I':
	addInclude(*ap);
	if (!ap[0][2])
	  addInclude(*++ap);
	break;
      case 'D':
      case 'U':
	addInclude(*ap);
	break;
      case 'O':
	outDir = *++ap;
	break;
      default:
	fprintf(stderr, "Unknown flag: %s\n", *ap);
	return 1;
      }
    else {
      if (doProto) {
	if ((err = emitProtocol(outDir, *ap, structName)))
	  fprintf(stderr, "Error generating OpenCPI protocol file from IDL: %s\n", err);
      } else if (doIDL && (err = emitIDL(outDir, *ap)))
	fprintf(stderr, "Error generating IDL file from OpenCPI protocol file \"%s\": %s\n", *ap, err);
    }
  if (doTest)
    dataTypeTest(doTest);
  return err ? 1 : 0;
}
