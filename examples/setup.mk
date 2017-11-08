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

Package=local
ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=$(realpath ../../exports)
endif

all:

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
 $(eval $(OcpiEnsureToolPlatform))
  export PATH:=$(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_DIR):$(PATH)
  export OCPI_LIBRARY_PATH=.:$(OCPI_CDK_DIR)/lib/components/rcc:$(OCPI_CDK_DIR)/lib/components/ocl:$(OCPI_CDK_DIR)/lib/hdl/assemblies
endif
OUT= > /dev/null
DIR=target-$(OCPI_TOOL_DIR)
ifdef APP
OCPI_DEBUG=1
OcpiAppNoRun=1
PROG=$(DIR)/$(APP)
# override the name since these are not created by ocpidev (yet)
OcpiApp=$(APP)
include $(OCPI_CDK_DIR)/include/application.mk
endif

clean::
	$(AT)rm -r -f lib target-* *.*~ timeData.raw output_image.jpg test.output
