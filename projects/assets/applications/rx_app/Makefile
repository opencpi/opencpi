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

$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# This is the application Makefile for the "rx_app" application
# If there is a rx_app.cc (or rx_app.cxx) file, it will be assumed to be a C++ main program to build and run
# If there is a rx_app.xml file, it will be assumed to be an XML app that can be run with ocpirun.
# The RunArgs variable can be set to a standard set of arguments to use when executing either.

include $(OCPI_CDK_DIR)/include/application.mk

clean::
	rm -rf odata/*

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
----------------------------------------------------------------------------
The instructions below are used to test the Rx application on a remote
platform like the Matchstiq-Z1 or Zedboard/Zipper.

There are four steps for executing a test:
1) Build ACI and/or input test data (development system)
2) Setup the OCPI_LIBRARY_PATH (remote system)
3) Execution of application (remote system)
4) Verification of output data (development system)

----------------------------------------------------------------------------
1) Build ACI and/or input test data: (development system)
NOTE: Required only if ACI has never been built.
ocpidev build --rcc-platform xilinx13_3
or
ocpidev build --rcc-platform xilinx13_4
----------------------------------------------------------------------------
2) Setup the OCPI_LIBRARY_PATH: (remote system)

IMPORTANT NOTE: this guide assumes you are operating in NFS/Network mode on the
radio and you have your projects mounted at:
/mnt/ocpi_core and /mnt/ocpi_assets

NOTE: Reminder to setup the run-time OCPI environment on remote system.
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

NOTE: Convenience cmd to quickly switch to application directory.
cd /mnt/ocpi_assets/applications/rx_app

REQUIRED - Configure for Rx operation.
Copy/paste string to terminal of remote system:
export OCPI_LIBRARY_PATH=$$(pwd)/../../hdl/assemblies/dc_offset_iq_imbalance_mixer_cic_dec_timestamper:$$(pwd)/../../artifacts:/mnt/ocpi_core/artifacts
----------------------------------------------------------------------------
3) Execution of application is specific to platform: (remote system)
----------------------------------------------------------------------------

FMCOMMS2:
- Example configuration:
	* Signal generator is set to 2400.8 MHz with an amplitude of -40 dBm.
	* Signal generator is connected to the FMCOMMS2 RX1A SMA.
- Execution commands:
SAMP_RATE_MHZ=2.5
RF_TUNE_FREQ_MHZ=2400
IF_TUNE_FREQ_MHZ=0.1
./<target>/rx_app $$RF_TUNE_FREQ_MHZ $$SAMP_RATE_MHZ -1 24 2.5 -1 $$IF_TUNE_FREQ_MHZ 1 1 FMCOMMS2 RX1A

FMCOMMS3:
- Example configuration:
	* Signal generator is set to 2400.8 MHz with an amplitude of -40 dBm.
	* Signal generator is connected to the FMCOMMS3 RX1A SMA.
- Execution commands:
SAMP_RATE_MHZ=2.5
RF_TUNE_FREQ_MHZ=2400
IF_TUNE_FREQ_MHZ=0.1
./<target>/rx_app $$RF_TUNE_FREQ_MHZ $$SAMP_RATE_MHZ -1 24 2.5 -1 $$IF_TUNE_FREQ_MHZ 1 1 FMCOMMS3 RX1A

Matchstiq-Z1:
- Example configuration:
	* Signal generator is set to 2400.8 MHz with an amplitude of -60 dBm.
	* Signal generator is connected to the Matchstiq-Z1 RX SMA.
- Execute app by copy/paste of the below onto terminal of remote system
SAMP_RATE_MHZ=2.5
RF_TUNE_FREQ_MHZ=2400
IF_TUNE_FREQ_MHZ=0.1
./target-xilinx13_3/rx_app $$RF_TUNE_FREQ_MHZ $$SAMP_RATE_MHZ 400 10 1.25 51 $$IF_TUNE_FREQ_MHZ 1 1 matchstiq_z1

Zedboard/Zipper:
- Example configuration:
	* Signal generator is set to 2400.8 MHz with an amplitude of -40 dBm.
	* Signal generator is connected to the Zipper's MyriadRF's RXTEST SMA.
- Execute app by copy/paste of the below onto terminal of remote system
SAMP_RATE_MHZ=2.5
RF_TUNE_FREQ_MHZ=2400
IF_TUNE_FREQ_MHZ=0.1
./target-xilinx13_3/rx_app $$RF_TUNE_FREQ_MHZ $$SAMP_RATE_MHZ -1 6 1.25 51 $$IF_TUNE_FREQ_MHZ 1 1 zipper

----------------------------------------------------------------------------
4) Verification of output data: (development system)
SAMP_RATE_HZ=2500000
python ./scripts/plotAndFftAndTime.py odata/rx_app_shortened.out complex 65536 $$SAMP_RATE_HZ 16352 &

Verify that the signal generator tone exists at 700,000 Hz in the plotted baseband signal FFT (calculated as signal generator frequency - RF tune frequency - IF tune frequency = 2400.8 MHz - 2400 MHz - 0.1 MHz = 700,000Hz).

Alternatively, use non-shortened rx_app_raw.out (results in potentially slow plot script)
SAMP_RATE_HZ=2500000
python ./scripts/plotAndFftAndTime.py odata/rx_app_raw.out complex 65536 $$SAMP_RATE_HZ 16352 &
----------------------------------------------------------------------------
----------------------------------------------------------------------------
endef
export showhelp
