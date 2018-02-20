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

# The zed_ise platform.
Libraries=axi zynq sdp
ParamValues_useGP1=0/1
Configurations=\
  base \
	cfg_0rx_1tx_fmcomms_2_3_lpc_lvds \
	cfg_0rx_2tx_fmcomms_2_3_lpc_lvds \
	cfg_1rx_0tx_fmcomms_2_3_lpc_lvds \
	cfg_1rx_1tx_fmcomms_2_3_lpc_lvds \
	cfg_1rx_2tx_fmcomms_2_3_lpc_lvds \
	cfg_2rx_0tx_fmcomms_2_3_lpc_lvds \
	cfg_2rx_1tx_fmcomms_2_3_lpc_lvds \
	cfg_2rx_2tx_fmcomms_2_3_lpc_lvds \
	gp1 \
	zed_zipper_fmc_rx_tx \
	zed_zipper_fmc_rx \
	zed_zipper_fmc_tx

ExportFiles=zed_ise.ucf zed_ise.ut zed_ise.mk testbias_zed_ise_base.bitz

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  $(shell \
    test -e zed_ise.xml || sed "s/'zed'/'zed_ise'/" ../zed/zed.xml > zed_ise.xml; \
    test -e zed_ise.vhd || sed "s/zed_worker/zed_ise_worker/;" ../zed/zed.vhd > zed_ise.vhd; \
    mkdir -p gen; \
    for c in $(filter-out base,$(Configurations)); do \
      test -e gen/$$c.xml || ln -s ../../zed/$${c//zed_ise/zed}.xml gen/$$c.xml; \
    done)
endif


include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

zed_ise.xml: ../zed/zed.xml; sed "s/'zed'/'zed_ise'/" $< > $@

clean::
	rm -f zed_ise.{xml,vhd}
