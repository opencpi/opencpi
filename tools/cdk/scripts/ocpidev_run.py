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
import argparse
import os
import sys
import pydoc
import types
import ocpiassets
import ocpiutil

NOUNS = ["test", "tests", "library", "application", "applications", "project"]
MODES = ["gen_build", "prep_run_verify", "clean_all", "gen", "prep", "run", "prep_run",
         "verify", "view", "clean_run", "clean_sim", "all"]

def parse_cl_vars():
    """
    Construct the argparse object and parse all the command line arguments into a dictionary to
    return
    """
    description = ("Utility for running component unit-tests and simple applications \n" +
                  "Component unit-tests have 5 phases: \n" +
                  "    Generate- generate testing artifacts after finding the spec and the " +
                  "workers\n" +
                  "    Build- building HDL bitstream/executable artifacts for testing\n" +
                  "    Prepare-  examine available built workers and available platforms, " +
                  "creating execution scripts to use them all for executing feasible tests.\n" +
                  "    Run-execute tests for all workers, configurations, test cases and " +
                  "platforms\n"
                  "    Verify- verify results from the execution of tests on workers and " +
                  "platforms\n" +
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
                                     formatter_class=argparse.RawTextHelpFormatter)
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
                        "valid options are " + ",".join(NOUNS) + ".", choices=NOUNS)
    parser.add_argument("name", default=None, type=str, action="store", nargs='?',
                        help="This is the name of the test or application to run.")
    parser.add_argument("-v", "--verbose", action="store_true", help="Be verbose with output.")
    parser.add_argument("--keep-simulations", dest="sims", action="store_true",
                        help="Keep HDL simulation files for any simulation platforms.  " +
                        "(files can become large)")
    parser.add_argument("--accumulate-errors", dest="errors", action="store_true",
                        help="When set causes execution or verification errors to accumulate " +
                        "(i.e. not stop the process) and simply be reported as they occurr.")
    parser.add_argument("-G", "--only-platform", dest="only_plats", action="append",
                        help="Specify which platforms to use with a unit test from the " +
                        "list of runtime platforms.")
    parser.add_argument("-Q", "--exclude-platform ", dest="ex_plats", action="append",
                        help="Specify which platforms not to use with a unit test from the " +
                        "list of runtime platforms.")
    parser.add_argument("--rcc-platform", dest="rcc_plats", action="append",
                        help="Specify which RCC platform from the list of buildable " +
                        "platforms to use with unit test.  Only valid in generate " +
                        "and build phases.  For application specifies which RCC " +
                        "platform to build ACI applications.")
    parser.add_argument("--hdl-platform", dest="hdl_plats", action="append",
                        help="Specify which HDL platform from the list of buildable " +
                        "platforms to use with unit test. only valid in generate " +
                        "and build phases.  Not valid for Application")
    parser.add_argument("-d", dest="cur_dir", default=os.path.curdir,
                        help="Change directory to the specified path before proceeding. " +
                        "Changing directory may have no effect for some commands.")
    parser.add_argument("-l", dest="lib", default="components",
                        help="Specify the component library for the test to be run.  " +
                        "Not valid for Application.")
    parser.add_argument("--case", dest="cases", action="append",
                        help="Specify Which test case that will be run/verified.  " +
                        "Not valid for Application.")
    parser.add_argument("--before", dest="run_before", action="append",
                        help="Arguments to insert before the ACI executable or ocpirun, such as " +
                        "environment settings or prefix commands.  Not valid for Test.")
    parser.add_argument("--after", dest="run_after", action="append",
                        help="Arguments to insert at the end of the execution command line " +
                        "Not valid for Test.")
    parser.add_argument("--run-args", dest="run_args", action="append",
                        help="Arguments to insert immediately after the ACI executable or " +
                        "ocpirun.  Not valid for Test.")
    parser.add_argument("--mode", dest="mode", default="all", choices=MODES,
                        help="Specify which phases of the unit test to run valid options are:  " +
                        ",".join(MODES) + ".  Not valid for Application.")
    parser.add_argument("--remotes", dest="remote_test_sys", action="append",
                        help="Specify remote systems to run the test(s).  see OpenCPI Component " +
                        "Development Guide (section 13.8) for more information on the "
                        "OCPI_REMOTE_TEST_SYSTEMS  variable.  Not valid for Application.")

    cmd_args, args_value = parser.parse_known_args()

    if args_value:
        ocpiutil.logging.warning("invalid options were ignored: " + " ".join(args_value))

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
    elif args['noun'] in ["test", "library"] and name and name != os.path.basename(os.path.realpath('.')):
        ret_val = os.path.realpath('.') + "/" + lib + "/" + name
        if args['noun'] == "test" and not ret_val.endswith(".test"):
            ret_val = ret_val + ".test"
    elif args['noun'] in ["test", "library"]:
        ret_val = os.path.realpath('.')
    elif args['noun'] == "application" and dir_type != "application":
        ret_val = os.path.realpath('.') + "/applications/" + name
    return ret_val

def set_init_values(args, dir_type):
    """
    Determine which contents of library and project objects need to be initialized and set the
    corresponding kwargs values
    """
    if args['noun'] == "library":
        args['init_tests'] = True
    if args['noun'] == "project":
        args['init_libs'] = True
        args['init_tests'] = True
        args['init_apps'] = True
    if args['noun'] == "tests":
        args['init_libs'] = True
        args['init_tests'] = True
        #TODO add libraries here and libraries collection to allow call in a libraries directory
        if dir_type in ["library", "project"]:
            args['noun'] = dir_type
        else:
            raise ocpiutil.OCPIException("Use of tests noun in an invalid directory type: \"" +
                                         dir_type + "\". Valid types are library and project")

def main():
    ocpiutil.configure_logging()
    args = parse_cl_vars()
    try:
        with ocpiutil.cd(args['cur_dir']):
            dir_type = ocpiutil.get_dirtype()
            # args['name'] could be None if no name is provided at the command line
            name = args['name']
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
            set_init_values(args, dir_type)
            lib = args.get("lib", "")
            if args['noun'] in ["test", "library"]:
                if dir_type == "library":
                    lib = ""
                if (lib != "components") and ('/' not in lib) and (dir_type == "project"):
                    lib = "components/" + lib
                # pylint:disable=bad-continuation
                if (dir_type == "libraries" and
                    os.path.basename(os.path.realpath('.')) == "components"):
                    #set lib to a blank string
                    lib = ""
                # pylint:enable=bad-continuation
            directory = get_directory(args, name, lib)
            ocpiutil.logging.debug("creating asset as the following \nname: " + str(name) + "\n" +
                                   "directory: " + str(directory) + "\nargs: " + str(args))
            my_asset = ocpiassets.AssetFactory.factory(args['noun'], directory, name, **args)
            #TODO make sure return value is working right always returning zero ?

            sys.exit(my_asset.run())
    except ocpiutil.OCPIException as ex:
        ocpiutil.logging.error(str(ex))
        sys.exit(1)

if __name__ == '__main__':
    main()
