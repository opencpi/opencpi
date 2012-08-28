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

# Makefile fragment for HDL primitives, cores, and workers etc.
# 'pre' means should be included before anything else
# This file is target and tool independent: LET's KEEP IT THAT WAY
# This file will be included at the start of:
#  primitive libraries
#  imported cores
#  workers
#  device workers
#  platforms
#  applications

ifndef __HDL_PRE_MK__
__HDL_PRE_MK__=x
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
Model=hdl

################################################################################
# The generic hdl compile that depends on HdlToolCompile
HdlLog=$(or $(Core),$(LibName))-$(HdlToolSet).out
HdlTime=$(or $(Core),$(LibName))-$(HdlToolSet).time
HdlCompile=\
  cd $(TargetDir); \
  export HdlCommand="$(HdlToolCompile)"; \
  $(TIME) sh -c \
   '(/bin/echo Command: "$$HdlCommand"; eval "$$HdlCommand") > $(HdlLog) 2>&1' \
    > $(HdlTime) 2>&1; \
  HdlExit=$$?; \
  cat $(HdlTime) >> $(HdlLog); \
  grep -i error $(HdlLog)| grep -v Command: |\
    grep -v '^WARNING:'|grep -v " 0 errors," | grep -i -v '[_a-z]error'; \
  $(HdlToolPost) \
  if test "$$OCPI_HDL_VERBOSE_OUTPUT" != ''; then \
    cat $(HdlLog); \
  fi; \
  if test $$HdlExit != 0; then \
    $(ECHO) Error: $(HdlToolSet) failed\($$HdlExit\). See $(TargetDir)/$(HdlLog); \
  else \
    $(ECHO) -n ' '$(HdlToolSet) succeeded.'  '; \
    cat $(HdlTime); \
  fi; \
  rm -f $(HdlTime); \
  exit $$HdlExit
################################################################################
# The post processing by simulators that do not produce any intermediate
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

define HdlSimNoLibraries
HdlToolPost=$$(HdlSimPost)
HdlToolLinkFiles=$$(sort\
  $$(HdlToolFiles) \
  $$(foreach f,$$(DefsFile) $$(ImplHeaderFiles),\
     $$(call FindRelative,$$(TargetDir),$$(f))))
endef



################################################################################
# $(call HdlLibraryRefDir,location-dir)
# $(call HdlCoreRefDir,location-dir,target)
# These functions take a user-specified (friendly, target-independent) library
# or core location and a target name.  They return the actual directory of that
# library/core that the tool wants to see for that target.
# These are not for component libraries, but a primitive libraries and cores
HdlLibraryRefDir=$(strip \
  $(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/hdl/$1)/$(strip \
    $(call HdlToolLibRef,$(notdir $1),$2,$3)))

HdlCoreRefDir=$(if $(findstring /,$1),$1,$(OCPI_CDK_DIR)/lib/hdl/$1)/$2


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

################################################################################
# $(call HdlGetTargetsForToolSet,toolset,targets)
# Return all the targets that work with this too
HdlGetTargetsForToolSet=$(sort \
    $(foreach t,$(2),\
       $(and $(findstring $(1),$(call HdlGetToolSet,$(t))),$(t))))

# The general pattern is:
# If Target is specified, build for that target.
# If Targets is specified, build for all, BUT, if they need
# different toolsets, we recurse into make for each set of targets that has a common
# set of tools
$(call OcpiDbgVar,HdlTarget)
$(call OcpiDbgVar,HdlTargets)
ifneq ($(HdlTarget),)
override HdlTargets=$(HdlTarget)
endif
ifeq ($(HdlTargets),)
override HdlTargets=all
endif

$(call OcpiDbgVar,HdlTargets)
$(call OcpiDbgVar,HdlTarget)
$(call OcpiDbgVar,OnlyTargets)
# Map "all" and top level targets down into "families"
HdlActualTargets:=$(sort \
              $(foreach t,$(HdlTargets),\
                 $(if $(findstring $t,all)$(findstring $t,$(HdlTopTargets)),\
                    $(call HdlGetFamily,$t,x),\
                    $t)))
$(call OcpiDbgVar,HdlActualTargets)
# Map "only" targets down to families too
HdlOnlyTargets:=$(sort \
              $(foreach t,\
                $(or $(OnlyTargets),all),\
                  $(if $(findstring $(t),all)$(findstring $(t),$(HdlTopTargets)),\
                    $(call HdlGetFamily,$(t),x),\
                    $(t))))
$(call OcpiDbgVar,HdlOnlyTargets)
# Now prune to include only targets mentioned in OnlyTargets
# Question is:  for each target, is it in onlytargets?
# We know that we all at the family level or the part level
# So we have several cases, other than pure matches
# target is family, only is part
#  -- replace with part.
# target is part, only is family
#  -- replace with part
HdlPreExcludeTargets:=$(HdlActualTargets)
$(call OcpiDbgVar,HdlPreExcludeTargets,Before only: )
HdlActualTargets:=$(sort \
              $(foreach t,$(HdlActualTargets),\
		 $(or $(findstring $(t),$(HdlOnlyTargets)), \
                   $(and $(findstring $(t),$(HdlAllFamilies)), \
		     $(foreach o,$(HdlOnlyTargets), \
		       $(if $(findstring $(t),$(call HdlGetFamily,$(o))),$(o)))), \
                   $(foreach o,$(HdlOnlyTargets),\
		      $(if $(findstring $(o),$(call HdlGetFamily,$(t))),$(t))))))
$(call OcpiDbgVar,HdlActualTargets,After only: )
# Now prune to exclude targets mentioned in ExcludeTargets
# We don't expand families into constituents, but we do
# convert a family into its parts if some of the parts are excluded
$(call OcpiDbgVar,ExcludeTargets,Makefile exclusions: )
$(call OcpiDbgVar,OCPI_EXCLUDE_TARGETS,Environment exclusions: )
ExcludeTargetsInternal:=\
$(sort $(foreach t,$(ExcludeTargets) $(OCPI_EXCLUDE_TARGETS),\
         $(if $(and $(findstring $t,$(HdlTopTargets)),$(HdlTargets_$t)),\
             $(HdlTargets_$t),$t)))

$(call OcpiDbgVar,ExcludeTargetsInternal)
HdlActualTargets:=$(sort \
 $(foreach t,$(HdlActualTargets),\
   $(if $(findstring $t,$(ExcludeTargetsInternal)),,\
      $(if $(findstring $t,$(HdlAllFamilies)),\
	  $(if $(filter $(HdlTargets_$t),$(ExcludeTargetsInternal)),\
	      $(filter-out $(ExcludeTargetsInternal),$(HdlTargets_$t)),\
              $(t)),\
          $(if $(findstring $(call HdlGetFamily,$t),$(ExcludeTargetsInternal)),,\
               $t)))))
$(call OcpiDbgVar,HdlActualTargets,After exclusion: )
override HdlTargets:=$(HdlActualTargets)

HdlFamilies=$(call HdlGetFamilies,$(HdlActualTargets))
$(call OcpiDbgVar,HdlFamilies)

HdlToolSets=$(sort $(foreach t,$(HdlFamilies),$(call HdlGetToolSet,$(t))))
# We will already get an error if there are no toolsets.
$(call OcpiDbgVar,HdlToolSets)

# In all cases if SourceFiles is not specified in the Makefile,
# we look for any relevant
ifndef SourceFiles
CompiledSourceFiles:= $(wildcard *.[vV]) $(wildcard *.vhd) $(wildcard *.vhdl)
else
CompiledSourceFiles:= $(SourceFiles)
endif

ifndef HdlInstallDir
HdlInstallDir=$(OCPI_CDK_DIR)/lib/hdl
$(HdlInstallDir):
	$(AT)mkdir -p $@
endif

################################################################################
# Now we decide whether to recurse, and run a sub-make per toolset, or, if we
# have only one toolset for all our targets, we just build for those targets in
# this make process.
ifneq ($(word 2,$(HdlToolSets)),)
################################################################################
# So here we recurse.  Note we are recursing for targets and NOT platforms.
$(call OcpiDbg,=============Recursing with all: HdlToolSets=$(HdlToolSets))
all: $(HdlToolSets)
install: $(HdlToolSets:%=install_%)
stublibrary: $(HdlToolSets:%=stublibrary_%)
define HdlDoToolSet
$(1):
	$(AT)$(MAKE) --no-print-directory HdlPlatforms="$(HdlPlatforms)" HdlTarget= \
           HdlTargets="$(call HdlGetTargetsForToolSet,$(1),$(HdlActualTargets))"

stublibrary_$(1):
	$(AT)$(MAKE) --no-print-directory HdlPlatforms= HdlTarget= \
           HdlTargets="$(call HdlGetTargetsForToolSet,$(1),$(HdlActualTargets))" \
	   stublibrary

install_$(1):
	$(AT)$(MAKE) --no-print-directory HdlPlatforms= HdlTarget= \
           HdlTargets="$(call HdlGetTargetsForToolSet,$(1),$(HdlActualTargets))" \
           install
endef
$(foreach ts,$(HdlToolSets),$(eval $(call HdlDoToolSet,$(ts))))
# this "skip" tells the file that included this file, that it shouldn't do anything
# after including this file, and thus "skip" the rest of its makefile.
HdlSkip=1

################################################################################
# Here is where we ended up with nothing to do due to filtering
else ifeq ($(HdlToolSets),)
$(call OcpiDbg,=============No tool sets at all, skipping)
$(info Not building filtered (only/excluded) targets: $(HdlPreExcludeTargets))
HdlSkip=1
install:
else
################################################################################
# Here we are NOT recursing, but simply build targets for one toolset in this 
# make.
$(call OcpiDbg,=============Performing for one tool set: $(HdlToolSets). $(HdlPlatforms))
HdlSkip=
HdlToolSet=$(HdlToolSets)
$(call OcpiDbgVar,HdlToolSet)

ifneq ($(HdlToolSet),)
include $(OCPI_CDK_DIR)/include/hdl/$(HdlToolSet).mk
ifneq ($(findstring platform,$(HdlMode)),)
HdlTolNeedBB:=
endif
ifneq ($(findstring $(HdlToolSet),$(HdlSimTools)),)
HdlSimTool=yes
endif
endif # we have a tool set
endif # for multiple tool sets.

################################################################################
# These are rules for both recursive and non-recursive cases.
clean:: cleanfirst
	$(AT)$(if $(findstring $(HdlMode),worker),,\
		echo Cleaning HDL $(HdlMode) `pwd` for all targets.;)\
	rm -r -f $(OutDir)target-* $(OutDir)gen

cleanfirst::
cleanimports::
	rm -r -f imports

ImportsDir=imports

endif # include this once
