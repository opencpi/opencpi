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

ifndef _HDL_TARGETS_
_HDL_TARGETS_=here
# Include this here so that hdl-targets.mk (this file) can be included on its own when
# things like HdlAllPlatforms is required.
include $(OCPI_CDK_DIR)/include/util.mk

# IncludeProject so that platforms in current project are discovered
$(OcpiIncludeProject)

# This file is the database of hdl targets and associated tools
# It is a "leaf file" that is used in several places.

# This is the list of top level targets.
# All other targets are some level underneath these
# The levels are: top, family, part, speed

#Testing: HdlTopTargets=xilinx altera verilator icarus
HdlTopTargets:=xilinx altera modelsim # icarus altera # verilator # altera

# The HdlDefaultTarget_<family> is the one used for core building (primitives, workers...).
# TODO: HdlDefaultTarget_<family> is only supported by Vivado at this time.
# If the default is unset, the first part in a family is the one used for core building.
# Usually the default should be the smallest so that you ensure each worker will fit
# on the smaller parts. If you want to ensure that worker-synthesis uses as many
# resources as necessary, you can set it to a larger part or set HdlExactPart
# for a worker or library.
#
# HdlPart_<platform> in a <platform>.mk file will define a full part. That part
# can be mapped to a part here and therefore a family as well.
# E.g. in zed.mk, HdlPart_zed=xc7z020-1-clg484, which maps to xc7z020, which
# maps to the 'zynq' family with a default target of xc7z020 for building pre-platform cores.
HdlTargets_xilinx:=isim virtex5 virtex6 spartan3adsp spartan6 zynq_ise zynq xsim
HdlTargets_virtex5:=xc5vtx240t xc5vlx50t xc5vsx95t xc5vlx330t xc5vlx110t
HdlTargets_virtex6:=xc6vlx240t
HdlTargets_spartan6:=xc6slx45
HdlTargets_spartan3adsp:=xc3sd3400a
HdlTargets_zynq_ise:=xc7z020_ise_alias
HdlTargets_zynq:=xc7z020 xc7z045
HdlDefaultTarget_zynq:=xc7z020

HdlTargets_altera:=arria10soc_std stratix4 stratix5 # altera-sim
HdlDefaultTarget_stratix4:=AUTO
HdlDefaultTarget_stratix5:=AUTO
# Quartus Pro (and maybe newer versions of standard) does not
# support the 'AUTO' part for arria10 because you cannot reuse
# synthesized partitions from different devices.
# We must enforce one exact part per target for Quartus Pro
# (and maybe newer/17+ versions of standard).
HdlDefaultTarget_arria10soc_std:=10AS066N3F40E2SG_std_alias


# The "k", when present indicates the transceiver count (k = 36)
# But in many places it is left off..
HdlTargets_stratix4:=ep4sgx230k ep4sgx530k ep4sgx360
HdlTargets_stratix5:=ep5sgsmd8k2
HdlTargets_arria10soc_std:=10AS066N3F40E2SG_std_alias

HdlSimTools=isim icarus verilator ghdl xsim modelsim

# Tools are associated with the family or above
HdlToolSet_ghdl:=ghdl
HdlToolSet_isim:=isim
HdlToolSet_xsim:=xsim
HdlToolSet_modelsim:=modelsim
HdlToolSet_spartan3adsp:=xst
HdlToolSet_virtex5:=xst
HdlToolSet_virtex6:=xst
HdlToolSet_spartan6:=xst
HdlToolSet_zynq_ise:=xst
HdlToolSet_zynq:=vivado
HdlToolSet_verilator:=verilator
HdlToolSet_icarus:=icarus
HdlToolSet_stratix4:=quartus
HdlToolSet_stratix5:=quartus
HdlToolSet_arria10soc_std:=quartus

# Call the tool-specific function to get the full part incase the
# tool needs to rearrange the different part elements
# If the tool does not define this function, return part as-is
# Arg1 is the full/exact part number
HdlFullPart=$(or $(call HdlFullPart_$(HdlToolSet),$1),$1)
# In the platform and post-platform stages, get the part from the <platform>.mk
# In other stages, use the HdlExactPart if set, or the Default part if set,
# or the first part for this target
# Arg1 can be optionally set instead of determining the part here.
HdlChoosePart=$(strip \
  $(if $(findstring $(HdlMode),platform config container),\
    $(call HdlFullPart,$(HdlPart_$(HdlPlatform))),\
    $(or \
      $(and $(HdlExactPart),$(call HdlFullPart,$(HdlExactPart))),\
      $(HdlDefaultTarget_$(HdlTarget)),\
      $(firstword $(HdlTargets_$(HdlTarget))))))

# Make the initial definition as a simply-expanded variable
HdlAllPlatforms:=
HdlBuiltPlatforms:=

override OCPI_HDL_PLATFORM_PATH:=$(subst $(Space),:,$(call Unique,\
  $(subst :, ,$(OCPI_HDL_PLATFORM_PATH)) \
  $(foreach p,$(OcpiGetExtendedProjectPath),$(call OcpiExists,$p/lib/platforms))))
export OCPI_HDL_PLATFORM_PATH
$(call OcpiDbgVar,OCPI_HDL_PLATFORM_PATH)
################################################################################
# These functions are here because this file is leaf and used when callers
# only want to know the facts about targets, without pulling in any other
# aspects of the HDL building machinery.
# Otherwise various HDL utilities are in hdl-make.mk which this file does not
# depend on.
################################################################################
HdlError:=error
# Add a platform to the database.
# Arg 1: The directory where the *.mk file is
# Arg 2: The name of the platform
# Arg 3: The actual platform directory for using the platform (which may not exist).
HdlAddPlatform=\
  $(call OcpiDbg,HdlAddPlatform($1,$2,$3))\
  $(if $(HdlPlatformDir_$2),,\
    $(eval include $1/$2.mk)\
    $(eval HdlAllPlatforms+=$2)\
    $(eval HdlPlatformDir_$2:=$3)\
    $(if $(or \
	   $(call OcpiExists,$3/lib/hdl/$(HdlFamily_$(HdlPart_$2))),\
           $(call OcpiExists,$3/hdl/$(HdlFamily_$(HdlPart_$2)))),\
      $(eval HdlBuiltPlatforms+=$2)))

# Call this with a directory that is a platform's directory, either source (with "lib" subdir
# if built) or exported. For the individual platform directories we need to deal with
# the prebuilt, postbuilt, and exported scenarios.  Hence the complexity.
# Both the *.xml and *.mk are generally needed, but the *.mk is more critical here,
# so we key on that.
# If we are pointing at a non-exported platform directory, we prefer its local export subdir
# ("lib"), if the hdl/*.mk is present.
# (under hdl since it is in fact a worker in a library)
HdlDoPlatform=\
  $(foreach p,$(notdir $1),\
    $(foreach d,$(if $(wildcard $1/lib/$p.mk),$1/lib,$1),\
      $(if $(filter clean%,$(MAKECMDGOALS))$(call OcpiExists,$d/hdl/$p.mk)$(call OcpiExists,$d/$p.mk),,$(error no $p.mk file found for platform under: $1))\
      $(if $(wildcard $d/$p.mk),,$(error no $p.mk file found under $1. $p not built?))\
      $(call HdlAddPlatform,$d,$p,$d)))

# Handle a directory named "platforms", exported or not
HdlDoPlatformsDir=\
  $(if $(wildcard $1/mk),\
    $(foreach d,$(wildcard $1/mk/*.mk),\
      $(foreach p,$(basename $(notdir $d)),\
        $(call HdlAddPlatform,$1/mk,$p,$1/$p))),\
    \
    $(foreach d,$(wildcard $1/*),\
      $(foreach p,$(notdir $d),\
        $(if $(wildcard $d/$p.mk)$(wildcard $d/lib/hdl/$p.mk)$(wildcard $d/hdl/$p.mk),\
          $(call HdlDoPlatform,$d)))))

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

$(call OcpiDbgVar,HdlAllPlatforms)
$(call OcpiDbgVar,OCPI_HDL_PLATFORM_PATH)
# The warning below would apply, e.g. if a new project has been registered.
$(foreach d,$(subst :, ,$(OCPI_HDL_PLATFORM_PATH)),\
  $(if $(wildcard $d),,$(warning "$d" does not exist, so no hardware platform(s) can be imported from it))\
  $(if $(filter platforms,$(notdir $d)),\
    $(call HdlDoPlatformsDir,$d),\
    $(call HdlDoPlatform,$d)))

# Families are either top level targets with nothing underneath or one level down
HdlAllFamilies:=$(call Unique,$(foreach t,$(HdlTopTargets),$(or $(HdlTargets_$(t)),$(t))))
HdlAllTargets:=$(call Unique,$(HdlAllFamilies) $(HdlTopTargets))
export OCPI_ALL_HDL_TARGETS:=$(HdlAllTargets)
export OCPI_ALL_HDL_PLATFORMS:=$(strip $(HdlAllPlatforms))
export OCPI_BUILT_HDL_PLATFORMS:=$(strip $(HdlBuiltPlatforms))
$(call OcpiDbgVar,HdlAllFamilies)
$(call OcpiDbgVar,HdlAllPlatforms)
$(call OcpiDbgVar,HdlBuiltPlatforms)
#$(info OCPI_ALL_HDL_PLATFORMS is $(OCPI_ALL_HDL_PLATFORMS))
#$(info OCPI_ALL_HDL_TARGETS is $(OCPI_ALL_HDL_TARGETS))
# Assignments that can be used to extract make variables into bash/python...
ifdef ShellHdlTargetsVars
all:
$(info HdlTopTargets="$(HdlTopTargets)";\
       HdlSimTools="$(HdlSimTools)";\
       HdlAllFamilies="$(HdlAllFamilies)";\
       HdlAllPlatforms="$(HdlAllPlatforms)";\
       HdlBuiltPlatforms="$(HdlBuiltPlatforms)";\
       HdlAllTargets="$(HdlAllTargets)";\
       HdlTargets="$(foreach t,$(HdlTopTargets),$(or $(HdlTargets_$t),$t))";\
       $(foreach p,$(HdlAllPlatforms),HdlPart_$p=$(HdlPart_$p); )\
       $(foreach f,$(HdlAllTargets),\
         $(if $(HdlTargets_$f),HdlTargets_$f="$(HdlTargets_$f)";)\
         $(if $(HdlToolSet_$f),HdlToolSet_$f="$(HdlToolSet_$f)";)\
         $(foreach t,$(HdlTargets_$f),\
           $(if $(HdlTargets_$t),HdlTargets_$t="$(HdlTargets_$t)";)))\
       $(foreach t,$(call Unique,\
                     $(foreach f,$(HdlAllTargets),$(if $(HdlToolSet_$f),$(HdlToolSet_$f) ))),\
         $(eval __ONLY_TOOL_VARS__:=true)\
         $(eval include $(OCPI_CDK_DIR)/include/hdl/$t.mk)\
         HdlToolName_$t="$(or $(HdlToolName_$t),$t)";)\
       $(foreach p,$(HdlAllPlatforms),\
         HdlFamily_$(HdlPart_$p)=$(call HdlGetFamily,$(HdlPart_$p));))
endif
endif
