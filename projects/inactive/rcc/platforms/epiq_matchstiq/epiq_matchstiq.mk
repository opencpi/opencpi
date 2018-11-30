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

# This section is currently commented out. I'm not sure if HDL for matchstiq will need it...
# Setup to build this target
# OcpiThisFile=$(lastword $(MAKEFILE_LIST))
# include $(dir $(OcpiThisFile))/xilinx-target.mk
# f:=$(OCPI_XILINX_EDK_DIR)/gnu/arm/lin/bin
# ifeq ($(wildcard $f),)
#   $(error When setting up to build for zed, OCPI_XILINX_EDK_DIR is "$(OCPI_XILINX_EDK_DIR)". Cannot find $f. Perhaps the EDK was not installed when Xilinx tools were installed?).
# endif
MatchTool=cs_lite_match
OcpiCrossCompile=$(OCPI_PREREQUISITES_DIR)/$(MatchTool)/$(OCPI_TOOL_PLATFORM)/bin64/arm-none-linux-gnueabi-
MatchFlags=-Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 \
           -mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9 -Wno-psabi
OcpiRequiredCFlags:=$(patsubst -grecord-gcc-switches,-frecord-gcc-switches,$(OcpiRequiredCFlags))
OcpiRequiredCXXFlags:=$(patsubst -grecord-gcc-switches,-frecord-gcc-switches,$(OcpiRequiredCXXFlags))
OcpiCFlags+=$(MatchFlags)
OcpiCXXFlags+=$(MatchFlags)
OcpiStaticProgramFlags=-rdynamic
OcpiPlatformPrerequisites=$(MatchTool) busybox:epiq_matchstiq
OcpiKernelDir=$(or $(AV_MATCH_VENDOR_REPO),vendor)/zynq_kernel
OcpiPlatformOs=linux
OcpiPlatformOsVersion=zynq
OcpiPlatformArch=arm_cs
# CLOCK_MONOTONIC_RAW is not available
OcpiGetTimeClockId=CLOCK_MONOTONIC
