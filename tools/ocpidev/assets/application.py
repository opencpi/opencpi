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
Definition of Application and ApplicationCollection classes
"""

import logging
import _opencpi.util as ocpiutil
from .factory import AssetFactory
from .abstract import RunnableAsset, RCCBuildableAsset

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
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Application.build() is not implemented")

    @staticmethod
    def get_working_dir(name, library, hdl_library, hdl_platform):
        """
        return the directory of an Application given the name (name) and
        library specifiers (library, hdl_library, hdl_platform)
        """
        ocpiutil.check_no_libs("application", library, hdl_library, hdl_platform)
        if ocpiutil.get_dirtype() not in ["application", "applications", "project"]:
            ocpiutil.throw_not_valid_dirtype_e(["applications", "project"])
        if not name: ocpiutil.throw_not_blank_e("application", "name", True)
        #assume the only valid place for a application in a project is in the applications directory
        return ocpiutil.get_path_to_project_top() + "/applications/" + name

class ApplicationsCollection(RunnableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI applications directory.  Ability act on multiple applications
    with a single instance are located in this class.
    """
    valid_settings = ["run_before", "run_after", "run_arg"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes ApplicationsCoclletion member data  and calls the super class __init__.
        Throws an exception if the directory passed in is not a valid applications directory.
        valid kwargs handled at this level are:
            run_before (list) - Arguments to insert before the ACI executable or ocpirun
            run_after (list) - Arguments to insert at the end of the execution command line A
            run_arg (list) - Arguments to insert immediately after the ACI executable or ocpirun
        """
        self.check_dirtype("applications", directory)
        super().__init__(directory, name, **kwargs)

        self.apps_list = None
        if kwargs.get("init_apps_col", False):
            self.apps_list = []
            logging.debug("ApplicationsCollection constructor creating Applications Objects")
            for app_directory in self.get_valid_apps():
                self.apps_list.append(AssetFactory.factory("application", app_directory, **kwargs))

        self.run_before = kwargs.get("run_before", None)
        self.run_after = kwargs.get("run_after", None)
        self.run_arg = kwargs.get("run_arg", None)

    def get_valid_apps(self):
        """
        Gets a list of all directories of type applications in the project and puts that
        applications directory and the basename of that directory into a dictionary to return
        """
        return ocpiutil.get_subdirs_of_type("application", self.directory)

    def run(self):
        """
        Runs the ApplicationsCollection with the settings specified in the object.  Running a
        ApplicationsCollection will run all the applications that are contained in the
        ApplicationsCollection
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory, ["run"])

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("ApplicationsCollection.build() is not implemented")

    @staticmethod
    def get_working_dir(name, library, hdl_library, hdl_platform):
        """
        return the directory of an Application Collection given the name (name) and
        library specifiers (library, hdl_library, hdl_platform)
        """
        ocpiutil.check_no_libs("applications", library, hdl_library, hdl_platform)
        if name: ocpiutil.throw_not_blank_e("applications", "name", False)
        if ocpiutil.get_dirtype() not in ["applications", "project"]:
            ocpiutil.throw_not_valid_dirtype_e(["applications", "project"])
        #assume the only valid place for a applications collection in a project is in the
        #applications directory
        return ocpiutil.get_path_to_project_top() + "/applications"
