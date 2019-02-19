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
This file contains common ReportableItems/regexs for both ISE and Vivado.
This has been tested with build logs from Xilinx ISE 14.7 and Vivado 2017.1.
"""
import _opencpi.util as ocpiutil
from _opencpi.hdlreportableitem import ReportableItem


# NOTE: If adding regular expressions for new tool versions, individual regular
#       expressions can be replaced with lists of regexs. This way, we can add
#       support for new tool versions without breaking or changing support for
#       older versions.

# Xilinx tools (XST/ISE and Vivado) have common regular expressions that can be used
REGS_ITEM = ReportableItem("Registers (Typ)", r"Slice Registers.*",
                           match_and_transform_synth_function=ocpiutil.match_regex_get_first_num)
LUTS_ITEM = ReportableItem("LUTs (Typ)", r"Slice LUTs.*",
                           match_and_transform_synth_function=ocpiutil.match_regex_get_first_num)
