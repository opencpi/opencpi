
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

# Define the SourceFiles variable in a directory full of sources.
# This makefile will create precompiled libraries in both old (pre-v6) and new (v6+)
# formats.  If this library is named "foo", then the created libraries will be in $(OutDir)virtex5/foo
# and $(OutDir)virtex6/foo.  Then "make install" will install them in $(InstallDir)/lib/hdl/foo/virtex{5,6}
# "Family" is used here when it isn't quite accurate.  We are really talking about pre-v6 and v6-post.
# This file is xst-specific.  It should be split so the XST-specifics are elsewhere.
ifndef LibName
LibName=$(CwdName)
endif
ifndef Targets
# These libraries need to be built for "families", which really just means that the format of the library
# depends on the target to some extent.
Targets=virtex5 virtex6
endif
include $(OCPI_CDK_DIR)/include/hdl/hdl.mk
Core=onewire
CompiledSourceFiles:= $(OCPI_CDK_DIR)/include/hdl/onewire.v $(CompiledSourceFiles)
WorkLibrarySources:=$(WorkLibrarySources) $(OCPI_CDK_DIR)/include/hdl/onewire.v

.SECONDEXPANSION:
# Determine families based on targets
OutLibFile=$(OutDir)target-$(1)/$(LibName)/$(call LibraryFileTarget,$(1))
define DoFamily
OutLibFiles+=$(call OutLibFile,$(1))
$(call OutLibFile,$(1)): TargetDir=$(OutDir)target-$(1)
$(call OutLibFile,$(1)): Target=$(1)
endef

$(foreach f,$(Families),$(eval $(call DoFamily,$(f))))

ifdef Imports
$(OutLibFiles): $(ImportsDir)
endif
$(OutLibFiles): $$(CompiledSourceFiles) | $$(TargetDir)
	$(AT)echo Building the $(LibName) primitive library for $(Target)
	$(Compile)

all: $(OutLibFiles)

#	$(AT)(for i in $(SourceFiles);\
#		do echo verilog $(LibName) $(call FindRelative,$(OutDir)$(Family),.)/$$i;\
#	      done;\
#	      echo verilog work $(call FindRelative,$(OutDir)$(Family),$(OCPI_CDK_DIR)/include/hdl)/onewire.v)\
#	         >$(OutDir)$(Family)/bsv.prj
#	$(AT)echo -e set -xsthdpdir . \\n $(XstCmd) -ifn bsv.prj > $(OutDir)$(Family)/bsv.xst
#	cd $(OutDir)$(Family); $(TIME) xst -ifn bsv.xst > xst.out; grep -i error xst.out

install: | $(InstallDir)
	$(AT)for f in $(Targets); do \
	       if ! diff -q -r $(OutDir)target-$$f/$(LibName) $(InstallDir)/$$f >/dev/null; then \
	         echo Installing primitive library $(LibName) for target: $$f; \
	         rm -f -r $(InstallDir)/$$f; \
		 cp -L -r -p $(OutDir)target-$$f/$(LibName) $(InstallDir)/$$f; \
	       fi; \
	     done
ifneq ($(Imports)$(ImportCore)$(ImportBlackBox),)
include $(OCPI_CDK_DIR)/include/hdl/hdl-import.mk
endif
