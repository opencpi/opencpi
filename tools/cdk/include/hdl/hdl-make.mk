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
  $(infox HdlMyLibraries=$(HdlMyLibraries) HdlVhdlLibraries=$(and $(HdlMyLibraries),$(foreach l,$(sort $(HdlMyLibraries)),$(and $(notdir $l),-l $(notdir $l)))))\
  $(and $(HdlMyLibraries),$(foreach l,$(call Unique,$(HdlMyLibraries)),$(strip\
                             -l $(or $(notdir $l),$(error BROKEN)))))

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

# Families are either top level targets with nothing underneath or one level down
HdlAllFamilies:=$(call Unique,$(foreach t,$(HdlTopTargets),$(or $(HdlTargets_$(t)),$(t))))
$(call OcpiDbgVar,HdlAllFamilies)


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
	),$$(if $$(filter-out ocscp ocscp_rv metadata metadata_rv time_client time_client_rv unoc_node unoc_node_rv,$$w),$$(warning Warning: Worker $$w was not found in any of the component libraries)))))
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
	     $(and $(findstring $(call HdlGetTargetFromPart,$1),$(HdlTargets_$f)),$f))),$(strip \
	  $(and $(findstring $1,$(HdlAllPlatforms)), \
	        $(call HdlGetFamily_core,$(call HdlGetTargetFromPart,$(HdlPart_$1))))),\
	  $(call $(HdlError),$(strip \
	     The build target '$1' is not a family or a part in any family))),\
     $(gf)))

#     $(call OcpiDbg,HdlGetFamily($1,$2)->$(gf))$(gf)))


################################################################################
# $(call HdlGetFamilies,hdl-target)
# Return all the families for this target
# HdlGetFamilies=$(call OcpiDbg,Entering HdlGetFamilies($1))$(strip 
ifdef NEVER
HdlGetFamilies=$(eval m1=$(subst $(Space),___,$1))$(strip \
  $(if $(HdlGetFamilies_cached<$(m1)>),,\
    $(call OcpiDbg,HdlGetFamilies($1) cache miss)$(eval export HdlGetFamilies_cached<$(m1)>:=$(strip $(call HdlGetFamilies_core,$1))))\
  $(infox HdlGetFamilies($1)->$(HdlGetFamilies_cached<$(m1)>))$(HdlGetFamilies_cached<$(m1)>))

endif
#HdlGetFamilies_core=$(strip \

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
  $(infox SUBCORES:$(SubCores_$(HdlTarget))) \
  cd $(TargetDir) && \
  $(infox PRECOMPILE:$(HdlPreCompile))$(and $(HdlPreCompile), $(HdlPreCompile) &&)\
  export HdlCommand="set -e; $(HdlToolCompile)"; \
  $(TIME) bash -c \
   '(/bin/echo Commands to execute tool:@"$$HdlCommand" | sed "s/\([^\\]\); */\1;@/g" | tr "@" "\n"; /bin/echo Output from executing commands above:;eval "$$HdlCommand") > $(HdlLog) 2>&1' \
    > $(HdlTime) 2>&1; \
  HdlExit=$$?; \
  (cat $(HdlTime) | tr -d "\n"; $(ECHO) -n " at "; date +%T) >> $(HdlLog); \
  grep -i error $(HdlLog)| grep -v Command: |\
    grep -v '^WARNING:'|grep -v " 0 errors," | grep -i -v -e '[_a-z]error' -e 'error[a-rt-z_]'; \
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

# Stash all the cores we needed in a file so that tools that do not implement
# proper hierarchies can include indirectly required cores later
# Called from HdlCompile which is already tool-specific
HdlRecordCores=\
  $(infox Record:$1:$(SubCores_$(HdlTarget)):$(HdlTarget))\
  $(and $(call HdlExists,$(dir $1)),\
  (\
   echo '\#' This generated file records cores necessary to build this $(LibName) $(HdlMode); \
   echo $(foreach c,$(call HdlCollectCores,$(HdlTarget),HdlRecordCores),$(strip\
           $(call OcpiAbsPath,$(call HdlCoreRef,$(call HdlToolCoreRef,$c),$(HdlTarget))))) \
  ) > $(call HdlRmRv,$1).cores;)

#	             $(foreach r,$(call HdlRmRv,$(basename $(call HdlCoreRef,$c,$1))),\

HdlCollectCores=$(infox CCCC:$(SubCores_$(HdlTarget)):$1:$2)$(call Unique,\
		  $(foreach a,\
                   $(foreach c,$(SubCores_$(HdlTarget)),$(infox ZC:$c)$c \
	             $(foreach r,$(basename $(call HdlCoreRef,$(call HdlToolCoreRef,$c),$1)),$(infox ZR:$r)\
                       $(foreach f,$(call HdlExists,$(call HdlRmRv,$r).cores),$(infox ZF:$f)\
                          $(foreach z,$(shell grep -v '\#' $f),$(infox found:$z)$z)))),$a))

HdlPassTargets=$(and $(HdlTargets),HdlTargets="$(HdlTargets)") \
               $(and $(HdlTarget),HdlTargets="$(HdlTarget)") \
               $(and $(HdlPlatforms),HdlPlatforms="$(HdlPlatforms)") \
               $(and $(HdlPlatform),HdlPlatforms="$(HdlPlatform)")

# Do target-specific file shadowing
HdlShadowFiles=\
  $(foreach f,$(filter-out $(filter-out %.vhd,$(CoreBlackBoxFiles)),\
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
HdlTargetSrcFiles=\
  $(if $(and $2,$(filter-out 0,$2)),\
    $(call HdlVHDLTargetDefs,$1,$2) $(call HdlVerilogTargetDefs,$1,$2), \
    $(DefsFile) $(WDefsFile)) \
  $(and $(WorkerParamNames),$(strip \
     $(call WkrTargetDir,$1,$2)/generics$(HdlVHDLIncSuffix)\
     $(and $(filter .v,$(HdlSourceSuffix)),\
       $(call WkrTargetDir,$1,$2)/generics$(HdlVerilogIncSuffix))))\
  $(if $(and $2,$(filter-out 0,$2)),$(call HdlVHDLTargetImpl,$1,$2),$(ImplHeaderFiles))

#		$(and $(ParamVHDLtype_$(ParamConfig)_$n), \
#		   echo '$(ParamVHDLtype_$(ParamConfig)_$n)' ;) \
#
# $(AT)(\
#      echo -- This file sets values for top level generics ;\
#      echo library ocpi\; use ocpi.all, ocpi.types.all\; ;\
#      echo package body $(Worker)_constants is ;\
#      $(foreach n,$(WorkerParamNames)$(infox WPN:$(WorkerParamNames):),\
# 	echo '$(ParamVHDL_$(ParamConfig)_$n)'\; ;) \
#      echo end $(Worker)_constants\; \
# ) > $@

$(OutDir)target-%/generics.vhd: $$(VHDLDefsFile) | $(OutDir)target-%
	$(AT)echo Generating the VHDL constants file for config $(ParamConfig): $@
	$(AT)$(OcpiGen) -D $(dir $@) \
	                $(and $(Assembly),-S $(Assembly)) $(and $(Platform),-P $(Platform)) \
	                $(and $(PlatformDir),-F $(PlatformDir)) \
	                -g$(ParamConfig) $(ImplXmlFile)

$(OutDir)target-%/generics.vh: | $(OutDir)target-%
	$(AT)(\
	     echo // This file sets values for top level parameters ;\
	     $(foreach n,$(WorkerParamNames),\
		echo "$(ParamVerilog_$(ParamConfig)_$n)"\; ;) \
	) > $@

ifneq (,)
# Establish where the platforms are
ifndef HdlPlatformsDir
  HdlPlatformsDir:=$(OCPI_CDK_DIR)/lib/platforms
#  ifeq ($(realpath $(HdlPlatformsDir)),)
#    HdlPlatformsDir:=$(OCPI_BASE_DIR)/hdl/platforms
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
	$(AT)$$(OcpiGen) -D $$(GeneratedDir) \
                         $(and $(Assembly),-S $(Assembly)) $(and $(Platform),-P $(Platform)) \
			 $(and $(PlatformDir),-F $(PlatformDir)) \
                         -a $$<
  # 4. Make the generated assembly source file one of the files to compile
  WorkerSourceFiles=$$(ImplFile)
  # 5. Define the variable used for dependencies when the worker is actually built
  HdlPreCore=$$(eval $$(HdlSetWorkers))$$(call HdlCollectCores,$$(HdlTarget),HdlPrepareAssembly)
endef
#ifndef OCPI_HDL_PLATFORM
#OCPI_HDL_PLATFORM=zed
#endif
define HdlPreprocessTargets
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
