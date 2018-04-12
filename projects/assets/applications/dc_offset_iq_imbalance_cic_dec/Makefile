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

include $(OCPI_CDK_DIR)/include/application.mk

.SILENT: show
.PHONY: clean show


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
Zynq-Based Platform - setup environment
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

cd /mnt/ocpi_assets/applications/dc_offset_iq_imbalance_cic_dec

export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/lib:$$(pwd)/../../hdl/assemblies/$${PWD##*/}:$$(pwd)/../../exports/lib
#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute
HDL (Matchstiq-Z1);
ocpirun -v -d -t 1 app_matchstiq_z1_dc_offset_corr_iq_imbalance_fixer_cic_dec_file_write.xml
HDL (Zedboard):
ocpirun -v -d -t 1 app_zipper_dc_offset_corr_iq_imbalance_fixer_cic_dec_file_write.xml
----------------------------------------------------------------------------
Development Computer - verify output data
python ./scripts/plotAndFft.py odata/output_file.bin complex 65536 1000000 &
----------------------------------------------------------------------------
endef
export showhelp
