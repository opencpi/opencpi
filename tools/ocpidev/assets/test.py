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
Definition of Test class
"""

import _opencpi.util as ocpiutil
from .abstract import RunnableAsset, HDLBuildableAsset, RCCBuildableAsset

class Test(RunnableAsset, HDLBuildableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI Component Unit test.  Contains build/run settings that are
    specific to Tests.
    """
    valid_settings = ["keep_sims", "acc_errors", "cases", "verbose", "remote_test_sys", "view"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Test member data  and calls the super class __init__. Throws an exception if
        the directory passed in is not a valid test directory.
        valid kwargs handled at this level are:
            keep_sims (T/F) - Keep HDL simulation files for any simulation platforms
            acc_errors (T/F) - Causes errors to accumulate and tests to continue on
            cases (list) - Specify Which test cases that will be run/verified
            mode (list) - Specify which phases of the unit test to run
            remote_test_sys (list) - Specify remote systems to run the test(s)
        """
        if not directory.endswith(".test") and not directory.endswith(".test/"):
            directory = directory + ".test"
        self.check_dirtype("test", directory)
        super().__init__(directory, name, **kwargs)

        self.keep_sims = kwargs.get("keep_sims", False)
        self.view = kwargs.get("view", False)
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
        self.mode_dict['gen']             = ["generate"]
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
