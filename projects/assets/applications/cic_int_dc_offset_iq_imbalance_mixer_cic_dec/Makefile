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

$(if $(realpath $(OCPI_CDK_DIR)),,\$(error The OCPI_CDK_DIR environment variable is not set correctly.))
# This is the application Makefile for the "cic_int_dc_offset_iq_imbalance_mixer_cic_dec" application
# If there is cic_int_dc_offset_iq_imbalance_mixer_cic_dec.cc (or cic_int_dc_offset_iq_imbalance_mixer_cic_dec.cxx) file, it will be assumed to be a C++ main program to build and run
# If there is a cic_int_dc_offset_iq_imbalance_mixer_cic_dec.xml file, it will be assumed to be an XML app that can be run with ocpirun.
# The RunArgs variable can be set to a standard set of arguments to use when executing either.
include $(OCPI_CDK_DIR)/include/application.mk

.SILENT: show
.PHONY: clean show

# build the unit test input data
all: tap_gen 

tap_gen:
	$(eval TARGET_FREQ=5000)
	$(eval SAMPLE_FREQ=2062500)
	$(eval INPUT_DATA_AMPLITUDE=1024)
	$(eval NUM_CYCLES=16)
	python ./scripts/test_sinewave.py -g idata/input_file.bin $(TARGET_FREQ) $(SAMPLE_FREQ) $(INPUT_DATA_AMPLITUDE) $(NUM_CYCLES)

clean::
	rm -rf idata/* odata/*

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
----------------------------------------------------------------------------
The instructions below are used to execute an application on a remote
platform like the Matchstiq-Z1 or Zedboard/Zipper.

There are four steps for executing an application:
1) Build ACI and/or input test data (development system)
2) Setup the OCPI_LIBRARY_PATH (remote system)
3) Execution of application (remote system)
4) Verification of output data (development system)

----------------------------------------------------------------------------
----------------------------------------------------------------------------
Setup:
----------------------------------------------------------------------------
1) Build ACI and/or input test data: (development system)
NOTE: Required only if test data has never been built.
ocpidev build --rcc-platform xilinx13_3
or
ocpidev build --rcc-platform xilinx13_4
----------------------------------------------------------------------------
2) Setup the OCPI_LIBRARY_PATH: (remote system)

NOTE: Reminder to setup the run-time OCPI environment on remote system.
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

NOTE: Convenience cmd to quickly switch to application directory.
cd /mnt/ocpi_assets/applications/cic_int_dc_offset_iq_imbalance_mixer_cic_dec

REQUIRED - Reconfigure to default.
Copy/paste string to terminal of remote system:
export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/$$OCPI_TOOL_PLATFORM/artifacts

REQUIRED - Configure for operation.
Copy/paste string to terminal of remote system:
export OCPI_LIBRARY_PATH+=:$$(pwd)/../../hdl/assemblies/$${PWD##*/}:$$(pwd)/../../artifacts
NOTE: When copy/paste of the above 'export' from the Makefile source file, the redundant '$' must be manually removed.
----------------------------------------------------------------------------
3) Execution of application is specific to platform: (remote system)

3a) Install a loopback cable between Tx and Rx
3b) Launch app using the appropriate OAS for the target platform:
Matchstiq-Z1:
ocpirun -v -d -t 1 app_cic_int_dc_offset_iq_imbalance_mixer_cic_dec_matchstiq_z1.xml
Zedboard/Zipper:
ocpirun -v -d -t 1 app_cic_int_dc_offset_iq_imbalance_mixer_cic_dec_zed_zipper.xml
----------------------------------------------------------------------------
4) Verification of output data: (development system)
python ./scripts/plotAndFft.py odata/out_app_cic_int_dc_offset_iq_imbalance_mixer_cic_dec_txrx.bin complex 131072 2062500 &
----------------------------------------------------------------------------
----------------------------------------------------------------------------
endef
export showhelp


