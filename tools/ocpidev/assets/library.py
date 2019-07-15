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
Definition of Library and Library collection classes
"""

import os
import logging
import _opencpi.util as ocpiutil
from .abstract import RunnableAsset, RCCBuildableAsset, HDLBuildableAsset, ReportableAsset, Asset
from .factory import AssetFactory
from .worker import Worker, HdlWorker

class Library(RunnableAsset, RCCBuildableAsset, HDLBuildableAsset, ReportableAsset):
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
            init_tests   (T/F) - Instructs the method whether to construct all test objects
                                 contained in the library
            init_workers (T/F) - Instructs the method whether to construct all worker objects
                                 contained in the library
        """
        self.check_dirtype("library", directory)
        super().__init__(directory, name, **kwargs)
        self.test_list = None
        self.tests_names = None
        self.wkr_names = None
        self.package_id, self.tests_names, self.wkr_names = (
            self.get_package_id_wkrs_tests(self.directory))
        if kwargs.get("init_tests", False):
            self.test_list = []
            logging.debug("Library constructor creating Test Objects")
            for test_directory in self.tests_names:
                self.test_list.append(AssetFactory.factory("test", test_directory, **kwargs))

        kwargs["package_id"] = self.package_id
        self.worker_list = None
        if kwargs.get("init_workers", False):
            # Collect the list of workers and initialize Worker objects for each worker
            # of a supported authoring model
            self.worker_list = []
            logging.debug("Library constructor creating Worker Objects")
            for worker_directory in self.wkr_names:
                auth = Worker.get_authoring_model(worker_directory)
                if auth not in Asset.valid_authoring_models:
                    logging.debug("Skipping worker \"" + directory +
                                  "\" with unsupported authoring model \"" + auth + "\"")
                else:
                    wkr_name = os.path.splitext(os.path.basename(worker_directory))[0]
                    self.worker_list.append(AssetFactory.factory("worker", worker_directory,
                                                                 name=wkr_name,
                                                                 **kwargs))
        self.comp_list = None
        if kwargs.get("init_comps", False):
            self.comp_list = []
            for comp_directory in self.get_valid_components():
                comp_name = ocpiutil.rchop(os.path.basename(comp_directory), "spec.xml")[:-1]
                self.comp_list.append(AssetFactory.factory("component", comp_directory,
                                                           name=comp_name, **kwargs))

    @staticmethod
    def get_package_id(directory='.'):
        """
        return the package id of the library.  This information is determined from the make build
        system in order to be accurate.
        """
        lib_vars = ocpiutil.set_vars_from_make(mk_file=directory + "/Makefile",
                                               mk_arg="ShellLibraryVars=1 showlib",
                                               verbose=True)
        return "".join(lib_vars['Package'])

    def get_package_id_wkrs_tests(self, directory='.'):
        """
        Return the package id of the Library from the make variable that is returned
        """
        lib_vars = ocpiutil.set_vars_from_make(mk_file=directory + "/Makefile",
                                               mk_arg="ShellLibraryVars=1 showlib",
                                               verbose=True)
        ret_package = "".join(lib_vars['Package'])
        make_wkrs = lib_vars['Workers'] if lib_vars['Workers'] != [''] else []
        make_tests = lib_vars['Tests'] if lib_vars['Tests'] != [''] else []
        ret_tests = []
        ret_wkrs = []
        for name in make_tests:
            if name != "":
                ret_tests.append(self.directory + "/" + name)
        for name in make_wkrs:
            if name.endswith((".rcc", ".rcc/", ".hdl", ".hdl/")):
                ret_wkrs.append(self.directory + "/" + name)
        return ret_package, ret_tests, ret_wkrs

    def get_valid_tests_workers(self):
        """
        Probe make in order to determine the list of active tests in the library
        """
        # If this function has already been called don't call make again because its very expensive
        if self.tests_names is not None and self.wkr_names is not None:
            return (self.tests_names, self.wkr_names)
        ret_tests = []
        ret_wkrs = []
        ocpiutil.logging.debug("Getting valid tests from: " + self.directory + "/Makefile")
        make_dict = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                mk_arg="ShellLibraryVars=1 showlib",
                                                verbose=True)
        make_tests = make_dict["Tests"]
        make_wkrs = make_dict["Workers"]

        for name in make_tests:
            if name != "":
                ret_tests.append(self.directory + "/" + name)
        for name in make_wkrs:
            if name.endswith((".rcc", ".rcc/", ".hdl", ".hdl/")):
                ret_wkrs.append(self.directory + "/" + name)
        self.tests_names = ret_tests
        self.wkr_names = ret_wkrs
        return (ret_tests, ret_wkrs)

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

    def show_utilization(self):
        """
        Show utilization separately for each HdlWorker in this library
        """
        for worker in self.worker_list:
            if isinstance(worker, HdlWorker):
                worker.show_utilization()

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Library.build() is not implemented")

    @staticmethod
    def get_working_dir(name, library, hdl_library, hdl_platform):
        """
        return the directory of a Library given the name (name) and
        library specifiers (library, hdl_library, hdl_platform)
        """
        # if more then one of the library location variables are not None it is an error.
        # a length of 0 means that a name is required and a default location of components/
        if len(list(filter(None, [library, hdl_library, hdl_platform]))) > 1:
            ocpiutil.throw_invalid_libs_e()
        if name: ocpiutil.check_no_libs("library", library, hdl_library, hdl_platform)
        if library:
            return "components/" + library
        elif hdl_library:
            return "hdl/" + hdl_library
        elif hdl_platform:
            return "hdl/platforms/" + hdl_platform + "/devices"
        elif name:
            if name != "components" and ocpiutil.get_dirtype() != "libraries":
                name = "components/" + name
            return name
        else:
            ocpiutil.throw_specify_lib_e()

class LibraryCollection(RunnableAsset, RCCBuildableAsset, HDLBuildableAsset, ReportableAsset):
    """
    This class represents an OpenCPI Library Collection.  Contains a list of the libraries that
    are in this library collection and can be initialized or left as None if not needed
    """
    def __init__(self, directory, name=None, **kwargs):
        self.check_dirtype("libraries", directory)
        super().__init__(directory, name, **kwargs)
        self.library_list = None
        if kwargs.get("init_libs_col", False):
            self.library_list = []
            logging.debug("LibraryCollection constructor creating Library Objects")
            for lib in next(os.walk(directory))[1]:
                lib_directory = directory + "/" + lib
                self.library_list.append(AssetFactory.factory("library", lib_directory, **kwargs))

    def run(self):
        """
        Runs the Library with the settings specified in the object.  Throws an exception if the
        tests were not initialized by using the init_tests variable at initialization.  Running a
        Library will run all the component unit tests that are contained in the Library
        """
        ret_val = 0
        for lib in self.library_list:
            run_val = lib.run()
            ret_val = ret_val + run_val
        return ret_val

    def show_utilization(self):
        """
        Show utilization separately for each library
        """
        for lib in self.library_list:
            lib.show_utilization()

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("LibraryCollection.build() is not implemented")
