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

# This file contains various utilities for hdl makefiles.
# It is distinguishd from hdl-pre.mk in that it doesn't really mess with any variables
# Thus HDL utility stuff goes here unless it is order sensitive, in which case it
# is in hdl-pre.mk

ifndef __HDL_MAKE_MK__
__HDL_MAKE_MK__=x
Model:=hdl
# This file is included by the various levels of hdl makefiles (not the leaf makefiles)
# i.e. a Makefile file that does other makefiles.
# Note that targets are generally families except when a primitive core is actually part-specific.
ifdef HdlTarget
override HdlTarget:=$(patsubst %_pf,%,$(HdlTarget))
endif
ifdef HdlTargets
override HdlTargets:=$(patsubst %_pf,%,$(HdlTargets))
endif
ifdef OnlyTargets
override OnlyTargets:=$(patsubst %_pf,%,$(OnlyTargets))
endif
ifdef ExcludeTargets
override ExcludeTargets:=$(patsubst %_pf,%,$(ExcludeTargets))
endif
ifdef HdlPlatform
override HdlPlatform:=$(patsubst %_pf,%,$(HdlPlatform))
endif
ifdef HdlPlatforms
override HdlPlatforms:=$(patsubst %_pf,%,$(HdlPlatforms))
endif
ifdef OnlyPlatforms
override OnlyPlatforms:=$(patsubst %_pf,%,$(OnlyPlatforms))
endif
ifdef ExcludePlatforms
override ExcludePlatforms:=$(patsubst %_pf,%,$(ExcludePlatforms))
endif


$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)

include $(OCPI_CDK_DIR)/include/util.mk
include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk

# The libraries with names suitable for the library clause in VHDL, as passed to ocpigen
HdlVhdlLibraries=\
  $(infox HdlMyLibraries=$(HdlMyLibraries) HdlVhdlLibraries=$(and $(HdlMyLibraries),$(foreach l,$(sort $(HdlMyLibraries)),$(and $(notdir $l),-B $(notdir $l)))))\
  $(and $(HdlMyLibraries),$(foreach l,$(call Unique,$(HdlMyLibraries)),$(strip\
                             -B $(or $(notdir $l),$(error BROKEN)))))

# This default is only overridden for testing the makefiles
HdlError:=error
HdlSourceSuffix=.v


#HdlAllParts:=$(call Unique,$(foreach t,$(HdlTopTargets),$(if $(findstring $(f),$(HdlSimTools)),,$(foreach f,$(HdlTargets_$(t)),$(HdlTargets_$(f))))))
$(call OcpiDbgVar,HdlTopTargets)
HdlAllParts:=$(call Unique,$(foreach t,$(HdlTopTargets),\
	              $(or $(foreach f,$(HdlTargets_$t),\
                             $(or $(HdlTargets_$f),$f)),$t)))
$(call OcpiDbgVar,HdlAllParts)

HdlAllPlatformParts:=$(call Unique,$(foreach pl,$(HdlAllPlatforms),$(firstword $(subst -, ,$(HdlPart_$(pl))))))
$(call OcpiDbgVar,HdlAllPlatformParts)

# Read the workers file and set things accordingly
# 1. Set the instance list, which is a list of <worker-name>:<instance-name>
# 2. The workers list, which is a list of worker names
# 3. Add the core file associated with each worker to the cores list,
#    doing the search through $(ComponentLibraries)
#    The core for the worker will be _rv if the assembly file is VHDL
# Note we are silent about workers that don't have cores.
# FIXME: put in the error check here, but avoid building platform modules like "occp" etc.
HdlInstanceWkr=$(word 1,$(subst :, ,$1))
HdlInstanceCfg=$(word 2,$(subst :, ,$1))
HdlInstanceName=$(word 3,$(subst :, ,$1))
HdlInstanceWkrCfg=$(call HdlInstanceWkr,$1)$(and $(HdlToolRealCore),$(filter %.vhd,$(ImplFile)),_rv)$(foreach c,$(call HdlInstanceCfg,$1),$(if $(filter 0,$c),,_c$c))

define HdlSetWorkers
  HdlInstances:=$$(and $$(AssyWorkersFile),$$(strip $$(foreach i,$$(shell grep -h -v '\\\#' $$(AssyWorkersFile)),\
	               $$(if $$(filter $$(call HdlInstanceWkr,$$i),$$(HdlPlatformWorkers)),,$$i))))
  HdlWorkers:=$$(call Unique,$$(foreach i,$$(HdlInstances),$$(call HdlInstanceWkrCfg,$$i)))
  $$(infox HdlSetWorkers:Cores:'$$(Cores)':'$$(HdlWorkers)':'$$(HdlInstances)':'$$(HdlTarget)')
  SubCores_$$(HdlTarget):=$$(call Unique,\
    $$(Cores) \
    $$(foreach w,$$(HdlWorkers),\
      $$(or $(strip\
        $$(foreach f,$$(strip\
          $$(firstword \
            $$(foreach d,$$(call HdlTargetComponentLibraries,$$(HdlTarget),HSW),\
               $$(call HdlExists,$$d/$$w$$(HdlBin))))),\
          $$(call FindRelative,.,$$f)),\
	),$$(if $$(filter-out ocscp ocscp_rv metadata metadata_rv time_client time_client_rv unoc_node unoc_node_rv,$$w),$$(warning Warning: Worker $$w was not found in any of the component libraries built for $$(HdlTarget))))))
   $$(infox Cores SubCores_$$(HdlTarget) is $$(origin SubCores_$$(HdlTarget)) $$(flavor SubCores_$$(HdlTarget)):$$(SubCores_$$(HdlTarget)))

endef
# Get the list of cores we depend on, returning the real files that make can depend on
# With the deferred evaluation of target-specific items
HdlGetCores=$(infox HGC:$(Cores):$(HdlWorkers):$(HdlTarget))$(call Unique,\
    $(foreach c,$(Cores),$(call HdlCoreRef1,$c,$(HdlTarget))) \
    $(foreach w,$(HdlWorkers),\
      $(foreach f,$(strip\
        $(firstword \
          $(foreach d,$(call HdlTargetComponentLibraries,$(HdlTarget),HGG),\
            $(call HdlExists,$d/$w$(HdlBin))))),\
        $(call FindRelative,.,$f))))

################################################################################
# $(call HdlGetTargetFromPart,hdl-part)
# Return the target name from a hyphenated partname
HdlGetTargetFromPart=$(firstword $(subst -, ,$1))

################################################################################
# $(call HdlGetTop,family)
# Return the top name from a family
HdlGetTop=$(strip $(foreach v,$(HdlTopTargets),$(or $(filter $1,$v),$(and $(filter $1,$(HdlTargets_$v)),$v))))

################################################################################
# $(call HdlGetFamily,hdl-target,[multi-ok?])
# Return the family name associated with the target(usually a part)
# If the target IS a family, just return it.
# If it is a top level target with no family, return itself
# If it is a top level target with one family, return that family
# Otherwise return the family of the supplied part
# If the second argument is present, it is ok to return multiple families
# (The second argument should not contain spaces)

# StringEq=$(if $(subst x$1,,x$2)$(subst x$2,,x$1),,x)
HdlGetFamily=$(eval m1=$(subst $(Space),___,$1))$(strip \
  $(if $(HdlGetFamily_cached<$(m1)__$2>),,\
    $(call OcpiDbg,HdlGetFamily($1,$2) cache miss)$(eval export HdlGetFamily_cached<$(m1)__$2>=$(call HdlGetFamily_core,$1,$2)))\
  $(infox HdlGetFamily($1,$2)->$(HdlGetFamily_cached<$(m1)__$2>))$(HdlGetFamily_cached<$(m1)__$2>))

HdlGetFamily_core=$(call OcpiDbg,Entering HdlGetFamily_core($1,$2))$(strip \
  $(foreach gf,\
     $(or $(findstring $(1),$(HdlAllFamilies)),$(strip \
          $(if $(findstring $(1),all), \
	      $(if $(2),$(HdlAllFamilies),\
		   $(call $(HdlError),$(strip \
	                  HdlFamily is ambiguous for '$(1)'))))),$(strip \
          $(and $(findstring $(1),$(HdlTopTargets)),$(strip \
	        $(if $(and $(if $(2),,x),$(word 2,$(HdlTargets_$(1)))),\
                   $(call $(HdlError),$(strip \
	             HdlFamily is ambiguous for '$(1)'. Choices are '$(HdlTargets_$(1))')),\
	           $(or $(HdlTargets_$(1)),$(1)))))),$(strip \
	  $(foreach f,$(HdlAllFamilies),\
	     $(and $(filter $(call HdlGetTargetFromPart,$1),$(HdlTargets_$f)),$f))),$(strip \
	  $(and $(filter $1,$(HdlAllPlatforms)), \
	        $(call HdlGetFamily_core,$(call HdlGetTargetFromPart,$(HdlPart_$1))))),\
	  $(call $(HdlError),$(strip \
	     The build target '$1' is not a family or a part in any family))),\
     $(gf)))

#     $(call OcpiDbg,HdlGetFamily($1,$2)->$(gf))$(gf)))


################################################################################
# $(call HdlGetFamilies,hdl-target)
# Return all the families for this target
# HdlGetFamilies=$(call OcpiDbg,Entering HdlGetFamilies($1))$(strip
HdlGetFamilies=$(strip \
  $(foreach fs,$(call Unique,$(foreach t,$1,\
                         $(if $(findstring $(t),all),\
                             $(HdlAllFamilies),\
                             $(call HdlGetFamily,$t,x)))),\
     $(call OcpiDbg,HdlGetFamilies_core($1)->$(fs))$(fs)))

################################################################################
# $(call HdlGetPart,platform)
HdlGetPart=$(call OcpiDbg,Entering HdlGetPart($1))$(strip \
$(foreach gp,$(firstword $(subst -, ,$(HdlPart_$1))),\
   $(call OcpiDbg,HdlGetPart($1)->$(gp))$(gp)))


################################################################################
# The generic hdl compile that depends on HdlToolCompile
HdlName=$(or $(Core),$(LibName))
# if $(findstring $(HdlMode),library),$(LibName),$(Core))
HdlLog=$(HdlName)-$(HdlToolSet).out
HdlTime=$(HdlName)-$(HdlToolSet).time
HdlCompile=\
  $(infox Compile0:$(HdlWorkers):$(Cores):$(ImplWorkersFile):$(ImplFile):to-$@) \
  $(infox Compile:$(HdlWorkers):$(Cores):$(ImplWorkersFile)) \
  $(and $(SubCores_$(HdlTarget)),$(call HdlRecordCores,$(basename $@))$(infox DONERECORD:$(HdlTarget))) \
  $(HdlSourceListCompile)\
  $(infox SUBCORES:$(SubCores_$(HdlTarget))) \
  cd $(TargetDir) && \
  $(infox PRECOMPILE:$(HdlPreCompile))$(and $(HdlPreCompile), $(HdlPreCompile) &&)\
  export HdlCommand="set -e; $(HdlToolCompile)"; \
  $(TIME) bash -c \
   '(/bin/echo Commands to execute tool:@@@"$$HdlCommand" | sed "s/\([^\\]\); */\1;@@@/g" | sed "s/@@@/\n/g"; /bin/echo Output from executing commands above:;eval "$$HdlCommand") > $(HdlLog) 2>&1' \
    > $(HdlTime) 2>&1; \
  HdlExit=$$?; \
  (cat $(HdlTime) | tr -d "\n"; $(ECHO) -n " at "; date +%T) >> $(HdlLog); \
  grep -i error $(HdlLog)| grep -v Command: |\
    grep -v '^WARNING:'|grep -v " 0 errors," | grep -i -v -e '[_a-z]error' -e 'error[a-rt-z_]' $(HdlGrepExclude_$(HdlToolSet)); \
  if grep -q '^ERROR:' $(HdlLog); then HdlExit=1; fi; \
  $(HdlToolPost) \
  if test "$$OCPI_HDL_VERBOSE_OUTPUT" != ''; then \
    cat $(HdlLog); \
  fi; \
  if test $$HdlExit != 0; then \
    $(ECHO) -n Error: $(HdlToolSet) failed\($$HdlExit\). See $(TargetDir)/$(HdlLog).'  '; \
  else \
    $(ECHO) -n ' Tool "$(HdlToolSet)" for target "$(HdlTarget)" succeeded.  '; \
  fi; \
  (cat $(HdlTime) | tr -d "\n"; $(ECHO) -n " at "; date +%T); \
  rm -f $(HdlTime); \
  exit $$HdlExit
################################################################################
# The post processing by tools that do not produce any intermediate
# build results
HdlSimPost=\
  rm -r -f links; \
  if test $$HdlExit = 0; then \
    if ! test -d $(LibName); then \
      mkdir $(LibName); \
    else \
      rm -f $(LibName)/*; \
    fi;\
    for s in $(HdlToolLinkFiles); do \
      if [[ $$s == /* ]]; then \
        ln -s $$s $(LibName); \
      else \
        ln -s ../$$s $(LibName); \
      fi; \
    done; \
  fi;

HdlExists=$(call OcpiExists,$1)

# Default for all tools: libraries are directories whose name is the library name itself
# Return the name of the thing in the build directory (whose name is target-<target>)
# But if there is a single file there, that should be returned
HdlToolLibraryBuildFile=$1
# Default for all tools: installed libraries are just the directory whose name is the target
# Return the name of the thing in the install directory (whose name is the target)
# But if there is a single file there, that should be returned
HdlToolLibraryInstallFile=

################################################################################
# $(call HdlGetToolSet,hdl-target)
# Return the tool set for this target
HdlGetToolSet=$(infox HdlGetToolSet ($1) HdlTopTargets="$(HdlTopTargets)" HdlTargets_$(1)="$(HdlTargets_$(1))" HdlToolSet_$1="$(HdlToolSet_$1)" HdlToolSet_$(call HdlGetFamily,$(1))="$(HdlToolSet_$(call HdlGetFamily,$(1)))" HdlToolSet_$(HdlTargets_$(1))="$(HdlToolSet_$(HdlTargets_$(1)))"  )$(strip \
  $(or $(and $(findstring $(1),$(HdlTopTargets)),$(strip \
           $(or $(HdlToolSet_$(1)),$(strip \
                $(if $(word 2,$(HdlTargets_$(1))),,\
                   $(HdlToolSet_$(HdlTargets_$(1)))))))),$(strip \
       $(HdlToolSet_$(call HdlGetFamily,$(1)))),$(strip \
       $(call $(HdlError),Cannot infer tool set from '$(1)' Family='$(call HdlGetFamily,$(1))'))))

HdlGetBinSuffix=$(HdlBin_$(call HdlGetToolSet,$1))
################################################################################
# $(call HdlGetTargetsForToolSet,toolset,targets)
# Return all the targets that work with this tool
HdlGetTargetsForToolSet=$(call Unique,\
    $(foreach t,$(2),\
       $(and $(findstring $(1),$(call HdlGetToolSet,$(t))),$(t))))

################################################################################
# $(call HdlGetConstraintsSuffix,<platform>)
# Return the suffix for constraints files for this platform
HdlGetConstraintsSuffix=$(strip \
  $(HdlConstraintsSuffix_$(HdlToolSet_$(call HdlGetFamily,$(HdlPart_$1)))))
################################################################################
# $(call HdlGetConstraintsFile,<file,platform>)
# Return the constraints file name adding suffix if not present
HdlGetConstraintsFile=$(strip $(and $1,\
  $(foreach s,$(call HdlGetConstraintsSuffix,$2),\
    $1$(if $(filter %$s,$1),,$s))))

$(call OcpiDbgVar,HdlPlatform)
$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)

#  $(foreach c,$(ComponentLibraries),\
#    $(foreach o,$(ComponentLibraries),\
#       $(if $(findstring $o,$c),,\
#          $(and $(filter $(notdir $o),$(notdir $c)),
#            $(error The component libraries "$(c)" and "$(o)" have the same base name, which is not allowed)))))

define HdlSearchComponentLibraries

  override XmlIncludeDirsInternal := $(call Unique,$(XmlIncludeDirsInternal) $(call HdlXmlComponentLibraries))
endef
HdlRmRv=$(if $(filter %_rv,$1),$(patsubst %_rv,%,$1),$1)

#########################################################################################################

# Stash all the cores we needed in a file so that tools that do not implement
# proper hierarchies can include indirectly required cores later
# Called from HdlCompile which is already tool-specific
HdlRecordCores=\
  $(infox Record:$1:$(SubCores_$(HdlTarget)):$(HdlTarget))\
  $(and $(call HdlExists,$(dir $1)),\
  (\
   echo '\#' This generated file records cores necessary to build this $(LibName) $(HdlMode); \
   echo $(call OcpiPathsFromProjectTopOrImports,$(dir $1),\
          $(foreach p,$(foreach c,$(call HdlCollectCores,$(HdlTarget),HdlRecordCores),$(strip\
            $(call HdlCoreRefMaybeTargetSpecificFile,$c,$(HdlTarget)))),$p)); \
  ) > $(call HdlRmRv,$1).cores;)

# Collect a list of cores that this asset depends on. If the path to the core
# is correct as is, use that path. Otherwise, use HdlCoreRef to determine the
# path to the core.
HdlCollectCores=$(infox CCCC:$(SubCores_$(HdlTarget)):$1:$2)$(strip $(call Unique,\
  $(foreach a,\
    $(foreach c,$(SubCores_$(HdlTarget)),$(infox ZC:$c)$c \
      $(foreach r,$(basename $(call HdlCoreRefMaybeTargetSpecificFile,$c,$1)),$(infox ZR:$r)\
        $(foreach f,$(call HdlExists,$(call HdlRmRv,$r).cores),$(infox ZF:$f)\
          $(foreach p,$(call OcpiAbsPathToContainingProject,$f),\
            $(foreach z,$(shell grep -v '\#' $f),$(infox found:$z)\
              $(if $(filter /%,$z),\
                $z,\
	        $(or $(call OcpiExists,$p/$z),\
                     $(call OcpiExists,$p/$(subst /lib/,/,$(patsubst hdl/platforms/%,lib/platforms/%,$z))),\
                     $(call OcpiExists,$p/$(subst /lib/,/,$(patsubst hdl/devices/%,lib/devices/%,$z))),\
	             $(call OcpiExists,$p/$(subst /lib/,/,$(patsubst hdl/primitives/%,lib/hdl/%,$z)))))))))),\
    $a)))

#########################################################################################################
# Record all of the libraries and sources required for this asset (onl when a tool requires this is done)
# When recording sources for a core, we only record the stub files since those will be needed later.
HdlSourceListCompile=\
  $(if $(HdlToolNeedsSourceList_$(HdlToolSet)),\
    $(and $(HdlLibrariesInternal),$(call HdlRecordLibraries,$(basename $@))) \
    $(and $(or $(HdlSources),$(if $(filter $(HdlMode),core),$(CoreBlackBoxFiles))),$(call HdlRecordSources,$(basename $@))))

# Record the list of libraries required by this asset/library in the .libs file
# If we see an absolute path, record it just by the asset's name. It will be
# searched for later (e.g. via HdlSearchPrimitivePath). If it is a relative
# path, we assume it is in the local project and therefore record it as a
# relative path.
HdlRecordLibraries=\
  $(infox Record:$1:$(HdlLibrariesInternal):$(HdlTarget))\
  $(and $(call HdlExists,$(dir $1)),\
  (\
   echo '\#' This generated file records libraries necessary to build this $(LibName) $(HdlMode); \
   $(foreach l,$(HdlLibrariesInternal),\
     echo $(if $(filter /%,$l),$(notdir $l),$(call OcpiPathToAssetOutsideProject,.,$l));) \
  ) > $(patsubst %/,%,$(call HdlRmRv,$1)).libs;)\

# Extract the list of libraries required by an asset/library $2 for target $1
HdlExtractLibrariesFromFile=$(infox Extract:$2:$1)$(call Unique,\
	$(foreach f,$(call HdlExists,$(call HdlRmRv,$2)/$(HdlTarget)/$(notdir $(call HdlRmRv,$2)).libs),$(infox ZF:$f)\
	  $(foreach z,$(shell grep -v '\#' $f),$(infox found:$z)$z )))

# If the libname consists of one word, search the primitive path.
# If the libname is an absolute path, return it abspath of $2,
# otherwise, return $1/$2 (path $2 starting from $1).
HdlNotdirAbsOrRelPath=$(infox NORP:$1:$2)\
  $(if $(findstring $2,$(notdir $2)),\
    $(patsubst %/,%,$(call HdlSearchPrimitivePath,$2,,CollectLibs)),\
    $(if $(filter /%,$2),\
      $(abspath $2),\
      $1/$2))

# This function is a helper to HdlCollectLibraries and is used for
# collecting libraries that $2 depends on. If $2 is a name (not a path),
# we search the primitive path for it. Since we are recursing through
# libraries, if we find that $2 is a relative path, we assume it is
# relative to $1 and use $1/$2 via "HdlNotdirAbsOrRelPath"
HdlCollectLibsRecurse=$(infox RecursingOn:$2:$1)\
	$(foreach l,$(call HdlExtractLibrariesFromFile,$1,$(if $(findstring $2,$(notdir $2)),$(call HdlSearchPrimitivePath,$2,,extract),$2)),$(infox COL:$2:$l)\
	  $(if $(HdlRecurseLibraries_$(HdlToolSet)),$(call HdlCollectLibsRecurse,$1,$l) )$(call HdlNotdirAbsOrRelPath,$2,$l) )


# Accumulate a list of the libraries required by the current asset
# If the HdlRecurseLibraries_<tool> flag is set, gather this list recursively
HdlCollectLibraries=$(infox PPPP:$(HdlLibrariesInternal):$1)$(call OcpiUniqueNotDir,\
	$(foreach a,\
	  $(foreach p,$(HdlLibrariesInternal),$(infox ZP:$p)\
	    $(call HdlCollectLibsRecurse,$1,$p)$p ),$(infox A:$a)$a ))


# Record the list of sources required by this asset/library in the .sources file
# Here we check the flag indicating that there are tool-specific source collections for this asset
# in which case we create <asset>.<tool>.sources
# If this is a true core, we record the black box stub files
HdlRecordSources=\
  $(infox Record:$1:$(HdlSources):$(HdlTarget))\
  $(and $(call HdlExists,$(dir $1)),\
  (\
   echo '\#' This generated file records sources necessary to build this $(LibName) $(HdlMode); \
   $(foreach s,$(if $(filter $(HdlMode),core),$(wildcard $(CoreBlackBoxFiles)),$(HdlSources)),echo $(notdir $s);) \
  ) > $(patsubst %/,%,$(call HdlRmRv,$1)).sources ;)

# Here, we use HdlLibraryRefDir to determine the path to the library
# asset $2 in question. If we are working with an absolute path,
# just return the result of HdlLibraryRefDir. Otherwise, we are
# working with a relative path. Return the path relative to $3.
HdlRelativeOrAbsolutePathToLib=$(infox HRAPL:$1:$2:$3)$(strip \
  $(if $(filter /%,$2),\
    $(patsubst %/,%,$(call HdlLibraryRefDir,$2,$1,,X4)),\
    $(call FindRelative,$3,$(strip \
      $(call HdlLibraryRefDir,$2,$1,,X4)))))

# For a given library $2 and target $1, extract the library's source
# files from <lib>.sources
#
# Find the path relative or absolute path to '.' to determine a usable
# path for the .sources file. Grep the .sources file to collect source file
# names.
#
# Return the paths (relative or absolute) to each file listed in .sources
# relative to $3.
HdlExtractSourcesForLib=$(infox Extract:$2:$1)\
  $(foreach f,\
    $(call HdlRelativeOrAbsolutePathToLib,$1,$2,.),$(infox ZF:$f)\
      $(foreach z,$(shell grep -v  '\#' $f/$(notdir $2).sources),$(infox found:$z)\
        $(call HdlRelativeOrAbsolutePathToLib,$1,$2,$3)/$z ))

#########################################################################################################

HdlPassTargets=$(and $(HdlTargets),HdlTargets="$(HdlTargets)") \
               $(and $(HdlTarget),HdlTargets="$(HdlTarget)") \
               $(and $(HdlPlatforms),HdlPlatforms="$(HdlPlatforms)") \
               $(and $(HdlPlatform),HdlPlatforms="$(HdlPlatform)")

# Do target-specific file shadowing
HdlShadowFiles=\
  $(foreach f,$(filter-out $(filter-out %.vhd,$(call CoreBlackBoxFiles,$1,$2)),\
              $(CompiledSourceFiles)),\
     $(or $(wildcard $(HdlTarget)/$f),\
	  $(wildcard $(call HdlGetTop,$(HdlTarget))/$f),\
          $f))

# This is the list of files that will be generated in the TARGET
# directory.
# The VHDL defs file must preceed the generics file
# Return nothing if no parameters
# FIXME: this has worker stuff in it - should it be elsewhere?
# $(call HdlTargetSrcFiles,target-dir,paramconfig)
HdlTargetSrcFiles=$(and $(filter-out library core,$(HdlMode)),\
  $(call HdlVHDLTargetDefs,$1,$2)\
  $(call HdlVerilogTargetDefs,$1,$2)\
  $(call WkrTargetDir,$1,$2)/generics$(HdlVHDLIncSuffix)\
  $(call WkrTargetDir,$1,$2)/generics$(HdlVerilogIncSuffix)\
  $(call HdlVHDLTargetImpl,$1,$2)\
  $(TargetSourceFiles_$2) \
  $(ImplFile))

$(OutDir)target-%/generics.vhd: $$(ImplXmlFile) | $(OutDir)target-%
	$(AT)echo Generating the VHDL constants file for config $(ParamConfig): $@
	$(AT)$(call OcpiGen, -D $(@D) \
	                $(and $(Assembly),-S $(Assembly)) $(and $(Platform),-P $(Platform)) \
	                $(and $(PlatformDir),-F $(PlatformDir)) \
	                -g$(ParamConfig) $(and $(filter verilog,$(HdlLanguage)),-w) $(ImplXmlFile))

$(OutDir)target-%/generics.vh: $$(ImplXmlFile) | $(OutDir)target-%
	$(AT)echo Generating the Verilog constants file for config $(ParamConfig): $@
	$(AT)$(call OcpiGen, -D $(dir $@) \
	                $(and $(Assembly),-S $(Assembly)) $(and $(Platform),-P $(Platform)) \
	                $(and $(PlatformDir),-F $(PlatformDir)) \
	                -g$(ParamConfig) $(and $(filter vhdl,$(HdlLanguage)),-w) $(ImplXmlFile))

ifneq (,)
# Establish where the platforms are
ifndef HdlPlatformsDir
  HdlPlatformsDir:=$(OCPI_CDK_DIR)/lib/platforms
    ifeq ($(realpath $(HdlPlatformsDir)),)
      $(error No HDL platforms found. Looked in $(OCPI_CDK_DIR)/lib/platforms)
    endif
#  endif
endif
endif
# Do the stuff necessary when building an assembly
# This applies to platform configurations, application assemblies, and containers
define HdlPrepareAssembly

  # 1. Scan component libraries to add to XmlIncludeDirs
  $$(call OcpiSetXmlIncludes)
  $$(eval $$(HdlSearchComponentLibraries))
  # 2. Generate (when needed) the workers file immediately to use for dependencies
  AssyWorkersFile:=$$(GeneratedDir)/$$(Worker).wks
  $$(if\
    $$(call DoShell,$$(MAKE) -f $$(OCPI_CDK_DIR)/include/hdl/hdl-get-workers.mk\
                    Platform=$(Platform) \
                    PlatformDir=$(PlatformDir) \
                    Assembly=$(Assembly) \
                    XmlIncludeDirsInternal="$$(XmlIncludeDirsInternal)" \
                    AssyWorkersFile=$$(AssyWorkersFile) \
                    Worker=$$(Worker) Worker_xml=$$(Worker_xml) \
                    AT=$(AT), \
                    Output), \
    $$(error Error deriving workers from file $$(Worker).xml: $$(Output)),\
   )
  # 3. Generated the assembly source file
  ImplFile:=$$(GeneratedDir)/$$(Worker)-assy$$(HdlSourceSuffix)
  $$(ImplFile): $$$$(ImplXmlFile) | $$$$(GeneratedDir)
	$(AT)echo Generating the $$(HdlMode) source file: $@ from $$<
	$(AT)$$(call OcpiGen, -D $$(GeneratedDir) \
                         $(and $(Assembly),-S $(Assembly)) $(and $(Platform),-P $(Platform)) \
			 $(and $(PlatformDir),-F $(PlatformDir)) \
                         -a $$<)
  # 4. Make the generated assembly source file one of the files to compile
  WorkerSourceFiles=$$(ImplFile)
  # 5. Define the variable used for dependencies when the worker is actually built
  HdlPreCore=$$(eval $$(HdlSetWorkers))$$(call HdlCollectCores,$$(HdlTarget),HdlPrepareAssembly)
endef
#ifndef OCPI_HDL_PLATFORM
#OCPI_HDL_PLATFORM=zed
#endif
define HdlPreprocessTargets
  ifdef HdlPlatform
    override HdlPlatforms:=$$(call Unique,$$(HdlPlatform) $$(HdlPlatforms))
  endif
  ifeq ($$(origin HdlPlatforms),undefined)
    ifdef HdlPlatform
      ifneq ($$(words $$(HdlPlatform)),1)
        $$(error HdlPlatform variable must only have one platform)
      endif
      HdlPlatforms:=$$(HdlPlatform)
    else
      HdlPlatforms:=$$(if $$(filter platform,$$(HdlMode)),$$(CwdName),$$(OCPI_HDL_PLATFORM))
    endif
  else ifeq ($$(HdlPlatforms),all)
    override HdlPlatforms:=$$(HdlAllPlatforms)
  endif
  ifndef HdlTargets
    ifdef HdlTarget
      HdlTargets:=$$(HdlTarget)
    else
      ifdef HdlPlatforms
        override HdlTargets:=\
          $$(call Unique,$$(foreach p,$$(HdlPlatforms),\
            $$(if $$(HdlPart_$$p),$$(call HdlGetFamily,$$(HdlPart_$$p)),\
               $$($$(if $$(filter clean%,$$(MAKECMDGOAL)),warning,error) Unknown platform: $$p))))
      else
        override HdlTargets:=$$(and $$(OCPI_HDL_PLATFORM),$$(call HdlGetFamily,$$(OCPI_HDL_PLATFORM)))
      endif
    endif
  else ifeq ($$(HdlTargets),all)
    override HdlTargets:=$$(HdlAllFamilies)
  endif
endef

include $(OCPI_CDK_DIR)/include/hdl/hdl-search.mk
endif
