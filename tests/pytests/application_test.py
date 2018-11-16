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
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util as ocpiutil
from  _opencpi.assets import factory

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
                          factory.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_application_good_no_name(self):
        """
        create a Application and use the default name
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/applications/aci_property_test_app")
        assert my_asset.run() == 0

    def test_application_good(self):
        """
        create a Application in the default way
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
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
                          factory.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_applications_good_no_name(self):
        """
        create a ApplicationCollection and use the default name
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/applications")
        assert my_asset.run() == 0

    def test_applications_good(self):
        """
        create a ApplicationCollection in the default way
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/applications",
                                                  self.asset_type)

        assert my_asset.run() == 0

if __name__ == '__main__':
    unittest.main()
