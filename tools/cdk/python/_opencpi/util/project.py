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
definitions for utility functions that have to do with opencpi project layout
"""

import os
import os.path
import logging
from glob import glob
import subprocess
import re
from _opencpi.util import cd, set_vars_from_make, OCPIException

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
        elif directory.endswith(("rcc/platforms", "rcc/platforms/")):
            return "rcc-platforms"
        elif "rcc/platforms/" in directory:
            return "rcc-platform"
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
        # pylint:disable=undefined-variable
        raise OCPIException("origin_path \"" + str(origin_path) + "\" is not in a project")
        # pylint:enable=undefined-variable
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
        # pylint:disable=undefined-variable
        raise OCPIException("origin_path \"" + str(origin_path) + "\" is not in a project")
        # pylint:enable=undefined-variable
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
            if (not project_vars is None and 'ProjectPackage' in project_vars and
                    len(project_vars['ProjectPackage']) > 0):
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
    if (is_path_in_project(directory) and
            os.path.isdir(get_path_to_project_top(directory) + "/imports")):
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

VALID_PLURAL_NOUNS = ["tests", "libraries", "workers"]
def get_ocpidev_working_dir(noun, name, library=None, hdl_library=None, hdl_platform=None):
    """
    TODO
    notes:
        - maybe this function shoud return noun and dir based on options does this even help
        - can only use from outside of a project for global show stuff and when specifying a
            project or registry
        - need to validate when you can leave name/noun blank and stuff too
        - should this be a class on its own instead of a utility function?
        - should they be static method on all thier respective asset classes ?
        -
    """
    # what about showing of global things is this function even called in that case? isnt the object
    # that is created always the current registry?
    if not is_path_in_project(".") and not is_path_in_project(name):
        raise OCPIException("Path \"" + os.path.realpath(".") + "\" is not in a project, " +
                            "so this command is invalid.")
    cur_dirtype = get_dirtype() if get_dirtype() != "libraries" else "library"
    name = "" if name == os.path.basename(os.path.realpath(".")) else name

    cur_dir_not_name = noun == cur_dirtype and not name
    noun_valid_not_name = noun in VALID_PLURAL_NOUNS and not name

    if (not noun and not name) or cur_dir_not_name or noun_valid_not_name:
        return "."

    # pylint:disable=no-name-in-module
    # pylint:disable=import-error
    import _opencpi.assets.application
    import _opencpi.assets.test
    import _opencpi.assets.worker
    import _opencpi.assets.platform
    import _opencpi.assets.assembly
    import _opencpi.assets.library
    import _opencpi.assets.project
    import _opencpi.assets.registry
    import _opencpi.assets.component

    # pylint:enable=no-name-in-module
    # pylint:enable=import-error

    # pylint:disable=no-member
    working_dir_dict = {
        "application"    : _opencpi.assets.application.Application.get_working_dir,
        "applications"   : _opencpi.assets.application.ApplicationsCollection.get_working_dir,
        "project"        : _opencpi.assets.project.Project.get_working_dir,
        "library"        : _opencpi.assets.library.Library.get_working_dir,
        "test"           : _opencpi.assets.test.Test.get_working_dir,
        "component"      : _opencpi.assets.component.Component.get_working_dir,
        "worker"         : _opencpi.assets.worker.Worker.get_working_dir,
        "hdl-platform"   : _opencpi.assets.platform.HdlPlatformWorker.get_working_dir,
        "hdl-platforms"  : _opencpi.assets.platform.HdlPlatformsCollection.get_working_dir,
        "hdl-assembly"   : _opencpi.assets.assembly.HdlApplicationAssembly.get_working_dir,
        "hdl-assemblies" : _opencpi.assets.assembly.HdlAssembliesCollection.get_working_dir,
        }
    # pylint:enable=no-member

    if noun in working_dir_dict:
        asset_dir = working_dir_dict[noun](name, library, hdl_library, hdl_platform)
    else:
        raise OCPIException("Invalid noun \"" + noun + "\" .  Valid nouns are: " +
                            ' '.join(working_dir_dict.keys()))

    # ensure existence and return
    if os.path.exists(asset_dir):
        return os.path.realpath(asset_dir)
    else:
        # pylint:disable=undefined-variable
        raise OCPIException("Determined working directory of \"" + asset_dir + "\" that does " +
                            "not exist.")
        # pylint:enable=undefined-variable

def throw_not_valid_dirtype_e(valid_loc):
    """
    throws an exception and forms a error message if not in a valid directory
    """
    raise OCPIException("The directory of type " + get_dirtype() + " is not a valid directory " +
                        "type to run this command in. Valid directory types are: " +
                        ' '.join(valid_loc))

def check_no_libs(dir_type, library, hdl_library, hdl_platform):
    if library: throw_not_blank_e(dir_type, "--library option", False)
    if hdl_library: throw_not_blank_e(dir_type, "--hdl-library option", False)
    if hdl_platform: throw_not_blank_e(dir_type, "-P option", False)

def throw_not_blank_e(noun, var, always):
    """
    throws an exception and forms a error message if a variable is blank and shouldn't be or
    if a variable is not bank ans should be blank
    """
    always_dict = {True: "not", False : "always"}
    raise OCPIException(var + " should " +  always_dict[always] + " be blank for " + noun +
                        " operations.  Command is invalid.")

def throw_invalid_libs_e():
    """
    throws an exception and forms a error message if too many library locations are specified
    """
    raise OCPIException("only one of hdl_libary, library, and hdl_platform can be used.  Invalid " +
                        "Command, Choose only one.")

def throw_specify_lib_e():
    """
    throws an exception and forms a error message if no library is specified
    """
    raise OCPIException("No library specified, Invalid Command.")

def get_component_filename(library, name):
    """
    >>> os.mknod("/tmp/my_file-spec.xml")
    >>> get_component_filename("/tmp", "my_file-spec.xml")
    '/tmp/my_file-spec.xml'
    >>> get_component_filename("/tmp", "my_file-spec")
    '/tmp/my_file-spec.xml'
    >>> get_component_filename("/tmp", "my_file")
    '/tmp/my_file-spec.xml'
    >>> os.remove("/tmp/my_file-spec.xml")
    >>> os.mknod("/tmp/my_file_spec.xml")
    >>> get_component_filename("/tmp", "my_file")
    '/tmp/my_file_spec.xml'
    >>> os.remove("/tmp/my_file_spec.xml")
    """
    basename = library + "/" + name
    end_list = ["", ".xml", "_spec.xml", "-spec.xml"]
    for ending in end_list:
        if os.path.exists(basename + ending):
            return basename + ending
    return basename

def get_package_id_from_vars(package_id, package_prefix, package_name, directory="." ):
    """
    >>> get_package_id_from_vars("package_id", "package_prefix", "package_name")
    'package_id'
    >>> get_package_id_from_vars(None, "package_prefix", "package_name")
    'package_prefix.package_name'
    >>> get_package_id_from_vars(None, None, "package_name")
    'local.package_name'
    >>> get_package_id_from_vars(None, None, None, "/etc")
    'local.etc'
    """
    if package_id:
        return package_id
    if not package_prefix:
        package_prefix = "local"
    if not package_name:
        package_name = os.path.basename(os.path.realpath(directory))
    return package_prefix + "." + package_name

if __name__ == "__main__":
    import doctest
    import sys
    __LOG_LEVEL = os.environ.get('OCPI_LOG_LEVEL')
    __VERBOSITY = False
    if __LOG_LEVEL:
        try:
            if int(__LOG_LEVEL) >= 8:
                __VERBOSITY = True
        except ValueError:
            pass
    doctest.testmod(verbose=__VERBOSITY, optionflags=doctest.ELLIPSIS)
    sys.exit(doctest.testmod()[0])
