
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
#include "cdkutils.h"
#include "ocpidds.h"

#define OCPI_OPTIONS_HELP \
  "Usage is: ocpidds [options] <input-files>\n"

//          name      abbrev  type    value description
#define OCPI_OPTIONS \
 CMD_OPTION(protocol, p, Bool,   NULL, "Generate the protocol XML file from  DDS IDL files") \
 CMD_OPTION(idl,      d, Bool,   NULL, "Generate the DDS IDL file from an XML protocol file") \
 CMD_OPTION(structname, s, String, NULL,  "IDL struct name for DDS topic for -p. Default is IDL file name") \
 CMD_OPTION(test,       t, String,  0,     "Internal data type test iteration count") \
 CMD_OPTION(output,     O, String, NULL,  "the output directory for generated files") \
 CMD_OPTION_S(define,   D, String, NULL,  "preprocessor definition for IDL") \
 CMD_OPTION_S(undefine, U, String, NULL, "preprocessor undefinition for IDL") \
 CMD_OPTION_S(include,  I, String, NULL, "an include search directory for IDL/XML processing") \
 CMD_OPTION(depend,     M, String, NULL, "file to write makefile dependencies to\n") \

#include "CmdOption.h"
/*
 * Generate things related to DDS.
 * In particular, generate the OpenCPI protocol XML from DDS IDL
 */
static int mymain(const char **argv) {
  const char *err = 0;
  if (options.protocol()) {
    size_t n;
    std::vector<const char *> args;
    for (const char **ap = options.define(n); *ap; ++ap)
      args.push_back(*ap);
    for (const char **ap = options.undefine(n); *ap; ++ap)
      args.push_back(*ap);
    for (const char **ap = options.include(n); *ap; ++ap)
      args.push_back(*ap);
    args.push_back(NULL);
    if ((err = emitProtocol(&args[0], options.output(), *argv, options.structname())))
      fprintf(stderr, "Error generating OpenCPI protocol file from IDL: %s\n", err);
  } else if (options.idl() && (err = emitIDL(options.output(), *argv)))
    fprintf(stderr, "Error generating IDL file from OpenCPI protocol file \"%s\": %s\n", *argv,
	    err);
  else if (options.test())
    dataTypeTest(options.test());
  return err ? 1 : 0;
}

int
main(int /*argc*/, const char **argv) {
  return options.main(argv, mymain);
}
