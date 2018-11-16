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

from .abstract import *
from .registry import *
from .factory import *
from .library import *
import os
import sys
import logging
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util
import json

# TODO: Should also extend CreatableAsset, ShowableAsset
class Project(RunnableAsset, RCCBuildableAsset, HDLBuildableAsset, ShowableAsset, ReportableAsset):
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
            init_libs        (T/F) - Instructs the method whether to construct all library objects
                                     contained in the project
            init_apps        (T/F) - Instructs the method whether to construct all application
                                     objects contained in the project
            init_hdl_plats   (T/F) - Instructs the method whether to construct all HdlPlatformWorker
                                     objects contained in the project (at least those with a
                                     corresponding build platform listed in self.hdl_platforms)
            init_hdl_assembs (T/F) - Instructs the method whether to construct all
                                     HdlApplicationAssembly objects contained in the project
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
        # __is_exported is set to true in __init_package_id() if this is a non-source exported
        # project
        self.__is_exported = False
        # Determine the package-ID for this project and set self.package_id
        self.__init_package_id()

        # NOTE: imports link to registry is NOT initialized in this constructor.
        #       Neither is the __registry Registry object.
        if kwargs.get("init_libs", False):
            self.lib_list = []
            logging.debug("Project constructor creating Library Objects")
            for lib_directory in self.get_valid_libraries():
                self.lib_list.append(AssetFactory.factory("library", lib_directory, **kwargs))

        if kwargs.get("init_apps", False):
            self.apps_list = []
            logging.debug("Project constructor creating Applications Objects")
            for app_directory in self.get_valid_apps():
                self.apps_list.append(AssetFactory.factory("applications", app_directory, **kwargs))

        if kwargs.get("init_hdlplats", False):
            logging.debug("Project constructor creating HdlPlatformsCollection Object")
            plats_directory = self.directory + "/hdl/platforms"
            # If hdl/platforms exists for this project, construct the HdlPlatformsCollection
            # instance
            if os.path.exists(plats_directory):
                self.hdlplatforms = AssetFactory.factory("hdl-platforms", plats_directory,
                                                         **kwargs)
            else:
                self.hdlplatforms = None
        else:
            self.hdlplatforms = None

        if kwargs.get("init_hdlassembs", False):
            logging.debug("Project constructor creating HdlAssembliesCollection Object")
            assemb_directory = self.directory + "/hdl/assemblies"
            # If hdl/assemblies exists for this project, construct the HdlAssembliesCollection
            # instance
            if os.path.exists(assemb_directory):
                self.hdlassemblies = AssetFactory.factory("hdl-assemblies", assemb_directory,
                                                          **kwargs)
            else:
                self.hdlassemblies = None
        else:
            self.hdlassemblies = None

    def __eq__(self, other):
        """
        Two projects are equivalent iff their directories match
        """
        #TODO: do we need realpath too? remove the abs/realpaths if we instead call
        # them in the Asset constructor
        return other is not None and \
                os.path.realpath(self.directory) == os.path.realpath(other.directory)

    def delete(self, force=False):
        """
        Remove the project from the registry if it is registered anywhere and remove the project
        from disk
        """
        try:
            self.registry().remove(package_id=self.package_id)
        except ocpiutil.OCPIException:
            # do nothing it's ok if the unregistering fails
            pass
        super().delete(force)

    def __init_package_id(self):
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
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectpackage ShellProjectVars=1",
                                                       verbose=True)
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

    def get_valid_libraries(self):
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

    def show_tests(self, **kwargs):
        if self.lib_list is None:
            raise ocpiutil.OCPIException("For a Project to show tests \"init_libs\" "
                                         "must be set to True when the object is constructed")

        details = kwargs.get("details", "")
        if details == "simple":
            for lib in self.lib_list:
                valid_tests = lib.get_valid_tests()
                if  valid_tests:
                    print("Library: " + lib.directory)
                for test_dir in valid_tests:
                    print("    Test: " + os.path.basename(test_dir.rstrip('/')))
        elif details == "table":
            row_1 = ["Library Directory", "Test"]
            row_2 = ["----------------------------------------", "-------------------"]
            rows = [row_1, row_2]
            for lib in self.lib_list:
                valid_tests = lib.get_valid_tests()
                for test_dir in valid_tests:
                    row_n = [lib.directory, os.path.basename(test_dir.rstrip('/'))]
                    rows.append(row_n)
            ocpiutil.print_table(rows)
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
                    lib_package = lib.package_id
                    lib_dict["package"] = lib_package
                    lib_dict["directory"] = lib.directory
                    lib_dict["tests"] = {os.path.basename(test.rstrip('/')):test
                                         for test in valid_tests}
                    libraries_dict[lib_package] = lib_dict
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectdeps ShellProjectVars=1",
                                                       verbose=True)
            project_dict["dependencies"] = project_vars['ProjectDependencies']
            project_dict["directory"] = self.directory
            project_dict["libraries"] = libraries_dict
            project_dict["package"] = self.package_id
            json_dict["project"] = project_dict
            json.dump(json_dict, sys.stdout)
            print()

    def show_libraries(self, **kwargs):
        details = kwargs.get("details", "")
        if details == "simple":
            for lib_directory in self.get_valid_libraries():
                print("Library: " + lib_directory)
        elif details == "table":
            row_1 = ["Library Directories"]
            row_2 = ["----------------------------------------"]
            rows = [row_1, row_2]
            for lib_directory in self.get_valid_libraries():
                row_n = [lib_directory]
                rows.append(row_n)
            ocpiutil.print_table(rows)
        else:
            json_dict = {}
            project_dict = {}
            libraries_dict = {}
            for lib_directory in self.get_valid_libraries():
                lib_dict = {}
                lib_package = Library.get_package_id(lib_directory)
                lib_dict["package"] = lib_package
                lib_dict["directory"] = lib_directory
                libraries_dict[lib_package] = lib_dict
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectdeps ShellProjectVars=1",
                                                       verbose=True)
            project_dict["dependencies"] = project_vars['ProjectDependencies']
            project_dict["directory"] = self.directory
            project_dict["libraries"] = libraries_dict
            project_dict["package"] = self.package_id
            json_dict["project"] = project_dict
            json.dump(json_dict, sys.stdout)
            print()

    def print_project_table(self):
        project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                   mk_arg="projectdeps ShellProjectVars=1",
                                                   verbose=True)

        row_1 = ["Project Directory", "Package-ID", "Project Dependencies"]
        row_2 = ["---------------------------", "---------------", "---------------------"]
        row_3 = [self.directory, self.package_id, ", ".join(project_vars['ProjectDependencies'])]
        rows = [row_1, row_2, row_3]
        ocpiutil.print_table(rows)

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
        details = kwargs.get("details", "")
        if kwargs.get("tests", False):
            self.show_tests(**kwargs)
        elif kwargs.get("libraries", False):
            self.show_libraries(**kwargs)
        elif not kwargs.get("verbose", False):
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectdeps ShellProjectVars=1",
                                                       verbose=True)
            if details == "simple":
                print("Project Directory: " + self.directory)
                print("Package-ID: " + self.package_id)
                print("Project Dependencies: " + ", ".join(project_vars['ProjectDependencies']))
            elif details == "table":
                self.print_project_table()
            else:
                json_dict = {'project': {'directory': self.directory,
                                         'package': self.package_id,
                                         'dependencies': project_vars['ProjectDependencies']}}
                json.dump(json_dict, sys.stdout)
                print()
        else:
            if self.lib_list is None:
                raise ocpiutil.OCPIException("For a Project to show verbosely \"init_libs\" "
                                             "must be set to True when the object is constructed")
            if details == "simple":
                project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                           mk_arg="projectdeps ShellProjectVars=1",
                                                           verbose=True)
                print("Project Directory: " + self.directory)
                print("Package-ID: " + self.package_id)
                print("Project Dependencies: " + ", ".join(project_vars['ProjectDependencies']))
                # this will likely change when we have more to show under libraries but for now
                # all we have is tests
                prim_dir = self.directory + "/hdl/primitives"
                if os.path.isdir(prim_dir):
                    print("  HDL Primitives: " + prim_dir)
                    prims = [dir for dir in os.listdir(prim_dir)
                             if not os.path.isfile(os.path.join(prim_dir, dir))]
                    prims = [x for x in prims if x != "lib"]
                    for prim in prims:
                        print("    Primitive: " + prim)
                for lib in self.lib_list:
                    print("  Library: " + lib.directory)
                    valid_tests = lib.get_valid_tests()
                    for test_dir in valid_tests:
                        print("    Test: " + os.path.basename(test_dir.rstrip('/')))
            elif details == "table":
                print("Overview:")
                self.print_project_table()
                print("Libraries:")
                self.show_libraries(details="table")
                print("Tests:")
                self.show_tests(details="table")
                print("Primitives:")
                row_1 = ["Primitive Directory", "Primitive"]
                row_2 = ["---------------------------", "---------------"]
                rows = [row_1, row_2]
                prim_dir = self.directory + "/hdl/primitives"
                if os.path.isdir(prim_dir):
                    prims = [dir for dir in os.listdir(prim_dir)
                             if not os.path.isfile(os.path.join(prim_dir, dir))]
                    prims = [x for x in prims if x != "lib"]
                    for prim in prims:
                        row_n = [prim_dir, prim]
                        rows.append(row_n)
                    ocpiutil.print_table(rows)
            else:
                json_dict = {}
                project_dict = {}
                libraries_dict = {}
                for lib in self.lib_list:
                    lib_dict = {}
                    lib_package = lib.package_id
                    lib_dict["package"] = lib_package
                    lib_dict["directory"] = lib.directory
                    valid_tests = lib.get_valid_tests()
                    if  valid_tests:
                        lib_dict["tests"] = {os.path.basename(test.rstrip('/')):test
                                             for test in valid_tests}
                    libraries_dict[lib_package] = lib_dict
                prim_dir = self.directory + "/hdl/primitives"
                if os.path.isdir(prim_dir):
                    prims_dict = {}
                    prims = [dir for dir in os.listdir(prim_dir)
                             if not os.path.isfile(os.path.join(prim_dir, dir))]
                    prims = [x for x in prims if x != "lib"]
                    prim_vars = ocpiutil.set_vars_from_make(mk_file=prim_dir + "/Makefile",
                                                            mk_arg="ShellTestVars=1 showpackage")
                    prim_pkg = "".join(prim_vars['Package'])
                    for prim in prims:
                        prims_dict[prim] = prim_dir + "/" + prim
                    project_dict["primitives"] = prims_dict
                project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                           mk_arg="projectdeps ShellProjectVars=1",
                                                           verbose=True)
                project_dict["dependencies"] = project_vars['ProjectDependencies']
                project_dict["directory"] = self.directory
                project_dict["libraries"] = libraries_dict
                project_dict["package"] = self.package_id
                json_dict["project"] = project_dict
                json.dump(json_dict, sys.stdout)
                print()

    def show_utilization(self):
        """
        Show utilization separately for each library in this project and for all assemblies
        """
        for library in self.lib_list:
            library.show_utilization()

        # if this project has an hdl/platforms dir, show its utilization
        if self.hdlplatforms:
            self.hdlplatforms.show_utilization()

        # if this project has an hdl/assemblies dir, show its utilization
        if self.hdlassemblies:
            self.hdlassemblies.show_utilization()

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
            import _opencpi.assets.registry
            # Get the default project registry set by the environment state
            self.set_registry(_opencpi.assets.registry.Registry.get_default_registry_dir())

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
            import _opencpi.assets.registry
            # Get the default project registry set by the environment state
            registry_path = _opencpi.assets.registry.Registry.get_default_registry_dir()
        reg = self.registry()
        # If we are about to set a new registry, and this project is registered in the current one,
        # raise an exception; user needs to unregister the project first.
        # pylint:disable=bad-continuation
        if ((os.path.realpath(reg.directory) != os.path.realpath(registry_path)) and
            reg.contains(directory=self.directory)):
            raise ocpiutil.OCPIException("Before (un)setting the registry for the project at \"" +
                                         self.directory + "\", you must unregister the project.\n" +
                                         "This can be done by running 'ocpidev unregister project" +
                                         " " + self.package_id + "'.")
        # pylint:enable=bad-continuation

        ocpiutil.logging.warning("To use this registry, run the following command and add it " +
                                 "to your ~/.bashrc:\nexport OCPI_PROJECT_REGISTRY_DIR=" +
                                 os.path.realpath(registry_path))

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
                if os.path.realpath(actual_registry_path) == os.path.realpath(imports_link):
                    # Imports already point to this registry
                    return
                else:
                    self.unset_registry()
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

    def unset_registry(self):
        """
        Unset the project registry link for this project.
        I.e. remove the 'imports' link at the top-level of the project.
        """
        # If the 'imports' link exists at the project-top, and it is a link, remove it.
        # If it is not a link, let the user remove it manually.
        reg = self.registry()
        # If we are about to unset the, but this project is registered in the current one,
        # raise an exception; user needs to unregister the project first.
        if reg.contains(directory=self.directory):
            raise ocpiutil.OCPIException("Before (un)setting the registry for the project at \"" +
                                         self.directory + "\", you must unregister the project.\n" +
                                         "This can be done by running 'ocpidev unregister project" +
                                         " " + self.package_id + "'.")

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
            import _opencpi.assets.registry
            self.__registry = AssetFactory.factory("registry",
                              _opencpi.assets.registry.Registry.get_registry_dir(self.directory))
            if self.__registry is None:
                raise ocpiutil.OCPIException("The registry for the current project ('" +
                                             self.directory + "') could not be determined.")
        return self.__registry
