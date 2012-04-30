
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

# Makefile fragment for all workers

ifneq ($(Worker),)
ifneq ($(Workers),)
$(error Cannot set both Worker and Workers variables in Makefile)
else
Workers=$(Worker)
endif
else
ifeq ($(Workers),)
Worker=$(CwdName)
Workers=$(Worker)
endif
endif
$(call OcpiDbgVar,Workers)
$(call OcpiDbgVar,Worker)

################################################################################
# Tools for metadata and generated files
ToolsTarget=$(HostTarget)
ToolsDir=$(OCPI_CDK_DIR)/bin/$(HostTarget)
ifeq ($(HostSystem),darwin)
DYN_PREFIX=DYLD_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/$(HostTarget)
else
DYN_PREFIX=LD_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/$(HostTarget)
endif
#$(info OCDK $(OCPI_CDK_DIR))
OcpiGen=\
  $(DYN_PREFIX) $(ToolsDir)/ocpigen -M $(GeneratedDir)/$(@F).deps \
    -D $(GeneratedDir) $(XmlIncludeDirs:%=-I%)

################################################################################
# metadata and generated files that are target-independent
ImplXmlFiles=$(Workers:%=%.xml)
ImplSuffix=$($(CapModel)ImplSuffix)
SkelSuffix=$($(CapModel)SkelSuffix)
SourceSuffix=$($(CapModel)SourceSuffix)
ifndef Application
ImplHeaderFiles=$(foreach w,$(Workers),$(GeneratedDir)/$(w)$(ImplSuffix))
SkelFiles=$(foreach w,$(Workers),$(GeneratedDir)/$(w)$(SkelSuffix))
$(ImplHeaderFiles): $(GeneratedDir)/%$(ImplSuffix) : %.xml | $(GeneratedDir)
	$(AT)echo Generating the implementation header file: $@
	$(AT)$(OcpiGen) -i  $<

skeleton: $(SkelFiles)

$(SkelFiles): $(GeneratedDir)/%$(SkelSuffix) : %.xml | $(GeneratedDir)
	$(AT)echo Generating the implementation skeleton file: $@
	$(AT)$(OcpiGen) -s $<
endif
IncludeDirs:=$(OCPI_CDK_DIR)/include/$(Model) $(GeneratedDir) $(IncludeDirs)
override XmlIncludeDirs+=. $(XmlIncludeDirsInternal)
-include $(GeneratedDir)/*.deps
-include $(TargetDir)/*.deps

clean::
	$(AT)rm -r -f $(GeneratedDir) $(TargetDir)

################################################################################
# source files that are target-independent
ifndef Application
ifeq ($(origin WorkerSourceFiles),undefined)
WorkerSourceFiles=$(foreach w,$(Workers),$(w)$(SourceSuffix))
ifeq ($(origin ModelSpecificBuildHook),undefined)
$(call OcpiDbgVar,SourceSuffix)
$(call OcpiDbgVar,WorkerSourceFiles)
# This rule get's run a lot, since it is basically sees that the generated
# skeleton is newer than the source file.
skeleton: $(WorkerSourceFiles)
$(WorkerSourceFiles): %$(SourceSuffix) : $(GeneratedDir)/%$(SkelSuffix)
	$(AT)if test ! -e $@; then \
		echo No source file exists. Copying skeleton \($<\) to $@. ; \
		cp $< $@;\
	fi
endif
endif
endif
# The files we need to compile
AuthoredSourceFiles=$(sort $(SourceFiles) $(WorkerSourceFiles))

################################################################################
# Compilation of source to binary into target directories
# We assume all outputs are in the generated directory, so -M goes in that directory
ifndef WkrBinaryName
ifdef BinaryName
WkrBinaryName=$(BinaryName)
else
WkrBinaryName=$(word 1,$(Workers))
endif
endif
$(call OcpiDbgVar,ModelSpecificBuildHook,Before all: )
all: $(ModelSpecificBuildHook) 

# Function to generate target dir from target: $(call WkrTargetDir,target)
WkrTargetDir=$(OutDir)target-$(1)

# Function to generate final binary from target: $(call WkrBinary,target)
WkrBinary=$(call WkrTargetDir,$(1))/$(WkrBinaryName)$(BF)

# Function to generate object file name from source: $(call WkrObject,src,target)
WkrObject=$(call WkrTargetDir,$(2))/$(basename $(notdir $(1)))$(OBJ)

# Function to make an object from source: $(call WkrMakeObject,src,target)
define WkrMakeObject
# A line is needed here for the "define" to work (no extra eval)
ObjectFiles_$(2) += $(call WkrTargetDir,$(2))/$(basename $(notdir $(1)))$(OBJ)
$(call WkrObject,$(1),$(2)): $(1) $(ImplHeaderFiles) | $(call WkrTargetDir,$(2))
	$(Compile_$(subst .,,$(suffix $(1))))

endef

# Function to make worker objects depend on impl headers: 
# $(call WkrWorkerDep,worker,target)
define WkrWorkerDep

$(call WkrObject,$(1),$(2)): $(GeneratedDir)/$(1)$(ImplSuffix)

endef

################################################################################
# Function to do stuff per target: $(eval $(call WkrDoTarget,target))
define WkrDoTarget
# The target directory
$(call WkrTargetDir,$(1)): | $(OutDir) $(GeneratedDir)
	$(AT)mkdir $$@

# If object files are separate from the final binary,
# Make them individually, and then link them together
ifdef ToolSeparateObjects
$$(call OcpiDbgVar,AuthoredSourceFiles)
$$(call OcpiDbgVar,GeneratedSourceFiles)
$(foreach s,$(AuthoredSourceFiles) $(GeneratedSourceFiles),\
          $(call WkrMakeObject,$(s),$(1)))

$(call WkrBinary,$(1)): $$(ObjectFiles_$(1)) $(ArtifactXmlFile) \
			| $(call WkrTargetDir,$(1))
	$(LinkBinary) $$(ObjectFiles_$(1)) $(OtherLibraries)
	$(AT)if test -f "$(ArtifactXmlFile)"; then \
		(cat $(ArtifactXmlFile); \
                 sh -c 'echo X$$$$4' `ls -l $(ArtifactXmlFile)`) >> $$@; \
	fi
endif
# If not an application, make the worker object files depend on the impl headers
ifndef Application
$(foreach w,$(Workers),$(call WkrWorkerDep,$(w),$(1)))
endif
# Make sure we actuall make the final binary for this target
$(call OcpiDbg,Before all: WkrBinary is "$(call WkrBinary,$(1))")
all: $(call WkrBinary,$(1))
endef
# Do all the targets
$(foreach t,$($(CapModel)Targets),$(eval $(call WkrDoTarget,$(t))))

################################################################################
# Export support - what we put into the (export) library above us

ifdef LibDir
# The default for things in the target dir to export into the component library's
# export directory for this target
ifndef WkrExportNames
WkrExportNames=$(WkrBinaryName)$(BF)
endif

define DoLink
LibLinks+=$(LibDir)/$(1)/$(notdir $(2))
$(LibDir)/$(1)/$(notdir $(2)): $(OutDir)target-$(1)/$(2) | $(LibDir)/$(1)
	$(AT)$$(call MakeSymLink,$$^,$(LibDir)/$(1))
endef

define DoLinks

LibDirs+=$(LibDir)/$(1)
$(LibDir)/$(1): | $(LibDir)
$(foreach n,$(WkrExportNames),$(call DoLink,$(1),$(n)))
endef

$(foreach t,$($(CapModel)Targets),$(eval $(call DoLinks,$(t))))


$(call OcpiDbgVar,LibLinks,Before all:)
all: $(LibLinks)

$(LibDir) $(LibDirs):
	$(AT)mkdir $@

endif # LibDir to export into

################################################################################
# Cleanup.  Tricky/careful removal of skeletons that were copied up into the 
#           directory.
clean::
	$(AT)for s in $(AuthoredSourceFiles); do \
	   sk=$(GeneratedDir)/`echo $$s | sed s~$(SourceSuffix)$$~$(SkelSuffix)~`; \
	   if test -e $$s -a -e $$sk; then \
	      sed 's/GENERATED ON.*$$//' < $$s > $(GeneratedDir)/$$s; \
	      if (sed 's/GENERATED ON.*$$//' < $$sk | \
                  cmp -s - $(GeneratedDir)/$$s); then \
		echo Source file \($$s\) identical to skeleton file \($$sk\).  Removing it.; \
	        rm $$s; \
	      fi; \
	    fi; \
	done
