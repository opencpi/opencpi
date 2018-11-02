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
This module a collection of OpenCPI utility functions.

Some of the utilities found here include string/number manipulation
and parsing functions as well as path calculation/manipulation functions.
There are also utilities for collecting variable values from GNU make.

Documentation and testing:
    Docstring tests can be executed as follows:
        $ python3 ocpiutil.py -v
    Documentation can be viewed by running:
        $ pydoc3 ocpiutil
Note on testing:
    When adding functions to this file, add unit tests to
    opencpi/tests/pytests/ocpiutil_test.py
"""
import sys
import os
import os.path
import re
import subprocess
from glob import glob
import logging
from contextlib import contextmanager
from functools import reduce
import operator
import datetime

# Use python3's input name
# FIXME: should not have to do this now that we are in python3
try:
    input = raw_input
except NameError:
    pass

class OCPIException(Exception):
    """
    A exception class that we can throw and catch within OpenCPI code while ignoring other exception
    types. This class inherits from the built-in Exception class and doesn't extend it in any way
    """
    pass

def configure_logging(level=None, output_fd=sys.stderr):
    """
    Initialize the root logging module such that:
            OCPI_LOG_LEVEL <  8 : only log WARNINGS, ERRORS and CRITICALs
       8 <= OCPI_LOG_LEVEL <  10: also log INFOs
            OCPI_LOG_LEVEL >= 10: also log DEBUGs
    This can be used in other modules to change the default log level of the
    logging module. For example, from another module:
    >>> import logging
    >>> import ocpiutil
    >>> os.environ['OCPI_LOG_LEVEL'] = "11"
    >>> ocpiutil.configure_logging(output_fd=sys.stdout)
    <logging.RootLogger...>

    # Now, logging will be determined based on OCPI_LOG_LEVEL environment
    # variable. So, the following will only print if OCPI_LOG_LEVEL is >= 8
    >>> logging.info("info message")

    # You should see 'OCPI:INFO: info message', but it will print to stderr, not stdout
    """
    logging.basicConfig(stream=output_fd, format='OCPI:%(levelname)s: %(message)s')
    rootlogger = logging.getLogger()
    if level:
        rootlogger.setLevel(level=level)
    else:
        log_level = os.environ.get('OCPI_LOG_LEVEL')
        if not log_level or int(log_level) < 8:
            rootlogger.setLevel(level=logging.WARNING)
        elif int(log_level) < 10:
            rootlogger.setLevel(level=logging.INFO)
        else:
            rootlogger.setLevel(level=logging.DEBUG)
    return rootlogger

###############################################################################
# Utility functions for extracting variables and information from and calling
# Makefiles
###############################################################################

def execute_cmd(settings, directory, action=None):
    """
    This command is a wrapper around any calls to make in order to encapsulate the use of make to a
    minimal number of places.  The function contains a hard-coded dictionary of generic settings to
    make variables that it uses to construct the call to make at that given directory
    """
    settings_dict = {'rcc_plat'        : "RccPlatforms",
                     'hdl_plats'       : "HdlPlatforms",
                     'hdl_tgts'        : "HdlTargets",
                     'only_plat'       : "OnlyPlatforms",
                     'ex_plat'         : "ExcludePlatforms",
                     'keep_sims'       : "KeepSimulations",
                     'acc_errors'      : "TestAccumulateErrors",
                     'view'            : "View",
                     'case'            : "Cases",
                     'run_before'      : "OcpiRunBefore",
                     'run_after'       : "OcpiRunAfter",
                     'run_arg'         : "OcpiRunArgs",
                     'remote_test_sys' : "OCPI_REMOTE_TEST_SYSTEMS",
                     'verbose'         : "TestVerbose"}
    make_list = []

    make_list.append("make")
    make_list.append("-C")
    make_list.append(directory)
    debug_string = "make -C " + directory

    if action is not None:
        make_list.extend(action)
        debug_string += " " + ' '.join(action) + " "

    for setting, value in settings.items():
        if isinstance(value, bool):
            make_list.append(settings_dict[setting] + '=1')
            debug_string += settings_dict[setting] + '=1'
        elif isinstance(value, list) or isinstance(value, set):
            make_list.append(settings_dict[setting] + '='  + ' '.join(value))
            if len(value) > 1:
                debug_string += settings_dict[setting] + '="'  + ' '.join(value) + '"'
            else:
                debug_string += settings_dict[setting] + '='  + ' '.join(value)
        else:
            raise OCPIException("Invalid setting data-type passed to execute_cmd().  Valid data-" +
                                "types are bool and list")

    logging.debug("running make command: " + debug_string)
    #shell=True is bad dont set it here running the following command was able to execute
    # arbitary code
    #ocpidev run test --hdl-platform \$\(./script.temp\)
    # all the script would need to do is cat isim then go on its merry way doing whatever it wanted
    child = subprocess.Popen(make_list)
    child.wait()
    return child.returncode

def set_vars_from_make(mk_file, mk_arg="", verbose=None):
    """
    Collect a dictionary of variables from a makefile
    --------------------------------------------------
    First arg is .mk file to use
    Second arg is make arguments needed to invoke correct output
        The output can be an assignment or a target
    Third arg is a verbosity flag
    Return a dictionary of variable names mapped to values from make

    OCPI_LOG_LEVEL>=6  will print stderr from make for user to see
    OCPI_LOG_LEVEL>=10 will pass OCPI_DEBUG_MAKE to make command and will
                       print both stdout and stderr for user to see
    """
    with open(os.devnull, 'w') as fnull:
        make_exists = subprocess.Popen(["which", "make"],\
                      stdout=subprocess.PIPE, stderr=fnull).communicate()[0]
        if make_exists is None or make_exists == "":
            if verbose != None and verbose != "":
                logging.error("The '\"make\"' command is not available.")
            return 1

        # If log level >= 10 set OCPI_DEBUG_MAKE=1 (max debug level)
        ocpi_log_level = int(os.environ.get('OCPI_LOG_LEVEL', 0))
        if ocpi_log_level >= 10:
            mk_dbg = "OCPI_DEBUG_MAKE=1"
        else:
            mk_dbg = ""

        # If mk_file is a "Makefile" then we use the -C option on the directory containing
        # the makefile else (is a .mk) use the -f option on the file
        if mk_file.endswith("/Makefile"):
            make_cmd = "make " + mk_dbg + " -n -r -s -C " + os.path.dirname(mk_file) + " " + mk_arg
        else:
            make_cmd = "make " + mk_dbg + " -n -r -s -f " + mk_file + " " + mk_arg

        logging.debug("Calling make via:" + str(make_cmd.split()))

        child = subprocess.Popen(make_cmd.split(), stderr=subprocess.PIPE, stdout=subprocess.PIPE,
                                 universal_newlines=True)
        mk_output, mk_err = child.communicate()

        # Print out output from make if log level is high
        logging.debug("STDOUT output from Make (set_vars_from_make):\n" + str(mk_output))

        # Print out stderr from make if log level is medium/high or if make returned error
        if child.returncode != 0 or ocpi_log_level >= 6:
            if mk_err:
                logging.error("STDERR output from Make (set_vars_from_make):\n" + str(mk_err))
            if child.returncode != 0:
                raise OCPIException("The following make command returned an error:\n" + make_cmd)

        try:
            grep_str = re.search(r'(^|\n)[a-zA-Z_][a-zA-Z_]*=.*',
                                 str(mk_output.strip())).group()
        except AttributeError:
            logging.warning("No variables are set from \"" + mk_file + "\"")
            return None

        assignment_strs = [x.strip() for x in grep_str.split(';') if len(x.strip()) > 0]
        make_vars = {}
        for var_assignment in assignment_strs:
            var_name, var_val = var_assignment.split('=')
            # If the value is an empty string or just matching quotes, assign [] as the value
            if var_val == "\"\"" or var_val == "\'\'" or var_val == "":
                assignment_value = []
            else:
                assignment_value = var_val.strip('"').strip().split(' ')
            make_vars[var_name] = assignment_value
        return make_vars

def get_make_vars_rcc_targets():
    """
    Get make variables from rcc-targets.mk
    Dictionary key examples are:
        RccAllPlatforms, RccPlatforms, RccAllTargets, RccTargets
    """
    return set_vars_from_make(os.environ["OCPI_CDK_DIR"] +
                              "/include/rcc/rcc-targets.mk",
                              "ShellRccTargetsVars=1", "verbose")

###############################################################################
# Utility functions for collecting information about the directories
# in a project
###############################################################################

def get_dirtype(directory="."):
    """
    Determine a directory's type by parsing it for the last 'include ... *.mk' line
    """
    match = None
    if os.path.isfile(directory + "/Makefile"):
        with open(directory + "/Makefile") as mk_file:
            for line in mk_file:
                result = re.match(r"^\s*include\s*.*OCPI_CDK_DIR.*/include/(hdl/)?(.*)\.mk.*", line)
                match = result.group(2) if result != None else match
    if match is None:
        if os.path.isfile(directory + "/project-package-id"):
            return "project"
        return None
    return match

def get_subdirs_of_type(dirtype, directory="."):
    """
    Return a list of directories underneath the given directory
    that have a certain type (library, worker, hdl-assembly...)
    """
    subdir_list = []
    if dirtype:
        for subdir, _, _ in os.walk(directory):
            if get_dirtype(subdir) == dirtype:
                subdir_list.append(subdir)
    return subdir_list

###############################################################################
# Utility function for exporting libraries in a project
###############################################################################
def export_libraries():
    """
    Build the lib directory and links to specs in each library in a project.
    This will allow specs to be exported before workers in a library are built.
    """
    for lib_dir in get_subdirs_of_type("library"):
        logging.debug("Library found at \"" + lib_dir + "\", running \"make speclinks\" there.")
        proc = subprocess.Popen(["make", "-C", lib_dir, "speclinks"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        my_out = proc.communicate()
        if proc.returncode != 0:
            logging.warning("Failed to export library at " + lib_dir + " because of error : \n" +
                            str(my_out[1]))

###############################################################################
# Utility functions for determining paths to/from the top level of a project
# or a project's imports directory.
###############################################################################

def is_path_in_project(origin_path=".",):
    """
    Determine whether a path is inside a project
    """
    if os.path.exists(origin_path):
        abs_path = os.path.abspath(origin_path)
        if get_dirtype(origin_path) == "project":
            return True
        elif abs_path != "/":
            return is_path_in_project(os.path.dirname(abs_path))
    return False


# Get the path to the project containing 'origin_path'
# relative_mode : default=False
# If relative_mode is False,
#     an absolute path to the containing project is returned
# If relative_mode is True,
#     a relative path from origin_path is returned
# accum_path is an internal argument that accumulates the path to
#    return across recursive calls
def __get_path_to_project_top(origin_path, relative_mode, accum_path):
    if origin_path and os.path.exists(origin_path):
        abs_path = os.path.abspath(origin_path)
        if get_dirtype(origin_path) == "project":
            return abs_path if relative_mode is False else accum_path
        elif abs_path != "/":
            return __get_path_to_project_top(os.path.dirname(abs_path),
                                             relative_mode, accum_path + "/..")
    return None
def get_path_to_project_top(origin_path=".", relative_mode=False):
    """
    Get the path to the top of the project containing 'origin_path'.
    Optionally enable relative_mode for relative paths.
    Note: call aux function to set accum_path internal arg
    """
    path_to_top = __get_path_to_project_top(origin_path, relative_mode, ".")
    if path_to_top is None:
        if origin_path is None:
            logging.debug("Cannot get path to project for origin_path=None.")
        else:
            logging.debug("Path \"" + os.path.realpath(origin_path) + "\" is not in a project")
    return path_to_top

# Go to the project top and check if the project-package-id file is present.
def is_path_in_exported_project(origin_path):
    """
    Given a path, determine whether it is inside an exported project.
    """
    project_top = get_path_to_project_top(origin_path)
    if project_top is not None:
        if os.path.isfile(project_top + "/project-package-id"):
            logging.debug("Path \"" + os.path.realpath(origin_path) +
                          "\" is in an exported project.")
            return True
    return False


# Get the path from 'to_path's project top to 'to_path'.
# accum_path is an internal argument that accumulates the path to
#    return across recursive calls
def __get_path_from_project_top(to_path, accum_path):
    if os.path.exists(to_path):
        abs_path = os.path.abspath(to_path)
        if get_dirtype(to_path) == "project":
            return accum_path
        elif abs_path != "/":
            appended_accum = os.path.basename(abs_path)
            if accum_path != "":
                appended_accum = appended_accum + "/" + accum_path
            return __get_path_from_project_top(os.path.dirname(abs_path), appended_accum)
    return None
def get_path_from_project_top(to_path="."):
    """
    Get the path to a location from the top of the project containing it.
    The returned path STARTS AT THE PROJECT TOP and therefore does not include
    the path TO the project top.
    Note: call aux function to set accum_path internal arg
    """
    path_from_top = __get_path_from_project_top(to_path, "")
    if path_from_top is None:
        if to_path is None:
            logging.debug("Cannot get path from project for to_path=None.")
        else:
            logging.debug("Path \"" + os.path.realpath(to_path) + "\" is not in a project")
    return path_from_top

def get_project_imports(origin_path="."):
    """
    Get the contents of a project's imports directory.
    The current project is determined based on 'origin_path'
    """
    project_top = get_path_to_project_top(origin_path, False)
    return os.listdir(project_top + "/imports") if project_top is not None else None

# NOTE: This function is not thoroughly tested
def get_path_from_given_project_top(origin_top, dest_path, path_to_origin_top=""):
    """
    Determine the path from the top level of a project (origin_top) to the
    destination path (dest_path). Whenever possible, try to stay internal to
    the project or go through the project's imports directory. If that is not
    possible, return destination path as handed to this function.

    Optionally, a path TO the origin-path's top directory can be provided
    and prepended to the return value whenever possible (when the function
    determined a path inside the project or its imports.
    """
    dest_top = get_path_to_project_top(dest_path, False)
    if dest_top:
        path_from_dest_top = get_path_from_project_top(dest_path)
        prepend_path = path_to_origin_top + "/" if path_to_origin_top != "" else ""
        if os.path.samefile(origin_top, dest_top):
            return prepend_path + path_from_dest_top
        else:
            to_import = "imports/" + os.path.basename(dest_top)
            if os.path.isdir(origin_top + "/" + to_import):
                return prepend_path + to_import + "/" + path_from_dest_top
    return dest_path

# NOTE: This function is not thoroughly tested
def get_paths_from_project_top(origin_path, dest_paths):
    """
    Given an origin and a list of destination paths, return a list of paths
    from origin's project top to each destination (potentially through imports).
    If a destination path is not in the current or an imported project,
    return an absolute path.
    """
    origin_top = get_path_to_project_top(origin_path, False)
    if origin_top is None:
        raise OCPIException("origin_path \"" + str(origin_path) + "\" is not in a project")
    paths_from_top = []
    for dest_p in dest_paths:
        paths_from_top.append(get_path_from_given_project_top(origin_top, dest_p))
    return paths_from_top

# NOTE: This function does is not thoroughly tested
def get_paths_through_project_top(origin_path, dest_paths):
    """
    origin to each destination going through the project top (and potentially
    imports) when possible.
    """
    origin_top = get_path_to_project_top(origin_path, False)
    origin_top_rel = get_path_to_project_top(origin_path, True)
    if origin_top is None:
        raise OCPIException("origin_path \"" + str(origin_path) + "\" is not in a project")
    paths_through_top = []
    for dest_p in dest_paths:
        paths_through_top.append(
            get_path_from_given_project_top(origin_top, dest_p, origin_top_rel))
    return paths_through_top

###############################################################################
# Functions for determining project package information
###############################################################################
def get_project_package(origin_path="."):
    """
    Get the Package Name of the project containing 'origin_path'.
    """
    path_to_project = get_path_to_project_top(origin_path)
    if path_to_project is None:
        logging.debug("Path \"" + str(origin_path) + "\" is not inside a project")
        return None

    # From the project top, probe the Makefile for the projectpackage
    # which is printed in cdk/include/project.mk in the projectpackage rule
    # if ShellProjectVars is defined
    with cd(path_to_project):
        project_package = None
        # If the project-package-id file exists, set package-id to its contents
        if os.path.isfile(path_to_project + "/project-package-id"):
            with open(path_to_project + "/project-package-id", "r") as package_id_file:
                project_package = package_id_file.read().strip()
                logging.debug("Read Project-ID '" + project_package + "' from file: " +
                              path_to_project + "/project-package-id")

        # Otherwise, ask Makefile at the project top for the ProjectPackage
        if project_package is None or project_package == "":
            project_vars = set_vars_from_make("Makefile",
                                              "projectpackage ShellProjectVars=1", "verbose")
            if not project_vars is None and 'ProjectPackage' in project_vars and \
               len(project_vars['ProjectPackage']) > 0:
                # There is only one value associated with ProjectPackage, so get element 0
                project_package = project_vars['ProjectPackage'][0]
            else:
                logging.error("Could not determine Package-ID of project.")
                return None
    return project_package

def does_project_with_package_exist(origin_path=".", package=None):
    """
    Determine if a project with the given package exists and is registered. If origin_path is not
    specified, assume we are interested in the current project. If no package is given, determine
    the current project's package.
    """
    project_registry_dir_exists, project_registry_dir = get_project_registry_dir()
    if not project_registry_dir_exists:
        logging.debug("Registry does not exist, so project with any package cannot be found.")
        return False
    if package is None:
        package = get_project_package(origin_path)
        if package is None:
            logging.debug("No package was provided to the does_project_with_package_exist " +
                          "function, and the path provided does not have a package.")
            return False
    for project in glob(project_registry_dir + "/*"):
        if get_project_package(project) == package or os.path.basename(project) == package:
            return True
    return False

def is_path_in_registry(origin_path="."):
    """
    Is the path provided one of the projects in the registry?
    """
    project_registry_dir_exists, project_registry_dir = get_project_registry_dir()
    if not project_registry_dir_exists:
        # If registry does not exist, origin path cannot be a project in it
        return False
    origin_realpath = os.path.realpath(origin_path)
    # For each project in the registry, check equivalence to origin_path
    for project in glob(project_registry_dir + "/*"):
        if origin_realpath == os.path.realpath(project):
            # A project was found that matches origin path!
            return True
    # No matching project found. Project/path is not in registry
    return False

###############################################################################
# Functions for and accessing/modifying the project registry and collecting
# existing projects
###############################################################################

# Could not think of better/shorted function name, so we disable the pylint checker
# that was erroring due to name length
# pylint:disable=invalid-name
def get_default_project_registry_dir():
    """
    Get the default registry from the environment setup. Check in the following order:
    OCPI_PROJECT_REGISTRY_DIR, OCPI_CDK_DIR/../project-registry or /opt/opencpi/project-registry
    """
    project_registry_dir = os.environ.get('OCPI_PROJECT_REGISTRY_DIR')
    if project_registry_dir is None:
        cdkdir = os.environ.get('OCPI_CDK_DIR')
        if cdkdir:
            project_registry_dir = cdkdir + "/../project-registry"
        else:
            project_registry_dir = "/opt/opencpi/project-registry"
    return project_registry_dir
# pylint:enable=invalid-name

def get_project_registry_dir(directory="."):
    """
    Determine the project registry directory. If in a project, check for the imports link.
    Otherwise, get the default registry from the environment setup:
        OCPI_PROJECT_REGISTRY_DIR, OCPI_CDK_DIR/../project-registry or /opt/opencpi/project-registry

    Determine whether the resulting path exists.

    Return the exists boolean and the path to the project registry directory.
    """
    if is_path_in_project(directory) and \
       os.path.isdir(get_path_to_project_top(directory) + "/imports"):
        # allow imports to be a link OR a directory (needed for deep copies of exported projects)
        project_registry_dir = os.path.realpath(get_path_to_project_top(directory) + "/imports")
    else:
        project_registry_dir = get_default_project_registry_dir()

    exists = os.path.exists(project_registry_dir)
    if not exists:
        logging.warning("The project registry directory '" + project_registry_dir +
                        "' does not exist.\nCorrect " + "'OCPI_PROJECT_REGISTRY_DIR' or run: " +
                        "'ocpidev create registry " + project_registry_dir + "'")
    elif not os.path.isdir(project_registry_dir):
        raise OSError("The current project registry '" + project_registry_dir +
                      "' exists but is not a directory.\nCorrect " +
                      "'OCPI_PROJECT_REGISTRY_DIR'")
    return exists, project_registry_dir

def get_all_projects():
    """
    Iterate through the project path and project registry.
    If the registry does not exist, manually locate the CDK.
    Return the list of all projects.
    """
    projects = []
    project_path = os.environ.get('OCPI_PROJECT_PATH')
    if project_path:
        projects += project_path.split(':')
    exists, project_registry_dir = get_project_registry_dir()
    if exists:
        projects += glob(project_registry_dir + '/*')
    else:
        cdkdir = os.environ.get('OCPI_CDK_DIR')
        if cdkdir:
            projects.append(cdkdir)
    logging.debug("All projects: " + str(projects))
    return projects

###############################################################################
# Utility functions for use with ocpidev driver files such as
# ocpidev_run.py, ocpidev_utilization.py. In other words, these functions are
# for use in the code very close to the command-line parsing.
###############################################################################

# Directory types that may contain sub-assets
COLLECTION_DIRTYPES = ["project", "applications", "library", "libraries", "hdl-library",
                       "hdl-platform", "hdl-platforms", "hdl-assemblies", "hdl-primitives"]

def get_ocpidev_working_dir(origin_path=".", noun="", name=".",
                            library=None, hdl_library=None, hdl_platform=None):
    """
    Given an origin path, noun, name  and location/library directives,
    determine the target assets's directory to work in.

    Basically, the arguments of this function describe the current state/location in a project
    as well as a description of where to look for an asset of a certain name and type.
    This function completes that mapping of:
    'current-state/location + asset-description' -> asset-directory,
    and returns the working directory of interest.

    First, this function converts the current state/location to location/library directives
    like library, hdl_library, hdl_platform and collection_type. So basically, convert the
    current-state + location-directives, to just location-directives. These location-directives
    along with descriptors like noun and name comprise an asset-description.

    At that point, the _get_asset_dir() function can be called which converts an asset-description
    to asset-directory. This nested function call does not require any current-state/location
    information, and will provide the location of the target asset relative to a project top.

    Finally, the path to the project of interest is prepended to this asset-directory and
    the resulting full-path is returned.

    Args:
        origin_path: path from which we are operating - this provides runtime context that may
                     translate to location directives:
                         (library/hdl_library/hdl_platform/collection_type)
        noun: noun/asset-type of interest. Defaults to blank
        name: name of the asset to locate. Basically, what is the name of the asset of type <noun>
        library: the library in which the asset of interest lives
        hdl_library: the HDL library in which the asset of interest lives (in the hdl/ subtree)
        hdl_platform: the HDL platform in which the asset of interest lives.
                      might itself be the asset

    Return:
        asset_dir: the full path to asset described via this function's arguments

    Notes:
        name and noun describe the name and type of asset being described.
        The library, platform arguments are location-directives or modifiers
        that dictate where to the asset should be found.
    """

    logging.debug("Getting ocpidev working dir from options:\n" +
                  str((origin_path, noun, name, library, hdl_library, hdl_platform)))

    # If the noun is project, append name to origin_path to ensure we operate from within a project
    # from here forward
    if noun == "project":
        origin_path = origin_path + "/" + name
        name = "."

    if not is_path_in_project(origin_path):
        raise OCPIException("Path \"" + origin_path + "\" is not in a project, so its settings " +
                            "as a subdirectory of a project could not be determined.")

    # if this is a collection type, we care about the current directory's dirtype,
    # otherwise we want the current asset's containing collection's dirtype
    cur_dirtype = get_dirtype(origin_path)
    if cur_dirtype is not None and cur_dirtype in COLLECTION_DIRTYPES:
        # operating on a collection-type, so just proceed as usual
        working_dir = origin_path
        dirtype = cur_dirtype
    else:
        # not operating on a collection-type. It is most convenient here
        # to determine the location/library-directives associated with the
        # asset's containing collection
        working_dir = origin_path + "/.."
        dirtype = get_dirtype(working_dir)
        # if no name was provided, set name to the origin_path's basename to preserve that
        # information as we move to the parent collection
        if (name is None or name == ".") and get_dirtype(origin_path) is not None:
            name = os.path.basename(os.path.realpath(origin_path))

    # error checking
    if (library is not None or hdl_library is not None) and dirtype in ["library", "hdl-library"]:
        raise OCPIException("[hdl-]library option cannot be provided when operating in a " +
                            "directory of [hdl-]library type.")
    if (hdl_platform is not None) and dirtype in "hdl_platform":
        raise OCPIException("hdl-platform option cannot be provided when operating in a " +
                            "directory of hdl-platform type.")

    working_basename = os.path.basename(os.path.realpath(working_dir))

    # if origin path is the 'hdl' subdir, and library is set, prepend 'hdl/'
    if dirtype is None and working_basename == "hdl" and library is not None:
        library = "hdl/" + library

    elif dirtype == "library":
        # Determine where this library lives to choose which locatin-directive
        # to set
        parent_dt = get_dirtype(working_dir + "/..")
        parent_basename = os.path.basename(os.path.realpath(working_dir + "/.."))
        library = working_basename
        if parent_dt in ["project", "libraries"]:
            # if the parent directory is type project or libraries, then this is
            # a component library (e.g. components or a sublibrary of components)
            library = working_basename
        elif parent_dt == "hdl-platform":
            # parent directory is an HDL platform, so set that directive
            hdl_platform = parent_basename
        elif parent_dt is None and parent_basename == "hdl":
            # parent directory has no type and is named 'hdl'. So, this is an hdl library
            # e.g. devices, cards, adapters
            library = "hdl/" + working_basename

    elif dirtype == "hdl-platform":
        hdl_platform = working_basename

    logging.debug("Getting ocpidev working dir from options (auxiliary function):\n" +
                  str((noun, name, library, hdl_library, hdl_platform, dirtype)))

    # If no noun was specified, and the dirtype is a collection, set noun to dirtype
    # For example, if a command was run in hdl/assemblies, but no noun was specified,
    # then just set the noun to hdl-assemblies
    if noun == "":
        noun = dirtype if dirtype in COLLECTION_DIRTYPES else ""

    # Now that the current state/directory has been converted to location directives,
    # call this helper function to convert or state-less asset-description to an
    # asset-directory from a project top
    asset_from_pj_top = _get_asset_dir(noun, name, library, hdl_library, hdl_platform)

    proj_path = get_path_to_project_top(origin_path)

    # prepend the asset-directory with the path to the project top
    asset_dir = os.path.realpath(proj_path + "/" + asset_from_pj_top)

    # ensure existence and return
    if os.path.exists(asset_dir):
        return asset_dir
    else:
        raise OCPIException("Determined working directory of \"" + asset_dir + "\" that does " +
                            "not exist.")

def _get_asset_dir(noun="", name=".", library=None, hdl_library=None, hdl_platform=None):
    """
    Given a noun, a name, and library directives
    determine the target asset's directory from a project top.

    Basically, the arguments of this function specify/describe an asset in a project.
    This function completes that mapping of asset-description -> asset-directory,
    and returns the directory of interest relative to the top of the project.

    Args:
        noun: noun/asset-type of interest. Defaults to blank
        name: name of the asset to locate. Basically, what is the name of the asset of type <noun>
        library: the library in which the asset of interest lives
        hdl_library: the HDL library in which the asset of interest lives (in the hdl/ subtree)
        hdl_platform: the HDL platform in which the asset of interest lives.
                      might itself be the asset

    Return:
        asset: the project subdirectory containing the asset described via this function's arguments

    Notes:
        name and noun describe the name and type of asset being described.
        The library and platform arguments are location-directives or modifiers
        that dictate where to the asset is found.

    Examples (doctest):
        >>> _get_asset_dir(noun="library", name="components")
        'components'
        >>> _get_asset_dir(noun="library", name="dsp_comps")
        'components/dsp_comps'
        >>> _get_asset_dir(noun="library", name="components/dsp_comps")
        'components/dsp_comps'
        >>> _get_asset_dir(noun="library", name="hdl/devices")
        'hdl/devices'
        >>> _get_asset_dir(noun="worker", name="complex_mixer.hdl", library="dsp_comps")
        'components/dsp_comps/complex_mixer.hdl'
        >>> _get_asset_dir(noun="test", name="bias.test", library="components")
        'components/bias.test'
        >>> _get_asset_dir(noun="worker", name="ad9361_adc.hdl", hdl_library="devices")
        'hdl/devices/ad9361_adc.hdl'
        >>> _get_asset_dir(noun="worker", name="matchstiq_z1_avr.hdl", hdl_platform="matchstiq_z1")
        'hdl/platforms/matchstiq_z1/devices/matchstiq_z1_avr.hdl'
        >>> _get_asset_dir(noun="applications")
        'applications'
        >>> _get_asset_dir(noun="application", name="rx_app")
        'applications/rx_app'
        >>> _get_asset_dir(noun="application", name="rx_app")
        'applications/rx_app'
        >>> _get_asset_dir(noun="hdl-assemblies")
        'hdl/assemblies'
        >>> _get_asset_dir(noun="hdl-assembly", name="fsk_modem")
        'hdl/assemblies/fsk_modem'
        >>> _get_asset_dir(noun="hdl-assembly", name="fsk_modem")
        'hdl/assemblies/fsk_modem'
        >>> _get_asset_dir(noun="hdl-primitive", name="zynq")
        'hdl/primitives/zynq'
        >>> _get_asset_dir(noun="hdl-primitive", name="zynq")
        'hdl/primitives/zynq'
        >>> _get_asset_dir(noun="hdl-library", name="devices")
        'hdl/devices'
        >>> _get_asset_dir(noun="hdl-platform", name="matchstiq_z1")
        'hdl/platforms/matchstiq_z1'
        >>> _get_asset_dir(noun="project", name="assets")
        'assets'
        >>> _get_asset_dir(noun="project")
        '.'
        >>> _get_asset_dir(library="components", hdl_library="devices", hdl_platform="matchstiq_z1")
        Traceback (most recent call last):
            ...
        OCPIException: ...
        >>> _get_asset_dir(noun="INVALID")
        Traceback (most recent call last):
            ...
        OCPIException: ...
    """
    # library-directives are mutually exclusive
    if library is not None and hdl_library is not None:
        #raise OCPIException("library, hdl_library, and hdl_platform options are " +
        raise OCPIException("library and hdl_library are mutually exclusive.")

    # asset types that live inside a library
    library_assets = ["worker", "test"]

    # For most nouns, there is only a single directory to choose from
    if noun == "libraries" and library is None:
        asset = "components"
    elif noun in ["application", "applications"]:
        asset = "applications"
    elif noun in ["hdl-primitive", "hdl-primitives"]:
        asset = "hdl/primitives"
    elif noun == "hdl-platforms":
        asset = "hdl/platforms"
    elif noun in ["hdl-assembly", "hdl-assemblies"]:
        asset = "hdl/assemblies"
    elif noun == "hdl-platform" or hdl_platform is not None:
        # hdl platform is specified in some way

        # cannot have a platform noun AND directive
        if noun == "hdl-platform" and name is not "." and hdl_platform is not None:
            raise OCPIException("Could not choose between two HDL platform directories: '" +
                                name + "' and '" + hdl_platform + "'.")
        # default hdl platform if needed
        hdl_platform = name if hdl_platform is None else hdl_platform
        # hdl_platform location directive or noun implies directories
        # like hdl/platforms/<hdl-platform>
        if noun in library_assets + ["library", "workers", "tests"]:
            # can only specify library as 'devices' in a platform directory
            if noun == "library" and name not in [".", "devices", "devices/"]:
               raise OCPIException("Only valid library in an HDL platform is 'devices'")
            # if the hdl_platform location directive is used and a library-asset is being
            # located, drill down into the platform's devices library
            asset = "hdl/platforms/" + hdl_platform + "/devices"
        else:
            asset = "hdl/platforms/" + hdl_platform

    elif noun == "library" or library is not None:
        # library is specified in some way

        # cannot have a library noun AND directive
        if noun == "library" and name is not "." and library is not None:
            raise OCPIException("Could not choose between two library directories: '" +
                                name + "' and '" + library + "'.")
        # default library if needed
        library = name if library is None else library
        if library == "components" or "/" in library:
            # library is either the components directory or a full path - use it as-is
            asset = library
        else:
            # library is just a name (not a path) - prepend components/ to it
            asset = "components/" + library

    elif noun == "hdl-library" or hdl_library is not None:
        # hdl library is specified in some way

        # cannot have a library noun AND directive
        if noun == "hdl-library" and name is not "." and hdl_library is not None:
            raise OCPIException("Could not choose between two hdl_library directories: '" +
                                name + "' and '" + hdl_library + "'.")
        # default hdl library if needed
        hdl_library = name if hdl_library is None else hdl_library
        # hdl libraries live in the hdl/ subtree
        asset = "hdl/" + hdl_library

    elif noun in ["project", "workers", "tests"]:
        # If we have gotten this far and a "workers" or "tests" is discovered, the only
        # remaining asset type that it can be is "project".

        # When locating a project, it is either the current project (name is none), or the
        # the directory is specified by name
        asset = "." if name is None else name

    elif noun is None:
        # when no noun or location-directive gives direction regarding where to look for an
        # asset, assume the top-level of the project
        asset = "."
    else:
        raise OCPIException("Invalid noun provided: " + str(noun))

    # If the asset-type/noun is a collection-type, then we already know its location.
    #     E.g. If the noun is 'library' (a collection-type) and the name is 'dsp_comps',
    #          then by now we have obtained asset='components/dsp_comps'. Just return that.
    # Otherwise append 'name' to the determined collection-type asset.
    #     E.g. If library directives describe a collection-type/asset named 'components/dsp_comps',
    #          and the noun is 'worker' with name 'complex_mixer.hdl', then this will result in
    #          'components/dsp_comps/complex_mixer.hdl
    if noun in COLLECTION_DIRTYPES or name is None:
        return asset
    else:
        return asset + "/" + name

###############################################################################
# String, number and dictionary manipulation utility functions
###############################################################################

# https://stackoverflow.com/questions/38987/how-to-merge-two-dictionaries-in-a-single-expression
# when code is moved to python 3.5+ this function goes away and becomes: z = {**x, **y}
def merge_two_dicts(dict1, dict2):
    """
    This function combines two dictionaries into a single dictionary, if both have the same key
    the value in dict2 overwrites the value in dict1
    >>> dict1 = {'a': 'avalue'}
    >>> dict2 = {'b': 'bvalue'}
    >>> dict3 = merge_two_dicts(dict1, dict2)
    >>> sorted(dict3.items())
    [('a', 'avalue'), ('b', 'bvalue')]
    """
    merged = dict1.copy()   # start with dict's keys and values
    merged.update(dict2)    # modifies merged with dict2's keys and values & returns None
    return merged

def python_list_to_bash(pylist):
    """
    Convert a python list to a Makefile list (a space separated string)

    For example (docstring):
    >>> python_list_to_bash(["element1", "element2"])
    'element1 element2'
    >>> python_list_to_bash([""])
    ''
    >>> python_list_to_bash([])
    ''
    """
    mklist = ""
    for pyelem in pylist:
        mklist = mklist + " " + str(pyelem)
    return mklist.strip()

def bash_list_to_python(mklist):
    """
    Convert a bash list (a space separated string) to a python list

    For example (doctest):
    >>> bash_list_to_python("element0 element1 element2     element3     ")
    ['element0', 'element1', 'element2', 'element3']
    >>> bash_list_to_python(" element1 ")
    ['element1']
    >>> bash_list_to_python("  ")
    []
    >>> bash_list_to_python("")
    []
    """
    return [elem for elem in mklist.strip().split(" ") if elem != ""]


def isfloat(value):
    """
    Can the parameter be represented as an float?

    For example (doctest):
    >>> isfloat("1")
    True
    >>> isfloat("1.0")
    True
    >>> isfloat("")
    False
    >>> isfloat("string")
    False
    """
    try:
        float(value)
        return True
    except ValueError:
        return False

def isint(value):
    """
    Can the parameter be represented as an int?

    For example (doctest):
    >>> isint(1)
    True
    >>> isint("1")
    True
    >>> isint("1.0")
    False
    >>> isint("")
    False
    >>> isint("string")
    False
    """
    try:
        if isinstance(value, str):
            return value.replace(",", "").isdigit()
        return isinstance(value, int)
    except ValueError:
        return False

def str_to_num(num_str):
    num_str_stripped = num_str.replace(",", "")
    return int(num_str_stripped) if isint(num_str_stripped) else float(num_str_stripped)

def freq_from_str_period(prd_string):
    """
    Convert a string representing period in ps, ns, us, ms, s (default)
    to a string representing frequency in Hz
    Return the string or "" on failure

    For example (doctest):
    >>> freq_from_str_period("250ps")
    4000000000.0
    >>> freq_from_str_period("250ns")
    4000000.0
    >>> freq_from_str_period("250us")
    4000.0
    >>> freq_from_str_period("250ms")
    4.0
    >>> freq_from_str_period("250s")
    0.004

    No units provided, 's' is assumed
    >>> freq_from_str_period("250")
    0.004

    >>> freq_from_str_period("")
    >>> freq_from_str_period("string")
    """
    if prd_string is None:
        return None
    period = 0.0
    ps_prd = re.sub('ps', '', prd_string)
    ns_prd = re.sub('ns', '', prd_string)
    ms_prd = re.sub('ms', '', prd_string)
    us_prd = re.sub('us', '', prd_string)
    # Make sure 's' is only detected if 's' directly follows a digit
    s_prd = re.sub(r'([0-9])s', '\g<1>', prd_string)
    # Choose the numerator based on which units were detected
    if ps_prd != prd_string:
        prd = ps_prd
        numerator = 1E12
    elif ns_prd != prd_string:
        prd = ns_prd
        numerator = 1E9
    elif us_prd != prd_string:
        prd = us_prd
        numerator = 1E6
    elif ms_prd != prd_string:
        prd = ms_prd
        numerator = 1E3
    else:
        prd = s_prd
        numerator = 1
    if isfloat(prd):
        period = float(prd)
    if period != 0:
        freq = numerator/float(period)
        return freq
        #return '{0:.3f}'.format(freq)
    return None

def first_num_in_str(parse_string):
    """
    Return the first number in a list of strings. Allow ',' or '.'  and trailing text
    (e.g. MHz, ns). Return "" if no number is found.

    For example (doctest):
    >>> first_num_in_str("Here is my number: 12345.12345 (here is a second number 222.222)")
    '12345.12345'
    >>> first_num_in_str("Another number: 12,345,123")
    '12,345,123'
    >>> first_num_in_str("A frequency: 123,000MHz")
    '123,000'
    >>> first_num_in_str("A period: 123,000ns")
    '123,000'
    >>> first_num_in_str("abcd1234efgh5678")
    '1234'
    >>> first_num_in_str("There is no string here!")
    """
    first_num_regex = re.compile(r"([-+]?([0-9]{1,3}\,?)+(\.[0-9]*)?)")
    result = re.search(first_num_regex, parse_string)
    if result:
        return result.group(0)
    else:
        return None

def match_regex(target_file, regex):
    """
    Parse the target file for a regex and return the first group/element
    of the first match

    For example (doctest):
    Here, we have a line of text below and we parse this current file
    using a regular expression to grab the 'ns' number from the string.
    # This text right here contains a number (2.000ns)!!!! It should match the regex
    >>> match_regex(os.path.realpath(__file__), "#.*This text right.*\((.*)ns\)!!!!.*")
    '2.000'
    """
    if isinstance(regex, str):
        regex = re.compile(regex)
    elif not isinstance(regex, type(re.compile(''))):
        raise OCPIException("Error: regular expression invalid")

    # Have to use encoding/errors flags here to avoid failure when file has non-ASCII
    with open(target_file, encoding="utf8", errors="ignore") as tgt_file:
        for line in tgt_file:
            # List of all matches on the line
            matches = re.findall(regex, line)
            # Were any matches found?
            if matches and matches[0]:
                # Get the first match on the line
                match = matches[0]
                if isinstance(match, tuple):
                    # Match might be a tuple (if 1+ groups are in the pattern). In that case
                    # return the first group in the match tuple
                    return match[0]
                return match
    # no match found
    return None



def match_regex_get_first_num(target_file, regex):
    """
    Match the given regex in the given file (get the first match),
    and return the first number in the matched string.
    """
    # TODO add doctest
    if isinstance(regex, str):
        # if regex is a string, compile to regex
        regex = re.compile(regex)
    elif not isinstance(regex, type(re.compile(''))):
        raise OCPIException("Error: regular expression invalid")

    # Have to use encoding/errors flags here to avoid failure when file has non-ASCII
    with open(target_file, encoding="utf8", errors='ignore') as tgt_file:
        for line in tgt_file:
            # List of all matches on the line
            matches = re.findall(regex, line)
            # Were any matches found?
            if matches and matches[0]:
                # Get the first number in the first match on the line
                first_num = first_num_in_str(matches[0])
                if first_num != None:
                    return first_num
    # no number/match found
    return None

# This function first finds the max-length of each 'column' and then iterates
# through each list and reformats each element to match the length corresponding
# to its 'column'
def normalize_column_lengths(lists):
    """
    This function takes a list of lists and treats each list like a row in a table.
    It then space-expands the length of each string element so that all elements in
    the same column have the same length.

    For example (doctest):
    >>> list1, list2 = normalize_column_lengths([["15 chr long str",\
                                                  "this is a longgg string"],\
                                                 ["< 15", "pretty short"]])
    >>> print (str(list1))
    ['15 chr long str', 'this is a longgg string']
    >>> print (str(list2))
    ['< 15           ', 'pretty short           ']
    """
    lists = [[str(elem) for elem in lst] for lst in lists]
    format_function = lambda length, string_elem: ("{0:<" + str(length) + "}").format(string_elem)
    newlens = []
    for column in zip(*lists):
        newlens.append(len(max(column, key=len)))
    newlists = []
    for oldlist in lists:
        newlists.append([format_function(length, elem) for elem, length in zip(oldlist, newlens)])
    return newlists

def format_table(rows, col_delim='|', row_delim=None, surr_cols_delim='|', surr_rows_delim='-',
                 underline=None):
    """
    Return a table specified by the list of rows in the 'rows' parameter. Optionally specify
    col_delim and row_delim which are each a single character that will be repeated to separate
    cols and rows. Note that if row_delim is a string with len>1, it will be used a single time
    for each row instead of being repeated to form a row. surr_rows_delim and surr_cols_delim
    determine the border of the table.
    """

    rows_norm = normalize_column_lengths(rows)

    # If an underline character was provided, insert a row containing this character repeated
    # at position 1 (right below the header)
    if underline:
        rows_norm.insert(1, [underline * len(elem) for elem in rows_norm[0]])

    # The start/end column delimeters should have a space after/before them IFF they are nonempty
    start_col_delim = surr_cols_delim + ' ' if surr_cols_delim else ''
    end_col_delim = ' ' + surr_cols_delim if surr_cols_delim else ''

    row_strs = []
    # Separate the elements in each row by spaces and column-delimeters
    for row in rows_norm:
        row_strs.append(start_col_delim
                        + reduce(lambda x, y: x + ' ' + col_delim + ' ' + y, row)
                        + end_col_delim)
    max_row_len = len(max(row_strs, key=len))
    if row_delim:
        # row_delim can be a single char that is repeated to form a row, or can be multiple
        # characters which would be used as-is as the row-delimeter
        if len(row_delim) == 1:
            row_line = row_delim * max_row_len
        else:
            row_line = row_delim

    table_str = ""
    if surr_rows_delim:
        # surr_row_delim is a single character that will be repeated to form the rows that form
        # the borders of the table
        table_str += surr_rows_delim * max_row_len + "\n"

    # Print each row of the table followed by a delimeter row (e.g. a line)
    for line in row_strs:
        table_str += line.rstrip() + "\n"
        if row_delim:
            table_str += row_line.rstrip() + "\n"

    if surr_rows_delim:
        # surr_row_delim is a single character that will be repeated to form the rows that form
        # the borders of the table
        table_str += surr_rows_delim * max_row_len + "\n"

    return table_str

def print_table(rows, col_delim='|', row_delim=None, surr_cols_delim='|', surr_rows_delim='-',
                underline=None):
    """
    Wrapper for "format_table()" that prints the returned table
    """
    print(format_table(rows, col_delim, row_delim, surr_cols_delim, surr_rows_delim, underline))


class Report(object):
    """
    A Report consists of multiple data points. Each data point is a dictionary of key=>value
    pairs. The headers (which correspond to the keys for each data-point), should be represented
    via the "ordered_headers" list which determines the ordering each data-point's items when
    presented.
    """
    def __init__(self, ordered_headers=[], sort_priority=[]):
        # User can initialize with the list of ordered headers
        self.ordered_headers = ordered_headers
        # User can initialize with the list of headers to sort by
        self.sort_priority = sort_priority
        self.data_points = []

    def __bool__(self):
        """
        If data_points = [], then 'if self' should return False
        """
        return bool(self.data_points)

    def append(self, data_point):
        """
        Appending a data-point to this Report just appends the data-point to
        this Report's list of data_points
        """
        self.data_points.append(data_point)

    def __iadd__(self, other):
        """
        Adding a Report to this one is the equivalent of adding a Report's data_points
        to this Reports data-point list. If this Report's ordered_headers is empty,
        use the other Report's ordered_headers.
        if not isinstance(other, Report):
        """
        if not isinstance(other, Report):
            raise OCPIException("Can only add another Report object to a Report.")
        # Headers are unchanged on addition unless headers are empty in which case,
        # user other's headers
        if not self.ordered_headers:
            self.ordered_headers = other.ordered_headers
        self.data_points += other.data_points
        return self

    def assign_for_all_points(self, key, value):
        """
        Add a key=>value pair to every data-point in this report
        Essentially, add a dimension to the data-set, and initialize every data-point's value
        """
        for point in self.data_points:
            point[key] = value

    def print_table(self, none_str="N/A"):
        """
        Print a table of this Report's data-points. Lead with the headers and present
        each data-point's elements as per the order in "ordered_headers".
        Let the caller specify a string to use for empty/None entries (Default: N/A)
        """
        if self.data_points:
            # Fill in empty values in data-points with none_str
            for header in self.ordered_headers:
                for point in self.data_points:
                    if header not in point:
                        point[header] = none_str
            # Construct the list of rows. Do so following the ordered_headers order
            rows = []
            for point in self.data_points:
                row = []
                for header in self.ordered_headers:
                    elem = point[header]
                    if elem is None:
                        row.append(none_str)
                    elif isinstance(elem, str):
                        # element is a string, just append to row
                        row.append(elem)
                    elif isinstance(elem, list):
                        # element is a list, join its contents with comma and append to row
                        row.append(", ".join(elem))
                    else:
                        # Unsupported element type (not a string or list)
                        try:
                            row.append(str(elem))
                        except Exception as ex:
                            logging.error(ex)
                            raise OCPIException("Element in Report is not a string and could not" +
                                                " be cast to a string.")

                rows.append(row)
            # Sort the rows by each header listed in sort_priority. Do this in reverse order
            # so the last sort key (and therefore the most significant) is the first element
            # in sort_priority
            for sort_header in reversed(self.sort_priority):
                rows.sort(key=operator.itemgetter(self.ordered_headers.index(sort_header)))
            # insert the headers as row 0
            rows.insert(0, self.ordered_headers)
            # format the table using a generic function and print
            print(format_table(rows, underline="-"))
        else:
            logging.warning("Not printing table for empty report.")

    def get_latex_table(self, caption="Utilization Table", none_str="N/A",
                        gen_stamp="Generated on %s" % datetime.datetime.now().ctime()):
        """
        Get a table of this Report's data-points. Lead with the headers and present each
        data-point's elements as per the order in "ordered_headers".
        Do this in a LaTeX parseable table format, and allow for a caption to be provided.
        Let the caller specify a string to use for empty/None entries (Default: N/A)
        Let the caller specify a generation-stamp, which will be placed at the top
            of the output. Defaults to "Generated on <date-and-time>"
        """
        # LaTeX strings being generated have many backslashes as is common in LaTeX.
        # So, disable the pylint checker for this:
        # pylint:disable=anomalous-backslash-in-string
        if self.data_points:
            # Fill in empty values in data-points with none_str
            for header in self.ordered_headers:
                for point in self.data_points:
                    if header not in point:
                        point[header] = none_str
            # Set the first row to a copy of our ordered_headers list
            # Sort the rows by each header listed in sort_priority. Do this in reverse order
            rows = [[header.replace("_", "\_") for header in self.ordered_headers.copy()]]
            # so the last sort key (and therefore the most significant) is the first element
            # in sort_priority
            for sort_header in reversed(self.sort_priority):
                rows.sort(key=operator.itemgetter(self.ordered_headers.index(sort_header)))
            # Add an indent to the first element for pretty alignment in LaTeX
            rows[0][0] = " " * 12 + rows[0][0]
            # End the line in a LaTeX newline
            rows[0][-1] += " \\\\"

            # The string below is the LaTeX table template. It is filled out before being returned
            latex_table_tmplt = \
"""%% %s

%% It is best to wrap this table in \\begin{landscape} and \\end{landscape} in its including doc
\\begin{tiny}
    \\begin{longtable}[l]{* {%d}{|c}|}
    \captionsetup{justification=raggedright,singlelinecheck=false}
    \caption{%s}\\\\
        \hline
        \\rowcolor{blue}
%s        \end{longtable}
\end{tiny}
"""

            for point in self.data_points:
                latex_row = []
                for header in self.ordered_headers:
                    # Even if str() is removed here, must make sure we copy the point
                    # and do not change it in-place
                    elem = point[header]
                    if elem is None:
                        elem_str = none_str
                    elif isinstance(elem, list):
                        # If the value for this cell is a list, create a sub-table
                        # with a row for each entry
                        elem_str = "\\begin{tabular}{@{}l@{}}" + \
                                   " \\\\ ".join(elem) + \
                                   "\end{tabular}"
                    elif isint(elem) or isfloat(elem):
                        if isinstance(elem, str):
                            elem_str = str_to_num(elem)
                        else:
                            elem_str = elem
                    else:
                        # loop above ensures that all elements are lists or strings,
                        # so this is a str
                        elem_str = elem
                    # LaTeX requires that plain-text underscores by escaped
                    latex_row.append(str(elem_str).replace("_", "\_"))
                # indent the row
                latex_row[0] = " " * 12 + latex_row[0]
                # add a LaTeX newline
                latex_row[-1] += " \\\\"
                rows.append(latex_row)

            # Format the table so that columns align in the LaTeX source. Separate lines
            # with "\hline", and separate columns with "&".
            latex_table = format_table(rows, col_delim="&", row_delim=" " * 12 + "\\hline",
                                       surr_cols_delim="", surr_rows_delim="", underline="")
            # Fill in the LaTeX table template with the current date/time, the provided caption,
            # the column descriptor string, and the actual table content
            return latex_table_tmplt % (gen_stamp, len(self.ordered_headers),
                                        caption.replace("_", "\_"), latex_table)
        else:
            logging.warning("Not printing table for empty report.")
            return ""
        # pylint:enable=anomalous-backslash-in-string

###############################################################################
# Functions to ease filesystem navigation
###############################################################################
def name_of_dir(directory="."):
    """
    Return the name of the directory provided. This will return the actual
    Directory name even if the argument is something like "."
    For example (doctest):
    >>> name_of_dir("/etc/.")
    'etc'
    >>> name_of_dir("/etc/../etc")
    'etc'
    """
    return os.path.basename(os.path.realpath(directory))

# Disabling a pylint check so we use the name 'cd' even though it is so short
# pylint:disable=invalid-name
@contextmanager
def cd(target):
    """
    Change directory to 'target'. To be used with 'with' so that origin directory
    is automatically returned to on completion of 'with' block
    """
    origin = os.getcwd()
    os.chdir(os.path.expanduser(target))
    try:
        yield
    finally:
        os.chdir(origin)
# pylint:enable=invalid-name

###############################################################################
# Functions for prompting the user for input
###############################################################################
def get_ok(prompt="", default=False):
    """
    Prompt the user to say okay
    """
    print(prompt, end=' ')
    while True:
        ok_input = input(" [y/n]? ")
        if ok_input.lower() in ['y', 'yes', 'ok']:
            return True
        if ok_input.lower() in ['n', 'no', 'nope']:
            return False
        if ok_input.lower() == '':
            return default

if __name__ == "__main__":
    import doctest
    __LOG_LEVEL = os.environ.get('OCPI_LOG_LEVEL')
    __VERBOSITY = False
    if __LOG_LEVEL:
        try:
            if int(__LOG_LEVEL) >= 8:
                __VERBOSITY = True
        except ValueError:
            pass
    doctest.testmod(verbose=__VERBOSITY, optionflags=doctest.ELLIPSIS)
configure_logging()
