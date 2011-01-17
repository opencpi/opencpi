
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

OCPI_CDK_DIR=../../ocpi
include $(OCPI_CDK_DIR)/include/util.mk
Libs=bsv util util_virtex6 util_virtex5
Cores=xfft_v7_1 duc_ddc_compiler_v1_0 \
 dram_mig33_v6 dram_mig34_v5 \
 temac_v6 \
 pcie_v6_x4_250 pcie_v5_gtp_x1_125 pcie_v5_gtp_x8_125 pcie_v5_gtx_x8_125 pcie_s6_x1_125
all: $(Libs) $(Cores)
# enable cores to use libs
$(Cores): $(Libs)

clean:
	$(AT)$(foreach l,$(Libs) $(Cores),\
		$(MAKE) -C $(l) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) clean;)
cleanimports:
	$(AT)$(foreach l,$(Libs) $(Cores),\
		$(MAKE) -C $(l) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) cleanimports;)

install:
	$(AT)set -e;$(foreach l,$(Libs) $(Cores),$(MAKE) -C $(l) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) install;)

.PHONY: $(Libs) $(Cores)

$(Libs) $(Cores):
	$(AT)echo ============== For library/core $@:
	$(AT)$(MAKE) -C $@ OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR))
	$(AT)$(MAKE) -C $@ OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) install
