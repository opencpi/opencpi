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

# This is the makefile contents for the hdl/primitives directory in a project.
# The variables that drive it are:
#
# Libs
# Cores
# ImportCoreDirs

include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

ifndef HdlInstallDir
  HdlInstallDir:=lib
endif
MyMake=$(MAKE) $(and $(HdlTargets),HdlTargets="$(HdlTargets)") --no-print-directory -C $1 \
  OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) \
  HdlInstallDir=$(call AdjustRelative,$(HdlInstallDir)) \

ifdef ImportCoreDirs
  # this will set ImportCores
  include $(OCPI_CDK_DIR)/include/hdl/hdl-import-cores.mk
endif	
ifdef PrimitiveCores
Cores:=$(PrimitiveCores)
endif
MyCores=$(ImportCores) $(Cores)
ifdef PrimitiveLibraries
Libs:=$(PrimitiveLibraries)
endif
all: $(Libs) $(MyCores)
hdl: all
# enable cores to use libs
$(Cores): $(Libs)

clean: uninstall
	$(AT)$(foreach l,$(Libs) $(MyCores), $(call MyMake,$l) clean;)

cleanimports:
	$(AT)$(foreach l,$(Libs) $(MyCores), $(call MyMake,$l) cleanimports;)

install:
	$(AT)set -e;$(foreach l,$(Libs) $(MyCores),$(call MyMake,$l) install;)

uninstall:
	$(AT)echo Removing all installed HDL primitives and codes from: ./lib
	$(AT)rm -r -f lib

.PHONY: $(Libs) $(MyCores)

define MakeCoreOrLib
	$(AT)$(call MyMake,$@)
	$(AT)$(call MyMake,$@) install
endef

$(Libs):
	$(AT)echo ============== For library $@:
	$(MakeCoreOrLib)

$(Cores):
	$(AT)echo ============== For core $@:
	$(MakeCoreOrLib)

$(ImportCores):
	$(AT)echo ============== For imported core $@:
	$(MakeCoreOrLib)
