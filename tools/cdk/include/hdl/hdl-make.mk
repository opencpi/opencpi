# This file contains various utilities for hdl makefiles.
# It is distinguishd from hdl-pre.mk in that it doesn't really mess with any variables
# Thus HDL utility stuff goes here unless it is order sensitive, in which case it
# is in hdl-pre.mk
ifndef __HDL_MAKE_MK__
__HDL_MAKE_MK__=x
# This file is included by the various levels of hdl makefiles (not the leaf makefiles)
# i.e. a Makefile file that does other makefiles.
# Note that targets are generally families except when a primitive core is actually part-specific.

include $(OCPI_CDK_DIR)/include/util.mk
include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk

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


################################################################################
# $(call HdlComponentLibrary,lib,target)
# Return the actual name (pointing to the target dir) and check for errors
HdlComponentLibrary=$(strip \
  $(foreach x,\
  $(foreach l,$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/$1),\
   $(foreach f,$(if $(filter ./,$(dir $2)),,$(dir $2))$(call HdlGetFamily,$(notdir $2)),\
     $(or $(wildcard $l/hdl/$f),$(wildcard $l/lib/hdl/$f),\
      $(error Component library '$l' not found at either $l/hdl/$f or $l/lib/hdl/$f)))),$x))

HdlComponentCore=$(strip \
  $(foreach l,$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/$1),\
   $(foreach t,$(call Unique,$3 $(call HdlGetFamily,$3)),\
    $(foreach c,hdl/$t/$2$(HdlBin),\
    $(or $(wildcard $l/$c),$(wildcard $l/lib/$c))))))

#      $(error Component core '$l' not found at either $l/hdl/$2 or $l/lib/hdl/$2))))

################################################################################
# $(call HdlFindWorkerCoreFile,worker)
# use ComponentLibraries
# Return the actual file name of the found core or error 
HdlFindWorkerCoreFile=$(strip \
  $(firstword \
    $(or $(foreach c,$(ComponentLibraries),\
            $(foreach d,$(call HdlComponentCore,$c,$1,$(HdlTarget)),$d)),\
         $(error Worker $1 not found in any component library.))))

# Return the generic directory where the spec files are exported
HdlXmlComponentLibrary=$(strip \
    $(foreach l,$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/$1),\
      $(foreach d,$(or $(wildcard $l/lib),$(wildcard $l),\
	            $(error Component library '$1' not found at $l)),\
	$d)))

# Return the two levels of xml include dirs: generic (for spec xml) and hdl (for worker xml)
HdlXmlComponentLibraries=$(strip \
  $(foreach c,$1,\
   $(foreach d,$(call HdlXmlComponentLibrary,$c),$d $d/hdl)))

# Read the workers file.  Arg1 is the file name(s) of the instance files
define HdlSetWorkers
  HdlInstances:=$$(strip $$(foreach i,$$(shell grep -h -v '\\\#' $(ImplWorkersFiles)),\
	               $$(if $$(filter $$(firstword $$(subst :, ,$$i)),$$(HdlPlatformWorkers)),,$$i)))
  HdlWorkers:=$$(call Unique,$$(foreach i,$$(HdlInstances),$$(firstword $$(subst :, ,$$i))))
#  $$(info Instances are: $$(HdlInstances))
#  $$(info Workers are: $$(HdlWorkers))
endef




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
# If the second argument is present,it is ok to return multiple families
#HdlGetFamily=$(call OcpiDbg,Entering HdlGetFamily($1,$2))$(strip 
HdlGetFamily=$(strip \
  $(foreach gf,\
     $(or $(findstring $(1),$(HdlAllFamilies)),$(strip \
          $(if $(findstring $(1),all), \
	      $(if $(2),$(HdlAllFamilies),\
		   $(call $(HdlError),$(strip \
	                  HdlFamily is ambigous for '$(1)'))))),$(strip \
          $(and $(findstring $(1),$(HdlTopTargets)),$(strip \
	        $(if $(and $(if $(2),,x),$(word 2,$(HdlTargets_$(1)))),\
                   $(call $(HdlError),$(strip \
	             HdlFamily is ambigous for '$(1)'. Choices are '$(HdlTargets_$(1))')),\
	           $(or $(HdlTargets_$(1)),$(1)))))),$(strip \
	  $(foreach f,$(HdlAllFamilies),\
	     $(and $(findstring $(call HdlGetTargetFromPart,$1),$(HdlTargets_$f)),$f))),$(strip \
	  $(and $(findstring $1,$(HdlAllPlatforms)), \
	        $(call HdlGetFamily,$(call HdlGetTargetFromPart,$(HdlPart_$1))))),\
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
     $(call OcpiDbg,HdlGetFamilies($1)->$(fs))$(fs)))

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
  cd $(TargetDir) && \
  $(and $(HdlPreCompile), $(HdlPreCompile) &&)\
  export HdlCommand="set -e; $(HdlToolCompile)"; \
  $(TIME) sh -c \
   '(/bin/echo Commands to execute tool:@"$$HdlCommand" | sed "s/\([^\\]\); */\1;@/g" | tr "@" "\n"; /bin/echo Output from executing commands above:;eval "$$HdlCommand") > $(HdlLog) 2>&1' \
    > $(HdlTime) 2>&1; \
  HdlExit=$$?; \
  (cat $(HdlTime); $(ECHO) -n " at "; date +%T) >> $(HdlLog); \
  grep -i error $(HdlLog)| grep -v Command: |\
    grep -v '^WARNING:'|grep -v " 0 errors," | grep -i -v '[_a-z]error'; \
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

################################################################################
# This works when wildcard doesn't.  FIXME: put into util.mk and use it more
# (Note: the make wildcard function caches results so can't probe something that
# might come into existence during execution of make)
# Sad, as it slows things down.  Perhaps better to use realpath?
HdlExists=$(strip $(shell if test -e $1; then echo $1; fi))

################################################################################
# $(call HdlLibraryRefDir,location-dir,target)
# $(call HdlCoreRefDir,location-dir,target)
# These functions take a user-specified (friendly, target-independent) library
# or core location and a target name.  They return the actual directory of that
# library/core that the tool wants to see for that target.
# These are not for component libraries, but a primitive libraries and cores
# The third argument is just for callers to pass something private to HdlToolLibRef
HdlLibraryRefDir=$(foreach i,$(call HdlLibraryRefFile,$1,$2,$3),$i)

#strip \
#  $(foreach r,$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/hdl/$1),$(strip\
#    $(if $(wildcard $r/target-$2),$r/target-$2/$(notdir $1),)/$(strip\
#      $(call HdlToolLibRef,$(notdir $1),$2,$3)))))
#$(info HCRF:$1,$2,$3)

HdlCRF= $(strip \
  $(foreach r,\
    $(if $(call HdlExists,$1$(HdlBin)),$1$(HdlBin),\
      $(if $(call HdlExists,$1/target-$2/$3),$1/target-$2/$3,\
         $(if $(call HdlExists,$1/$3),$1/$3,\
            $1/$2/$3))),\
     $r))

#$(info Result:$1,$2,$3,$r)$r))





HdlCoreRef=$(strip \
  $(foreach d,$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/hdl/$1),\
   $(foreach c,$(notdir $1)$(HdlBin),\
     $(or $(call HdlExists,$(call HdlCRF,$d,$2,$c)),\
	  $(call HdlExists,$(call HdlCRF,$d,$(call HdlGetFamily,$2),$c)),\
	$(error No core file ($c) for target "$2" found for "$1".)))))


################################################################################
# $(call HdlLibraryRefFile,location-dir,target)
# This function takes a user-specified (friendly, target-independent) library
# or core location and a target name.  They return the actual pathname of that
# library/core file or directory for "make" dependencies.
# We rely on the underlying tool for the actual filename, if it is not just
# the directory
#HdlLRF=$(info LRF:$(shell pwd):$1:$2:$1/target-$2:$(wildcard $1/target-$2/*))$(strip\
#    $(if $(info D:$(shell pwd):$1:$2:$1/target-$2:$(call HdlWild,$1/target-$2))$(call HdlWild,$1/target-$2),\


HdlLRF=$(strip \
  $(foreach r,\
    $(if $(call HdlExists,$1/target-$2),\
       $1/target-$2/$(call HdlToolLibraryBuildFile,$(or $3,$(notdir $1))),\
       $1/$2/$(call HdlToolInstallFile,$(or $3,$(notdir $1)),$t)),\
       $r))


#   $(info LRF Result:$r)$r))



HdlLibraryRefFile=$(strip \
  $(foreach r,$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/hdl/$1),\
    $(foreach f,$(call HdlGetFamily,$2),\
       $(if $(filter $f,$2),\
          $(call HdlLRF,$r,$2,$3),\
	  $(or $(call HdlExists,$(call HdlLRF,$r,$2)),$(call HdlLRF,$r,$f,$3))))))

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
HdlGetToolSet=$(strip \
  $(or $(and $(findstring $(1),$(HdlTopTargets)),$(strip \
           $(or $(HdlToolSet_$(1)),$(strip \
                $(if $(word 2,$(HdlTargets_$(1))),,\
                   $(HdlToolSet_$(HdlTargets_$(1)))))))),$(strip \
       $(HdlToolSet_$(call HdlGetFamily,$(1)))),$(strip \
       $(call $(HdlError),Cannot infer tool set from '$(1)'))))

HdlGetBinSuffix=$(HdlBin_$(call HdlGetToolSet,$1))
################################################################################
# $(call HdlGetTargetsForToolSet,toolset,targets)
# Return all the targets that work with this tool
HdlGetTargetsForToolSet=$(call Unique,\
    $(foreach t,$(2),\
       $(and $(findstring $(1),$(call HdlGetToolSet,$(t))),$(t))))

$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)


# This function adjusts only things that have a slash
HdlAdjustLibraries=$(foreach l,$1,$(if $(findstring /,$l),$(call AdjustRelative,$l),$l))

$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)

define HdlSearchComponentLibraries
override XmlIncludeDirs += $(call HdlXmlComponentLibraries,$(ComponentLibraries))
endef

# Establish where the platforms are
ifndef HdlPlatformsDir
  HdlPlatformsDir:=$(OCPI_CDK_DIR)/lib/hdl/platforms
  ifeq ($(realpath $(HdlPlatformsDir)),)
    HdlPlatformsDir:=$(OCPI_BASE_DIR)/hdl/platforms
    ifeq ($(realpath $(HdlPlatformsDir)),)
      $(error No HDL platforms found. Looked in $(OCPI_CDK_DIR)/lib/hdl/platforms and $(OCPI_BASE_DIR)/hdl/platforms.)
    endif
  endif
endif

endif
