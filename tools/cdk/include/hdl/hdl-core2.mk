
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

ifndef Tops
ifdef Top
Tops:=$(Top)
else
Tops:=$(Core)
endif
endif
ifndef LibName
LibName=$(word 1,$(HdlCores))
endif

$(call OcpiDbgVar,HdlToolRealCore)
$(call OcpiDbgVar,LibName)
ifdef HdlToolRealCore
################################################################################
# Build the real core if the tools can do it $(call DoCore,target,name)
define DoCore
CoreResults+=$(OutDir)target-$1/$2$(HdlBin)
$(OutDir)target-$1/$2$(HdlBin): override HdlTarget:=$1
$(OutDir)target-$1/$2$(HdlBin): TargetDir=$(OutDir)target-$1
$(OutDir)target-$1/$2$(HdlBin): | $$$$(TargetDir)
$(OutDir)target-$1/$2$(HdlBin): Core=$2
$(OutDir)target-$1/$2$(HdlBin): Top=$3
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
$(OutDir)target-$1/$2$(HdlBin): \
   HdlSources=$$(filter-out $$(filter-out %.vhd,$$(CoreBlackBoxFiles)),$$(CompiledSourceFiles))
#$(OutDir)target-$1/$2$(HdlBin): AllCores=$(call HdlCollectCores,$1)
$(OutDir)target-$1/$2$(HdlBin): $$$$(HdlPreCore) \
      $$(filter-out $$(filter-out %.vhd,$$(CoreBlackBoxFiles)) $$(TargetSourceFiles),$$(CompiledSourceFiles)) 
	$(AT)echo Building $(and $(filter-out core,$(HdlMode))) core \"$(2)\" for target \"$$(HdlTarget)\"
	$(AT)$$(call HdlRecordCores,$(basename $(OutDir)target-$1/$2))
	$(AT)$$(HdlCompile)
endif

all: $(OutDir)target-$1/$2$(HdlBin)

endef # DoCore

$(call OcpiDbgVar,CompiledSourceFiles,b2 )
$(call OcpiDbgVar,HdlActualTargets)
$(call OcpiDbgVar,HdlCores)
#$(foreach t,$(HdlActualTargets),$(eval $(call DoCore,$(t),$(Core))))
ifneq ($(MAKECMDGOALS),clean)
$(foreach t,$(HdlActualTargets),\
  $(foreach both,$(join $(HdlCores),$(Tops:%=:%)),\
    $(foreach core,$(word 1,$(subst :, ,$(both))),\
     $(foreach top,$(word 2,$(subst :, ,$(both))),\
       $(eval $(call DoCore,$t,$(core),$(top)))))))
endif
$(call OcpiDbgVar,CompiledSourceFiles,b3 )

ifdef Imports
$(CoreResults): $(ImportsDir)
endif

cores:$(LibResults) $(CoreResults)
all: cores
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
# On two conditions: first that we actually need to do it (e.g. platforms,cores,assemblies),
# second that the tools need it
ifneq ($(findstring $(HdlMode),assembly core platform),)
ifdef HdlToolNeedBB

################################################################################
# Make the black box library
# Black box libraries for cores are built when it is necessary to have a library containing
# the black box module in order for higher level designs to find the core during synthesis.
# Too enable instantiation from verilog this means a foo_bb.v file with an empty module.
# Too enable instantiaiton from VHDL this just means the foo_pkg.vhd file with the module definition
# The bb library is built using the same library name, but in the bb subdirectory
# BUT: it is installed under the foo_bb name
# FIXME: we don't yet build with the VHDL pkg file for instantiating cores from VHDL
$(call OcpiDbgVar,HdlToolNeedBB)

# A function taking a target-dir-name, a libname, and a build target
BBLibFile=$(OutDir)target-$1/bb/$(call HdlToolLibraryFile,$3,$2)

# $(call DoBBLibraryTarget,target-dir-name,libname,target,bbfile)
define DoBBLibraryTarget
BBLibResults+=$(call BBLibFile,$1,$2,$3)
$(call BBLibFile,$1,$2,$3): | $(OutDir)target-$1/bb
$(call BBLibFile,$1,$2,$3): LibName=$2
$(call BBLibFile,$1,$2,$3): Core=onewire
$(call BBLibFile,$1,$2,$3): Top=onewire
$(call BBLibFile,$1,$2,$3): override HdlTarget=$3
$(call BBLibFile,$1,$2,$3): TargetDir=$(OutDir)target-$1/bb
$(call BBLibFile,$1,$2,$3): HdlSources=$4 $(OCPI_CDK_DIR)/include/hdl/onewire.v
$(call BBLibFile,$1,$2,$3): $4 | $$$$(TargetDir)
	$(AT)$(ECHO) -n Building stub/blackbox library \($$@\) for target' '
	$(AT)$(ECHO) $$(HdlTarget) from $4
	$(AT)$$(HdlCompile)

$(OutDir)target-$1/bb:
	$(AT)mkdir -p $$@

endef

$(foreach f,$(HdlActualTargets),\
  $(eval $(call DoBBLibraryTarget,$f,$(word 1,$(HdlCores)),$f,$(CoreBlackBoxFiles))))

cores: $(BBLibResults)

$(call OcpiDbgVar,BBLibResults)
$(call OcpiDbgVar,HdlActualTargets)

HdlCoreBBInstallDir=$(call HdlCoreInstallDir,$(word 1,$(HdlCores)))_bb
# Install the black box library
$(HdlCoreBBInstallDir):
	$(AT)mkdir $@

install_bb: | $(HdlCoreBBInstallDir)
	$(AT)for f in $(HdlFamilies); do \
	   $(call ReplaceContentsIfDifferent,$(strip \
               $(OutDir)target-$$f/bb/$(LibName)),$(strip \
	       $(HdlCoreBBInstallDir)/$$f)); \
	   done
install: install_bb
endif # of making and installing the black box library
endif
