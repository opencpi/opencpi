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
import _opencpi.util as ocpiutil
from  _opencpi.assets import factory

"""
This file contains the unit tests for the Component object
"""
class ComponentTest(unittest.TestCase):
    asset_type = "component"
    def test_component_bad_dir(self):
        """
        create a Component in an invalid directory and an exception is thrown
        """
        self.assertRaises(ocpiutil.OCPIException,
                          factory.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_component_good_no_name(self):
        """
        create a Component and use the default name
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                "../av-test/components/test_worker-spec.xml")

    def test_component_good(self):
        """
        create a Component in the default way
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/specs/",
                                                  "test_worker-spec.xml")

    def test_component_good_extra_info(self):
        """
        create a Component with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/specs/",
                                                  "test_worker-spec.xml",
                                                  init_ocpigen_details=True)
        my_asset.show("simple", 0)

    def test_component_good_show_table(self):
        """
        create a Component with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/specs/",
                                                  "test_worker-spec.xml",
                                                  init_ocpigen_details=True)
        my_asset.show("table", 0)

    def test_component_good_show_json(self):
        """
        create a Component with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/specs/",
                                                  "test_worker-spec.xml",
                                                  init_ocpigen_details=True)
        my_asset.show("json", 0)

    def test_component_good_show_verbose(self):
        """
        create a Component with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/specs/",
                                                  "test_worker-spec.xml",
                                                  init_ocpigen_details=True)
        my_asset.show("json", 1)

class WorkerTest(unittest.TestCase):
    asset_type = "worker"
    def test_component_bad_dir(self):
        """
        create a Worker in an invalid directory and an exception is thrown
        """
        self.assertRaises(ocpiutil.OCPIException,
                          factory.AssetFactory.factory,
                          self.asset_type,
                          "/dev")

    def test_component_good_no_name(self):
        """
        create a Worker and use the default name
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                "../av-test/components/proxy1.rcc")

    def test_component_good(self):
        """
        create a Worker in the default way
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/proxy1.rcc",
                                                  "proxy1.xml")

    def test_component_good_extra_info(self):
        """
        create a Worker with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/proxy1.rcc/",
                                                  "proxy1",
                                                  init_ocpigen_details=True)
        my_asset.show("simple", 0)

    def test_component_good_show_table(self):
        """
        create a Worker with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/proxy1.rcc",
                                                  "proxy1",
                                                  init_ocpigen_details=True)
        my_asset.show("table", 0)

    def test_component_good_show_json(self):
        """
        create a Worker with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/proxy1.rcc",
                                                  "proxy1",
                                                  init_ocpigen_details=True)
        my_asset.show("json", 0)

    def test_component_good_show_verbose(self):
        """
        create a Worker with all the port and property information
        """
        my_asset = factory.AssetFactory.factory(self.asset_type,
                                                  "../av-test/components/proxy1.rcc",
                                                  "proxy1",
                                                  init_ocpigen_details=True)
        my_asset.show("json", 1)

if __name__ == '__main__':
    unittest.main()
