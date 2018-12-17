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
This file contains ReportableItems/regexs for Vivado.
This has been tested with build logs from Xilinx Vivado 2017.1.
"""
import _opencpi.util as ocpiutil
from _opencpi.hdlreportableitem import ReportableItem
from . import _xilinx

# NOTE: If adding regular expressions for new tool versions, individual regular
#       expressions can be replaced with lists of regexs. This way, we can add
#       support for new tool versions without breaking or changing support for
#       older versions.

# ReportableItems specific to Vivado
# Use a single regex for synthesis and implementation
part_item = ReportableItem("Device", r"Loading part ([0-9a-zA-Z\-]+)")
# Use a single regex for synthesis and implementation
vrsn_item = ReportableItem("Version", r"Vivado v([0-9]{4}\.[0-9]).*")

# Custom function for vivado fmax
def match_prd_ns_to_freq_MHz(target_file, regex):
    """
    Find the first number match for the regex in the target file, and convert to frequency in MHz
    Assume the original number is in ns.
    """
    prd = ocpiutil.match_regex_get_first_num(target_file, regex)
    return '{0:.3f}'.format(1E3 / float(prd)) if prd is not None else None

# Here, Fmax is disabled for sythesis, and uses a custom matching function
# for implementation to get the first number in the matched string and
# convert from period to frequency. Uses a single regex for synth and impl.
fmax_item = ReportableItem("Fmax (MHz) (Typ)", r"Path Delay.*", r"^\| Requirement.*",
                           match_and_transform_synth_function=\
                               lambda f, r: None,
                           match_and_transform_impl_function=match_prd_ns_to_freq_MHz)

# Various special functions and their corresponding regexs
spec_item = ReportableItem("Memory/Special Functions",
                           {"DSP48E1":r"DSP48E1\s*\|\s*([0-9]+)\s*\|",
                            "RAM64M":r"\|\s+RAM64M only\s*\|\s+([0-9]+) \|\s+\|\s+\|\s+\|",
                            "RAMB36E1":r"\|\s+RAMB36E1 only\s*\|\s+([0-9]+) \|\s+\|\s+\|\s+\|",
                            "RAMB18E1":r"\|\s+RAMB18E1 only\s*\|\s+([0-9]+) \|\s+\|\s+\|\s+\|",
                            "ODDR":r"\|\s*ODDR\s*\|\s*([0-9]+)\s*\|\s*IO\s*\|",
                            "BUFR":r"\|\s*BUFR\s*\|\s*([0-9]+)\s*\|\s*Clock\s*\|",
                            "BUFG":r"\|\s+BUFG\s+\|\s+([0-9]+) \|[^|]+\|",
                            "BUFGCTRL":r"\|\s+BUFGCTRL\s+\|\s+([0-9]+) \|[^|]+\|"})

# The item ordering of this list is the report order
reportable_items = [vrsn_item, part_item, _xilinx.regs_item, _xilinx.luts_item,
                    fmax_item, spec_item]

# Synthesis and implementation files for utilization reporting.
synth_files = ["*-vivado.out"]
impl_files = ["bit.out"]
