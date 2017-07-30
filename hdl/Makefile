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

# Make the hdl assets
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

MyMake=echo ===================$(or $(3),Making) HDL $(1); \
       $(MAKE) --no-print-directory -C $(2)$(1) $(HdlPassTargets)
.PHONY: base portable primitives devices platforms components adapters assemblies clean
ifndef OCPI_CDK_DIR
export OCPI_CDK_DIR=$(shell pwd)/../ocpi
$(info Setting OCPI_CDK_DIR to $(OCPI_CDK_DIR))
endif
all hdl:assemblies

base: portable platforms

portable: components adapters devices cards

adapters: primitives
	$(AT)$(call MyMake,adapters)

cards: primitives
	$(AT)$(call MyMake,cards)

assemblies: base
	$(AT)$(call MyMake,assemblies)

components: primitives
	$(AT)$(call MyMake,components,../) hdl

devices: primitives
	$(AT)$(call MyMake,devices)

platforms: devices adapters cards
	$(AT)$(call MyMake,platforms)

primitives:
	$(AT)$(call MyMake,primitives)

clean:
	$(AT)for i in primitives platforms; do \
		$(call MyMake,$$i,,Cleaning) clean; \
		done
	$(AT)$(call MyMake,components,../,Cleaning) clean
	$(AT)$(call MyMake,devices,,Cleaning) clean
	$(AT)$(call MyMake,adapters,,Cleaning) clean
	$(AT)$(call MyMake,cards,,Cleaning) clean
	$(AT)$(call MyMake,assemblies,,Cleaning) clean
