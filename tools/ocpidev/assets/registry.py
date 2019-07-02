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
Defintion of Registery class
"""

import os
import sys
import json
from glob import glob
import logging
import _opencpi.util as ocpiutil
from .abstract import ShowableAsset
from .factory import AssetFactory

# TODO: Should also extend CreatableAsset, ShowableAsset
class Registry(ShowableAsset):
    """
    The Registry class represents an OpenCPI project registry. As an OpenCPI
    registry contains project-package-ID named symlinks to project directories,
    registry instances contain dictionaries mapping package-ID to project instances.
    Projects can be added or removed from a registry
    """

    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)

        # Each registry instance has a list of projects registered within it.
        # Initialize this list by probing the file-system for links that exist
        # in the registry directory.
        # __projects maps package-ID --> project instance
        self.__projects = {}
        for proj in glob(self.directory + '/*'):
            pid = os.path.basename(proj)
            if os.path.exists(proj):
                self.__projects[pid] = AssetFactory.factory("project", proj, **kwargs)

    def contains(self, package_id=None, directory=None):
        """
        Given a project's package-ID or directory, determine if the project is present
        in this registry.
        """
        # If neither package_id nor directory are provided, exception
        if package_id is None:
            if directory is None:
                raise ocpiutil.OCPIException("Could determine whether project exists because " +
                                             "the package_id and directory provided were both " +
                                             "None.\nProvide one or both of these arguments.")
            # Project's package-ID is determined from its directory if not provided
            package_id = ocpiutil.get_project_package(directory)
            if package_id is None:
                raise ocpiutil.OCPIException("Could not determine package-ID of project located " +
                                             "at \"" + directory + "\".\nDouble check this " +
                                             "configurations.")

        # If the project is registered here by package-ID, return True.
        # If not, or if a different project is registered here with that package-ID, return False
        if package_id in self.__projects:
            # pylint:disable=bad-continuation
            if (directory is not None and
                os.path.realpath(directory) != self.__projects[package_id].directory):
                logging.warning("Registry at \"" + self.directory + "\" contains a project with " +
                                "package-ID \"" + package_id + "\", but it is not the same " +
                                "project as \"" + directory + "\".")
                return False
            # pylint:enable=bad-continuation
            return True
        return False

    def add(self, directory=".", force=False):
        """
        Given a project, get its package-ID, create the corresponding link in the project registry,
        and add it to this Registry instance's __projects dictionary.
        If a project with the same package-ID already exists in the registry, fail.
        """
        if not ocpiutil.is_path_in_project(directory):
            raise ocpiutil.OCPIException("Failure to register project.  Project \"" + directory +
                                         "\" in location: " + os.getcwd() + " is not in a " +
                                         "project or does not exist.")

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
                print("Successfully registered the " + project.package_id + " project: " +
                      os.path.realpath(project.directory) + "\nInto the registry: " +
                      os.path.realpath(self.directory) + "\n")
                return
            if not force:
                raise ocpiutil.OCPIException("Failure to register project with package '" + pid +
                                             "'.\nA project/link with that package qualifier " +
                                             "already exists and is registered in '" +
                                             self.directory + "'.\nThe old registration is not " +
                                             "being overwitten. To unregister the original " +
                                             "project, call: 'ocpidev unregister project " +
                                             self.__projects[pid].directory +
                                             "'.\nThen, run the command: 'ocpidev -d " +
                                             project.directory + " register project' or use the " +
                                             "'force' option")
            else:
                self.remove(package_id=pid)

        # link will be created at <registry>/<package-ID>
        project_link = self.directory + "/" + pid

        # if this statement is reached and the link exists, it is a broken link
        if os.path.lexists(project_link):
            # remove the broken link that would conflict
            self.remove_link(pid)

        # Perform the actual registration: create the symlink to the project in this registry
        # directory
        self.create_link(project)
        # Add the project to this registry's projects dictionary
        self.__projects[project.package_id] = project

        print("Successfully registered the " + project.package_id + " project: " +
              os.path.realpath(project.directory) + "\nInto the registry: " +
              os.path.realpath(self.directory) + "\n")

    def remove(self, package_id=None, directory=None):
        """
        Given a project's package-ID or directory, determine if the project is present
        in this registry. If so, remove it from this registry's __projects dictionary
        and remove the registered symlink.
        """
        logging.debug("package_id=" + str(package_id)+ " directory=" + str(directory))
        if package_id is None:
            package_id = ocpiutil.get_project_package(directory)
            if package_id is None:
                raise ocpiutil.OCPIException("Could not unregister project located at \"" +
                                             directory + "\" because the project's package-ID " +
                                             "could not be determined.\nIs it really a project?")

        if package_id not in self.__projects:
            link_path = self.directory + "/" + package_id
            if os.path.exists(link_path) and not os.path.exists(os.readlink(link_path)):
                logging.debug("Removing the following broken link from the registry:\n" +
                              link_path + " -> " + os.readlink(link_path))
                self.remove_link(package_id)
                print("Successfully unregistered the " + package_id + " project: " +
                      os.path.realpath(directory) + "\nFrom the registry: " +
                      os.path.realpath(self.directory) + "\n")
                return
            raise ocpiutil.OCPIException("Could not unregister project with package-ID \"" +
                                         package_id + "\" because the project is not in the " +
                                         "registry.\n Run 'ocpidev show registry --table' for " +
                                         "information about the currently registered projects.\n")

        # if a project is deleted from disk underneath our feet this could be None (AV-4483)
        if self.__projects[package_id] is not None:
            project_link = self.__projects[package_id].directory
            if directory is not None and os.path.realpath(directory) != project_link:
                raise ocpiutil.OCPIException("Failure to unregister project with package '" +
                                             package_id + "'.\nThe registered project with link '" +
                                             package_id + " --> " + project_link + "' does not " +
                                             "point to the specified project '" +
                                             os.path.realpath(directory) + "'." + "\nThis " +
                                             "project does not appear to be registered.")

        if directory is None:
            directory = str(self.__projects[package_id].directory)
        # Remove the symlink registry/package-ID --> project
        self.remove_link(package_id)
        # Remove the project from this registry's dict
        self.__projects.pop(package_id)
        print("Successfully unregistered the " + package_id + " project: " +
              os.path.realpath(directory) + "\nFrom the registry: " +
              os.path.realpath(self.directory) + "\n")

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

    def get_project(self, package_id):
        """
        Return the project with the specified package-id that is registered in this registry
        """
        if package_id not in self.__projects:
            raise ocpiutil.OCPIException("\"" + package_id + "\" is not a valid package-id or " +
                                         "project directory")
        return self.__projects[package_id]

    @staticmethod
    def create(asset_dir="."):
        """
        Create a registry (which is essentially a folder) at the location specified by asset_dir
        """
        print("making: " + asset_dir)
        os.mkdir(asset_dir)
        return AssetFactory.factory("registry", asset_dir)

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
                project_registry_dir = os.path.realpath(project_registry_dir)
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

        Return the exists Boolean and the path to the project registry directory.
        """
        if (ocpiutil.is_path_in_project(directory) and
                os.path.isdir(ocpiutil.get_path_to_project_top(directory) + "/imports")):
            #allow imports to be a link OR a directory (needed for deep copies of exported projects)
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

    def _collect_workers_dict(self):
        """
        return a dictionary with all the workers in all the projects in the registry
        """
        ret_dict = {}
        proj_dict = {}
        for proj in self.__projects:
            lib_dict = {}
            for lib in self.__projects[proj].lib_list:
                wkr_dict = {}
                # pylint:disable=unused-variable
                tests, wkrs = lib.get_valid_tests_workers()
                # pylint:enable=unused-variable
                for wkr in wkrs:
                    wkr_dict[os.path.basename(wkr)] = wkr
                if wkr_dict:
                    wkrs_dict = {"workers":wkr_dict,
                                 "directory":lib.directory,
                                 "package_id": lib.package_id}
                    lib_package = lib.package_id
                    # in case two or more libraries have the same package id we update the key to
                    # end with a number
                    i = 1
                    while lib_package in lib_dict:
                        lib_package += ":" + str(i)
                        i += 1
                    lib_dict[lib_package] = wkrs_dict
            if lib_dict:
                libs_dict = {"libraries":lib_dict,
                             "directory":self.__projects[proj].directory,
                             "package_id": self.__projects[proj].package_id}
                proj_dict[self.__projects[proj].package_id] = libs_dict

        ret_dict["projects"] = proj_dict
        return ret_dict

    def _collect_components_dict(self):
        """
        return a dictionary with all the components in all the projects in the registry
        """
        ret_dict = {}
        proj_dict = {}
        for proj in self.__projects:
            top_comp_dict = {}
            for comp in self.__projects[proj].get_valid_components():
                comp_name = ocpiutil.rchop(os.path.basename(comp), "spec.xml")[:-1]
                top_comp_dict[comp_name] = comp
            lib_dict = {}
            for lib in self.__projects[proj].lib_list:
                comp_dict = {}
                for comp in lib.get_valid_components():
                    comp_name = ocpiutil.rchop(os.path.basename(comp), "spec.xml")[:-1]
                    comp_dict[comp_name] = comp
                if comp_dict:
                    comps_dict = {"components":comp_dict,
                                  "directory":lib.directory,
                                  "package_id": lib.package_id}
                    lib_package = lib.package_id
                    # in case two or more  libraries have the same package id we update the key to
                    # end with a number
                    i = 1
                    while lib_package in lib_dict:
                        lib_package += ":" + str(i)
                        i += 1
                    lib_dict[lib_package] = comps_dict
            if lib_dict:
                libs_dict = {"libraries":lib_dict,
                             "directory":self.__projects[proj].directory,
                             "package_id": self.__projects[proj].package_id}
                if top_comp_dict:
                    libs_dict["components"] = top_comp_dict
                proj_dict[self.__projects[proj].package_id] = libs_dict

        ret_dict["projects"] = proj_dict
        return ret_dict

    # pylint:disable=unused-argument
    def show_workers(self, details, verbose, **kwargs):
        """
        Show all the workers in all the projects in the registry
        """
        reg_dict = self._collect_workers_dict()
        if details == "simple":
            for proj in reg_dict["projects"]:
                for lib in reg_dict["projects"][proj]["libraries"]:
                    for wkr in reg_dict["projects"][proj]["libraries"][lib]["workers"]:
                        print(wkr + " ", end="")
            print()
        elif details == "table":
            rows = [["Project", "Library Directory", "Worker"]]
            for proj in reg_dict["projects"]:
                for lib in reg_dict["projects"][proj]["libraries"]:
                    for wkr in reg_dict["projects"][proj]["libraries"][lib]["workers"]:
                        lib_dict = reg_dict["projects"][proj]["libraries"][lib]
                        rows.append([proj, lib_dict["directory"], wkr])
            ocpiutil.print_table(rows, underline="-")
        elif details == "json":
            json.dump(reg_dict, sys.stdout)
            print()
    # pylint:enable=unused-argument

    # pylint:disable=unused-argument
    def show_components(self, details, verbose, **kwargs):
        """
        Show all the components in all the projects in the registry
        """
        reg_dict = self._collect_components_dict()
        if details == "simple":
            for proj in reg_dict["projects"]:
                for comp in reg_dict["projects"][proj].get("components", []):
                    print(comp + " ", end="")
                for lib in reg_dict["projects"][proj]["libraries"]:
                    for comp in reg_dict["projects"][proj]["libraries"][lib]["components"]:
                        print(comp + " ", end="")
            print()
        elif details == "table":
            rows = [["Project", "Component Spec Directory", "Component"]]
            for proj in reg_dict["projects"]:
                for comp in reg_dict["projects"][proj].get("components", []):
                    rows.append([proj, reg_dict["projects"][proj]["directory"] + "/specs", comp])
                for lib in reg_dict["projects"][proj]["libraries"]:
                    for comp in reg_dict["projects"][proj]["libraries"][lib]["components"]:
                        lib_dict = reg_dict["projects"][proj]["libraries"][lib]
                        rows.append([proj, lib_dict["directory"] + "/specs", comp])
            ocpiutil.print_table(rows, underline="-")
        elif details == "json":
            json.dump(reg_dict, sys.stdout)
            print()
    # pylint:enable=unused-argument

    def get_dict(self, get_package):
        """
        return a dictionary with with information about the registry
        """
        proj_dict = {}
        for proj in self.__projects:
            if self.__projects[proj]:
                if get_package:
                    package_id = self.__projects[proj].package_id
                else:
                    package_id = proj
                proj_dict[package_id] = {
                    "real_path":self.__projects[proj].directory,
                    "exists":(os.path.exists(self.__projects[proj].directory) and
                              os.path.isdir(self.__projects[proj].directory))}

        json_dict = {"registry_location": self.directory}
        json_dict["projects"] = proj_dict
        return json_dict

    def show(self, details, verbose, **kwargs):
        """
        show information about the registry in the format specified by details
        (simple, table, or json)
        """
        reg_dict = self.get_dict(False)
        if details == "simple":
            print(" ".join(sorted(reg_dict["projects"])))
        elif details == "table":
            print("Project registry is located at: " + reg_dict["registry_location"])
            # Table header
            row_1 = ["Project Package-ID", "Path to Project", "Valid/Exists"]
            rows = [row_1]
            for proj in reg_dict["projects"]:
                rows.append([proj, reg_dict["projects"][proj]["real_path"],
                             reg_dict["projects"][proj]["exists"]])
            ocpiutil.print_table(rows, underline="-")
        elif details == "json":
            json.dump(reg_dict, sys.stdout)
            print()
