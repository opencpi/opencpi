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

# This file should be include in makefiles for hdl primitive cores
# compiled from sources or imported as ngc/edif.
# At the moment, pure edif cores are only for single targets.
# There are potentially three different "results" from a core:
# 1. The actual binary file representing a core (like an edif)
# 2. The "stub library" that is needed by some tools to find the core (e.g. xst)
# 3. A library containing the actual implementation
#    (when the tool cannot do layered builds with cores, like most simulators)
#
# The use cases are:
# - Import a prebuilt core
# - Simply drop a prebuilt core into the directory
# - Built a core from sources, imported or not
#
# Depending on the tool:
# - create a black-box stub library to access the tool, using a stub source file
# - export an implementation library if the tool can't really produce a "core"

HdlMode:=core
include $(OCPI_CDK_DIR)/include/util.mk
$(OcpiIncludeProject)
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifndef HdlSkip

ifndef HdlCores
ifdef Core
HdlCores:=$(Core)	
else
HdlCores:=$(CwdName)
endif
endif
ifndef Tops
ifdef Top
Tops:=$(Top)
else
Tops:=$(HdlCores)
endif
endif

#$(info ImportCore=$(ImportCore) PreBuiltCore=$(PreBuiltCore))
ifdef ImportCore
ifdef PreBuiltCore
$(error Cannot set both "PreBuiltCore" and "ImportCore")
endif
Imports += $(ImportCore)
# We don't rename the file when we import it.
CoreFile=$(notdir $(ImportCore))
PreBuiltCore=$(CoreFile)
else
ifdef PreBuiltCore
CoreFile=$(PreBuiltCore)
else
CoreFile=$(Core)$(HdlBin)
endif
endif
HdlCoreInstallDir=$(HdlInstallDir)/$1
HdlCoreInstallDirs=$(HdlCores:$(HdlInstallDir)/%)
$(HdlCoreInstallDirs): | $(HdlInstallDir)
	$(AT)mkdir $@

################################################################################
# First get the black box source file.  We need to get this even if this
# tool won't use it, since that file must be filtered out of the 
# source files actually used to build the core.
# It must be defined before we include the file below
ifdef ImportBlackBox
Imports += $(ImportBlackBox)
endif

CoreBlackBoxFiles=$(strip\
  $(if $(ImportBlackBox),\
        $(OutDir)imports/$(notdir $(ImportBlackBox)),\
          $(Top)_bb.v )\
  $(call HdlExists,$(Top)_pkg.vhd))

################################################################################
# include the shared file (shared with workers) that 
# builds the real core or just a library for the core
include $(OCPI_CDK_DIR)/include/hdl/hdl-core2.mk

HdlInstallLibDir=$(HdlInstallDir)/$(LibName)
$(HdlInstallLibDir):
	$(AT)echo Creating directory $@ for library $(LibName)
	$(AT)mkdir -p $@

# Install the lib-name directory. This is used for non-real cores
# or for real cores when a tool requires source listings (for stubs)
install_lib_dir: | $(HdlInstallLibDir)
	$(AT)for f in $(HdlActualTargets); do \
	  $(call ReplaceIfDifferent,$(strip \
	      $(OutDir)target-$$f/$(LibName)),$(strip \
	      $(HdlInstallLibDir)/$$f)); \
	done

ifdef HdlToolNeedsSourceList_$(HdlToolSet)
# When we need source listings, we enable the creation of the exported
# lib directory as well as the installation of this core's stub files
# in that directory.
HdlStubSources=install_stubs install_lib_dir

install_stubs:
	$(AT)for f in $(HdlActualTargets); do \
          if test -f $(OutDir)target-$$f/$(call RmRv,$(LibName)).sources; then \
            $(call ReplaceIfDifferent,$(strip \
                $(OutDir)target-$$f/$(call RmRv,$(LibName)).sources),$(strip \
                $(OutDir)target-$$f/$(WorkLib)));\
          fi;\
	done
endif

ifdef HdlToolRealCore
# Install the core, however it was buit
install_cores: $(HdlStubSources) | $(HdlCoreInstallDirs)
	$(AT)echo Installing core for targets: $(HdlActualTargets)
	$(AT)for f in $(HdlActualTargets); do \
	  $(foreach c,$(HdlCores),$(strip \
	    $(call ReplaceIfDifferent,$(OutDir)target-$$f/$c$(HdlBin),$(strip \
	   	$(call HdlCoreInstallDir,$c)/$$f));))\
	  done
install: install_cores

# If the core name is not the same as its "top", make links
ifneq ($(Core),$(Top))
install_link: install_cores
	$(AT)$(foreach f,$(HdlActualTargets),\
		 $(call MakeSymLink2,$(strip \
                     $(call HdlCoreInstallDir,$(word 1,$(HdlCores)))/$f/$(word 1,$(HdlCores))$(HdlBin)),$(strip \
                     $(call HdlCoreInstallDir,$(word 1,$(HdlCores)))/$f),$(Top)$(HdlBin));)
install: install_link
endif # for links
else
# Installation is not recursive
install: $(HdlStubSources) install_lib_dir

#| $(HdlInstallDir)
#	$(AT)for f in $(HdlActualTargets); do \
	  $(call ReplaceIfDifferent,$(strip \
	      $(OutDir)target-$$f/$(LibName)),$(strip \
	      $(HdlInstallDir)/$(LibName)/$$f)); \
	done
endif # for building a real core

#$(info ===$(Imports)=$(ImportCore)=$(ImportBlackBox))
ifneq ($(Imports)$(ImportCore)$(ImportBlackBox),)
include $(OCPI_CDK_DIR)/include/hdl/hdl-import.mk
endif # imports
else
install:
endif # HdlSkip
