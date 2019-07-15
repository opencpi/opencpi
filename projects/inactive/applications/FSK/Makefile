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

.SILENT: show
.PHONY: show

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
