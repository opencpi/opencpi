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
This file contains the unit tests for the Asset factory class and any of the Asset abstract classes
"""
class AssetTest(unittest.TestCase):
    def test_bad_asset_type(self):
        """
        create an asset of an invalid type and an exception is thrown
        """
        self.assertRaises(ocpiutil.OCPIException,
                          ocpiassets.AssetFactory.factory,
                          "bad",
                          "../av-test")

    def test_settings(self):
        """
        create an asset of an invalid type and an exception is thrown
        """
        ocpiassets.Project.get_valid_settings()

class AssetFactoryTest(unittest.TestCase):
    """
    try to remove a asset from the asset factory static variable without specifying which asset,
    which will throw an exception
    """
    def test_bad_use_remove(self):
        self.assertRaises(ocpiutil.OCPIException,
                          ocpiassets.AssetFactory.remove)

if __name__ == '__main__':
    unittest.main()
