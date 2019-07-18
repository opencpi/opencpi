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

##########################################################################################
# This file preprocesses the RccPlatform(s) and RccTarget(s) variables, defines common
# RCC-related definitions not specific to "workers", and ensures that all the platform
# variables are set for the requested platforms.
# Thus this file has significant side-effects
ifndef RCC_MAKE_MK
RCC_MAKE_MK:=xxx
include $(OCPI_CDK_DIR)/include/rcc/rcc-targets.mk

$(call OcpiDbgVar,RccPlatforms)
$(call OcpiDbgVar,RccTargets)

# for a clean environment, ensure OCPI_TOOL_PLATFORM at least, unlikely needed, but...
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
         $(eval override RccPlatforms:=$(call Unique,$(RccPlatforms) $(OCPI_TOOL_PLATFORM)))\
         $(warning There is no RCC platform associated with the HDL platform: $p. Using $(OCPI_TOOL_PLATFORM))),\
       $(eval override RccPlatforms:=$(call Unique,$(RccPlatforms) $(OCPI_TOOL_PLATFORM)))\
       $(warning There is no HDL platform named: $p, so no RCC platform for it. Using $(OCPI_TOOL_PLATFORM))\
     ))
endif

# Be careful to respect empty, but specified platform and target lists
ifneq ($(origin RccPlatforms),undefined)
  # nothing here - we process later
else ifneq ($(origin RccPlatform),undefined)
  RccPlatforms:=$(RccPlatform)
else ifneq ($(origin RccTargets),undefined)
  # nothing to do here
else ifneq ($(origin RccTarget),undefined)
  RccTargets:=$(RccTarget)
else ifeq ($(origin RccPlatforms),undefined)
  ifdef OCPI_TARGET_PLATFORM
    RccPlatforms:=$(OCPI_TARGET_PLATFORM)
  else ifdef OCPI_TOOL_PLATFORM
    # If no target platform was specified, and we are not cleaning, set to the running one
    ifeq ($(filter clean%,$(MAKECMDGOALS)),)
      RccPlatforms:=$(OCPI_TOOL_PLATFORM)
    endif
  else
    $(error Unexpected failure to figure out which RCC compiler to use.)
  endif
endif

$(call OcpiDbgVar,RccPlatforms)
$(call OcpiDbgVar,RccTargets)

# This must be called with a list of platforms
# It converts "platforms that might have build options" to "platforms"
# I.e. strips off the build options from the input list
RccRealPlatforms=$(infox GETREAL:$1)$(foreach p,$1,$(word 1,$(subst -, ,$p)))
# This operates on the target-specific variable assignment of RccPlatform
# And strips off the build options if present
RccRealPlatform=$(strip $(infox RRP:$(RccPlatform))\
                $(foreach p,$(call RccRealPlatforms,$(RccPlatform)),$(infox RRPr:$p)$p))
# Find the platform that has the argument as a target
# Look through all RccTarget_% variables (where the value or RccTarget_<platform> is the target
# of the <platform>) to find one that maps a platform to the target in $1.  Return <platform>
# This relies on the 1:1 mapping of rcc platforms and targets
RccGetPlatform=$(strip $(infox RGP:$1:$2:)\
  $(or $(filter $1,$(RccAllPlatforms)),\
       $(foreach v,$(filter RccTarget_%,$(.VARIABLES)),$(infox VV:$v:$($v))\
         $(foreach p,$(v:RccTarget_%=%),\
           $(and $(filter $p,$(RccAllPlatforms)),$(filter $1,$(value $v)),\
	         $(infox RGPr:$1:$v:$p:$(value $v))$p))),\
       $($(or $2,error) Cannot find an RCC platform for the target: $1)))
# The model-specific determination of the "tail end" of the target directory,
# after the prefix (target), and build configuration.
# The argument is a TARGET, more or less for legacy reasons now.
RccTargetDirTail=$(infox RTDT:$1:$(RccTarget_$1):$(RccPlatform))$(strip\
  $(or $(and $(RccTarget_$1),$1),\
       $(and $(filter $1,$(RccTarget_$(RccPlatform))),$(RccPlatform)),\
       $(call RccGetPlatform,$1,error)))

# Transfer build options to target from platform
# $(call RccPlatformTarget,<platform>,<target>)
RccPlatformTarget=$2$(foreach b,$(word 2,$(subst -, ,$1)),$(and $b,-$b))
ifdef RccPlatforms
  # Exclude any platform whose real platform matches one in the exluded platform lists
  override RccPlatforms:=$(strip\
    $(foreach p,$(RccPlatforms),\
      $(if $(filter $(call RccRealPlatforms,$p),$(ExcludePlatforms) $(RccExcludePlatforms)),,$p)))
  # If (Rcc)OnlyPlatforms is specified, retain only platforms whose real platform is in the list
  ifneq ($(OnlyPlatforms)$(RccOnlyPlatforms),)
    override RccPlatforms:=$(strip\
      $(foreach p,$(RccPlatforms),\
        $(if $(filter $(call RccRealPlatforms,$p),$(OnlyPlatforms) $(RccOnlyPlatforms)),$p,)))
  endif
  RccTargets:=
  $(call OcpiDbgVar,RccTargets)
  # Set the target (RccTarget_<platform>) for each platform specified, retaining
  # build options here on both sides.  If there are no build options for a platform this
  # essentially rewrites the RccTarget_<platform> variable that is already set from
  # RccAllPlatforms.  All create the RccTargets list too.
  $(foreach p,$(RccPlatforms),\
    $(foreach r,$(call RccRealPlatforms,$p),\
      $(if $(filter $r,$(RccAllPlatforms)),,\
	$(if $(filter $r,$(OCPI_TOOL_PLATFORM)),\
          $(error Host RCC Platform "$r" was not found in any registered project. Make sure the OpenCPI core project is registered as well as any other projects that may contain the missing RCC Platform. Current valid RCC Platforms are: $(RccAllPlatforms)),\
          $(error RCC Platform "$r" was not found in any registered project. Current valid RCC Platforms are: $(RccAllPlatforms))))\
      $(foreach t,$(call RccPlatformTarget,$p,$(RccTarget_$r)),\
        $(eval RccTarget_$p:=$t)\
        $(eval RccTargets=$(strip $(RccTargets) $t)))))
  $(call OcpiDbgVar,RccPlatforms)
  $(call OcpiDbgVar,RccTargets)
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
      $(error The RccTarget "$t" is not a valid target. Specifying an RccPlatform is preferred. Valid RCC Platforms are: $(RccAllPlatforms))))
endif

$(call OcpiDbgVar,RccPlatforms)
$(call OcpiDbgVar,RccTargets)

# This should avoid confusion since they should not be used after this point
override RccTarget:=
override RccPlatform:=

# Allow either platforms or targets for now
# Unfortunately these are called sometimes with target assignment of RccTarget and
# sometimes not.
RccOs=$(or $(OcpiPlatformOs_$1),$(word 1,$(subst -, ,$(or $1,$(RccTarget),$(error Internal)))))
RccOsVersion=$(or $(OcpiPlatformOsVersion_$1),$(word 2,$(subst -, ,$1)))
RccArch=$(or $(OcpiPlatformArch_$1),$(word 3,$(subst -, ,$1)))

# Read in all the tool sets for the platforms we are building
ifeq ($(filter clean cleanrcc,$(MAKECMDGOALS)),)
  $(foreach x,$(RccPlatforms),\
      $(eval $(call OcpiSetPlatformVariables,$(call RccRealPlatforms,$x))))
endif
endif # guard for whole file
