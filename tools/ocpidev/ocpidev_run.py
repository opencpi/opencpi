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
Main program that processes the run verb for ocpidev
"""
import argparse
import os
import sys
import pydoc
import types
import _opencpi.assets.factory as ocpifactory
import _opencpi.util as ocpiutil

NOUNS = ["test", "tests", "library", "application", "applications", "project", "libraries"]
NOUNS_NO_LIBS = ["test", "tests", "library", "application", "applications", "project"]
MODES = ["all", "gen", "gen_build", "prep_run_verify", "prep", "run", "prep_run", "verify", "view",
         "clean_all", "clean_run", "clean_sim"]

def parse_cl_vars():
    """
    Construct the argparse object and parse all the command line arguments into a dictionary to
    return
    """
    description = ("Utility for running component unit-tests and simple applications \n" +
                   "Component unit-tests have 5 phases: \n" +
                   "    Generate: generate testing artifacts after finding the spec and the \n" +
                   "              workers\n" +
                   "    Build:    building HDL bitstream/executable artifacts for testing\n" +
                   "    Prepare:  examine available built workers and available platforms, \n" +
                   "              creating execution scripts to use them all for executing \n" +
                   "              feasible tests.\n" +
                   "    Run:      execute tests for all workers, configurations, test cases \n" +
                   "              and platforms\n"
                   "    Verify:   verify results from the execution of tests on workers and \n" +
                   "              platforms\n \n" +
                   "Usage Examples: \n" +
                   "run a single application: \n" +
                   "    ocpidev run application <app_name> \n"
                   "run all applications in a project: \n" +
                   "    ocpidev run applications \n"
                   "run a single test: \n" +
                   "    ocpidev run -l <library_dir> test <test_name> \n"
                   "run all tests in a library: \n" +
                   "    ocpidev run library <library_name> \n"
                   "run generate and build stages of a single test: \n" +
                   "    ocpidev run -l <library_dir>  --mode gen_build test <test_name>\n")

    parser = argparse.ArgumentParser(description=description,
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     prog="ocpidev run")
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
    parser.add_argument("noun", type=str, nargs='?', help="This is the noun to run.  The only " +
                        "valid options are " + ",".join(NOUNS_NO_LIBS) + ".", choices=NOUNS_NO_LIBS)
    parser.add_argument("name", default=None, type=str, action="store", nargs='?',
                        help="This is the name of the test or application to run.")
    parser.add_argument("-v", "--verbose", action="store_true", help="Be verbose with output.")
    parser.add_argument("--keep-simulations", dest="keep_sims", action="store_true",
                        help="Keep HDL simulation files regardless of verification results.  " +
                        "By default, simulation files are removed if the verification is " +
                        "successful.  Warning: Simulation files can become large!")
    parser.add_argument("--accumulate-errors", dest="acc_errors", action="store_true",
                        help="When enabled, execution or verification errors are accumulated " +
                        "(i.e. not stop the process) and simply be reported as they occur, " +
                        "rather than ending the test upon first failure detected.")
    parser.add_argument("--view", dest="view", action="store_true",
                        help="When set the view script (view.sh) for this test is run at the " +
                        "conclusion of the test's execution.  Not valid for Application")
    parser.add_argument("-G", "--only-platform", metavar="ONLY_PLAT", dest="only_plats",
                        action="append", help="Specify which platforms to use with a unit test " +
                        "from the list of runtime platforms.")
    parser.add_argument("-Q", "--exclude-platform ", metavar="EXCLUDE_PLAT", dest="ex_plats",
                        action="append", help="Specify which platforms not to use with a unit " +
                        "test from the list of runtime platforms.")
    parser.add_argument("--rcc-platform", metavar="RCC_PLAT", dest="rcc_plats", action="append",
                        help="Specify which RCC platform from the list of buildable " +
                        "platforms to use with unit test.  Only valid in generate " +
                        "and build phases.  For application specifies which RCC " +
                        "platform to build ACI applications.")
    parser.add_argument("--hdl-platform", metavar="HDL_PLAT", dest="hdl_plats", action="append",
                        help="Specify which HDL platform from the list of buildable " +
                        "platforms to use with unit test. only valid in generate " +
                        "and build phases.  Not valid for Application")
    parser.add_argument("-d", dest="cur_dir", default=os.path.curdir,
                        help="Change directory to the specified path before proceeding. " +
                        "Changing directory may have no effect for some commands.")
    parser.add_argument("-l", "--library", dest="library", default=None,
                        help="Specify the component library in which this operation will be " +
                        "performed.")
    parser.add_argument("--hdl-library", dest="hdl_library", default=None,
                        help="Specify the hdl library in which this operation will be " +
                        "performed.")
    parser.add_argument("-P", dest="hdl_plat_dir", default=None,
                        help="Specify the hdl platform subdirectory to operate in.")
    parser.add_argument("--case", metavar="CASE", dest="cases", action="append",
                        help="Specify which test case(s) that will be run/verified.  Wildcards " +
                        "are valid, ex. case*.0, case0.0*, case00.01.  Not valid for Application.")
    parser.add_argument("--before", dest="run_before", action="append",
                        help="Argument(s) to insert before the ACI executable or ocpirun, such " +
                        "as environment settings or prefix commands.  Not valid for Test.")
    parser.add_argument("--after", dest="run_after", action="append",
                        help="Argument(s) to insert at the end of the execution command line " +
                        "Not valid for Test.")
    parser.add_argument("--run-arg", dest="run_arg", action="append",
                        help="Argument(s) to insert immediately after the ACI executable or " +
                        "ocpirun.  Not valid for Test.")
    parser.add_argument("--mode", dest="mode", default="all", choices=MODES,
                        help="Specify which phase(s) of the unit test to execute.  Not valid " +
                        "for Application.")
    parser.add_argument("--remotes", dest="remote_test_sys", action="append",
                        help="Specify remote systems to run the test(s) using the format " +
                        "<IP address=u/n=p/w=NFS mount directory>.  see OpenCPI Component " +
                        "Development Guide (section 13.8) for more information on the "
                        "OCPI_REMOTE_TEST_SYSTEMS  variable.  Not valid for Application.")

    cmd_args, args_value = parser.parse_known_args()

    if args_value:
        ocpiutil.logging.error("invalid options were used: " + " ".join(args_value))
        sys.exit(1)

    return vars(cmd_args)

def get_directory(args, name, lib):
    """
    Determine the correct directory for the asset that should be run given the cmd line arguments
    passed in
    """
    if lib == name:
        lib = ""
    dir_type = ocpiutil.get_dirtype()
    ret_val = os.path.realpath('.')
    if args['noun'] == "applications":
        ret_val = ocpiutil.get_path_to_project_top() + "/applications"
    elif (args['noun'] in ["test", "library"] and name and
          name != os.path.basename(os.path.realpath('.'))):
        ret_val = os.path.realpath('.') + "/" + lib + "/" + name
        if args['noun'] == "test" and not ret_val.endswith(".test"):
            ret_val = ret_val + ".test"
    elif args['noun'] in ["test", "library"]:
        ret_val = os.path.realpath('.')
    elif args['noun'] == "application":
        if dir_type == "project" and name != os.path.basename(os.path.realpath('.')):
            ret_val = os.path.realpath('.') + "/applications/" + name
        elif dir_type == "applications" and name != os.path.basename(os.path.realpath('.')):
            ret_val = os.path.realpath('.') + "/" + name
    return ret_val

def set_init_values(args, dir_type):
    """
    Determine which contents of library and project objects need to be initialized and set the
    corresponding kwargs values
    """
    if args['noun'] in ["library", "libraries"]:
        args['init_tests'] = True
    if args['noun'] == "libraries":
        args['init_libs_col'] = True
    if args['noun'] == "project":
        args['init_libs'] = True
        args['init_tests'] = True
        args['init_apps_col'] = True
        args['init_apps'] = True
    if args['noun'] == "tests":
        args['init_libs'] = True
        args['init_tests'] = True
        args['init_libs_col'] = True
        if dir_type in ["library", "project", "libraries"]:
            args['noun'] = dir_type
        else:
            raise ocpiutil.OCPIException("Use of tests noun in an invalid directory type: \"" +
                                         dir_type + "\". Valid types are library and project")

def main():
    """
    Function that is called if this module is called as a mian function
    """
    args = parse_cl_vars()
    directory = None
    name = None
    try:
        cur_dir = args['cur_dir']
        with ocpiutil.cd(cur_dir):
            dir_type = ocpiutil.get_dirtype()
            # args['name'] could be None if no name is provided at the command line
            name = args['name']
            directory = ocpiutil.get_ocpidev_working_dir(noun=args.get("noun", ""),
                                                         name=name,
                                                         library=args['library'],
                                                         hdl_library=args['hdl_library'],
                                                         hdl_platform=args['hdl_plat_dir'])
            if (name is None) and (dir_type in [n for n in NOUNS if n != "tests"]):
                name = os.path.basename(os.path.realpath('.'))
            del args['name']
            if args['noun'] is None:
                if dir_type in [n for n in NOUNS if n != "tests"]:
                    args['noun'] = dir_type
                else:
                    raise ocpiutil.OCPIException("Invalid directory type \"" + str(dir_type) +
                                                 "\" Valid directory types are: " +
                                                 ", ".join([n for n in NOUNS if n != "tests"]))
            if ocpiutil.get_dirtype(directory) == "libraries" and args['noun'] == "library":
                args['noun'] = "libraries"
            set_init_values(args, dir_type)
            ocpiutil.logging.debug("creating asset as the following \nnoun: " + args['noun'] +
                                   "\nname: " + str(name) + "\n" + "directory: " + str(directory) +
                                   "\nargs: " + str(args))
            my_asset = ocpifactory.AssetFactory.factory(args['noun'], directory,
                                                        name, **args)
            sys.exit(my_asset.run())
    except ocpiutil.OCPIException as ex:
        ocpiutil.logging.error(ex)
        if args['noun'] is not None:
            my_noun = '' if args['noun'] is None else " \"" + args['noun'] + "\""
            my_name = '' if name is None else " named \"" + name + "\""
            my_dir = '' if directory is None else " in directory \"" + directory + "\""
            run_dbg = "Unable to run" + my_noun +  my_name + my_dir + " due to previous errors"
        else:
            run_dbg = "Unable to run due to previous errors"
        ocpiutil.logging.error(run_dbg)
        sys.exit(1)

if __name__ == '__main__':
    main()
