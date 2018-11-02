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
This file contains ReportableItems/regexs for Quartus Prime Standard Edition.
This has been tested with build logs from Quartus Prime Standard Edition 17.1.
"""

from hdlreportableitem import ReportableItem

# NOTE: If adding regular expressions for new tool versions, individual regular
#       expressions can be replaced with lists of regexs. This way, we can add
#       support for new tool versions without breaking or changing support for
#       older versions.

# Use a single regex for synthesis and implementation
regs_item = ReportableItem("Registers (Typ)", r";\s*Total registers\s*; ([0-9,]+)")
# Use a single regex for synthesis and implementation
luts_item = ReportableItem("LUTs (Typ)", r"^;\s*Combinational ALUTs\s*; ([0-9,]+)")
# Use a single regex for synthesis and implementation
part_item = ReportableItem("Device", r"; Device\s+;\s+([0-9a-zA-Z\-]+)\s+;")
# Use a single regex for synthesis and implementation
vrsn_item = ReportableItem("Version", r";\s*Quartus Prime Version\s*; ([0-9\.]+) ")
# Note: method for determining Fmax for Altera tools has not yet been determined
spec_item = ReportableItem("Memory/Special Functions",
                           {"DSP18":r"; DSP block 18-bit elements\s*;\s*([0-9]+)\s*(;|/)",
                            "Block Memory Bits":r"; Total block memory bits\s*;\s*([0-9]+)\s*(;|/)",
                            "GXB Receiver PCS":
                            r"; Total GXB Receiver Channel PCS   \s*;\s*([0-9]+)\s*(;|/)",
                            "GXB Receiver PMA":
                            r"; Total GXB Receiver Channel PMA   \s*;\s*([0-9]+)\s*(;|/)",
                            "GXB Transmitter PCS":
                            r"; Total GXB Transmitter Channel PCS\s*;\s*([0-9]+)\s*(;|/)",
                            "GXB Transmitter PMA":
                            r"; Total GXB Transmitter Channel PMA\s*;\s*([0-9]+)\s*(;|/)",
                            "PLL":r"; Total PLLs\s*;\s*([0-9]+)\s*(;|/)",
                            "DLL":r"; Total DLLs\s*;\s*([0-9]+)\s*(;|/)"})

# The item ordering of this list is the report order
reportable_items = [vrsn_item, part_item, regs_item, luts_item, spec_item]

# Synthesis and implementation files for utilization reporting.
synth_files = ["*.map.rpt"]
impl_files = ["*.fit.rpt"]
