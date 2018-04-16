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

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

include $(OCPI_CDK_DIR)/include/application.mk

.SILENT: show
.PHONY: clean show

all: gen_input

# build unit test input data
gen_input:
	python scripts/gen_rrcos_taps.py 128 0.95 `echo "1/64000" | bc -l` 3200000 16384 idata/tx_rrcos_taps.dat

clean::
	rm -rf odata/* scripts/*.pyc

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Instructions for setup/execution on remote platforms '(i.e. Matchstiq-Z1, Zedboard)'
These steps may be repeated to test each available HDL assembly
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Setup:
----------------------------------------------------------------------------
Development Computer - generate filter coefficients
'make'
----------------------------------------------------------------------------
Zynq-Based Platform - setup environment
. /mnt/card/opencpi/mynetsetup.sh {IPADDRESS}
cd /mnt/ocpi_assets/applications/zp_16i16o_fir_real_phase_to_amp_cordic

HDL:
export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/lib/components/rcc:$$(pwd)/../../hdl/assemblies/$${PWD##*/}

#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute
HDL: 'ocpirun -v -d app_zp_16i16o_fir_real_phase_to_amp_cordic_filerw.xml'
----------------------------------------------------------------------------
Development Computer - verify output data
'python ./scripts/plotAndFft.py odata/out_app_zp_16i16o_fir_real_phase_to_amp_cordic_filerw.bin complex 8192 1 &'
----------------------------------------------------------------------------
endef
export showhelp

