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
import unittest
import sys
import os
import subprocess
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util as ocpiutil
from  _opencpi.assets.factory import *

"""
This file contains the unit tests for the Project object
"""
class ProjectTest(unittest.TestCase):
    asset_type = "project"
    def test_prj_bad_dir(self):
        """
        create a project in an invalid directory and an exception should be thrown
        """
        self.assertRaises(ocpiutil.OCPIException,
                          AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_prj_good_no_name(self):
        """
        create a project and use the default name and just initialize the applications.  Then run
        the applications in the project
        """
        my_asset = AssetFactory.factory(self.asset_type,
                                                  "../av-test",
                                                  init_apps_col=True)
        assert my_asset.run() == 0
        AssetFactory.remove("../av-test")

    def test_prj_no_init(self):
        """
        create a without initializing apps or libraries an exception is thrown when trying to run
        """
        my_asset = AssetFactory.factory(self.asset_type,
                                                  "../av-test")
        self.assertRaises(ocpiutil.OCPIException, my_asset.run)

    def test_prj_good(self):
        """
        create a project in the default way
        """
        my_asset = AssetFactory.factory(self.asset_type,
                                                  "../av-test",
                                                  "av-test",
                                                  init_tests=True,
                                                  init_libs=True)
        assert my_asset.run() == 0
        AssetFactory.remove(instance=my_asset)

class DeleteProjectTest(unittest.TestCase):
    asset_type = "project"
    @classmethod
    def setUpClass(cls):
        ocpidev_command = "ocpidev create project mypj0_del"
        process = subprocess.Popen(ocpidev_command, shell=True)
        results = process.communicate()


    def test_del(self):
        my_asset = AssetFactory.factory(self.asset_type, "mypj0_del")
        my_asset.delete(True)

if __name__ == '__main__':
    unittest.main()
