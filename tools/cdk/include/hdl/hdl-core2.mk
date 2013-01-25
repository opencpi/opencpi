
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

# Internal file shared between primitive cores and workers
# This file basically imports a hard core or builds one from a library

ifndef Top
Top:=$(Core)
endif
LibName=$(Core)

ifdef HdlToolRealCore
################################################################################
# Build the real core if the tools can do it
define DoCore
CoreResults+=$(OutDir)target-$(1)/$(2)$(HdlBin)
$(OutDir)target-$(1)/$(2)$(HdlBin): override HdlTarget:=$(1)
$(OutDir)target-$(1)/$(2)$(HdlBin): LibName=$(HdlToolCoreLibName)
$(OutDir)target-$(1)/$(2)$(HdlBin): TargetDir=$(OutDir)target-$(1)
$(OutDir)target-$(1)/$(2)$(HdlBin): | $$$$(TargetDir)
ifdef PreBuiltCore
$(OutDir)target-$(1)/$(2)$(HdlBin): $(PreBuiltCore)
	$(AT)if test ! -L $@; then \
		$(ECHO) -n Establishing pre-built core file $(2)$(HdlBin) for; \
	        $(ECHO) ' 'target \"$(HdlTarget)\"; \
	     fi
	$(AT)$$(call MakeSymLink2,$(strip \
                $(PreBuiltCore)),$(strip \
                $$(TargetDir)),$(2)$(HdlBin))

else
$(call OcpiDbgVar,CompiledSourceFiles)
$$(call OcpiDbgVar,CompiledSourceFiles)
$(OutDir)target-$(1)/$(2)$(HdlBin): \
   HdlSources=$$(filter-out $$(CoreBlackBoxFile),$$(CompiledSourceFiles))
$(OutDir)target-$(1)/$(2)$(HdlBin): \
      $$(filter-out $$(CoreBlackBoxFile),$$(CompiledSourceFiles)) 
	$(AT)echo Building $(and $(filter-out core,$(HdlMode))) core \"$(2)\" for target \"$$(HdlTarget)\"
	$(AT)$$(HdlCompile)
endif
endef

$(call OcpiDbgVar,CompiledSourceFiles,b2 )
$(call OcpiDbgVar,HdlActualTargets)
$(foreach t,$(HdlActualTargets),$(eval $(call DoCore,$(t),$(Core))))
$(call OcpiDbgVar,CompiledSourceFiles,b3 )

ifdef Imports
$(CoreResults): $(ImportsDir)
endif

all:$(LibResults) $(CoreResults)

$(call OcpiDbgVar,PreBuiltCore)
$(call OcpiDbgVar,CoreResults)
$(call OcpiDbgVar,LibResults)

else
################################################################################
# Just build a library if the tools can't build a core
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib2.mk
endif

################################################################################
# Finally we create the black box/stub library for this core alone if needed
# On two conditions: first that we actually need to do it (e.g. cores and apps),
# second that the tools need it
ifneq ($(filter assembly core,$(HdlMode)),)
ifdef HdlToolNeedBB

#ifeq ($(realpath $(CoreBlackBoxFile)),)
#  $(error CoreBlackBoxFile "$(CoreBlackBoxFile)" does not exist)
#endif

################################################################################
# Make the black box library
# Black box libraries are built when it is necessary to
# have a library containing the black box module in order for
# higher level designs to find the core during synthesis.
# But if we have a prebuilt core, don't bother for simulators
$(call OcpiDbgVar,HdlToolNeedBB)
# A function taking a target and producing the BB result file
OutLibFile=$(OutDir)target-$(1)/$(call HdlToolLibraryFile,$(1),$(Core)_bb)
define DoBBLibraryTarget
BBLibResults+=$(call OutLibFile,$(1))
$(call OutLibFile,$(1)): LibName=$(Core)_bb
$(call OutLibFile,$(1)): Core=$(Core)_bb
# $(call OutLibFile,$(1)): Top=$(Core)
$(call OutLibFile,$(1)): override HdlTarget=$(1)
$(call OutLibFile,$(1)): TargetDir=$(OutDir)target-$(1)
$(call OutLibFile,$(1)): HdlSources=$(CoreBlackBoxFile) 
endef
$(foreach f,$(call HdlToolLibraryTargets,$(HdlActualTargets)),\
  $(eval $(call DoBBLibraryTarget,$(f))))
$(sort $(BBLibResults)): $$(CoreBlackBoxFile) | $$(TargetDir)
	$(AT)$(ECHO) -n Building stub/blackbox library \($@\) for target' '
	$(AT)$(ECHO) $(HdlTarget) from $(CoreBlackBoxFile)
	$(AT)$(HdlCompile)
all: $(BBLibResults)
$(call OcpiDbgVar,BBLibResults)
$(call OcpiDbgVar,HdlActualTargets)

# Install the black box library
$(HdlCoreInstallDir)_bb:
	$(AT)mkdir $@

install_bb: | $(HdlCoreInstallDir)_bb
	$(AT)for f in $(HdlFamilies); do \
	   $(call ReplaceContentsIfDifferent,$(strip \
               $(OutDir)target-$$f/$(HdlToolLibraryResult)_bb),$(strip \
	       $(HdlCoreInstallDir)_bb/$$f)); \
	   done
install: install_bb
endif # of making and installing the black box library
endif
