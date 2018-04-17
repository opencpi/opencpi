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

# Makefile for RCC common definitions not specific to "workers"
ifndef RCC_MAKE_MK
RCC_MAKE_MK:=xxx
include $(OCPI_CDK_DIR)/include/util.mk
#
# The block below needs to happen prior to the assignments below for
# extracting RccTarget/Platform info from this makefile
ifdef ShellRccTargetsVars
$(OcpiIncludeProject)
# When collecting a list of RCC targets/platforms, you do not need to be inside a project.
# So, collect all projects in the Project Registry Dir into the project path for searching.
# If inside a project, the registry should be searched automatically via the project's imports.
ifeq ($(OCPI_PROJECT_DIR),)
  export OCPI_PROJECT_PATH:=$(OCPI_PROJECT_PATH):$(subst $(Space),:,$(wildcard $(OcpiProjectRegistryDir)/*))
endif
endif

RccOs=$(word 1,$(subst -, ,$(or $(RccTarget_$1),$1,$(RccTarget))))
RccOsVersion=$(word 2,$(subst -, ,$1))
RccArch=$(word 3,$(subst -, ,$1))

$(call OcpiDbgVar,RccPlatforms)
$(call OcpiDbgVar,RccTargets)

# for a clean environment, ensure OCPI_TOOL_PLATFORM at least
$(eval $(OcpiEnsureToolPlatform))

# Allow the option of specifying an rcc platform by referencing the associated
# HDL platform, but only incurring the overhead of this search when this variable
# is specified.
ifdef RccHdlPlatform
  override RccHdlPlatforms+= $(RccHdlPlatform)
endif
ifdef RccHdlPlatforms
  include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk
  $(foreach p,$(RccHdlPlatforms),\
     $(if $(filter $p,$(HdlAllPlatforms)),\
       $(if $(HdlRccPlatform_$p),\
         $(eval override RccPlatforms:=$(call Unique,$(RccPlatforms) $(HdlRccPlatform_$p))), \
         $(eval override RccPlatforms:=$(call Unique,$(RccPlatforms) $(OCPI_TOOL_PLATFORM)))$(warning There is no RCC platform associated with the HDL platform: $p. Using $(OCPI_TOOL_PLATFORM))),\
       $(eval override RccPlatforms:=$(call Unique,$(RccPlatforms) $(OCPI_TOOL_PLATFORM)))$(warning There is no HDL platform named: $p, so no RCC platform for it. Using $(OCPI_TOOL_PLATFORM))\
     ))
endif

ifdef RccPlatforms
  # nothing here - we process later
else ifdef RccPlatform
  RccPlatforms:=$(RccPlatform)
else ifdef RccTargets
  # nothing to do here
else ifdef RccTarget
  RccTargets:=$(RccTarget)
else ifeq ($(origin RccPlatforms),undefined)
  ifdef OCPI_TARGET_PLATFORM
    RccPlatforms:=$(OCPI_TARGET_PLATFORM)
  else ifdef OCPI_TOOL_PLATFORM
    RccPlatforms:=$(OCPI_TOOL_PLATFORM)
  else
    $(error Unexpected failure to figure out which RCC compiler to use.)
  endif
endif

$(call OcpiDbgVar,RccPlatforms)
$(call OcpiDbgVar,RccTargets)

RccAllPlatforms:=
RccAllTargets:=
RccAddPlatform=\
    $(eval RccAllPlatforms=$(strip $(RccAllPlatforms) $1))\
    $(eval RccAllTargets=$(strip $(RccAllTargets) $2))\
    $(eval RccTarget_$1:=$2)\
    $(eval RccPlatformDir_$1:=$3)

ifdef OCPI_ALL_RCC_PLATFORMS
  RccAllPlatforms:=$(OCPI_ALL_RCC_PLATFORMS)
  RccAllTargets:=$(OCPI_ALL_RCC_TARGETS)
  $(foreach p,$(OCPI_RCC_PLATFORM_TARGETS),\
    $(eval RccTarget_$(word 1,$(subst =, ,$p)):=$(word 2,$(subst =, ,$p))))
else
  $(foreach d,$(OcpiGetRccPlatformPaths),\
    $(foreach p,$(wildcard $d/*),\
      $(and $(wildcard $p/$(notdir $p)-target.mk),$(wildcard $p/target),\
        $(call RccAddPlatform,$(notdir $p),$(shell cat $p/target),$p))))
  RccAllTargets:=$(call Unique,$(RccAllTargets))
  export OCPI_ALL_RCC_PLATFORMS:=$(RccAllPlatforms)
  export OCPI_ALL_RCC_TARGETS:=$(RccAllTargets)
  export OCPI_RCC_PLATFORM_TARGETS:=$(foreach p,$(RccAllPlatforms),$p=$(RccTarget_$p))
#  $(info OCPI_ALL_RCC_PLATFORMS is $(OCPI_ALL_RCC_PLATFORMS))
#  $(info OCPI_ALL_RCC_TARGETS is $(OCPI_ALL_RCC_TARGETS))
endif
$(call OcpiDbgVar,RccAllPlatforms)
$(call OcpiDbgVar,RccAllTargets)
$(foreach p,$(RccAllPlatforms),$(call OcpiDbgVar,RccTarget_$p))
# This must be called with a list of platforms
RccRealPlatforms=$(infox GETREAL:$1)$(foreach p,$1,$(word 1,$(subst -, ,$p)))
# This operates on the target-specific variable assignment of RccPlatform
RccRealPlatform=$(strip $(infox RRP:$(RccPlatform))\
                $(foreach p,$(call RccRealPlatforms,$(RccPlatform)),$(infox RRPr:$p)$p))
# Find the platform that has the argument as a platform
RccGetPlatform=$(strip\
  $(or $(foreach v,$(filter RccTarget_%,$(.VARIABLES)),$(infox VV:$v:$($v))\
         $(foreach p,$(v:RccTarget_%=%),\
           $(and $(filter $p,$(RccAllPlatforms)),$(filter $1,$(value $v)),\
	    $(infox RGPr:$1:$v:$p:$(value $v))$p))),\
	$($(or $2,error) Cannot find an RCC platform for the target: $1)))
# The model-specific determination of the "tail end" of the target directory,
# after the prefix (target), and build configuration.
# The argument is a TARGET
RccTargetDirTail=$(strip\
  $(or $(and $(RccTarget_$1),$1),\
       $(and $(filter $1,$(RccTarget_$(RccPlatform))),$(RccPlatform)),\
       $(call RccGetPlatform,$1,error)))

# Transfer build options to target from platform
# $(call RccPlatformTarget,<platform>,<target>)
RccPlatformTarget=$2$(foreach b,$(word 2,$(subst -, ,$1)),$(and $b,-$b))
ifdef RccPlatforms
  # We filter platforms that might have build options, but we filter based on the real platforms
  override RccPlatforms:=$(strip\
    $(foreach p,$(RccPlatforms),\
      $(if $(filter $(call RccRealPlatforms,$p),$(ExcludePlatforms) $(RccExcludePlatforms)),,$p)))
  ifneq ($(OnlyPlatforms)$(RccOnlyPlatforms),)
    override RccPlatforms:=$(strip\
      $(foreach p,$(RccPlatforms),\
        $(if $(filter $(call RccRealPlatforms,$p),$(OnlyPlatforms) $(RccOnlyPlatforms)),$p,)))
  endif
  RccTargets:=
  $(foreach p,$(RccPlatforms),\
    $(foreach r,$(call RccRealPlatforms,$p),\
      $(if $(filter $r,$(RccAllPlatforms)),,\
        $(error RccPlatform $r is unknown, it is not found in any registered project))\
      $(eval RccTarget_$p:=$(call RccPlatformTarget,$p,$(RccTarget_$r)))\
      $(eval RccTargets=$(strip $(RccTargets) $(RccTarget_$p)))))
else
  # Derive a platform from each target (somewhat expensive, but we have shortcuts)
  # This can be deprecated or accelerated as it makes sense
  # An easy accelerator would be to make the target an actual file in the platform's dir
  # (A single wildcard then does it)
  # We could also cache this
  RccPlatforms:=
  $(foreach t,$(RccTargets),\
    $(eval RccFound:=)\
    $(foreach p,$(RccAllPlatforms),\
      $(if $(filter $t,$(RccTarget_$p)),\
        $(eval RccPlatforms+=$p)\
        $(eval RccFound+=$t)))\
    $(if $(RccFound),,\
      $(error The RccTarget "$t" is not the target for any software platform in any registered project)))
endif

$(call OcpiDbgVar,RccPlatforms)
$(call OcpiDbgVar,RccTargets)

# This should avoid confusion since they should not be used after this point
override RccTarget:=
override RccPlatform:=

# Read in all the tool sets indicated by the targets
#
ifeq ($(filter clean cleanrcc,$(MAKECMDGOALS)),)
# include all the rcc compilation files for all target platforms
RccRTP:=$(call RccRealPlatforms,$(OCPI_TOOL_PLATFORM))
$(foreach x,$(RccPlatforms),\
  $(foreach t,$(RccTarget_$x),\
    $(eval files:=)\
    $(eval cross:=)\
    $(eval p:=$(call RccRealPlatforms,$x))\
    $(foreach d,$(call OcpiGetRccPlatformDir,$p),\
      $(foreach m,$(if $(filter $(RccRTP),$p),rcc=$p,rcc=$(RccRTP)=$p),\
        $(eval files:=$(files) $(wildcard $d/$m.mk))\
        $(and $(findstring =,$(subst rcc=,,$m)),$(eval cross:=1)))\
      $(foreach n,$(words $(files)),\
         $(if $(filter 0,$n),\
	    $(if $(cross),\
               $(error There is no cross compiler defined from $(OCPI_TOOL_PLATFORM) to $p),\
               $(eval include $(OCPI_CDK_DIR)/include/rcc/default.mk)),\
	    $(if $(filter 1,$n),,\
               $(warning More than one file defined for compiling $(OCPI_TOOL_PLATFORM) to $p, using $(word 1,$(files)), others are $(wordlist 2,$(words $(files)),$(files)).))\
            $(eval include $(word 1,$(files))))))))
endif
endif

# Assignments that can be used to extract make variables into bash/python...
ifdef ShellRccTargetsVars
all:
$(info RccAllPlatforms="$(sort $(RccAllPlatforms))";\
       RccPlatforms="$(sort $(RccPlatforms))";\
       RccAllTargets="$(sort $(RccAllTargets))";\
       RccTargets="$(sort $(RccTargets))";\
       $(foreach p,$(sort $(RccAllPlatforms)),\
         $(if $(RccTarget_$p),RccTarget_$p="$(RccTarget_$p)";)))
endif
#$(foreach t,$(RccTopTargets),$(or $(RccTargets_$t),$t))";\
