#!/usr/bin/env python3
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
"""
Main program that processes the show verb for ocpidev
"""

import argparse
import types
import os
import sys
import pydoc
import _opencpi.assets.project as ocpiproj
import _opencpi.util as ocpiutil

NOUNS_NO_LIBS=["project"]

CLASS_DICT = {
    "project": ocpiproj.Project,
}

def parse_cl_vars():
    """
    Construct the argparse object and parse all the command line arguments into a dictionary to
    return
    """
    description = ("TODO") # this shouldnt be used yet?

    parser = argparse.ArgumentParser(description=description,
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     prog="ocpidev create")
    parser.print_help = types.MethodType(lambda self,
                                                    _=None: pydoc.pager("\n" + self.format_help()),
                                             parser)
    parser.print_usage = types.MethodType(lambda self,
                                                 _=None: pydoc.pager("\n" + self.format_usage()),
                                          parser)
        # This displays the error AND help screen when there is a usage error or no arguments provided
    parser.error = types.MethodType(
        lambda self, error_message: (
            pydoc.pager(error_message + "\n\n" + self.format_help()),
            exit(1)
            ), parser)
    parser.add_argument("noun", type=str, nargs='?', help="This is the asset to create.  The " +
                        "only valid options are " + ",".join(NOUNS_NO_LIBS) + ".",
                        choices=NOUNS_NO_LIBS)
    parser.add_argument("name", default=None, type=str, action="store", nargs='?',
                        help="This is the name of the test or application to run.")
    parser.add_argument("-d", dest="cur_dir", default=os.path.curdir,
                        help="Change directory to the specified path before proceeding. " +
                        "Changing directory may have no effect for some commands.")
    parser.add_argument("--register", action="store_true",
                        help="using this option causes the newly created project to be " +
                        "registered into the default registry")
    parser.add_argument("-y", "--comp-lib", default=None, type=str, action="append",
                        help="Specify a component library to search for workers/devices/specs " +
                        "that thisasset references (adds to \"ComponentLibraries\" in Makefile)")
    parser.add_argument("-A", "--xml-include", default=None, type=str, action="append",
                        help="A directory to search for XML files (adds to \"XmlIncludeDirs\" in " +
                        "Makefile)")
    parser.add_argument("-I", "--include-dir", default=None, type=str, action="append",
                        help="A directory to search for language include files (e.g. C/C++ or " +
                        "Verilog) (Adds to \"IncludeDirs\" in Makefile)")
    parser.add_argument("-Y", "--prim-lib", default=None, type=str, action="append",
                        help="Specify a primitive library that this asset depends on" +
                        "(adds to \"Libraries\" in Makefile)")
    parser.add_argument("-D", "--depend", default=None, type=str, action="append",
                        help="Another project that this project depends on (use package-ID, e.g. " +
                       "ocpi.core) (adds to \"ProjectDependencies\" in Project.mk)")
    parser.add_argument("-K", "--package-id", default=None, type=str, action="store",
                        help="Package-ID (default: pkg-prefix.pkg-name. Overrides -N and -F if " +
                        "set)")
    parser.add_argument("-F", "--package-prefix", default=None, type=str, action="store",
                        help="Package prefix (default: 'local' for projects, package-ID of " +
                        "parent otherwise) Defaults to package-ID of parent for libraries.")
    parser.add_argument("-N", "--package-name", default=None, type=str, action="store",
                        help="Package name (default: name arg in 'ocpidev create project <name>' " +
                        "command)")
    parser.add_argument("-v", action="store_true", dest="verbose",
                        help="Be verbose, describing what is happening in more detail")

    cmd_args, args_value = parser.parse_known_args()

    if args_value:
        ocpiutil.logging.error("invalid options were used: " + " ".join(args_value))
        sys.exit(1)

    return vars(cmd_args)

def main():
    """
    Function that is called if this module is called as a main function
    """

    args = parse_cl_vars()
    name = args["name"]
    args.pop("name", None)
    try:
        CLASS_DICT[args["noun"]].create(name, ".", **args)
    except ocpiutil.OCPIException as ex:
        ocpiutil.logging.error(ex)
        ocpiutil.logging.error("Unable to run due to previous errors")
        sys.exit(1)


if __name__ == '__main__':
    main()
