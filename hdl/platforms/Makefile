# #####
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# FIXME: create an hdl-platforms.mk template to share among platform developers.
ifndef HdlPlatforms
ifndef HdlPlatform
ifeq ($(MAKECMDGOALS),clean)
HdlPlatforms=$(filter-out %.txt test Makefile common README README.txt lib specs old,$(wildcard *))
else
HdlPlatforms:=ml605
endif
else
HdlPlatforms:=$(HdlPlatform)
endif
endif
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
Package=ocpi
include $(OCPI_CDK_DIR)/include/package.mk

.PHONY: $(HdlPlatforms)

all: $(HdlPlatforms)

$(HdlPlatforms):
	$(AT)echo =============Building platform $@
	$(AT)$(MAKE) --no-print-directory -C $@

clean::
	$(AT)for p in $(HdlPlatforms); do \
	      echo Cleaning platform $$p; \
	      $(MAKE) --no-print-directory -C $$p clean; \
	     done
cleanall:
	$(AT)find . -depth -name gen -exec rm -r -f "{}" ";"
	$(AT)find . -depth -name "target-*" -exec rm -r -f "{}" ";"
	$(AT)rm -r -f lib
