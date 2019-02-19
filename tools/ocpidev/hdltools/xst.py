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
This file contains ReportableItems/regexs for Xilinx ISE (XST).
This has been tested with build logs from Xilinx ISE 14.7.
"""
from _opencpi.hdlreportableitem import ReportableItem
from . import _xilinx

# NOTE: If adding regular expressions for new tool versions, individual regular
#       expressions can be replaced with lists of regexs. This way, we can add
#       support for new tool versions without breaking or changing support for
#       older versions.

# ReportableItems specific to XST

# Use two different regexs for synthesis and implementation
PART_ITEM = ReportableItem("Device",
                           synth_regexs=r"Selected Device\s+:\s+([0-9a-zA-Z\-]+)",
                           impl_regexs=r"Using target part\s+\"([0-9a-zA-Z\-]+)\"")

# Use two different regexs for synthesis and implementation
VRSN_ITEM = ReportableItem("Version",
                           synth_regexs=r"Release ([0-9]{2}\.[0-9]) - xst.*",
                           impl_regexs=r"Release ([0-9]{2}\.[0-9]) - Map.*")

# Use a single regex for synthesis and implementation
FMAX_ITEM = ReportableItem("Fmax (MHz) (Typ)",
                           r"Maximum [fF]requency: ([0-9,]+\.[0-9]+)[MG]Hz")
# Various special functions and their corresponding regexs
SPEC_ITEM = ReportableItem("Memory/Special Functions",
                           {"DSP48E1":r"Number of DSP48E1s:\s*([0-9]+)\s*",
                            "RAM64M":r"\s+RAM64M\s+:\s+([0-9]+)\s*",
                            "RAMB36E1":r"\s+RAMB36E1\s+:\s+([0-9]+)\s*",
                            "ODDR":r"#\s* ODDR\s+:\s*([0-9]+)",
                            "BUFR":r"#\s* BUFR\s+:\s*([0-9]+)",
                            "BUFG":r"\s*Number of BUFG/BUFGCTRLs:\s+([0-9]+)\s+.*",
                            "BUFGCTRL":r"\s*Number of BUFG/BUFGCTRLs:\s+([0-9]+)\s+.*"})

# The item ordering of this list is the report order
REPORTABLE_ITEMS = [VRSN_ITEM, PART_ITEM, _xilinx.REGS_ITEM, _xilinx.LUTS_ITEM,
                    FMAX_ITEM, SPEC_ITEM]

# Synthesis and implementation files for utilization reporting.
SYNTH_FILES = ["*-xst.out"]
IMPL_FILES = ["map.out", "trce.out"]
