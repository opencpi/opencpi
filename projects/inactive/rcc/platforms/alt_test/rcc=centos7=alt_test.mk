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
my_bin_dir:=$(OCPI_PREREQUISITES_DIR)/gcc_linaro_arm_gnueabihf/$(OCPI_TOOL_DIR)/bin
my_cross_host:=arm-linux-gnueabihf
file:=$(basename $(notdir $(OcpiThisFile)))
my_host:=$(RccTarget_$(word 2,$(subst =, ,$(subst rcc=,,$(file)))))
Gc_$(my_host):=$(my_bin_dir)/$(my_cross_host)-gcc -std=c99
Gc_LINK_$(my_host):=$(Gc_$(my_host))
Gc++_$(my_host):=$(my_bin_dir)/$(my_cross_host)-g++ -std=c++0x
Gc++_LINK_$(my_host):=$(Gc++_$(my_host))
Gc++_MAIN_LIBS_$(my_host)=rt dl pthread
Gc++_MAIN_FLAGS_$(my_host)=-Xlinker --export-dynamic
