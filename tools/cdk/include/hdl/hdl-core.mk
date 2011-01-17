
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
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

# This file is for building a core, which will also build the library for it...
# A "core" may be imported, and thus the "import" target, usually based on OCPI_HDL_IMPORT_DIR,
# (re)populates the source files here under the "import" dir, to highlight the fact that
# they are not really edited in place here.

# Constant definitions and those trivially based on the Makefile
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk

ifndef Core
Core=$(CwdName)
endif
LibName=$(Core)
ifdef ImportCore
PreBuiltCore=yes
Imports += $(ImportCore)
CoreFile=$(notdir $(ImportCore))
else
CoreFile=$(Core)$(BF)
endif

ifndef Top
Top:=$(Core)
endif

ifndef Targets
  $(error Variable \"Targets\" for $(Core) must be defined)
endif

ifndef NoBlackBoxLib
ifdef ImportBlackBox
Imports += $(ImportBlackBox)
CoreBlackBoxFile:= $(OutDir)imports/$(notdir $(ImportBlackBox))
else
ifndef CoreBlackBoxFile
  CoreBlackBoxFile:=$(Top)_bb.v
endif
ifeq ($(realpath $(CoreBlackBoxFile)),)
  $(error Core BlackBox File "$(CoreBlackBoxFile)" does not exist)
endif
endif
endif

include $(OCPI_CDK_DIR)/include/hdl/hdl.mk

ifndef NoBlackBoxLib
# List of families from these targets
define DoFamilyTarget
LibResults+=$(OutDir)target-$(1)/$(Core)/$(call LibraryFileTarget,$(1))
$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): | $(OutDir)target-$(1)
$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): LibName=$(Core)
$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): Core=dummy
$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): Target=$(1)
$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): TargetDir=$(OutDir)target-$(1)
$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): CompiledSourceFiles=$(CoreBlackBoxFile) 
$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): $(CoreBlackBoxFile) 
endef
$(foreach f,$(Families),$(eval $(call DoFamilyTarget,$(f))))

$(sort $(LibResults)): $(CoreBlackBoxFile)
	$(AT)echo Building stub/blackbox library for target \"$(Family)\" from \"$(CoreBlackBoxFile)\"
	$(AT)rm -f $(TargetDir)/$(Core)$(BF) $(TargetDir)/*.lso
	$(AT)rm -r -f $(TargetDir)/work
	$(Compile)
endif

# I'm not sure this does anything...
$(info: Targets: $(Targets))
.SECONDEXPANSION:
CoreResults=$(Targets:%=$(OutDir)target-%/$(Core)$(BF))
#$(info cr $(CoreResults))
$(CoreResults): Target=$(@:$(OutDir)target-%/$(Core)$(BF)=%)
$(CoreResults): LibName=work
$(CoreResults): TargetDir=$(@:%/$(Core)$(BF)=%)

# Dependencies for all types
ifdef Imports
$(CoreResults): $(ImportsDir)
endif
$(CoreResults): | $$(TargetDir)
#$(info LR $(LibResults) CR $(CoreResults) F $(Families))
all:$(LibResults) $(CoreResults)

ifdef PreBuiltCore
$(CoreResults): $(OutDir)imports/$(CoreFile)
	$(AT)if test ! -L $@; then \
		echo Exporting pre-built coregen NGC file $(Core)$(BF) for target \"$(Part)\"; \
	     fi
	$(AT)$(call MakeSymLink2,$(OutDir)imports/$(CoreFile),$(TargetDir),$(Core)$(BF))
else
#	$(AT)echo sf1: $@ asf $(CompiledSourceFiles). sf $(SourceFiles)
$(CoreResults): $$(filter-out $$(CoreBlackBoxFile),$$(CompiledSourceFiles)) 
	$(AT)echo Building core \"$(Core)\" for target \"$(Target)\" part \"$(Part)\"
	$(AT)rm -f $@ $(TargetDir)/*.lso
	$(Compile)
endif

ifndef NoBlackBox

# Need to install both the stub library and the core itself
# but the core needs to be installed before the Top-named link
install_core:
	$(AT)for f in $(Targets); do \
	       if ! cmp -s $(OutDir)target-$$f/$(Core)$(BF) $(InstallDir)/$$f/$(Core)$(BF); then \
	         echo Installing core $(Core) for target: $$f; \
		 if ! test -d $(InstallDir); then mkdir $(InstallDir); fi; \
		 if ! test -d $(InstallDir)/$$f; then mkdir $(InstallDir)/$$f; fi; \
		 rm -f $(InstallDir)/$$f/$(Core)$(BF); \
		 cp -r -p -L $(OutDir)target-$$f/$(Core)$(BF) $(InstallDir)/$$f; \
	       fi; \
	     done
install: install_core | $(InstallDir)
	$(AT)for f in $(Families); do \
	       if ! diff -q -r --exclude='*.ngc' $(OutDir)target-$$f/$(LibName) $(InstallDir)/$$f >/dev/null 2>&1; then \
	         echo Installing stub/bb library for core $(Core) for target: $$f; \
	         rm -f -r $(InstallDir)/$$f; \
		 cp -r -p -L $(OutDir)target-$$f/$(LibName) $(InstallDir)/$$f; \
	       fi; \
	     done
	$(AT)$(foreach f,$(Targets),\
		$(if $(findstring $(Core),$(Top)),,\
		  $(call MakeSymLink2,$(InstallDir)/$(f)/$(Core)$(BF),$(InstallDir)/$(f),$(Top)$(BF));))
# endif
ifneq ($(Imports)$(ImportCore)$(ImportBlackBox),)
include $(OCPI_CDK_DIR)/include/hdl/hdl-import.mk
endif
endif
