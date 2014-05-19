
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
##nifndef HdlTargets
#HdlTargets:=virtex6
#endif
# Process the imported cores, which will end up in the
# list $(ImportCores).

# has problems, probably don't need anymore
#  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v6_mig34

ImportCoreDirs=\
  $(OCPI_HDL_IMPORTS_DIR)/coregen/temac_v6 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v6_mig37 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v5_mig34 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/fft_v5_4k_strm_nat \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v6_gtx_x4_250 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v5_gtp_x8_125 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v5_gtx_x8_125 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_hip_s4gx_gen2_x4_128 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/ddc_4243_4ch_v5

include $(OCPI_CDK_DIR)/include/hdl/hdl-import-cores.mk

# These libraries may be order sensitive if they depend on each other
# bsv is "leaf".
Libs=bsv ocpi util platform zynq stratix4 virtex5 virtex6 xilinx

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

HdlInstallDir:=lib
MyMake=$(MAKE) $(and $(HdlTargets),HdlTargets="$(HdlTargets)") --no-print-directory -C $1 \
  OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) \
  HdlInstallDir=$(call AdjustRelative,$(HdlInstallDir)) \

all: $(Libs) $(Cores)
hdl: all
# enable cores to use libs
$(LocalCores): $(Libs)

clean: uninstall
	$(AT)$(foreach l,$(Libs) $(Cores), $(call MyMake,$l) clean;)

cleanimports:
	$(AT)$(foreach l,$(Libs) $(LocalCores), $(call MyMake,$l) cleanimports;)

diff: diff_cores diff_libs

diff_libs:
	$(AT)$(foreach l,$(Libs), $(call MyMake,$l) DIFF_IMPORTS=sdf diff;)

install:
	$(AT)set -e;$(foreach l,$(Libs) $(Cores),$(call MyMake,$l) install;)

uninstall:
	$(AT)echo Removing all installed HDL primitives and codes from: ./lib
	$(AT)rm -r -f lib

.PHONY: $(Libs) $(Cores)

define MakeCoreOrLib
	$(AT)$(call MyMake,$@)
	$(AT)$(call MyMake,$@) install
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


