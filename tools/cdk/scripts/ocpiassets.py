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
This module a collection of OpenCPI Asset classes

The classes in this module are used for OpenCPI project structure management

Documentation and testing:
    Documentation can be viewed by running:
        $ pydoc ocpiassets
Note on testing:
    When adding functions to this file, add unit tests to
    opencpi/tests/pytests/*_test.py
"""
from abc import ABCMeta, abstractmethod
import os
import logging
import copy
from functools import partial
from glob import glob
import ocpiutil
import re
import json
import sys

class AssetFactory():
    """
    This class is used for intelligent construction of supported OpenCPI Assets. Given an asset type
    and arguments, the factory will provide an instance of the relevant class.
    """
    # __assets is a dictionary to keep track of existing instances for each asset subclass. Only
    # assets that use the __get_or_create function for construction will be tracked in this dict.
    # It can be used to determine whether an instance needs to be created or already exists.
    # {
    #     <asset-subclass> : {
    #                            <directory> : <asset-subclass-instance>
    #                        }
    # }
    __assets = {}

    @classmethod
    def factory(cls, asset_type, directory, name=None, **kwargs):
        """
        Class method that is the intended wrapper to create all instances of any Asset subclass.
        Returns a constructed object of the type specified by asset_type. Throws an exception for
        an invalid type.

        Every asset must have a directory, and may provide a name and other args.
        Some assets will be created via the corresponding subclass constructor.
        Others will use an auxiliary function to first check if a matching instance
        already exists.
        """
        # actions maps asset_type string to the function that creates objects of that type
        # Some types will use plain constructors,
        # and some will use __get_or_create with asset_cls set accordingly
        actions = {"test":         Test,
                   "application":  Application,
                   "applications": ApplicationsCollection,
                   "library":      Library,
                   "project":      partial(cls.__get_or_create, Project),
                   "registry":     partial(cls.__get_or_create, Registry)}

        if asset_type not in actions.keys():
            raise ocpiutil.OCPIException("Bad asset creation, \"" + asset_type + "\" not supported")

        # Call the action for this type and hand it the arguments provided
        return actions[asset_type](directory, name, **kwargs)

    @classmethod
    def remove(cls, directory=None, instance=None):
        """
        Removes an instance from the static class variable __assets by dierectory or the instance
        itself.  Throws an exception if neither optional argument is provided.
        """
        if directory is not None:
            real_dir = os.path.realpath(directory)
            dirtype_dict = {"project": Project,
                            "registry": Registry}
            dirtype = ocpiutil.get_dirtype(real_dir)
            cls.__assets[dirtype_dict[dirtype]].pop(real_dir, None)
        elif instance is not None:
            cls.__assets[instance.__class__] = {
                k:v for k, v in cls.__assets[instance.__class__].items() if v is not instance}
        else:
             raise ocpiutil.OCPIException("Invalid use of AssetFactory.remove() both directory " +
                                          "and instance are None.")

    @classmethod
    def __get_or_create(cls, asset_cls, directory, name, **kwargs):
        """
        Given an asset subclass type, check whether an instance of the
        subclass already exists for the provided directory. If so, return
        that instance. Otherwise, call the subclass constructor and return
        the new instance.
        """
        # Determine the sub-dictionary in __assets corresponding to the provided class (asset_cls)
        if asset_cls not in cls.__assets:
            cls.__assets[asset_cls] = {}
        asset_inst_dict = cls.__assets[asset_cls]

        # Does an instance of the asset_cls subclass exist with a directory matching the provided
        # "dictionary" parameter? If so, just return that instance.
        real_dir = os.path.realpath(directory)
        if real_dir not in asset_inst_dict:
            # If not, construct a new one, add it to the dictionary, and return it
            asset_inst_dict[real_dir] = asset_cls(directory, name, **kwargs)
        return asset_inst_dict[real_dir]

class Asset(metaclass=ABCMeta):
    #TODO add project top and package id as a variable here, maybe this becomes a method instead
    """
    Parent Class for all Asset objects.  Contains a factory to create each of the asset types.
    Not officially a virtual class but objects of this class are not intended to be directly
    created.
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        initializes Asset member data valid kwargs handled at this level are:
        verbose (T/F) - be verbose with output
        name - Optional argument that specifies the name of the asset if not set defaults to the
               basename of the directory argument
        directory - The location on the file system of the asset that is being constructed.
                    both relative and global file paths are valid.
        """
        if not name:
            self.name = os.path.basename(directory)
        else:
            self.name = name
        self.directory = os.path.realpath(directory)
        self.verbose = kwargs.get("verbose", False)

    @classmethod
    def get_valid_settings(cls):
        """
        Recursive class method that gathers all the valid settings static lists of the current
        class's base classes and combines them into a single set to return to the caller
        """
        ret_val = cls.valid_settings

        for base_class in cls.__bases__:
            # prevents you from continuing up the class hierarchy to "object"
            if callable(getattr(base_class, "get_valid_settings", None)):
                ret_val += base_class.get_valid_settings()

        return set(ret_val)

    def get_settings(self):
        """
        Generic method that returns a dictionary of settings associated with a single run or build
        of an object.  valid settings are set at the subclass level and any member variable that
        is not in this list or is not set(equal to None) are removed from the dictionary
        """
        settings_list = copy.deepcopy(vars(self))
        # list constructor is required here because the original arg_list is being
        # changed and we can't change a variable we are iterating over
        for setting, value in list(settings_list.items()):
            if (value in [None, False]) or (setting not in self.__class__.valid_settings):
                del settings_list[setting]

        return settings_list

    @staticmethod
    def check_dirtype(dirtype, directory):
        """
        checks that the directory passed in is of the type passed in and if not an exception is
        thrown
        """
        if not os.path.isdir(directory):
            raise ocpiutil.OCPIException("Tried creating a " + dirtype + " in a directory that " +
                                         "doesn't exist. " + directory)

        if ocpiutil.get_dirtype(directory) != dirtype:
            raise ocpiutil.OCPIException("Tried creating a " + dirtype + " in invalid directory " +
                                         "type: " +  str(ocpiutil.get_dirtype(directory)) +
                                         " in directory: " + directory)

class BuildableAsset(Asset):
    """
    Virtual class that requires that any child classes implement a build method.  Contains settings
    that are specific to all assets that can be run
    """
    valid_settings = ["only_plats", "ex_plats"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes BuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            ex_plats (list) - list of platforms(strings) to exclude from a build
            only_plats (list) - list of the only platforms(strings) to build for
        """
        super().__init__(directory, name, **kwargs)
        self.ex_plats = kwargs.get("ex_plats", None)
        self.only_plats = kwargs.get("only_plats", None)

    @abstractmethod
    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("BuildableAsset.build() is not implemented")

class HDLBuildableAsset(BuildableAsset):
    """
    Virtual class that requires that any child classes implement a build method.  Contains settings
    that are specific to all assets that can be run
    """
    valid_settings = ["hdl_plats"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HDLBuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            hdl_plats (list) - list of hdl platforms(strings) to build for
        """
        super().__init__(directory, name, **kwargs)
        self.hdl_plats = kwargs.get("hdl_plats", None)

    @abstractmethod
    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("BuildableAsset.build() is not implemented")

class RCCBuildableAsset(BuildableAsset):
    """
    Virtual class that requires that any child classes implement a build method.  Contains settings
    that are specific to all assets that can be run
    """
    valid_settings = ["rcc_plats"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HDLBuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            rcc_plats (list) - list of rcc platforms(strings) to build for
        """
        super().__init__(directory, name, **kwargs)
        self.rcc_plats = kwargs.get("rcc_plats", None)

    @abstractmethod
    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("BuildableAsset.build() is not implemented")

class RunnableAsset(Asset):
    """
    Virtual class that requires that any child classes implement a run method.
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes RunnableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            None
        """
        super().__init__(directory, name, **kwargs)

    @abstractmethod
    def run(self):
        """
        This function will run the asset must be implemented by the child class
        """
        raise NotImplementedError("RunnableAsset.run() is not implemented")


class ShowableAsset(Asset):
    """
    Virtual class that requires that any child classes implement a show function
    """
    @abstractmethod
    def show(self, **kwargs):
        """
        This function will show this asset must be implemented by the child class
        """
        raise NotImplementedError("ShowableAsset.show() is not implemented")

    '''
    @classmethod
    @abstractmethod
    def showall(cls, options, only_registry):
        """
        This function will show all assets of this type, must be implemented by the child class
        """
        raise NotImplementedError("ShowableAsset.showall() is not implemented")
    '''

class Test(RunnableAsset, HDLBuildableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI Component Unit test.  Contains build/run settings that are
    specific to Tests.
    """
    valid_settings = ["keep_sims", "errors", "cases", "verbose", "remote_test_sys"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Test member data  and calls the super class __init__.  Throws an exception if
        the directory passed in is not a valid test directory.
        valid kwargs handled at this level are:
            keep_sims (T/F) - Keep HDL simulation files for any simulation platforms
            acc_errors (T/F) - Causes errors to accumulate and tests to continue on
            cases (list) - Specify Which test cases that will be run/verified
            mode (list) - Specify which phases of the unit test to run
            remote_test_sys (list) - Specify remote systems to run the test(s)
        """
        self.check_dirtype("test", directory)
        super().__init__(directory, name, **kwargs)

        self.keep_sims = kwargs.get("keep_sims", False)
        self.acc_errors = kwargs.get("acc_errors", False)
        self.cases = kwargs.get("cases", None)
        self.mode = kwargs.get("mode", "all")
        self.remote_test_sys = kwargs.get("remote_test_sys", None)

        # using all instead of build so that old style unit tests wont blow up
        # all and build will evaluate to the same make target
        self.mode_dict = {}
        # pylint:disable=bad-whitespace
        self.mode_dict['gen_build']       = ["all"]
        self.mode_dict['prep_run_verify'] = ["run"]
        self.mode_dict['clean_all']       = ["clean"]
        self.mode_dict['prep']            = ["prepare"]
        self.mode_dict['run']             = ["runnoprepare"]
        self.mode_dict['prep_run']        = ["runonly"]
        self.mode_dict['verify']          = ["verify"]
        self.mode_dict['view']            = ["view"]
        self.mode_dict['clean_run']       = ["cleanrun"]
        self.mode_dict['clean_sim']       = ["cleansim"]
        self.mode_dict['all']             = ["all", "run"]
        # pylint:enable=bad-whitespace

    def run(self):
        """
        Runs the Test with the settings specified in the object
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory,
                                    self.mode_dict[self.mode])

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Test.build() is not implemented")

class Application(RunnableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI ACI Application.
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Application member data  and calls the super class __init__.  Throws an
        exception if the directory passed in is not a valid application directory.
        valid kwargs handled at this level are:
            None
        """
        self.check_dirtype("application", directory)
        super().__init__(directory, name, **kwargs)

    def run(self):
        """
        Runs the Application with the settings specified in the object
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory, ["run"])

    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Application.build() is not implemented")

class ApplicationsCollection(RunnableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI applications directory.  Ability act on multiple applications
    with a single instance are located in this class.
    """
    valid_settings = ["run_before", "run_after", "run_args"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Application member data  and calls the super class __init__.  Throws an
        exception if the directory passed in is not a valid applications directory.
        valid kwargs handled at this level are:
            run_before (list) - Arguments to insert before the ACI executable or ocpirun
            run_after (list) - Arguments to insert at the end of the execution command line A
            run_args (list) - Arguments to insert immediately after the ACI executable or ocpirun
        """
        self.check_dirtype("applications", directory)
        super().__init__(directory, name, **kwargs)

        self.run_before = kwargs.get("run_before", None)
        self.run_after = kwargs.get("run_after", None)
        self.run_args = kwargs.get("run_args", None)

    def run(self):
        """
        Runs the ApplicationsCollection with the settings specified in the object.  Running a
        ApplicationCollection will run all the applications that are contained in the
        ApplicationCollection
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory, ["run"])

    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("ApplicationCollection.build() is not implemented")

class Library(RunnableAsset, RCCBuildableAsset, HDLBuildableAsset):
    """
    This class represents an OpenCPI Library.  Contains a list of the tests that are in this
    library and can be initialized or left as None if not needed
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Library member data  and calls the super class __init__.  Throws an
        exception if the directory passed in is not a valid library directory.
        valid kwargs handled at this level are:
            init_tests (T/F) - Instructs the method weather to construct all test objects contained
                               in the library
        """
        self.check_dirtype("library", directory)
        super().__init__(directory, name, **kwargs)
        self.test_list = None
        if kwargs.get("init_tests", False):
            self.test_list = []
            logging.debug("Library constructor creating Library Objects")
            for test_directory in self.get_valid_tests():
                self.test_list.append(AssetFactory.factory("test", test_directory, **kwargs))

    def get_valid_tests(self):
        """
        Probes make in order to determine the list of active tests in the library
        """
        ret_val = []
        ocpiutil.logging.debug("getting valid tests from: " + self.directory + "/Makefile")
        make_tests = ocpiutil.set_vars_from_make(self.directory + "/Makefile",
                                                 "ShellTestVars=1 showtests", True)["Tests"]

        for name in make_tests:
            ret_val.append(self.directory + "/" + name)
        return ret_val

    def run(self):
        """
        Runs the Library with the settings specified in the object.  Throws an exception if the
        tests were not initialized by using the init_tests variable at initialization.  Running a
        Library will run all the component unit tests that are contained in the Library
        """
        ret_val = 0
        if self.test_list is None:
            raise ocpiutil.OCPIException("For a Library to be run \"init_tests\" must be set to " +
                                         "True when the object is constructed")
        for test in self.test_list:
            run_val = test.run()
            ret_val = ret_val + run_val
        return ret_val

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Library.build() is not implemented")

# TODO: Should also extend CreatableAsset, ShowableAsset
class Registry(Asset):
    """
    The Registry class represents an OpenCPI project registry. As an OpenCPI
    registry contains project-package-ID named symlinks to project directories,
    registry instances contain dictionaries mapping package-ID to project instances.
    Projects can be added or removed from a registry
    """

    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)

        # Each registry instance has a list of projects registered within it.
        # Initialize this list by probing the filesystem for links that exist
        # in the registry dir.
        # __projects maps package-ID --> project instance
        self.__projects = {}
        for proj in glob(self.directory + '/*'):
            pid = os.path.basename(proj)
            self.__projects[pid] = AssetFactory.factory("project", proj) if os.path.exists(proj) \
                                   else None

    def add(self, directory="."):
        """
        Given a project, get its package-ID, create the corresponding link in the project registry,
        and add it to this Registry instance's __projects dictionary.
        If a project with the same package-ID already exists in the registry, fail.
        """
        if not ocpiutil.is_path_in_project(directory):
            raise ocpiutil.OCPIException("Failure to register project. Directory \"" + directory +
                                         "\" is not in a project.")

        project = AssetFactory.factory("project", directory)
        pid = project.package_id

        if pid == "local":
            raise ocpiutil.OCPIException("Failure to register project. Cannot register a " +
                                         "project with package-ID 'local'.\nSet the  " +
                                         "PackageName, PackagePrefix and/or Package " +
                                         "variables in your Project.mk.")
        if pid in self.__projects and self.__projects[pid] is not None:
            # If the project is already registered and is the same
            if self.__projects[pid].directory == project.directory:
                logging.debug("Project link is already in the registry. Proceeding...")
                return
            raise ocpiutil.OCPIException("Failure to register project with package '" + pid +
                                         "'.\nA project/link with that package qualifier " +
                                         "already exists and is registered in '" + self.directory +
                                         "'.\nTo unregister that project, call: 'ocpidev " +
                                         "unregister project " + pid +"'.\nThen, rerun the " +
                                         "command: 'ocpidev -d " + project.directory +
                                         " register project'")

        # link will be created at <registry>/<package-ID>
        project_link = self.directory + "/" + pid

        # if this statement is reached and the link exists, it is a broken link
        if os.path.lexists(project_link):
            # remove the broken link that would conflict
            self.remove_link(pid)

        # Perform the actual registration: create the symlink to the project in this registry dir
        self.create_link(project)
        # Add the project to this registry's projects dictionary
        self.__projects[project.package_id] = project

    def remove(self, package_id=None, directory=None):
        """
        Given a project's package-ID or directory, determine if the project is present
        in this registry. If so, remove it from this registry's __projects dictionary
        and remove the registered symlink.
        """
        if package_id is None:
            package_id = ocpiutil.get_project_package(directory)
            if package_id is None:
                raise ocpiutil.OCPIException("Could not unregister project located at \"" +
                                             directory + "\" because the project's package-ID " +
                                             "could not be determined.\nIs it really a project?")

        if package_id not in self.__projects:
            raise ocpiutil.OCPIException("Could not unregister project with package-ID \"" +
                                         package_id + "\" because the project is not in the " +
                                         "registry.\n Run 'ocpidev show registry --table' for " +
                                         "information about the currently registered projects.")

        project_link = self.__projects[package_id].directory
        if directory is not None and os.path.realpath(directory) != project_link:
            raise ocpiutil.OCPIException("Failure to unregister project with package '" +
                                         package_id + "'.\nThe registered project with link '" +
                                         package_id + " --> " + project_link + "' does not " +
                                         "point to the specified project '" +
                                         os.path.realpath(directory) + "'." + "\nThis project " +
                                         "does not appear to be registered.")

        # Remove the symlink registry/package-ID --> project
        self.remove_link(package_id)
        # Remove the project from this registry's dict
        self.__projects.pop(package_id)

    def create_link(self, project):
        """
        Create a link to the provided project in this registry
        """
        # Try to make the path relative. This helps with environments involving mounted directories
        # Find the path that is common to the project and registry
        common_prefix = os.path.commonprefix([project.directory, self.directory])
        # If the two paths contain no common directory except root,
        #     use the path as-is
        # Otherwise, use the relative path from the registry to the project
        if common_prefix == '/' or common_prefix == '':
            project_to_reg = os.path.normpath(project.directory)
        else:
            project_to_reg = os.path.relpath(os.path.normpath(project.directory), self.directory)

        project_link = self.directory + "/" + project.package_id
        try:
            os.symlink(project_to_reg, project_link)
        except OSError:
            raise ocpiutil.OCPIException("Failure to register project link: " +  project_link +
                                         " --> " + project_to_reg + "\nCommand attempted: " +
                                         "'ln -s " + project_to_reg + " " + project_link +
                                         "'.\nTo (un)register projects in " +
                                         "/opt/opencpi/project-registry, you need to be a " +
                                         "member of the opencpi group.")

    def remove_link(self, package_id):
        """
        Remove link with name=package-ID from this registry
        """
        link_path = self.directory + "/" + package_id
        try:
            os.unlink(link_path)
        except OSError:
            raise ocpiutil.OCPIException("Failure to unregister link to project: " + package_id +
                                         " --> " + os.readlink(link_path) + "\nCommand " +
                                         "attempted: 'unlink " + link_path + "'\nTo " +
                                         "(un)register projects in " +
                                         "/opt/opencpi/project-registry, you need to be a " +
                                         "member of the opencpi group.")

    @staticmethod
    def get_default_registry_dir():
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

    @classmethod
    def get_registry_dir(cls, directory="."):
        """
        Determine the project registry directory. If in a project, check for the imports link.
        Otherwise, get the default registry from the environment setup:
            OCPI_PROJECT_REGISTRY_DIR, OCPI_CDK_DIR/../project-registry or
            /opt/opencpi/project-registry

        Determine whether the resulting path exists.

        Return the exists boolean and the path to the project registry directory.
        """
        if ocpiutil.is_path_in_project(directory) and \
           os.path.isdir(ocpiutil.get_path_to_project_top(directory) + "/imports"):
            # allow imports to be a link OR a dir (needed for deep copies of exported projects)
            project_registry_dir = os.path.realpath(ocpiutil.get_path_to_project_top(directory) +
                                                    "/imports")
        else:
            project_registry_dir = cls.get_default_registry_dir()

        exists = os.path.exists(project_registry_dir)
        if not exists:
            raise ocpiutil.OCPIException("The project registry directory '" + project_registry_dir +
                                         "' does not exist.\nCorrect " +
                                         "'OCPI_PROJECT_REGISTRY_DIR' or run: " +
                                         "'ocpidev create registry " + project_registry_dir + "'")
        elif not os.path.isdir(project_registry_dir):
            raise ocpiutil.OCPIException("The current project registry '" + project_registry_dir +
                                         "' exists but is not a directory.\nCorrect " +
                                         "'OCPI_PROJECT_REGISTRY_DIR'")
        return project_registry_dir


# TODO: Should also extend CreatableAsset, ShowableAsset
class Project(RunnableAsset, RCCBuildableAsset, HDLBuildableAsset, ShowableAsset):
    """
    The Project class represents an OpenCPI project. Only one Project class should
    exist per OpenCPI project. Projects can be built, run, registered, shown....
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Project member data  and calls the super class __init__.  Throws an
        exception if the directory passed in is not a valid project directory.
        valid kwargs handled at this level are:
            init_libs (T/F) - Instructs the method whether to construct all library objects
                             contained in the project
            init_apps (T/F) - Instructs the method whether to construct all application objects
                             contained in the project
        """
        self.check_dirtype("project", directory)
        super().__init__(directory, name, **kwargs)
        self.lib_list = None
        self.apps_list = None

        # Boolean for whether or the current directory is within this project
        # TODO: is current_project needed as a field, or can it be a function?
        #self.current_project = ocpiutil.get_path_to_project_top() == self.directory
        self.__registry = None

        # flag to determine if a project is exported
        # __is_exported is set to true in set_package_id() if this is a non-source exported project
        self.__is_exported = False
        # Determine the package-ID for this project and set self.package_id
        self.set_package_id()
        if not self.__is_exported:
            # make sure the imports link exists for this project
            self.initialize_registry_link()

        if kwargs.get("init_libs", False) is True:
            self.lib_list = []
            logging.debug("Project constructor creating Library Objects")
            for lib_directory in self.get_valid_libaries():
                self.lib_list.append(AssetFactory.factory("library", lib_directory,  **kwargs))

        if kwargs.get("init_apps", False) is True:
            self.apps_list = []
            logging.debug("Project constructor creating Applications Objects")
            for app_directory in self.get_valid_apps():
                self.apps_list.append(AssetFactory.factory("applications", app_directory, **kwargs))

    def __eq__(self, other):
        """
        Two projects are equivalent iff their directories match
        """
        #TODO: do we need realpath too? remove the abs/realpaths if we instead call
        # them in the Asset constructor
        return other is not None and \
                os.path.realpath(self.directory) == os.path.realpath(other.directory)
    def set_package_id(self):
        """
        Get the Package Name of the project containing 'self.directory'.
        """
        # From the project top, probe the Makefile for the projectpackage
        # which is printed in cdk/include/project.mk in the projectpackage rule
        # if ShellProjectVars is defined
        project_package = None
        # If the project-package-id file exists, set package-id to its contents
        if os.path.isfile(self.directory + "/project-package-id"):
            with open(self.directory + "/project-package-id", "r") as package_id_file:
                self.__is_exported = True
                project_package = package_id_file.read().strip()
                logging.debug("Read Project-ID '" + project_package + "' from file: " +
                              self.directory + "/project-package-id")

        # Otherwise, ask Makefile at the project top for the ProjectPackage
        if project_package is None or project_package == "":
            project_vars = ocpiutil.set_vars_from_make(self.directory + "/Makefile",
                                                       "projectpackage ShellProjectVars=1",
                                                       "verbose")
            if not project_vars is None and 'ProjectPackage' in project_vars and \
               len(project_vars['ProjectPackage']) > 0:
                # There is only one value associated with ProjectPackage, so get element 0
                project_package = project_vars['ProjectPackage'][0]
            else:
                raise ocpiutil.OCPIException("Could not determine Package-ID of project \"" +
                                             self.directory + "\".")
        self.package_id = project_package

    def get_valid_apps(self):
        """
        Gets a list of all directories of type applications in the project and puts that
        applications directory and the basename of that directory into a dictionary to return
        """
        return ocpiutil.get_subdirs_of_type("applications", self.directory)

    def get_valid_libaries(self):
        """
        Gets a list of all directories of type library in the project and puts that
        library directory and the basename of that directory into a dictionary to return
        """
        return ocpiutil.get_subdirs_of_type("library", self.directory)

    def run(self):
        """
        Runs the Project with the settings specified in the object Throws an exception if no
        applications or libraries are initialized using the init_apps or init_libs variables at
        initialization time
        """
        ret_val = 0
        if (self.apps_list is None) and (self.lib_list is None):
            raise ocpiutil.OCPIException("For a Project to be run \"init_libs\" and " +
                                         "\"init_tests\" or \"init_apps\" must be set to " +
                                         "True when the object is constructed")
        if self.apps_list is not None:
            for apps in self.apps_list:
                run_val = apps.run()
                ret_val = ret_val + run_val
        if self.lib_list is not None:
            for lib in self.lib_list:
                run_val = lib.run()
                ret_val = ret_val + run_val
        return ret_val
    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Project.build() is not implemented")

    def show(self, **kwargs):
        """
        This method prints out information about the project based on the options passed in as
        kwargs
        valid kwargs handled by this method are:
            json (T/F) - Instructs the method whether to output information in json format or
                         human readable format
            tests (T/F) - Instructs the method whether print out the tests that that exist in
                          this project
        """

        if kwargs.get("tests", False):
            if self.lib_list is None:
                raise ocpiutil.OCPIException("For a Project to show tests \"init_libs\" "
                                             "must be set to True when the object is constructed")

            if not kwargs.get("json", False):
                for lib in self.lib_list:
                    valid_tests = lib.get_valid_tests()
                    if  valid_tests:
                        print("Library: " + lib.directory)
                    for test_dir in valid_tests:
                        print("    Test: " + os.path.basename(test_dir.rstrip('/')))
            else:
                '''
                JSON format:
                {project:{
                  name: proj_name
                  directory: proj_directory
                  libraries:{
                    lib_name:{
                      name: lib_name
                      directory:lib_directory
                      tests:{
                        test_name : test_directory
                        ...
                      }
                    }
                  }
                }
                '''
                json_dict = {}
                project_dict = {}
                libraries_dict = {}
                for lib in self.lib_list:
                    valid_tests = lib.get_valid_tests()
                    if  valid_tests:
                        lib_dict = {}
                        lib_dict["name"] = lib.name
                        lib_dict["directory"] = lib.directory
                        lib_dict["tests"] = {os.path.basename(test.rstrip('/')):test
                                             for test in valid_tests}
                        libraries_dict[lib.name] = lib_dict

                project_dict["name"] = self.name
                project_dict["directory"] = self.directory
                project_dict["libraries"] = libraries_dict
                json_dict["project"] = project_dict
                json.dump(json_dict, sys.stdout)
                print()

    def initialize_registry_link(self):
        """
        If the imports link for the project does not exist, set it to the default project registry.
        Basically, make sure the imports link exists for this project.
        """
        imports_link = self.directory + "/imports"
        if os.path.exists(imports_link):
            logging.info("Imports link exists for project " + self.directory +
                         ". No registry initialization needed")
        else:
            # Get the default project registry set by the environment state
            self.set_registry(Registry.get_default_registry_dir())

    def set_registry(self, registry_path=None):
        """
        Set the project registry link for this project. If a registry path is provided,
        set the link to that path. Otherwise, set it to the default registry based on the
        current environment.
        I.e. Create the 'imports' link at the top-level of the project to point to the project
             registry
        """
        # If registry path is not provided, get the default
        if registry_path is None or registry_path == "":
            # Get the default project registry set by the environment state
            registry_path = Registry.get_default_registry_dir()

        #self.__registry = AssetFactory.factory("registry", registry_path)
        # TODO: pull this relative link functionality into a helper function
        # Try to make the path relative. This helps with environments involving mounted directories
        # Find the path that is common to the registry and project-top
        common_prefix = os.path.commonprefix([os.path.normpath(registry_path), self.directory])
        # If the two paths contain no common directory except root,
        #     use the path as-is
        # Otherwise, use the relative path from project to registry_path
        # actual_registry_path is the actual path that can be checked for existence of the registry
        #     it is either an absolute path or a relative path
        if common_prefix == '/' or common_prefix == '':
            rel_registry_path = os.path.normpath(registry_path)
            actual_registry_path = rel_registry_path
        else:
            rel_registry_path = os.path.relpath(os.path.normpath(registry_path), self.directory)
            actual_registry_path = self.directory + "/" + rel_registry_path
        # Registry must exist and must be a directory
        if os.path.isdir(actual_registry_path):
            imports_link = self.directory + "/imports"
            # If it exists and IS NOT a link, tell the user to manually remove it.
            # If 'imports' exists and is a link, remove the link.
            if os.path.exists(imports_link) or os.path.islink(imports_link):
                if not os.path.islink(imports_link):
                    raise ocpiutil.OCPIException("The 'imports' for the current project ('" +
                                                 imports_link + "') is not a symbolic link. " +
                                                 "\nMove or remove this file and retry setting " +
                                                 "the project registry.")
                elif os.path.realpath(actual_registry_path) == os.path.realpath(imports_link):
                    # Imports already point to this registry
                    return
                else:
                    os.unlink(imports_link)
            # ln -s registry_path imports_link
            try:
                os.symlink(rel_registry_path, imports_link)
            except OSError:
                # Symlink creation failed....
                # User probably does not have write permissions in the project
                raise ocpiutil.OCPIException("Failure to set project link: " + imports_link +
                                             " --> " + rel_registry_path + "\nCommand " +
                                             "attempted: " + "'ln -s " + rel_registry_path + " " +
                                             imports_link + "'.\nMake sure you have correct "+
                                             "permissions in this project.")
        else:
            raise ocpiutil.OCPIException("Failure to set project registry to: '" + registry_path +
                                         "'.  Tried to use relative path: " + rel_registry_path +
                                         " in project: " + self.directory + "'.\nPath does not " +
                                         "exist or is not a directory.")
        if not os.path.isdir(registry_path + "/ocpi.cdk"):
            logging.warning("There is no CDK registered in '" + registry_path + "'. Make sure to " +
                            "register the CDK before moving on.\nNext time, consider using " +
                            "'ocpidev create registry', which will automatically register your " +
                            "CDK.")

    def unset_registry(self):
        """
        Unset the project registry link for this project.
        I.e. remove the 'imports' link at the top-level of the project.
        """
        # If the 'imports' link exists at the project-top, and it is a link, remove it.
        # If it is not a link, let the user remove it manually.
        try:
            reg = self.registry()
            reg.remove(directory=self.directory)
            logging.warning("Removed project \"" + self.directory + "\" from its" +
                            "registry located at \"" + reg.directory + "\" before unbinding " +
                            " it from that registry.")
        except ocpiutil.OCPIException:
            pass

        imports_link = self.directory + "/imports"
        if os.path.islink(imports_link):
            os.unlink(imports_link)
            # Set this project's registry reference to the default
            self.__registry = None
        else:
            if os.path.exists(imports_link):
                raise ocpiutil.OCPIException("The 'imports' for the current project ('" +
                                             imports_link + "') is not a symbolic link.\nThe " +
                                             "file will need to be removed manually.")
            else:
                logging.debug("Unset project registry has succeeded, but nothing was done.\n" +
                              "Registry was not set in the first place for this project.")

    def registry(self):
        """
        This function will return the registry object for this Project instance.
        If the registry is None, it will try to find/construct it first
        """
        if self.__registry is None:
            self.__registry = AssetFactory.factory("registry",
                                                   Registry.get_registry_dir(self.directory))
            if self.__registry is None:
                raise ocpiutil.OCPIException("The registry for the current project ('" +
                                             self.directory + "') could not be determined.")
        return self.__registry
