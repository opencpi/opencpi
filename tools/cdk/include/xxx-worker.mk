
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
ImplSuffix:=$($(CapModel)ImplSuffix)
SkelSuffix:=$($(CapModel)SkelSuffix)
SourceSuffix:=$($(CapModel)SourceSuffix)
ImplXmlFiles:=$(foreach w,$(Workers),$(or $(Worker_$w_xml),$(Worker).xml))
$(call OcpiDbgVar,ImplXmlFiles)

# During the makefile reading process we possibly update the
# build parameter file, and always update and include the xxx-params.mk file.
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

IncludeDirs:=$(OCPI_CDK_DIR)/include/$(Model) $(GeneratedDir) $(IncludeDirs)
ifeq ($(origin XmlIncludeDirsInternal),undefined)
  ifeq ($(origin XmlIncludeDirs),undefined)
    ifneq ($(wildcard ../specs),)
      XmlIncludeDirsInternal:=../specs
    endif
  endif
endif
#FIXME: why doesn't lib/components flow from component libraries?
override XmlIncludeDirs:=$(XmlIncludeDirs) . $(XmlIncludeDirsInternal) \
   $(OCPI_CDK_DIR)/lib/components $(OCPI_CDK_DIR)/lib/components/specs
-include $(GeneratedDir)/*.deps

ParamShell:=(\
  mkdir -p $(GeneratedDir) &&\
  ($(MakeRawParams) |\
  $(OcpiGenTool) -D $(GeneratedDir) $(and $(Package),-p $(Package))\
    $(and $(Platform),-P $(Platform)) \
    $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
  $(and $(AssemblyName),-S $(AssemblyName)) \
  -r $(Worker_$(Worker)_xml)) || echo 1\
  )

ifeq ($(filter clean,$(MAKECMDGOALS)),)
  # This is the parameter configuration startup that must be run as the
  # makefile is being read, since it results in a file that the makefile must include.
  $(call OcpiDbgVar,ParamShell)
  X:=$(shell $(ParamShell))
  $(and $X,$(error Failed to process initial parameters for this worker: $X))
  include $(GeneratedDir)/$(Worker)-params.mk
  WorkerParamNames:=\
    $(foreach p, \
      $(filter ParamMsg_$(firstword $(ParamConfigurations))_%,$(.VARIABLES)),\
      $(p:ParamMsg_$(firstword $(ParamConfigurations))_%=%))
  $(call OcpiDbgVar,WorkerParamNames)
endif

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
        $(and $(AssemblyName),-S $(AssemblyName)) \
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
	$(AT)$(OcpiGen) -D $(GeneratedDir) \
              $(and $(AssemblyName),-S $(AssemblyName)) \
	      $(and $(Platform),-P $(Platform)) \
              $(and $(Package),-p $(Package)) -s $<
endif

clean:: cleanfirst
	$(AT)rm -r -f $(GeneratedDir) \
             $(if $(filter all,$($(CapModel)Targets)),\
                  $(call WkrTargetDirWild,*),\
                  $(foreach t,$(CleanTargets) $($(CapModel)Targets),$(call WkrTargetDirWild,$t)))

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

WkrTargetDirWild=$(OutDir)target-*$1

# Function to generate final binary from target: $(call WkrBinary,target,config)
WkrBinary=$(call WkrTargetDir,$1,$2)/$(WkrBinaryName)$(call BF,$2)
# Function to generate object file name from source: $(call WkrObject,src,target,config)
WkrObject=$(call WkrTargetDir,$2,$3)/$(basename $(notdir $1))$(call OBJ,$3)

# Function to make an object from source: $(call WkrMakeObject,src,target,config)
define WkrMakeObject
  # A line is needed here for the "define" to work (no extra eval)
  $(infox WkrMakeObject:$1:$2:$3:$(call WkrObject,$1,$2,$3))
  ObjectFiles_$2_$3 += $(call WkrObject,$1,$2,$3)
  $(call WkrObject,$1,$2,$3): ParamConfig=$3
  $(call WkrObject,$1,$2,$3): \
     $1 $(ImplHeaderFiles)\
     | $(call WkrTargetDir,$2,$3)
	$(Compile_$(subst .,,$(suffix $1)))

endef

################################################################################
# Function to make worker objects depend on impl headers and primitive libraries: 
# $(call WkrWorkerDep,worker,target,config)
define WkrWorkerDep

  $(call WkrObject,$1,$2,$3): TargetDir=$(call WkrTargetDir,$2,$3)
  $(call WkrObject,$1,$2,$3): $(CapModel)Target=$2
  $(call WkrObject,$1,$2,$3): Worker=$1
  $(call WkrObject,$1,$2,$3): \
     $(call ImplHeaderFile,$1) \
     $(foreach l,$(call $(CapModel)LibrariesInternal,$2),$$(call LibraryRefFile,$l,$2))

endef

################################################################################
# Function to do stuff per target per param config:
#   $(call WkrDoTargetConfig,target,config))
define WkrDoTargetConfig
  -include $(call WkrTargetDir,$1,$2)/*.deps
  # The target directory
  $(call WkrTargetDir,$1,$2): | $(OutDir) $(GeneratedDir)
	$(AT)mkdir $$@
  # If object files are separate from the final binary,
  # Make them individually, and then link them together
  ifdef ToolSeparateObjects
    $$(call OcpiDbgVar,CompiledSourceFiles)
    $(foreach s,$(CompiledSourceFiles),$(call WkrMakeObject,$s,$1,$2))
    $(call WkrBinary,$1,$2): $(CapModel)Target=$1
    # Note the use of ls -o -g -l below is to not be affected by
    # user and group names with spaces.
    $(call WkrBinary,$1,$2): $$(ObjectFiles_$1_$2) $$(call ArtifactXmlFile,$1,$2) \
                            | $(call WkrTargetDir,$1,$2)
	$(LinkBinary) $$(ObjectFiles_$1_$2) $(OtherLibraries)
	$(AT)if test -f "$(call ArtifactXmlFile,$1,$2)"; then \
	  $(ToolsDir)/../../scripts/addmeta "$(call ArtifactXmlFile,$1,$2)" $$@; \
	fi
    # Make sure we actually make the final binary for this target
    $(call OcpiDbg,Before all: WkrBinary is "$(call WkrBinary,$1,$2)")
    all: $(call WkrBinary,$1,$2)
  endif

  # If not an application, make the worker object files depend on the impl headers
  $(foreach w,$(Workers),$(call WkrWorkerDep,$w,$1,$2))

endef

################################################################################
# Function to do stuff per target: $(eval $(call WkrDoTarget,target))
define WkrDoTarget
  $(foreach c,$(ParamConfigurations),$(call WkrDoTargetConfig,$1,$c))
endef

# Do all the targets
$(foreach t,$($(CapModel)Targets),$(eval $(call WkrDoTarget,$t)))

################################################################################
# Export support - what we put into the (export) library above us

ifndef WkrExportNames
WkrExportNames+=$(WkrBinaryName)$(call BF,0)
endif
ifdef LibDir
# The default for things in the target dir to export into the component library's
# export directory for this target

# $(call DoLink,<target>,<binary>,<linkname>,<confname>)
MyBBLibFile=$(infox MBB:$1:$2:$3)$(call BBLibFile,$1,$(call HdlRmRv,$(basename $2))$(if $(filter 0,$3),,_c$3),$3,$1)

define DoLink

  $(infox DoLink:$1:$2:$3:$4)
  $$(infox DoLink2:$1:$2:$3:$4)
  LibLinks+=$(LibDir)/$1/$3
  ifeq ($(Model),hdl)
    $(LibDir)/$1/$(basename $3)-generics.vhd: | $(LibDir)/$1
	$(AT)$$(call MakeSymLink2,$(call WkrTargetDir,$1,$4)/generics.vhd,$(LibDir)/$1,$(basename $3)-generics.vhd)
	$(AT)if test -f $(call WkrTargetDir,$1,$4)/$(call HdlRmRv,$(basename $2)).cores; then \
               $$(call MakeSymLink,$(call WkrTargetDir,$1,$4)/$(call HdlRmRv,$(basename $2)).cores,$(LibDir)/$1);\
              fi
  endif
  $(LibDir)/$1/$3: $(call WkrTargetDir,$1,$4)/$2 $(and $(filter hdl,$(Model)),$(LibDir)/$1/$(basename $3)-generics.vhd) | $(LibDir)/$1
	$(AT)echo Creating link to export worker binary: $(LibDir)/$1/$3 '->' $(call WkrTargetDir,$1,$4)/$2
	$(AT)$$(call MakeSymLink2,$(call WkrTargetDir,$1,$4)/$2,$(LibDir)/$1,$3)

  $(infox DoLink3:$1:$(HdlToolSet_$1):$(HdlToolNeedBB_$(HdlToolSet_$1)))
  $$(infox DoLink4:$1:$(HdlToolSet_$1):$(HdlToolNeedBB_$(HdlToolSet_$1)))
  ifdef HdlToolNeedBB_$(HdlToolSet_$1)
      $(infox DLHTNB1:$1:$2:$3:$4==$$(call MyBBLibFile,$1,$2,$4))
      $$(infox DLHTNB2:$1:$2:$3:$4==$$(call MyBBLibFile,$1,$2,$4))
    ifeq ($(and $(filter %_rv,$(basename $2)),$(filter 2,$(words $(HdlCores)))),)
      $$(infox DLHTNB:$1:$2:$3:$4==$$(call MyBBLibFile,$1,$2,$4))
      LibLinks+=$(LibDir)/$1/$(call HdlRmRv,$(basename $3))
      # This will actually be included/evaluated twice
      $(LibDir)/$1/$(call HdlRmRv,$(basename $3)): $$$$(call MyBBLibFile,$1,$2,$4) | $(LibDir)/$1
	$(AT)echo Creating link from $$@ -\> $(call WkrTargetDir,$1,$4)/bb/$(basename $3) to export the stub library.
	$(AT)$$(call MakeSymLink2,$(call WkrTargetDir,$1,$4)/bb/$(call HdlRmRv,$(basename $3)),$(strip\
                                  $(LibDir)/$1),$(call HdlRmRv,$(basename $3)))
    endif
  endif

endef

# Do the links for the various binaries of the worker, for a given param configuration
# $(call DoLinks,<target>)
define DoLinks
  LibDirs+=$(LibDir)/$1
  $(LibDir)/$1: | $(LibDir)
  $(foreach c,$(ParamConfigurations),\
    $(foreach n,$(WkrExportNames),\
      $(foreach l,$(basename $(notdir $n))$(if $(filter 0,$c),,_c$c),\
        $(infox LLL:$n:$l:$1:$(HdlToolSet_$1))\
        $(call DoLink,$1,$(strip\
                           $(if $(or $(filter rcc ocl,$(Model)),\
                                     $(HdlToolRealCore_$(HdlToolSet_$1))),\
                             $(notdir $n),$(call HdlRmRv,$l))\
                   ),$(if $(HdlToolRealCore_$(HdlToolSet_$1)),$l$(suffix $n),$(or $(call HdlRmRv,$l),$l)$(suffix $n)),$c))))

  $$(call OcpiDbgVar,WkrExportNames,In Dolinks )

endef

# These links are to binaries
ifndef HdlSkip
  $(call OcpiDbgVar,WkrExportNames)
  $(foreach t,$($(CapModel)Targets),$(infox $(call DoLinks,$t))$(eval $(call DoLinks,$t)))
endif
LibLinks+=$(LibDir)/$(Worker)-params.xml
$(LibDir)/$(Worker)-params.xml: $(GeneratedDir)/$(Worker)-params.xml | $(LibDir)
	$(AT)$(call MakeSymLink,$(GeneratedDir)/$(Worker)-params.xml,$(LibDir))


$(call OcpiDbgVar,LibLinks,Before all:)
links: $(LibLinks)
all: links
$(LibDir) $(LibDirs):
	$(AT)mkdir -p $@

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

