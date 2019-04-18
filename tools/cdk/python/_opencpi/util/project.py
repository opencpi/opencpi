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
import copy
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

# Directory types that may contain sub-assets
COLLECTION_DIRTYPES = ["project", "applications", "library", "libraries", "hdl-library",
                       "hdl-platform", "hdl-platforms", "hdl-assemblies", "hdl-primitives",
                       "rcc-platforms", "component", "workers", "tests"]

# pylint:disable=too-many-arguments
# pylint:disable=too-many-branches
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
    if not name:
        name = "."
    if noun == "project":
        origin_path = origin_path + "/" + name
        name = "."
    if not is_path_in_project(origin_path):
        # pylint:enable=undefined-variable
        raise OCPIException("Path \"" + os.path.realpath(origin_path) + "\" is not in a project, " +
                            "so this command is invalid.")
        # pylint:enable=undefined-variable

    # if this is a collection type, we care about the current directory's dirtype,
    # otherwise we want the current asset's containing collection's dirtype
    cur_dirtype = get_dirtype(origin_path)
    col_and_test = copy.deepcopy(COLLECTION_DIRTYPES)
    col_and_test.append("test")
    if cur_dirtype is not None and cur_dirtype in col_and_test:
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
            dirtype = get_dirtype(origin_path)

    # error checking
    if (library is not None or hdl_library is not None) and dirtype in ["library", "hdl-library"]:
        # pylint:disable=undefined-variable
        raise OCPIException("[hdl-]library option cannot be provided when operating in a " +
                            "directory of [hdl-]library type.")
        # pylint:enable=undefined-variable
    if (hdl_platform is not None) and dirtype in ["hdl_platform"]:
        # pylint:disable=undefined-variable
        raise OCPIException("hdl-platform option cannot be provided when operating in a " +
                            "directory of hdl-platform type.")
        # pylint:enable=undefined-variable

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
    elif dirtype == "test" and name == "." and not noun:
        name = os.path.basename(os.path.realpath("."))
    logging.debug("Getting ocpidev working dir from options (auxiliary function):\n" +
                  str((noun, name, library, hdl_library, hdl_platform, dirtype)))

    # If no noun was specified, and the dirtype is a collection, set noun to dirtype
    # For example, if a command was run in hdl/assemblies, but no noun was specified,
    # then just set the noun to hdl-assemblies

    if not noun:
        col_and_test = copy.deepcopy(COLLECTION_DIRTYPES)
        col_and_test.append("test")
        noun = dirtype if dirtype in col_and_test else ""
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
        # pylint:disable=undefined-variable
        raise OCPIException("Determined working directory of \"" + asset_dir + "\" that does " +
                            "not exist.")
        # pylint:enable=undefined-variable
# pylint:enable=too-many-arguments
# pylint:enable=too-many-branches

# pylint:disable=too-many-statements
# pylint:disable=too-many-branches
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
        >>> _get_asset_dir(noun="test", name="bias", library="components")
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
        _opencpi.util.OCPIException: library and hdl_library are mutually exclusive.
        >>> _get_asset_dir(noun="INVALID")
        Traceback (most recent call last):
            ...
        _opencpi.util.OCPIException: Invalid noun provided: INVALID
    """
    #TODO if i do show project in a lower level dir it should use find project top to locate the
    # directory of the project
    if noun == "test" and not name.endswith((".test", ".test/")):
        name += ".test"
    # library-directives are mutually exclusive
    if library is not None and hdl_library is not None:
        # pylint:disable=undefined-variable
        raise OCPIException("library and hdl_library are mutually exclusive.")
        # pylint:enable=undefined-variable

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
    elif noun == "component":
        if hdl_library:
            asset = get_component_filename("hdl/" +  hdl_library + "/specs/", name)
        elif library:
            asset = get_component_filename(library + "/specs/", name)
        elif hdl_platform:
            asset = get_component_filename("hdl/platforms/" +  hdl_platform + "/devices/specs/",
                                           name)
        else:
            asset = get_component_filename("specs/", name)

    elif noun == "hdl-platform" or hdl_platform is not None:
        # hdl platform is specified in some way

        # cannot have a platform noun AND directive
        if noun == "hdl-platform" and name is not "." and hdl_platform is not None:
            # pylint:disable=undefined-variable
            raise OCPIException("Could not choose between two HDL platform directories: '" +
                                name + "' and '" + hdl_platform + "'.")
            # pylint:enable=undefined-variable
        # default hdl platform if needed
        hdl_platform = name if hdl_platform is None else hdl_platform
        # hdl_platform location directive or noun implies directories
        # like hdl/platforms/<hdl-platform>
        if noun in library_assets + ["library", "workers", "tests"]:
            # can only specify library as 'devices' in a platform directory
            if noun == "library" and name not in [".", "devices", "devices/"]:
                # pylint:disable=undefined-variable
                raise OCPIException("Only valid library in an HDL platform is 'devices'")
                # pylint:enable=undefined-variable
            # if the hdl_platform location directive is used and a library-asset is being
            # located, drill down into the platform's devices library
            asset = "hdl/platforms/" + hdl_platform + "/devices"
        else:
            asset = "hdl/platforms/" + hdl_platform

    elif noun == "library" or library is not None:
        # library is specified in some way

        # cannot have a library noun AND directive
        if noun == "library" and name is not "." and library is not None:
            # pylint:disable=undefined-variable
            raise OCPIException("Could not choose between two library directories: '" +
                                str(name) + "' and '" + str(library) + "'.")
            # pylint:enable=undefined-variable
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
            # pylint:disable=undefined-variable
            raise OCPIException("Could not choose between two hdl_library directories: '" +
                                name + "' and '" + hdl_library + "'.")
            # pylint:enable=undefined-variable
        # default hdl library if needed
        hdl_library = name if hdl_library is None else hdl_library
        # hdl libraries live in the hdl/ subtree
        asset = "hdl/" + hdl_library

    elif noun in library_assets:
        if get_dirtype("..") == "library" and get_dirtype("../..") == "libraries":
            asset =  "components/" + os.path.basename(os.path.realpath("..")) + "/"
        else:
            asset = "components/"

    elif noun in ["project", "workers", "tests"]:
        # If we have gotten this far and a "workers" or "tests" is discovered, the only
        # remaining asset type that it can be is "project".

        # When locating a project, it is either the current project (name is none), or the
        # the directory is specified by name
        use_name = name is not None and os.path.basename(os.path.realpath(".")) != name
        asset = "." if not use_name else name
    elif not noun:
        # when no noun or alocation-directive gives direction regarding where to look for an
        # asset, assume the top-level of the project
        asset = "."
        noun = get_dirtype()
    else:
        # pylint:disable=undefined-variable
        raise OCPIException("Invalid noun provided: " + str(noun))
        # pylint:enable=undefined-variable
    if noun == "libraries" and library is None:
        if name and name != ".":
            asset = name


    # If the asset-type/noun is a collection-type, then we already know its location.
    #     E.g. If the noun is 'library' (a collection-type) and the name is 'dsp_comps',
    #          then by now we have obtained asset='components/dsp_comps'. Just return that.
    # Otherwise append 'name' to the determined collection-type asset.
    #     E.g. If library directives describe a collection-type/asset named 'components/dsp_comps',
    #          and the noun is 'worker' with name 'complex_mixer.hdl', then this will result in
    #          'components/dsp_comps/complex_mixer.hdl
    if noun in COLLECTION_DIRTYPES or not name:
        return asset
    else:
        return asset + "/" + name
# pylint:disable=too-many-statements
# pylint:disable=too-many-branches

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
