
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
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

# Process the imported cores, which will end up in the
# list $(ImportCores).
ImportCoreDirs=\
  $(OCPI_HDL_IMPORTS_DIR)/coregen/temac_v6 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v6_mig34 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v6_mig37 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v5_mig34 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/fft_v5_4k_strm_nat \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v6_gtx_x4_250 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v5_gtp_x8_125 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v5_gtx_x8_125 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_hip_s4gx_gen2_x4_128 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/ddc_4243_4ch_v5

include $(OCPI_CDK_DIR)/include/hdl/hdl-import-cores.mk

Libs=bsv util util_virtex6 util_virtex5 util_stratix4 vendor_altera

#LocalCores=xfft_v7_1 \
 temac_v6 \
 pcie_4243_axi_v6_gtx_x4_250 \
 pcie_4243_trn_s6_x1_125 \
 pcie_4243_trn_v5_gtp_x1_125 \
 pcie_4243_trn_v5_gtp_x8_125 \
 pcie_4243_trn_v5_gtx_x8_125 \

# pcie_4243_trn_v6_gtx_x4_250 
# pcie_4243_trn_v6es_gtx_x4_250 

Cores=$(LocalCores) $(ImportCores)

MyMake=$(MAKE) HdlTargets="$(HdlTargets)" --no-print-directory
all: $(Libs) $(Cores)
# enable cores to use libs
$(LocalCores): $(Libs)

clean:
	$(AT)$(foreach l,$(Libs) $(Cores),\
		$(MyMake) -C $(l) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) clean;)
cleanimports:
	$(AT)$(foreach l,$(Libs) $(LocalCores),\
		$(MyMake) -C $(l) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) cleanimports;)

install:
	$(AT)set -e;$(foreach l,$(Libs) $(Cores),$(MyMake) -C $(l) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) install;)

uninstall:
	$(AT)echo Removing all installed HDL primitives and codes from $(OCPI_CDK_DIR)/lib
	$(AT)rm -r -f $(OCPI_CDK_DIR)/lib/hdl

.PHONY: $(Libs) $(Cores)

define MakeCoreOrLib
	$(AT)$(MyMake) -C $@ OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR))
	$(AT)$(MyMake) -C $@ OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) install
endef

$(Libs):
	$(AT)echo ============== For library $@:
	$(MakeCoreOrLib)

$(LocalCores):
	$(AT)echo ============== For core $@:
	$(MakeCoreOrLib)

$(ImportCores):
	$(AT)echo ============== For imported core $@:
	$(MakeCoreOrLib)


