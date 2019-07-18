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

# This file should be include in makefiles for hdl primitive libraries,
# compiled from sources.  The idea is to do as much precompilation as possible.
# Some tools do more than others, but this is a LIBRARY, which means we are not
# combining things together such that they can't be used separately.

# Tell all the general purpose and toolset make scripts we are building libraries
HdlMode:=library
include $(OCPI_CDK_DIR)/include/util.mk
$(OcpiIncludeProject)
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
.PHONY: stublibrary
ifndef HdlSkip
ifndef LibName
LibName:=$(CwdName)
endif
ifndef WorkLib
WorkLib:=$(LibName)
endif
ifdef HdlToolNeedBB
stublibrary: install
else
stublibrary:
	$(AT)echo No stub library necessary for: $(HdlActualTargets)
endif

include $(OCPI_CDK_DIR)/include/hdl/hdl-lib2.mk

# if there isnt a install dir set assume we are in a primitives library ans install them in ../lib
ifneq ($(HdlInstallDir)$(HdlInstallLibDir),)
  HdlInstallDir=../lib
  $(call OcpiIncludeAssetAndParent,..,hdl)
endif
ifdef HdlToolNeedsSourceList_$(HdlToolSet)
HdlLibsList=install_libs

$(HdlLibsList):
	$(AT)for f in $(HdlActualTargets); do \
          if test -f $(OutDir)target-$$f/$(call RmRv,$(LibName)).libs; then \
	    $(call ReplaceIfDifferent,$(strip \
	        $(OutDir)target-$$f/$(call RmRv,$(LibName)).libs),$(strip \
	        $(OutDir)target-$$f/$(WorkLib)));\
          fi;\
	done

HdlSourcesList=install_sources

$(HdlSourcesList):
	$(AT)for f in $(HdlActualTargets); do \
          if test -f $(OutDir)target-$$f/$(call RmRv,$(LibName)).sources; then \
            $(call ReplaceIfDifferent,$(strip \
	        $(OutDir)target-$$f/$(call RmRv,$(LibName)).sources),$(strip \
	        $(OutDir)target-$$f/$(WorkLib)));\
          fi;\
	done
endif

# This can be overriden
HdlInstallLibDir=$(HdlInstallDir)/$(LibName)
$(HdlInstallLibDir):
	$(AT)echo Creating directory $@ for library $(LibName)
	$(AT)mkdir -p $@

install: $(OutLibFiles) $(HdlLibsList) $(HdlSourcesList) | $(HdlInstallLibDir)
	$(AT)for f in $(HdlActualTargets); do \
	  $(call ReplaceIfDifferent,$(strip \
             $(OutDir)target-$$f/$(WorkLib)),$(strip \
             $(HdlInstallLibDir)/$$f)); \
	done

endif

ifneq ($(Imports)$(ImportCore)$(ImportBlackBox),)
include $(OCPI_CDK_DIR)/include/hdl/hdl-import.mk
endif

ifndef OcpiDynamicMakefile
$(OutLibFiles): Makefile
endif

build: $(OutLibFiles)
install: build
all: install
