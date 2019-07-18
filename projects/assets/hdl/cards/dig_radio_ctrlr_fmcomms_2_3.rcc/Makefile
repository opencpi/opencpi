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

# This is the Makefile for worker dig_radio_ctrlr_fmcomms_2_3.rcc

ifneq (,)
# No-OS library
RccStaticPrereqLibs=ad9361

# Our "platform" functions that the No-OS library calls
RccIncludeDirs+=./supporting/include/no-OS/ad9361/sw/platform_opencpi
SourceFiles=platform.cc

RccIncludeDirs+=./supporting/include/no-OS/ad9361/sw/
RccIncludeDirs+=./supporting/include/
RccIncludeDirs+=./supporting/include/OCPIProjects/
RccIncludeDirs+=./supporting/include/OCPIProjects/AD9361/
RccIncludeDirs+=./supporting/src/OCPIProjects/
RccIncludeDirs+=./supporting/src/OCPIProjects/AD9361/
RccIncludeDirs+=$(OCPI_PREREQUISITES_DIR)/ad9361/include/
endif

include $(OCPI_CDK_DIR)/include/worker.mk

clean::
	rm -rf supporting/html/ supporting/latex/
