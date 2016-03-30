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

# Makefile for RCC common definitions not specific to "workers"
ifndef RCC_MAKE_MK
RCC_MAKE_MK:=xxx
include $(OCPI_CDK_DIR)/include/util.mk

# This allows component library level RCC libraries to be fed down to workers,
# via RccLibrariesInternal, while allowing the worker itself to have more libraries
# via setting Libraries or RccLibraries.
# Library level RCC libraries also are specified via RccLibraries.
override RccLibrariesInternal := $(RccLibraries) $(Libraries) $(RccLibrariesInternal)
override RccIncludeDirsInternal := \
  $(foreach l,$(RccLibrariesInternal),$(dir $l)include) \
  $(RccIncludeDirs) $(IncludeDirs) $(RccIncludeDirsInternal)
$(call OcpiDbgVar,RccLibrariesInternal)
$(call OcpiDbgVar,RccIncludeDirsInternal)
ifndef RccTargets
ifdef RccTarget
RccTargets:=$(RccTarget)
else
RccTargets:=$(OCPI_TARGET_HOST)
endif
endif
ifndef RccTarget
RccTarget:=$(firstword $(RccTargets))
endif

$(call OcpiDbgVar,RccTargets)

RccOs=$(word 1,$(subst -, ,$(or $1,$(RccTarget))))
RccOsVersion=$(word 2,$(subst -, ,$1))
RccArch=$(word 3,$(subst -, ,$1))
OcpiTargetSubdir=$(and $(OCPI_TARGET_MODE),/$(OCPI_TARGET_MODE))
# Read in all the tool sets indicated by the targets
# 
ifeq ($(filter clean cleanrcc,$(MAKECMDGOALS)),)

$(foreach t,$(RccTargets),\
  $(foreach m,$(if $(findstring $(OCPI_TOOL_HOST),$t),$t,$(OCPI_TOOL_HOST)=$t),\
    $(if $(or $(wildcard $(OCPI_CDK_DIR)/include/rcc/$m.mk),$(strip \
              $(wildcard $(OCPI_CDK_DIR)/platforms/$(OCPI_TARGET_PLATFORM)/$m.mk))),\
       $(eval include $(OCPI_CDK_DIR)/include/rcc/$m.mk),\
       $(if $(findstring =,$m),\
         $(error There is no cross compiler defined from $(OCPI_TOOL_HOST) to $t),\
         $(eval include $(OCPI_CDK_DIR)/include/rcc/default.mk)))))

endif
endif
