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
This file contains the unit tests for the hdltargets module.
Unit tests here include those for the HdlTarget, HdlPlatform, and HdlToolSet classes.
We use TGT_DICT which is a hardcoded dictionary representation of the contents of
the local hdl-targets.mk (which is a snapshot of cdk/include/hdl-targets.mk.

We load the contents of hdl-targets.mk into HdlPlatform/Target/ToolSet objects
and check the resulting objects and their functions against the hardcoded TGT_DICT.

Run from this directory as follows:
    $ coverage3 run hdltargets_test.py -v

To view the code coverage report:
    $ coverage3 report
To view the line-by-line code coverage report:
    $ coverage3 annotate ../../tools/cdk/scripts/hdltargets.py
    $ vim ../../tools/cdk/scripts/hdltargets.py,cover
"""
import unittest
import os
import sys
import logging
from shutil import copyfile
sys.path.insert(0, os.path.realpath(os.getenv('OCPI_CDK_DIR') + '/scripts/'))
import ocpiutil
from hdltargets import HdlToolFactory

# Initialize ocpiutil's logging settings which switch
# based on OCPI_LOG_LEVEL
ocpiutil.configure_logging()

# Directory of THIS test file
DIR_PATH = os.path.dirname(os.path.realpath(__file__))

# Dummy/static hdl-targets.mk file to mimic the real one in the CDK
TARGETS_FILE = DIR_PATH + "/include/hdl/hdl-targets.mk"

def setUpUnitTest():
    os.environ['OCPI_CDK_DIR'] = os.path.realpath('.')
    logging.info("CDK: " + os.environ['OCPI_CDK_DIR'])

    # Mimic CDK directory tree here with temporary dirs
    os.makedirs(DIR_PATH + "/include/hdl/")
    copyfile("hdl-targets.mk", TARGETS_FILE)

def tearDownUnitTest():
    os.remove(TARGETS_FILE)
    os.removedirs(DIR_PATH + "/include/hdl")

# Hard-coded dictionary representation of the contents of hdl-targets.mk
# for the purpose of testing hdltarget.py
TGT_DICT = {
    'HdlAllTargets': ['isim', 'virtex5', 'virtex6', 'spartan3adsp', 'spartan6', 'zynq_ise',
                      'zynq', 'xsim', 'stratix4', 'stratix5', 'modelsim', 'xilinx', 'altera'],
    'HdlToolSet_zynq': ['vivado'],
    'HdlToolName_vivado': ['Vivado'],
    'HdlPart_zed_zipper': ['xc7z020-1-clg484'],
    'HdlFamily_isim': ['isim'],
    'HdlPart_isim': ['isim'],
    'HdlFamily_xsim': ['xsim'],
    'HdlTargets_xilinx': ['isim', 'virtex5', 'virtex6', 'spartan3adsp', 'spartan6',
                          'zynq_ise', 'zynq', 'xsim'],
    'HdlTargets_zynq': ['xc7z020'],
    'HdlToolSet_virtex6': ['xst'],
    'HdlToolSet_virtex5': ['xst'],
    'HdlAllPlatforms': ['zed', 'zed_zipper', 'alst4x', 'alst4', 'modelsim',
                        'isim', 'xsim', 'zed_ise', 'ml605'],
    'HdlBuiltPlatforms': ['modelsim', 'isim', 'xsim'],
    'HdlFamily_ep4sgx230k-c2-f40': ['stratix4'],
    'HdlFamily_xc6vlx240t-1-ff1156': ['virtex6'],
    'HdlToolSet_xsim': ['xsim'],
    'HdlToolName_xsim': ['Vivado'],
    'HdlToolSet_zynq_ise': ['xst'],
    'HdlToolSet_modelsim': ['modelsim'],
    'HdlToolName_modelsim': ['Modelsim'],
    'HdlPart_xsim': ['xsim'],
    'HdlPart_zed': ['xc7z020-1-clg484'],
    'HdlFamily_ep4sgx530k-c2-h40': ['stratix4'],
    'HdlTargets': ['isim', 'virtex5', 'virtex6', 'spartan3adsp', 'spartan6',
                   'zynq_ise', 'zynq', 'xsim', 'stratix4', 'stratix5', '', 'modelsim'],
    'HdlToolSet_isim': ['isim'],
    'HdlToolName_isim': ['ISE'],
    'HdlTopTargets': ['xilinx', 'altera', 'modelsim'],
    'HdlPart_alst4': ['ep4sgx230k-c2-f40'],
    'HdlSimTools': ['isim', 'icarus', 'verilator', 'ghdl', 'xsim', 'modelsim'],
    'HdlPart_modelsim': ['modelsim'],
    'HdlAllFamilies': ['isim', 'virtex5', 'virtex6', 'spartan3adsp', 'spartan6',
                       'zynq_ise', 'zynq', 'xsim', 'stratix4', 'stratix5', 'modelsim'],
    'HdlPart_zed_ise': ['xc7z020_ise_alias-1-clg484'],
    'HdlTargets_stratix5': ['ep5sgsmd8k2'],
    'HdlTargets_stratix4': ['ep4sgx230k', 'ep4sgx530k', 'ep4sgx360'],
    'HdlTargets_spartan3adsp': ['xc3sd3400a'],
    'HdlTargets_virtex6': ['xc6vlx240t'],
    'HdlTargets_virtex5': ['xc5vtx240t', 'xc5vlx50t', 'xc5vsx95t', 'xc5vlx330t', 'xc5vlx110t'],
    'HdlToolSet_spartan3adsp': ['xst'],
    'HdlFamily_xc7z020_ise_alias-1-clg484': ['zynq_ise'],
    'HdlTargets_spartan6': ['xc6slx45'],
    'HdlTargets_altera': ['stratix4', 'stratix5'],
    'HdlPart_alst4x': ['ep4sgx530k-c2-h40'],
    'HdlFamily_modelsim': ['modelsim'],
    'HdlFamily_xc7z020-1-clg484': ['zynq'],
    'HdlToolSet_spartan6': ['xst'],
    'HdlToolName_xst': ['ISE'],
    'HdlTargets_zynq_ise': ['xc7z020_ise_alias'],
    'HdlPart_ml605': ['xc6vlx240t-1-ff1156'],
    'HdlToolSet_stratix5': ['quartus'],
    'HdlToolSet_stratix4': ['quartus'],
    'HdlToolName_quartus': ['Quartus']}

class TestHdlTargets(unittest.TestCase):
    """
    Test the HdlTarget class
    """
    @classmethod
    def setUpClass(cls):
        setUpUnitTest()
    @classmethod
    def tearDownClass(cls):
        tearDownUnitTest()

    def test_top_targets(self):
        logging.info("********************************************")
        toptargets = TGT_DICT['HdlTopTargets']
        logging.info("List of top targets should be: " + str(toptargets))
        self.assertEqual(set(HdlToolFactory.get_all_vendors()), set(toptargets))
        logging.info("Testing target associations with top-targets/vendors")
        for top in toptargets:
            logging.info("--------------------------------------------")
            logging.info("Top target: " + top)
            if 'HdlTargets_' + top in TGT_DICT:
                logging.info("Children targets should be: " + str(TGT_DICT['HdlTargets_' + top]))
                for name in TGT_DICT['HdlTargets_' + top]:
                    self.assertEqual(HdlToolFactory.factory("hdltarget", name).name, name)
                    self.assertEqual(HdlToolFactory.factory("hdltarget", name).vendor, top)
            else:
                logging.info("Children targets should be: " + str([top]))
                self.assertEqual(HdlToolFactory.factory("hdltarget", top).name, top)
                self.assertEqual(HdlToolFactory.factory("hdltarget", top).vendor, top)

    def test_targets_inits(self):
        logging.info("********************************************")
        logging.info("Testing initialization of targets")
        for name in TGT_DICT['HdlAllFamilies']:
            logging.info("--------------------------------------------")
            logging.info("Initialization of target: " + name)
            self.assertEqual(HdlToolFactory.factory("hdltarget", name).name, name)
            # test __str__
            self.assertEqual(str(HdlToolFactory.factory("hdltarget", name)), name)
            logging.info("Parts list of target: " + name)
            if 'HdlTargets_' + name in TGT_DICT:
                logging.info("Parts list should be: " + str(TGT_DICT['HdlTargets_' + name]))
                self.assertEqual(HdlToolFactory.factory("hdltarget", name).parts,
                                 TGT_DICT['HdlTargets_' + name])
            else:
                logging.info("Parts list should be: " + str([name]))
                self.assertEqual(HdlToolFactory.factory("hdltarget", name).parts,
                                 [name])
            logging.info("ToolSet for should be: " + str(TGT_DICT['HdlToolSet_' + name][0]))
            self.assertEqual(HdlToolFactory.factory("hdltarget", name).toolset.name,
                             TGT_DICT['HdlToolSet_' + name][0])
        self.assertRaises(ocpiutil.OCPIException,
                          HdlToolFactory.factory, "hdltarget", "Not a valid name")

    def test_all_targets_for_vendor(self):
        logging.info("********************************************")
        for top in TGT_DICT['HdlTopTargets']:
            alltargets0 = HdlToolFactory.get_all_targets_for_vendor(top)
            if 'HdlTargets_' + top in TGT_DICT:
                golden_list_of_targets = TGT_DICT['HdlTargets_' + top]
            else:
                golden_list_of_targets = [top]
            logging.info("Targets for vendor \"" + top + "\" should be: " + str(golden_list_of_targets))
            self.assertEqual(set([tgt.name for tgt in alltargets0]), set(golden_list_of_targets))
            self.assertEqual(set([str(tgt) for tgt in alltargets0]), set(golden_list_of_targets))

    def test_all_targets(self):
        logging.info("********************************************")
        golden_targets = TGT_DICT['HdlAllFamilies']
        logging.info("All targets should be: " + str(golden_targets))

        all_targets = HdlToolFactory.get_or_create_all("hdltarget")
        self.assertEqual(set([tgt.name for tgt in all_targets]), set(golden_targets))

class TestHdlToolSet(unittest.TestCase):
    """
    Test the HdlToolSet class
    """
    @classmethod
    def setUpClass(cls):
        setUpUnitTest()
    @classmethod
    def tearDownClass(cls):
        tearDownUnitTest()

    def test_toolset_inits(self):
        logging.info("********************************************")
        logging.info("Testing initialization of toolsets")
        for tgt, name in [(tgt, TGT_DICT['HdlToolSet_' + tgt][0])\
                             for tgt in TGT_DICT['HdlAllFamilies']]:
            logging.info("--------------------------------------------")
            logging.info("Target: \"" + tgt + "\" should have toolset: " + name)
            self.assertEqual(HdlToolFactory.factory("hdltarget", tgt).toolset.name, name)
            self.assertEqual(HdlToolFactory.factory("hdltoolset", name).name, name)
            title = TGT_DICT['HdlToolName_' + name][0]
            logging.info("Toolset: \"" + name + "\" should have title: " + title)
            self.assertEqual(HdlToolFactory.factory("hdltoolset", name).title, title)
            # test __str__
            self.assertEqual(str(HdlToolFactory.factory("hdltoolset", name)), name)
            if name in TGT_DICT['HdlSimTools']:
                logging.info("This SHOULD be a simulator tool")
                self.assertTrue(HdlToolFactory.factory("hdltoolset", name).is_simtool)
            else:
                logging.info("This should NOT be a simulator tool")
                self.assertFalse(HdlToolFactory.factory("hdltoolset", name).is_simtool)
        self.assertRaises(ocpiutil.OCPIException,
                          HdlToolFactory.factory, "hdltoolset", "Not a valid name")

class TestHdlPlatform(unittest.TestCase):
    """
    Test the HdlPlatform class
    """
    @classmethod
    def setUpClass(cls):
        setUpUnitTest()
    @classmethod
    def tearDownClass(cls):
        tearDownUnitTest()

    def test_platforms_inits(self):
        logging.info("********************************************")
        logging.info("Testing initialization of platforms")
        for name in TGT_DICT['HdlAllPlatforms']:
            logging.info("--------------------------------------------")
            logging.info("Initialization of platform: " + name)
            exactpart = TGT_DICT['HdlPart_' + name][0]
            target = TGT_DICT['HdlFamily_' + exactpart][0]
            logging.info("Fields should be: \"" + name + ", " + target + ", " + exactpart)
            platform = HdlToolFactory.factory("hdlplatform", name)
            self.assertEqual(platform.name, name)
            # test __str__
            self.assertEqual(str(platform), name)
            self.assertEqual(platform.exactpart, exactpart)
            self.assertEqual(platform.target.name, target)
            tool = str(platform.target.toolset)
            logging.info("Platform should have toolset: " + tool)
            self.assertEqual(str(platform.get_toolset()), tool)
        self.assertRaises(ocpiutil.OCPIException,
                          HdlToolFactory.factory, "hdlplatform", "Not a valid name")

    def test_all_platforms(self):
        logging.info("********************************************")
        golden_platforms = TGT_DICT['HdlAllPlatforms']
        logging.info("All platforms should be: " + str(golden_platforms))
        all_platforms = HdlToolFactory.get_or_create_all("hdlplatform")
        self.assertEqual(set([plat.name for plat in all_platforms]), set(golden_platforms))

if __name__ == '__main__':
    unittest.main()
