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
from .factory import *
from .worker import *
from .component import *
import os
import sys
import logging
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util

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
            init_tests   (T/F) - Instructs the method weather to construct all test objects contained
                                 in the library
            init_workers (T/F) - Instructs the method weather to construct all worker objects contained
                                 in the library
        """
        self.check_dirtype("library", directory)
        super().__init__(directory, name, **kwargs)
        self.test_list = None
        if kwargs.get("init_tests", False) or kwargs.get("init_workers", False):
           tests, wkrs = self.get_valid_tests_workers()
        if kwargs.get("init_tests", False):
            self.test_list = []
            logging.debug("Library constructor creating Test Objects")
            for test_directory in tests:
                self.test_list.append(AssetFactory.factory("test", test_directory, **kwargs))

        self.worker_list = None
        if kwargs.get("init_workers", False):
            # Collect the list of workers and initialize Worker objects for each worker
            # of a supported authoring model
            self.worker_list = []
            logging.debug("Library constructor creating Worker Objects")
            for worker_directory in wkrs:
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
        self.package_id = Library.get_package_id(self.directory)

    @staticmethod
    def get_package_id(directory='.'):
        lib_vars = ocpiutil.set_vars_from_make(mk_file=directory + "/Makefile",
                                               mk_arg="ShellLibraryVars=1 showpackage",
                                               verbose=True)
        return "".join(lib_vars['Package'])

    def get_valid_tests_workers(self):
        """
        Probe make in order to determine the list of active tests in the library
        """
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
