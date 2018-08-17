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

# This file is used to build the Application Control Interface api testbench and
# is used to run the respective application. This file handles hardware workers
# and has been tested on the Matchstiq-Z1 platform.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

include $(OCPI_CDK_DIR)/include/application.mk

#APP=fsk_app

#include setup.mk

.SILENT: show
.PHONY: show

# build the testbench executable
#run: all
#	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(OUT)

# build ...
#all: test

#test: app

all: tap_gen 

idata/tx_rrcos_taps.dat idata/rx_rrcos_taps.dat: tap_gen

tap_gen:
	$(eval numTaps=128)
	$(eval alpha=0.95)
	$(eval baudRate=64000)
	$(eval spb=39)
	$(eval maxTap=4096)
	# Remove old test data files
	rm -rf idata/*.dat idata/*.bin odata/*
	# Create test input data
	python scripts/gen_rrcos_taps.py $(numTaps) $(alpha) `echo "1/$(baudRate)" | bc -l` `echo "$(baudRate)*$(spb)" | bc -l` $(maxTap) idata/tx_rrcos_taps.dat
	python scripts/gen_rrcos_taps.py $(numTaps) $(alpha) `echo "1/$(baudRate)" | bc -l` `echo "$(baudRate)*$(spb)" | bc -l` $(maxTap) idata/rx_rrcos_taps.dat

clean::
	rm -rf idata/*.dat idata/*.bin odata/* scripts/*.pyc

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Instructions for setup/execution on remote platforms '(i.e. Matchstiq-Z1, Zedboard/Zipper)'
One of five available tests may be executed per the following instructions
Test 1: Application Workers only in loopback mode (no RF; no device workers)
Test 2: RX only mode
Test 3: TX only mode
Test 4: TX/RX mode
Test 5: TX/RX mode with baseband loopback within the transceiver chip
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Setup:
----------------------------------------------------------------------------
Development Computer - generate test ACI executable and filter coefficients
'OCPI_TARGET_PLATFORM=xilinx13_3 make'

----------------------------------------------------------------------------
Zynq-Based Platform - setup environment
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

cd /mnt/ocpi_assets/applications/FSK

HDL ('common' export) :
export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/artifacts

Pick one of the following, depending upon the test to be executed:
Test 1:
export OCPI_LIBRARY_PATH=$$OCPI_LIBRARY_PATH:$$(pwd)/../../hdl/assemblies/fsk_filerw:$$(pwd)/../../exports/lib
Test 2:
export OCPI_LIBRARY_PATH=$$OCPI_LIBRARY_PATH:$$(pwd)/../../hdl/assemblies/dc_offset_iq_imbalance_mixer_cic_dec_rp_cordic_fir_real:$$(pwd)/../../exports/lib
Test 3:
export OCPI_LIBRARY_PATH=$$OCPI_LIBRARY_PATH:$$(pwd)/../../hdl/assemblies/mfsk2_zp16_fir_real_phase_to_amp_cordic_cic_int:$$(pwd)/../../exports/lib
Test 4:
export OCPI_LIBRARY_PATH=$$OCPI_LIBRARY_PATH:$$(pwd)/../../hdl/assemblies/fsk_modem:$$(pwd)/../../exports/lib
Test 5:
export OCPI_LIBRARY_PATH=$$OCPI_LIBRARY_PATH:$$(pwd)/../../hdl/assemblies/fsk_modem:$$(pwd)/../../exports/lib

#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute (pick one)
Test 1: ./target-x13_3-arm/FSK filerw
Test 2: ./target-x13_3-arm/FSK rx
Test 3: ./target-x13_3-arm/FSK tx
Test 4: ./target-x13_3-arm/FSK txrx
Test 5: ./target-x13_3-arm/FSK bbloopback
----------------------------------------------------------------------------
Development Computer - verify output data (pick one)
Test 1: eog odata/out_app_fsk_filerw.bin &
Test 2: eog odata/out_app_fsk_rx.bin &
Test 3: Visually inspect the spectrum on a spectrum analyzer.
Test 4: eog odata/out_app_fsk_txrx.bin &
Test 5: eog odata/out_app_fsk_bbloopback.bin &
----------------------------------------------------------------------------
endef
export showhelp
