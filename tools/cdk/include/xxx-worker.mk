
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


################################################################################
# metadata and generated files that are target-independent
ImplSuffix=$($(CapModel)ImplSuffix)
SkelSuffix=$($(CapModel)SkelSuffix)
SourceSuffix=$($(CapModel)SourceSuffix)
ImplXmlFiles=$(foreach w,$(Workers),$(or $(Worker_$w_xml),$(Worker).xml))
$(call OcpiDbgVar,ImplXmlFiles)

# During the makefile reading process we will possibly update the
# build parameter file.
# We look at all variables of the form Param_<name>.
RawParamVariables:=$(filter Param_%,$(.VARIABLES)) $(filter ParamValues_%,$(.VARIABLES))
RawParamFile:=$(GeneratedDir)/rawparams.xml
RawParamName=$(if $(filter Param_%,$1),$(1:Param_%=%),$(1:ParamValues_%=%))
RawParamNames:=$(foreach v,$(RawParamVariables),$(call RawParamName,$v))
ifneq ($(words $(RawParamNames)),$(words $(sort $(RawParamNames))))
  $(error Both Param_ and ParamValues_ used for same parameter.)
endif
ParamValue=$(Param_$1)
# FIXME: what are the quoting conventions at work here?
# 1. The core syntax is from our textual encoding of data types.
# 2. Make pretty much allows anything but # and newline in an assignment - both can
#    be escaped using backslash, so in fact there are three special characters
#    #, \, and <newline>, but newlines will never be included.
# 3. So when we output the file for XML, we need to deal with XML quoting conventions.
#    But XML textual data has only two things to protect: < and &.
#    But our format also has backslash encoding too.
# 4. The Values string has commas. Is there anything better?
#    Perhaps if we choose something else?:
#     -- transparent to Make
#     -- transparent to XML
#     -- not a comma
#     -- uncommon to avoid too much quoting, not in any numeric format
#     -- looks reasonable: | is used in BNF
# 5. We need a shell command to put the value into a file.
#    Metacharacters for the shell are: | & ; ( ) < > space tab
#    Backslash protects everything
#    Single quotes can't protect single quotes
#    Double quotes don't protect $ ` \ !
# Since XML already has a mechanism to encode single quotes (&apos;),
# using single quotes is best.
MakeRawParams:= \
  (echo "<parameters>"; \
   $(foreach i,$(RawParamVariables),\
     echo "<parameter name='$(call RawParamName,$i)'$(strip \
                      )$(if $(filter ParamValues_%,$i), values='true')>";\
     echo '$(subst <,&lt;,$(subst ',&apos;,$(subst &,&amp;,$($i))))';\
     echo "</parameter>";) \
   echo "</parameters>")
ifeq ($(filter clean,$(MAKECMDGOALS)),)
MakeRawParamsFile:=$(strip\
  $(if $(wildcard $(RawParamFile)), \
     $(shell $(MakeRawParams) | cmp -s $(RawParamFile) 2>&1 || \
             ($(MakeRawParams) > $(RawParamFile)) 2>&1),\
     $(shell (mkdir -p $(GeneratedDir) ; $(MakeRawParams) > $(RawParamFile)) 2>&1)))
$(and $(MakeRawParamsFile),$(error when processing parameters: $(MakeRawParamsFile)))
#$(info MakeRawParams:$(MakeRawParams), \
   MakeParamsFile:$(MakeRawParamsFile), RawParamValues=$(RawParamValues))
#$(info X:$(foreach v,$(RawParamVariables),name is:$(call RawParamName,$v), value is "$($v)"))
#$(info RawParamNames:$(RawParamNames))
#$(info RawParamVariables:$(RawParamVariables))
endif
ParamFile=$(GeneratedDir)/$1-params.mk
ParamFiles:=$(foreach w,$(Workers),$(call ParamFile,$w))
#$(info PARAMFILES:$(ParamFiles))
LoadWorkerParams:=\
  $(eval WorkerParamNames:=) \
  $(eval -include $(call ParamFile,$(Worker)))
$(ParamFiles): $(GeneratedDir)/%-params.mk: $$(Worker_%_xml) $(RawParamFile) | $(GeneratedDir)
	$(AT)$(OcpiGen) -D $(GeneratedDir) $(and $(Package),-p $(Package)) \
	 $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) -r $(RawParamFile) $<

# Only workers need the implementation "header" file and the skeleton
# Allow this to be set to override this default
ifeq ($(origin ImplHeaderFiles),undefined)
ImplHeaderFiles=$(foreach w,$(Workers),$(call ImplHeaderFile,$w))
ImplHeaderFile=$(GeneratedDir)/$1$(ImplSuffix)
$(call OcpiDbgVar,ImplHeaderFiles)
# FIXME: HdlPlatform is bogus here
$(ImplHeaderFiles): $(GeneratedDir)/%$(ImplSuffix) : $$(Worker_%_xml) | $(GeneratedDir)
	$(AT)echo Generating the implementation header file: $@ from $< 
	$(AT)$(OcpiGen) -D $(GeneratedDir) $(and $(Package),-p $(Package)) \
	$(and $(HdlPlatform),-P $(HdlPlatform)) \
	 $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) -i $< \

ifeq ($(origin SkelFiles),undefined)
SkelFiles=$(foreach w,$(Workers),$(GeneratedDir)/$w$(SkelSuffix))
endif

# Making the skeleton may also make a default OWD
skeleton:  $(ImplHeaderFiles) $(SkelFiles)
all: skeleton

$(SkelFiles): $(GeneratedDir)/%$(SkelSuffix) : $$(Worker_%_xml) | $(GeneratedDir)
	$(AT)echo Generating the implementation skeleton file: $@
	$(AT)$(OcpiGen) -D $(GeneratedDir) $(and $(Package),-p $(Package)) -s $<
endif
IncludeDirs:=$(OCPI_CDK_DIR)/include/$(Model) $(GeneratedDir) $(IncludeDirs)
ifeq ($(origin XmlIncludeDirsInternal),undefined)
ifeq ($(origin XmlIncludeDirs),undefined)
ifneq ($(wildcard ../specs),)
XmlIncludeDirsInternal=../specs
endif
endif
endif
override XmlIncludeDirs+=. $(XmlIncludeDirsInternal) \
   $(OCPI_CDK_DIR)/lib/components $(OCPI_CDK_DIR)/lib/components/specs
-include $(GeneratedDir)/*.deps

clean:: cleanfirst
	$(AT)rm -r -f $(GeneratedDir) \
             $(if $(filter all,$($(CapModel)Targets)),\
                  $(wildcard $(call WkrTargetDir,*)),\
                  $(foreach t,$($(CapModel)Targets),$(call WkrTargetDir,$t)))

################################################################################
# source files that are target-independent
# FIXME: this should not reference an Hdl variable
ifeq ($(findstring $(HdlMode),assembly container),)
ifeq ($(origin WorkerSourceFiles),undefined)
WorkerSourceFiles=$(foreach w,$(Workers),$(w)$(SourceSuffix))
# We must preserve the order of CompiledSourceFiles
#$(call OcpiDbgVar,CompiledSourceFiles)
#AuthoredSourceFiles:=$(strip $(SourceFiles) $(filter-out $(SourceFiles),$(WorkerSourceFiles)))
ifeq ($(origin ModelSpecificBuildHook),undefined)
$(call OcpiDbgVar,SourceSuffix)
$(call OcpiDbgVar,WorkerSourceFiles)
$(call OcpiDbgVar,AuthoredSourceFiles)
# This rule get's run a lot, since it is basically sees that the generated
# skeleton is newer than the source file.
$(WorkerSourceFiles): %$(SourceSuffix) : $(GeneratedDir)/%$(SkelSuffix)
	$(AT)if test ! -e $@; then \
		echo No source file exists. Copying skeleton \($<\) to $@. ; \
		cp $< $@;\
	fi
endif
endif
endif
skeleton: $(WorkerSourceFiles)
AuthoredSourceFiles=$(call Unique,$(SourceFiles) $(WorkerSourceFiles))
$(call OcpiDbgVar,AuthoredSourceFiles)

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
$(call OcpiDbgVar,WkrBinaryName)
$(call OcpiDbgVar,Workers)
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
ObjectFiles_$2 += $(call WkrTargetDir,$2)/$(basename $(notdir $1))$(OBJ)
$(call WkrObject,$1,$2): \
   $1 $(ImplHeaderFiles) \
   $(if $(filter $1,$(WorkerSourceFiles)),,$(call ParamFile,$(word 1,$(Workers)))) \
    | $(call WkrTargetDir,$2)
	$(Compile_$(subst .,,$(suffix $1)))

endef

# Function to make worker objects depend on impl headers and primitive libraries: 
# $(call WkrWorkerDep,worker,target)
define WkrWorkerDep

$(call WkrObject,$1,$2): TargetDir=$(OutDir)target-$2
$(call WkrObject,$1,$2): $(CapModel)Target=$2
$(call WkrObject,$1,$2): Worker=$1
$(call WkrObject,$1,$2): \
   $(call ParamFile,$1) \
   $(call ImplHeaderFile,$1) \
   $(foreach l,$(call $(CapModel)LibrariesInternal,$2),$$(call LibraryRefFile,$l,$2))

endef

################################################################################
# Function to do stuff per target: $(eval $(call WkrDoTarget,target))
define WkrDoTarget
-include $(call WkrTargetDir,$1)/*.deps
# The target directory
$(call WkrTargetDir,$1): | $(OutDir) $(GeneratedDir)
	$(AT)mkdir $$@

# If object files are separate from the final binary,
# Make them individually, and then link them together
ifdef ToolSeparateObjects
$$(call OcpiDbgVar,CompiledSourceFiles)
$(foreach s,$(CompiledSourceFiles),$(call WkrMakeObject,$(s),$1))

$(call WkrBinary,$(1)): $(CapModel)Target=$1
$(call WkrBinary,$(1)): $$(ObjectFiles_$1) $$(call ArtifactXmlFile,$1) \
			| $(call WkrTargetDir,$1)
	$(LinkBinary) $$(ObjectFiles_$1) $(OtherLibraries)
	$(AT)if test -f "$(ArtifactXmlFile)"; then \
		(cat $(ArtifactXmlFile); \
                 bash -c 'echo X$$$$4' `ls -l $(ArtifactXmlFile)`) >> $$@; \
	fi
endif
# Make sure we actuall make the final binary for this target
$(call OcpiDbg,Before all: WkrBinary is "$(call WkrBinary,$1)")
all: $(call WkrBinary,$1)

# If not an application, make the worker object files depend on the impl headers
$(foreach w,$(Workers),$(eval $(call WkrWorkerDep,$w,$1)))

endef

# Do all the targets
$(foreach t,$($(CapModel)Targets),$(eval $(call WkrDoTarget,$t)))

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
$$(call OcpiDbgVar,WkrExportNames,In Dolinks )
endef

$(call OcpiDbgVar,WkrExportNames)
$(foreach t,$($(CapModel)Targets),$(eval $(call DoLinks,$(t))))


$(call OcpiDbgVar,LibLinks,Before all:)
all: $(LibLinks)

$(LibDir) $(LibDirs):
	$(AT)mkdir $@

endif # LibDir to export into

################################################################################
# Cleanup.  Tricky/careful removal of skeletons that were copied up into the 
#           directory.
cleanfirst::
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

