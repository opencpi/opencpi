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
sys.path.insert(0, os.path.realpath(os.getenv('OCPI_CDK_DIR') + '/scripts/'))
import ocpiutil
import ocpiassets

"""
This file contains the unit tests for the Test object
"""
class TestsTest(unittest.TestCase):
    asset_type = "test"
    def test_test_bad_dir(self):
        """
        create a Test in an invalid directory and an exception is thrown
        """
        self.assertRaises(ocpiutil.OCPIException,
                          ocpiassets.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_test_good_no_name(self):
        """
        create a Test and use the default for name
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/test_worker.test")
        assert my_asset.run() == 0

    def test_test_good_(self):
        """
        create a Test in the default way
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/test_worker.test",
                                                  "prop_mem_align_info")
        assert my_asset.run() == 0

if __name__ == '__main__':
    unittest.main()
