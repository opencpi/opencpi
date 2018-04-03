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

# This file is used to build the Application Control Interface api testbench,
# calls make on the underlying unit test hdl assemblies, and is used to test the
# respective component. This file handles hardware workers and has been tested on
# the zed platform.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

APP=calculate_AD9361_BIST_PRBS_RX_BER

include $(OCPI_CDK_DIR)/include/application.mk
#setup.mk

.SILENT: show
.PHONY: run clean show

# build the testbench executable
run: tests

build:
	make -C assemblies

# build the HDL unit test assemblies
all: build

clean::
	rm -rf 7 xx yy .run*; make -C assemblies clean

show:
	echo "$$showhelp"

tests:
	OCPI_LIBRARY_PATH=$(OCPI_LIBRARY_PATH):$(PWD):$(PWD)/../ad9361_config_proxy.rcc/:$(PWD)/../../../../core/components/lib/ ./ad9361_dac_test_app

define showhelp
----------------------------------------------------------------------------
----------------------------------------------------------------------------
endef
export showhelp
