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
  $(foreach l,$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/$1),\
   $(foreach f,$(if $(filter ./,$(dir $2)),,$(dir $2))$(call HdlGetFamily,$(notdir $2)),\
     $(or $(wildcard $l/hdl/$f),$(wildcard $l/lib/hdl/$f),\
      $(error Component library '$l' not found at either $l/hdl/$f or $l/lib/hdl/$f)))))

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

HdlXmlComponentLibraries=$(strip \
  $(foreach c,$1,\
    $(foreach l,$(if $(findstring /,$c),$c,$(OCPI_CDK_DIR)/lib/$c),\
      $(foreach d,$(or $(wildcard $l/lib),$(wildcard $l),\
	            $(error Component library '$c' not found at $l)),\
	$d $d/hdl))))

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
# $(call HdlGetFamily,hdl-target,[multi-ok?])
# Return the family name associated with the target(usually a part)
# If the target IS a family, just return it.
# If it is a top level target with no family, return itself
# If it is a top level target with one family, return that family
# Otherwise return the family of the supplied part
# If the second argument is present,it is ok to return multiple families
HdlGetFamily=$(call OcpiDbg,Entering HdlGetFamily($1,$2))$(strip \
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
	     The build target '$(1)' is not a family or a part in any family))),\
     $(call OcpiDbg,HdlGetFamily($1,$2)->$(gf))$(gf)))


################################################################################
# $(call HdlGetFamilies,hdl-target)
# Return all the families for this target
HdlGetFamilies=$(call OcpiDbg,Entering HdlGetFamilies($1))$(strip \
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


$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)

ifeq ($(origin HdlPlatforms),undefined)
ifdef HdlPlatform
HdlPlatforms:=$(HdlPlatform)
else
HdlPlatforms:=ml605
endif
endif
ifeq ($(origin HdlTargets),undefined)
ifdef HdlPlatforms
HdlTargets:=$(foreach p,$(HdlPlatforms),$(call HdlGetFamily,$(HdlPart_$p)))
else
HdlTargets:=virtex6
endif
endif
ifeq ($(HdlTargets),all)
override HdlTargets:=$(HdlAllFamilies)
endif

ifeq ($(HdlPlatforms),all)
override HdlPlatforms:=$(HdlAllPlatforms)
override HdlTargets:=$(call Unique,$(HdlTargets) \
	               $(call HdlGetFamilies,$(HdlAllPlatformParts)))
endif


$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)
endif
