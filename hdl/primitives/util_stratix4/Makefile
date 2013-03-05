
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# This library is for things that have common interfaces (either in generic versions or other
# vendor/family secific versions), but have stratix4-specific implementations that will shadow
# modules in other libraries (util_altera, and util).
# NOTE********** The BRAMs are manually imported and renamed
# This following line would import if we didn't have to rename,
# But quartus (at least) requires file names in verilog libraries to match module names
# $(foreach i,BRAM1 BRAM1BE BRAM2,$(OCPI_HDL_IMPORTS_DIR)/libsrc/hdl/bsv/bram_patch/$i_alt.v)
OnlyTargets=stratix4
Imports=\
 $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_hip_s4gx_gen2_x4_128/pcie_hip_s4gx_gen2_x4_128_wrapper.v \
 $(foreach i,BRAM1 BRAM1BE BRAM2,$(OCPI_HDL_IMPORTS_DIR)/libsrc/hdl/bsv/bram_patch/$i_alt.v)
HdlNoLibraries=yes
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk # we are a library of primitives



