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

# Makefile for OCL common definitions not specific to "workers"
ifndef OCL_MAKE_MK
OCL_MAKE_MK:=xxx
include $(OCPI_CDK_DIR)/include/util.mk

OclOs=opencl
OclOsVersion=$(word 1,$(subst -, ,$1))
OclArch=$(word 2,$(subst -, ,$1))

$(call OcpiDbgVar,OclPlatform)
$(call OcpiDbgVar,OclPlatforms)
$(call OcpiDbgVar,OclTarget)
$(call OcpiDbgVar,OclTargets)

# OCL targets and platforms are dynamically determined by querying the OpenCL subsystem
ifdef OCPI_ALL_OCL_PLATFORMS
  OclAllPlatforms:=$(OCPI_ALL_OCL_PLATFORMS)
  OclAllTargets:=$(OCPI_ALL_OCL_TARGETS)
  OclTargetMap:=$(OCPI_OCL_TARGET_MAP)
else ifeq ($(OCPI_HAVE_OPENCL),1)
  OclTargetMap:=
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    # OCPI_HAVE_OPENCL enables us to even try to use OpenCL
    # OCPI_FOUND_OPENCL is internal telling us the guard has succeeeded previously
    $(infox OCPI_HAVE_OPENCL=$(OCPI_HAVE_OPENCL) and OCPI_FOUND_OPENCL=$(OCPI_FOUND_OPENCL))
    ifeq ($(OCPI_HAVE_OPENCL),1)
      ifndef OCPI_FOUND_OPENCL
        ifeq ($(call DoShell,\
                OCPI_OPENCL_LIB=$(OCPI_OPENCL_LIB) $(ToolsDir)/ocpiocltest test,OclTargetMap,1),)
          export OCPI_FOUND_OPENCL:=1
        endif
      endif
    endif
    ifdef OCPI_FOUND_OPENCL
      $(and $(call DoShell,\
               OCPI_OPENCL_LIB=$(OCPI_OPENCL_LIB) $(ToolsDir)/ocpiocl targets,OclTargetMap),\
             $(error $(OclTargetMap)))
      OclAllTargets:=$(foreach p,$(OclTargetMap),$(word 2,$(subst =, ,$p)))
      OclAllPlatforms:=$(foreach p,$(OclTargetMap),$(word 1,$(subst =, ,$p)))
      export OCPI_ALL_OCL_PLATFORMS:=$(OclAllPlatforms)
      export OCPI_ALL_OCL_TARGETS:=$(OclAllTargets)
      export OCPI_OCL_TARGET_MAP:=$(OclTargetMap)
    endif
  endif
endif
$(foreach m,$(OclTargetMap),\
  $(eval OclTarget_$(word 1,$(subst =, ,$m)):=$(word 2,$(subst =, ,$m))))
# Mostly copied from rcc...
ifdef OclPlatform
  OclPlatforms:=$(call Unique,$(OclPlatforms) $(OclPlatform))
endif
ifdef OclTarget
  OclTargets:=$(call Unique,$(OclTargets) $(OclTarget))
endif
ifeq ($(origin OclPlatforms),undefined)
  ifeq ($(origin OclPlatform),undefined)
    OclPlatforms=all
  endif
endif
ifdef OclPlatforms
  ifeq ($(OclPlatforms),all)
    OclPlatforms:=$(OclAllPlatforms)
  endif
  override OclPlatforms:=$(filter-out $(ExcludePlatforms) $(OclExcludePlatforms),$(OclPlatforms))
  ifneq ($(OnlyPlatforms)$(OclOnlyPlatforms),)
    override OclPlatforms:=$(filter $(OnlyPlatforms) $(OclOnlyPlatforms),$(OclPlatforms))
  endif
  OclTargets:=
  $(foreach p,$(OclPlatforms),\
     $(if $(filter $p,$(OclAllPlatforms)),\
        $(eval OclTargets+=$(OclTarget_$p)),\
        $(error OclPlatform $p is unknown/unsupported on this system)))
  OclTargets:=$(call Unique,$(OclTargets))
else
  ifdef OclTargets
    OclPlatforms:=
    $(foreach t,$(OclTargets),\
      $(if $(filter $t,$(OcpiAllTargets)),\
        $(foreach m,$(filter %=$t,$(OcpiTargetMap)),\
          $(eval OcpiPlatforms+=$(word 1,$(subst =, ,$m))))))
  else
    OclPlatforms:=$(OclAllPlatforms)
    OclTargets:=$(foreach p,$(OclPlatforms),$(OclTarget_$p))
  endif
endif
$(call OcpiDbgVar,OclPlatform)
$(call OcpiDbgVar,OclPlatforms)
$(call OcpiDbgVar,OclTarget)
$(call OcpiDbgVar,OclTargets)
endif
