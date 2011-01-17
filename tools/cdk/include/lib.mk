
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

# A component library, consisting of different models built for different targets
# The expectation is that a library has spec XML in the top level, and subdirectories for each implementation.
# Active implementations are lists in the Implementations variable
# This this makefile just names the library and lists implementations to be built.
# The name of an implementation subdirectory includes its authoring model as the file extension.
# This specification of model is redundant with the "include" line in the makefile inside the directory
# We also list the targets per model.
# A component library, consisting of different models built for different targets
include $(OCPI_CDK_DIR)/include/util.mk
ifndef LibName
LibName=$(CwdName)
endif
# we need to factor the model-specifics our of here...
include $(OCPI_CDK_DIR)/include/hdl/hdl.mk
XmImplementations=$(filter %.xm,$(Implementations))
RccImplementations=$(filter %.rcc,$(Implementations))
HdlImplementations=$(filter %.hdl,$(Implementations))
LibDir=$(OutDir)lib
GenDir=$(OutDir)gen
Models=xm rcc hdl
CapModels=$(foreach m,$(Models),$(call Capitalize,$(m)))
LibDirs=$(foreach m,$(CapModels),$(foreach ht,$($(m)Targets),$(LibDir)/$(call UnCapitalize,$(m))/$(ht)))
XmlIncludeDirs+=specs
export AT
# default is what we are running on

build_targets := speclinks

ifneq "$(XmTargets)" ""
build_targets += xm
endif

ifneq "$(RccTargets)" ""
build_targets += rcc
endif

ifneq "$(HdlTargets)" ""
build_targets += hdl
endif

# function to build the targets for an implemention.
#  First arg is model
#  second is implementation directory
ifdef OCPI_COMPONENT_OUT_DIR 
PassOutDir=OCPI_COMPONENT_OUT_DIR=$(call AdjustRelative,$(OutDir:%/=%))
endif
BuildImplementation=\
    set -e; for t in $($(call Capitalize,$(1))Targets); do \
	if ! test -d $(LibDir)/$(1)/$$t; then \
            mkdir $(LibDir)/$(1)/$$t; \
	fi; \
        $(ECHO) Building $(call ToUpper,$(1)) implementation $(2) for target $$t; \
	$(MAKE) -C $(2) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) Target=$$t \
	       LibDir=$(call AdjustRelative,$(LibDir)/$(1)) \
	       GenDir=$(call AdjustRelative,$(GenDir)/$(1)) \
	       $(PassOutDir) \
               VerilogIncludeDirs=$(call AdjustRelative,$(VerilogIncludeDirs)) \
               XmlIncludeDirsInternal=$(call AdjustRelative,$(XmlIncludeDirs));\
    done; \

BuildModel=\
	$(AT)set -e;if test "$($(call Capitalize,$(1))Implementations)"; then \
	  for i in $($(call Capitalize,$(1))Implementations); do \
		if test ! -d $$i; then \
			echo Implementation \"$$i\" has no directory here.; \
			exit 1; \
		else \
			$(call BuildImplementation,$(1),$$i) \
		fi;\
	  done; \
        fi

CleanModel=\
	$(AT)if test "$($(call Capitalize,$(1))Implementations)"; then \
	  for i in $($(call Capitalize,$(1))Implementations); do \
		if test -d $$i; then \
			for t in $($(call Capitalize,$(1))Targets); do \
				$(ECHO) Cleaning $(call ToUpper,$(1)) implementation $$i for target $$t; \
				$(MAKE) -C $$i $(PassOutDir) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) Target=$$t clean; \
			done; \
		fi;\
	  done; \
        fi

all: $(build_targets)

speclinks: | $(OutDir)lib
	$(AT)$(foreach f,$(wildcard specs/*_spec.xml) $(wildcard specs/*_protocol*.xml),$(call MakeSymLink,$(f),$(OutDir)lib);)

$(OutDir)lib $(OutDir)gen: |$(OutDir)
	$(AT)mkdir $@

$(Models:%=$(OutDir)lib/%): | $(OutDir)lib
	$(AT)mkdir $@

$(Models:%=$(OutDir)gen/%): | $(OutDir)gen
	$(AT)mkdir $@

xm: speclinks | $(OutDir)lib/xm
	$(call BuildModel,xm)

rcc: speclinks | $(OutDir)lib/rcc
	$(call BuildModel,rcc)

# The stubs library depend only on the generated hdl defs files
# Someday we'll need the VHDL -or- Verilog defs files
define DoDefs
hdldefs+=$(1).hdl/gen/$(1)_defs.vh
$(1).hdl/gen/$(1)_defs.vh: $(1).hdl
endef
$(foreach i,$(HdlImplementations),$(eval $(call DoDefs,$(firstword $(subst ., ,$(i))))))

define DoFamily
hdlstubs+=$(LibDir)/hdl/$(1)/$(call LibraryFileTarget,$(1),$(LibName))
$(LibDir)/hdl/$(1)/$(call LibraryFileTarget,$(1),$(LibName))): Family=$(1)
$(LibDir)/hdl/$(1)/$(call LibraryFileTarget,$(1),$(LibName))): $(hdldefs) | $(LibDir) $(LibDir)/hdl $(GenDir)/hdl
	$(AT)echo Building HDL stub library for $(1) for this component library: \"$(LibName)\"
	$(AT)$(MAKE) -C $(GenDir)/hdl -L -f $(call AdjustRelative2,$(OCPI_CDK_DIR))/include/hdl/hdl-lib.mk \
		OCPI_CDK_DIR=$(call AdjustRelative2,$(OCPI_CDK_DIR)) LibName=$(LibName) Targets=$(1)
	$(AT)echo Exporting the stub library for $(1)
	$(AT)rm -r -f $(LibDir)/hdl/$(1)
	$(AT)mkdir $(LibDir)/hdl/$(1)
	$(AT)cp -r -p $(GenDir)/hdl/target-$(1)/$(LibName)/* $(LibDir)/hdl/$(1)
endef
$(foreach f,\
          $(sort $(foreach t,$(HdlTargets),$(call LibraryAccessTarget,$(t)))),\
	  $(eval $(call DoFamily,$(f))))

hdl: $(HdlImplementations) $(hdlstubs)

cleanxm:
	$(call CleanModel,xm)

cleanrcc:
	$(call CleanModel,rcc)

cleanhdl:
	$(call CleanModel,hdl)

clean:: cleanxm cleanrcc cleanhdl
	$(AT)echo Cleaning library directory for all targets.
	$(AT)rm -fr $(OutDir)lib $(OutDir)gen $(OutDir)

$(HdlImplementations): | $(OutDir)lib/hdl $(OutDir)gen/hdl
	$(AT)$(call BuildImplementation,hdl,$@)

$(RccImplementations): | $(OutDir)lib/rcc
	$(AT)$(call BuildImplementation,rcc,$@)

$(XmImplementations): | $(OutDir)lib/xm
	$(AT)$(call BuildImplementation,xm,$@)

.PHONY: $(XmImplementations) $(RccImplementations) $(HdlImplementations) speclinks
