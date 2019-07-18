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

##########################################################################################
# This file defines the xilinx13_3 software platform.
# It sets platform variables as necessary to override the defaults in the file:
#   include/platform-defaults.mk file.
# See that file for a description of valid variables and their defaults.

include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
# Here we require Vivado SDK version 2013.4 for platform xilinx13_3
OCPI_XILINX_VIVADO_SDK_VERSION:=2013.4
tooldir:=$(OcpiXilinxEdkDir)/gnu/arm/lin/bin
ifeq ($(wildcard $(tooldir)),)
  $(error When setting up to build for zynq for $(OCPI_TARGET_PLATFORM), cannot find $(tooldir). Perhaps the EDK was not installed\
          when Xilinx tools were installed? The non-default Xilinx environment settings were: \
          $(foreach v,$(filter OCPI_XILINX%,$(.VARIABLES)), $v=$($v)))
endif
OcpiCrossCompile=$(tooldir)/arm-xilinx-linux-gnueabi-
OcpiCFlags+=-mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9
OcpiCXXFlags+=-mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9
OcpiStaticProgramFlags=-rdynamic
OcpiKernelDir=release/kernel-headers
OcpiPlatformOs=linux
OcpiPlatformOsVersion=x13_3
OcpiPlatformArch=arm
OcpiPlatformPrerequisites=busybox:xilinx13_3 gdb:xilinx13_3 rsync:xilinx13_3
