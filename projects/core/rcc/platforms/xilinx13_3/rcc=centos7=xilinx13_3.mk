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

# This file is not version-specific and just relies on its name for OS version
OcpiThisFile:=$(lastword $(MAKEFILE_LIST))
include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
# Here we require Vivado SDK version 2013.4 for platform xilinx13_3
OCPI_XILINX_VIVADO_SDK_VERSION:=2013.4
zynq_bin_dir:=$(OcpiXilinxEdkDir)/gnu/arm/lin/bin
zynq_cross_host:=arm-xilinx-linux-gnueabi
file:=$(basename $(notdir $(OcpiThisFile)))
zynq_host:=xilinx13_3
Gc_$(zynq_host):=$(zynq_bin_dir)/$(zynq_cross_host)-gcc -std=c99
Gc_LINK_$(zynq_host):=$(Gc_$(zynq_host))
Gc++_$(zynq_host):=$(zynq_bin_dir)/$(zynq_cross_host)-g++ -std=c++0x
Gc++_LINK_$(zynq_host):=$(Gc++_$(zynq_host))
Gc++_MAIN_LIBS_$(zynq_host)=rt dl pthread
Gc++_MAIN_FLAGS_$(zynq_host)=-Xlinker --export-dynamic
