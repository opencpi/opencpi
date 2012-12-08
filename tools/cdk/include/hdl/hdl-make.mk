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


#HdlAllParts:=$(sort $(foreach t,$(HdlTopTargets),$(if $(findstring $(f),$(HdlSimTools)),,$(foreach f,$(HdlTargets_$(t)),$(HdlTargets_$(f))))))
$(call OcpiDbgVar,HdlTopTargets)
HdlAllParts:=$(sort $(foreach t,$(HdlTopTargets),\
	              $(or $(foreach f,$(HdlTargets_$t),\
                             $(or $(HdlTargets_$f),$f)),$t)))
$(call OcpiDbgVar,HdlAllParts)

HdlAllPlatformParts:=$(sort $(foreach pl,$(HdlAllPlatforms),$(firstword $(subst -, ,$(HdlPart_$(pl))))))
$(call OcpiDbgVar,HdlAllPlatformParts)

# Families are either top level targets with nothing underneath or one level down
HdlAllFamilies:=$(sort $(foreach t,$(HdlTopTargets),$(or $(HdlTargets_$(t)),$(t))))
$(call OcpiDbgVar,HdlAllFamilies)


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
  $(foreach fs,$(sort $(foreach t,$1,\
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
HdlPlatforms:=ml605
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
override HdlTargets:=$(sort $(HdlTargets) \
	               $(call HdlGetFamilies,$(HdlAllPlatformParts)))
endif


$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)
endif
