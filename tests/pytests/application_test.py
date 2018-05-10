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
This file contains the unit tests for the Application and ApplicationCollection objects
"""
class ApplicationsTest(unittest.TestCase):
    asset_type = "application"
    def test_application_bad_dir(self):
        """
        create a Application in an invalid directory and an exception is thrown
        """
        self.assertRaises(ocpiutil.OCPIException, 
                          ocpiassets.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_application_good_no_name(self):
        """
        create a Application and use the default name
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/applications/aci_property_test_app")
        assert my_asset.run() == 0

    def test_application_good(self):
        """
        create a Application in the default way
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/applications/aci_property_test_app",
                                                  "aci_property_test_app")
        assert my_asset.run() == 0

class ApplicationsCollectionsTest(unittest.TestCase):
    asset_type = "applications"
    def test_applications_bad_dir(self):
        """
        create a ApplicationCollection in an invalid directory and an exception is thrown
        """
        self.assertRaises(ocpiutil.OCPIException, 
                          ocpiassets.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_applications_good_no_name(self):
        """
        create a ApplicationCollection and use the default name
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/applications")
        assert my_asset.run() == 0

    def test_applications_good(self):
        """
        create a ApplicationCollection in the default way
        """
        my_asset = ocpiassets.AssetFactory.factory(self.asset_type,
                                                  "../av-test/applications",
                                                  self.asset_type)

        assert my_asset.run() == 0

if __name__ == '__main__':
    unittest.main()