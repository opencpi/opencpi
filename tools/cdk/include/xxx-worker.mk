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

################################################################################
# metadata and generated files that are target-independent
ImplSuffix:=$($(CapModel)ImplSuffix)
SkelSuffix:=$($(CapModel)SkelSuffix)
SourceSuffix:=$($(CapModel)SourceSuffix)
ImplXmlFiles:=$(foreach w,$(Workers),$(or $(Worker_$w_xml),$(Worker).xml))
$(call OcpiDbgVar,ImplXmlFiles)

-include $(GeneratedDir)/*.deps

# Only workers need the implementation "header" file and the skeleton
# Allow this to be set to override this default
ifeq ($(origin ImplHeaderFiles),undefined)
ImplHeaderFiles=$(foreach w,$(Workers),$(call ImplHeaderFile,$w))
ImplHeaderFile=$(GeneratedDir)/$1$(ImplSuffix)
$(call OcpiDbgVar,ImplHeaderFiles)
# FIXME: HdlPlatform is incorrect here
$(ImplHeaderFiles): $(GeneratedDir)/%$(ImplSuffix) : $$(Worker_%_xml) | $(GeneratedDir)
	$(AT)echo Generating the implementation header file: $@ from $< 
	$(AT)$(call OcpiGen, -D $(GeneratedDir) $(and $(Package),-p $(Package)) \
        $(and $(Assembly),-S $(Assembly)) \
	$(and $(HdlPlatform),-P $(HdlPlatform)) \
	$(and $(PlatformDir),-F $(PlatformDir)) \
	$(HdlVhdlLibraries) -i $<)

ifeq ($(origin SkelFiles),undefined)
  SkelFiles=$(foreach w,$(Workers),$(GeneratedDir)/$w$(SkelSuffix))
endif

# Making the skeleton may also make a default OWD
# The "generate" goal is the generic one for workers, tests, etc.
.PHONY: skeleton generate
generate: skeleton
skeleton:  $(ImplHeaderFiles) $(SkelFiles)
all: skeleton

$(SkelFiles): $(GeneratedDir)/%$(SkelSuffix) : $$(Worker_%_xml) | $(GeneratedDir)
	$(AT)$(OcpiRemoveSkeletons)
	$(AT)echo Generating the implementation skeleton file: $@
	$(AT)$(call OcpiGen, -D $(GeneratedDir) \
              $(and $(Assembly),-S $(Assembly)) \
	      $(and $(Platform),-P $(Platform)) \
              $(and $(PlatformDir),-F $(PlatformDir)) \
              $(and $(Package),-p $(Package)) -s $<)
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
WorkerSourceFiles=$(foreach w,$(Workers),$(strip\
                    $(or $(wildcard $w.cpp_$(subst .,,$(SourceSuffix))),$w$(SourceSuffix))))
ifeq ($(origin ModelSpecificBuildHook),undefined)
$(call OcpiDbgVar,SourceSuffix)
$(call OcpiDbgVar,WorkerSourceFiles)
$(call OcpiDbgVar,AuthoredSourceFiles)
# This rule get's run a lot, since it is basically sees that the generated
# skeleton is newer than the source file.
$(filter %$(SourceSuffix),$(WorkerSourceFiles)): %$(SourceSuffix) : $(GeneratedDir)/%$(SkelSuffix)
	$(AT)if test ! -e $@; then \
		echo No source file exists. Copying skeleton \($<\) to $@. ; \
		cp $< $@;\
	fi
endif
endif
endif
skeleton: $(WorkerSourceFiles)
# $(call CPPGenFile,<file>,<target>,<config>)
CPPGenFile=$(call WkrTargetDir,$2,$3)/$(basename $(notdir $1))$(patsubst .cpp_%,.%,$(suffix $1))
# $(call WkrCPPSource,<file>,<target>,<config>)
define WkrCPPSource
  TargetSourceFiles_$3+=$$(call CPPGenFile,$1,$2,$3)
  $$(call WkrBinary,$2,$3): $$(call CPPGenFile,$1,$2,$3)
  $$(call CPPGenFile,$1,$2,$3): $1 | $(call WkrTargetDir,$2,$3)
	$(AT)gcc -MMD -MP -MF $$@.deps -E -P -std=c99 -xc \
               $$(foreach n,$$(WorkerParamNames), '-DOCPI_PARAM_$$n()=$$(Param_$3_$$n)') $$< | \
               tr '$$$$@`' "\n '" | sed '/^ *$$$$/d' > $$@
endef
MaybeCPPSources=$(call Unique,$(SourceFiles) $(WorkerSourceFiles))
$(call OcpiDbgVar,MaybeCPPSources)
CPPSources=$(strip $(foreach f,$(MaybeCPPSources),$(and $(filter .cpp_%,$(suffix $f)),$f)))
$(call OcpiDbgVar,CPPSources)
AuthoredSourceFiles=$(filter-out $(CPPSources),$(MaybeCPPSources))
$(call OcpiDbgVar,AuthoredSourceFiles)

################################################################################
# Compilation of source to binary into target directories
# We assume all outputs are in the generated directory, so -M goes in that directory
ifndef WkrBinaryName
ifdef BinaryName
WkrBinaryName=$(BinaryName)
else
#WkrBinaryName=$(word 1,$(Workers))
# This cannot be the above since that would mean that this could conflict with
# a single worker for the first of multiple workers in a multi-worker dir.
WkrBinaryName=$(CwdName)
endif
endif
$(call OcpiDbgVar,WkrBinaryName)
$(call OcpiDbgVar,Workers)
$(call OcpiDbgVar,ModelSpecificBuildHook,Before all: )
all: $(ModelSpecificBuildHook) 

WkrTargetDirWild=$(OutDir)target-*$1

# Function to generate final binary from target: $(call WkrBinary,target,config)
WkrBinary=$(call WkrTargetDir,$1,$2)/$(WkrBinaryName)$(call BF,$1,$2)
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
     $$$$($(CapModel)CompileDependencies) $$$$($(CapModel)CompileDependencies_$2)\
     | $(call WkrTargetDir,$2,$3)
	$(AT)echo Compiling $$< for target $2, configuration $3
	$(AT)$(Compile_$(subst .,,$(suffix $1)))

endef

################################################################################
# Function to make worker objects depend on impl headers and primitive libraries: 
# $(call WkrWorkerDep,worker,target,config)
define WkrWorkerDep

  $$(call WkrObject,$1,$2,$3): TargetDir=$$(call WkrTargetDir,$2,$3)
  $$(call WkrObject,$1,$2,$3): $(CapModel)Target=$2
  $$(call WkrObject,$1,$2,$3): $(CapModel)Platform=$$(call $(CapModel)GetPlatform,$2)
  $$(call WkrObject,$1,$2,$3): Worker=$1
  $$(call WkrObject,$1,$2,$3): \
     $$(call ImplHeaderFile,$1) \
     $$(foreach l,$$(call $(CapModel)LibrariesInternal,$2),$$(call LibraryRefFile,$$l,$2))

endef

################################################################################
# Function to do stuff per target per param config:
#   $(call WkrDoTargetConfig,target,config))
define WkrDoTargetConfig
  -include $$(call WkrTargetDir,$1,$2)/*.deps
  # The target directory
  $$(call WkrTargetDir,$1,$2): | $$(OutDir) $$(GeneratedDir)
	$(AT)mkdir $$@
  $$(foreach f,$$(CPPSources),$$(eval $$(call WkrCPPSource,$$f,$1,$2)))
  # If object files are separate from the final binary,
  # Make them individually, and then link them together
  ifdef ToolSeparateObjects
    $$(call OcpiDbgVar,CompiledSourceFiles)
    $$(foreach s,$$(CompiledSourceFiles),$$(eval $$(call WkrMakeObject,$$s,$1,$2)))
    $$(call WkrBinary,$1,$2): $(CapModel)Target=$1
    $$(call WkrBinary,$1,$2): $(CapModel)Platform=$$(call $$(CapModel)GetPlatform,$1)

    $$(call WkrBinary,$1,$2): $$$$(ObjectFiles_$1_$2) $$(call ArtifactXmlFile,$1,$2) $$$$($(CapModel)LinkDependencies_$$$$($(CapModel)Target)) \
                            | $$(call WkrTargetDir,$1,$2)
	$(AT)echo Linking final artifact file \"$$@\" and adding metadata to it...
	$(AT)$$(call LinkBinary,$$(ObjectFiles_$1_$2) $$(OtherLibraries))
	$(AT)$$(and $$(call ArtifactXmlFile,$1,$2),\
	       $$(call OcpiPrepareArtifact,$$(call ArtifactXmlFile,$1,$2),$$@))
    # Make sure we actually make the final binary for this target
    $$(call OcpiDbg,Before all: 1:$1 2:$2 RccTarget:$$(RccTarget). WkrBinary is "$$(call WkrBinary,$1,$2)")
    $$(eval $$(call $(CapModel)WkrBinary,$1,$2))
    all: $$(call WkrBinary,$1,$2)
  endif

  # If not an application, make the worker object files depend on the impl headers
  $$(foreach w,$$(Workers),$$(call WkrWorkerDep,$$w,$1,$2))

endef

################################################################################
# Function to do stuff per target: $(eval $(call WkrDoTarget,target))
$(call OcpiDbgVar,ParamConfigurations)
define WkrDoTarget
  $(foreach c,$(ParamConfigurations),$(call WkrDoTargetConfig,$1,$c))
endef

# Do all the targets
ifneq ($(MAKECMDGOALS),skeleton)
$(call OcpiDbgVar,HdlTargets)
$(call OcpiDbgVar,HdlTarget)
$(foreach t,$($(CapModel)Targets),$(eval $(call WkrDoTarget,$t)))
endif
################################################################################
# Export support - what we put into the (export) library above us

ifndef WkrExportNames
WkrExportNames+=$(WkrBinaryName)$(BF)
endif

# If LibDir is unset, but the parent directory is a library
# and we are currently building a worker (rcc, ocl, hdl),
# set libdir to ../lib/$(Model)
ShouldSetLibDir=$(strip \
  $(if $(and $(filter $(call OcpiGetDirType,../),library),\
             $(or $(filter $(Model),rcc ocl),\
                  $(and $(filter $(Model),hdl),\
	                $(filter $(HdlMode),worker)))),\
    true))
ifndef LibDir
  ifneq ($(ShouldSetLibDir),)
    LibDir=../lib/$(Model)
  endif
endif
ifdef LibDir
# The default for things in the target dir to export into the component library's
# export directory for this target

# $(call DoLink,<target>,<binary>,<linkname>,<confname>,<rmrvname>)
MyBBLibFile=$(infox MBB:$1:$2:$3)$(call BBLibFile,$1,$(call RmRv,$(basename $2))$(if $(filter 0,$3),,_c$3),$3,$1)
define DoLink
  $(infox DoLink:$1:$2:$3:$4:$5)
  $$(infox DoLink2:$1:$2:$3:$4:$5)
  BinLibLinks+=$(LibDir)/$1/$3
  ifeq ($(Model),hdl)
    $(LibDir)/$1/$(basename $3)-generics.vhd: | $(LibDir)/$1
	$(AT)$$(call MakeSymLink2,$(call WkrTargetDir,$1,$4)/generics.vhd,$(LibDir)/$1,$(basename $3)-generics.vhd)
	$(AT)if test -f $(call WkrTargetDir,$1,$4)/$(call RmRv,$(basename $2)).cores; then \
               $$(call MakeSymLink,$(call WkrTargetDir,$1,$4)/$(call RmRv,$(basename $2)).cores,$(LibDir)/$1);\
             fi
  endif
  $$(eval $$(call $(CapModel)WkrBinaryLink,$1,$2,$3,$4,$5))
  $(LibDir)/$1/$3: | $(call WkrTargetDir,$1,$4)/$2 $(and $(filter hdl,$(Model)),$(LibDir)/$1/$(basename $3)-generics.vhd) $(LibDir)/$1
	$(AT)echo Creating link to export worker binary: $(LibDir)/$1/$3 '->' $(call WkrTargetDir,$1,$4)/$2
	$(AT)$$(call MakeSymLink2,$(call WkrTargetDir,$1,$4)/$2,$(LibDir)/$1,$3)

  $(infox DoLink3:$1:$(HdlToolSet_$1):$(HdlToolNeedBB_$(HdlToolSet_$1)))
  $$(infox DoLink4:$1:$(HdlToolSet_$1):$(HdlToolNeedBB_$(HdlToolSet_$1)))
  ifdef HdlToolNeedBB_$(HdlToolSet_$1)
      $(infox DLHTNB1:$1:$2:$3:$4==$$(call MyBBLibFile,$1,$2,$4))
      $$(infox DLHTNB2:$1:$2:$3:$4==$$(call MyBBLibFile,$1,$2,$4))
    ifeq ($(and $(filter %_rv,$(basename $2)),$(filter 2,$(words $(HdlCores)))),)
      $$(infox DLHTNB:$1:$2:$3:$4==$$(call MyBBLibFile,$1,$2,$4))
      BinLibLinks+=$(LibDir)/$1/$5
      # This will actually be included/evaluated twice
      $(LibDir)/$1/$5: | $$$$(call MyBBLibFile,$1,$2,$4) $(LibDir)/$1
	$(AT)echo Creating link from $$@ -\> $$(patsubst %/,%,$$(dir $$(call MyBBLibFile,$1,$2,$4))) to export the stub library.
	$(AT)$$(call MakeSymLink2,$$(patsubst %/,%,$$(dir $$(call MyBBLibFile,$1,$2,$4))),$(strip\
                                  $(LibDir)/$1),$5)
    endif
  endif

endef

# Do the links for the various binaries of the worker, for a given param configuration
# $(call DoLinks,<target>)
define DoLinks
  LibDirs+=$(LibDir)/$1
  $(LibDir)/$1: | $(LibDir)
  $(foreach c,$(ParamConfigurations),\
    $(foreach n,$(call WkrExportNames,$1),\
     $(foreach b,$(basename $(notdir $n)),\
       $(foreach r,$(call RmRv,$b)$(if $(filter 0,$c),,_c$c),
         $(foreach l,$b$(if $(filter 0,$c),,_c$c),\
           $(infox LLL:$c:$n:$b:$r:$l:$1:$(HdlToolSet_$1))\
           $(call DoLink,$1,$(strip\
                             $(if $(or $(filter rcc ocl,$(Model)),\
                                       $(HdlToolRealCore_$(HdlToolSet_$1))),\
                               $(notdir $n),$r)),$(strip\
                     $(if $(HdlToolRealCore_$(HdlToolSet_$1)),$l,$r)$(suffix $n)),$c,$r))))))
  $$(call OcpiDbgVar,WkrExportNames,In Dolinks )

endef

# These links are to binaries
ifndef HdlSkip
  $(call OcpiDbgVar,WkrExportNames)
  $(foreach t,$($(CapModel)Targets),$(infox $(call DoLinks,$t))$(eval $(call DoLinks,$(call $(CapModel)TargetDirTail,$t))))
endif

# The generated build file is done as the makefile is read, so we can use
# a wildcard here, knowing that if it is not here it had no non-default values
ifneq ($(wildcard $(Worker).build),)
  LibLinks+=$(LibDir)/$(Worker)-build.xml
  $(LibDir)/$(Worker)-build.xml: $(Worker).build | $(LibDir)
	$(AT)$(call MakeSymLink2,$(Worker).build,$(LibDir),$(Worker)-build.xml)
else
  LibLinks+=$(LibDir)/$(Worker)-build.xml
  $(LibDir)/$(Worker)-build.xml: $(GeneratedDir)/$(Worker)-build.xml | $(LibDir)
	$(AT)$(call MakeSymLink,$(GeneratedDir)/$(Worker)-build.xml,$(LibDir))
endif

$(call OcpiDbgVar,LibLinks,Before all:)
$(call OcpiDbgVar,BinLibLinks,Before all:)
.PHONY: links binlinks genlinks
genlinks: $$(LibLinks)
binlinks: $$(BinLibLinks)
links: binlinks genlinks
all: links
$(LibDir) $(LibDirs):
	$(AT)mkdir -p $@

endif # LibDir to export into

################################################################################
# Cleanup.  Tricky/careful removal of skeletons that were copied up into the 
#           directory.
cleanfirst::
	$(AT)$(OcpiRemoveSkeletons)

OcpiRemoveSkeletons=\
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

