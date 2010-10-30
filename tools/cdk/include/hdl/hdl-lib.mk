
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
include $(OCPI_CDK_DIR)/include/hdl/hdl.mk
Core=onewire
override SourceFiles+=$(OCPI_CDK_DIR)/include/hdl/onewire.v
AuthoredSourceFiles=$(sort $(SourceFiles))
WorkLibrarySources+=$(OCPI_CDK_DIR)/include/hdl/onewire.v
# If build does not have any HDL components do not try to build any HDL
# components.
ifndef HdlTargets
Targets=
endif
ifndef Targets
# These libraries need to be built for "families", which really just means that the format of the library
# depends on the target to some extent.
Targets=virtex5 virtex6
endif

# all the per-family stuff
define DoLibraryTarget
OutLibFiles+=$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1))

$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): TargetDir=$(1)
$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): Target=$(1)
$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): SourceFiles+=$(SourceFiles_$(1))
$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): $(SourceFiles) $(SourceFiles_$(1)) | $(OutDir)$(1)

$(OutDir)$(1): | $(OutDir)
	$(AT)mkdir $$@

endef

$(foreach t,$(Targets),$(eval $(call DoLibraryTarget,$(t))))

$(OutLibFiles):
	$(AT)echo Building the $(LibName) library for $(Target)
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
	rm -f -r $(foreach f,$(Targets),$(foreach t,$(call FamilyCleanTargets,$(f)),$(InstallDir)/$(f)/$(t)))
	$(foreach f,$(Targets),if ! test -d $(InstallDir)/$(f); then mkdir $(InstallDir)/$(f); fi;\
		cp -r -p $(OutDir)$(f)/$(LibName)/* $(InstallDir)/$(f);)

clean:
	rm -r -f $(foreach f,$(Targets),$(OutDir)$(f))
