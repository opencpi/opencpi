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
import os
import sys
import logging
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util as ocpiutil

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

        self.run_before = kwargs.get("run_before", None)
        self.run_after = kwargs.get("run_after", None)
        self.run_arg = kwargs.get("run_arg", None)

    def run(self):
        """
        Runs the ApplicationsCollection with the settings specified in the object.  Running a
        ApplicationsCollection will run all the applications that are contained in the
        ApplicationsCollection
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory, ["run"])

    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("ApplicationsCollection.build() is not implemented")
