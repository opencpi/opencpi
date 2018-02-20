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

ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=$(realpath ../../exports)
endif

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  include $(OCPI_CDK_DIR)/include/ocpisetup.mk
endif
DIR=target-$(OCPI_TARGET_DIR)
PROG=$(DIR)/$(APP)
PROG2=$(DIR)/$(APP2)
PROG3=$(DIR)/$(APP3)
OUT= > /dev/null

INCS = -I$(OCPI_INC_DIR) -I$(OCPI_PREREQUISITES_DIR)/ad9361/include/  -I$(OCPI_CDK_DIR)/platforms/$(OCPI_TARGET_PLATFORM)/include/

all: $(PROG) $(PROG2) $(PROG3)

$(DIR):
	mkdir -p $(DIR)

ifdef APP
$(PROG): $(APP).cxx | $(DIR)
	$(AT)echo Building $@...
	$(AT)$(CXX) -g -std=c++11 -Wall $(OCPI_EXPORT_DYNAMIC) -o $@ $(INCS) $^ $(OCPI_LD_FLAGS)
endif

ifdef APP2
$(PROG2): $(APP2).cxx | $(DIR)
	$(AT)echo Building $@...
	$(AT)$(CXX) -g -std=c++11 -Wall $(OCPI_EXPORT_DYNAMIC) -o $@ $(INCS) $^ $(OCPI_LD_FLAGS)
endif

ifdef APP3
$(PROG3): $(APP3).cxx | $(DIR)
	$(AT)echo Building $@...
	$(AT)$(CXX) -g -std=c++11 -Wall $(OCPI_EXPORT_DYNAMIC) -o $@ $(INCS) $^ $(OCPI_LD_FLAGS)
endif

clean::
	$(AT)rm -r -f lib target-* *.*~

