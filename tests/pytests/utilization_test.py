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
This file contains the unit tests for the ocpidev_utilization module.
Unit tests here include those for the ReportableItem class.

Run from this directory as follows:
    $ coverage3 run utilization_test.py -v

To view the code coverage report:
    $ coverage3 report
To view the line-by-line code coverage report:
    $ coverage3 annotate ../../tools/cdk/scripts/ocpiassets.py
    $ vim ../../tools/cdk/scripts/ocpiassets.py,cover
"""

import unittest
import os
import sys
import fnmatch
import logging
import subprocess
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util as ocpiutil
from  _opencpi.assets.factory import *
from  _opencpi.assets.abstract import *

OCPI_CDK_DIR = os.environ.get('OCPI_CDK_DIR')
OCPI_TOOL_PLATFORM = os.environ.get('OCPI_TOOL_PLATFORM')
# Determine path to ocpidev based on CDK so that we avoid accidentally
# using the one installed by RPMs
OCPIDEV_PATH = OCPI_CDK_DIR + '/' + OCPI_TOOL_PLATFORM + '/bin/ocpidev'

# Project with dummy assets and read-only log files for testing utilization reporting
UTIL_PROJ = "utilization_proj/"

#TODO This test suite can be expanded to be more truly a unit test suite and test the
#     individual get_utilization functions for the lower level classes like
#     HdlLibraryWorkerConfig, HdlPlatformWorkerConfig, HdlContainerImplementation, HdlContainer.
#     This could also lead to another improvement, which would involve reorganizing REPORT_GOLD
#     to have separate entries for each of these lower level classes. So, there could be
#     an entry for the worker's configuration 0, which has sub-entries for each target. Similarly,
#     there could be entries for each platform configuration, container and container
#     implementation which are accessed by name. This would also simplify get_gold_data_point_by_key
#     because it would just be able to index the dictionary by configuration/container... instead
#     of iterating through the list of data points to find the one that has a certain configuration.

# Each mode correspons to an asset/directory in the utilization_proj test project
DIRS = {"project": ".",
        "library": "components",
        "worker": "components/util_wkr.hdl",
        "hdl-platform": "hdl/platforms/zynq_plat",
        "hdl-platforms": "hdl/platforms",
        "hdl-assembly": "hdl/assemblies/util_assemb",
        "hdl-assemblies": "hdl/assemblies"}

# Each different mode has a different data-point field/dimension that serves somewhat as an index
# and is unique within a single build-target. So, using this field and OCPI Target, data-point
# entries can be uniquely identified
DATA_KEYS = {"worker": "Configuration",
             "hdl-platform": "Configuration",
             "hdl-assembly": "Container"}

# This is the golden utilization Report.data_points contents for checking against get_utilization()
# results. It has a set of data points for the three modes that support get_utilization()
REPORT_GOLD = {"worker":       [{'Registers (Typ)': '8', 'Version': '14.7', 'Tool': 'ISE',
                                 'LUTs (Typ)': '18', 'Device': '6vcx75tff484-2', 'Configuration': 0,
                                 'OCPI Target': 'virtex6', 'Memory/Special Functions': None,
                                 'Fmax (MHz) (Typ)': '663.438'},
                                {'Registers (Typ)': '9', 'Version': '2017.1', 'Tool': 'Vivado',
                                 'LUTs (Typ)': '20', 'Device': 'xc7z020clg400-3',
                                 'Configuration': 0, 'OCPI Target': 'zynq',
                                 'Memory/Special Functions': None, 'Fmax (MHz) (Typ)': None},
                                {'Registers (Typ)': '12', 'Version': '17.1.0', 'Tool': 'Quartus',
                                 'LUTs (Typ)': '25', 'Device': None, 'Configuration': 0,
                                 'OCPI Target': 'stratix4', 'Memory/Special Functions': None},
                                {'Registers (Typ)': '8', 'Version': '14.7', 'Tool': 'ISE',
                                 'LUTs (Typ)': '18', 'Device': '6vcx75tff484-2', 'Configuration': 1,
                                 'OCPI Target': 'virtex6', 'Memory/Special Functions': None,
                                 'Fmax (MHz) (Typ)': '663.438'},
                                {'Registers (Typ)': '9', 'Version': '2017.1', 'Tool': 'Vivado',
                                 'LUTs (Typ)': '20', 'Device': 'xc7z020clg400-3',
                                 'Configuration': 1, 'OCPI Target': 'zynq',
                                 'Memory/Special Functions': None, 'Fmax (MHz) (Typ)': None},
                                {'Registers (Typ)': '12', 'Version': '17.1.0', 'Tool': 'Quartus',
                                 'LUTs (Typ)': '25', 'Device': None, 'Configuration': 1,
                                 'OCPI Target': 'stratix4', 'Memory/Special Functions': None}],
               "hdl-platform": [{'Registers (Typ)': '1265', 'Memory/Special Functions': None,
                                 'Version': '2017.1', 'Device': 'xc7z020clg484-1',
                                 'OCPI Target': 'zynq', 'Configuration': 'base',
                                 'Fmax (MHz) (Typ)': None, 'Tool': 'Vivado', 'LUTs (Typ)': '793'},
                                {'Registers (Typ)': '1265', 'Memory/Special Functions': None,
                                 'Version': '2017.1', 'Device': 'xc7z020clg484-1',
                                 'OCPI Target': 'zynq', 'Configuration': 'cfg0',
                                 'Fmax (MHz) (Typ)': None, 'Tool': 'Vivado', 'LUTs (Typ)': '793'}],
               "hdl-assembly": [{'Container': 'base', 'Registers (Typ)': '2045',
                                 'Memory/Special Functions':
                                 ['RAMB36E1: 2', 'BUFG: 1', 'BUFGCTRL: 1'], 'Version': '2017.1',
                                 'Device': 'xc7z020clg484-1', 'OCPI Target': 'zynq',
                                 'Fmax (MHz) (Typ)': '100.000', 'Tool': 'Vivado',
                                 'OCPI Platform': 'zynq_plat', 'LUTs (Typ)': '1992'},
                                {'Container': 'base', 'Registers (Typ)': '4,597',
                                 'Memory/Special Functions': ['BUFG: 6', 'BUFGCTRL: 6'],
                                 'Version': '14.7', 'Device': '6vlx240tff1156-1',
                                 'OCPI Target': 'virtex6', 'Fmax (MHz) (Typ)': '127.453',
                                 'Tool': 'ISE', 'OCPI Platform': 'virtex6_plat',
                                 'LUTs (Typ)': '4,866'},
                                {'Container': 'base', 'Registers (Typ)': '17624',
                                 'Version': '17.1.0', 'Device': 'EP4SGX230KF40C2',
                                 'OCPI Target': 'stratix4', 'Memory/Special Functions':
                                 ['GXB Transmitter PMA: 4', 'GXB Receiver PMA: 4',
                                  'Block Memory Bits: 137772', 'PLL: 1', 'GXB Receiver PCS: 4',
                                  'GXB Transmitter PCS: 4'], 'Tool': 'Quartus',
                                 'OCPI Platform': 'stratix4_plat', 'LUTs (Typ)': '10,669'}]
              }

def get_gold_data_point_by_key(mode, key_val, target):
    """
    Each mode has an associated key that is useful for finding unique data-points.
    Given a mode/asset-type, a value for such a key, and an OCPI Target,
    a data-point can be uniquely identified in the dictionary of golden Report results.
    """
    key = DATA_KEYS[mode] # get the key that is useful for identifying data points in this mode
    # Get the golden data_points list for this mode
    for data_point in REPORT_GOLD[mode]:
        if data_point[key] == key_val and data_point['OCPI Target'] == target:
            # if a data_point with key:key_val and 'OCPI Target':target is found, return it
            return data_point
    return None

def construct_asset(mode):
    """
    Use the given mode/asset-type to determine the directory of an asset using the global
    dictionaries above. Construct and return that asset>
    """
    with ocpiutil.cd(UTIL_PROJ):
        kwargs = {'output_format': "table",
                  'hdl_plats': ["zynq_plat", "virtex6_plat", "stratix4_plat"],
                  'init_libs': True, 'init_workers': True,
                  'init_assembs': True, 'init_hdlplats': True}
        logging.debug("Constructing " + mode + " with args: " + str(kwargs))
        return AssetFactory.factory(asset_type=mode,
                                    directory=DIRS[mode],
                                    **kwargs)

class TestUtilization(unittest.TestCase):
    """
    Test get/show_utilization for project, library, worker, hdl-platform(s), and hdl-assembly(s)
    """
    @classmethod
    def setUpClass(cls):
        """
        Need to make sure registry is set to current system default before proceeding
        """
        with ocpiutil.cd(UTIL_PROJ):
            process = subprocess.Popen([OCPIDEV_PATH, "set", "registry"])
            results = process.communicate()
            if results[1] or process.returncode != 0:
                raise ocpiutil.OCPIException("'ocpidev set registry' failed in utilization test\n" +
                                             "process.returncode: " +  str(process.returncode))

    def test_project(self):
        logging.info("Doing project show_utilization() to confirm success, " +
                     "but not checking output")
        construct_asset("project").show_utilization()

    def test_library(self):
        logging.info("Doing library show_utilization() to confirm success, " +
                     "but not checking output")
        construct_asset("library").show_utilization()

    def test_worker(self):
        logging.info("Doing worker show_utilization() to confirm success, " +
                     "but not checking output")
        construct_asset("worker").show_utilization()

    def test_hdl_platform(self):
        self.check_utilization_for_mode("hdl-platform")

    def test_hdl_platforms(self):
        logging.info("Doing hdl-platforms show_utilization() to confirm success, " +
                     "but not checking output")
        construct_asset("hdl-platforms").show_utilization()

    def test_hdl_assembly(self):
        self.check_utilization_for_mode("hdl-assembly")

    def test_hdl_assemblies(self):
        logging.info("Doing hdl-assemblies show_utilization() to confirm success, " +
                     "but not checking output")
        construct_asset("hdl-assemblies").show_utilization()

    def check_utilization_for_mode(self, mode):
        """
        For a given mode, construct the asset of interest, get its utilization,
        and check that utilization report against the golden/known-good results.
        """
        my_asset = construct_asset(mode)
        with ocpiutil.cd(UTIL_PROJ):
            report = my_asset.get_utilization()
            logging.info("Ensuring number of data_points matches golden dataset")
            self.assertEqual(len(report.data_points), len(REPORT_GOLD[mode]))
            # Iterate through these utilization report data_points and compare
            # the length and contents of each with its matching golden data_point
            for data_point in report.data_points:
                # DATA_KEYS will tell us which key is useful for uniquely identifying
                # data-points in this mode
                indexing_key = DATA_KEYS[mode]
                index = data_point[indexing_key]
                # Get the golden data-point using the "OCPI Target" and index
                gold_data_point = get_gold_data_point_by_key(mode,
                                                             index,
                                                             data_point["OCPI Target"])
                gold_len = len(gold_data_point)
                logging.info("Ensuring # of entries in data_point " + str(index) +
                             " matches len \"" + str(gold_len) + "\" from the golden dataset")
                self.assertEqual(len(data_point), gold_len)
                # Make sure all key/value pairs match the golden/expected data-point
                for key, val in data_point.items():
                    gold_val = gold_data_point[key]
                    logging.info("Ensuring that data_point entry for key=" + key +
                                 " matches entry \"" + str(gold_val) + "\" in golden dataset")
                    if isinstance(val, list):
                        # If the value is a list, convert to set so ordering does not affect
                        # comparison
                        self.assertEqual(set(val), set(gold_val))
                    else:
                        self.assertEqual(val, gold_val)

            logging.info("Doing show_utilization() to confirm success, but not checking output")
            my_asset.show_utilization()

if __name__ == '__main__':
    unittest.main()
