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
sys.path.insert(0, os.path.realpath('../../tools/cdk/scripts/'))
import ocpiutil
import ocpiassets

"""
This file contains the unit tests for the Library object
"""
class LibraryTest(unittest.TestCase):
    asset_type = "library"
    def test_lib_bad_dir(self):
        """
        create a library in an invalid directory and an exception should be thrown
        """
        self.assertRaises(ocpiutil.OCPIException, 
                          ocpiassets.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_lib_good_no_name(self):
        """
        create a library using the default name
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components",
                                                  init_tests=True)
        assert my_asset.run() == 0

    def test_lib_no_init(self):
        """
        create a library and forget to initialize the test objects and a exception is thrown
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                   "../av-test/components",
                                                   "components")

        self.assertRaises(ocpiutil.OCPIException, my_asset.run)

    def test_lib_good(self):
        """
        create a library in the normal way and run it
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components",
                                                  "components",
                                                  init_tests=True)
        assert my_asset.run() == 0

if __name__ == '__main__':
    unittest.main()