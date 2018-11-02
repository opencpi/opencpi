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
This file contains the unit tests for the hdlreportableitem module.
Unit tests here include those for the ReportableItem class.

Run from this directory as follows:
    $ coverage3 run hdlreportableitem_test.py -v

To view the code coverage report:
    $ coverage3 report
To view the line-by-line code coverage report:
    $ coverage3 annotate ../../tools/cdk/scripts/hdlreportableitem.py
    $ vim ../../tools/cdk/scripts/hdlreportableitem.py,cover
"""

import unittest
import os
import sys
import logging
sys.path.insert(0, os.path.realpath(os.getenv('OCPI_CDK_DIR') + '/scripts/'))
import ocpiutil
from hdlreportableitem import ReportableItem

# Initialize ocpiutil's logging settings which switch
# based on OCPI_LOG_LEVEL
ocpiutil.configure_logging()

class TestReportableItem(unittest.TestCase):
    def test_reportable_item(self):
        # Here, we create a reportable item named "Item0" with a regex that matches
        # the following line:
        # This text right here contains a number (2.000ns)!!!! It should match item0's regex
        # We then call match_and_transform_synth() and give it this current file as the argument.
        # It should find that line and parse out the first number it finds:
        item0 = ReportableItem("Item0", "#.*This text right.*\((.*)ns\)!!!!.*")
        match0 = item0.match_and_transform_synth(os.path.realpath(__file__))
        logging.info("Assert that the match for item0 is 2.000")
        self.assertEqual(match0, "2.000")

        # Here, we initialize the synth and impl regexs separately.
        item0_ns = ReportableItem("Item0",
                                  synth_regexs="#.*This text right.*\((.*)ns\)!!!!.*",
                                  impl_regexs="#.*This text right.*\((.*ns)\)!!!!.*")
        match0_ns_synth = item0_ns.match_and_transform_synth(os.path.realpath(__file__))
        logging.info("Assert that synth match for item0_ns is 2.000")
        self.assertEqual(match0_ns_synth, "2.000")

        match0_ns_impl = item0_ns.match_and_transform_impl(os.path.realpath(__file__))
        logging.info("Assert that impl match for item0_ns is 2.000ns")
        self.assertEqual(match0_ns_impl, "2.000ns")


        # Now, provide lists of regexs. This is useful if there are multiple regular
        # expressions that might be used to match a single item key.
        # Here, the first regex in each list will not be matched (because of the '^' chars)
        # and so the second ones will be used.
        item0_ns_multi = ReportableItem("Item0",
                                        synth_regexs=["^NO MATCH, OLD TOOL VERSION",
                                                      "#.*This text right.*\((.*)ns\)!!!!.*"],
                                        impl_regexs=["^ALSO NO MATCH, OLD TOOL VERSION",
                                                     "#.*This text right.*\((.*ns)\)!!!!.*"])
        match0_ns_multi_synth = item0_ns_multi.match_and_transform_synth(os.path.realpath(__file__))
        logging.info("Assert that synth match for item0_ns_multi is 2.000")
        self.assertEqual(match0_ns_multi_synth, "2.000")

        match0_ns_multi_impl = item0_ns_multi.match_and_transform_impl(os.path.realpath(__file__))
        logging.info("Assert that impl match for item0_ns is 2.000ns")
        self.assertEqual(match0_ns_multi_impl, "2.000ns")

        # Here we also initialize the match/transform functions separately for synth
        # and impl.
        # This line should be matched by the impl function
        # This impl text right here contains a number (3.000ns)!!!! It should match an impl regex
        item0_ns_functs = ReportableItem("Item0",
                                         synth_regexs=r"#.*This text right.*\((.*)ns\)!!!!.*",
                                         impl_regexs=r"#.*This impl text right.*\((.*ns)\)!!!!.*",
                                         match_and_transform_synth_function=\
                                                 lambda f, r: float(ocpiutil.match_regex(f, r)),
                                         match_and_transform_impl_function=\
                                                 lambda f, r: float(ocpiutil.match_regex(f, r)[:-2]))
        match0_ns_synth = item0_ns_functs.match_and_transform_synth(os.path.realpath(__file__))
        logging.info("Assert that synth match for item0_ns_functs is a float: 2.000")
        self.assertEqual(match0_ns_synth, 2.000)
        match0_ns_impl = item0_ns_functs.match_and_transform_impl(os.path.realpath(__file__))
        logging.info("Assert that impl match for item0_ns_functs is a float: 3.000")
        self.assertEqual(match0_ns_impl, 3.000)

        # This line should have matches for multiple keys underneath the Item0 header
        # This complex text right here contains a number (8 DSPs) and another (2 BRAMs)!!!!
        item0_dict = ReportableItem("Item0",
                                    synth_regexs={\
                                            "DSPs": r"#.*This complex text.*\((.*) DSPs\).*!!!!$",
                                            "BRAMs": r"#.*This complex text.*\((.*) BRAMs\)!!!!$"})
        match0_dict_synth = item0_dict.match_and_transform_synth(os.path.realpath(__file__))
        logging.info("Assert that synth match for item0_dict is a list containing: " +
                     "'DSPSs: 8' and 'BRAMS: 2'")
        self.assertEqual(set(match0_dict_synth), set(["DSPs: " + str(8), "BRAMs: " + str(2)]))

if __name__ == '__main__':
    unittest.main()
